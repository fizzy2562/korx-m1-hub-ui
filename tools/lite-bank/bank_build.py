import sys, os, json, struct, zlib
from array import array

ENTRY_FMT = '<IIIBBHII'
assert struct.calcsize(ENTRY_FMT) == 24

def get_codec_for_region(start, end, members):
    length = end - start + 1
    if length < 64:
        return "raw16"
    
    # Check if any member is a raw16-forced region
    # The prompt says: "or if any member is a raw16-forced region of the same kind"
    # In the context of the merge logic, "same kind" implies we are looking at a group of regions that were merged together.
    # If the group contains a raw16-forced region, and we are determining the codec for this merged group,
    # the instruction implies that if ANY member is raw16-forced, the result should be raw16?
    # Let's re-read carefully: "raw16 if the merged range is shorter than 64 words or if any member is a raw16-forced region of the same kind"
    # This phrasing is slightly ambiguous. Does "same kind" refer to the kind of the region being processed?
    # Looking at the merge step: "Merge only among regions of the same kind (raw16_forced vs others)".
    # So a merged group is either ALL raw16_forced or ALL non-raw16_forced.
    # If the group is raw16_forced, the codec is obviously raw16 (handled in the caller: `if mr["is_raw16_forced"]: codec_name = "raw16"`).
    # So `get_codec_for_region` is only called for non-raw16_forced groups.
    # Therefore, in a non-raw16_forced group, NO member is a raw16-forced region.
    # Wait, let's look at the caller again.
    # `if mr["is_raw16_forced"]: codec_name = "raw16" else: codec_name = get_codec_for_region(...)`
    # So `get_codec_for_region` is ONLY called when `mr["is_raw16_forced"]` is False.
    # This means all members in `members` have `is_raw16_forced` == False.
    # So the condition "if any member is a raw16-forced region" will never be true inside this function if it's only called for non-forced groups.
    # However, the prompt explicitly says: "raw16 if ... or if any member is a raw16-forced region of the same kind".
    # Maybe the prompt implies that the function should check this condition regardless, or maybe my interpretation of the caller is too strict?
    # Let's look at the code structure.
    # In Step 3, merging happens based on `is_raw16_forced`.
    # So a merged region `mr` has a consistent `is_raw16_forced` flag for all its members.
    # If `mr["is_raw16_forced"]` is True, the caller handles it.
    # If `mr["is_raw16_forced"]` is False, all members are False.
    # So the condition "any member is raw16-forced" is redundant if the caller filters correctly.
    # BUT, the prompt says "define ... with the semantics stated in its call and comments".
    # The comment in the code says: "The original code: `codec_name = get_codec_for_region(start, end, mr["members"])`."
    # And the prompt says: "raw16 if the merged range is shorter than 64 words or if any member is a raw16-forced region of the same kind".
    # Perhaps "same kind" refers to the fact that we are in a context where raw16 is a valid option?
    # Or maybe I should just implement the logic as stated:
    # 1. If length < 64, return "raw16".
    # 2. If any member has `is_raw16_forced` == True, return "raw16".
    # 3. Otherwise, return the codec of the largest member by word count.
    
    # Let's implement exactly that.
    for m in members:
        if m.get("is_raw16_forced", False):
            return "raw16"
            
    # Find the largest member by word count
    largest_member = max(members, key=lambda x: x["end"] - x["start"] + 1)
    return largest_member["codec"]

def main():
    if len(sys.argv) < 5 or len(sys.argv) > 7:
        print("Usage: bank_build.py <dir containing compress_study.py> <plan.json> <pcm.bin> <out.bin> [guard_words [guard_max_len]]")
        sys.exit(1)
    
    sys.path.insert(0, sys.argv[1])
    try:
        import compress_study
    except ImportError:
        print("Error: Could not import compress_study.py from the specified directory.")
        sys.exit(1)

    plan_path = sys.argv[2]
    pcm_path = sys.argv[3]
    out_path = sys.argv[4]
    
    guard_words = 0
    guard_max_len = 2048
    if len(sys.argv) >= 6:
        try:
            guard_words = int(sys.argv[5])
        except ValueError:
            print("Error: guard_words must be an integer")
            sys.exit(1)
    if len(sys.argv) >= 7:
        try:
            guard_max_len = int(sys.argv[6])
        except ValueError:
            print("Error: guard_max_len must be an integer")
            sys.exit(1)

    with open(plan_path, 'r') as f:
        plan = json.load(f)

    regions = plan.get("regions", [])
    if not regions:
        print("No regions found in plan.")
        sys.exit(1)

    # Load PCM
    with open(pcm_path, 'rb') as f:
        pcm_data = f.read()
    pcm = array('h')
    pcm.frombytes(pcm_data)
    total_words = len(pcm)

    # Codec mapping
    codec_map = {
        "raw16": 0,
        "ima4": 1,
        "adpcm3": 2,
        "adpcm2": 3
    }
    
    encoders = {
        "raw16": None, # handled manually
        "ima4": compress_study.encode_ima4,
        "adpcm3": compress_study.encode_adpcm3,
        "adpcm2": compress_study.encode_adpcm2
    }

    # Prepare region entries
    entries = []
    mismatches = []
    codec_bytes = {0: 0, 1: 0, 2: 0, 3: 0}
    
    # Track stats for guard mode
    widened_count = 0
    merged_count = 0
    original_region_count = len(regions)
    extended_count = 0
    extended_extra_words = 0
    raw16_forced_count = 0
    trimmed_count = 0

    if guard_words > 0:
        # Step 1: Apply Defect A (Extension) and Defect B (Codec Force) BEFORE widening/merging
        processed_regions = []
        for r in regions:
            start = r["start"]
            end = r["end"]
            loop = r["loop"]
            loop_on = 1 if r["loop_on"] else 0
            codec_name = r["codec"]
            
            is_raw16_forced = False
            new_start = start
            
            # Defect A: If loop_on and loop < start, extend start to loop
            if loop_on and loop < start:
                new_start = loop
                extended_count += 1
                extended_extra_words += (start - loop)
                # Defect B: Force raw16 codec
                is_raw16_forced = True
                codec_name = "raw16"
                raw16_forced_count += 1
            
            processed_regions.append({
                "start": new_start,
                "end": end,
                "loop": loop,
                "loop_on": loop_on,
                "codec": codec_name,
                "bytes": r["bytes"],
                "is_raw16_forced": is_raw16_forced
            })

        # Step 2: Widen regions (Guard widening)
        widened_regions = []
        for r in processed_regions:
            start = r["start"]
            end = r["end"]
            length = end - start + 1
            
            if length < guard_max_len:
                new_start = max(0, start - guard_words)
                new_end = min(2097151, end + guard_words)
                # Clamp to actual PCM bounds to avoid index errors later
                new_end = min(new_end, total_words - 1)
                
                if new_start != start or new_end != end:
                    widened_count += 1
                
                widened_regions.append({
                    "start": new_start,
                    "end": new_end,
                    "loop": r["loop"],
                    "loop_on": r["loop_on"],
                    "codec": r["codec"],
                    "bytes": r["bytes"],
                    "is_raw16_forced": r["is_raw16_forced"],
                    "original_start": start,
                    "original_end": end
                })
            else:
                widened_regions.append({
                    "start": start,
                    "end": end,
                    "loop": r["loop"],
                    "loop_on": r["loop_on"],
                    "codec": r["codec"],
                    "bytes": r["bytes"],
                    "is_raw16_forced": r["is_raw16_forced"],
                    "original_start": start,
                    "original_end": end
                })
        
        # Sort by start
        widened_regions.sort(key=lambda x: x["start"])
        
        # Step 3: Merge overlapping/touching regions
        # Rule: Merge only among regions of the same kind (raw16_forced vs others)
        merged_regions = []
        if widened_regions:
            current = dict(widened_regions[0])
            current["members"] = [widened_regions[0]]
            
            for i in range(1, len(widened_regions)):
                next_r = widened_regions[i]
                
                # Check if next.start <= current.end + 1 AND same kind
                if next_r["start"] <= current["end"] + 1 and next_r["is_raw16_forced"] == current["is_raw16_forced"]:
                    # Merge
                    current["end"] = max(current["end"], next_r["end"])
                    current["members"].append(next_r)
                    merged_count += 1
                else:
                    merged_regions.append(current)
                    current = dict(next_r)
                    current["members"] = [next_r]
            
            merged_regions.append(current)
        
        # Step 4: Resolve overlaps between raw16-forced and non-raw16 regions
        # Identify raw16-forced regions (they are now separate merged groups)
        raw_regions = [mr for mr in merged_regions if mr["is_raw16_forced"]]
        non_raw_regions = [mr for mr in merged_regions if not mr["is_raw16_forced"]]
        
        # We need to trim non-raw regions so they don't overlap raw regions.
        # A non-raw region [s, e] might overlap with one or more raw regions [rs, re].
        # We split the non-raw region into pieces that do not overlap any raw region.
        
        final_regions = []
        
        # Add all raw regions as is (they are lossless and take priority)
        for mr in raw_regions:
            final_regions.append(mr)
            
        # Process non-raw regions
        for mr in non_raw_regions:
            s = mr["start"]
            e = mr["end"]
            
            # Find all raw regions that overlap with [s, e]
            # Overlap condition: raw.start <= e AND raw.end >= s
            overlapping_raws = [r for r in raw_regions if r["start"] <= e and r["end"] >= s]
            
            if not overlapping_raws:
                # No overlap, keep as is
                final_regions.append(mr)
                continue
            
            # We need to cut out the overlapped parts.
            # The overlapped parts are the union of intersections with raw regions.
            # Let's find the valid segments of [s, e] that are NOT in any raw region.
            
            # Create a list of forbidden intervals (intersections)
            forbidden = []
            for r in overlapping_raws:
                # Intersection of [s, e] and [r.start, r.end]
                isect_start = max(s, r["start"])
                isect_end = min(e, r["end"])
                if isect_start <= isect_end:
                    forbidden.append((isect_start, isect_end))
            
            # Sort forbidden intervals by start
            forbidden.sort(key=lambda x: x[0])
            
            # Merge forbidden intervals to simplify
            merged_forbidden = []
            if forbidden:
                cur_f = list(forbidden[0])
                for f in forbidden[1:]:
                    if f[0] <= cur_f[1] + 1:
                        cur_f[1] = max(cur_f[1], f[1])
                    else:
                        merged_forbidden.append(tuple(cur_f))
                        cur_f = list(f)
                merged_forbidden.append(tuple(cur_f))
            
            # Now, carve out valid segments from [s, e] excluding merged_forbidden
            valid_segments = []
            current_pos = s
            
            for fs, fe in merged_forbidden:
                if fs > current_pos:
                    # Segment [current_pos, fs - 1] is valid
                    seg_start = current_pos
                    seg_end = fs - 1
                    if seg_end - seg_start + 1 >= 4: # Discard pieces shorter than 4 words
                        valid_segments.append((seg_start, seg_end))
                
                # Skip the forbidden part
                if fe >= current_pos:
                    current_pos = fe + 1
            
            # Check remaining tail
            if current_pos <= e:
                seg_start = current_pos
                seg_end = e
                if seg_end - seg_start + 1 >= 4:
                    valid_segments.append((seg_start, seg_end))
            
            # Create new region entries for each valid segment
            for vs, ve in valid_segments:
                # We need to preserve the metadata from the original non-raw region
                # But since we are splitting, we create new entries
                new_mr = dict(mr)
                new_mr["start"] = vs
                new_mr["end"] = ve
                # The members list should ideally be updated, but for encoding we just need start/end/codec
                # The codec is determined by the original region's logic (which was non-raw)
                # We keep the original codec name
                final_regions.append(new_mr)
                trimmed_count += 1
        
        # Step 5: Process final regions (encode and add to entries)
        # Sort final regions by start address
        final_regions.sort(key=lambda x: x["start"])
        
        for mr in final_regions:
            start = mr["start"]
            end = mr["end"]
            
            # Determine codec
            # If it was raw16 forced, it's raw16.
            # If it was non-raw, we use the logic from get_codec_for_region or the original codec?
            # The prompt says: "other regions with each other". The original logic used get_codec_for_region.
            # However, for trimmed pieces, the length might be small.
            # Let's stick to the original logic: if length < 64, raw16. Else, use the member's codec.
            # Since we split, we don't have a single "member" in the same way, but we can use the codec from the original merged region.
            # Actually, the original code used `get_codec_for_region` which picks the codec of the largest member.
            # For simplicity and consistency with "keep everything else exactly as it is", 
            # we will use the codec stored in the region dict (which was set during Defect B or original plan).
            # Wait, for non-raw regions, the codec was the original plan's codec.
            # If a non-raw region was merged, the codec was determined by `get_codec_for_region`.
            # In my implementation above, I kept `mr["codec"]` from the initial processing.
            # For merged non-raw regions, I should have determined the codec during merge or kept the dominant one.
            # Let's refine: In the merge step, I didn't update the codec.
            # Let's assume the codec is the one from the first member or the dominant one.
            # The original code: `codec_name = get_codec_for_region(start, end, mr["members"])`.
            # I will apply this logic here for non-raw regions.
            
            if mr["is_raw16_forced"]:
                codec_name = "raw16"
            else:
                # Use the original logic for non-raw
                codec_name = get_codec_for_region(start, end, mr["members"])
            
            if codec_name not in codec_map:
                print(f"Warning: Unknown codec {codec_name} in region {start}-{end}, skipping.")
                continue

            codec_id = codec_map[codec_name]
            
            # Extract samples
            if start < 0 or end >= total_words:
                print(f"Warning: Region {start}-{end} out of bounds (total {total_words}), skipping.")
                continue
                
            samples = pcm[start:end+1]
            
            # Encode
            if codec_name == "raw16":
                encoded = samples.tobytes()
            else:
                enc_func = encoders[codec_name]
                encoded, _ = enc_func(samples)
            
            codec_bytes[codec_id] += len(encoded)
            
            # Loop value: loop of the member with the smallest start
            min_start_member = min(mr["members"], key=lambda x: x["start"])
            loop = min_start_member["loop"]
            loop_on = min_start_member["loop_on"]
            
            entries.append({
                "start": start,
                "end": end,
                "loop": loop,
                "loop_on": loop_on,
                "codec_id": codec_id,
                "data": encoded
            })
        
        # Print guard mode stats
        # Calculate total words in bank
        bank_words = sum(e['end'] - e['start'] + 1 for e in entries)
        plan_words = sum(r['end'] - r['start'] + 1 for r in regions)
        
        print(f"guard mode: {original_region_count} plan regions -> {len(entries)} bank regions, +{bank_words - plan_words} words, bank size {0} bytes")
        print(f"  widened regions: {widened_count}")
        print(f"  merged regions: {merged_count}")
        print(f"  extended backwards: {extended_count} regions (+{extended_extra_words} words)")
        print(f"  raw16-forced regions: {raw16_forced_count}")
        print(f"  trimmed regions: {trimmed_count}")

    else:
        # Original behavior
        for r in regions:
            start = r["start"]
            end = r["end"]
            loop = r["loop"]
            loop_on = 1 if r["loop_on"] else 0
            codec_name = r["codec"]
            declared_bytes = r["bytes"]
            
            if codec_name not in codec_map:
                print(f"Warning: Unknown codec {codec_name} in region {start}-{end}, skipping.")
                continue

            codec_id = codec_map[codec_name]
            
            # Extract samples
            if start < 0 or end >= total_words:
                print(f"Warning: Region {start}-{end} out of bounds (total {total_words}), skipping.")
                continue
                
            samples = pcm[start:end+1]
            
            # Encode
            if codec_name == "raw16":
                encoded = samples.tobytes()
            else:
                enc_func = encoders[codec_name]
                encoded, _ = enc_func(samples)
                
            # Verify bytes
            if len(encoded) != declared_bytes:
                mismatches.append((start, end, codec_name, len(encoded), declared_bytes))
                
            codec_bytes[codec_id] += len(encoded)
            
            entries.append({
                "start": start,
                "end": end,
                "loop": loop,
                "loop_on": loop_on,
                "codec_id": codec_id,
                "data": encoded
            })

    # Sort by start address
    entries.sort(key=lambda x: x["start"])

    # Calculate offsets
    # Header: 16 bytes
    # Table: 24 bytes per entry
    header_size = 16
    table_size = 24 * len(entries)
    data_start_offset = header_size + table_size
    
    current_offset = data_start_offset
    total_data_bytes = 0
    for e in entries:
        e["offset"] = current_offset
        e["length"] = len(e["data"])
        # Pad to multiple of 4
        padded_len = (e["length"] + 3) & ~3
        current_offset += padded_len
        total_data_bytes += e["length"]
        
    total_file_size = current_offset

    # Build Header
    # magic b'KLB1', u16 region count, u16 version = 1, u32 total file size, u32 CRC32 of everything after the header
    
    # Build Table
    table_bytes = bytearray()
    for e in entries:
        table_bytes += struct.pack(ENTRY_FMT, 
            e["start"], 
            e["end"], 
            e["loop"], 
            e["loop_on"], 
            e["codec_id"], 
            0, # reserved
            e["offset"], 
            e["length"]
        )
        
    # Build Data Blobs
    data_bytes = bytearray()
    for e in entries:
        data_bytes += e["data"]
        # Pad to 4
        pad_len = (4 - (len(e["data"]) % 4)) % 4
        if pad_len:
            data_bytes += b'\x00' * pad_len

    # Calculate CRC32 of everything after header
    # Everything after header = table_bytes + data_bytes
    crc_body = zlib.crc32(bytes(table_bytes) + bytes(data_bytes)) & 0xFFFFFFFF

    # Build Header
    header = bytearray()
    header += b'KLB1'
    header += struct.pack('<H', len(entries))
    header += struct.pack('<H', 1) # version
    header += struct.pack('<I', total_file_size)
    header += struct.pack('<I', crc_body)

    # Write File
    with open(out_path, 'wb') as f:
        f.write(bytes(header))
        f.write(bytes(table_bytes))
        f.write(bytes(data_bytes))

    # Print Stats
    print(f"Region count: {len(entries)}")
    print(f"Total bank size: {total_file_size} bytes")
    print(f"Data bytes: {total_data_bytes}")
    print(f"Table bytes: {table_size}")
    
    codec_names = {0: "raw16", 1: "ima4", 2: "adpcm3", 3: "adpcm2"}
    print("Per-codec byte totals:")
    for cid in sorted(codec_bytes.keys()):
        print(f"  {codec_names[cid]}: {codec_bytes[cid]} bytes")

    if guard_words == 0:
        print(f"Mismatches: {len(mismatches)}")
        if mismatches:
            print("First 5 mismatches:")
            for m in mismatches[:5]:
                print(f"  Region {m[0]}-{m[1]} ({m[2]}): actual {m[3]}, declared {m[4]}")

if __name__ == "__main__":
    main()

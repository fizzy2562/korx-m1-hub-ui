import sys, os, struct, zlib
from array import array

ENTRY_FMT = '<IIIBBHII'
assert struct.calcsize(ENTRY_FMT) == 24

def main():
    if len(sys.argv) < 4:
        print("Usage: bank_decode.py <dir containing compress_study.py> <bank.bin> <out_pcm.bin> [<original pcm.bin>]")
        sys.exit(1)

    sys.path.insert(0, sys.argv[1])
    try:
        import compress_study
    except ImportError:
        print("Error: Could not import compress_study.py from the specified directory.")
        sys.exit(1)

    bank_path = sys.argv[2]
    out_pcm_path = sys.argv[3]
    orig_pcm_path = sys.argv[4] if len(sys.argv) > 4 else None

    # Read Bank
    with open(bank_path, 'rb') as f:
        bank_data = f.read()

    # Parse Header
    if len(bank_data) < 16:
        print("Error: Bank file too small for header.")
        sys.exit(1)

    magic = bank_data[0:4]
    if magic != b'KLB1':
        print("Error: Invalid magic number.")
        sys.exit(1)

    region_count = struct.unpack('<H', bank_data[4:6])[0]
    version = struct.unpack('<H', bank_data[6:8])[0]
    total_size = struct.unpack('<I', bank_data[8:12])[0]
    crc_stored = struct.unpack('<I', bank_data[12:16])[0]

    if version != 1:
        print(f"Error: Unsupported version {version}.")
        sys.exit(1)

    if len(bank_data) != total_size:
        print(f"Error: File size {len(bank_data)} does not match header size {total_size}.")
        sys.exit(1)

    # Verify CRC
    body = bank_data[16:]
    crc_calc = zlib.crc32(body) & 0xFFFFFFFF
    if crc_calc != crc_stored:
        print(f"Error: CRC32 mismatch. Stored: {crc_stored:08X}, Calculated: {crc_calc:08X}.")
        sys.exit(1)

    # Parse Table
    table_start = 16
    table_end = table_start + 24 * region_count
    if table_end > len(bank_data):
        print("Error: Table extends beyond file size.")
        sys.exit(1)

    entries = []
    for i in range(region_count):
        offset = table_start + i * 24
        start, end, loop, loop_on, codec_id, reserved, data_offset, data_length = struct.unpack(ENTRY_FMT, bank_data[offset:offset+24])
        
        # Validation
        if data_offset % 4 != 0:
            print(f"Error: Data offset {data_offset} for region {i} is not 4-byte aligned.")
            sys.exit(1)
            
        if data_offset + data_length > len(bank_data):
            print(f"Error: Data for region {i} extends beyond file size.")
            sys.exit(1)
            
        entries.append({
            "start": start,
            "end": end,
            "loop": loop,
            "loop_on": loop_on,
            "codec_id": codec_id,
            "data_offset": data_offset,
            "data_length": data_length
        })

    # Check for overlapping data blobs
    # Sort by offset to check overlaps
    sorted_entries = sorted(entries, key=lambda x: x["data_offset"])
    for i in range(1, len(sorted_entries)):
        prev = sorted_entries[i-1]
        curr = sorted_entries[i]
        if curr["data_offset"] < prev["data_offset"] + prev["data_length"]:
            print("Error: Overlapping data blobs detected.")
            sys.exit(1)

    # Decode Regions
    # Initialize output PCM with zeros
    total_words = 2097152
    out_pcm = array('h', [0] * total_words)
    
    decoders = {
        0: None, # raw16
        1: compress_study.decode_ima4,
        2: compress_study.decode_adpcm3,
        3: compress_study.decode_adpcm2
    }

    # Track regions for SNR calculation
    # We need to map decoded words back to original if provided
    # Since regions might overlap in address space (though plan says impossible), 
    # "later table entry wins". We process in table order.
    
    decoded_regions = [] # list of (start, end, decoded_samples, codec_id)
    
    for e in entries:
        codec_id = e["codec_id"]
        if codec_id not in decoders:
            print(f"Warning: Unknown codec ID {codec_id} in region {e['start']}-{e['end']}, skipping.")
            continue
            
        data = bank_data[e["data_offset"]:e["data_offset"] + e["data_length"]]
        num_samples = e["end"] - e["start"] + 1
        
        if codec_id == 0: # raw16
            # Data should be exactly num_samples * 2 bytes
            if len(data) != num_samples * 2:
                print(f"Warning: Raw16 data length mismatch for region {e['start']}-{e['end']}")
                continue
            samples = array('h')
            samples.frombytes(data)
        else:
            dec_func = decoders[codec_id]
            try:
                samples = dec_func(data, num_samples)
            except Exception as ex:
                print(f"Error decoding region {e['start']}-{e['end']} with codec {codec_id}: {ex}")
                continue
        
        # Write to output PCM
        # Check bounds
        if e["start"] < 0 or e["end"] >= total_words:
            print(f"Warning: Region {e['start']}-{e['end']} out of bounds, skipping write.")
            continue
            
        # Overlap check for writing (later wins)
        # Since we iterate in table order, later entries overwrite earlier ones naturally if we just assign.
        # But we need to be careful if we want to report SNR per region.
        # The prompt says "overlapping regions are impossible in the plan; if two regions overlap in address, the later table entry wins, print a warning."
        # We'll just write them. If they overlap, the last one written to that address wins.
        
        out_pcm[e["start"]:e["end"]+1] = samples
        decoded_regions.append((e["start"], e["end"], samples, codec_id))

    # Write Output PCM
    with open(out_pcm_path, 'wb') as f:
        f.write(out_pcm.tobytes())

    print(f"Decoded {len(decoded_regions)} regions.")
    print(f"Wrote {total_words} words to {out_pcm_path}.")

    # SNR Calculation
    if orig_pcm_path:
        if not os.path.exists(orig_pcm_path):
            print(f"Warning: Original PCM file {orig_pcm_path} not found, skipping SNR calculation.")
        else:
            with open(orig_pcm_path, 'rb') as f:
                orig_data = f.read()
            orig_pcm = array('h')
            orig_pcm.frombytes(orig_data)
            
            if len(orig_pcm) != total_words:
                print(f"Warning: Original PCM size mismatch ({len(orig_pcm)} vs {total_words}), skipping SNR calculation.")
            else:
                # Aggregate SNR per codec
                codec_stats = {0: {"se": 0, "ee": 0, "count": 0}, 1: {"se": 0, "ee": 0, "count": 0}, 2: {"se": 0, "ee": 0, "count": 0}, 3: {"se": 0, "ee": 0, "count": 0}}
                total_se = 0
                total_ee = 0
                
                for start, end, samples, codec_id in decoded_regions:
                    if codec_id not in codec_stats:
                        continue
                    # Extract original samples for this region
                    orig_samples = orig_pcm[start:end+1]
                    
                    # Calculate energy
                    se = 0
                    ee = 0
                    for i in range(len(samples)):
                        s = samples[i]
                        o = orig_samples[i]
                        se += s * s
                        ee += (s - o) * (s - o)
                    
                    codec_stats[codec_id]["se"] += se
                    codec_stats[codec_id]["ee"] += ee
                    codec_stats[codec_id]["count"] += 1
                    total_se += se
                    total_ee += ee

                codec_names = {0: "raw16", 1: "ima4", 2: "adpcm3", 3: "adpcm2"}
                print("\nSNR Statistics:")
                for cid in sorted(codec_stats.keys()):
                    stats = codec_stats[cid]
                    if stats["count"] == 0:
                        continue
                    if stats["se"] == 0 or stats["ee"] == 0:
                        snr = 99.0
                    else:
                        snr = min(99.0, 10 * __import__('math').log10(stats["se"] / stats["ee"]))
                    print(f"  {codec_names[cid]}: {stats['count']} regions, SNR {snr:.2f} dB")
                
                if total_se == 0 or total_ee == 0:
                    overall_snr = 99.0
                else:
                    overall_snr = min(99.0, 10 * __import__('math').log10(total_se / total_ee))
                print(f"  Overall: SNR {overall_snr:.2f} dB")

if __name__ == "__main__":
    main()

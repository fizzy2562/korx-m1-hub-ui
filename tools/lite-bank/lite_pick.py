import sys
import os
import re
import csv
import json
import math
from collections import defaultdict

# MUST-KEEP list (exact match on program name after rstrip)
MUST_KEEP = [
    "Universe", "Piano16'", "Piano8'", "MagicPiano", "E.Piano1",
    "Strings", "StringRise", "Symphonic", "Brass1", "Brass2",
    "Organ2", "A.Bass", "PickBass", "Drums#1", "Drums#2", "Drum#3"
]

def parse_int(s):
    s = s.strip()
    if s.startswith('0x') or s.startswith('0X'):
        return int(s, 16)
    return int(s)

def find_nearest_names(target, all_names, limit=3):
    """Find nearest names using case-insensitive substring match."""
    target_lower = target.lower()
    matches = []
    for name in all_names:
        if not name:
            continue
        name_lower = name.lower()
        # Check if target is substring of name or name is substring of target
        if target_lower in name_lower or name_lower in target_lower:
            matches.append(name)
    # Also try simple prefix/suffix similarity if no direct substring match
    if not matches:
        for name in all_names:
            if not name:
                continue
            name_lower = name.lower()
            # Simple check: do they share a common prefix of length > 2?
            common_len = 0
            for i in range(min(len(target_lower), len(name_lower))):
                if target_lower[i] == name_lower[i]:
                    common_len += 1
                else:
                    break
            if common_len > 2:
                matches.append(name)
    
    # Deduplicate and sort by length (shorter usually means more specific/close)
    unique_matches = list(dict.fromkeys(matches))
    unique_matches.sort(key=lambda x: len(x))
    return unique_matches[:limit]

def main():
    if len(sys.argv) < 6:
        print("Usage: python lite_pick.py <m1_rom_dir> <os_bin_dir> <header_path> <regions_csv> <out_dir> [budgets_kib] [overhead_kib] [mk_mode] [override_list]")
        sys.exit(1)

    rom_dir = sys.argv[1]
    os_bin_dir = sys.argv[2]
    header_path = sys.argv[3]
    regions_csv_path = sys.argv[4]
    out_dir = sys.argv[5]

    # Parse optional 6th argument: budgets
    if len(sys.argv) >= 7:
        budgets_arg = sys.argv[6]
        try:
            budgets_kib = [int(x.strip()) for x in budgets_arg.split(',') if x.strip()]
            if not budgets_kib:
                raise ValueError("Empty budget list")
        except ValueError:
            print("Error: Invalid budget list format. Expected comma-separated integers (e.g., '280,300,320').")
            sys.exit(1)
    else:
        budgets_kib = [340, 620]

    # Parse optional 7th argument: overhead
    if len(sys.argv) >= 8:
        try:
            overhead_kib = int(sys.argv[7])
        except ValueError:
            print("Error: Invalid overhead value. Expected an integer.")
            sys.exit(1)
    else:
        overhead_kib = 6

    # Parse optional 8th argument: must-keep mode
    must_keep_list = MUST_KEEP
    if len(sys.argv) >= 9:
        if sys.argv[8] == "none":
            must_keep_list = []
            print("Must-keep list DISABLED (cheapest-first)")
        # If any other value, behaviour is unchanged (use default MUST_KEEP)
        
        # Parse optional 9th argument: override list
        if len(sys.argv) >= 10:
            override_arg = sys.argv[9]
            if override_arg and sys.argv[8] != "none":
                override_names = [x.strip() for x in override_arg.split(',') if x.strip()]
                if override_names:
                    must_keep_list = override_names
                    print(f"Must-keep list OVERRIDDEN: {', '.join(override_names)}")

    os.makedirs(out_dir, exist_ok=True)

    # 1. Load ROM metadata
    sys.path.insert(0, rom_dir)
    try:
        import m1_rom
    except ImportError as e:
        print(f"Error importing m1_rom: {e}")
        sys.exit(1)

    rs = m1_rom.M1RomSet.__new__(m1_rom.M1RomSet)
    rs.romdir = os_bin_dir
    rs.os = rs._load_os()
    
    try:
        ms = rs.multisounds()
        dr = rs.drum_sounds()
    except Exception as e:
        print(f"Error loading multisounds/drum_sounds: {e}")
        sys.exit(1)

    # 2. Parse C arrays from header
    with open(header_path, 'r') as f:
        header_text = f.read()

    def parse_array(name, text):
        pattern = re.compile(r'k' + name + r'\s*\[\s*\d+\s*\]\s*=\s*\{(.*?)\};', re.DOTALL)
        match = pattern.search(text)
        if not match:
            return []
        content = match.group(1)
        num_pattern = re.compile(r'-?\s*(?:0[xX][0-9a-fA-F]+|\d+)')
        nums = num_pattern.findall(content)
        return [parse_int(n) for n in nums]

    kPrograms = parse_array('Programs', header_text)
    kCombis = parse_array('Combis', header_text)
    kKits = parse_array('Kits', header_text)

    if len(kPrograms) < 14300:
        print(f"Warning: kPrograms length {len(kPrograms)} < 14300")
    if len(kCombis) < 12400:
        print(f"Warning: kCombis length {len(kCombis)} < 12400")
    if len(kKits) < 840:
        print(f"Warning: kKits length {len(kKits)} < 840")

    # 3. Parse regions.csv
    cost = defaultdict(dict)
    region_lengths = {}
    
    with open(regions_csv_path, 'r') as f:
        reader = csv.DictReader(f)
        for row in reader:
            try:
                start = int(row['start'])
                end = int(row['end'])
                length = int(row['length'])
                codec = row['codec'].strip()
                bytes_val = int(row['bytes'])
                
                key = (start, end)
                cost[codec][key] = bytes_val
                region_lengths[key] = length
            except (ValueError, KeyError) as e:
                continue

    # 4. Determine regions needed for each program
    program_regions = [set() for _ in range(100)]
    program_names = [''] * 100
    program_is_drum = [False] * 100

    for n in range(100):
        p = kPrograms[143*n : 143*n+143]
        if len(p) < 143:
            p = p + [0] * (143 - len(p))
        
        name_bytes = p[0:10]
        name = ''.join(chr(b) for b in name_bytes).rstrip()
        program_names[n] = name
        
        m = p[10]
        
        if m == 2:
            program_is_drum[n] = True
            kit = p[12] & 3
            for i in range(30):
                ins_offset = 210 * kit + 7 * i
                ins = kKits[ins_offset : ins_offset + 7]
                if len(ins) < 7:
                    ins = ins + [0] * (7 - len(ins))
                if ins[0] != 0 and ins[0] <= 44:
                    drum_num = ins[0]
                    if drum_num - 1 < len(dr):
                        d = dr[drum_num - 1]
                        key = (d['start'], d['end'])
                        program_regions[n].add(key)
        else:
            ms_idx1 = p[12]
            if ms_idx1 < 0 or ms_idx1 > 99:
                ms_idx1 = 0 if ms_idx1 < 0 else 99
            if ms_idx1 < len(ms):
                ms_data = ms[ms_idx1]
                splits = ms_data.get('splits', [])
                for split in splits:
                    key = (split['start'], split['end'])
                    program_regions[n].add(key)
            
            if m == 1:
                ms_idx2 = p[14]
                if ms_idx2 < 0 or ms_idx2 > 99:
                    ms_idx2 = 0 if ms_idx2 < 0 else 99
                if ms_idx2 < len(ms):
                    ms_data = ms[ms_idx2]
                    splits = ms_data.get('splits', [])
                    for split in splits:
                        key = (split['start'], split['end'])
                        program_regions[n].add(key)

    # 5. Determine programs needed for each combination
    combi_programs = [set() for _ in range(100)]
    combi_names = [''] * 100
    
    for k in range(100):
        c = kCombis[124*k : 124*k+124]
        if len(c) < 124:
            c = c + [0] * (124 - len(c))
        
        # Name: first 10 bytes
        name_bytes = c[0:10]
        combi_names[k] = ''.join(chr(b) for b in name_bytes).rstrip()

        for t in range(8):
            r_offset = 36 + 11 * t
            r = c[r_offset : r_offset + 11]
            if len(r) < 11:
                r = r + [0] * (11 - len(r))
            
            if (r[10] & 0x10) != 0:
                continue
            
            prog_num = r[0]
            if prog_num < 100:
                combi_programs[k].add(prog_num)

    # 6. Define Cost Strategies
    # First, build region_meta for all regions to support the new cost model
    region_meta = {}
    
    # From multisounds
    for ms_data in ms:
        splits = ms_data.get('splits', [])
        for split in splits:
            key = (split['start'], split['end'])
            loop_val = split.get('loop', 0)
            loop_on_val = split.get('loop_on', False)
            if key not in region_meta:
                region_meta[key] = {
                    'loop': int(loop_val),
                    'loop_on': bool(loop_on_val)
                }
            else:
                # Keep entry with loop_on true and smallest loop
                existing = region_meta[key]
                if bool(loop_on_val) and not existing['loop_on']:
                    region_meta[key] = {
                        'loop': int(loop_val),
                        'loop_on': bool(loop_on_val)
                    }
                elif bool(loop_on_val) == existing['loop_on'] and int(loop_val) < existing['loop']:
                    region_meta[key] = {
                        'loop': int(loop_val),
                        'loop_on': bool(loop_on_val)
                    }
    
    # From drum sounds
    for d in dr:
        key = (d['start'], d['end'])
        loop_val = d.get('loop', 0)
        loop_on_val = d.get('loop_on', False)
        if key not in region_meta:
            region_meta[key] = {
                'loop': int(loop_val),
                'loop_on': bool(loop_on_val)
            }
        else:
            # Keep entry with loop_on true and smallest loop
            existing = region_meta[key]
            if bool(loop_on_val) and not existing['loop_on']:
                region_meta[key] = {
                    'loop': int(loop_val),
                    'loop_on': bool(loop_on_val)
                }
            elif bool(loop_on_val) == existing['loop_on'] and int(loop_val) < existing['loop']:
                region_meta[key] = {
                    'loop': int(loop_val),
                    'loop_on': bool(loop_on_val)
                }

    # Sanity check output about loop lookbacks
    extended_raw_count = 0
    extended_raw_total_bytes = 0
    loop_lookback_count = 0
    
    for key in region_meta:
        start, end = key
        meta = region_meta[key]
        loop = meta['loop']
        loop_on = meta['loop_on']
        
        if loop_on and loop < start:
            extended_raw_count += 1
            # Cost is 2 * (end - loop + 1)
            extended_raw_total_bytes += 2 * (end - loop + 1)
            loop_lookback_count += 1

    print(f"Extended-raw rule applied to {extended_raw_count} distinct regions.")
    print(f"Total extended-raw bytes: {extended_raw_total_bytes}")
    print(f"Sanity check: {loop_lookback_count} regions have loop lookbacks (loop < start and loop_on).")

    def get_base_cost(strategy, key):
        """Get the base cost from regions.csv logic (hyb logic)."""
        if strategy == 'hyb4':
            length = region_lengths.get(key, 0)
            if length < 64:
                return 2 * length
            else:
                return cost['ima4'].get(key, 0)
        elif strategy == 'hyb3':
            length = region_lengths.get(key, 0)
            if length < 64:
                return 2 * length
            else:
                return cost['adpcm3'].get(key, 0)
        elif strategy == 'hyb2':
            length = region_lengths.get(key, 0)
            if length < 64:
                return 2 * length
            else:
                return cost['adpcm2'].get(key, 0)
        else:
            return 0

    def get_final_cost(strategy, key):
        """
        Calculate the final cost for a region according to the new rules:
        (a) EXTENDED RAW rule: if loop_on is true and loop < start, cost = 2 * (end - loop + 1)
        (b) otherwise cost is existing codec cost
        (c) GUARD allowance added if stored length < 2048 words
        """
        start, end = key
        meta = region_meta.get(key, {'loop': 0, 'loop_on': False})
        loop = meta['loop']
        loop_on = meta['loop_on']
        
        # Determine if extended raw rule applies
        is_extended_raw = loop_on and (loop < start)
        
        if is_extended_raw:
            # (a) EXTENDED RAW rule
            base_cost = 2 * (end - loop + 1)
            # Stored length for guard check: end - min(start, loop) + 1
            # Since loop < start, min(start, loop) is loop
            stored_length_words = end - loop + 1
            is_raw_region = True
        else:
            # (b) otherwise existing codec cost
            base_cost = get_base_cost(strategy, key)
            # Stored length for guard check: end - min(start, loop if loop_on else start) + 1
            # If not extended raw, we use start as the effective start for length calculation
            # The prompt says: "stored length (end - min(start, loop if loop_on else start) + 1)"
            # If loop_on is true but loop >= start, min(start, loop) is start.
            # If loop_on is false, min(start, start) is start.
            # So effectively, if not extended raw, stored_length = end - start + 1
            stored_length_words = end - start + 1
            is_raw_region = False # It's a codec region (or raw16 from hyb logic, but treated as codec for guard rate)
        
        # (c) GUARD allowance
        guard_cost = 0
        if stored_length_words < 2048:
            if is_extended_raw:
                # for a raw16 region guard_cost = 256 bytes
                guard_cost = 256
            else:
                # for a codec region guard_cost = round(128 * cost_bytes_without_guard / stored_length_in_words)
                if stored_length_words > 0 and base_cost > 0:
                    guard_cost = round(128 * base_cost / stored_length_words)
                else:
                    guard_cost = 0
        
        return base_cost + guard_cost

    strategies = ['hyb4', 'hyb3', 'hyb2']
    
    # Precompute region costs for each strategy
    strategy_region_cost = {}
    all_regions_set = set()
    for regs in program_regions:
        all_regions_set.update(regs)
        
    for s in strategies:
        costs = {}
        for key in all_regions_set:
            costs[key] = get_final_cost(s, key)
        strategy_region_cost[s] = costs

    # 7. Identify Must-Keep programs
    must_keep_indices = []
    all_names_list = [n for n in program_names if n]
    
    for name in must_keep_list:
        found = False
        for i in range(100):
            if program_names[i] == name:
                must_keep_indices.append(i)
                found = True
                break
        if not found:
            nearest = find_nearest_names(name, all_names_list)
            print(f"Warning: Must-keep program '{name}' not found. Nearest matches: {nearest}")

    # 8. Selection Algorithm
    def select_programs(budget_bytes, strategy):
        # Metadata overhead: passed via closure or global? 
        # The prompt says: "the selection budget = budget_KiB*1024 - overhead_KiB*1024 as before."
        # And "Optional 7th argument argv[7]: the budget-overhead in KiB (default 6, the current constant)"
        # So we use the global overhead_kib variable defined in main scope.
        METADATA_OVERHEAD = overhead_kib * 1024
        effective_budget = budget_bytes - METADATA_OVERHEAD
        
        if effective_budget < 0:
            effective_budget = 0

        kept = [False] * 100
        paid_regions = set()
        total_cost = 0
        dropped_must_keeps = []

        # Step 1: Pay for must-keep programs
        # We need to handle the case where must-keeps exceed budget.
        # Strategy: Add all must-keeps. If over budget, remove the one with largest incremental cost.
        
        # First, let's calculate the cost if we keep all must-keeps.
        # We need to be careful about shared regions.
        
        # Let's simulate adding them one by one to determine incremental costs, 
        # but the requirement says: "drop must-keep programs one at a time, always the one with the largest INCREMENTAL cost first"
        # Incremental cost is defined as "cost of its regions not shared with the other still-kept must-keeps".
        
        # This implies we should start with the full set of must-keeps, calculate the total cost, 
        # and if it's too high, iteratively remove the one that saves the most bytes (largest incremental cost relative to the rest).
        
        # Let's build the set of must-keep indices that we *want* to keep initially.
        candidate_must_keeps = list(must_keep_indices)
        
        # Calculate total cost of all candidate must-keeps
        def calc_cost_for_indices(indices):
            regions = set()
            for idx in indices:
                regions.update(program_regions[idx])
            c = 0
            for key in regions:
                c += strategy_region_cost[strategy].get(key, 0)
            return c

        current_must_keeps = list(candidate_must_keeps)
        
        # Check if it fits
        while current_must_keeps:
            total_mk_cost = calc_cost_for_indices(current_must_keeps)
            if total_mk_cost <= effective_budget:
                break
            
            # If it doesn't fit, find the must-keep with the largest incremental cost
            # Incremental cost of program P = Cost(All MK) - Cost(All MK except P)
            # Or more simply: Cost of regions in P that are NOT in (All MK except P)
            
            max_inc_cost = -1
            max_inc_idx = -1
            
            for idx in current_must_keeps:
                # Calculate cost if we remove idx
                others = [i for i in current_must_keeps if i != idx]
                cost_without = calc_cost_for_indices(others)
                inc_cost = total_mk_cost - cost_without
                
                if inc_cost > max_inc_cost:
                    max_inc_cost = inc_cost
                    max_inc_idx = idx
            
            # Remove the one with largest incremental cost
            if max_inc_idx != -1:
                current_must_keeps.remove(max_inc_idx)
                dropped_must_keeps.append(max_inc_idx)
            else:
                # Should not happen if list is not empty
                break
        
        # Now, add the remaining must-keeps to 'kept' and 'paid_regions'
        for idx in current_must_keeps:
            kept[idx] = True
            for key in program_regions[idx]:
                if key not in paid_regions:
                    total_cost += strategy_region_cost[strategy].get(key, 0)
                    paid_regions.add(key)
        
        # Step 2: Fill with cheapest-incremental-cost-first among remaining programs
        remaining_programs = [i for i in range(100) if not kept[i]]
        
        # We will iterate until we can't add any more
        while True:
            best_prog = -1
            best_cost = float('inf')
            best_regions = set()
            
            for n in remaining_programs:
                if kept[n]:
                    continue
                
                inc_cost = 0
                new_regions = set()
                for key in program_regions[n]:
                    if key not in paid_regions:
                        c = strategy_region_cost[strategy].get(key, 0)
                        inc_cost += c
                        new_regions.add(key)
                
                # Tie breaker: lowest program number
                if inc_cost < best_cost or (inc_cost == best_cost and (best_prog == -1 or n < best_prog)):
                    best_cost = inc_cost
                    best_prog = n
                    best_regions = new_regions
            
            if best_prog == -1:
                break
            
            if total_cost + best_cost <= effective_budget:
                kept[best_prog] = True
                total_cost += best_cost
                paid_regions.update(best_regions)
                remaining_programs.remove(best_prog)
            else:
                break

        # Results
        programs_kept = sum(kept)
        
        combis_playable = 0
        playable_combi_nums = []
        for k in range(100):
            playable = True
            for p_num in combi_programs[k]:
                if not kept[p_num]:
                    playable = False
                    break
            if playable:
                combis_playable += 1
                playable_combi_nums.append(k)
        
        drum_kept = sum(1 for n in range(100) if kept[n] and program_is_drum[n])
        
        # Calculate unique cost per kept program
        # "number of bytes it uniquely costs" -> Cost of regions that ONLY this program uses among the kept set?
        # Or incremental cost when it was added?
        # The prompt says: "Also compute, per kept program, the number of bytes it uniquely costs."
        # This usually means: For each kept program P, sum of costs of regions R in P such that no other kept program Q uses R.
        
        unique_costs = {}
        # Build a map of region -> list of programs using it (among kept)
        region_to_progs = defaultdict(list)
        for n in range(100):
            if kept[n]:
                for key in program_regions[n]:
                    region_to_progs[key].append(n)
        
        for n in range(100):
            if kept[n]:
                unique_cost = 0
                for key in program_regions[n]:
                    if len(region_to_progs[key]) == 1:
                        unique_cost += strategy_region_cost[strategy].get(key, 0)
                unique_costs[n] = unique_cost

        return {
            'programs_kept': programs_kept,
            'combis_playable': combis_playable,
            'drum_kept': drum_kept,
            'total_bytes': total_cost,
            'kept_indices': [i for i in range(100) if kept[i]],
            'playable_combis': playable_combi_nums,
            'dropped_must_keeps': dropped_must_keeps,
            'unique_costs': unique_costs,
            'effective_budget': effective_budget
        }

    # 9. Run all combinations
    results = {}
    
    print("Running lite_pick calculations...")
    print(f"Metadata overhead assumed: {overhead_kib} KiB")
    
    for b_kib in budgets_kib:
        b_bytes = b_kib * 1024
        results[b_kib] = {}
        for s in strategies:
            res = select_programs(b_bytes, s)
            results[b_kib][s] = res

    # 10. Output
    # Print compact summary to stdout
    print("\n=== Lite Pick Summary ===")
    print(f"{'Budget':<10} {'Strategy':<10} {'Progs':<6} {'Combis':<7} {'Drums':<6} {'Bytes':<10} {'Dropped MK':<12}")
    print("-" * 60)
    
    for b_kib in budgets_kib:
        for s in strategies:
            res = results[b_kib][s]
            dropped_names = [program_names[i] for i in res['dropped_must_keeps']]
            dropped_str = ", ".join(dropped_names) if dropped_names else "-"
            print(f"{b_kib:<10} {s:<10} {res['programs_kept']:<6} {res['combis_playable']:<7} {res['drum_kept']:<6} {res['total_bytes']:<10} {dropped_str:<12}")

    # Write lite_pick_report.md
    report_path = os.path.join(out_dir, 'lite_pick_report.md')
    with open(report_path, 'w') as f:
        f.write("# Lite Pick Report\n\n")
        f.write("## Summary Table\n\n")
        f.write("| Budget (KiB) | Strategy | Programs Kept | Combis Playable | Drums Kept | Total Bytes | Dropped Must-Keeps |\n")
        f.write("|--------------|----------|---------------|-----------------|------------|-------------|--------------------|\n")
        
        for b_kib in budgets_kib:
            for s in strategies:
                res = results[b_kib][s]
                dropped_names = [program_names[i] for i in res['dropped_must_keeps']]
                dropped_str = ", ".join(dropped_names) if dropped_names else "-"
                f.write(f"| {b_kib} | {s} | {res['programs_kept']} | {res['combis_playable']} | {res['drum_kept']} | {res['total_bytes']} | {dropped_str} |\n")
        
        f.write("\n## Details per Configuration\n\n")
        
        for b_kib in budgets_kib:
            for s in strategies:
                res = results[b_kib][s]
                f.write(f"### Budget: {b_kib} KiB, Strategy: {s}\n\n")
                
                # Programs kept
                kept_indices = res['kept_indices']
                f.write(f"**Programs Kept ({len(kept_indices)}):**\n")
                for i in kept_indices:
                    mk_marker = " [MUST-KEEP]" if i in must_keep_indices else ""
                    f.write(f"- {i}: {program_names[i]}{mk_marker}\n")
                
                # Dropped must-keeps
                if res['dropped_must_keeps']:
                    f.write(f"\n**Dropped Must-Keeps:**\n")
                    for i in res['dropped_must_keeps']:
                        f.write(f"- {i}: {program_names[i]}\n")
                else:
                    f.write("\n**Dropped Must-Keeps:** None\n")
                
                # Playable combinations
                playable_combis = res['playable_combis']
                f.write(f"\n**Playable Combinations ({len(playable_combis)}):**\n")
                for k in playable_combis:
                    f.write(f"- {k}: {combi_names[k]}\n")
                
                # Drum programs kept
                drum_kept_indices = [i for i in kept_indices if program_is_drum[i]]
                f.write(f"\n**Drum Programs Kept ({len(drum_kept_indices)}):**\n")
                for i in drum_kept_indices:
                    f.write(f"- {i}: {program_names[i]}\n")
                
                # Bytes used
                f.write(f"\n**Bytes Used:** {res['total_bytes']} ({res['total_bytes']/1024.0:.2f} KiB)\n")
                
                # Weighted number of regions
                # "weighted number of regions" -> sum of costs? Or count of regions?
                # Given "bytes used" is separate, "weighted number of regions" likely means the sum of the costs (which is bytes used) 
                # OR it could mean the count of distinct regions. 
                # Let's assume it means the count of distinct regions stored.
                distinct_regions = set()
                for i in kept_indices:
                    distinct_regions.update(program_regions[i])
                f.write(f"**Distinct Regions Stored:** {len(distinct_regions)}\n")
                
                # Unique costs per kept program
                f.write(f"\n**Unique Cost per Kept Program (bytes):**\n")
                for i in kept_indices:
                    f.write(f"- {i}: {program_names[i]} -> {res['unique_costs'].get(i, 0)}\n")
                
                f.write("\n---\n\n")

    # Write JSON files
    for b_kib in budgets_kib:
        for s in strategies:
            res = results[b_kib][s]
            json_filename = f"lite_plan_{b_kib}_{s}.json"
            json_path = os.path.join(out_dir, json_filename)
            
            # Prepare regions list
            # "regions to be stored, one entry per distinct (start,end)"
            # "take loop and loop_on from the multisound/drum data where available, else 0/false"
            
            # We need to map region keys to loop/loop_on info.
            # This info is in ms (multisounds) and dr (drum sounds).
            # We need to find which multisound/drum sound corresponds to a region.
            # The region key is (start, end).
            
            # Let's build a lookup: (start, end) -> {loop, loop_on}
            # region_meta is already built in step 6.
            
            # Determine codec for each region based on strategy
            def get_codec_for_region(strategy, key):
                length = region_lengths.get(key, 0)
                if strategy == 'hyb4':
                    if length < 64:
                        return 'raw16'
                    else:
                        return 'ima4'
                elif strategy == 'hyb3':
                    if length < 64:
                        return 'raw16'
                    else:
                        return 'adpcm3'
                elif strategy == 'hyb2':
                    if length < 64:
                        return 'raw16'
                    else:
                        return 'adpcm2'
                return 'raw16'

            # Collect distinct regions from kept programs
            distinct_regions = set()
            for i in res['kept_indices']:
                distinct_regions.update(program_regions[i])
            
            regions_list = []
            for key in sorted(distinct_regions):
                start, end = key
                codec = get_codec_for_region(s, key)
                bytes_val = strategy_region_cost[s].get(key, 0)
                meta = region_meta.get(key, {'loop': 0, 'loop_on': False})
                
                regions_list.append({
                    "start": start,
                    "end": end,
                    "loop": meta['loop'],
                    "loop_on": meta['loop_on'],
                    "codec": codec,
                    "bytes": bytes_val
                })
            
            json_data = {
                "budget_bytes": b_kib * 1024,
                "strategy": s,
                "programs": res['kept_indices'],
                "combos": res['playable_combis'],
                "regions": regions_list
            }
            
            with open(json_path, 'w') as f:
                json.dump(json_data, f, indent=2)

    print(f"\nReport written to {report_path}")
    print(f"JSON files written to {out_dir}")

if __name__ == '__main__':
    main()

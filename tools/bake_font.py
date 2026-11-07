import sys
import re
from PIL import Image, ImageDraw, ImageFont

def parse_args(args):
    if len(args) < 4:
        print("Usage: python bake_font.py <font.ttf> <out.h> <name1>:<px1> [<name2>:<px2> ...]", file=sys.stderr)
        sys.exit(1)
    
    ttf_path = args[0]
    out_path = args[1]
    specs = args[2:]
    
    parsed_specs = []
    for spec in specs:
        if ':' not in spec:
            print(f"Error: Invalid spec format '{spec}'. Expected 'name:px'", file=sys.stderr)
            sys.exit(1)
        parts = spec.split(':')
        if len(parts) != 2:
            print(f"Error: Invalid spec format '{spec}'. Expected 'name:px'", file=sys.stderr)
            sys.exit(1)
        name, px_str = parts
        if not re.match(r'^[A-Za-z_][A-Za-z0-9_]*$', name):
            print(f"Error: Invalid identifier name '{name}'", file=sys.stderr)
            sys.exit(1)
        try:
            px = int(px_str)
            if px <= 0:
                raise ValueError
        except ValueError:
            print(f"Error: Invalid pixel size '{px_str}'", file=sys.stderr)
            sys.exit(1)
        parsed_specs.append((name, px))
        
    return ttf_path, out_path, parsed_specs

def bake_font(ttf_path, name, px):
    try:
        font = ImageFont.truetype(ttf_path, px)
    except Exception as e:
        print(f"Error: Could not load font '{ttf_path}' at size {px}: {e}", file=sys.stderr)
        sys.exit(1)
    
    ascent, descent = font.getmetrics()
    line_height = ascent + descent
    
    # Rasterization parameters
    img_w = 3 * px
    img_h = 3 * px
    baseline_y = 2 * px
    pen_x = px
    
    bitmap_bytes = []
    glyphs = []
    
    for char_code in range(32, 127):
        ch = chr(char_code)
        
        # Create image
        img = Image.new('L', (img_w, img_h), 0)
        draw = ImageDraw.Draw(img)
        
        # Draw text
        draw.text((pen_x, baseline_y), ch, font=font, fill=255, anchor="ls")
        
        # Get bounding box
        bbox = img.getbbox()
        
        if bbox is None:
            # Empty glyph (e.g., space)
            w = 0
            h = 0
            bx = 0
            by = 0
            off = len(bitmap_bytes)
        else:
            # Crop to bbox
            cropped = img.crop(bbox)
            w = cropped.width
            h = cropped.height
            bx = bbox[0] - pen_x
            by = bbox[1] - baseline_y
            
            # Append bytes
            pixels = list(cropped.getdata())
            bitmap_bytes.extend(pixels)
            off = len(bitmap_bytes) - len(pixels)
        
        # Advance width
        adv = round(font.getlength(ch))
        if adv < 1:
            adv = 1
        
        glyphs.append({
            'off': off,
            'w': w,
            'h': h,
            'bx': bx,
            'by': by,
            'adv': adv
        })
    
    return {
        'name': name,
        'px': px,
        'ascent': ascent,
        'descent': descent,
        'line_height': line_height,
        'bitmap': bitmap_bytes,
        'glyphs': glyphs
    }

def format_byte_array(name, data):
    lines = []
    for i in range(0, len(data), 16):
        chunk = data[i:i+16]
        line = "    " + ", ".join(str(b) for b in chunk)
        if i + 16 < len(data):
            line += ","
        lines.append(line)
    return "\n".join(lines)

def generate_header(ttf_path, out_path, specs):
    # Load font once to check if it's valid (optional, but good for early error)
    try:
        # Just check if we can open it
        with open(ttf_path, 'rb') as f:
            pass
    except Exception as e:
        print(f"Error: Could not open font file '{ttf_path}': {e}", file=sys.stderr)
        sys.exit(1)
    
    baked_fonts = []
    for name, px in specs:
        font_data = bake_font(ttf_path, name, px)
        baked_fonts.append(font_data)
        print(f"{name} px={px} ascent={font_data['ascent']} descent={font_data['descent']} bytes={len(font_data['bitmap'])}")
    
    # Generate header content
    lines = []
    lines.append("#pragma once")
    lines.append("#include <stdint.h>")
    lines.append("")
    lines.append("struct GlyphInfo {")
    lines.append("    uint32_t off;")
    lines.append("    uint8_t w, h;")
    lines.append("    int8_t bx, by;")
    lines.append("    uint8_t adv;")
    lines.append("};")
    lines.append("")
    lines.append("struct FontDef {")
    lines.append("    uint8_t px;")
    lines.append("    int8_t ascent;")
    lines.append("    int8_t descent;")
    lines.append("    uint8_t lineH;")
    lines.append("    const GlyphInfo* glyphs;")
    lines.append("    const uint8_t* bitmap;")
    lines.append("};")
    lines.append("")
    
    for font_data in baked_fonts:
        name = font_data['name']
        px = font_data['px']
        ascent = font_data['ascent']
        descent = font_data['descent']
        line_height = font_data['line_height']
        bitmap = font_data['bitmap']
        glyphs = font_data['glyphs']
        
        # Bitmap array
        lines.append(f"static const uint8_t {name}_bitmap[] = {{")
        if bitmap:
            lines.append(format_byte_array(name, bitmap))
        lines.append("};")
        lines.append("")
        
        # Glyphs array
        lines.append(f"static const GlyphInfo {name}_glyphs[95] = {{")
        glyph_lines = []
        for g in glyphs:
            glyph_lines.append(f"    {{{g['off']}, {g['w']}, {g['h']}, {g['bx']}, {g['by']}, {g['adv']}}}")
        lines.append(",\n".join(glyph_lines))
        lines.append("};")
        lines.append("")
        
        # FontDef
        lines.append(f"static const FontDef {name}_font = {{ {px}, {ascent}, {descent}, {line_height}, {name}_glyphs, {name}_bitmap }};")
        lines.append("")
    
    # Write to file
    with open(out_path, 'w') as f:
        f.write("\n".join(lines))

def main():
    args = sys.argv[1:]
    ttf_path, out_path, specs = parse_args(args)
    generate_header(ttf_path, out_path, specs)

if __name__ == "__main__":
    main()

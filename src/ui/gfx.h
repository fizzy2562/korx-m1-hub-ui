#pragma once

#include <stdint.h>
#include <math.h>
#include "font_data.h"

namespace gfx {

constexpr int W = 480;
constexpr int H = 320;

static uint16_t fb[W * H];

constexpr uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) {
    return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

uint16_t mix(uint16_t a, uint16_t b, uint8_t t) {
    int ar = (a >> 11) & 0x1F;
    int ag = (a >> 5) & 0x3F;
    int ab = a & 0x1F;
    int br = (b >> 11) & 0x1F;
    int bg = (b >> 5) & 0x3F;
    int bb = b & 0x1F;
    int tr = (ar * (255 - t) + br * t + 127) / 255;
    int tg = (ag * (255 - t) + bg * t + 127) / 255;
    int tb = (ab * (255 - t) + bb * t + 127) / 255;
    if (tr < 0) tr = 0; if (tr > 31) tr = 31;
    if (tg < 0) tg = 0; if (tg > 63) tg = 63;
    if (tb < 0) tb = 0; if (tb > 31) tb = 31;
    return (uint16_t)((tr << 11) | (tg << 5) | tb);
}

void clear(uint16_t c) {
    for (int i = 0; i < W * H; i++) fb[i] = c;
}

void fillRect(int x, int y, int w, int h, uint16_t c) {
    if (w <= 0 || h <= 0) return;
    int x0 = x < 0 ? 0 : x;
    int y0 = y < 0 ? 0 : y;
    int x1 = x + w > W ? W : x + w;
    int y1 = y + h > H ? H : y + h;
    if (x0 >= x1 || y0 >= y1) return;
    for (int yy = y0; yy < y1; yy++) {
        for (int xx = x0; xx < x1; xx++) {
            fb[yy * W + xx] = c;
        }
    }
}

void blendRect(int x, int y, int w, int h, uint16_t c, uint8_t alpha) {
    if (w <= 0 || h <= 0 || alpha == 0) return;
    int x0 = x < 0 ? 0 : x;
    int y0 = y < 0 ? 0 : y;
    int x1 = x + w > W ? W : x + w;
    int y1 = y + h > H ? H : y + h;
    if (x0 >= x1 || y0 >= y1) return;
    for (int yy = y0; yy < y1; yy++) {
        for (int xx = x0; xx < x1; xx++) {
            fb[yy * W + xx] = mix(fb[yy * W + xx], c, alpha);
        }
    }
}

void hline(int x, int y, int w, uint16_t c) {
    if (w <= 0) return;
    int x0 = x < 0 ? 0 : x;
    int x1 = x + w > W ? W : x + w;
    if (y < 0 || y >= H || x0 >= x1) return;
    for (int xx = x0; xx < x1; xx++) {
        fb[y * W + xx] = c;
    }
}

void vline(int x, int y, int h, uint16_t c) {
    if (h <= 0) return;
    int y0 = y < 0 ? 0 : y;
    int y1 = y + h > H ? H : y + h;
    if (x < 0 || x >= W || y0 >= y1) return;
    for (int yy = y0; yy < y1; yy++) {
        fb[yy * W + x] = c;
    }
}

void line(int x0, int y0, int x1, int y1, uint16_t c) {
    int dx = x1 - x0;
    int dy = y1 - y0;
    int sx = (dx > 0) ? 1 : -1;
    int sy = (dy > 0) ? 1 : -1;
    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;
    int err = dx - dy;
    while (1) {
        if (x0 >= 0 && x0 < W && y0 >= 0 && y0 < H) {
            fb[y0 * W + x0] = c;
        }
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx) { err += dx; y0 += sy; }
    }
}

void fillRoundRect(int x, int y, int w, int h, int r, uint16_t c) {
    if (w <= 0 || h <= 0) return;
    if (r < 0) r = 0;
    if (r > w / 2) r = w / 2;
    if (r > h / 2) r = h / 2;
    
    int x0 = x < 0 ? 0 : x;
    int y0 = y < 0 ? 0 : y;
    int x1 = x + w > W ? W : x + w;
    int y1 = y + h > H ? H : y + h;
    if (x0 >= x1 || y0 >= y1) return;

    // Straight edges
    if (r > 0) {
        // Top straight
        if (x + r < x1) fillRect(x + r, y0, (x + w - r) - (x + r), y0 + r - y0, c);
        // Bottom straight
        if (x + r < x1) fillRect(x + r, y1 - r, (x + w - r) - (x + r), y1 - (y1 - r), c);
        // Left straight
        if (y + r < y1) fillRect(x0, y + r, x0 + r - x0, (y + h - r) - (y + r), c);
        // Right straight
        if (y + r < y1) fillRect(x1 - r, y + r, x1 - (x1 - r), (y + h - r) - (y + r), c);
        fillRect(x + r, y + r, w - 2 * r, h - 2 * r, c);
    } else {
        fillRect(x0, y0, x1 - x0, y1 - y0, c);
        return;
    }

    // Corners (anti-aliased)
    // Top-Left
    int cx_tl = x + r;
    int cy_tl = y + r;
    for (int yy = y0; yy < cy_tl; yy++) {
        for (int xx = x0; xx < cx_tl; xx++) {
            float dx = (float)(xx + 0.5f) - (float)cx_tl;
            float dy = (float)(yy + 0.5f) - (float)cy_tl;
            float dist = sqrtf(dx * dx + dy * dy);
            float cov = (float)r + 0.5f - dist;
            if (cov > 0.0f) {
                if (cov > 1.0f) cov = 1.0f;
                uint8_t alpha = (uint8_t)(cov * 255.0f);
                fb[yy * W + xx] = mix(fb[yy * W + xx], c, alpha);
            }
        }
    }

    // Top-Right
    int cx_tr = x + w - r;
    int cy_tr = y + r;
    for (int yy = y0; yy < cy_tr; yy++) {
        for (int xx = cx_tr; xx < x1; xx++) {
            float dx = (float)(xx + 0.5f) - (float)cx_tr;
            float dy = (float)(yy + 0.5f) - (float)cy_tr;
            float dist = sqrtf(dx * dx + dy * dy);
            float cov = (float)r + 0.5f - dist;
            if (cov > 0.0f) {
                if (cov > 1.0f) cov = 1.0f;
                uint8_t alpha = (uint8_t)(cov * 255.0f);
                fb[yy * W + xx] = mix(fb[yy * W + xx], c, alpha);
            }
        }
    }

    // Bottom-Left
    int cx_bl = x + r;
    int cy_bl = y + h - r;
    for (int yy = cy_bl; yy < y1; yy++) {
        for (int xx = x0; xx < cx_bl; xx++) {
            float dx = (float)(xx + 0.5f) - (float)cx_bl;
            float dy = (float)(yy + 0.5f) - (float)cy_bl;
            float dist = sqrtf(dx * dx + dy * dy);
            float cov = (float)r + 0.5f - dist;
            if (cov > 0.0f) {
                if (cov > 1.0f) cov = 1.0f;
                uint8_t alpha = (uint8_t)(cov * 255.0f);
                fb[yy * W + xx] = mix(fb[yy * W + xx], c, alpha);
            }
        }
    }

    // Bottom-Right
    int cx_br = x + w - r;
    int cy_br = y + h - r;
    for (int yy = cy_br; yy < y1; yy++) {
        for (int xx = cx_br; xx < x1; xx++) {
            float dx = (float)(xx + 0.5f) - (float)cx_br;
            float dy = (float)(yy + 0.5f) - (float)cy_br;
            float dist = sqrtf(dx * dx + dy * dy);
            float cov = (float)r + 0.5f - dist;
            if (cov > 0.0f) {
                if (cov > 1.0f) cov = 1.0f;
                uint8_t alpha = (uint8_t)(cov * 255.0f);
                fb[yy * W + xx] = mix(fb[yy * W + xx], c, alpha);
            }
        }
    }
}

void strokeRoundRect(int x, int y, int w, int h, int r, int thick, uint16_t c) {
    if (w <= 0 || h <= 0 || thick <= 0) return;
    if (r < 0) r = 0;
    if (r > w / 2) r = w / 2;
    if (r > h / 2) r = h / 2;
    
    int x0 = x < 0 ? 0 : x;
    int y0 = y < 0 ? 0 : y;
    int x1 = x + w > W ? W : x + w;
    int y1 = y + h > H ? H : y + h;
    if (x0 >= x1 || y0 >= y1) return;

    // Straight edges
    if (r > 0) {
        // Top
        if (x + r < x1) fillRect(x + r, y0, (x + w - r) - (x + r), thick, c);
        // Bottom
        if (x + r < x1) fillRect(x + r, y1 - thick, (x + w - r) - (x + r), thick, c);
        // Left
        if (y + r < y1) fillRect(x0, y + r, thick, (y + h - r) - (y + r), c);
        // Right
        if (y + r < y1) fillRect(x1 - thick, y + r, thick, (y + h - r) - (y + r), c);
    } else {
        fillRect(x0, y0, x1 - x0, thick, c);
        fillRect(x0, y1 - thick, x1 - x0, thick, c);
        fillRect(x0, y0, thick, y1 - y0, c);
        fillRect(x1 - thick, y0, thick, y1 - y0, c);
        return;
    }

    // Corners (anti-aa stroke)
    // We approximate the stroke corner by blending based on distance from the corner center.
    // For a stroke of thickness `thick`, the "inner" radius is r - thick/2 and "outer" is r + thick/2.
    // However, the prompt specifies: "for each pixel in the corner squares compute its distance from the corner centre and use coverage = clamp(r + 0.5 - dist, 0, 1) for fill".
    // For stroke, we can interpret this as: the stroke covers the area where the distance is between (r - thick/2) and (r + thick/2).
    // But to keep it simple and consistent with the "smooth corners" request, we will blend based on the distance to the corner center, 
    // treating the stroke as a band. 
    // Actually, a simpler interpretation for "strokeRoundRect" with AA corners is to draw the outer rounded rect and inner rounded rect, 
    // but that's complex. Let's stick to the pixel distance method for the corner squares.
    
    // Corner squares are r x r.
    // For each pixel in the corner square, we calculate distance to the corner center.
    // The stroke is the region where dist is in [r - thick/2, r + thick/2].
    // Coverage = clamp( (r + thick/2 + 0.5 - dist) - (r - thick/2 + 0.5), 0, 1 )? 
    // Let's use: coverage = clamp( (r + 0.5 - dist) / (thick * 0.5 + 0.5), 0, 1 )? No.
    
    // Let's use the standard AA stroke approach:
    // dist to center. 
    // If dist < r - thick/2, it's inside (not stroke).
    // If dist > r + thick/2, it's outside (not stroke).
    // If in between, it's stroke.
    // AA: coverage = clamp( (r + thick/2 + 0.5 - dist) / (thick + 1.0), 0, 1 )? 
    // Let's simplify: coverage = clamp( (r + 0.5 - dist) / (thick * 0.5 + 0.5), 0, 1 ) is not quite right.
    
    // Let's use: 
    // float inner = (float)r - (float)thick * 0.5f;
    // float outer = (float)r + (float)thick * 0.5f;
    // if (dist < inner - 0.5f) cov = 1.0f;
    // else if (dist > outer + 0.5f) cov = 0.0f;
    // else cov = (outer + 0.5f - dist) / (thick + 1.0f);
    
    // This is a reasonable approximation.
    
    float inner = (float)r - (float)thick * 0.5f;
    float outer = (float)r + (float)thick * 0.5f;
    
    // Top-Left
    int cx_tl = x + r;
    int cy_tl = y + r;
    for (int yy = y0; yy < cy_tl; yy++) {
        for (int xx = x0; xx < cx_tl; xx++) {
            float dx = (float)(xx + 0.5f) - (float)cx_tl;
            float dy = (float)(yy + 0.5f) - (float)cy_tl;
            float dist = sqrtf(dx * dx + dy * dy);
            float cov = 0.0f;
            if (dist < inner - 0.5f) cov = 1.0f;
            else if (dist > outer + 0.5f) cov = 0.0f;
            else cov = (outer + 0.5f - dist) / (thick + 1.0f);
            if (cov > 0.0f) {
                if (cov > 1.0f) cov = 1.0f;
                uint8_t alpha = (uint8_t)(cov * 255.0f);
                fb[yy * W + xx] = mix(fb[yy * W + xx], c, alpha);
            }
        }
    }

    // Top-Right
    int cx_tr = x + w - r;
    int cy_tr = y + r;
    for (int yy = y0; yy < cy_tr; yy++) {
        for (int xx = cx_tr; xx < x1; xx++) {
            float dx = (float)(xx + 0.5f) - (float)cx_tr;
            float dy = (float)(yy + 0.5f) - (float)cy_tr;
            float dist = sqrtf(dx * dx + dy * dy);
            float cov = 0.0f;
            if (dist < inner - 0.5f) cov = 1.0f;
            else if (dist > outer + 0.5f) cov = 0.0f;
            else cov = (outer + 0.5f - dist) / (thick + 1.0f);
            if (cov > 0.0f) {
                if (cov > 1.0f) cov = 1.0f;
                uint8_t alpha = (uint8_t)(cov * 255.0f);
                fb[yy * W + xx] = mix(fb[yy * W + xx], c, alpha);
            }
        }
    }

    // Bottom-Left
    int cx_bl = x + r;
    int cy_bl = y + h - r;
    for (int yy = cy_bl; yy < y1; yy++) {
        for (int xx = x0; xx < cx_bl; xx++) {
            float dx = (float)(xx + 0.5f) - (float)cx_bl;
            float dy = (float)(yy + 0.5f) - (float)cy_bl;
            float dist = sqrtf(dx * dx + dy * dy);
            float cov = 0.0f;
            if (dist < inner - 0.5f) cov = 1.0f;
            else if (dist > outer + 0.5f) cov = 0.0f;
            else cov = (outer + 0.5f - dist) / (thick + 1.0f);
            if (cov > 0.0f) {
                if (cov > 1.0f) cov = 1.0f;
                uint8_t alpha = (uint8_t)(cov * 255.0f);
                fb[yy * W + xx] = mix(fb[yy * W + xx], c, alpha);
            }
        }
    }

    // Bottom-Right
    int cx_br = x + w - r;
    int cy_br = y + h - r;
    for (int yy = cy_br; yy < y1; yy++) {
        for (int xx = cx_br; xx < x1; xx++) {
            float dx = (float)(xx + 0.5f) - (float)cx_br;
            float dy = (float)(yy + 0.5f) - (float)cy_br;
            float dist = sqrtf(dx * dx + dy * dy);
            float cov = 0.0f;
            if (dist < inner - 0.5f) cov = 1.0f;
            else if (dist > outer + 0.5f) cov = 0.0f;
            else cov = (outer + 0.5f - dist) / (thick + 1.0f);
            if (cov > 0.0f) {
                if (cov > 1.0f) cov = 1.0f;
                uint8_t alpha = (uint8_t)(cov * 255.0f);
                fb[yy * W + xx] = mix(fb[yy * W + xx], c, alpha);
            }
        }
    }
}

void fillCircle(int cx, int cy, int r, uint16_t c) {
    if (r <= 0) return;
    int x0 = cx - r < 0 ? 0 : cx - r;
    int y0 = cy - r < 0 ? 0 : cy - r;
    int x1 = cx + r + 1 > W ? W : cx + r + 1;
    int y1 = cy + r + 1 > H ? H : cy + r + 1;
    if (x0 >= x1 || y0 >= y1) return;
    
    for (int yy = y0; yy < y1; yy++) {
        for (int xx = x0; xx < x1; xx++) {
            float dx = (float)(xx + 0.5f) - (float)cx;
            float dy = (float)(yy + 0.5f) - (float)cy;
            float dist = sqrtf(dx * dx + dy * dy);
            float cov = (float)r + 0.5f - dist;
            if (cov > 0.0f) {
                if (cov > 1.0f) cov = 1.0f;
                uint8_t alpha = (uint8_t)(cov * 255.0f);
                fb[yy * W + xx] = mix(fb[yy * W + xx], c, alpha);
            }
        }
    }
}

void arc(int cx, int cy, int r, int thick, float a0, float a1, uint16_t c) {
    if (r <= 0 || thick <= 0) return;
    
    // Normalize angles to [0, 360)
    a0 = fmodf(a0, 360.0f);
    if (a0 < 0) a0 += 360.0f;
    a1 = fmodf(a1, 360.0f);
    if (a1 < 0) a1 += 360.0f;
    
    // Ensure a0 < a1 for clockwise sweep
    if (a1 < a0) a1 += 360.0f;
    
    float r_outer = (float)r;
    float r_inner = (float)r - (float)thick;
    if (r_inner < 0) r_inner = 0;
    
    int x0 = cx - r - 1 < 0 ? 0 : cx - r - 1;
    int y0 = cy - r - 1 < 0 ? 0 : cy - r - 1;
    int x1 = cx + r + 2 > W ? W : cx + r + 2;
    int y1 = cy + r + 2 > H ? H : cy + r + 2;
    if (x0 >= x1 || y0 >= y1) return;
    
    for (int yy = y0; yy < y1; yy++) {
        for (int xx = x0; xx < x1; xx++) {
            float dx = (float)(xx + 0.5f) - (float)cx;
            float dy = (float)(yy + 0.5f) - (float)cy;
            float dist = sqrtf(dx * dx + dy * dy);
            
            // Radial coverage
            float cov_rad = (r_outer + 0.5f - dist);
            float cov_in = (dist - (r_inner - 0.5f));
            if (cov_in < cov_rad) cov_rad = cov_in;
            if (cov_rad > 1.0f) cov_rad = 1.0f;
            
            if (cov_rad <= 0.0f) continue;
            
            // Angular coverage (hard edges)
            // Angle clockwise from 12 o'clock (0 = up, 90 = right)
            // Standard math angle: 0 = right, 90 = up.
            // Our angle: 0 = up, 90 = right.
            // So: math_angle = 90 - our_angle.
            // Or: our_angle = 90 - math_angle.
            // math_angle = atan2(dy, dx) in radians.
            // our_angle_deg = 90 - (atan2(dy, dx) * 180 / PI)
            
            float math_rad = atan2f(dy, dx);
            float our_deg = 90.0f + (math_rad * 180.0f / 3.14159265f);
            if (our_deg < 0) our_deg += 360.0f;
            
            // Check if our_deg is in [a0, a1]
            // Since a1 might be > 360, we normalize our_deg to [0, 360) and check against a0 and a1 mod 360?
            // Actually, the arc is from a0 to a1 clockwise.
            // If a1 > 360, it wraps around.
            // Let's normalize a0 and a1 to [0, 360) for comparison, but handle wrap-around.
            float a0_norm = fmodf(a0, 360.0f);
            if (a0_norm < 0) a0_norm += 360.0f;
            float a1_norm = fmodf(a1, 360.0f);
            if (a1_norm < 0) a1_norm += 360.0f;
            
            bool in_arc = false;
            if (a1_norm >= a0_norm) {
                if (our_deg >= a0_norm && our_deg <= a1_norm) in_arc = true;
            } else {
                // Wrap-around
                if (our_deg >= a0_norm || our_deg <= a1_norm) in_arc = true;
            }
            
            if (!in_arc) continue;
            
            float cov = cov_rad;
            if (cov > 1.0f) cov = 1.0f;
            uint8_t alpha = (uint8_t)(cov * 255.0f);
            fb[yy * W + xx] = mix(fb[yy * W + xx], c, alpha);
        }
    }
    
    // Round the ends with small filled circles
    float mid_r = (r_outer + r_inner) * 0.5f;
    float end_r = (float)thick * 0.5f;
    
    // End 1: a0
    float a0_rad = (a0 - 90.0f) * 3.14159265f / 180.0f;
    int ex1 = (int)(cx + mid_r * cosf(a0_rad));
    int ey1 = (int)(cy + mid_r * sinf(a0_rad));
    fillCircle(ex1, ey1, (int)end_r, c);
    
    // End 2: a1
    float a1_rad = (a1 - 90.0f) * 3.14159265f / 180.0f;
    int ex2 = (int)(cx + mid_r * cosf(a1_rad));
    int ey2 = (int)(cy + mid_r * sinf(a1_rad));
    fillCircle(ex2, ey2, (int)end_r, c);
}

int textWidth(const FontDef& f, const char* s) {
    int w = 0;
    while (*s) {
        char c = *s++;
        if (c >= 32 && c <= 126) {
            w += f.glyphs[c - 32].adv;
        } else {
            w += f.glyphs['?' - 32].adv;
        }
    }
    return w;
}

void text(const FontDef& f, int x, int baselineY, const char* s, uint16_t c) {
    int penX = x;
    while (*s) {
        char ch = *s++;
        int idx;
        if (ch >= 32 && ch <= 126) {
            idx = ch - 32;
        } else {
            idx = '?' - 32;
        }
        const GlyphInfo& g = f.glyphs[idx];
        
        int gx = penX + g.bx;
        int gy = baselineY + g.by;
        
        for (int yy = 0; yy < g.h; yy++) {
            int py = gy + yy;
            if (py < 0 || py >= H) continue;
            for (int xx = 0; xx < g.w; xx++) {
                int px = gx + xx;
                if (px < 0 || px >= W) continue;
                uint8_t alpha = f.bitmap[g.off + yy * g.w + xx];
                if (alpha > 0) {
                    fb[py * W + px] = mix(fb[py * W + px], c, alpha);
                }
            }
        }
        penX += g.adv;
    }
}

void textC(const FontDef& f, int cx, int baselineY, const char* s, uint16_t c) {
    int w = textWidth(f, s);
    text(f, cx - w / 2, baselineY, s, c);
}

void textR(const FontDef& f, int rx, int baselineY, const char* s, uint16_t c) {
    int w = textWidth(f, s);
    text(f, rx - w, baselineY, s, c);
}

} // namespace gfx

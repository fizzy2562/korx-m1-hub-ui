#pragma once
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "gfx.h"
#include "m1ui_model.h"

namespace m1ui {

static const uint16_t BG       = gfx::rgb(11,17,25);
static const uint16_t CHIP     = gfx::rgb(22,32,48);
static const uint16_t TRACK    = gfx::rgb(31,43,61);
static const uint16_t BLUE     = gfx::rgb(77,163,232);
static const uint16_t VIOLET   = gfx::rgb(183,148,246);
static const uint16_t GREY     = gfx::rgb(138,154,176);
static const uint16_t WHITE    = gfx::rgb(235,240,248);
static const uint16_t DISC     = gfx::rgb(27,38,56);
static const uint16_t DIVIDER  = gfx::rgb(30,42,58);
static const uint16_t BTN      = gfx::rgb(24,38,60);
static const uint16_t DARK     = gfx::rgb(11,17,25);

struct Meters { float l = 0, r = 0; int voices = 0; };

inline void draw(const Model& m, const Meters& mt) {
    gfx::clear(BG);

    // 2. HEADER
    {
        const char* modeStr = (m.mode == 0) ? "PROGRAM" : "COMBI";
        gfx::text(mid_font, 12, 24, modeStr, VIOLET);

        char num[4];
        int cur = (m.mode == 0) ? m.program : m.combi;
        num[0] = (char)('0' + (cur / 10) % 10);
        num[1] = (char)('0' + (cur % 10));
        num[2] = 0;

        char name[11];
        getName(m, name);

        int wNum = gfx::textWidth(mid_font, num);
        int wName = gfx::textWidth(mid_font, name);
        int total = wNum + 8 + wName;
        int startX = 250 - total / 2;

        gfx::text(mid_font, startX, 24, num, GREY);
        gfx::text(mid_font, startX + wNum + 8, 24, name, WHITE);

        if (m.edited) {
            gfx::text(mid_font, startX + wNum + 8 + wName, 24, "*", BLUE);
        }
        if (m.comparing) {
            gfx::textR(small_font, 330, 22, "STORED", BLUE);
        }

        // Voice squares: 16 squares, 5x5, spacing 7, last ends at x=468, y=14
        // Last square x = 468 - 5 = 463. First square x = 463 - 15*7 = 463 - 105 = 358.
        int sqX = 463 - 15 * 7;
        for (int i = 0; i < 16; i++) {
            int x = sqX + i * 7;
            uint16_t c = (i < mt.voices) ? VIOLET : CHIP;
            gfx::fillRect(x, 14, 5, 5, c);
        }

        gfx::hline(10, 34, 460, DIVIDER);
    }

    // 3. TABS
    {
        int nGroups = m.nGroups;
        if (nGroups > 0) {
            // Calculate widths
            int widths[40];
            int totalW = 0;
            for (int g = 0; g < nGroups; g++) {
                widths[g] = gfx::textWidth(small_font, m.groupName[g]) + 18;
                totalW += widths[g];
            }
            totalW += (nGroups - 1) * 6; // gaps

            // Determine window
            int start = m.group;
            int end = m.group;
            int curW = widths[start];

            bool left = true;
            int fails = 0;
            while (true) {
                bool added = false;
                if (left) {
                    if (start > 0) {
                        int newW = curW + widths[start - 1] + 6;
                        if (newW <= 460) {
                            start--;
                            curW = newW;
                            added = true;
                        }
                    }
                    left = false;
                } else {
                    if (end < nGroups - 1) {
                        int newW = curW + widths[end + 1] + 6;
                        if (newW <= 460) {
                            end++;
                            curW = newW;
                            added = true;
                        }
                    }
                    left = true;
                }
                if (added) fails = 0;
                else if (++fails >= 2) break;
            }

            // Draw window starting at x=10
            int x = 10;
            for (int g = start; g <= end; g++) {
                int w = widths[g];
                bool isCur = (g == m.group);
                uint16_t fill = isCur ? VIOLET : CHIP;
                uint16_t txt = isCur ? DARK : GREY;
                gfx::fillRoundRect(x, 41, w, 26, 8, fill);
                gfx::textC(small_font, x + w / 2, 59, m.groupName[g], txt);
                x += w + 6;
            }
        }
    }

    // 4. KNOBS
    for (int i = 0; i < 4; i++) {
        int cx = 67 + 115 * i;
        char text[12];
        float norm = -1.0f;
        const char* label = nullptr;
        slotText(m, i, text, 12, &norm, &label);

        if (text[0] == 0) continue;

        // (a) Label
        if (label) {
            char up[15];
            int len = 0;
            while (label[len] && len < 14) {
                char c = label[len];
                if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
                up[len] = c;
                len++;
            }
            up[len] = 0;

            // Trim if wider than 108
            while (len > 0 && gfx::textWidth(small_font, up) > 108) {
                up[len - 1] = 0;
                len--;
            }
            gfx::textC(small_font, cx, 88, up, GREY);
        }

        // (b) Arc track
        gfx::arc(cx, 127, 40, 4, 225, 495, TRACK);

        // (c) Disc
        gfx::fillCircle(cx, 127, 32, DISC);

        // (b) Value arc and (d) pointer
        if (norm >= 0.005f) {
            float n = norm;
            if (n > 1.0f) n = 1.0f;
            float a = 225.0f + 270.0f * n;
            gfx::arc(cx, 127, 40, 4, 225, (int)a, BLUE);

            // Pointer
            float rad = a * (float)M_PI / 180.0f;
            float dx = 26.0f * sinf(rad);
            float dy = -26.0f * cosf(rad);
            int x1 = cx;
            int y1 = 127;
            int x2 = (int)(cx + dx);
            int y2 = (int)(127 + dy);
            gfx::line(x1, y1, x2, y2, WHITE);
            gfx::line(x1 + 1, y1, x2 + 1, y2, WHITE);
        }

        // (e) Value text
        int wBig = gfx::textWidth(big_font, text);
        int wMid = gfx::textWidth(mid_font, text);
        int wSml = gfx::textWidth(small_font, text);

        if (wBig <= 108) {
            gfx::textC(big_font, cx, 197, text, WHITE);
        } else if (wMid <= 108) {
            gfx::textC(mid_font, cx, 195, text, WHITE);
        } else {
            gfx::textC(small_font, cx, 193, text, WHITE);
        }
    }

    // 5. STATUS LINE
    {
        char status[80];
        int a = m.sub + 1;
        int n = groupSize(m, m.group);
        int b = m.sub + 4;
        if (b > n) b = n;

        int len = 0;
        len += (int)snprintf(status + len, sizeof(status) - len, "%s - %d-%d of %d", m.groupName[m.group], a, b, n);
        len += (int)snprintf(status + len, sizeof(status) - len, "  (page %d/%d)", m.group + 1, m.nGroups);

        gfx::textC(small_font, 240, 222, status, GREY);
    }

    // 6. METERS
    {
        float levels[2] = { mt.l, mt.r };
        int ys[2] = { 240, 246 };
        for (int ch = 0; ch < 2; ch++) {
            int y = ys[ch];
            gfx::fillRect(10, y, 460, 3, CHIP);
            float lvl = levels[ch];
            if (lvl < 0.0f) lvl = 0.0f;
            if (lvl > 1.0f) lvl = 1.0f;
            int w = (int)(lvl * 460.0f);
            if (w > 0) {
                // Blue for most, Violet for last 15%
                int blueW = (int)(460.0f * 0.85f);
                if (w <= blueW) {
                    gfx::fillRect(10, y, w, 3, BLUE);
                } else {
                    gfx::fillRect(10, y, blueW, 3, BLUE);
                    gfx::fillRect(10 + blueW, y, w - blueW, 3, VIOLET);
                }
            }
        }
    }

    // 7. SOFT KEYS
    {
        char caps[25][8];
        captions(m, caps);
        for (int i = 0; i < 6; i++) {
            int x = 6 + 79 * i;
            uint16_t fill = BTN;
            uint16_t txt = WHITE;
            if (i == 3 && m.comparing) {
                fill = VIOLET;
                txt = DARK;
            }
            gfx::fillRoundRect(x, 286, 73, 30, 8, fill);
            gfx::textC(small_font, x + 36, 306, caps[i], txt);
        }
    }
}

inline void fillLeds(const Model& m, uint8_t* buf384) {
    memset(buf384, 0, 384);
    uint32_t* u = (uint32_t*)buf384;
    float* f = (float*)buf384;

    // K1..K6 (b = 10..15)
    for (int b = 10; b <= 15; b++) {
        int g = b - 10;
        if (g < m.nGroups) {
            u[32 + b] = 0xB794F6;
            f[57 + b] = (g == m.group) ? 1.0f : 0.25f;
        }
    }

    // F4 (b=3)
    if (m.comparing) {
        u[32 + 3] = 0xB794F6;
        f[57 + 3] = 1.0f;
    }

    // Button 17 (HOME) off (already zeroed)
}

} // namespace m1ui

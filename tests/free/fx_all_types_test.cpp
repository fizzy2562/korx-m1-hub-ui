#include "m1free_all.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

static m1::RomTables g_rom;
static int g_rom_filled = 0;
static int16_t g_fxmem[65536];
static uint8_t g_blk[143];
static m1::Engine g_eng;

static void renderType(int type, int mode, float* outL, int total) {
    if (!g_rom_filled) {
        m1free::fillRom(m1free::g_free, g_rom);
        g_rom_filled = 1;
    }
    g_eng.init(g_rom, g_fxmem);
    memcpy(g_blk, m1free::g_free.p.programs + 143 * 1, 143);
    g_blk[38] = (uint8_t)type;
    g_blk[39] = 33;
    g_blk[40] = 100;
    g_blk[41] = 100;
    g_blk[42] = 0;
    g_blk[43] = 0;
    g_blk[44] = 0;
    g_blk[45] = 0;
    g_blk[46] = 0x03;
    int k;
    for (k = 0; k < 8; k++) {
        if (mode == 0) {
            g_blk[47 + k] = 0;
        } else if (mode == 1) {
            g_blk[47 + k] = 50;
        } else if (mode == 2) {
            g_blk[47 + k] = 99;
        } else if (mode == 3) {
            g_blk[47 + k] = 127;
        } else {
            g_blk[47 + k] = 0x80;
        }
    }
    g_eng.playProgram(g_blk);
    g_eng.noteOn(0, 60, 127);
    int rendered = 0;
    bool released = false;
    while (rendered < total) {
        if (!released && rendered >= 15625) {
            g_eng.noteOff(0, 60);
            released = true;
        }
        int chunk = 625;
        if (total - rendered < chunk) {
            chunk = total - rendered;
        }
        g_eng.render(outL + rendered, outL + rendered, chunk);
        rendered += chunk;
    }
}

int main(void) {
    int any_fail = 0;
    int pass_count = 0;
    int fail_count = 0;
    int t;
    for (t = 0; t <= 33; t++) {
        double maxes[5];
        int m;
        for (m = 0; m < 5; m++) {
            static float A[31250];
            renderType(t, m, A, 31250);
            int finite = 1;
            double maxAbs = 0.0;
            int j;
            for (j = 0; j < 31250; j++) {
                double v = (double)A[j];
                if (!isfinite(v)) {
                    finite = 0;
                }
                double av = fabs(v);
                if (av > maxAbs) {
                    maxAbs = av;
                }
            }
            maxes[m] = maxAbs;
            char name[64];
            snprintf(name, sizeof(name), "t%d_m%d", t, m);
            if (finite && maxAbs < 8.0) {
                printf("PASS %s\n", name);
                pass_count++;
            } else {
                printf("FAIL %s maxAbs=%.6f finite=%d\n", name, maxAbs, finite);
                fail_count++;
                any_fail = 1;
            }
        }
        printf("type %d maxAbs over modes = %.6f %.6f %.6f %.6f %.6f\n", t, maxes[0], maxes[1], maxes[2], maxes[3], maxes[4]);
    }
    printf("TOTAL PASS=%d FAIL=%d\n", pass_count, fail_count);
    if (any_fail) {
        return 1;
    }
    return 0;
}

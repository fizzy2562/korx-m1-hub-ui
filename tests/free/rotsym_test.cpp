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

static float AL[62500];
static float AR[62500];
static float BL[62500];
static float BR[62500];

static void render(int typ, const uint8_t* p8, int dry, float* L, float* R, int total) {
    if (!g_rom_filled) {
        m1free::fillRom(m1free::g_free, g_rom);
        g_rom_filled = 1;
    }
    g_eng.init(g_rom, g_fxmem);
    memcpy(g_blk, m1free::g_free.p.programs + 0, 143);
    g_blk[98] = 0;
    if (dry) {
        g_blk[38] = 33;
    } else {
        g_blk[38] = typ;
        g_blk[39] = 0;
        g_blk[40] = 99;
        g_blk[46] = 1;
        for (int i = 0; i < 8; i++) {
            g_blk[47 + i] = p8[i];
        }
    }
    g_eng.playProgram(g_blk);
    g_eng.noteOn(0, 60, 100);
    int done = 0;
    while (done < total) {
        int chunk = 500;
        if (total - done < chunk) {
            chunk = total - done;
        }
        g_eng.render(L + done, R + done, chunk);
        done += chunk;
    }
}

static double rms(const float* x, int a, int b) {
    double sum = 0.0;
    int n = b - a;
    for (int i = a; i < b; i++) {
        double v = (double)x[i];
        sum += v * v;
    }
    return sqrt(sum / (double)n);
}

static int finiteBounded(const float* x, int n) {
    for (int i = 0; i < n; i++) {
        double v = (double)x[i];
        if (!isfinite(v) || v > 1.0 || v < -1.0) {
            return 0;
        }
    }
    return 1;
}

static double winRatio(const float* x) {
    double max_rms = 0.0;
    double min_rms = 1e9;
    for (int w = 0; w < 40; w++) {
        int start = 20000 + w * 1024;
        double sum = 0.0;
        for (int i = 0; i < 1024; i++) {
            double v = (double)x[start + i];
            sum += v * v;
        }
        double r = sqrt(sum / 1024.0);
        if (r > max_rms) max_rms = r;
        if (r < min_rms) min_rms = r;
    }
    if (min_rms < 1e-9) min_rms = 1e-9;
    return max_rms / min_rms;
}

int main(void) {
    int pass = 0;
    int fail = 0;
    m1free::fillRom(m1free::g_free, g_rom);
    g_rom_filled = 1;

    // 1 sym_table
    {
        int ok = 1;
        for (int k = 0; k < 3; k++) {
            if (g_rom.fx->sym.baseL[k] + 760 >= 2000 || g_rom.fx->sym.baseR[k] + 760 >= 2000) {
                ok = 0;
            }
        }
        if (g_rom.fx->rot.slow[10] != 30 || g_rom.fx->rot.fast[10] != 250) {
            ok = 0;
        }
        for (int i = 0; i < 19; i++) {
            if (g_rom.fx->rot.slow[i] >= g_rom.fx->rot.slow[i + 1]) {
                ok = 0;
            }
            if (g_rom.fx->rot.fast[i] >= g_rom.fx->rot.fast[i + 1]) {
                ok = 0;
            }
        }
        if (g_rom.fx->rot.fast[20] >= 600) {
            ok = 0;
        }
        if (ok) {
            printf("PASS sym_table\n");
            pass++;
        } else {
            printf("FAIL sym_table\n");
            fail++;
        }
    }

    // Symphonic
    {
        uint8_t p8[8] = {60, 0, 0, 0, 0, 0, 0, 0};
        render(23, p8, 0, AL, AR, 62500);
        render(23, p8, 1, BL, BR, 62500);

        // 2 sym_finite
        if (finiteBounded(AL, 62500) && finiteBounded(AR, 62500)) {
            printf("PASS sym_finite\n");
            pass++;
        } else {
            printf("FAIL sym_finite\n");
            fail++;
        }

        // 3 sym_rms
        double rmsL = rms(AL, 42500, 62500);
        double rmsR = rms(AR, 42500, 62500);
        if (rmsL > 1e-3 && rmsR > 1e-3) {
            printf("PASS sym_rms\n");
            pass++;
        } else {
            printf("FAIL sym_rms rmsL=%.6e rmsR=%.6e\n", rmsL, rmsR);
            fail++;
        }

        // 4 sym_lr
        double ratio = rmsL / rmsR;
        double db = 20.0 * log10(ratio);
        if (fabs(db) < 6.0) {
            printf("PASS sym_lr\n");
            pass++;
        } else {
            printf("FAIL sym_lr db=%.2f\n", db);
            fail++;
        }

        // 5 sym_differs_from_dry
        double sumDiff = 0.0;
        for (int i = 42500; i < 62500; i++) {
            sumDiff += fabs((double)AL[i] - (double)BL[i]);
        }
        if (sumDiff > 1.0) {
            printf("PASS sym_differs_from_dry\n");
            pass++;
        } else {
            printf("FAIL sym_differs_from_dry sumDiff=%.6e\n", sumDiff);
            fail++;
        }
    }

    // Rotary slow
    {
        uint8_t p8[8] = {99, 0, 0, 0, 0, 0, 0, 0};
        render(24, p8, 0, AL, AR, 62500);

        // 6 rot_slow_finite
        if (finiteBounded(AL, 62500) && finiteBounded(AR, 62500)) {
            printf("PASS rot_slow_finite\n");
            pass++;
        } else {
            printf("FAIL rot_slow_finite\n");
            fail++;
        }

        // 7 rot_slow_rms
        double rmsL = rms(AL, 42500, 62500);
        if (rmsL > 1e-3) {
            printf("PASS rot_slow_rms\n");
            pass++;
        } else {
            printf("FAIL rot_slow_rms rmsL=%.6e\n", rmsL);
            fail++;
        }

        // 8 rot_slow_mod
        double ratio = winRatio(AL);
        if (ratio > 1.05) {
            printf("PASS rot_slow_mod ratio=%.4f\n", ratio);
            pass++;
        } else {
            printf("FAIL rot_slow_mod ratio=%.4f\n", ratio);
            fail++;
        }
    }

    // Rotary fast
    {
        uint8_t p8[8] = {99, 0, 0, 0, 0, 0, 0, 1};
        render(24, p8, 0, AL, AR, 62500);

        // 9 rot_fast_finite
        if (finiteBounded(AL, 62500) && finiteBounded(AR, 62500)) {
            printf("PASS rot_fast_finite\n");
            pass++;
        } else {
            printf("FAIL rot_fast_finite\n");
            fail++;
        }

        // 10 rot_fast_mod
        double ratio_fast = winRatio(AL);
        printf("INFO rot_fast_mod ratio=%.4f\n", ratio_fast);
        if (isfinite(ratio_fast) && ratio_fast > 1.0) {
            printf("PASS rot_fast_mod\n");
            pass++;
        } else {
            printf("FAIL rot_fast_mod ratio=%.4f\n", ratio_fast);
            fail++;
        }
    }

    printf("SUMMARY pass=%d fail=%d\n", pass, fail);
    return fail ? 1 : 0;
}

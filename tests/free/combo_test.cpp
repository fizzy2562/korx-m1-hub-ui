#include "m1free_all.h"
#include "m1free_fx_combo.h"
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

static void render(int typ, const uint8_t* p8, int dry, float* L, float* R, int total, int releaseAt) {
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
        g_blk[41] = 99;
        g_blk[39] = 33;
        g_blk[43] = 0;
        g_blk[42] = 0;
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
        if (done >= releaseAt) {
            g_eng.noteOff(0, 60);
        }
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
        if (!isfinite(v) || v > 8.0 || v < -8.0) {
            return 0;
        }
    }
    return 1;
}

int main(void) {
    int pass = 0;
    int fail = 0;
    m1free::fillRom(m1free::g_free, g_rom);
    g_rom_filled = 1;
    m1free::generateFxTablesCombo(m1free::g_free.fx);

    // 1 crev_tables
    {
        int ok = 1;
        for (int i = 0; i < 2; i++) {
            const m1::FxComboRev& r = g_rom.fx->crev[i];
            for (int k = 0; k < 4; k++) {
                if (r.comb_d[k] < 64) ok = 0;
            }
            for (int k = 0; k < 3; k++) {
                if (r.ap_d[k] < 8) ok = 0;
            }
            for (int k = 0; k < 4; k++) {
                for (int rt = 0; rt < 98; rt++) {
                    float v = r.fb[k][rt];
                    if (v <= 0.0f || v > 0.97f) ok = 0;
                }
                for (int hd = 0; hd < 100; hd++) {
                    float v = r.damp[k][hd];
                    if (v < 0.0f || v > 0.7f) ok = 0;
                }
            }
            int max_base = r.base_ap > r.base_b ? r.base_ap : r.base_b;
            int budget = 15628 + (150 * 31.25 + 6 + max_base + 4);
            for (int k = 0; k < 3; k++) {
                budget += r.ap_d[k] + 2;
            }
            for (int k = 0; k < 4; k++) {
                int m = r.comb_d[k] > r.comb_tap[k] ? r.comb_d[k] : r.comb_tap[k];
                budget += m + 2;
            }
            if (budget > 32768) ok = 0;
        }
        if (ok) {
            printf("PASS crev_tables\n");
            pass++;
        } else {
            printf("FAIL crev_tables\n");
            fail++;
        }
    }

    // 2 cer_tables
    {
        int ok = 1;
        const m1::FxComboEr& e = g_rom.fx->cer;
        double maxT = 0.0;
        for (int k = 0; k < 25; k++) {
            if (e.T[k] > maxT) maxT = e.T[k];
        }
        int budget = 15628 + (40 * (int)maxT + 100) * 31.25 + 3;
        for (int j = 0; j < 4; j++) {
            budget += 40 * (int)e.A[j] + 3;
        }
        if (budget > 32768) ok = 0;
        if (ok) {
            printf("PASS cer_tables\n");
            pass++;
        } else {
            printf("FAIL cer_tables budget=%d\n", budget);
            fail++;
        }
    }

    // 3-5 reverb types
    {
        int types[2] = {25, 26};
        uint8_t p8s[2][8] = {
            {150, 0, 20, 30, 60, 0, 30, 10},
            {150, 0, 20, 30, 30, 0, 30, 10}
        };
        for (int t = 0; t < 2; t++) {
            int typ = types[t];
            const uint8_t* p8 = p8s[t];
            char name[64];
            sprintf(name, "rev%d_finite", typ);
            render(typ, p8, 0, AL, AR, 62500, 20000);
            render(typ, p8, 1, BL, BR, 62500, 20000);

            if (finiteBounded(AL, 62500) && finiteBounded(AR, 62500)) {
                printf("PASS %s\n", name);
                pass++;
            } else {
                printf("FAIL %s\n", name);
                fail++;
            }

            sprintf(name, "rev%d_reverb_tail", typ);
            double eA = rms(AR, 30000, 62500);
            double eB = rms(BR, 30000, 62500);
            if (eA > 1e-4 && eA > 10.0 * eB) {
                printf("PASS %s\n", name);
                pass++;
            } else {
                printf("FAIL %s eA=%.6e eB=%.6e\n", name, eA, eB);
                fail++;
            }

            sprintf(name, "rev%d_decay", typ);
            double eLate = rms(AR, 50000, 62500);
            double eEarly = rms(AR, 30000, 42500);
            if (eLate < eEarly) {
                printf("PASS %s\n", name);
                pass++;
            } else {
                printf("FAIL %s eLate=%.6e eEarly=%.6e\n", name, eLate, eEarly);
                fail++;
            }

            sprintf(name, "rev%d_delay_echo", typ);
            double eAL = rms(AL, 24000, 40000);
            double eBL = rms(BL, 24000, 40000);
            if (eAL > 1e-5 && eAL > 10.0 * eBL) {
                printf("PASS %s\n", name);
                pass++;
            } else {
                printf("FAIL %s eAL=%.6e eBL=%.6e\n", name, eAL, eBL);
                fail++;
            }
        }
    }

    // 6 early reflection
    {
        uint8_t p8[8] = {100, 0, 20, 30, 30, 100, 0, 0};
        render(27, p8, 0, AL, AR, 62500, 20000);
        render(27, p8, 1, BL, BR, 62500, 20000);

        if (finiteBounded(AL, 62500) && finiteBounded(AR, 62500)) {
            printf("PASS er_finite\n");
            pass++;
        } else {
            printf("FAIL er_finite\n");
            fail++;
        }

        double eA = rms(AR, 21500, 24500);
        double eB = rms(BR, 21500, 24500);
        if (eA > 1e-5 && eA > 10.0 * eB) {
            printf("PASS er_tail\n");
            pass++;
        } else {
            printf("FAIL er_tail eA=%.6e eB=%.6e\n", eA, eB);
            fail++;
        }
    }

    printf("SUMMARY pass=%d fail=%d\n", pass, fail);
    return fail ? 1 : 0;
}

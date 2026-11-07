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
        g_blk[39] = 33;
        g_blk[40] = 99;
        g_blk[41] = 99;
        g_blk[42] = 0;
        g_blk[43] = 0;
        g_blk[44] = 0;
        g_blk[45] = 0;
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
        if (done >= releaseAt) {
            g_eng.noteOff(0, 60);
        }
        done += chunk;
    }
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

static double rms(const float* x, int a, int b) {
    double sum = 0.0;
    int n = b - a;
    for (int i = a; i < b; i++) {
        double v = (double)x[i];
        sum += v * v;
    }
    return sqrt(sum / (double)n);
}

static double diffRms(const float* A, const float* B, int a, int b) {
    double sum = 0.0;
    int n = b - a;
    for (int i = a; i < b; i++) {
        double d = (double)A[i] - (double)B[i];
        sum += d * d;
    }
    return sqrt(sum / (double)n);
}

static double maxAbs(const float* x, int n) {
    double m = 0.0;
    for (int i = 0; i < n; i++) {
        double v = fabs((double)x[i]);
        if (v > m) m = v;
    }
    return m;
}

int main(void) {
    int pass = 0;
    int fail = 0;
    m1free::fillRom(m1free::g_free, g_rom);
    g_rom_filled = 1;

    uint8_t vectors[4][8] = {
        {50, 30, 0, 30, 30, 30, 30, 30},
        {90, 60, 2, 60, 60, 60, 60, 60},
        {20, 10, 0, 10, 10, 10, 10, 10},
        {70, 100, 1, 40, 20, 80, 0, 50}
    };

    for (int t = 0; t <= 32; t++) {
        int allBounded = 1;
        int anyWorks = 0;
        double bestRatio = 0.0;
        int bestVec = -1;
        double maxAbsAll = 0.0;

        for (int v = 0; v < 4; v++) {
            render(t, vectors[v], 0, AL, AR, 62500, 40000);
            render(t, vectors[v], 1, BL, BR, 62500, 40000);

            int fb = finiteBounded(AL, 62500) && finiteBounded(AR, 62500);
            if (!fb) allBounded = 0;

            double rmsWet = sqrt(0.5 * (rms(AL, 2000, 62500) * rms(AL, 2000, 62500) + rms(AR, 2000, 62500) * rms(AR, 2000, 62500)));
            double rmsDry = sqrt(0.5 * (rms(BL, 2000, 62500) * rms(BL, 2000, 62500) + rms(BR, 2000, 62500) * rms(BR, 2000, 62500)));
            double diff = sqrt(0.5 * (diffRms(AL, BL, 2000, 62500) * diffRms(AL, BL, 2000, 62500) + diffRms(AR, BR, 2000, 62500) * diffRms(AR, BR, 2000, 62500)));

            double m = maxAbs(AL, 62500);
            double m2 = maxAbs(AR, 62500);
            if (m > maxAbsAll) maxAbsAll = m;
            if (m2 > maxAbsAll) maxAbsAll = m2;

            double ratio = (rmsDry > 1e-9) ? (diff / rmsDry) : 0.0;
            if (ratio > bestRatio) {
                bestRatio = ratio;
                bestVec = v;
            }

            if (fb && rmsWet > 1e-4 && diff > 0.02 * rmsDry) {
                anyWorks = 1;
            }
        }

        printf("INFO type=%d best_diff_ratio=%.6f vector=%d maxabs=%.6f\n", t, bestRatio, bestVec, maxAbsAll);

        if (allBounded) {
            printf("PASS type%d_bounded\n", t);
            pass++;
        } else {
            printf("FAIL type%d_bounded\n", t);
            fail++;
        }

        if (anyWorks) {
            printf("PASS type%d_changes_sound\n", t);
            pass++;
        } else {
            printf("FAIL type%d_changes_sound\n", t);
            fail++;
        }
    }

    printf("SUMMARY pass=%d fail=%d\n", pass, fail);
    return fail ? 1 : 0;
}

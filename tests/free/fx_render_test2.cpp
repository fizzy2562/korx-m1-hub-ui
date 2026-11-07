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

static void renderCase(int prog, int dry, float* outL, int total) {
    if (!g_rom_filled) {
        m1free::fillRom(m1free::g_free, g_rom);
        g_rom_filled = 1;
    }
    g_eng.init(g_rom, g_fxmem);
    memcpy(g_blk, m1free::g_free.p.programs + 143 * prog, 143);
    g_blk[98] = 0;   // VDA EG release time 0: after note-off only the effect tail remains
    if (dry) {
        g_blk[38] = 33;
    }
    g_eng.playProgram(g_blk);
    g_eng.noteOn(0, 60, 100);
    int rendered = 0;
    bool released = false;
    while (rendered < total) {
        if (!released && rendered >= 37500) {
            g_eng.noteOff(0, 60);   // release the note at 1.2 s, then keep rendering
            released = true;
        }
        int chunk = 500;
        if (total - rendered < chunk) {
            chunk = total - rendered;
        }
        g_eng.render(outL + rendered, outL + rendered, chunk);
        rendered += chunk;
    }
}

int main(void) {
    int any_fail = 0;
    int progs[5] = {14, 15, 16, 17, 18};
    int i;
    for (i = 0; i < 5; i++) {
        int prog = progs[i];
        static float A[78125];
        static float B[78125];
        renderCase(prog, 0, A, 78125);
        renderCase(prog, 1, B, 78125);
        double sumA = 0.0;
        double sumB = 0.0;
        double sumDiff = 0.0;
        int finite = 1;
        int j;
        for (j = 0; j < 78125; j++) {
            double a = (double)A[j];
            double b = (double)B[j];
            if (!(a == a) || !(b == b) || a > 8.0 || a < -8.0 || b > 8.0 || b < -8.0) {
                finite = 0;
            }
            sumA += a * a;
            sumB += b * b;
            double d = a - b;
            sumDiff += d * d;
        }
        double rmsA = sqrt(sumA / 78125.0);
        double rmsB = sqrt(sumB / 78125.0);
        double diffRms = sqrt(sumDiff / 78125.0);
        char name[64];
        snprintf(name, sizeof(name), "p%d_finite", prog);
        if (finite) {
            printf("PASS %s\n", name);
        } else {
            printf("FAIL %s non-finite or out of range\n", name);
            any_fail = 1;
        }
        snprintf(name, sizeof(name), "p%d_audible", prog);
        if (rmsA > 1e-4 && rmsB > 1e-4) {
            printf("PASS %s\n", name);
        } else {
            printf("FAIL %s rmsA=%.6e rmsB=%.6e\n", name, rmsA, rmsB);
            any_fail = 1;
        }
        snprintf(name, sizeof(name), "p%d_effect_differs", prog);
        if (diffRms > 0.02 * rmsB) {
            printf("PASS %s\n", name);
        } else {
            printf("FAIL %s diffRms=%.6e rmsB=%.6e\n", name, diffRms, rmsB);
            any_fail = 1;
        }
        if (prog == 14 || prog == 15 || prog == 16 || prog == 17) {
            double eA = 0.0;
            double eB = 0.0;
            for (j = 40625; j < 56250; j++) {
                double a = (double)A[j];
                double b = (double)B[j];
                eA += a * a;
                eB += b * b;
            }
            printf("tail energy A=%.6e B=%.6e\n", eA, eB);
            snprintf(name, sizeof(name), "p%d_reverb_tail", prog);
            if (eA > 5.0 * eB) {
                printf("PASS %s\n", name);
            } else {
                printf("FAIL %s eA=%.6e eB=%.6e\n", name, eA, eB);
                any_fail = 1;
            }
        }
        if (prog == 18) {
            double eA = 0.0;
            double eB = 0.0;
            for (j = 38300; j < 41500; j++) {
                double a = (double)A[j];
                double b = (double)B[j];
                eA += a * a;
                eB += b * b;
            }
            printf("er energy A=%.6e B=%.6e\n", eA, eB);
            if (eA > 1e-4 && eA > 10.0 * eB) {
                printf("PASS p18_er_tail\n");
            } else {
                printf("FAIL p18_er_tail eA=%.6e eB=%.6e\n", eA, eB);
                any_fail = 1;
            }
        }
        printf("prog %d rmsA=%.6e rmsB=%.6e diff=%.6e\n", prog, rmsA, rmsB, diffRms);
    }
    if (any_fail) {
        return 1;
    }
    return 0;
}

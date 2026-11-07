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
    if (dry) {
        g_blk[38] = 33;
    }
    g_eng.playProgram(g_blk);
    g_eng.noteOn(0, 60, 100);
    int rendered = 0;
    while (rendered < total) {
        int chunk = 500;
        if (total - rendered < chunk) {
            chunk = total - rendered;
        }
        g_eng.render(outL + rendered, outL + rendered, chunk);
        rendered += chunk;
    }
    g_eng.noteOff(0, 60);
}

int main(void) {
    int any_fail = 0;
    int progs[6] = {8, 9, 10, 11, 12, 13};
    int i;
    for (i = 0; i < 6; i++) {
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
        if (prog == 13) {
            double eA = 0.0;
            double eB = 0.0;
            for (j = 9375; j < 15625; j++) {
                double a = (double)A[j];
                double b = (double)B[j];
                eA += a * a;
                eB += b * b;
            }
            printf("echo energy A=%.6e B=%.6e\n", eA, eB);
            if (eA > 1.5 * eB) {
                printf("PASS p13_echo_energy\n");
            } else {
                printf("FAIL p13_echo_energy eA=%.6e eB=%.6e\n", eA, eB);
                any_fail = 1;
            }
        }
        if (prog == 11) {
            double maxA = 0.0;
            double minA = 1e30;
            double maxB = 0.0;
            double minB = 1e30;
            int w;
            for (w = 0; w < 36; w++) {
                int start = 9375 + w * 781;
                double sA = 0.0;
                double sB = 0.0;
                for (j = start; j < start + 781; j++) {
                    double a = (double)A[j];
                    double b = (double)B[j];
                    sA += a * a;
                    sB += b * b;
                }
                double rA = sqrt(sA / 781.0);
                double rB = sqrt(sB / 781.0);
                if (rA > maxA) maxA = rA;
                if (rA < minA) minA = rA;
                if (rB > maxB) maxB = rB;
                if (rB < minB) minB = rB;
            }
            double ratioA = (minA > 0.0) ? (maxA / minA) : 0.0;
            double ratioB = (minB > 0.0) ? (maxB / minB) : 0.0;
            if (ratioA > 1.15 && ratioB < 1.10) {
                printf("PASS p11_tremolo ratioA=%.4f ratioB=%.4f\n", ratioA, ratioB);
            } else {
                printf("FAIL p11_tremolo ratioA=%.4f ratioB=%.4f\n", ratioA, ratioB);
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

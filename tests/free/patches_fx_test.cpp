#include "m1free_patches_fx.h"
#include <stdio.h>

using namespace m1free;

static int g_failures = 0;

static void check(const char* name, int cond, const char* detail) {
    if (cond) {
        printf("PASS %s\n", name);
    } else {
        printf("FAIL %s %s\n", name, detail);
        g_failures++;
    }
}

int main() {
    static PatchSet p;
    static PatchSet p_ref;

    generatePatches(p_ref);
    generatePatches(p);
    generateFxPatches(p);

    // Program 8 checks
    {
        uint8_t* blk = p.programs + 143 * 8;
        check("prog8_name", memcmp(blk, "Chorus Pad", 10) == 0, "name mismatch");
        check("prog8_fx1_type", blk[38] == 11, "fx1 type != 11");
        check("prog8_fx2_type", blk[39] == 33, "fx2 type != 33");
        check("prog8_bal_l", blk[40] == 40, "bal L != 40");
        check("prog8_bal_r", blk[41] == 40, "bal R != 40");
        check("prog8_switches", blk[46] == 3, "switches != 3");
        check("prog8_p0", blk[47] == 45, "p0 != 45");
        check("prog8_p1", blk[48] == 30, "p1 != 30");
        check("prog8_p4", blk[51] == 15, "p4 != 15");
        check("prog8_wave", blk[12] == 6, "wave != 6");
    }

    // Program 9 checks
    {
        uint8_t* blk = p.programs + 143 * 9;
        check("prog9_fx1_type", blk[38] == 13, "fx1 type != 13");
        check("prog9_bal_l", blk[40] == 50, "bal L != 50");
        check("prog9_p3", blk[50] == 25, "p3 (feedback) != 25");
        check("prog9_p2", blk[49] == 7, "p2 (wave) != 7");
        check("prog9_p4", blk[51] == 6, "p4 (delay) != 6");
    }

    // Program 10 checks
    {
        uint8_t* blk = p.programs + 143 * 10;
        check("prog10_fx1_type", blk[38] == 15, "fx1 type != 15");
        check("prog10_p0", blk[47] == 70, "p0 (depth) != 70");
        check("prog10_p4", blk[51] == 50, "p4 (manual) != 50");
    }

    // Program 11 checks
    {
        uint8_t* blk = p.programs + 143 * 11;
        check("prog11_fx1_type", blk[38] == 17, "fx1 type != 17");
        check("prog11_bal_l", blk[40] == 70, "bal L != 70");
        check("prog11_p1", blk[48] == 60, "p1 (speed) != 60");
    }

    // Program 12 checks
    {
        uint8_t* blk = p.programs + 143 * 12;
        check("prog12_fx1_type", blk[38] == 20, "fx1 type != 20");
        check("prog12_p2", blk[49] == 60, "p2 (drive) != 60");
        check("prog12_p3", blk[50] == 70, "p3 (level) != 70");
    }

    // Program 13 checks
    {
        uint8_t* blk = p.programs + 143 * 13;
        check("prog13_fx1_type", blk[38] == 9, "fx1 type != 9");
        check("prog13_p0", blk[47] == 0xFA, "p0 (delayL low) != 0xFA");
        check("prog13_p1", blk[48] == 0x00, "p1 (delayL high) != 0x00");
        check("prog13_p2", blk[49] == 40, "p2 (feedback) != 40");
        check("prog13_p3", blk[50] == 30, "p3 (high damp) != 30");
        check("prog13_p4", blk[51] == 0x77, "p4 (delayR low) != 0x77");
        check("prog13_p5", blk[52] == 0x01, "p5 (delayR high) != 0x01");
        check("prog13_bal_l", blk[40] == 45, "bal L != 45");
    }

    // Common checks for programs 8..13
    for (int n = 8; n <= 13; n++) {
        uint8_t* blk = p.programs + 143 * n;
        char name[64];
        snprintf(name, sizeof name, "prog%d_osc_mode", n);
        check(name, blk[10] == 0, "osc mode != 0");

        snprintf(name, sizeof name, "prog%d_level", n);
        check(name, blk[86] == 99, "level != 99");

        snprintf(name, sizeof name, "prog%d_switches", n);
        check(name, blk[46] == 3, "switches != 3");
    }

    // Programs 0..7 unchanged
    {
        int ok = 1;
        for (int n = 0; n < 8; n++) {
            if (memcmp(p.programs + 143 * n, p_ref.programs + 143 * n, 143) != 0) {
                ok = 0;
                break;
            }
        }
        check("prog0_7_unchanged", ok, "programs 0..7 modified");
    }

    // Programs 14..99 entirely zero
    {
        int ok = 1;
        for (int n = 14; n < 100; n++) {
            uint8_t* blk = p.programs + 143 * n;
            for (int i = 0; i < 143; i++) {
                if (blk[i] != 0) {
                    ok = 0;
                    break;
                }
            }
            if (!ok) break;
        }
        check("prog14_99_zero", ok, "programs 14..99 not zero");
    }

    // Combinations unchanged
    {
        int ok = (memcmp(p.combis, p_ref.combis, sizeof p.combis) == 0);
        check("combis_unchanged", ok, "combis modified");
    }

    // Kits unchanged
    {
        int ok = (memcmp(p.kits, p_ref.kits, sizeof p.kits) == 0);
        check("kits_unchanged", ok, "kits modified");
    }

    // Global unchanged
    {
        int ok = (memcmp(p.global, p_ref.global, sizeof p.global) == 0);
        check("global_unchanged", ok, "global modified");
    }

    if (g_failures > 0) {
        printf("FAILED: %d check(s) failed\n", g_failures);
        return 1;
    }
    printf("ALL TESTS PASSED\n");
    return 0;
}

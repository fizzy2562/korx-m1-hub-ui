#include "m1free_patches.h"
#include <stdio.h>

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
    static m1free::PatchSet p;
    m1free::generatePatches(p);

    // Program 0 checks
    {
        uint8_t* blk = p.programs + 143 * 0;
        char expected_name[11] = "Sine Pad  ";
        int name_ok = 1;
        for (int i = 0; i < 10; i++) {
            if (blk[i] != (uint8_t)expected_name[i]) {
                name_ok = 0;
                break;
            }
        }
        check("prog0_name", name_ok, "name mismatch");
        check("prog0_byte12", blk[12] == 0, "byte12 != 0");
        check("prog0_byte86", blk[86] == 99, "byte86 != 99");
        check("prog0_byte38", blk[38] == 33, "byte38 != 33");
        check("prog0_byte39", blk[39] == 33, "byte39 != 33");
        check("prog0_byte97", blk[97] == 85, "byte97 != 85");
    }

    // Program 2 checks
    {
        uint8_t* blk = p.programs + 143 * 2;
        check("prog2_byte13", blk[13] == 0xFF, "byte13 != 0xFF");
        check("prog2_byte12", blk[12] == 2, "byte12 != 2");
        check("prog2_byte71", blk[71] == 55, "byte71 != 55");
    }

    // Programs 0..7 common checks
    for (int n = 0; n < 8; n++) {
        uint8_t* blk = p.programs + 143 * n;
        char name_buf[64];
        snprintf(name_buf, sizeof name_buf, "prog%d_byte10", n);
        check(name_buf, blk[10] == 0, "byte10 != 0");

        snprintf(name_buf, sizeof name_buf, "prog%d_byte11", n);
        check(name_buf, blk[11] == 0, "byte11 != 0");

        snprintf(name_buf, sizeof name_buf, "prog%d_byte93", n);
        check(name_buf, blk[93] == 99, "byte93 != 99");

        snprintf(name_buf, sizeof name_buf, "prog%d_byte74", n);
        check(name_buf, blk[74] == 0, "byte74 != 0");

        snprintf(name_buf, sizeof name_buf, "prog%d_byte32", n);
        check(name_buf, blk[32] == 2, "byte32 != 2");

        int sus = blk[97];
        int expected_bp = sus + 10;
        if (expected_bp > 99) expected_bp = 99;
        snprintf(name_buf, sizeof name_buf, "prog%d_byte95", n);
        check(name_buf, blk[95] == (uint8_t)expected_bp, "byte95 != min(99, sus+10)");

        snprintf(name_buf, sizeof name_buf, "prog%d_byte12", n);
        check(name_buf, blk[12] == (uint8_t)n, "byte12 != n");
    }

    // Programs 8..99 all zero
    {
        int all_zero = 1;
        for (int n = 8; n < 100; n++) {
            uint8_t* blk = p.programs + 143 * n;
            for (int i = 0; i < 143; i++) {
                if (blk[i] != 0) {
                    all_zero = 0;
                    break;
                }
            }
            if (!all_zero) break;
        }
        check("progs8_99_zero", all_zero, "non-zero bytes found");
    }

    // Combination 0 checks
    {
        uint8_t* blk = p.combis + 124 * 0;
        char expected_name[11] = "Sine+Saw  ";
        int name_ok = 1;
        for (int i = 0; i < 10; i++) {
            if (blk[i] != (uint8_t)expected_name[i]) {
                name_ok = 0;
                break;
            }
        }
        check("combo0_name", name_ok, "name mismatch");
        check("combo0_byte11", blk[11] == 33, "byte11 != 33");
        check("combo0_t0_prog", blk[36] == 0, "timbre0 program != 0");
        check("combo0_t0_level", blk[37] == 99, "timbre0 level != 99");
        check("combo0_t0_flags", blk[46] == 0x00, "timbre0 flags != 0x00");
        check("combo0_t1_prog", blk[47] == 1, "timbre1 program != 1");
        check("combo0_t1_level", blk[48] == 70, "timbre1 level != 70");
        check("combo0_t1_detune", blk[50] == 7, "timbre1 detune != 7");
        check("combo0_t1_flags", blk[57] == 0x00, "timbre1 flags != 0x00");
        check("combo0_t2_flags", blk[68] == 0x10, "timbre2 flags != 0x10");
    }

    // Combination 1 checks
    {
        uint8_t* blk = p.combis + 124 * 1;
        check("combo1_t0_keytop", blk[36 + 5] == 59, "timbre0 key top != 59");
        check("combo1_t1_keybot", blk[47 + 6] == 60, "timbre1 key bottom != 60");
    }

    // Combination 2 checks
    {
        uint8_t* blk = p.combis + 124 * 2;
        check("combo2_t1_transpose", blk[47 + 2] == 12, "timbre1 transpose != 12");
    }

    // Combination 3 checks
    {
        uint8_t* blk = p.combis + 124 * 3;
        check("combo3_t2_detune", blk[58 + 3] == 0xFA, "timbre2 detune != 0xFA");
    }

    // Combinations 4..99 all zero
    {
        int all_zero = 1;
        for (int n = 4; n < 100; n++) {
            uint8_t* blk = p.combis + 124 * n;
            for (int i = 0; i < 124; i++) {
                if (blk[i] != 0) {
                    all_zero = 0;
                    break;
                }
            }
            if (!all_zero) break;
        }
        check("combos4_99_zero", all_zero, "non-zero bytes found");
    }

    // Kits all zero
    {
        int all_zero = 1;
        for (int i = 0; i < 4 * 210; i++) {
            if (p.kits[i] != 0) {
                all_zero = 0;
                break;
            }
        }
        check("kits_zero", all_zero, "non-zero bytes found");
    }

    // Global all zero
    {
        int all_zero = 1;
        for (int i = 0; i < 21; i++) {
            if (p.global[i] != 0) {
                all_zero = 0;
                break;
            }
        }
        check("global_zero", all_zero, "non-zero bytes found");
    }

    if (g_failures > 0) {
        printf("TOTAL FAILURES: %d\n", g_failures);
        return 1;
    }
    printf("ALL TESTS PASSED\n");
    return 0;
}

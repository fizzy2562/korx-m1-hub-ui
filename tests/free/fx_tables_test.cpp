#include "m1free_fx.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>

static int g_failures = 0;

static void check(bool cond, const char* name, const char* detail) {
    if (cond) {
        printf("PASS %s\n", name);
    } else {
        printf("FAIL %s %s\n", name, detail);
        g_failures++;
    }
}

static int abs_diff_int(int a, int b) {
    int d = a - b;
    return d < 0 ? -d : d;
}

int main() {
    static m1::FxTables t;
    m1free::generateFxTablesBasic(t);

    const double PI = 3.14159265358979323846;

    // --- wt checks ---
    {
        double expected_wt0 = sin(PI * 0.5 / 128.0);
        expected_wt0 = expected_wt0 * expected_wt0;
        double diff = (double)t.wt[0] - expected_wt0;
        if (diff < 0) diff = -diff;
        check(diff < 1e-6, "wt[0]", "value mismatch");
    }
    {
        double diff = (double)t.wt[64] - 1.0;
        if (diff < 0) diff = -diff;
        check(diff < 2e-4, "wt[64]", "value mismatch");
    }
    check(t.wt[128] == t.wt[0], "wt[128]==wt[0]", "mismatch");
    check(t.wt[129] == 0.0f, "wt[129]==0", "not zero");
    check(t.wt[0x102] == 0.0f, "wt[0x102]==0", "not zero");
    check(t.wt[0x102 + 64] == 1.0f, "wt[0x102+64]==1", "not one");
    check(t.wt[0x102 + 128] == 0.0f, "wt[0x102+128]==0", "not zero");
    {
        double diff = (double)t.wt[0x102 + 32] - 0.5;
        if (diff < 0) diff = -diff;
        check(diff < 1e-6, "wt[0x102+32]==0.5", "value mismatch");
    }

    // --- wt2 checks ---
    {
        double expected_wt2_0 = sin(PI * 0.5 / 128.0);
        expected_wt2_0 = expected_wt2_0 * expected_wt2_0;
        double diff = (double)t.wt2[0] - expected_wt2_0;
        if (diff < 0) diff = -diff;
        check(diff < 1e-6, "wt2[0]", "value mismatch");
    }
    {
        double expected_wt2_32 = sin(PI * 64.5 / 128.0);
        expected_wt2_32 = expected_wt2_32 * expected_wt2_32;
        double diff = (double)t.wt2[32] - expected_wt2_32;
        if (diff < 0) diff = -diff;
        check(diff < 1e-6, "wt2[32]", "value mismatch");
    }
    {
        double diff = (double)t.wt2[64] - (double)t.wt2[0];
        if (diff < 0) diff = -diff;
        check(diff < 1e-6, "wt2[64]==wt2[0]", "mismatch");
    }

    // --- lfo_inc checks ---
    check(t.lfo_inc[0] == (3 * 100) / 298, "lfo_inc[0]", "value mismatch");
    check(t.lfo_inc[99] == (300 * 100) / 298, "lfo_inc[99]", "value mismatch");
    check(t.lfo_inc[100] == ((10 + 0x136) * 100) / 298, "lfo_inc[100]", "value mismatch");
    check(t.lfo_inc[199] == ((1000 + 0x136) * 100) / 298, "lfo_inc[199]", "value mismatch");
    check(t.lfo_inc[216] == ((17 * 100 + 0x578) * 100) / 298, "lfo_inc[216]", "value mismatch");
    {
        bool non_decreasing = true;
        for (int v = 1; v <= 216; v++) {
            if (t.lfo_inc[v] < t.lfo_inc[v - 1]) {
                non_decreasing = false;
                break;
            }
        }
        check(non_decreasing, "lfo_inc non-decreasing", "sequence not non-decreasing");
    }

    // --- dly_a / dly_b checks ---
    check(t.dly_a[0] == 0.0f, "dly_a[0]==0", "not zero");
    check(t.dly_b[0] == 1.0f, "dly_b[0]==1", "not one");
    {
        bool sum_ok = true;
        bool range_ok = true;
        for (int hd = 0; hd <= 99; hd++) {
            double sum = (double)t.dly_a[hd] + (double)t.dly_b[hd] - 1.0;
            if (sum < 0) sum = -sum;
            if (sum >= 1e-6) {
                sum_ok = false;
                break;
            }
            if ((double)t.dly_a[hd] < 0.0 || (double)t.dly_a[hd] > 0.75) {
                range_ok = false;
                break;
            }
        }
        check(sum_ok, "dly_a+dly_b==1", "sum not 1");
        check(range_ok, "dly_a in [0,0.75]", "out of range");
    }

    // --- trem_k / exc_k checks ---
    check(t.trem_k == 0.01f, "trem_k", "value mismatch");
    check(t.exc_k == 0.0078f, "exc_k", "value mismatch");

    // --- eq_k checks ---
    {
        int diff = abs_diff_int((int)t.eq_k[2], 6691);
        check(diff <= 2, "eq_k[2]~6691", "value mismatch");
    }
    {
        bool strictly_decreasing = true;
        for (int i = 1; i < 5; i++) {
            if (t.eq_k[i] >= t.eq_k[i - 1]) {
                strictly_decreasing = false;
                break;
            }
        }
        check(strictly_decreasing, "eq_k strictly decreasing", "not strictly decreasing");
    }

    // --- eq_gain checks ---
    check(t.eq_gain[0] == 32767, "eq_gain[0]==32767", "value mismatch");
    {
        int diff = abs_diff_int((int)t.eq_gain[6], 16423);
        check(diff <= 2, "eq_gain[6]~16423", "value mismatch");
    }
    {
        int diff = abs_diff_int((int)t.eq_gain[12], 8231);
        check(diff <= 2, "eq_gain[12]~8231", "value mismatch");
    }
    {
        bool strictly_decreasing = true;
        for (int a = 1; a <= 12; a++) {
            if (t.eq_gain[a] >= t.eq_gain[a - 1]) {
                strictly_decreasing = false;
                break;
            }
        }
        check(strictly_decreasing, "eq_gain strictly decreasing", "not strictly decreasing");
    }

    if (g_failures > 0) {
        printf("TOTAL FAILURES: %d\n", g_failures);
        return 1;
    }
    printf("ALL TESTS PASSED\n");
    return 0;
}

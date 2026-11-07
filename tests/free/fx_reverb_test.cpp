#include "m1free_fx_reverb.h"
#include <stdio.h>
#include <stdint.h>

static m1::FxTables t;

int main() {
    int failures = 0;
    
    m1free::generateFxTablesReverb(t);
    
    // 1. Reverb Delays and Taps
    for (int ty = 0; ty < 6; ++ty) {
        m1::FxRevType& r = t.rev[ty];
        int ok = 1;
        for (int k = 0; k < 4; ++k) {
            if (r.comb_d[k] < 64 || r.comb_d[k] > 4500) ok = 0;
            if (r.tap_l[k] > r.comb_d[k] || r.tap_r[k] > r.comb_d[k]) ok = 0;
            if (r.tap_l[k] < 1) ok = 0;
            if (r.ap_d[k] < 8) ok = 0;
        }
        if (ok) {
            printf("PASS rev_delays_ty%d\n", ty);
        } else {
            printf("FAIL rev_delays_ty%d invalid delays/taps\n", ty);
            failures++;
        }
    }
    
    // 2. Memory Budget
    for (int ty = 0; ty < 6; ++ty) {
        m1::FxRevType& r = t.rev[ty];
        int maxTap = 6256 + r.base_ap;
        if (6256 + r.base_erc > maxTap) maxTap = 6256 + r.base_erc;
        if (6256 + r.base_erl > maxTap) maxTap = 6256 + r.base_erl;
        if (6256 + r.base_err > maxTap) maxTap = 6256 + r.base_err;
        
        int total = (maxTap + 2);
        for (int k = 0; k < 4; ++k) {
            total += (r.ap_d[k] + 2);
            int max_cd = r.comb_d[k];
            if (r.tap_l[k] > max_cd) max_cd = r.tap_l[k];
            if (r.tap_r[k] > max_cd) max_cd = r.tap_r[k];
            total += (max_cd + 2);
        }
        
        if (total <= 32768) {
            printf("PASS mem_budget_ty%d total=%d\n", ty, total);
        } else {
            printf("FAIL mem_budget_ty%d total=%d > 32768\n", ty, total);
            failures++;
        }
    }
    
    // 3. fb and damp ranges/monotonicity
    for (int ty = 0; ty < 6; ++ty) {
        m1::FxRevType& r = t.rev[ty];
        int ok_fb = 1;
        for (int k = 0; k < 4; ++k) {
            for (int rt = 0; rt < 98; ++rt) {
                if (r.fb[k][rt] <= 0.0f || r.fb[k][rt] > 0.97f) ok_fb = 0;
                if (rt > 0 && r.fb[k][rt] < r.fb[k][rt-1]) ok_fb = 0;
            }
        }
        if (ok_fb) {
            printf("PASS fb_ty%d\n", ty);
        } else {
            printf("FAIL fb_ty%d range/monotonicity\n", ty);
            failures++;
        }
        
        int ok_damp = 1;
        for (int k = 0; k < 4; ++k) {
            if (r.damp[k][0] != 0.0f) ok_damp = 0;
            for (int hd = 0; hd < 100; ++hd) {
                if (r.damp[k][hd] < 0.0f || r.damp[k][hd] > 0.7f) ok_damp = 0;
                if (hd > 0 && r.damp[k][hd] < r.damp[k][hd-1]) ok_damp = 0;
            }
        }
        if (ok_damp) {
            printf("PASS damp_ty%d\n", ty);
        } else {
            printf("FAIL damp_ty%d range/monotonicity\n", ty);
            failures++;
        }
    }
    
    // 4. T60 Recovery
    // ty 0 (Hall), rt 97, k 0. Target T60 = 8.0
    {
        m1::FxRevType& r = t.rev[0];
        float fb = r.fb[0][97];
        if (fb == 0.97f) {
            printf("PASS t60_hall_rt97 (clamped)\n");
        } else {
            double T60 = -3.0 * (double)r.comb_d[0] / (31250.0 * log10((double)fb * 0.9668));
            if (fabs(T60 - 8.0) <= 0.4) { // 5% of 8.0
                printf("PASS t60_hall_rt97 T60=%.4f\n", T60);
            } else {
                printf("FAIL t60_hall_rt97 T60=%.4f (expected ~8.0)\n", T60);
                failures++;
            }
        }
    }
    
    // ty 3 (Room), rt 0. Target T60 = 0.25
    {
        m1::FxRevType& r = t.rev[3];
        float fb = r.fb[0][0];
        if (fb == 0.97f) {
            printf("PASS t60_room_rt0 (clamped)\n");
        } else {
            double T60 = -3.0 * (double)r.comb_d[0] / (31250.0 * log10((double)fb * 0.9668));
            if (fabs(T60 - 0.25) <= 0.025) { // 10% of 0.25
                printf("PASS t60_room_rt0 T60=%.4f\n", T60);
            } else {
                printf("FAIL t60_room_rt0 T60=%.4f (expected ~0.25)\n", T60);
                failures++;
            }
        }
    }
    
    // 5. Early Reflections
    {
        int ok_erT = 1;
        for (int k = 0; k < 25; ++k) {
            if (t.er_T[k] < 0.02f || t.er_T[k] > 1.0f) ok_erT = 0;
        }
        if (t.er_T[24] <= t.er_T[0]) ok_erT = 0;
        if (ok_erT) {
            printf("PASS er_T\n");
        } else {
            printf("FAIL er_T range/order\n");
            failures++;
        }
        
        double sum_A = (double)t.er_A[0] + (double)t.er_A[1] + (double)t.er_A[2] + (double)t.er_A[3];
        if (sum_A * 80.0 + 12.0 <= 1500.0) {
            printf("PASS er_A_sum\n");
        } else {
            printf("FAIL er_A_sum %.2f > 1500\n", sum_A * 80.0 + 12.0);
            failures++;
        }
        
        if (fabs((double)t.er[0].tg[0] - 0.40) < 1e-6) {
            printf("PASS er0_tg0\n");
        } else {
            printf("FAIL er0_tg0 %.6f\n", (double)t.er[0].tg[0]);
            failures++;
        }
        
        if (t.er[2].tg[22] == 0.0f && fabs((double)t.er[2].tg[21] - 0.45) < 1e-6) {
            printf("PASS er2_tg\n");
        } else {
            printf("FAIL er2_tg tg22=%.6f tg21=%.6f\n", (double)t.er[2].tg[22], (double)t.er[2].tg[21]);
            failures++;
        }
    }
    
    // 6. Untouched members
    if (t.wt[0] == 0 && t.lfo_inc[5] == 0) {
        printf("PASS untouched\n");
    } else {
        printf("FAIL untouched wt[0]=%g lfo_inc[5]=%d\n", (double)t.wt[0], (int)t.lfo_inc[5]);
        failures++;
    }
    
    if (failures > 0) {
        printf("TOTAL FAILURES: %d\n", failures);
        return 1;
    }
    
    printf("ALL TESTS PASSED\n");
    return 0;
}

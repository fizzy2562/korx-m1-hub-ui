#pragma once

#include "m1_engine/m1_fx.h"
#include <math.h>
#include <stdint.h>

namespace m1free {

static inline long fxrnd(double x) {
    return (long)(x + 0.5);
}

inline void generateFxTablesBasic(m1::FxTables& t) {
    // 1. wt: SINE segment (wt[0..128]) and TRIANGLE segment (wt[0x102..0x102+128])
    // Zero the whole array first
    for (int i = 0; i < 0x102 + 130; i++) {
        t.wt[i] = 0.0f;
    }

    // SINE segment: wt[j] = sin(pi*(j + 0.5)/128)^2 for j = 0..128
    for (int j = 0; j <= 128; j++) {
        double angle = 3.14159265358979323846 * (j + 0.5) / 128.0;
        double s = sin(angle);
        t.wt[j] = (float)(s * s);
    }

    // TRIANGLE segment: wt[0x102 + j] for j = 0..128
    for (int j = 0; j <= 128; j++) {
        double tri;
        if (j <= 64) {
            tri = (double)j / 64.0;
        } else {
            tri = (128.0 - (double)j) / 64.0;
        }
        t.wt[0x102 + j] = (float)tri;
    }

    // 2. wt2[j] for j = 0..129: sin(pi*(2*j + 0.5)/128)^2
    for (int j = 0; j <= 129; j++) {
        double angle = 3.14159265358979323846 * (2.0 * (double)j + 0.5) / 128.0;
        double s = sin(angle);
        t.wt2[j] = (float)(s * s);
    }

    // 3. lfo_inc[v] for v = 0..216
    for (int v = 0; v <= 216; v++) {
        int32_t raw;
        if (v <= 99) {
            raw = 3 * (v + 1);
        } else if (v <= 199) {
            raw = (v - 99) * 10 + 0x136;
        } else {
            raw = (v - 199) * 100 + 0x578;
        }
        t.lfo_inc[v] = (int32_t)(raw * 100 / 0x12A);
    }

    // 4. dly_a[hd], dly_b[hd] for hd = 0..99
    for (int hd = 0; hd <= 99; hd++) {
        double x = (double)hd / 99.0;
        double a = 0.75 * pow(x, 1.3);
        double b = 1.0 - a;
        t.dly_a[hd] = (float)a;
        t.dly_b[hd] = (float)b;
    }

    // 5. trem_k and exc_k
    t.trem_k = 0.01f;
    t.exc_k = 0.0078f;

    // 6. eq_k[i] for i = 0..4
    const double fc[5] = {250.0, 500.0, 1000.0, 2000.0, 4000.0};
    for (int i = 0; i < 5; i++) {
        double T = tan(3.14159265358979323846 * fc[i] / 31250.0);
        double k = (1.0 - T) / (1.0 + T);
        t.eq_k[i] = (int16_t)fxrnd(8192.0 * k);
    }

    // 7. eq_gain[a] for a = 0..12
    for (int a = 0; a <= 12; a++) {
        if (a == 0) {
            t.eq_gain[a] = 32767;
        } else {
            double g = pow(10.0, -(double)a / 20.0);
            t.eq_gain[a] = (int16_t)fxrnd(32768.0 * g);
        }
    }
}

} // namespace m1free

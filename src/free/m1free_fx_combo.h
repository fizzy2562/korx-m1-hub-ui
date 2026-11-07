#pragma once
#include "m1_engine/m1_fx.h"
#include <math.h>

namespace m1free {

static inline long fxcb_rnd(double x) {
    return (long)floor(x + 0.5);
}

inline void generateFxTablesCombo(m1::FxTables& t) {
    for (int i = 0; i < 2; i++) {
        m1::FxComboRev& r = t.crev[i];
        float s = (i == 0) ? 1.0f : 0.5f;
        double Tmin = (i == 0) ? 0.6 : 0.25;
        double Tmax = (i == 0) ? 8.0 : 3.0;

        r.g[0] = 0.55f;
        r.g[1] = 0.5f;
        r.g[2] = 0.5f;
        r.ger[0] = 0.30f;
        r.ger[1] = 0.25f;
        r.cout[0] = 0.25f;
        r.cout[1] = 0.25f;
        r.cout[2] = 0.25f;
        r.cout[3] = 0.25f;
        r.g_in = 0.45f;

        double a = 0.5 + 0.5 * (s < 1.0 ? s : 1.0);
        r.ap_d[0] = (int16_t)fxcb_rnd(142.0 * a);
        r.ap_d[1] = (int16_t)fxcb_rnd(277.0 * a);
        r.ap_d[2] = (int16_t)fxcb_rnd(107.0 * a);
        if (r.ap_d[0] < 8) r.ap_d[0] = 8;
        if (r.ap_d[1] < 8) r.ap_d[1] = 8;
        if (r.ap_d[2] < 8) r.ap_d[2] = 8;

        const long cb0[4] = {1687, 1601, 2053, 2251};
        const double tr[4] = {0.37, 0.63, 0.50, 0.45};
        for (int k = 0; k < 4; k++) {
            long cd = fxcb_rnd((double)cb0[k] * s);
            if (cd < 64) cd = 64;
            r.comb_d[k] = (int16_t)cd;
            r.comb_tap[k] = (int16_t)fxcb_rnd((double)cd * tr[k]);
        }

        r.eq[0] = 0.0f;
        r.eq[1] = 0.0f;
        r.eq[2] = 0.0f;
        r.eq[3] = 0.0f;
        r.eq[4] = 1.0f;
        r.eq[5] = 1.0f;

        r.base_ap = 0;
        r.base_c = (int16_t)fxcb_rnd(300.0 * s);
        r.base_b = (int16_t)fxcb_rnd(470.0 * s);

        for (int k = 0; k < 4; k++) {
            for (int rt = 0; rt < 98; rt++) {
                double T60;
                if (i == 1 && rt > 47) {
                    T60 = Tmin * pow(Tmax / Tmin, 47.0 / 47.0);
                } else {
                    double denom = (i == 1 && rt <= 47) ? 47.0 : 97.0;
                    T60 = Tmin * pow(Tmax / Tmin, (double)rt / denom);
                }
                double val = pow(10.0, -3.0 * (double)r.comb_d[k] / (31250.0 * T60)) / 0.9668;
                if (val > 0.97) val = 0.97;
                r.fb[k][rt] = (float)val;
            }
            for (int hd = 0; hd < 100; hd++) {
                double val = 0.7 * pow((double)hd / 99.0, 1.2) * (1.0 + 0.02 * k);
                if (val > 0.7) val = 0.7;
                r.damp[k][hd] = (float)val;
            }
        }
    }

    m1::FxComboEr& e = t.cer;
    e.g[0] = 0.50f;
    e.g[1] = 0.45f;
    e.g[2] = 0.40f;
    e.g[3] = 0.40f;
    e.g_in = 0.5f;
    for (int k = 0; k < 25; k++) {
        e.tg[k] = 0.40f * (float)exp(-0.12 * k) * ((k % 3 == 1) ? -1.0f : 1.0f);
        double T = 0.04 + 0.96 * pow((double)k / 24.0, 0.9) + 0.012 * sin(k * 5.1);
        if (T < 0.02) T = 0.02;
        if (T > 1.0) T = 1.0;
        e.T[k] = (float)T;
    }
    e.A[0] = 3.1f;
    e.A[1] = 2.3f;
    e.A[2] = 4.7f;
    e.A[3] = 3.9f;
}

} // namespace m1free

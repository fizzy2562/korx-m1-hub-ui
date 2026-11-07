#pragma once

#include "m1_engine/m1_fx.h"
#include <math.h>

namespace m1free {

static inline long fxrv_rnd(double x) { return (long)floor(x + 0.5); }

inline void generateFxTablesReverb(m1::FxTables& t) {
    // A. Reverbs
    const double s[6] = {1.00, 1.15, 1.35, 0.35, 0.60, 0.80};
    const double Tmin[6] = {0.6, 0.6, 0.8, 0.25, 0.4, 0.3};
    const double Tmax[6] = {8.0, 9.0, 10.0, 2.5, 4.0, 3.5};
    const double erl[6] = {0.45, 0.45, 0.45, 0.60, 0.55, 0.50};
    
    const int cb0[4] = {1687, 1601, 2053, 2251};
    const int ab0[4] = {142, 107, 379, 277};
    
    for (int ty = 0; ty < 6; ++ty) {
        m1::FxRevType& r = t.rev[ty];
        
        // Delays
        for (int k = 0; k < 4; ++k) {
            long cd = fxrv_rnd((double)cb0[k] * s[ty]);
            if (cd < 64) cd = 64;
            r.comb_d[k] = (int16_t)cd;
            r.tap_l[k] = (int16_t)fxrv_rnd((double)r.comb_d[k] * 0.37);
            r.tap_r[k] = (int16_t)fxrv_rnd((double)r.comb_d[k] * 0.63);
            
            double ap_scale = 0.5 + 0.5 * (s[ty] < 1.0 ? s[ty] : 1.0);
            long ad = fxrv_rnd((double)ab0[k] * ap_scale);
            if (ad < 8) ad = 8;
            r.ap_d[k] = (int16_t)ad;
        }
        
        // Gains
        r.g[0] = 0.55f;
        r.g[1] = 0.50f;
        r.g[2] = 0.50f;
        r.g[3] = 0.50f;
        
        for (int k = 0; k < 4; ++k) {
            r.comb_out[2*k] = 0.25f * (k % 2 == 0 ? 1.0f : -1.0f);
            r.comb_out[2*k+1] = 0.25f * (k < 2 ? 1.0f : -1.0f);
        }
        r.g_in = 0.45f;
        
        // Base delays
        r.base_ap = 0;
        r.base_erc = (int16_t)fxrv_rnd(450.0 * s[ty]);
        r.base_erl = (int16_t)fxrv_rnd(300.0 * s[ty]);
        r.base_err = (int16_t)fxrv_rnd(620.0 * s[ty]);
        
        // ER Scale
        r.er_scale[0] = (float)erl[ty];
        r.er_scale[1] = (float)(0.8888888888888888 * erl[ty]);
        r.er_scale[2] = (float)(0.8888888888888888 * erl[ty]);
        
        // Feedback (fb)
        for (int rt = 0; rt < 98; ++rt) {
            double T60 = Tmin[ty] * pow(Tmax[ty] / Tmin[ty], (double)rt / 97.0);
            for (int k = 0; k < 4; ++k) {
                double val = pow(10.0, -3.0 * (double)r.comb_d[k] / (31250.0 * T60)) / 0.9668;
                if (val > 0.97) val = 0.97;
                r.fb[k][rt] = (float)val;
            }
        }
        
        // Damping
        for (int hd = 0; hd < 100; ++hd) {
            for (int k = 0; k < 4; ++k) {
                double val = 0.7 * pow((double)hd / 99.0, 1.2) * (1.0 + 0.02 * (double)k);
                if (val > 0.7) val = 0.7;
                r.damp[k][hd] = (float)val;
            }
        }
    }
    
    // B. Early Reflections
    for (int et = 0; et < 3; ++et) {
        m1::FxErType& e = t.er[et];
        e.g[0] = 0.50f;
        e.g[1] = 0.45f;
        e.g[2] = 0.40f;
        e.g[3] = 0.40f;
        
        for (int k = 0; k < 25; ++k) {
            if (et == 0) {
                e.tg[k] = (float)(0.40 * exp(-0.12 * (double)k));
            } else if (et == 1) {
                double sign = (k % 2 == 0) ? 1.0 : -1.0;
                e.tg[k] = (float)(0.35 * exp(-0.08 * (double)k) * sign);
            } else { // et == 2
                if (k < 22) {
                    e.tg[k] = (float)(0.05 + 0.40 * (double)k / 21.0);
                } else {
                    e.tg[k] = 0.0f;
                }
            }
        }
    }
    
    // C. Shared
    for (int k = 0; k < 25; ++k) {
        double val = 0.04 + 0.96 * pow((double)k / 24.0, 0.9) + 0.012 * sin((double)k * 5.1);
        if (val < 0.02) val = 0.02;
        if (val > 1.0) val = 1.0;
        t.er_T[k] = (float)val;
    }
    
    t.er_A[0] = 1.3f;
    t.er_A[1] = 0.9f;
    t.er_A[2] = 2.2f;
    t.er_A[3] = 1.7f;
}

} // namespace m1free

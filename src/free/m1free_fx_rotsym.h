#pragma once

#include "m1_engine/m1_fx.h"
#include <math.h>

namespace m1free {

static inline long fxrs_rnd(double x) {
    return (long)floor(x + 0.5);
}

inline void generateFxTablesRotSym(m1::FxTables& t) {
    // --- Symphonic (FxSym) ---
    // Voice spacing 120 degrees: off = {0, 43, 85}
    t.sym.off[0] = 0;
    t.sym.off[1] = 43;
    t.sym.off[2] = 85;

    // mix2 = {0.5, 0.35, 0.5}
    t.sym.mix2[0] = 0.5f;
    t.sym.mix2[1] = 0.35f;
    t.sym.mix2[2] = 0.5f;

    // baseL = {180, 260, 340}, baseR = {340, 180, 260}
    t.sym.baseL[0] = 180;
    t.sym.baseL[1] = 260;
    t.sym.baseL[2] = 340;

    t.sym.baseR[0] = 340;
    t.sym.baseR[1] = 180;
    t.sym.baseR[2] = 260;

    // inc[0] = 12, inc[1] = 47
    t.sym.inc[0] = 12;
    t.sym.inc[1] = 47;

    // --- Rotary (FxRotary) ---
    // HP crossover (horn band) with a=0.85
    // c20=-1.0f, c40=-(1+a)/2/0.966797, c28=a/(-0.966797)
    t.rot.c20 = -1.0f;
    t.rot.c40 = -(1.0f + 0.85f) / 2.0f / 0.966797f;
    t.rot.c28 = 0.85f / (-0.966797f);

    // LP (drum band) with a=0.85
    // c24=1.0f, c44=(1-a)/2, c2c=a
    t.rot.c24 = 1.0f;
    t.rot.c44 = (1.0f - 0.85f) / 2.0f;
    t.rot.c2c = 0.85f;

    // c30=0.0f, c38=0.0f, h_fb_tap=8
    t.rot.c30 = 0.0f;
    t.rot.c38 = 0.0f;
    t.rot.h_fb_tap = 8;

    // h_w=0, h_base=20, h_fix=45, d_w=0, d_base=24
    t.rot.h_w = 0;
    t.rot.h_base = 20;
    t.rot.h_fix = 45;
    t.rot.d_w = 0;
    t.rot.d_base = 24;

    // micL = {12,30,52,90,140}, micR = {20,44,66,118,170}
    t.rot.micL[0] = 12;
    t.rot.micL[1] = 30;
    t.rot.micL[2] = 52;
    t.rot.micL[3] = 90;
    t.rot.micL[4] = 140;

    t.rot.micR[0] = 20;
    t.rot.micR[1] = 44;
    t.rot.micR[2] = 66;
    t.rot.micR[3] = 118;
    t.rot.micR[4] = 170;

    // g120=0.25f, g124=0.35f, g130=0.35f, g134=0.25f
    t.rot.g120 = 0.25f;
    t.rot.g124 = 0.35f;
    t.rot.g130 = 0.35f;
    t.rot.g134 = 0.25f;

    // accel = {210, 410}
    t.rot.accel[0] = 210;
    t.rot.accel[1] = 410;

    // slow[i] = fxrs_rnd(30 * 2^((i-10)/10.0)), fast[i] = fxrs_rnd(250 * 2^((i-10)/10.0)) for i=0..20
    for (int i = 0; i < 21; i++) {
        double exp_val = (i - 10) / 10.0;
        t.rot.slow[i] = (int16_t)fxrs_rnd(30.0 * pow(2.0, exp_val));
        t.rot.fast[i] = (int16_t)fxrs_rnd(250.0 * pow(2.0, exp_val));
    }

    // depth_k = {0.3f, 0.4f, 20.0f, 8.0f}
    t.rot.depth_k[0] = 0.3f;
    t.rot.depth_k[1] = 0.4f;
    t.rot.depth_k[2] = 20.0f;
    t.rot.depth_k[3] = 8.0f;
}

} // namespace m1free

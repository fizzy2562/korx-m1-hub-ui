#pragma once

#include "m1free_patches.h"

namespace m1free {

static inline void setFx1(uint8_t* blk, int type, int bal, const uint8_t* p8) {
    put(blk, 38, type);
    put(blk, 39, 33);
    put(blk, 40, bal);
    put(blk, 41, bal);
    put(blk, 42, 0);
    put(blk, 43, 0);
    put(blk, 44, 0);
    put(blk, 45, 0);
    put(blk, 46, 0x03);
    for (int k = 0; k < 8; k++) {
        put(blk, 47 + k, p8[k]);
    }
}

inline void generateFxPatches(PatchSet& p) {
    // Program 8: "Chorus Pad"
    {
        uint8_t* blk = p.programs + 143 * 8;
        makeProgram(blk, "Chorus Pad", 6, 0, 80, 20, 35, 80, 30);
        uint8_t p8[8] = {45, 30, 0, 0, 15, 0, 0, 0};
        setFx1(blk, 11, 40, p8);
    }

    // Program 9: "Flange Saw"
    {
        uint8_t* blk = p.programs + 143 * 9;
        makeProgram(blk, "Flange Saw", 1, 0, 80, 2, 30, 80, 20);
        uint8_t p8[8] = {55, 12, 7, 25, 6, 0, 0, 0};   // wave 7 = unipolar triangle: a sine+offset LFO can dip below zero and make the modulated read extrapolate, which rings at Nyquist with feedback
        setFx1(blk, 13, 50, p8);
    }

    // Program 10: "Phase Keys"
    {
        uint8_t* blk = p.programs + 143 * 10;
        makeProgram(blk, "Phase Keys", 4, 0, 99, 0, 0, 99, 5);
        uint8_t p8[8] = {70, 25, 0, 30, 50, 0, 0, 0};
        setFx1(blk, 15, 50, p8);
    }

    // Program 11: "Trem Organ"
    {
        uint8_t* blk = p.programs + 143 * 11;
        makeProgram(blk, "Trem Organ", 4, 0, 99, 0, 0, 99, 5);
        uint8_t p8[8] = {80, 60, 0, 0, 0, 0, 0, 0};
        setFx1(blk, 17, 70, p8);
    }

    // Program 12: "Drive Lead"
    {
        uint8_t* blk = p.programs + 143 * 12;
        makeProgram(blk, "Drive Lead", 1, 0, 90, 2, 30, 80, 20);
        uint8_t p8[8] = {0, 0, 60, 70, 0, 0, 0, 0};
        setFx1(blk, 20, 70, p8);
    }

    // Program 13: "Echo Pluck"
    {
        uint8_t* blk = p.programs + 143 * 13;
        makeProgram(blk, "Echo Pluck", 3, 0, 99, 0, 45, 10, 40);
        uint8_t p8[8] = {250 & 0xFF, 250 >> 8, 40, 30, 375 & 0xFF, 375 >> 8, 0, 0};
        setFx1(blk, 9, 45, p8);
    }
}

} // namespace m1free

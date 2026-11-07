#pragma once

#include "m1free_patches_fx.h"

namespace m1free {

inline void generateFxPatches2(PatchSet& p) {
    // Program 14: "Hall Pad"
    {
        uint8_t* blk = p.programs + 143 * 14;
        makeProgram(blk, "Hall Pad", 6, 0, 85, 25, 40, 85, 45);
        uint8_t p8[8] = {60, 0, 40, 20, 40, 0, 0, 0};
        setFx1(blk, 0, 35, p8);
    }

    // Program 15: "Room Keys"
    {
        uint8_t* blk = p.programs + 143 * 15;
        makeProgram(blk, "Room Keys", 4, 0, 99, 0, 0, 99, 5);
        uint8_t p8[8] = {25, 0, 30, 5, 50, 0, 0, 0};
        setFx1(blk, 3, 30, p8);
    }

    // Program 16: "Stage Lead"
    {
        uint8_t* blk = p.programs + 143 * 16;
        makeProgram(blk, "Stage Lead", 1, 0, 90, 2, 30, 80, 20);
        uint8_t p8[8] = {40, 0, 35, 10, 45, 0, 0, 0};
        setFx1(blk, 5, 30, p8);
    }

    // Program 17: "Conc Strings"
    {
        uint8_t* blk = p.programs + 143 * 17;
        makeProgram(blk, "Conc Strings", 6, 0, 70, 30, 35, 85, 40);
        uint8_t p8[8] = {75, 0, 45, 30, 35, 0, 0, 0};
        setFx1(blk, 2, 40, p8);
    }

    // Program 18: "Early Bell"
    {
        uint8_t* blk = p.programs + 143 * 18;
        makeProgram(blk, "Early Bell", 3, 0, 99, 0, 45, 10, 40);
        uint8_t p8[8] = {40, 15, 0, 0, 0, 0, 0, 0};
        setFx1(blk, 6, 45, p8);
    }
}

} // namespace m1free

#pragma once

#include <stdint.h>
#include <string.h>

namespace m1free {

struct PatchSet {
    uint8_t programs[100 * 143];
    uint8_t combis[100 * 124];
    uint8_t kits[4 * 210];
    uint8_t global[21];
};

inline void put(uint8_t* blk, int off, int v) {
    blk[off] = (uint8_t)v;
}

inline void setName(uint8_t* blk, const char* s) {
    bool ended = false;
    for (int i = 0; i < 10; i++) {
        if (!ended && s[i] == '\0') ended = true;
        blk[i] = ended ? (uint8_t)' ' : (uint8_t)s[i];
    }
}

inline void makeProgram(uint8_t* blk, const char* name, int wave, int octave, int cutoff, int atk, int dcy, int sus, int rel) {
    memset(blk, 0, 143);
    setName(blk, name);
    put(blk, 10, 0);   // osc mode
    put(blk, 11, 0);   // assign/hold
    put(blk, 12, wave); // OSC1 multisound
    put(blk, 13, octave); // OSC1 octave
    put(blk, 86, 99);  // OSC1 level
    put(blk, 71, cutoff); // VDF1 cutoff
    put(blk, 72, 60);  // VDF1 keyboard centre note
    put(blk, 73, 0);   // VDF1 keyboard track
    put(blk, 74, 0);   // VDF1 EG intensity
    put(blk, 87, 60);  // VDA1 keyboard centre note
    put(blk, 88, 0);   // VDA1 keyboard track
    put(blk, 89, 20);  // VDA1 velocity sense
    put(blk, 92, atk); // VDA1 EG attack time
    put(blk, 93, 99);  // VDA1 EG attack level
    put(blk, 94, dcy); // VDA1 EG decay time
    int bp = sus + 10;
    if (bp > 99) bp = 99;
    put(blk, 95, bp);  // VDA1 EG break point
    put(blk, 96, 40);  // VDA1 EG slope time
    put(blk, 97, sus); // VDA1 EG sustain level
    put(blk, 98, rel); // VDA1 EG release time
    put(blk, 32, 2);   // joystick pitch-bend range
    put(blk, 38, 33);  // effects 1
    put(blk, 39, 33);  // effects 2
}

inline void generatePatches(PatchSet& p) {
    memset(&p, 0, sizeof p);

    // Programs 0..7
    makeProgram(p.programs + 143 * 0, "Sine Pad",    0, 0,  99, 25, 40, 85, 40);
    makeProgram(p.programs + 143 * 1, "Saw Lead",    1, 0,  80, 2,  30, 80, 20);
    makeProgram(p.programs + 143 * 2, "Square Bass", 2, -1, 55, 0,  25, 70, 15);
    makeProgram(p.programs + 143 * 3, "Tri Bell",    3, 0,  99, 0,  45, 10, 40);
    makeProgram(p.programs + 143 * 4, "Organ 1",     4, 0,  99, 0,  0,  99, 5);
    makeProgram(p.programs + 143 * 5, "Pulse Brass", 5, 0,  65, 15, 25, 85, 20);
    makeProgram(p.programs + 143 * 6, "Soft Saw",    6, 0,  70, 20, 35, 80, 30);
    makeProgram(p.programs + 143 * 7, "Noise Wind",  7, 0,  50, 40, 60, 60, 50);

    // Combinations 0..3
    // Combo 0: "Sine+Saw"
    uint8_t* c0 = p.combis + 124 * 0;
    setName(c0, "Sine+Saw");
    put(c0, 10, 0);
    put(c0, 11, 33);
    put(c0, 12, 33);
    // Timbre 0 at 36
    put(c0, 36 + 0, 0);  // program 0
    put(c0, 36 + 1, 99); // level 99
    put(c0, 36 + 4, 5);  // pan centre
    put(c0, 36 + 5, 127); // key top
    put(c0, 36 + 6, 0);   // key bottom
    put(c0, 36 + 7, 127); // vel top
    put(c0, 36 + 8, 1);   // vel bottom
    put(c0, 36 + 10, 0x00); // in use
    // Timbre 1 at 47
    put(c0, 47 + 0, 1);  // program 1
    put(c0, 47 + 1, 70); // level 70
    put(c0, 47 + 3, 7);  // detune +7
    put(c0, 47 + 4, 5);  // pan centre
    put(c0, 47 + 5, 127); // key top
    put(c0, 47 + 6, 0);   // key bottom
    put(c0, 47 + 7, 127); // vel top
    put(c0, 47 + 8, 1);   // vel bottom
    put(c0, 47 + 10, 0x00); // in use
    // Timbres 2..7 unused
    for (int t = 2; t < 8; t++) {
        int base = 36 + 11 * t;
        put(c0, base + 0, 0);
        put(c0, base + 1, 0);
        put(c0, base + 4, 5);
        put(c0, base + 5, 127);
        put(c0, base + 6, 0);
        put(c0, base + 7, 127);
        put(c0, base + 8, 1);
        put(c0, base + 10, 0x10);
    }

    // Combo 1: "Bass+Lead"
    uint8_t* c1 = p.combis + 124 * 1;
    setName(c1, "Bass+Lead");
    put(c1, 10, 0);
    put(c1, 11, 33);
    put(c1, 12, 33);
    // Timbre 0 at 36
    put(c1, 36 + 0, 2);  // program 2
    put(c1, 36 + 1, 99); // level 99
    put(c1, 36 + 4, 5);  // pan centre
    put(c1, 36 + 5, 59); // key top 59
    put(c1, 36 + 6, 0);  // key bottom
    put(c1, 36 + 7, 127); // vel top
    put(c1, 36 + 8, 1);   // vel bottom
    put(c1, 36 + 10, 0x00); // in use
    // Timbre 1 at 47
    put(c1, 47 + 0, 1);  // program 1
    put(c1, 47 + 1, 90); // level 90
    put(c1, 47 + 4, 5);  // pan centre
    put(c1, 47 + 5, 127); // key top
    put(c1, 47 + 6, 60);  // key bottom 60
    put(c1, 47 + 7, 127); // vel top
    put(c1, 47 + 8, 1);   // vel bottom
    put(c1, 47 + 10, 0x00); // in use
    // Timbres 2..7 unused
    for (int t = 2; t < 8; t++) {
        int base = 36 + 11 * t;
        put(c1, base + 0, 0);
        put(c1, base + 1, 0);
        put(c1, base + 4, 5);
        put(c1, base + 5, 127);
        put(c1, base + 6, 0);
        put(c1, base + 7, 127);
        put(c1, base + 8, 1);
        put(c1, base + 10, 0x10);
    }

    // Combo 2: "Organ Duo"
    uint8_t* c2 = p.combis + 124 * 2;
    setName(c2, "Organ Duo");
    put(c2, 10, 0);
    put(c2, 11, 33);
    put(c2, 12, 33);
    // Timbre 0 at 36
    put(c2, 36 + 0, 4);  // program 4
    put(c2, 36 + 1, 99); // level 99
    put(c2, 36 + 4, 5);  // pan centre
    put(c2, 36 + 5, 127); // key top
    put(c2, 36 + 6, 0);   // key bottom
    put(c2, 36 + 7, 127); // vel top
    put(c2, 36 + 8, 1);   // vel bottom
    put(c2, 36 + 10, 0x00); // in use
    // Timbre 1 at 47
    put(c2, 47 + 0, 4);  // program 4
    put(c2, 47 + 1, 60); // level 60
    put(c2, 47 + 2, 12); // transpose 12
    put(c2, 47 + 4, 5);  // pan centre
    put(c2, 47 + 5, 127); // key top
    put(c2, 47 + 6, 0);   // key bottom
    put(c2, 47 + 7, 127); // vel top
    put(c2, 47 + 8, 1);   // vel bottom
    put(c2, 47 + 10, 0x00); // in use
    // Timbres 2..7 unused
    for (int t = 2; t < 8; t++) {
        int base = 36 + 11 * t;
        put(c2, base + 0, 0);
        put(c2, base + 1, 0);
        put(c2, base + 4, 5);
        put(c2, base + 5, 127);
        put(c2, base + 6, 0);
        put(c2, base + 7, 127);
        put(c2, base + 8, 1);
        put(c2, base + 10, 0x10);
    }

    // Combo 3: "Wind Pad"
    uint8_t* c3 = p.combis + 124 * 3;
    setName(c3, "Wind Pad");
    put(c3, 10, 0);
    put(c3, 11, 33);
    put(c3, 12, 33);
    // Timbre 0 at 36
    put(c3, 36 + 0, 7);  // program 7
    put(c3, 36 + 1, 80); // level 80
    put(c3, 36 + 4, 5);  // pan centre
    put(c3, 36 + 5, 127); // key top
    put(c3, 36 + 6, 0);   // key bottom
    put(c3, 36 + 7, 127); // vel top
    put(c3, 36 + 8, 1);   // vel bottom
    put(c3, 36 + 10, 0x00); // in use
    // Timbre 1 at 47
    put(c3, 47 + 0, 0);  // program 0
    put(c3, 47 + 1, 90); // level 90
    put(c3, 47 + 4, 5);  // pan centre
    put(c3, 47 + 5, 127); // key top
    put(c3, 47 + 6, 0);   // key bottom
    put(c3, 47 + 7, 127); // vel top
    put(c3, 47 + 8, 1);   // vel bottom
    put(c3, 47 + 10, 0x00); // in use
    // Timbre 2 at 58
    put(c3, 58 + 0, 6);  // program 6
    put(c3, 58 + 1, 60); // level 60
    put(c3, 58 + 3, -6); // detune -6
    put(c3, 58 + 4, 5);  // pan centre
    put(c3, 58 + 5, 127); // key top
    put(c3, 58 + 6, 0);   // key bottom
    put(c3, 58 + 7, 127); // vel top
    put(c3, 58 + 8, 1);   // vel bottom
    put(c3, 58 + 10, 0x00); // in use
    // Timbres 3..7 unused
    for (int t = 3; t < 8; t++) {
        int base = 36 + 11 * t;
        put(c3, base + 0, 0);
        put(c3, base + 1, 0);
        put(c3, base + 4, 5);
        put(c3, base + 5, 127);
        put(c3, base + 6, 0);
        put(c3, base + 7, 127);
        put(c3, base + 8, 1);
        put(c3, base + 10, 0x10);
    }
}

} // namespace m1free

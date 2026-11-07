#pragma once

#include "m1_engine/m1_engine.h"
#include "m1free_tables.h"
#include "m1free_samples.h"
#include "m1free_patches.h"
#include "m1free_patches_fx.h"
#include "m1free_fx.h"
#include "m1free_patches_fx2.h"
#include "m1free_fx_reverb.h"
#include "m1free_fx_rotsym.h"
#include "m1free_fx_combo.h"
#include <string.h>

namespace m1free {

struct Free {
    Tables t;
    SampleSet s;
    PatchSet p;
    m1::FxTables fx;
};

inline Free g_free;

inline void fillRom(Free& f, m1::RomTables& rom) {
    generateTables(f.t);
    generateSamples(f.s);
    generatePatches(f.p);
    generateFxPatches(f.p);
    generateFxPatches2(f.p);
    memset(&f.fx, 0, sizeof f.fx);
    generateFxTablesBasic(f.fx);
    generateFxTablesReverb(f.fx);
    generateFxTablesRotSym(f.fx);
    generateFxTablesCombo(f.fx);

    rom.pcm = f.s.pcm;
    rom.pcmWords = f.s.pcmWords;
    rom.ms = f.s.ms;
    rom.splits = f.s.splits;
    rom.drums = f.s.drums;
    rom.nDrums = 1;
    rom.kernel = f.t.kernel;
    rom.vdf1k = f.t.vdf1k;
    rom.vdf2k = f.t.vdf2k;
    rom.vdf1t0 = f.t.vdf1t0;
    rom.vdf2t0 = f.t.vdf2t0;
    rom.egRise = f.t.egRise;
    rom.egFall = f.t.egFall;
    rom.velCurve = f.t.velCurve;
    rom.mgFreq = f.t.mgFreq;
    rom.mgDelay = f.t.mgDelay;
    rom.mgIntP = f.t.mgIntP;
    rom.mgIntF = f.t.mgIntF;
    rom.pitchS = f.t.pitchS;
    rom.pitchF = f.t.pitchF;
    rom.panReg = f.t.panReg;
    rom.pan101 = f.t.pan101;
    rom.delayStart = f.t.delayStart;
    rom.fx = &f.fx;
    rom.programs = f.p.programs;
    rom.combis = f.p.combis;
    rom.kits = f.p.kits;
    rom.global = f.p.global;
    rom.scale = f.t.scale;
}

} // namespace m1free

#include "m1_engine/m1_engine.h"
#include "m1_rom_tables.h"
#include "lite_sets.h"
#include "gfx.h"
#include "m1_params.h"
#include "m1ui_model.h"
#include "m1ui_view.h"
#include <stdint.h>
#include <stddef.h>

extern "C" const uint8_t korx_pcm_data[4194304];

namespace {

m1::Engine g_eng;
int16_t g_fx[65536];
m1::RomTables g_rom;

int g_mode = 0;
int g_index = 0;
double g_sr = 48000.0;

static bool liteHas(int mode, int idx);
const uint8_t* uiProgramData(int n) { return m1rom::kPrograms + 143 * n; }
const uint8_t* uiCombiData(int n) { return m1rom::kCombis + 124 * n; }
void uiPlayProgram(const uint8_t* b) { g_eng.playProgram(b); }
void uiPlayCombi(const uint8_t* b) { g_eng.playCombi(b); }
void uiRefreshFx(const uint8_t* b) { g_eng.refreshEffects(b); }
bool uiHasProgram(int n) { return liteHas(0, n); }
bool uiHasCombi(int n) { return liteHas(1, n); }
const m1ui::Host g_uiHost = { uiProgramData, uiCombiData, uiPlayProgram, uiPlayCombi, uiRefreshFx, uiHasProgram, uiHasCombi };
m1ui::Model g_ui;
float g_lvlL = 0.0f, g_lvlR = 0.0f;

// Output soft limiter (same curve as the PicoFaceM1 bridge): linear below 0.9, then a smooth knee that never exceeds 1.0.
static inline float softClip(float v) {
    const float a = 0.9f, range = 0.1f;
    if (v > a) return a + range * (1.0f - 1.0f / (1.0f + (v - a) / range));
    if (v < -a) return -a - range * (1.0f - 1.0f / (1.0f + (-v - a) / range));
    return v;
}

const int kRingSize = 2048;
float g_ringL[kRingSize];
float g_ringR[kRingSize];
int g_ringWrite = 0;
double g_readPos = 0.0;
int g_framesRendered = 0;

void ensureFrames(int needed) {
    int have = g_framesRendered - (int)g_readPos;
    while (have < needed) {
        float L[64];
        float R[64];
        g_eng.render(L, R, 64);
        for (int i = 0; i < 64; ++i) {
            int idx = g_ringWrite;
            g_ringL[idx] = L[i];
            g_ringR[idx] = R[i];
            g_ringWrite = (g_ringWrite + 1) & (kRingSize - 1);
        }
        g_framesRendered += 64;
        have += 64;
    }
}

void renderOutput(float* outL, float* outR, int n) {
    double step = 31250.0 / g_sr;
    for (int i = 0; i < n; ++i) {
        double pos = g_readPos;
        int i0 = (int)pos;
        double frac = pos - i0;
        
        ensureFrames(i0 + 4);

        int idx0 = i0 & (kRingSize - 1);
        int idx1 = (i0 + 1) & (kRingSize - 1);
        int idx2 = (i0 + 2) & (kRingSize - 1);
        int idx3 = (i0 + 3) & (kRingSize - 1);

        float p0 = g_ringL[idx0];
        float p1 = g_ringL[idx1];
        float p2 = g_ringL[idx2];
        float p3 = g_ringL[idx3];

        float r0 = g_ringR[idx0];
        float r1 = g_ringR[idx1];
        float r2 = g_ringR[idx2];
        float r3 = g_ringR[idx3];

        float t = (float)frac;
        float t2 = t * t;
        float t3 = t2 * t;

        float vL = 0.5f * (2.0f * p1 + (p0 - p2) * t + (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 + (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3);
        float vR = 0.5f * (2.0f * r1 + (r0 - r2) * t + (2.0f * r0 - 5.0f * r1 + 4.0f * r2 - r3) * t2 + (-r0 + 3.0f * r1 - 3.0f * r2 + r3) * t3);

        outL[i] = vL;
        outR[i] = vR;

        g_readPos += step;
    }
}

static bool liteHas(int mode, int idx) {
    const int* a = mode ? kLiteCombis : kLiteProgs;
    int n = mode ? kLiteCombiCount : kLiteProgCount;
    for (int i = 0; i < n; ++i) if (a[i] == idx) return true;
    return false;
}
// next member strictly after idx going in direction dir (+1/-1), wrapping 0..99
static int liteStep(int mode, int idx, int dir) {
    for (int k = 1; k <= 100; ++k) {
        int c = ((idx + dir * k) % 100 + 100) % 100;
        if (liteHas(mode, c)) return c;
    }
    return idx;
}
// idx itself if a member, else the next member upward (wrapping)
static int liteSnap(int mode, int idx) {
    if (liteHas(mode, idx)) return idx;
    return liteStep(mode, idx, 1);
}
void handleMidi(uint32_t midi) {
    uint8_t status = midi & 0xF0;
    uint8_t data1 = (midi >> 8) & 0x7F;
    uint8_t data2 = (midi >> 16) & 0x7F;

    if (status == 0x90) {
        if (data2 > 0) g_eng.noteOn(0, data1, data2);
        else g_eng.noteOff(0, data1);
    } else if (status == 0x80) {
        g_eng.noteOff(0, data1);
    } else if (status == 0xB0) {
        if (data1 == 1) g_eng.modWheel(0, data2);
        else if (data1 == 2) g_eng.modWheelNeg(0, data2);
        else if (data1 == 64) g_eng.damper(0, data2 >= 64);
        else if (data1 == 65) g_eng.setRotaryFast(data2 >= 64);
        else if (data1 == 120) {
            g_eng.allSoundOff();
            for (int i = 0; i < 65536; ++i) {
                g_fx[i] = 0;
            }
        }
        else if (data1 == 123) g_eng.allNotesOff();
    } else if (status == 0xC0) {
        int p = data1;
        if (p > 99) p = 99;
        int q = liteSnap(g_ui.mode, p);
        if (g_ui.mode == 0) m1ui::selectProgram(g_ui, q);
        else m1ui::selectCombi(g_ui, q);
    } else if (status == 0xD0) {
        g_eng.aftertouch(0, data1);
    } else if (status == 0xE0) {
        int bend = ((data2 << 7) | data1) - 8192;
        g_eng.pitchBend(0, bend);
    }
}

void drawSevenSegment(uint16_t* fb, int x, int y, int w, int h, int digit) {
    // 0x0841 is background, 0xFD20 is foreground
    uint16_t bg = 0x0841;
    uint16_t fg = 0xFD20;
    
    // Fill background
    for (int i = 0; i < h; ++i) {
        for (int j = 0; j < w; ++j) {
            fb[(y + i) * 480 + (x + j)] = bg;
        }
    }

    // Define segments (approximate)
    // A: top, B: top-right, C: bottom-right, D: bottom, E: bottom-left, F: top-left, G: middle
    // We'll draw simple rectangles for segments.
    int sw = w / 4; // segment width
    int sh = h / 6; // segment height
    
    auto drawSeg = [&](int sx, int sy, int sw_, int sh_) {
        for (int i = 0; i < sh_; ++i) {
            for (int j = 0; j < sw_; ++j) {
                fb[(sy + i) * 480 + (sx + j)] = fg;
            }
        }
    };

    // A (top)
    if (digit == 0 || digit == 2 || digit == 3 || digit == 5 || digit == 6 || digit == 7 || digit == 8 || digit == 9) {
        drawSeg(x + sw, y, w - 2*sw, sh);
    }
    // B (top right)
    if (digit == 0 || digit == 1 || digit == 2 || digit == 3 || digit == 4 || digit == 7 || digit == 8 || digit == 9) {
        drawSeg(x + w - sw, y + sh, sw, sh);
    }
    // C (bottom right)
    if (digit == 0 || digit == 1 || digit == 3 || digit == 4 || digit == 5 || digit == 6 || digit == 7 || digit == 8 || digit == 9) {
        drawSeg(x + w - sw, y + 3*sh, sw, sh);
    }
    // D (bottom)
    if (digit == 0 || digit == 2 || digit == 3 || digit == 5 || digit == 6 || digit == 8 || digit == 9) {
        drawSeg(x + sw, y + 5*sh, w - 2*sw, sh);
    }
    // E (bottom left)
    if (digit == 0 || digit == 2 || digit == 8 || digit == 9) {
        drawSeg(x, y + 3*sh, sw, sh);
    }
    // F (top left)
    if (digit == 0 || digit == 4 || digit == 5 || digit == 6 || digit == 8 || digit == 9) {
        drawSeg(x, y + sh, sw, sh);
    }
    // G (middle)
    if (digit == 2 || digit == 3 || digit == 4 || digit == 5 || digit == 6 || digit == 8 || digit == 9) {
        drawSeg(x + sw, y + 2*sh, w - 2*sw, sh);
    }
}

} // namespace

extern "C" {

__attribute__((export_name("vibe_sdk_version")))
int vibe_sdk_version() {
    return 5;
}

__attribute__((export_name("vibe_wasm_init")))
void vibe_wasm_init(float sr) {
    if (sr < 8000.0f) sr = 8000.0f;
    if (sr > 192000.0f) sr = 192000.0f;
    g_sr = (double)sr;
    
    g_rom.pcm = reinterpret_cast<const int16_t*>(korx_pcm_data);
    g_rom.pcmWords = 2097152;
    g_rom.ms = m1rom::kMultisounds;
    g_rom.splits = m1rom::kSplits;
    g_rom.drums = m1rom::kDrums;
    g_rom.nDrums = m1rom::kDrumCount;
    g_rom.kernel = m1rom::kKernel;
    g_rom.vdf1k = m1rom::kVdf1k;
    g_rom.vdf2k = m1rom::kVdf2k;
    g_rom.vdf1t0 = m1rom::kVdf1t0;
    g_rom.vdf2t0 = m1rom::kVdf2t0;
    g_rom.egRise = m1rom::kEgRise;
    g_rom.egFall = m1rom::kEgFall;
    g_rom.velCurve = m1rom::kVelCurve;
    g_rom.mgFreq = m1rom::kMgFreq;
    g_rom.mgDelay = m1rom::kMgDelay;
    g_rom.mgIntP = m1rom::kMgIntP;
    g_rom.mgIntF = m1rom::kMgIntF;
    g_rom.pitchS = m1rom::kPitchS;
    g_rom.pitchF = m1rom::kPitchF;
    g_rom.panReg = m1rom::kPanReg;
    g_rom.pan101 = m1rom::kPan101;
    g_rom.delayStart = m1rom::kDelayStart;
    g_rom.fx = &m1rom::kFx;
    g_rom.programs = m1rom::kPrograms;
    g_rom.combis = m1rom::kCombis;
    g_rom.kits = m1rom::kKits;
    g_rom.global = m1rom::kGlobal;
    g_rom.scale = m1rom::kScale;

    g_eng.init(g_rom, g_fx);
    m1::setNameSources(m1rom::kMultisoundNames, m1rom::kDrumNames, m1rom::kPrograms);
    m1ui::init(g_ui, g_uiHost);
    g_lvlL = 0.0f;
    g_lvlR = 0.0f;
    
    g_mode = 0;
    g_index = kLiteProgs[0];
    g_readPos = 0.0;
    g_framesRendered = 0;
    g_ringWrite = 0;
    for (int i = 0; i < kRingSize; ++i) {
        g_ringL[i] = 0.0f;
        g_ringR[i] = 0.0f;
    }
}

__attribute__((export_name("vibe_wasm_io")))
uint32_t vibe_wasm_io() {
    // Returns a pointer to the io buffer. 
    // The contract says: in1 at 0..255, OUT L 256..511, OUT R 512..767, in2 768..1023.
    // We need a static buffer for this.
    static float io_buf[1024];
    return (uint32_t)(uintptr_t)io_buf;
}

__attribute__((export_name("vibe_wasm_audio")))
void vibe_wasm_audio(uint32_t n, uint32_t flags) {
    if (n > 128) n = 128;
    float* io = reinterpret_cast<float*>(vibe_wasm_io());
    float* outL = io + 256;
    float* outR = io + 512;
    renderOutput(outL, outR, n);
    for (uint32_t i = 0; i < n; ++i) {
        outL[i] = softClip(outL[i]);
        outR[i] = softClip(outR[i]);
    }
    float pl = 0.0f, pr = 0.0f;
    for (uint32_t i = 0; i < n; ++i) {
        float a = outL[i] < 0.0f ? -outL[i] : outL[i];
        float b = outR[i] < 0.0f ? -outR[i] : outR[i];
        if (a > pl) pl = a;
        if (b > pr) pr = b;
    }
    g_lvlL = pl > g_lvlL ? pl : g_lvlL * 0.92f;
    g_lvlR = pr > g_lvlR ? pr : g_lvlR * 0.92f;
}

__attribute__((export_name("vibe_wasm_event")))
void vibe_wasm_event(uint32_t type, uint32_t id, uint32_t down, float value, uint32_t midi) {
    if (type == 5) {
        handleMidi(midi);
    } else if (type == 2) {
        int note = 48 + (int)id;
        if (note < 0) note = 0;
        if (note > 127) note = 127;
        if (down) {
            int vel = value > 0.0f ? (int)(value * 127.0f + 0.5f) : 100;
            if (vel < 1) vel = 1;
            if (vel > 127) vel = 127;
            g_eng.noteOn(0, note, vel);
        } else {
            g_eng.noteOff(0, note);
        }
    } else if (type == 3 || type == 4) {
        m1ui::event(g_ui, (int)type, (int)id, (int)down, value);
    } else if (type == 99) {
        if (id == 4 || id == 6) {
            int delta = (int)value;
            int dir = delta < 0 ? -1 : 1;
            int steps = delta < 0 ? -delta : delta;
            if (steps < 1) steps = 1;
            if (steps > 8) steps = 8;
            for (int s = 0; s < steps; ++s) g_index = liteStep(g_mode, g_index, dir);
            if (g_mode == 0) g_eng.selectProgram(g_index);
            else g_eng.selectCombi(g_index);
        }
    } else if (type == 3) {
        if (id == 9 && down) {
            g_mode = g_mode ? 0 : 1;
            g_index = liteSnap(g_mode, g_index);
            if (g_mode == 0) g_eng.selectProgram(g_index);
            else g_eng.selectCombi(g_index);
        }
    } else if (type == 6) {
        g_eng.damper(0, value >= 0.5f && (value <= 1.0f || value >= 64.0f));
    }
}

__attribute__((export_name("vibe_wasm_draw")))
uint32_t vibe_wasm_draw() {
    m1ui::Meters mt;
    mt.l = g_lvlL;
    mt.r = g_lvlR;
    mt.voices = g_eng.activeVoices();
    m1ui::draw(g_ui, mt);
    return (uint32_t)(uintptr_t)gfx::fb;
}

static uint32_t old_draw_unused() {
    static uint16_t fb[480 * 320];
    
    // Background
    for (int i = 0; i < 480 * 320; ++i) fb[i] = 0x0841;

    // Mode indicator (top-left 40x40)
    uint16_t modeColor = (g_mode == 0) ? 0x07E0 : 0x001F;
    for (int y = 0; y < 40; ++y) {
        for (int x = 0; x < 40; ++x) {
            fb[y * 480 + x] = modeColor;
        }
    }

    // Index display (centered)
    int d1 = g_index / 10;
    int d2 = g_index % 10;
    int w = 80;
    int h = 140;
    int totalW = w * 2 + 20;
    int startX = (480 - totalW) / 2;
    int startY = (320 - h) / 2;
    
    drawSevenSegment(fb, startX, startY, w, h, d1);
    drawSevenSegment(fb, startX + w + 20, startY, w, h, d2);

    // Voice bar (bottom)
    int active = g_eng.activeVoices();
    int barW = (active * 440) / 16;
    if (barW > 440) barW = 440;
    if (barW < 0) barW = 0;
    int barH = 12;
    int barY = 320 - barH;
    for (int y = 0; y < barH; ++y) {
        for (int x = 0; x < barW; ++x) {
            fb[(barY + y) * 480 + x] = 0xFFFF;
        }
    }

    return (uint32_t)(uintptr_t)fb;
}

__attribute__((export_name("vibe_wasm_leds")))
uint32_t vibe_wasm_leds() {
    static uint8_t leds[384];
    m1ui::fillLeds(g_ui, leds);
    return (uint32_t)(uintptr_t)leds;
}

__attribute__((export_name("vibe_wasm_labels")))
uint32_t vibe_wasm_labels() {
    static char caps[25][8];
    m1ui::captions(g_ui, caps);
    return (uint32_t)(uintptr_t)caps;
}

__attribute__((export_name("korx_name_ptr")))
const char* korx_name_ptr() {
    return g_eng.name();
}

__attribute__((export_name("korx_mode")))
int korx_mode() {
    return g_ui.mode;
}

__attribute__((export_name("korx_index")))
int korx_index() {
    return g_ui.mode ? g_ui.combi : g_ui.program;
}

} // extern "C"

#include "gfx.h"
#include <stdint.h>
#include <math.h>

static float io_buf[1024];
static uint8_t leds_buf[384];

extern "C" __attribute__((export_name("vibe_sdk_version")))
int vibe_sdk_version() { return 5; }

extern "C" __attribute__((export_name("vibe_wasm_init")))
void vibe_wasm_init(unsigned sr) {}

extern "C" __attribute__((export_name("vibe_wasm_io")))
float* vibe_wasm_io() { return io_buf; }

extern "C" __attribute__((export_name("vibe_wasm_audio")))
void vibe_wasm_audio(int n, int flags) {}

extern "C" __attribute__((export_name("vibe_wasm_event")))
void vibe_wasm_event(int type, int id, int down, float value, int midi) {}

extern "C" __attribute__((export_name("vibe_wasm_leds")))
unsigned vibe_wasm_leds() { return (unsigned)(uintptr_t)leds_buf; }

extern "C" __attribute__((export_name("vibe_wasm_draw")))
unsigned vibe_wasm_draw() {
    // 1. Background
    gfx::clear(gfx::rgb(11, 17, 25));

    // 2. Top text
    gfx::text(small_font, 10, 18, "The quick brown fox 0123456789", gfx::rgb(255, 255, 255));

    // 3. Program text
    gfx::text(big_font, 10, 56, "PROGRAM 42 Piano16", gfx::rgb(183, 148, 246));

    // 4. Rounded rects
    // Rect 1
    gfx::fillRoundRect(10, 80, 90, 30, 8, gfx::rgb(22, 32, 48));
    gfx::textC(small_font, 10 + 45, 80 + 20, "ONE", gfx::rgb(255, 255, 255));
    
    // Rect 2 (Violet)
    gfx::fillRoundRect(110, 80, 90, 30, 8, gfx::rgb(183, 148, 246));
    gfx::textC(small_font, 110 + 45, 80 + 20, "TWO", gfx::rgb(11, 17, 25));
    
    // Rect 3
    gfx::fillRoundRect(210, 80, 90, 30, 8, gfx::rgb(22, 32, 48));
    gfx::textC(small_font, 210 + 45, 80 + 20, "THREE", gfx::rgb(255, 255, 255));

    // 5. Arcs (Knobs)
    // Knob 1 at (80, 150)
    gfx::arc(80, 150, 40, 5, 135.0f, 405.0f, gfx::rgb(31, 43, 61));
    gfx::arc(80, 150, 40, 5, 135.0f, 300.0f, gfx::rgb(77, 163, 232));
    gfx::fillCircle(80, 150, 31, gfx::rgb(27, 38, 56));
    // Pointer 1 (300 deg)
    float a1_rad = (300.0f - 90.0f) * 3.14159265f / 180.0f;
    int px1 = 80 + (int)(28 * cosf(a1_rad));
    int py1 = 150 + (int)(28 * sinf(a1_rad));
    gfx::line(80, 150, px1, py1, gfx::rgb(255, 255, 255));

    // Knob 2 at (200, 150)
    gfx::arc(200, 150, 40, 5, 135.0f, 405.0f, gfx::rgb(31, 43, 61));
    gfx::arc(200, 150, 40, 5, 135.0f, 200.0f, gfx::rgb(77, 163, 232));
    gfx::fillCircle(200, 150, 31, gfx::rgb(27, 38, 56));
    // Pointer 2 (200 deg)
    float a2_rad = (200.0f - 90.0f) * 3.14159265f / 180.0f;
    int px2 = 200 + (int)(28 * cosf(a2_rad));
    int py2 = 150 + (int)(28 * sinf(a2_rad));
    gfx::line(200, 150, px2, py2, gfx::rgb(255, 255, 255));

    // 6. Knob Labels
    gfx::textC(big_font, 80, 215, "185Hz", gfx::rgb(255, 255, 255));
    gfx::textC(big_font, 200, 215, "22", gfx::rgb(255, 255, 255));

    // 7. ASCII Grid
    uint16_t grey = gfx::rgb(200, 200, 200);
    // Row 1: 32-79
    for (int i = 0; i < 48; i++) {
        char c = (char)(32 + i);
        char buf[2] = {c, 0};
        gfx::text(small_font, 10 + i * 9, 262, buf, grey);
    }
    // Row 2: 80-126
    for (int i = 0; i < 47; i++) {
        char c = (char)(80 + i);
        char buf[2] = {c, 0};
        gfx::text(small_font, 10 + i * 9, 282, buf, grey);
    }

    // 8. Blend Rect
    gfx::blendRect(400, 100, 60, 100, gfx::rgb(100, 100, 255), 128);

    return (unsigned)(uintptr_t)gfx::fb;
}

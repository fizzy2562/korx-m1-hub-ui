#pragma once
#include <stdint.h>
#include "m1ui_model.h"

namespace m1ui {

// Combination timbres are 11-byte records beginning at byte 36.
// These helpers intentionally expose no mutable model state.
inline const uint8_t* combiTimbre(const Model& m, int timbre) {
    if (timbre < 0 || timbre >= 8) return nullptr;
    return m.combiEdit + 36 + timbre * 11;
}

inline int combiTimbreProgram(const Model& m, int timbre) {
    const uint8_t* t = combiTimbre(m, timbre);
    return t ? t[0] : 0; // block offset 36: program number
}

inline int combiTimbreLevel(const Model& m, int timbre) {
    const uint8_t* t = combiTimbre(m, timbre);
    return t ? t[1] : 0; // block offset 37: level
}

inline bool combiTimbreOn(const Model& m, int timbre) {
    const uint8_t* t = combiTimbre(m, timbre);
    return t ? ((t[10] & 0x10) == 0) : false; // block offset 46, bit 4: bit set = timbre OFF (same convention as m1_params Toggle)
}

inline const char* programName(const Model& m, int program, char out[11]) {
    const uint8_t* p = m.host && m.host->programData ? m.host->programData(program) : nullptr;
    for (int i = 0; i < 10; ++i) {
        uint8_t c = p ? p[i] : ' ';
        out[i] = (c >= 32 && c <= 126) ? (char)c : ' ';
    }
    out[10] = 0;
    return out;
}

} // namespace m1ui

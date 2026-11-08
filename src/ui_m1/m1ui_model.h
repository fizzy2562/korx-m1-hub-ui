#pragma once

#include "m1_params.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

namespace m1ui {

struct Host {
    const uint8_t* (*programData)(int n);
    const uint8_t* (*combiData)(int n);
    void (*playProgram)(const uint8_t* buf);
    void (*playCombi)(const uint8_t* buf);
    void (*refreshFx)(const uint8_t* fxBlock);
    bool (*hasProgram)(int n);
    bool (*hasCombi)(int n);
};

constexpr int kMaxParams = 256, kMaxGroups = 40;

struct Model {
    const Host* host = nullptr;
    uint8_t mode = 0;
    uint8_t program = 0, combi = 0;
    uint8_t progEdit[143] = {0};
    uint8_t combiEdit[124] = {0};
    bool playing = true, help = false;
    int helpPage = 0;
    bool edited = false, comparing = false, shift = false;
    m1::ParamDef params[kMaxParams];
    int nParams = 0;
    char groupName[kMaxGroups][12];
    uint16_t groupFirst[kMaxGroups];
    int nGroups = 0;
    int group = 0;
    int sub = 0;
};

inline void selectProgram(Model& m, int n);

inline void init(Model& m, const Host& h) {
    m.host = &h;
    int first = -1;
    for (int i = 0; i < 100; ++i) {
        if (h.hasProgram(i)) { first = i; break; }
    }
    selectProgram(m, first >= 0 ? first : 0);
    m.group = 0;
}

inline uint8_t* block(Model& m) {
    return m.mode == 0 ? m.progEdit : m.combiEdit;
}

inline const uint8_t* block(const Model& m) {
    return m.mode == 0 ? m.progEdit : m.combiEdit;
}

inline void buildPages(Model& m) {
    m.nParams = 0;
    m.nGroups = 0;
    
    int pageCount = 0;
    const m1::PageDef* pages = (m.mode == 0) ? m1::programPages(pageCount) : m1::combiPages(pageCount);
    const uint8_t* blk = block(m);
    
    for (int i = 0; i < pageCount; ++i) {
        const m1::PageDef& pg = pages[i];
        
        // Skip DoubleOnly pages if not in Double mode (byte 10 == 1)
        if (m.mode == 0 && (pg.flags & m1::kPageDoubleOnly) && blk[10] != 1) continue;
        
        // FX Pages
        if (pg.flags & (m1::kPageFx1 | m1::kPageFx2)) {
            int k = (pg.flags & m1::kPageFx1) ? 0 : 1;
            int fxBase = (m.mode == 0) ? m1::kProgFx : m1::kCombiFx;
            int type = blk[fxBase + k];
            
            char title[12];
            const char* fxName = (type < 34) ? m1::kFxNames[type] : m1::kFxNames[33];
            snprintf(title, sizeof(title), "FX%d %s", k + 1, fxName);
            
            const m1::ParamDef* fp = m1::effectParams(type);
            int dOff = fxBase + 9 + 8 * k;
            
            bool started = false;
            for (int j = 0; j < 8; ++j) {
                if (!fp[j].name) continue;
                
                if (!started) {
                    if (m.nGroups >= kMaxGroups) break;
                    snprintf(m.groupName[m.nGroups], 12, "%s", title);
                    m.groupFirst[m.nGroups] = (uint16_t)m.nParams;
                    m.nGroups++;
                    started = true;
                }
                
                if (m.nParams >= kMaxParams) break;
                m1::ParamDef p = fp[j];
                p.offset = (uint8_t)(p.offset + dOff);
                m.params[m.nParams++] = p;
            }
            continue;
        }
        
        // Timbre Pages
        if (pg.flags & m1::kPageTimbre) {
            for (int tm = 0; tm < 8; ++tm) {
                char title[12];
                snprintf(title, sizeof(title), "Timbre %d", tm + 1);
                
                bool started = false;
                for (int j = 0; j < pg.count; ++j) {
                    const m1::ParamDef& p = pg.params[j];
                    if (!p.name) continue;
                    
                    if (!started) {
                        if (m.nGroups >= kMaxGroups) break;
                        snprintf(m.groupName[m.nGroups], 12, "%s", title);
                        m.groupFirst[m.nGroups] = (uint16_t)m.nParams;
                        m.nGroups++;
                        started = true;
                    }
                    
                    if (m.nParams >= kMaxParams) break;
                    m1::ParamDef np = p;
                    np.offset = (uint8_t)(np.offset + 11 * tm);
                    m.params[m.nParams++] = np;
                }
            }
            continue;
        }
        
        // Standard Pages
        bool started = false;
        for (int j = 0; j < pg.count; ++j) {
            const m1::ParamDef& p = pg.params[j];
            if (!p.name) continue;
            
            if (!started) {
                if (m.nGroups >= kMaxGroups) break;
                snprintf(m.groupName[m.nGroups], 12, "%s", pg.title);
                m.groupFirst[m.nGroups] = (uint16_t)m.nParams;
                m.nGroups++;
                started = true;
            }
            
            if (m.nParams >= kMaxParams) break;
            m.params[m.nParams++] = p;
        }
    }
    
    // Clamp group and sub
    if (m.nGroups == 0) {
        m.group = 0;
        m.sub = 0;
    } else {
        if (m.group >= m.nGroups) m.group = m.nGroups - 1;
        if (m.group < 0) m.group = 0;
        
        int size = (m.group + 1 < m.nGroups) ? (m.groupFirst[m.group + 1] - m.groupFirst[m.group]) : (m.nParams - m.groupFirst[m.group]);
        int maxSub = ((size - 1) / 4) * 4;
        if (m.sub > maxSub) m.sub = maxSub;
        if (m.sub < 0) m.sub = 0;
    }
}

inline int groupCount(const Model& m) {
    return m.nGroups;
}

inline int groupSize(const Model& m, int g) {
    if (g < 0 || g >= m.nGroups) return 0;
    if (g + 1 < m.nGroups) return m.groupFirst[g + 1] - m.groupFirst[g];
    return m.nParams - m.groupFirst[g];
}

inline const m1::ParamDef* slotParam(const Model& m, int slot) {
    if (m.group < 0 || m.group >= m.nGroups) return nullptr;
    int idx = m.groupFirst[m.group] + m.sub + slot;
    int size = groupSize(m, m.group);
    if (idx < m.groupFirst[m.group] || idx >= m.groupFirst[m.group] + size) return nullptr;
    return &m.params[idx];
}

inline void slotText(const Model& m, int slot, char* text, int n, float* norm, const char** label) {
    const m1::ParamDef* p = slotParam(m, slot);
    if (!p) {
        if (text && n > 0) text[0] = '\0';
        if (norm) *norm = -1.0f;
        if (label) *label = "";
        return;
    }
    if (text && n > 0) {
        m1::paramFormat(*p, block(m), text, n, norm);
        if (norm && *norm < 0.0f && p->hi > p->lo) {
            float v = (float)m1::paramGet(*p, block(m));
            float nn = (v - (float)p->lo) / (float)(p->hi - p->lo);
            *norm = nn < 0.0f ? 0.0f : (nn > 1.0f ? 1.0f : nn);
        }
    } else if (norm) {
        *norm = -1.0f;
    }
    if (label) *label = p->name;
}

inline void selectProgram(Model& m, int n) {
    if (n < 0) n = 0;
    if (n > 99) n = 99;
    m.program = (uint8_t)n;
    if (m.host && m.host->programData) {
        const uint8_t* data = m.host->programData(n);
        if (data) memcpy(m.progEdit, data, 143);
    }
    m.edited = false;
    m.comparing = false;
    if (m.host && m.host->playProgram) m.host->playProgram(m.progEdit);
    if (m.mode == 0) buildPages(m);
}

inline void selectCombi(Model& m, int n) {
    if (n < 0) n = 0;
    if (n > 99) n = 99;
    m.combi = (uint8_t)n;
    if (m.host && m.host->combiData) {
        const uint8_t* data = m.host->combiData(n);
        if (data) memcpy(m.combiEdit, data, 124);
    }
    m.edited = false;
    m.comparing = false;
    if (m.host && m.host->playCombi) m.host->playCombi(m.combiEdit);
    if (m.mode == 1) buildPages(m);
}

inline void stepItem(Model& m, int dir);

inline void setMode(Model& m, int mode) {
    m.playing = true;
    m.mode = (uint8_t)(mode ? 1 : 0);
    m.comparing = false;
    if (m.host) {
        int n = (m.mode == 0) ? m.program : m.combi;
        bool ok = (m.mode == 0) ? m.host->hasProgram(n) : m.host->hasCombi(n);
        if (ok) {
            if (m.mode == 0) selectProgram(m, n);
            else selectCombi(m, n);
        } else {
            stepItem(m, +1);
        }
    }
    m.group = 0;
    m.sub = 0;
    buildPages(m);
    m.group = 0;
}

inline void stepItem(Model& m, int dir) {
    if (!m.host) return;
    int start = (m.mode == 0) ? m.program : m.combi;
    for (int i = 1; i <= 100; ++i) {
        int next = (start + dir * i + 1000) % 100;
        bool ok = (m.mode == 0) ? m.host->hasProgram(next) : m.host->hasCombi(next);
        if (ok) {
            if (m.mode == 0) selectProgram(m, next);
            else selectCombi(m, next);
            return;
        }
    }
}

inline void toggleCompare(Model& m) {
    m.comparing = !m.comparing;
    if (!m.host) return;
    if (m.mode == 0) {
        if (m.comparing) {
            if (m.host->programData && m.host->playProgram)
                m.host->playProgram(m.host->programData(m.program));
        } else {
            if (m.host->playProgram) m.host->playProgram(m.progEdit);
        }
    } else {
        if (m.comparing) {
            if (m.host->combiData && m.host->playCombi)
                m.host->playCombi(m.host->combiData(m.combi));
        } else {
            if (m.host->playCombi) m.host->playCombi(m.combiEdit);
        }
    }
}

inline void stepGroup(Model& m, int dir) {
    if (m.nGroups == 0) return;
    m.group += dir;
    if (m.group >= m.nGroups) m.group = 0;
    if (m.group < 0) m.group = m.nGroups - 1;
    m.sub = 0;
}

inline void stepSub(Model& m, int dir) {
    if (m.nGroups == 0) return;
    m.sub += 4 * dir;
    
    int size = groupSize(m, m.group);
    if (m.sub >= size) {
        // Move to next group
        m.group++;
        if (m.group >= m.nGroups) m.group = 0;
        m.sub = 0;
    } else if (m.sub < 0) {
        // Move to previous group
        m.group--;
        if (m.group < 0) m.group = m.nGroups - 1;
        int prevSize = groupSize(m, m.group);
        m.sub = ((prevSize - 1) / 4) * 4;
    }
}

inline void editSlot(Model& m, int slot, int delta) {
    const m1::ParamDef* p = slotParam(m, slot);
    if (!p) return;
    
    if (m.comparing) {
        m.comparing = false;
        if (m.mode == 0) {
            if (m.host && m.host->playProgram) m.host->playProgram(m.progEdit);
        } else {
            if (m.host && m.host->playCombi) m.host->playCombi(m.combiEdit);
        }
    }
    
    m1::ParamDef pd = *p; // Copy before potential buildPages
    
    if (m1::paramStep(pd, block(m), delta)) {
        m.edited = true;
        
        if (m.mode == 0) {
            if (pd.offset >= 38 && pd.offset < 63) {
                if (m.host && m.host->refreshFx) m.host->refreshFx(m.progEdit + 38);
            }
            if (pd.offset == 38 || pd.offset == 39 || pd.offset == 10) {
                buildPages(m);
            }
        } else {
            if (pd.offset >= 11 && pd.offset < 36) {
                if (m.host && m.host->refreshFx) m.host->refreshFx(m.combiEdit + 11);
                if (pd.offset == 11 || pd.offset == 12) {
                    buildPages(m);
                }
            } else if (pd.offset >= 10) {
                if (m.host && m.host->playCombi) m.host->playCombi(m.combiEdit);
            }
        }
    }
}

inline void getName(const Model& m, char* out) {
    const uint8_t* blk = block(m);
    for (int i = 0; i < 10; ++i) {
        uint8_t c = blk[i];
        if (c < 32 || c > 126) c = ' ';
        out[i] = (char)c;
    }
    out[10] = '\0';
    
    // Trim trailing spaces
    int len = 10;
    while (len > 0 && out[len - 1] == ' ') len--;
    out[len] = '\0';
}

inline int jumpGroup(const Model& m, int key) {
    if (key == 0) return 0;
    if (key == 1) return 1;
    if (key == 5) return m.mode ? 2 : 0;
    const char* prefix = key == 2 ? "Effects" : key == 3 ? "FX1 " : "FX2 ";
    for (int g=0; g<m.nGroups; ++g) if (!strncmp(m.groupName[g],prefix,strlen(prefix))) return g;
    return -1;
}
inline void event(Model& m, int type, int id, int down, float value) {
    if (type == 3 && down && id == 17) { m.help = !m.help; m.helpPage=0; return; }
    if (m.help) {
        if (type == 3 && down && (id == 18 || id == 20)) { m.helpPage ^= 1; return; }
        if (type == 3 && down && id == 16) { m.help=false; return; }
        if ((type == 3 && down) || (type == 4 && value != 0)) m.help=false;
    }
    if (type == 3) { // BUTTON
        if (id == 6) { // SHIFT
            m.shift = (down != 0);
            return;
        }
        
        if (down == 0) return;
        
        switch (id) {
            case 0: stepItem(m, -1); break;
            case 1: stepItem(m, +1); break;
            case 2: setMode(m, !m.mode); break;
            case 3: toggleCompare(m); break;
            case 4: m.playing=false; stepGroup(m, -1); break;
            case 5: m.playing=false; stepGroup(m, +1); break;
            case 9: m.playing = !m.playing; break;
            case 16: m.playing=true; break;
            case 10: case 11: case 12: case 13: case 14: case 15: {
                int g = jumpGroup(m, id - 10);
                m.playing = id == 10;
                if (g >= 0 && g < m.nGroups) {
                    m.group = g;
                    m.sub = 0;
                }
                break;
            }
            case 17:
                m.group = 0;
                m.sub = 0;
                break;
            case 18: m.playing=false; stepSub(m, -1); break;
            case 20: m.playing=false; stepSub(m, +1); break;
        }
    } else if (type == 4) { // ENCODER
        if (value == 0.0f) return;
        int delta = (value > 0.0f) ? 1 : -1;
        if (m.shift) delta *= 4;
        
        if (id >= 0 && id <= 3) {
            if (!m.playing) editSlot(m, id, delta);
        } else if (id == 4 || id == 6) {
            stepItem(m, delta);
        } else if (id == 5) {
            setMode(m, !m.mode);
        }
    }
}

inline void captions(const Model& m, char out[25][8]) {
    memset(out,0,25*8);
    const char* labels[25] = {
        m.mode?"CMB -":"PRG -", m.mode?"CMB +":"PRG +", m.mode?"PROG":"COMBI",
        m.comparing?"EDITED":"COMPARE", "PAGE -", "PAGE +", "X4", "", "",
        m.playing?"EDIT":"PLAY", "PLAY", m.mode?"TYPE":"OSC", "EFFECTS", "FX1", "FX2",
        m.mode?"TIMBRE":"NAME", m.help?"CLOSE":"PLAY", "HELP",
        m.help?"HELP <":"PART -", "", m.help?"HELP >":"PART +", "", "", "", ""
    };
    for(int i=0;i<25;++i) if(labels[i]) snprintf(out[i],8,"%s",labels[i]);
    for(int i=3;i<=4;++i) if(jumpGroup(m,i)<0) out[10+i][0]=0;
}

} // namespace m1ui

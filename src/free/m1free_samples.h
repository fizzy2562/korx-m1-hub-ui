#pragma once

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "m1_engine/m1_engine.h"

namespace m1free {

constexpr int kCycle = 239;
constexpr int kPad = 8;
constexpr int kWaves = 8;
constexpr int kVariants = 4;
constexpr int kWaveStride = kCycle + 2 * kPad;
constexpr int kDrumLen = 2400;

struct SampleSet {
    int16_t pcm[kWaves * kVariants * kWaveStride + kDrumLen + 2 * kPad];
    m1::Multisound ms[100];
    m1::Split splits[400];
    m1::DrumSound drums[2];
    char msNameBuf[100][12];
    char drumNameBuf[44][12];
    const char* msNames[100];
    const char* drumNames[44];
    uint32_t pcmWords;
};

inline void generateSamples(SampleSet& s)
{
    static const int cycles[kVariants] = {2, 4, 8, 16};
    static const int roots[kVariants] = {60, 72, 84, 96};
    static const int tops[kVariants] = {71, 83, 95, 127};
    static const char* waveNames[kWaves] = {
        "Sine", "Saw", "Square", "Triangle", "Organ", "Pulse25", "SoftSaw", "Noise"
    };
    const int totalPcm = kWaves * kVariants * kWaveStride + kDrumLen + 2 * kPad;
    memset(&s, 0, sizeof(s));

    for (int w = 0; w < kWaves; ++w) {
        for (int v = 0; v < kVariants; ++v) {
            const int base = (w * kVariants + v) * kWaveStride;
            const int start = base + kPad;
            const int hmaxV = (kCycle / cycles[v]) / 2 - 1;
            double buf[kCycle];
            double peak = 0.0;

            for (int i = 0; i < kCycle; ++i) {
                const double ph = 2.0 * M_PI * static_cast<double>(cycles[v] * i) / static_cast<double>(kCycle);
                double value = 0.0;
                if (w == 0) {
                    value = sin(ph);
                } else if (w == 1) {
                    for (int h = 1; h <= 24 && h <= hmaxV; ++h) {
                        const double sign = (h & 1) != 0 ? 1.0 : -1.0;
                        value += sign * sin(static_cast<double>(h) * ph) / static_cast<double>(h);
                    }
                } else if (w == 2) {
                    for (int h = 1; h <= 25 && h <= hmaxV; h += 2) {
                        value += sin(static_cast<double>(h) * ph) / static_cast<double>(h);
                    }
                } else if (w == 3) {
                    for (int h = 1; h <= 25 && h <= hmaxV; h += 2) {
                        const double sign = (((h - 1) / 2) & 1) != 0 ? -1.0 : 1.0;
                        value += sign * sin(static_cast<double>(h) * ph) / static_cast<double>(h * h);
                    }
                } else if (w == 4) {
                    static const int harmonic[6] = {1, 2, 3, 4, 6, 8};
                    static const double amplitude[6] = {1.0, 0.8, 0.6, 0.4, 0.25, 0.15};
                    for (int j = 0; j < 6; ++j) {
                        if (harmonic[j] <= hmaxV) {
                            value += amplitude[j] * sin(static_cast<double>(harmonic[j]) * ph);
                        }
                    }
                } else if (w == 5) {
                    const double offset = M_PI * 0.25;
                    for (int h = 1; h <= 24 && h <= hmaxV; ++h) {
                        value += (sin(static_cast<double>(h) * M_PI * 0.25) / static_cast<double>(h)) *
                                 cos(static_cast<double>(h) * (ph - offset));
                    }
                } else if (w == 6) {
                    for (int h = 1; h <= 8 && h <= hmaxV; ++h) {
                        const double sign = (h & 1) != 0 ? 1.0 : -1.0;
                        value += sign * sin(static_cast<double>(h) * ph) / static_cast<double>(h);
                    }
                } else {
                    {
                        static uint32_t nseed = 12345u;
                        if (i == 0) {
                            nseed = 12345u;
                        }
                        nseed = nseed * 1103515245u + 12345u;
                        value = (static_cast<int>((nseed >> 16) & 0x7FFFu) - 16384) / 16384.0;
                    }
                }
                buf[i] = value;
                const double absolute = fabs(value);
                if (absolute > peak) {
                    peak = absolute;
                }
            }

            for (int i = 0; i < kCycle; ++i) {
                s.pcm[start + i] = peak == 0.0 ? 0 : static_cast<int16_t>(lround(buf[i] * 4096.0 / peak));
            }
            for (int i = 0; i < kPad; ++i) {
                s.pcm[base + i] = s.pcm[start + kCycle - kPad + i];
                s.pcm[start + kCycle + i] = s.pcm[start + i];
            }
        }
    }

    const int dbase = kWaves * kVariants * kWaveStride + kPad;
    {
        uint32_t dseed = 777u;
        for (int i = 0; i < kDrumLen; ++i) {
            dseed = dseed * 1103515245u + 12345u;
            const double noise = (static_cast<int>((dseed >> 16) & 0x7FFFu) - 16384) / 16384.0;
            s.pcm[dbase + i] = static_cast<int16_t>(lround(4096.0 * exp(-static_cast<double>(i) / 500.0) * noise));
        }
    }

    for (int m = 0; m < 100; ++m) {
        const int wave = m % kWaves;
        s.ms[m].first = static_cast<uint16_t>(m * 4);
        s.ms[m].count = 4;
        s.ms[m].dwgs = 0;
        if (m < kWaves) {
            snprintf(s.msNameBuf[m], sizeof(s.msNameBuf[m]), "%s", waveNames[wave]);
        } else {
            snprintf(s.msNameBuf[m], sizeof(s.msNameBuf[m]), "Wave %02d", m);
        }
        memcpy(s.ms[m].name, s.msNameBuf[m], sizeof(s.ms[m].name));
        s.msNames[m] = s.msNameBuf[m];
        for (int v = 0; v < kVariants; ++v) {
            m1::Split& sp = s.splits[m * 4 + v];
            sp.start = static_cast<uint32_t>((wave * kVariants + v) * kWaveStride + kPad);
            sp.loop = sp.start;
            sp.end = sp.start + kCycle;
            sp.tune128 = 2;   // calibrated: measured pitch error was a constant -0.78 cents with 3
            sp.top = static_cast<uint8_t>(tops[v]);
            sp.pan = 5;
            sp.root = static_cast<uint8_t>(roots[v]);
            sp.flags = 1;
        }
    }

    s.drums[1].start = static_cast<uint32_t>(dbase);
    s.drums[1].loop = static_cast<uint32_t>(dbase);
    s.drums[1].end = static_cast<uint32_t>(dbase + kDrumLen);
    s.drums[1].tune128 = 2;
    s.drums[1].flags = 0;
    snprintf(s.drumNameBuf[0], sizeof(s.drumNameBuf[0]), "Noise Hit");
    s.drumNames[0] = s.drumNameBuf[0];
    for (int i = 1; i < 44; ++i) {
        snprintf(s.drumNameBuf[i], sizeof(s.drumNameBuf[i]), "Drum %02d", i + 1);
        s.drumNames[i] = s.drumNameBuf[i];
    }
    s.pcmWords = static_cast<uint32_t>(totalPcm);
}

}

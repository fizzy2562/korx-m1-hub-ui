#include "m1free_samples.h"

static int check(int condition, const char* name)
{
    if (condition != 0) {
        printf("PASS %s\n", name);
        return 0;
    }
    printf("FAIL %s detail\n", name);
    return 1;
}

int main()
{
    static m1free::SampleSet samples;
    m1free::generateSamples(samples);
    int failures = 0;
    const int cycles[m1free::kVariants] = {2, 4, 8, 16};
    const int roots[m1free::kVariants] = {60, 72, 84, 96};
    const int tops[m1free::kVariants] = {71, 83, 95, 127};

    int multisoundOk = 1;
    for (int m = 0; m < 100; ++m) {
        if (samples.ms[m].count != 4 || samples.ms[m].first != static_cast<uint16_t>(4 * m)) {
            multisoundOk = 0;
        }
        for (int v = 0; v < m1free::kVariants; ++v) {
            const m1::Split& sp = samples.splits[samples.ms[m].first + v];
            if (sp.end - sp.start != m1free::kCycle || sp.loop != sp.start ||
                sp.top != tops[v] || sp.root != roots[v] || sp.flags != 1 || sp.tune128 != 2 ||
                sp.start < 8 || sp.end + 4 > samples.pcmWords) {
                multisoundOk = 0;
            }
        }
    }
    failures += check(multisoundOk, "multisounds_and_splits");

    int paddingOk = 1;
    for (int w = 0; w < m1free::kWaves; ++w) {
        for (int v = 0; v < m1free::kVariants; ++v) {
            const int base = (w * m1free::kVariants + v) * m1free::kWaveStride;
            const int start = base + m1free::kPad;
            for (int i = 0; i < m1free::kPad; ++i) {
                if (samples.pcm[base + i] != samples.pcm[start + m1free::kCycle - m1free::kPad + i] ||
                    samples.pcm[start + m1free::kCycle + i] != samples.pcm[start + i]) {
                    paddingOk = 0;
                }
            }
        }
    }
    failures += check(paddingOk, "wave_padding");

    int peakOk = 1;
    int sineOk = 1;
    for (int w = 0; w < m1free::kWaves; ++w) {
        for (int v = 0; v < m1free::kVariants; ++v) {
            const int start = (w * m1free::kVariants + v) * m1free::kWaveStride + m1free::kPad;
            int peak = 0;
            for (int i = 0; i < m1free::kCycle; ++i) {
                const int value = samples.pcm[start + i];
                const int absolute = value < 0 ? -value : value;
                if (absolute > peak) {
                    peak = absolute;
                }
                if (w == 0) {
                    const double expected = 4096.0 * sin(2.0 * M_PI * cycles[v] * i / 239.0);
                    if (fabs(static_cast<double>(value) - expected) > 2.0) {
                        sineOk = 0;
                    }
                }
            }
            if (peak != 4096 || (w == 0 && samples.pcm[start] < -1) || (w == 0 && samples.pcm[start] > 1)) {
                peakOk = 0;
            }
        }
    }
    failures += check(peakOk, "all_wave_peaks");
    failures += check(sineOk, "sine_accuracy");

    int periodicOk = 1;
    for (int i = 0; i <= 100; ++i) {
        const int a = samples.pcm[m1free::kPad + i];
        const int b = samples.pcm[m1free::kPad + i + 120];
        if (a - b > 600 || b - a > 600) {
            periodicOk = 0;
        }
    }
    failures += check(periodicOk, "sine_periodicity");

    const m1::DrumSound& drum = samples.drums[1];
    failures += check(drum.end - drum.start == m1free::kDrumLen && drum.flags == 0 && samples.pcm[drum.start] != 0,
                      "drum");
    failures += check(strcmp(samples.msNames[0], "Sine") == 0 && strcmp(samples.msNames[8], "Wave 08") == 0 &&
                      strcmp(samples.msNames[99], "Wave 99") == 0 && strcmp(samples.drumNames[0], "Noise Hit") == 0,
                      "names");
    return failures == 0 ? 0 : 1;
}

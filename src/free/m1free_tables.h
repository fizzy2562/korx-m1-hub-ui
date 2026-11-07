#pragma once
#include <stdint.h>
#include <math.h>

namespace m1free {

struct Tables {
    uint16_t pitchS[153], pitchF[128];
    uint8_t  kernel[4096 * 8];
    uint16_t vdf1k[4096], vdf2k[4096];
    int16_t  vdf1t0[4096], vdf2t0[4096];
    uint16_t egRise[256], egFall[256], velCurve[256];
    uint16_t mgFreq[100], mgDelay[100], mgIntP[100], mgIntF[100];
    uint16_t panReg[14], pan101[101], delayStart[100];
    int8_t   scale[12];
};

static inline long rnd(double x) {
    return (long)floor(x + 0.5);
}

static inline uint16_t clamp16(double x) {
    if (x < 0.0) x = 0.0;
    if (x > 65535.0) x = 65535.0;
    return (uint16_t)rnd(x);
}

static inline int16_t clamp16s(double x) {
    if (x < -32768.0) x = -32768.0;
    if (x > 32767.0) x = 32767.0;
    return (int16_t)rnd(x);
}

static inline int8_t clamp8s(double x) {
    if (x < -128.0) x = -128.0;
    if (x > 127.0) x = 127.0;
    return (int8_t)rnd(x);
}

static inline uint8_t clamp8u(double x) {
    if (x < 0.0) x = 0.0;
    if (x > 255.0) x = 255.0;
    return (uint8_t)rnd(x);
}

inline void generateTables(Tables& t) {
    const double PI = 3.14159265358979323846;

    // 1. pitchS
    for (int hi = 0; hi < 153; ++hi) {
        int n = hi - 128;
        double val = 4096.0 * pow(2.0, n / 12.0);
        t.pitchS[hi] = clamp16(val);
    }

    // 2. pitchF
    for (int lo = 0; lo < 128; ++lo) {
        double val = 65536.0 * (pow(2.0, lo / 1536.0) - 1.0);
        t.pitchF[lo] = clamp16(val);
    }

    // 3. kernel
    const int mul[8] = {1, 1, 2, 8, 8, 2, 1, 1};
    for (int p = 0; p < 4096; ++p) {
        double f = (double)p / 4096.0;
        double h[8];
        double H = 0.0;
        for (int l = 0; l < 8; ++l) {
            double d = (4.0 - (double)l) - f;
            double sinc_val = (d == 0.0) ? 1.0 : sin(PI * d) / (PI * d);
            double win_val = 0.0;
            if (fabs(d) < 4.5) {
                win_val = 0.5 * (1.0 + cos(PI * d / 4.5));
            }
            h[l] = sinc_val * win_val;
            H += h[l];
        }

        int w[8];
        for (int l = 0; l < 8; ++l) {
            double w_real = (h[l] / H) * 1226.0 / (double)mul[l];
            if (l == 3 || l == 4) {
                w[l] = (int)clamp8u(w_real);
            } else {
                w[l] = (int)clamp8s(w_real);
            }
        }

        // DC Gain Correction
        int sum = 0;
        for (int l = 0; l < 8; ++l) sum += mul[l] * w[l];
        int R = 1226 - sum;

        // Step 1: Fix with larger of lanes 3/4 by R/8
        if (R != 0) {
            int target = (w[3] >= w[4]) ? 3 : 4;
            int adj = R / 8;
            if (adj != 0) {
                int new_val = w[target] + adj;
                if (new_val >= 0 && new_val <= 255) {
                    w[target] = new_val;
                    sum += mul[target] * adj;
                    R = 1226 - sum;
                }
            }
        }

        // Step 2: Fix with lane 2 or 5 by R/2
        if (R != 0) {
            int target = (w[2] >= w[5]) ? 2 : 5;
            int adj = R / 2;
            if (adj != 0) {
                int new_val = w[target] + adj;
                if (new_val >= -128 && new_val <= 127) {
                    w[target] = new_val;
                    sum += mul[target] * adj;
                    R = 1226 - sum;
                }
            }
        }

        // Step 3: Fix with lane 0 or 7 by R/1
        if (R != 0) {
            int target = (w[0] >= w[7]) ? 0 : 7;
            int adj = R;
            if (adj != 0) {
                int new_val = w[target] + adj;
                if (new_val >= -128 && new_val <= 127) {
                    w[target] = new_val;
                    sum += mul[target] * adj;
                    R = 1226 - sum;
                }
            }
        }

        // Store
        for (int l = 0; l < 8; ++l) {
            if (l == 3 || l == 4) {
                t.kernel[p * 8 + l] = (uint8_t)w[l];
            } else {
                t.kernel[p * 8 + l] = (uint8_t)(int8_t)w[l];
            }
        }
    }

    // 4. mgFreq
    for (int v = 0; v < 100; ++v) {
        if (v == 0) t.mgFreq[v] = 0;
        else t.mgFreq[v] = clamp16(1.64 * v * v);
    }

    // 5. mgDelay
    for (int v = 0; v < 100; ++v) {
        if (v == 0) {
            t.mgDelay[v] = 65535;
        } else {
            double T = 0.05 * pow(100.0, v / 99.0);
            double step = 32768.0 / (T / 0.0208);
            if (step < 1.0) step = 1.0;
            t.mgDelay[v] = clamp16(step);
        }
    }

    // 6. mgIntP, mgIntF
    for (int v = 0; v < 100; ++v) {
        double val = 8191.0 * pow((double)v / 99.0, 2.0);
        if (val > 8191.0) val = 8191.0;
        if (val < 0.0) val = 0.0;
        t.mgIntP[v] = (uint16_t)rnd(val);
        t.mgIntF[v] = (uint16_t)rnd(val);
    }

    // 7. egRise
    for (int idx = 0; idx < 256; ++idx) {
        double N = pow(1500.0, idx / 255.0);
        double val = 65500.0 / N;
        if (val < 1.0) val = 1.0;
        t.egRise[idx] = clamp16(val);
    }

    // 8. egFall
    for (int idx = 0; idx < 256; ++idx) {
        double N = pow(1500.0, idx / 255.0);
        double m = pow(0.004, 1.0 / N);
        double val = 65536.0 * (1.0 - m);
        if (val < 1.0) val = 1.0;
        t.egFall[idx] = clamp16(val);
    }

    // 9. velCurve
    for (int i = 0; i < 256; ++i) {
        if (i >= 252) {
            t.velCurve[i] = 65535;
        } else {
            double val = 65535.0 * pow((double)i / 252.0, 1.6);
            t.velCurve[i] = clamp16(val);
        }
    }

    // 10. panReg
    for (int code = 0; code < 14; ++code) {
        uint16_t A = 0, B = 0;
        int C = 0, D = 0;
        if (code <= 10) {
            A = (uint16_t)rnd(63.0 * cos(code * PI / 20.0));
            B = (uint16_t)rnd(63.0 * sin(code * PI / 20.0));
        } else if (code == 11) {
            C = 1;
        } else if (code == 12) {
            C = 1; D = 1;
        } else if (code == 13) {
            D = 1;
        }
        t.panReg[code] = (A & 0x3F) | ((B & 0x3F) << 6) | (C << 12) | (D << 13);
    }

    // 11. pan101
    for (int i = 0; i < 101; ++i) {
        double x = (double)i / 100.0;
        double val = 512.0 * (3.0 * x * x - 2.0 * x * x * x);
        t.pan101[i] = clamp16(val);
    }

    // 12. delayStart
    for (int v = 0; v < 100; ++v) {
        t.delayStart[v] = clamp16(0.08 * v * v);
    }

    // 13. scale
    double justCents[12] = {0, 111.731, 203.910, 315.641, 386.314, 498.045, 590.224, 701.955, 813.686, 884.359, 1017.596, 1088.269};
    for (int k = 0; k < 12; ++k) {
        double val = (justCents[k] - 100.0 * k) * 1.28;
        t.scale[k] = clamp8s(val);
    }

    // 14. VDF tables
    for (int i = 0; i < 4096; ++i) {
        int reg = 2 * i + 1;
        double fc = 20.0 * pow(2.0, 10.0 * reg / 8191.0);
        double kp = 1.0 - exp(-2.0 * PI * fc / 62500.0);
        if (kp < 0.0) kp = 0.0;
        if (kp > 1.0) kp = 1.0;
        double kt = kp;
        if (kt > 0.97) kt = 0.97;

        t.vdf2k[i] = clamp16(8192.0 * kp);
        t.vdf1k[i] = clamp16(8192.0 * 0.707 * kp);

        t.vdf2t0[i] = clamp16s(8192.0 * (2.0 * kt - 1.0));
        double min_kp = 0.707 * kp;
        if (min_kp > 0.97) min_kp = 0.97;
        t.vdf1t0[i] = clamp16s(8192.0 * (2.0 * min_kp - 1.0));
    }
}

} // namespace m1free

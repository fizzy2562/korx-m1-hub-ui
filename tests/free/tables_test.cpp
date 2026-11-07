#include "m1free_tables.h"
#include <stdio.h>

static int failures = 0;

void check(bool cond, const char* name, const char* detail) {
    if (cond) {
        printf("PASS %s\n", name);
    } else {
        printf("FAIL %s %s\n", name, detail);
        failures++;
    }
}

int main() {
    static m1free::Tables t;
    m1free::generateTables(t);

    // 1. pitchS checks
    check(t.pitchS[128] == 4096, "pitchS[128]", "expected 4096");
    check(t.pitchS[140] >= 8191 && t.pitchS[140] <= 8193, "pitchS[140]", "expected ~8192");
    check(t.pitchS[116] >= 2047 && t.pitchS[116] <= 2049, "pitchS[116]", "expected ~2048");

    // 2. pitchF checks
    check(t.pitchF[0] == 0, "pitchF[0]", "expected 0");
    double expected_pf127 = 65536.0 * (pow(2.0, 127.0 / 1536.0) - 1.0);
    long rounded_pf127 = (long)floor(expected_pf127 + 0.5);
    check(t.pitchF[127] >= rounded_pf127 - 2 && t.pitchF[127] <= rounded_pf127 + 2, "pitchF[127]", "expected ~rounded value");

    // 3. Kernel DC sum checks
    int bad_phases = 0;
    const int mul[8] = {1, 1, 2, 8, 8, 2, 1, 1};
    for (int p = 0; p < 4096; ++p) {
        int sum = 0;
        for (int l = 0; l < 8; ++l) {
            int w;
            if (l == 3 || l == 4) {
                w = (int)t.kernel[p * 8 + l];
            } else {
                w = (int)(int8_t)t.kernel[p * 8 + l];
            }
            sum += mul[l] * w;
        }
        if (sum != 1226) {
            bad_phases++;
        }
    }
    char detail[64];
    sprintf(detail, "%d phases failed", bad_phases);
    check(bad_phases == 0, "kernel_dc_sum", detail);

    // Phase 0 specific checks
    int w0_3 = (int)t.kernel[0 * 8 + 3];
    int w0_4 = (int)t.kernel[0 * 8 + 4];
    check(w0_4 > w0_3 && w0_4 > (int)(int8_t)t.kernel[0 * 8 + 0] && w0_4 > (int)(int8_t)t.kernel[0 * 8 + 1] && w0_4 > (int)(int8_t)t.kernel[0 * 8 + 2] && w0_4 > (int)(int8_t)t.kernel[0 * 8 + 5] && w0_4 > (int)(int8_t)t.kernel[0 * 8 + 6] && w0_4 > (int)(int8_t)t.kernel[0 * 8 + 7], "kernel_phase0_lane4_max", "lane 4 should be largest");
    check(w0_3 >= -3 && w0_3 <= 3, "kernel_phase0_lane3_near_0", "lane 3 should be near 0");

    // Phase 2048 specific checks
    int w2048_3 = (int)t.kernel[2048 * 8 + 3];
    int w2048_4 = (int)t.kernel[2048 * 8 + 4];
    int diff = w2048_3 - w2048_4;
    if (diff < 0) diff = -diff;
    check(diff <= 2, "kernel_phase2048_lanes34_close", "lanes 3 and 4 should be close");

    // 4. mgFreq checks
    check(t.mgFreq[0] == 0, "mgFreq[0]", "expected 0");
    check(t.mgFreq[99] >= 16073 && t.mgFreq[99] <= 16075, "mgFreq[99]", "expected ~16074");

    // 5. mgDelay checks
    check(t.mgDelay[0] == 65535, "mgDelay[0]", "expected 65535");

    // 6. egRise checks
    bool egRise_ok = true;
    for (int i = 1; i < 256; ++i) {
        if (t.egRise[i] > t.egRise[i - 1]) {
            egRise_ok = false;
            break;
        }
    }
    check(egRise_ok, "egRise_non_increasing", "should be non-increasing");
    check(t.egRise[0] >= 65000, "egRise[0]", "expected >= 65000");

    // 7. egFall checks
    bool egFall_ok = true;
    for (int i = 1; i < 256; ++i) {
        if (t.egFall[i] > t.egFall[i - 1]) {
            egFall_ok = false;
            break;
        }
    }
    check(egFall_ok, "egFall_non_increasing", "should be non-increasing");

    // 8. velCurve checks
    check(t.velCurve[252] == 65535, "velCurve[252]", "expected 65535");
    check(t.velCurve[255] == 65535, "velCurve[255]", "expected 65535");
    bool velCurve_ok = true;
    for (int i = 1; i <= 252; ++i) {
        if (t.velCurve[i] < t.velCurve[i - 1]) {
            velCurve_ok = false;
            break;
        }
    }
    check(velCurve_ok, "velCurve_non_decreasing", "should be non-decreasing up to 252");
    check(t.velCurve[0] == 0, "velCurve[0]", "expected 0");

    // 9. panReg checks
    check(t.panReg[0] == 63, "panReg[0]", "expected 63");
    int A5 = t.panReg[5] & 0x3F;
    int B5 = (t.panReg[5] >> 6) & 0x3F;
    int diff5 = A5 - B5;
    if (diff5 < 0) diff5 = -diff5;
    check(diff5 <= 1, "panReg[5]", "A and B should be close");
    check((t.panReg[10] & 0x3F) == 0 && ((t.panReg[10] >> 6) & 0x3F) == 63, "panReg[10]", "expected A=0, B=63");
    check(t.panReg[11] == 0x1000, "panReg[11]", "expected 0x1000");
    check(t.panReg[12] == 0x3000, "panReg[12]", "expected 0x3000");
    check(t.panReg[13] == 0x2000, "panReg[13]", "expected 0x2000");

    // 10. pan101 checks
    check(t.pan101[0] == 0, "pan101[0]", "expected 0");
    check(t.pan101[100] == 512, "pan101[100]", "expected 512");
    check(t.pan101[50] == 256, "pan101[50]", "expected 256");

    // 11. scale checks
    check(t.scale[4] >= -19 && t.scale[4] <= -17, "scale[4]", "expected ~-18");
    check(t.scale[0] == 0, "scale[0]", "expected 0");

    // 12. vdf2k checks
    bool vdf2k_ok = true;
    for (int i = 1; i < 4096; ++i) {
        if (t.vdf2k[i] < t.vdf2k[i - 1]) {
            vdf2k_ok = false;
            break;
        }
    }
    check(vdf2k_ok, "vdf2k_non_decreasing", "should be non-decreasing");
    check(t.vdf2k[4095] >= 7000, "vdf2k[4095]", "expected >= 7000");
    check(t.vdf2k[0] <= 20, "vdf2k[0]", "expected <= 20");

    // 13. vdf1k <= vdf2k checks
    bool vdf1k_ok = true;
    for (int i = 0; i < 4096; ++i) {
        if (t.vdf1k[i] > t.vdf2k[i]) {
            vdf1k_ok = false;
            break;
        }
    }
    check(vdf1k_ok, "vdf1k_le_vdf2k", "vdf1k should be <= vdf2k");

    // 14. vdf2t0 checks
    bool vdf2t0_ok = true;
    for (int i = 0; i < 4096; ++i) {
        int val = t.vdf2t0[i];
        if (val < -8192 || val > 8192) {
            vdf2t0_ok = false;
            break;
        }
    }
    check(vdf2t0_ok, "vdf2t0_range", "should be within [-8192, 8192]");

    if (failures > 0) {
        printf("FAILED: %d checks failed\n", failures);
        return 1;
    }
    printf("ALL TESTS PASSED\n");
    return 0;
}

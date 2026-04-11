#include "ProgressionEngine.h"

namespace ChordMatrix
{
    void ProgressionEngine::applyModulation(const std::array<StepData, TotalSteps>& source,
        std::array<StepData, TotalSteps>& dest,
        int targetBar, int targetKey, int targetScale, int method,
        int stepsPerBar, int stepsPerBeat, float ppqPerStep)
    {
        dest = source;
        if (targetBar <= 0 || targetBar >= 16) return;

        float chordGate = static_cast<float>(stepsPerBeat) * ppqPerStep;

        int targetStepStart = targetBar * stepsPerBar;
        dest[targetStepStart].keyRoot = targetKey;
        dest[targetStepStart].scaleType = targetScale;
        dest[targetStepStart].chordDegree = 0;
        for (int v = 0; v < 7; ++v) {
            dest[targetStepStart].voices[v].isActive = (v == 0 || v == 1 || v == 2 || v == 3);
        }
        dest[targetStepStart].voicingMode = 0;
        dest[targetStepStart].gateLength = chordGate * 2.0f;

        int prevBar = targetBar - 1;
        int prevStepStart = prevBar * stepsPerBar;
        int beat2FromEnd = stepsPerBar - 2 * stepsPerBeat;
        int beat1FromEnd = stepsPerBar - stepsPerBeat;

        float modulationStartPPQ = static_cast<float>(beat2FromEnd) * ppqPerStep;
        for (int i = 0; i < beat2FromEnd; ++i) {
            float currentStepPPQ = static_cast<float>(i) * ppqPerStep;
            float maxAllowedGate = modulationStartPPQ - currentStepPPQ;
            if (dest[prevStepStart + i].gateLength > maxAllowedGate) {
                dest[prevStepStart + i].gateLength = maxAllowedGate;
            }
        }

        for (int i = beat2FromEnd; i < stepsPerBar; ++i) {
            for (int v = 0; v < 7; ++v) dest[prevStepStart + i].voices[v].isActive = false;
        }

        int s_ii = prevStepStart + beat2FromEnd;
        int s_V = prevStepStart + beat1FromEnd;

        if (method == TwoFiveOne && beat2FromEnd >= 0) {
            dest[s_ii].keyRoot = targetKey; dest[s_ii].scaleType = targetScale; dest[s_ii].chordDegree = 1;
            for (int v = 0; v < 7; ++v) dest[s_ii].voices[v].isActive = (v == 0 || v == 1 || v == 2 || v == 3);
            dest[s_ii].voicingMode = 0; dest[s_ii].gateLength = chordGate;

            dest[s_V].keyRoot = targetKey; dest[s_V].scaleType = 0; dest[s_V].chordDegree = 4;
            for (int v = 0; v < 7; ++v) dest[s_V].voices[v].isActive = (v == 0 || v == 1 || v == 2 || v == 3);
            dest[s_V].voicingMode = 0; dest[s_V].gateLength = chordGate;
        }
        else if (method == TritoneSub && beat2FromEnd >= 0) {
            int subVKey = (targetKey + 1) % 12;
            dest[s_ii].keyRoot = subVKey; dest[s_ii].scaleType = 7; dest[s_ii].chordDegree = 0;
            for (int v = 0; v < 7; ++v) dest[s_ii].voices[v].isActive = (v == 0 || v == 1 || v == 2 || v == 3);
            dest[s_ii].voicingMode = 0; dest[s_ii].gateLength = chordGate * 2.0f;
        }
        else {
            dest[s_ii].keyRoot = targetKey; dest[s_ii].scaleType = 0; dest[s_ii].chordDegree = 4;
            for (int v = 0; v < 7; ++v) dest[s_ii].voices[v].isActive = (v == 0 || v == 1 || v == 2 || v == 3);
            dest[s_ii].voicingMode = 0; dest[s_ii].gateLength = chordGate * 2.0f;
        }
    }

    // ★修正: 日本語(マルチバイト文字)を完全に排除し、安全なピュアASCII文字列に置換
    const std::vector<ProgressionPreset>& ProgressionEngine::getProgressionDictionary() {
        static std::vector<ProgressionPreset> dict;
        if (dict.empty()) {

            // J-Pop / Anime Standards
            dict.push_back({ "J-Pop / Anime Standards", "Royal Road (IV-V-iii-vi)", 4, {
                {0, 4, 3, 5, 0,0,0,0}, {4, 4, 4, 4, 0,0,0,0}, {8, 4, 2, 4, 0,0,0,0}, {12, 4, 5, 4, 0,0,0,0}
            }, "IVmaj7 - V7 - iiim7 - vim7" });

            dict.push_back({ "J-Pop / Anime Standards", "TK Progression (vi-IV-V-I)", 4, {
                {0, 4, 5, 4, 0,0,0,0}, {4, 4, 3, 5, 0,0,0,0}, {8, 4, 4, 4, 0,0,0,0}, {12, 4, 0, 5, 0,0,0,0}
            }, "vim7 - IVmaj7 - V7 - Imaj7" });

            dict.push_back({ "J-Pop / Anime Standards", "Anime Standard (vi-V-IV-V)", 4, {
                {0, 4, 5, 4, 0,0,0,0}, {4, 4, 4, 4, 0,0,0,0}, {8, 4, 3, 5, 0,0,0,0}, {12, 4, 4, 4, 0,0,0,0}
            }, "vim7 - V7 - IVmaj7 - V7" });

            // R&B / Neo-Soul / Jazz
            dict.push_back({ "R&B / Neo-Soul / Jazz", "Just The Two Of Us (IV-III-vi-I)", 4, {
                {0, 4, 3, 5, 0,0,0,0}, {4, 4, 2, 4, 0,1,0,0}, {8, 4, 5, 4, 0,0,0,0}, {12, 4, 0, 4, 0,-1,0,-1}
            }, "IVmaj7 - III7 - vim7 - Im7" });

            dict.push_back({ "R&B / Neo-Soul / Jazz", "2-5-1 Progression", 4, {
                {0, 4, 1, 4, 0,0,0,0}, {4, 4, 4, 4, 0,0,0,0}, {8, 4, 0, 5, 0,0,0,0}, {12, 4, 0, 5, 0,0,0,0}
            }, "iim7 - V7 - Imaj7 - Imaj7" });

            dict.push_back({ "R&B / Neo-Soul / Jazz", "Autumn Leaves", 8, {
                {0, 4, 1, 4, 0,0,0,0}, {4, 4, 4, 4, 0,0,0,0}, {8, 4, 0, 5, 0,0,0,0}, {12, 4, 3, 5, 0,0,0,0},
                {16, 4, 6, 0, 0,0,-1,0}, {20, 4, 2, 4, 0,1,0,0}, {24, 4, 5, 4, 0,0,0,0}, {28, 4, 5, 4, 0,0,0,0}
            }, "iim7 - V7 - Imaj7 - IVmaj7 - viim7b5 - III7 - vim7" });

            // Emotional / Modern
            dict.push_back({ "Emotional / Modern", "Subdominant Minor", 4, {
                {0, 4, 3, 5, 0,0,0,0}, {4, 4, 3, 4, 0,-1,0,-1}, {8, 4, 0, 5, 0,0,0,0}, {12, 4, 0, 5, 0,0,0,0}
            }, "IVmaj7 - ivm7 - Imaj7 - Imaj7" });

            dict.push_back({ "Emotional / Modern", "Descending 4-3-2-1", 4, {
                {0, 4, 3, 5, 0,0,0,0}, {4, 4, 2, 4, 0,0,0,0}, {8, 4, 1, 4, 0,0,0,0}, {12, 4, 0, 5, 0,0,0,0}
            }, "IVmaj7 - iiim7 - iim7 - Imaj7" });

            dict.push_back({ "Emotional / Modern", "Pretender (IV-V-III-vi)", 4, {
                {0, 4, 3, 5, 0,0,0,0}, {4, 4, 4, 4, 0,0,0,0}, {8, 4, 2, 4, 0,1,0,0}, {12, 4, 5, 4, 0,0,0,0}
            }, "IVmaj7 - V7 - III7 - vim7" });

            // Classic / Standard
            dict.push_back({ "Classic / Standard", "Canon Progression", 4, {
                {0, 2, 0, 3, 0,0,0,-128}, {2, 2, 4, 3, 0,0,0,-128}, {4, 2, 5, 3, 0,0,0,-128}, {6, 2, 2, 3, 0,0,0,-128},
                {8, 2, 3, 3, 0,0,0,-128}, {10, 2, 0, 3, 0,0,0,-128}, {12, 2, 3, 3, 0,0,0,-128}, {14, 2, 4, 3, 0,0,0,-128}
            }, "I - V - vim - iiim - IV - I - IV - V" });

            dict.push_back({ "Classic / Standard", "Doo-wop (1-6-4-5)", 4, {
                {0, 4, 0, 3, 0,0,0,-128}, {4, 4, 5, 3, 0,0,0,-128}, {8, 4, 3, 3, 0,0,0,-128}, {12, 4, 4, 3, 0,0,0,-128}
            }, "I - vim - IV - V" });

            dict.push_back({ "Classic / Standard", "Let It Be (1-5-6-4)", 4, {
                {0, 4, 0, 3, 0,0,0,-128}, {4, 4, 4, 3, 0,0,0,-128}, {8, 4, 5, 3, 0,0,0,-128}, {12, 4, 3, 3, 0,0,0,-128}
            }, "I - V - vim - IV" });

            dict.push_back({ "Classic / Standard", "Pop Punk (vi-IV-I-V)", 4, {
                {0, 4, 5, 3, 0,0,0,-128}, {4, 4, 3, 3, 0,0,0,-128}, {8, 4, 0, 3, 0,0,0,-128}, {12, 4, 4, 3, 0,0,0,-128}
            }, "vim - IV - I - V" });

            // Advanced / Theory
            dict.push_back({ "Advanced / Theory", "Line Cliche (Minor)", 4, {
                {0, 4, 5, 0, 0,0,0,-128}, {4, 4, 5, 0, 0,0,0,-1}, {8, 4, 5, 0, 0,0,0,-2}, {12, 4, 5, 0, 0,0,0,-3}
            }, "vim - vim(maj7) - vim7 - vim6" });

            dict.push_back({ "Advanced / Theory", "Andalusian Cadence", 4, {
                {0, 4, 5, 3, 0,0,0,-128}, {4, 4, 4, 3, 0,0,0,-128}, {8, 4, 3, 3, 0,0,0,-128}, {12, 4, 2, 3, 0,1,0,-128}
            }, "vim - V - IV - III" });
        }
        return dict;
    }
}
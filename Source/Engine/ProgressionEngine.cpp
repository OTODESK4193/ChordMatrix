#include "ProgressionEngine.h"
#include <cstdint> // for int64_t

namespace ChordMatrix
{
    juce::StringArray ProgressionEngine::getModulationNames() {
        return {
            "Direct (V7 -> I)",
            "Standard ii-V-I",
            "Tritone Sub (bII7)",
            "Minor ii-V-i",
            "Backdoor (IVm7-bVII7)",
            "Passing Diminished",
            "Secondary Dominant",
            "Double ii-V",
            "Coltrane Matrix",
            "Extended Dominant",
            "Chromatic Up",
            "Chromatic Down",
            "Deceptive (V7->vi)",
            "Constant Structure",
            "Pedal Point",
            "Neo-Riemannian P",
            "Neo-Riemannian L",
            "Neo-Riemannian R"
        };
    }

    // =========================================================================
    // ★修正: 古いstd::vector<int>の記述を削除し、ChordSuggestionを返す推論エンジンに統合
    // =========================================================================
    std::vector<ChordSuggestion> ProgressionEngine::suggestNextChords(int currentDegree, int scaleType) {
        std::vector<ChordSuggestion> suggestions;

        bool isMinor = (scaleType == 1 || scaleType == 2 || scaleType == 3);

        if (currentDegree == 4) {
            // V (Dominant) -> Requires Resolution
            suggestions.push_back({ 0, scaleType, 0, 0.70f, "Authentic Cadence (V -> I)" });
            suggestions.push_back({ 5, scaleType, 0, 0.15f, "Deceptive Cadence (V -> vi)" });
            if (!isMinor) suggestions.push_back({ 2, scaleType, 0, 0.05f, "Deceptive Sub (V -> iii)" });
            suggestions.push_back({ 0, isMinor ? 0 : 1, 0, 0.10f, "Neo-Riemannian P (Parallel)" });
        }
        else if (currentDegree == 1) {
            // ii (Subdominant / Pre-Dominant)
            suggestions.push_back({ 4, scaleType, 0, 0.75f, "Standard Jazz Motion (ii -> V)" });
            suggestions.push_back({ 0, 7, 1, 0.15f, "Tritone Sub (ii -> bII7)" });
            suggestions.push_back({ 0, scaleType, 0, 0.10f, "Plagal Variant (ii -> I)" });
        }
        else if (currentDegree == 3) {
            // IV (Subdominant)
            suggestions.push_back({ 4, scaleType, 0, 0.40f, "Functional Motion (IV -> V)" });
            suggestions.push_back({ 0, scaleType, 0, 0.30f, "Plagal Cadence (IV -> I)" });
            if (!isMinor) suggestions.push_back({ 3, 1, 0, 0.20f, "Modal Interchange (IV -> ivm)" });
            suggestions.push_back({ 1, scaleType, 0, 0.10f, "Subdominant Chain (IV -> ii)" });
        }
        else if (currentDegree == 0) {
            // I (Tonic)
            suggestions.push_back({ 3, scaleType, 0, 0.35f, "Tonic to Subdominant (I -> IV)" });
            suggestions.push_back({ 5, scaleType, 0, 0.25f, "Tonic to Relative Minor (I -> vi)" });
            suggestions.push_back({ 1, scaleType, 0, 0.20f, "Tonic to Supertonic (I -> ii)" });
            suggestions.push_back({ 2, scaleType, 0, 0.10f, "Tonic to Mediant (I -> iii)" });
            suggestions.push_back({ 0, isMinor ? 0 : 1, isMinor ? 4 : 8, 0.05f, "Neo-Riemannian L (Leading-tone)" });
            suggestions.push_back({ 0, isMinor ? 0 : 1, isMinor ? 3 : 9, 0.05f, "Neo-Riemannian R (Relative)" });
        }
        else if (currentDegree == 5) {
            // vi (Submediant)
            suggestions.push_back({ 1, scaleType, 0, 0.40f, "Cycle of 5ths (vi -> ii)" });
            suggestions.push_back({ 3, scaleType, 0, 0.35f, "Subdominant Approach (vi -> IV)" });
            suggestions.push_back({ 4, scaleType, 0, 0.25f, "Direct to Dominant (vi -> V)" });
        }
        else if (currentDegree == 2) {
            // iii (Mediant)
            suggestions.push_back({ 5, scaleType, 0, 0.60f, "Cycle of 5ths (iii -> vi)" });
            suggestions.push_back({ 3, scaleType, 0, 0.40f, "Stepwise Descent (iii -> IV)" });
        }

        // Fallback for edge cases
        if (suggestions.empty()) {
            suggestions.push_back({ 0, scaleType, 0, 0.50f, "Resolve to Tonic" });
            suggestions.push_back({ (currentDegree + 3) % 7, scaleType, 0, 0.50f, "Cycle of 5ths fall" });
        }

        return suggestions;
    }

    void ProgressionEngine::applyModulation(const std::array<StepData, TotalSteps>& source,
        std::array<StepData, TotalSteps>& dest,
        int targetBar, int targetKey, int targetScale, int method,
        int stepsPerBar, int stepsPerBeat, float ppqPerStep)
    {
        dest = source;
        if (targetBar <= 0 || targetBar >= 16) return;

        // ★修正: オーバーフロー警告(C26451)を防ぐための安全なキャスト計算
        float chordGate = static_cast<float>(stepsPerBeat) * ppqPerStep;
        int targetStepStart = static_cast<int>(static_cast<int64_t>(targetBar) * static_cast<int64_t>(stepsPerBar));

        dest[targetStepStart].keyRoot = targetKey;
        dest[targetStepStart].scaleType = targetScale;
        dest[targetStepStart].chordDegree = 0;
        dest[targetStepStart].voicingMode = 0;

        for (int v = 0; v < 7; ++v) {
            dest[targetStepStart].voices[v].isActive = (v < 4);
        }
        dest[targetStepStart].gateLength = chordGate * 2.0f;

        int prevBar = targetBar - 1;
        int prevStepStart = static_cast<int>(static_cast<int64_t>(prevBar) * static_cast<int64_t>(stepsPerBar));
        int b2 = static_cast<int>(static_cast<int64_t>(stepsPerBar) - 2LL * static_cast<int64_t>(stepsPerBeat));
        int b1 = static_cast<int>(static_cast<int64_t>(stepsPerBar) - 1LL * static_cast<int64_t>(stepsPerBeat));

        float modStartPPQ = static_cast<float>(b2) * ppqPerStep;
        for (int i = 0; i < b2; ++i) {
            float currentStepPPQ = static_cast<float>(i) * ppqPerStep;
            float maxGate = modStartPPQ - currentStepPPQ;
            if (dest[prevStepStart + i].gateLength > maxGate) {
                dest[prevStepStart + i].gateLength = maxGate;
            }
        }

        for (int i = b2; i < stepsPerBar; ++i) {
            for (int v = 0; v < 7; ++v) {
                dest[prevStepStart + i].voices[v].isActive = false;
            }
            dest[prevStepStart + i].voicingMode = 0;
            dest[prevStepStart + i].shift = 0;
        }

        int s_ii = prevStepStart + b2;
        int s_V = prevStepStart + b1;

        if (method >= 15 && method <= 17) {
            bool targetIsMinor = (targetScale == 1 || targetScale == 2 || targetScale == 3 || targetScale == 4);
            dest[s_V].voicingMode = 0;
            dest[s_V].gateLength = chordGate * 2.0f;

            if (method == NeoRiemannianP) {
                dest[s_V].keyRoot = targetKey;
                dest[s_V].scaleType = targetIsMinor ? 0 : 1;
                dest[s_V].chordDegree = 0;
            }
            else if (method == NeoRiemannianL) {
                dest[s_V].keyRoot = targetIsMinor ? (targetKey + 8) % 12 : (targetKey + 4) % 12;
                dest[s_V].scaleType = targetIsMinor ? 0 : 1;
                dest[s_V].chordDegree = 0;
            }
            else if (method == NeoRiemannianR) {
                dest[s_V].keyRoot = targetIsMinor ? (targetKey + 3) % 12 : (targetKey + 9) % 12;
                dest[s_V].scaleType = targetIsMinor ? 0 : 1;
                dest[s_V].chordDegree = 0;
            }

            for (int v = 0; v < 4; ++v) {
                dest[s_V].voices[v].isActive = true;
            }
            return;
        }

        if (method == TwoFiveOne && b2 >= 0) {
            dest[s_ii].keyRoot = targetKey; dest[s_ii].scaleType = targetScale; dest[s_ii].chordDegree = 1; dest[s_ii].voicingMode = 4;
            dest[s_V].keyRoot = targetKey; dest[s_V].scaleType = 0; dest[s_V].chordDegree = 4; dest[s_V].voicingMode = 4;
        }
        else if (method == DeceptiveCadence && b2 >= 0) {
            dest[s_ii].keyRoot = targetKey; dest[s_ii].scaleType = targetScale; dest[s_ii].chordDegree = 1; dest[s_ii].voicingMode = 4;
            dest[s_V].keyRoot = targetKey; dest[s_V].scaleType = 0; dest[s_V].chordDegree = 4; dest[s_V].voicingMode = 4;
            dest[targetStepStart].chordDegree = 5;
        }
        else if (method == TritoneSub && b2 >= 0) {
            dest[s_ii].keyRoot = (targetKey + 1) % 12; dest[s_ii].scaleType = 7; dest[s_ii].chordDegree = 0; dest[s_ii].voicingMode = 4;
        }
        else if (method == MinorTwoFive && b2 >= 0) {
            dest[s_ii].keyRoot = targetKey; dest[s_ii].scaleType = 1; dest[s_ii].chordDegree = 1; dest[s_ii].voicingMode = 0;
            dest[s_V].keyRoot = targetKey; dest[s_V].scaleType = 1; dest[s_V].chordDegree = 4; dest[s_V].voicingMode = 0;
            dest[s_V].voices[1].accidental = 1;
        }
        else if (method == Backdoor && b2 >= 0) {
            dest[s_ii].keyRoot = targetKey; dest[s_ii].scaleType = 1; dest[s_ii].chordDegree = 3; dest[s_ii].voicingMode = 4;
            dest[s_V].keyRoot = targetKey; dest[s_V].scaleType = 1; dest[s_V].chordDegree = 6; dest[s_V].voicingMode = 4;
        }
        else if (method == ExtendedDominant && b2 >= 0) {
            dest[s_ii].keyRoot = (targetKey + 2) % 12; dest[s_ii].scaleType = 0; dest[s_ii].chordDegree = 4; dest[s_ii].voicingMode = 4;
            dest[s_V].keyRoot = (targetKey + 7) % 12; dest[s_V].scaleType = 0; dest[s_V].chordDegree = 4; dest[s_V].voicingMode = 4;
        }
        else if (method == PassingDiminished && b2 >= 0) {
            dest[s_ii].keyRoot = targetKey; dest[s_ii].scaleType = targetScale; dest[s_ii].chordDegree = 1; dest[s_ii].voicingMode = 0;
            dest[s_V].keyRoot = (targetKey + 11) % 12; dest[s_V].scaleType = 0; dest[s_V].chordDegree = 0; dest[s_V].voicingMode = 0;
            dest[s_V].voices[1].accidental = -1; dest[s_V].voices[2].accidental = -1; dest[s_V].voices[3].accidental = -2;
        }
        else if (method == ChromaticApproachUp && b2 >= 0) {
            dest[s_ii].keyRoot = (targetKey + 10) % 12; dest[s_ii].scaleType = targetScale; dest[s_ii].chordDegree = 0; dest[s_ii].voicingMode = 4;
            dest[s_V].keyRoot = (targetKey + 11) % 12; dest[s_V].scaleType = targetScale; dest[s_V].chordDegree = 0; dest[s_V].voicingMode = 4;
        }
        else if (method == ChromaticApproachDown && b2 >= 0) {
            dest[s_ii].keyRoot = (targetKey + 2) % 12; dest[s_ii].scaleType = targetScale; dest[s_ii].chordDegree = 0; dest[s_ii].voicingMode = 4;
            dest[s_V].keyRoot = (targetKey + 1) % 12; dest[s_V].scaleType = targetScale; dest[s_V].chordDegree = 0; dest[s_V].voicingMode = 4;
        }
        else if (method == ConstantStructure && b2 >= 0) {
            dest[s_ii].keyRoot = (targetKey + 10) % 12; dest[s_ii].scaleType = 0; dest[s_ii].chordDegree = 0; dest[s_ii].voicingMode = 4;
            dest[s_V].keyRoot = (targetKey + 11) % 12; dest[s_V].scaleType = 0; dest[s_V].chordDegree = 0; dest[s_V].voicingMode = 4;
        }
        else if (method == PedalPointApproach && b2 >= 0) {
            dest[s_ii].keyRoot = targetKey; dest[s_ii].scaleType = targetScale; dest[s_ii].chordDegree = 3; dest[s_ii].voicingMode = 4;
            dest[s_V].keyRoot = targetKey; dest[s_V].scaleType = 0; dest[s_V].chordDegree = 4; dest[s_V].voicingMode = 4;
        }
        else {
            dest[s_ii].keyRoot = targetKey; dest[s_ii].scaleType = 0; dest[s_ii].chordDegree = 4; dest[s_ii].voicingMode = 4;
            dest[s_ii].gateLength = chordGate * 2.0f;
        }

        if (b2 >= 0) {
            for (int v = 0; v < 4; ++v) {
                dest[s_ii].voices[v].isActive = true;
                if (method != DirectDominant) {
                    dest[s_V].voices[v].isActive = true;
                }
            }
            if (method == DirectDominant) {
                dest[s_ii].gateLength = chordGate * 2.0f;
            }
            else {
                dest[s_ii].gateLength = chordGate;
                dest[s_V].gateLength = chordGate;
            }
        }
    }

    const std::vector<ProgressionPreset>& ProgressionEngine::getProgressionDictionary() {
        static std::vector<ProgressionPreset> dict;
        if (dict.empty()) {

            // 1. Pop Basics (13パターンの完全網羅)
            dict.push_back({ "1. Pop Basics", "I - IV (Plagal)", 2, {{0,4,0,0,0,0,0,0}, {4,4,3,0,0,0,0,0}}, "Iadd9 - IVM9" });
            dict.push_back({ "1. Pop Basics", "I - V (Authentic)", 2, {{0,4,0,0,0,0,0,0}, {4,4,4,0,0,0,0,0}}, "Iadd9 - V11" });
            dict.push_back({ "1. Pop Basics", "I - IV - V (Blues/Rock Base)", 4, {{0,4,0,0,0,0,0,0}, {4,4,3,0,0,0,0,0}, {8,8,4,0,0,0,0,0}}, "I - IV - V" });
            dict.push_back({ "1. Pop Basics", "The Pop Progression (I-V-vi-IV)", 4, {{0,4,0,0,0,0,0,0}, {4,4,4,0,0,0,0,0}, {8,4,5,0,0,0,0,0}, {12,4,3,0,0,0,0,0}}, "I - V - vim - IV" });
            dict.push_back({ "1. Pop Basics", "Pop Chorus (I-IV-vi-V)", 4, {{0,4,0,0,0,0,0,0}, {4,4,3,0,0,0,0,0}, {8,4,5,0,0,0,0,0}, {12,4,4,0,0,0,0,0}}, "I - IV - vim - V" });
            dict.push_back({ "1. Pop Basics", "Minor Rotation (vi-IV-I-V)", 4, {{0,4,5,0,0,0,0,0}, {4,4,3,0,0,0,0,0}, {8,4,0,0,0,0,0,0}, {12,4,4,0,0,0,0,0}}, "vim - IV - I - V" });
            dict.push_back({ "1. Pop Basics", "Doo-Wop (I-vi-IV-V)", 4, {{0,4,0,0,0,0,0,0}, {4,4,5,0,0,0,0,0}, {8,4,3,0,0,0,0,0}, {12,4,4,0,0,0,0,0}}, "I - vim - IV - V" });
            dict.push_back({ "1. Pop Basics", "I - vi - ii - V", 4, {{0,4,0,0,0,0,0,0}, {4,4,5,0,0,0,0,0}, {8,4,1,0,0,0,0,0}, {12,4,4,0,0,0,0,0}}, "I - vim - iim - V" });
            dict.push_back({ "1. Pop Basics", "I - IV - ii - V", 4, {{0,4,0,0,0,0,0,0}, {4,4,3,0,0,0,0,0}, {8,4,1,0,0,0,0,0}, {12,4,4,0,0,0,0,0}}, "I - IV - iim - V" });
            dict.push_back({ "1. Pop Basics", "I - iii - IV - V", 4, {{0,4,0,0,0,0,0,0}, {4,4,2,0,0,0,0,0}, {8,4,3,0,0,0,0,0}, {12,4,4,0,0,0,0,0}}, "I - iiim - IV - V" });
            dict.push_back({ "1. Pop Basics", "I - iii - vi - IV", 4, {{0,4,0,0,0,0,0,0}, {4,4,2,0,0,0,0,0}, {8,4,5,0,0,0,0,0}, {12,4,3,0,0,0,0,0}}, "I - iiim - vim - IV" });
            dict.push_back({ "1. Pop Basics", "Rock Mixolydian (I-bIII-bVII-IV)", 4, {{0,4,0,0,0,0,0,0}, {4,4,2,0,-1,0,-1,-1}, {8,4,6,0,-1,0,-1,-1}, {12,4,3,0,0,0,0,0}}, "I - bIII - bVII - IV" });
            dict.push_back({ "1. Pop Basics", "Stadium Rock (I-V-bVII-IV)", 4, {{0,4,0,0,0,0,0,0}, {4,4,4,0,0,0,0,0}, {8,4,6,0,-1,0,-1,-1}, {12,4,3,0,0,0,0,0}}, "I - V - bVII - IV" });

            // 2. J-Pop / Royal Road Variations (8パターンの完全網羅)
            dict.push_back({ "2. J-Pop / Royal Road", "Royal Road Basic (IV-V-iii-vi)", 4, {{0,4,3,4,0,0,0,0}, {4,4,4,4,0,0,0,0}, {8,4,2,4,0,0,0,0}, {12,4,5,4,0,0,0,0}}, "IVmaj7 - V7 - iiim7 - vim7" });
            dict.push_back({ "2. J-Pop / Royal Road", "Royal Road with Sec Dom (III7)", 4, {{0,4,3,4,0,0,0,0}, {4,4,4,4,0,0,0,0}, {8,4,2,4,0,1,0,0}, {12,4,5,4,0,0,0,0}}, "IVmaj7 - V7 - III7 - vim7" });
            dict.push_back({ "2. J-Pop / Royal Road", "Royal Road with Passing Dim", 4, {{0,4,3,4,0,0,0,0}, {4,4,4,4,0,0,0,0}, {8,4,4,0,1,-1,-1,-2}, {12,4,5,4,0,0,0,0}}, "IVmaj7 - V7 - #Vdim7 - vim7" });
            dict.push_back({ "2. J-Pop / Royal Road", "Royal Road to ii-V-I", 8, {{0,4,3,4,0,0,0,0}, {4,4,4,4,0,0,0,0}, {8,4,2,4,0,0,0,0}, {12,4,5,4,0,0,0,0}, {16,4,1,4,0,0,0,0}, {20,4,4,4,0,0,0,0}, {24,8,0,4,0,0,0,0}}, "IV-V-iii-vi-ii-V-I" });
            dict.push_back({ "2. J-Pop / Royal Road", "Royal Road Bass Descending", 4, {{0,4,3,4,0,0,0,0}, {4,4,4,0,-2,0,0,0}, {8,4,2,4,0,0,0,0}, {12,4,5,4,0,0,0,0}}, "IVmaj7 - V/IV - iiim7 - vim7" });
            dict.push_back({ "2. J-Pop / Royal Road", "Minor Perspective (bVI-bVII-v-i)", 4, {{0,4,5,4,-1,0,-1,-1}, {4,4,6,4,-1,0,-1,-1}, {8,4,4,4,0,-1,0,-1}, {12,4,0,4,0,-1,0,-1}}, "bVImaj7 - bVII7 - vm7 - im7" });
            dict.push_back({ "2. J-Pop / Royal Road", "TK Progression (vi-IV-V-I)", 4, {{0,4,5,4,0,0,0,0}, {4,4,3,4,0,0,0,0}, {8,4,4,4,0,0,0,0}, {12,4,0,4,0,0,0,0}}, "vim7 - IVmaj7 - V7 - Imaj7" });
            dict.push_back({ "2. J-Pop / Royal Road", "Anime Standard (vi-V-IV-V)", 4, {{0,4,5,4,0,0,0,0}, {4,4,4,4,0,0,0,0}, {8,4,3,4,0,0,0,0}, {12,4,4,4,0,0,0,0}}, "vim7 - V7 - IVmaj7 - V7" });

            // 3. Marunouchi / JTTOU Variations (6パターンの完全網羅)
            dict.push_back({ "3. Marunouchi / JTTOU", "Basic (IV-III-vi-I)", 4, {{0,4,3,4,0,0,0,0}, {4,4,2,4,0,0,0,0}, {8,4,5,4,0,0,0,0}, {12,4,0,4,0,0,0,0}}, "IVM7 - iiim7 - vim7 - IM7" });
            dict.push_back({ "3. Marunouchi / JTTOU", "With Sec Dom (IV-III7-vi-I)", 4, {{0,4,3,4,0,0,0,0}, {4,4,2,4,0,1,0,0}, {8,4,5,4,0,0,0,0}, {12,4,0,4,0,0,0,0}}, "IVM7 - III7 - vim7 - IM7" });
            dict.push_back({ "3. Marunouchi / JTTOU", "Chromatic Descending (IV-III7-bIIIdim-ii)", 4, {{0,4,3,4,0,0,0,0}, {4,4,2,4,0,1,0,0}, {8,4,2,0,-1,-1,-2,-3}, {12,4,1,4,0,0,0,0}}, "IVM7 - III7 - bIIIdim7 - iim7" });
            dict.push_back({ "3. Marunouchi / JTTOU", "J-Pop Modernized (IV-III7-vi-v-I7)", 4, {{0,4,3,4,0,0,0,0}, {4,4,2,4,0,1,0,0}, {8,2,5,4,0,0,0,0}, {10,2,4,4,0,-1,0,-1}, {12,4,0,4,0,0,0,-1}}, "IVM7 - III7 - vim7 - vm7 - I7" });
            dict.push_back({ "3. Marunouchi / JTTOU", "Minor Perspective (bVI-V7-i-II7)", 4, {{0,4,5,4,-1,0,-1,-1}, {4,4,4,4,0,1,0,0}, {8,4,0,4,0,-1,0,-1}, {12,4,1,4,0,1,0,0}}, "bVIM7 - V7 - im7 - II7" });
            dict.push_back({ "3. Marunouchi / JTTOU", "Parallel Shift (vi-iii-vi-ii-I-iii)", 8, {{0,4,5,4,0,0,0,0}, {4,4,2,4,0,0,0,0}, {8,4,5,4,0,0,0,0}, {12,4,1,4,0,0,0,0}, {16,4,0,4,0,0,0,0}, {20,4,2,4,0,0,0,0}, {24,8,5,4,0,0,0,0}}, "vim - iiim - vim - iim - I - iiim" });

            // 4. Emotional / Modals (7パターンの完全網羅)
            dict.push_back({ "4. Emotional / Modals", "Subdominant Minor (IV-iv-I)", 4, {{0,4,3,4,0,0,0,0}, {4,4,3,4,0,-1,0,-1}, {8,8,0,4,0,0,0,0}}, "IVmaj7 - ivm7 - Imaj7" });
            dict.push_back({ "4. Emotional / Modals", "Subdominant Minor 6th (IV-iv6-I)", 4, {{0,4,3,4,0,0,0,0}, {4,4,3,0,0,-1,0,-2}, {8,8,0,4,0,0,0,0}}, "IVmaj7 - iv6 - Imaj7" });
            dict.push_back({ "4. Emotional / Modals", "Descending 4-3-2-1", 4, {{0,4,3,4,0,0,0,0}, {4,4,2,4,0,0,0,0}, {8,4,1,4,0,0,0,0}, {12,4,0,4,0,0,0,0}}, "IVmaj7 - iiim7 - iim7 - Imaj7" });
            dict.push_back({ "4. Emotional / Modals", "Mario Cadence (bVI-bVII-I)", 4, {{0,4,5,4,-1,0,-1,-1}, {4,4,6,4,-1,0,-1,-1}, {8,8,0,4,0,0,0,0}}, "bVImaj7 - bVIImaj7 - Imaj7" });
            dict.push_back({ "4. Emotional / Modals", "Pedal Point (I/III - IV - V/IV)", 4, {{0,4,0,0,4,0,0,0}, {4,4,3,4,0,0,0,0}, {8,8,4,0,-2,0,0,0}}, "I/III - IVM7 - V/IV" });
            dict.push_back({ "4. Emotional / Modals", "Line Cliche Minor (vi-viM7-vi7-vi6)", 4, {{0,4,5,0,0,0,0,-128}, {4,4,5,0,0,0,0,1}, {8,4,5,0,0,0,0,0}, {12,4,5,0,0,0,0,-1}}, "vim - vim(M7) - vim7 - vim6" });
            dict.push_back({ "4. Emotional / Modals", "Line Cliche Major (I-Iaug-I6-I7)", 4, {{0,4,0,0,0,0,0,-128}, {4,4,0,0,0,0,1,-128}, {8,4,0,0,0,0,2,-128}, {12,4,0,0,0,0,0,-1}}, "I - Iaug - I6 - I7" });

            // 5. Jazz / Blues (10パターンの完全網羅)
            dict.push_back({ "5. Jazz / Blues", "Major 2-5-1", 4, {{0,4,1,4,0,0,0,0}, {4,4,4,4,0,0,0,0}, {8,8,0,4,0,0,0,0}}, "iim9 - V13 - Imaj9" });
            dict.push_back({ "5. Jazz / Blues", "Minor 2-5-1 (Half Dim to Alt)", 4, {{0,4,1,0,0,0,-1,0}, {4,4,4,0,0,1,0,0}, {8,8,5,4,0,0,0,0}}, "iim7b5 - V7alt - vim9" });
            dict.push_back({ "5. Jazz / Blues", "Rhythm Changes Base", 8, {{0,2,0,4,0,0,0,0}, {2,2,5,4,0,1,0,0}, {4,2,1,4,0,1,0,0}, {6,2,4,4,0,0,0,0}, {8,2,2,4,0,0,0,0}, {10,2,5,4,0,1,0,0}, {12,2,1,4,0,0,0,0}, {14,2,4,4,0,0,0,0}, {16,8,0,4,0,0,0,0}, {24,8,0,4,0,0,0,0}}, "I-VI7-II7-V7-iii-VI7-ii-V7" });
            dict.push_back({ "5. Jazz / Blues", "Autumn Leaves (Circle of 5ths)", 8, {{0,4,1,4,0,0,0,0}, {4,4,4,4,0,0,0,0}, {8,4,0,4,0,0,0,0}, {12,4,3,4,0,0,0,0}, {16,4,6,0,0,0,-1,0}, {20,4,2,4,0,1,0,0}, {24,8,5,4,0,0,0,0}}, "iim7-V7-Imaj7-IVmaj7-viim7b5-III7-vim7" });
            dict.push_back({ "5. Jazz / Blues", "Dominant Chain (III7-VI7-II7-V7)", 4, {{0,4,2,4,0,1,0,0}, {4,4,5,4,0,1,0,0}, {8,4,1,4,0,1,0,0}, {12,4,4,4,0,0,0,0}}, "III7 - VI7 - II7 - V7" });
            dict.push_back({ "5. Jazz / Blues", "Altered Dominant (ii-V7#5-I)", 4, {{0,4,1,4,0,0,0,0}, {4,4,4,4,0,0,1,0}, {8,8,0,4,0,0,0,0}}, "iim7 - V7(#5) - Imaj7" });
            dict.push_back({ "5. Jazz / Blues", "Coltrane Giant Steps Matrix", 8, {{0,4,0,0,0,0,0,0}, {4,4,2,0,-1,0,-1,-1}, {8,4,5,0,-1,0,-1,-1}, {12,4,6,0,-1,0,-1,-1}, {16,4,2,0,0,1,0,0}, {20,4,4,0,0,0,0,0}, {24,8,0,0,0,0,0,0}}, "I - bIII7 - bVI - VII7 - III - V7 - I" });
            dict.push_back({ "5. Jazz / Blues", "Charlie Parker Blues (Bird Changes)", 12, {
                {0,4,0,4,0,0,0,-1}, {4,4,6,0,0,0,-1,0}, {8,4,2,4,0,1,0,0}, {12,4,5,4,0,1,0,0}, {16,4,1,4,0,1,0,0}, {20,4,4,4,0,0,0,0},
                {24,4,0,4,0,0,0,-1}, {28,4,5,4,0,1,0,0}, {32,4,1,4,0,0,0,0}, {36,4,4,4,0,0,0,0}, {40,2,0,4,0,0,0,0}, {42,2,5,4,0,1,0,0}, {44,2,1,4,0,0,0,0}, {46,2,4,4,0,0,0,0}
            }, "I7 - viim7b5 - III7 - VI7 - II7 - V7 - I7 - VI7 - iim7 - V7 - I-VI-ii-V" });
            dict.push_back({ "5. Jazz / Blues", "Andalusian Cadence", 4, {{0,4,5,3,0,0,0,-128}, {4,4,4,3,0,0,0,-128}, {8,4,3,3,0,0,0,-128}, {12,4,2,3,0,1,0,-128}}, "vim - V - IV - III" });
            dict.push_back({ "5. Jazz / Blues", "Passing Diminished Ascent (I-#Idim-ii-V)", 4, {{0,4,0,4,0,0,0,0}, {4,4,0,0,1,-1,-1,-2}, {8,4,1,4,0,0,0,0}, {12,4,4,4,0,0,0,0}}, "IM7 - #Idim7 - iim7 - V7" });

            // 6. Advanced Theory (12パターンの完全網羅)
            dict.push_back({ "6. Advanced Theory", "IVmaj7 as ii Sub (IV-V-I)", 4, {{0,4,3,4,0,0,0,0}, {4,4,4,4,0,0,0,0}, {8,8,0,4,0,0,0,0}}, "IVM7 - V7 - IM7" });
            dict.push_back({ "6. Advanced Theory", "Related ii-V to vi (viim7b5-III7-vi)", 4, {{0,4,6,0,0,0,-1,0}, {4,4,2,4,0,1,0,0}, {8,8,5,4,0,0,0,0}}, "viim7b5 - III7 - vim7" });
            dict.push_back({ "6. Advanced Theory", "Classic Deceptive (V7 -> vi)", 4, {{0,4,1,4,0,0,0,0}, {4,4,4,4,0,0,0,0}, {8,8,5,4,0,0,0,0}}, "iim7 - V7 - vim7" });
            dict.push_back({ "6. Advanced Theory", "Tonic Sub Deceptive (V7 -> iii)", 4, {{0,4,1,4,0,0,0,0}, {4,4,4,4,0,0,0,0}, {8,8,2,4,0,0,0,0}}, "iim7 - V7 - iiim7" });
            dict.push_back({ "6. Advanced Theory", "Modal Deceptive (V7 -> bVIM7)", 4, {{0,4,1,4,0,0,0,0}, {4,4,4,4,0,0,0,0}, {8,8,5,4,-1,0,-1,-1}}, "iim7 - V7 - bVImaj7" });
            dict.push_back({ "6. Advanced Theory", "Neapolitan Deceptive (V7 -> bIIM7)", 4, {{0,4,1,4,0,0,0,0}, {4,4,4,4,0,0,0,0}, {8,8,1,4,-1,0,-1,-1}}, "iim7 - V7 - Dbmaj7" });
            dict.push_back({ "6. Advanced Theory", "Stella Deceptive (#ivm7b5 - VII7 - iiim)", 4, {{0,4,3,0,1,0,0,0}, {4,4,6,4,0,1,0,0}, {8,8,2,4,0,0,0,0}}, "#ivm7b5 - VII7 - iiim7" });
            dict.push_back({ "6. Advanced Theory", "Minor Tonic 6th (Im6)", 4, {{0,4,1,0,0,0,-1,0}, {4,4,4,0,0,1,0,0}, {8,8,0,0,0,0,0,-2}}, "iim7b5 - V7alt - Im6" });
            dict.push_back({ "6. Advanced Theory", "Minor Tonic maj7 (Im(maj7))", 4, {{0,4,1,0,0,0,-1,0}, {4,4,4,0,0,1,0,0}, {8,8,0,0,0,0,0,-1}}, "iim7b5 - V7alt - Im(maj7)" });
            dict.push_back({ "6. Advanced Theory", "Minor Tonic 9th (Im9)", 4, {{0,4,1,0,0,0,-1,0}, {4,4,4,0,0,1,0,0}, {8,8,0,4,0,0,0,0}}, "iim7b5 - V7alt - Im9" });
            dict.push_back({ "6. Advanced Theory", "Altered Sec Dom (V7#5/vi)", 4, {{0,4,2,0,0,0,-1,0}, {4,4,2,0,0,1,1,0}, {8,8,5,4,0,0,0,0}}, "iiim7b5 - III7(#5) - vim9" });
            dict.push_back({ "6. Advanced Theory", "Harmonic Major 2-5-1", 4, {{0,4,1,4,0,0,-1,0}, {4,4,4,4,0,0,0,0}, {8,8,0,4,0,0,0,0}}, "iim7b5 - V7(b9,13) - Imaj7" });
        }
        return dict;
    }
}
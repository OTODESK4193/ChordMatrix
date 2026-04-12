#include "ProgressionEngine.h"
#include <cstdint>

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

    std::vector<ChordSuggestion> ProgressionEngine::suggestNextChords(const std::array<StepData, TotalSteps>& seq, int currentStep, float ppqPerStep) {
        std::vector<ChordSuggestion> suggestions;

        int prevStep = -1;
        for (int s = currentStep; s >= 0; --s) {
            bool hasActive = false;
            for (int v = 0; v < 7; ++v) {
                if (seq[s].voices[v].isActive) { hasActive = true; break; }
            }
            if (hasActive) { prevStep = s; break; }
        }

        int nextStep = -1;
        for (int s = currentStep + 1; s < TotalSteps; ++s) {
            bool hasActive = false;
            for (int v = 0; v < 7; ++v) {
                if (seq[s].voices[v].isActive) { hasActive = true; break; }
            }
            if (hasActive) { nextStep = s; break; }
        }

        int prevDeg = (prevStep >= 0) ? seq[prevStep].chordDegree : 0;
        int prevScale = (prevStep >= 0) ? seq[prevStep].scaleType : 0;
        bool isPrevMinor = (prevScale == 1 || prevScale == 2 || prevScale == 3);

        int nextDeg = (nextStep >= 0) ? seq[nextStep].chordDegree : -1;
        int nextScale = (nextStep >= 0) ? seq[nextStep].scaleType : -1;

        if (nextStep >= 0 && (nextStep - currentStep) * 0.25f <= 2.0f) {
            if (nextDeg == 0) {
                suggestions.push_back({ 4, nextScale, 0, 0.9f, "Target: I (V7 -> I)" });
                suggestions.push_back({ 1, nextScale, 0, 0.7f, "Target: I (ii7 -> I)" });
                suggestions.push_back({ 0, 10, 0, 0.6f, "Target: I (Lyd.Dom SubV)" });
            }
            else if (nextDeg == 5) {
                suggestions.push_back({ 2, 14, 0, 0.8f, "Target: vi (III7 -> vi)" });
                suggestions.push_back({ 6, nextScale, 0, 0.6f, "Target: vi (vii -> vi)" });
            }
            else if (nextDeg == 3) {
                suggestions.push_back({ 0, 7, 0, 0.8f, "Target: IV (I7 -> IV)" });
            }
            else {
                int domDeg = (nextDeg + 3) % 7;
                suggestions.push_back({ domDeg, nextScale, 0, 0.7f, "Sec. Dominant Approach" });
                suggestions.push_back({ (nextDeg + 6) % 7, nextScale, 0, 0.6f, "Chromatic Down Approach" });
            }
        }
        else if (prevStep >= 0) {
            if (prevDeg == 4) {
                suggestions.push_back({ 0, prevScale, 0, 0.80f, "Auth. Cadence (V -> I)" });
                suggestions.push_back({ 5, prevScale, 0, 0.25f, "Deceptive (V -> vi)" });
                suggestions.push_back({ 0, 13, 0, 0.35f, "Jazz Resol. (V7alt -> I)" });
                suggestions.push_back({ 0, 29, 0, 0.15f, "Diminished (HW) Approach" });
            }
            else if (prevDeg == 1) {
                suggestions.push_back({ 4, prevScale, 0, 0.75f, "Standard Motion (ii -> V)" });
                suggestions.push_back({ 0, 7, 1, 0.15f, "Tritone Sub (ii -> bII7)" });
            }
            else if (prevDeg == 3) {
                suggestions.push_back({ 4, prevScale, 0, 0.45f, "Functional (IV -> V)" });
                suggestions.push_back({ 0, prevScale, 0, 0.35f, "Plagal (IV -> I)" });
                if (!isPrevMinor) {
                    suggestions.push_back({ 2, 14, 0, 0.30f, "J-POP Emotional (IV -> III7)" });
                }
            }
            else if (prevDeg == 0) {
                suggestions.push_back({ 3, prevScale, 0, 0.35f, "Tonic -> Subdom (IV)" });
                suggestions.push_back({ 5, prevScale, 0, 0.25f, "Tonic -> Rel Minor (vi)" });
                suggestions.push_back({ 1, prevScale, 0, 0.20f, "Tonic -> Supertonic (ii)" });
            }
            else if (prevDeg == 5) {
                suggestions.push_back({ 1, prevScale, 0, 0.40f, "Cycle of 5ths (vi -> ii)" });
                suggestions.push_back({ 3, prevScale, 0, 0.30f, "Approach (vi -> IV)" });
            }
            else if (prevDeg == 2) {
                suggestions.push_back({ 5, prevScale, 0, 0.60f, "Cycle of 5ths (iii -> vi)" });
                suggestions.push_back({ 5, 7, 0, 0.20f, "Modal Shift (iii -> vim)" });
            }
            else {
                suggestions.push_back({ 0, prevScale, 0, 0.50f, "Resolve to Tonic" });
                suggestions.push_back({ (prevDeg + 3) % 7, prevScale, 0, 0.40f, "Cycle of 5ths fall" });
            }
        }
        else {
            int currentScale = seq[currentStep].scaleType;
            suggestions.push_back({ 0, currentScale, 0, 0.9f, "Start with Tonic (I)" });
            suggestions.push_back({ 3, currentScale, 0, 0.6f, "Start with Subdominant (IV)" });
            suggestions.push_back({ 5, currentScale, 0, 0.5f, "Start with Relative Minor (vi)" });
            suggestions.push_back({ 1, currentScale, 0, 0.4f, "Start with Supertonic (ii)" });
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

            // =========================================================
            // 1. US: Trap & Neo-Soul (トラップのミニマリズムとR&Bの情動性)
            // =========================================================
            dict.push_back({ "1. US: Trap & Neo-Soul", "Modern Trap Soul", 4, {{0,8,0,9,0,0,0,0}, {8,4,3,9,0,0,0,0}, {12,4,6,9,0,0,0,0}}, "i - iv - VII (Shell)" });
            dict.push_back({ "1. US: Trap & Neo-Soul", "Neo R&B Loop", 4, {{0,4,1,4,0,0,0,0}, {4,4,4,4,0,0,0,0}, {8,4,0,4,0,0,0,0}, {12,4,5,4,0,0,0,0}}, "ii - V - I - vi (Rootless)" });
            dict.push_back({ "1. US: Trap & Neo-Soul", "Cinematic Trap", 4, {{0,4,0,13,0,0,0,0}, {4,4,5,13,-1,0,0,0}, {8,4,2,13,-1,0,0,0}, {12,4,6,13,-1,0,0,0}}, "i - bVI - bIII - bVII (Cluster)" });
            dict.push_back({ "1. US: Trap & Neo-Soul", "Late Night Vibe", 2, {{0,4,0,8,0,0,0,0}, {4,4,6,8,-1,-1,-1,-1}}, "i - bVII (Quartal)" });
            dict.push_back({ "1. US: Trap & Neo-Soul", "Axis of Pop", 4, {{0,4,0,1,0,0,0,0}, {4,4,4,1,0,0,0,0}, {8,4,5,1,0,0,0,0}, {12,4,3,1,0,0,0,0}}, "I - V - vi - IV (Drop 2)" });
            dict.push_back({ "1. US: Trap & Neo-Soul", "Emotional Twist", 4, {{0,4,5,1,0,0,0,0}, {4,4,3,1,0,0,0,0}, {8,4,0,1,0,0,0,0}, {12,4,4,1,0,0,0,0}}, "vi - IV - I - V (Drop 2)" });
            dict.push_back({ "1. US: Trap & Neo-Soul", "Cycle of 4ths", 8, {{0,4,0,4,0,0,0,0}, {4,4,3,4,0,0,0,0}, {8,4,6,4,0,0,0,0}, {12,4,2,4,0,0,0,0}, {16,4,5,4,0,0,0,0}, {20,4,1,4,0,0,0,0}, {24,4,4,4,0,0,0,0}, {28,4,0,4,0,0,0,0}}, "I - IV - viidim - iii - vi - ii - V - I" });
            dict.push_back({ "1. US: Trap & Neo-Soul", "Nineties Slow Jam", 4, {{0,4,3,4,0,0,0,0}, {4,4,4,4,0,0,0,0}, {8,4,2,4,0,0,0,0}, {12,4,5,4,0,0,0,0}}, "IV - V7sus - iii - vi (Rootless)" });
            dict.push_back({ "1. US: Trap & Neo-Soul", "Trap Diminished Approach", 4, {{0,4,0,13,0,0,0,0}, {4,4,0,13,1,0,0,0}, {8,4,1,13,0,0,0,0}, {12,4,4,13,0,0,0,0}}, "I - #Idim7 - ii - V (Cluster)" });
            dict.push_back({ "1. US: Trap & Neo-Soul", "Minor 9 Vamp", 4, {{0,4,0,4,0,0,0,0}, {4,4,1,4,0,0,0,0}, {8,4,4,4,0,0,0,0}, {12,4,3,4,0,0,0,0}}, "i - ii - v - IV (Rootless)" });

            // =========================================================
            // 2. UK: Garage & Indie Pop (ダンスの熱狂とインディーの哀愁)
            // =========================================================
            dict.push_back({ "2. UK: Garage & Indie Pop", "2-Step Garage Stab", 4, {{0,4,0,8,0,0,0,0}, {4,4,3,8,0,0,0,0}, {8,4,6,8,0,0,0,0}, {12,4,2,8,0,0,0,0}}, "i - iv - bVII - bIII (Quartal)" });
            dict.push_back({ "2. UK: Garage & Indie Pop", "Piano House Anthem", 4, {{0,4,0,9,0,0,0,0}, {4,4,4,9,0,0,0,0}, {8,4,5,9,0,0,0,0}, {12,4,3,9,0,0,0,0}}, "I - V - vi - IV (Shell)" });
            dict.push_back({ "2. UK: Garage & Indie Pop", "Hypnotic Techno", 4, {{0,4,0,13,0,0,0,0}, {4,4,6,13,-1,-1,-1,-1}, {8,4,5,13,-1,-1,-1,-1}, {12,4,6,13,-1,-1,-1,-1}}, "i - bVII - bVI - bVII (Cluster)" });
            dict.push_back({ "2. UK: Garage & Indie Pop", "Dreamy Indie Lydian", 4, {{0,4,0,1,0,0,0,0}, {4,4,1,1,0,1,0,0}, {8,4,3,1,0,0,0,0}, {12,4,0,1,0,0,0,0}}, "I - II - IV - I (Drop 2)" });
            dict.push_back({ "2. UK: Garage & Indie Pop", "Shoegaze Texture", 4, {{0,4,0,1,0,0,0,0}, {4,4,5,1,0,0,0,0}, {8,4,3,1,0,0,0,0}, {12,4,4,1,0,0,0,0}}, "I - vi - IV - V (Drop 2)" });
            dict.push_back({ "2. UK: Garage & Indie Pop", "UKG Parallel", 2, {{0,4,0,8,0,0,0,0}, {4,4,6,8,-2,-2,-2,-2}}, "i - bvii (Quartal)" });
            dict.push_back({ "2. UK: Garage & Indie Pop", "Indie Folk Cascade", 4, {{0,4,0,1,0,0,0,0}, {4,4,4,1,0,0,0,0}, {8,4,5,1,0,0,0,0}, {12,4,2,1,0,0,0,0}}, "i - v - VI - III (Drop 2)" });
            dict.push_back({ "2. UK: Garage & Indie Pop", "Club Banger Pop", 4, {{0,4,0,9,0,0,0,0}, {4,4,3,9,0,0,0,0}, {8,4,5,9,0,0,0,0}, {12,4,4,9,0,0,0,0}}, "I - IV - vi - V (Shell)" });
            dict.push_back({ "2. UK: Garage & Indie Pop", "Moody Deep House", 4, {{0,4,0,4,0,0,0,0}, {4,4,6,4,-1,-1,-1,-1}, {8,4,5,4,-1,-1,-1,-1}, {12,4,6,4,-1,-1,-1,-1}}, "i - bVII - bVI - bVII (Rootless)" });
            dict.push_back({ "2. UK: Garage & Indie Pop", "Ambient Garage Stab", 4, {{0,8,0,12,0,0,0,0}, {8,4,3,12,0,0,0,0}, {12,4,6,12,-1,-1,-1,-1}}, "i - IV - bVII (So What)" });

            // =========================================================
            // 3. J-POP: Anime & Modern (王道進行と高密度ハーモニック・リズム)
            // =========================================================
            dict.push_back({ "3. J-POP: Anime & Modern", "Royal Road (Shell)", 4, {{0,4,3,9,0,0,0,0}, {4,4,4,9,0,0,0,0}, {8,4,2,9,0,0,0,0}, {12,4,5,9,0,0,0,0}}, "IV - V - iii - vi" });
            dict.push_back({ "3. J-POP: Anime & Modern", "Just the Two of Us (Extended)", 4, {{0,4,5,4,-1,-1,-1,-1}, {4,4,4,4,0,1,0,0}, {8,4,0,4,0,0,0,0}, {12,2,6,4,-1,-1,-1,-1}, {14,2,2,4,-1,-1,-1,-1}}, "bVI - V7 - i - bVII - bIII" });
            dict.push_back({ "3. J-POP: Anime & Modern", "City Pop Night", 8, {{0,8,3,4,0,0,0,0}, {8,4,2,4,0,0,0,0}, {12,4,5,4,0,0,0,0}, {16,8,1,4,0,0,0,0}, {24,4,4,4,0,0,0,0}, {28,4,0,4,0,0,0,0}}, "IV - iii - vi - ii - V - I" });
            dict.push_back({ "3. J-POP: Anime & Modern", "Emotional Fall", 4, {{0,4,5,1,0,0,0,0}, {4,4,2,1,0,0,0,0}, {8,4,3,1,0,0,0,0}, {12,4,0,1,0,0,0,0}}, "vi - iii - IV - I (Drop 2)" });
            dict.push_back({ "3. J-POP: Anime & Modern", "Sub-Dominant Start", 4, {{0,4,3,9,0,0,0,0}, {4,4,4,9,0,0,0,0}, {8,4,5,9,0,0,0,0}, {12,4,2,9,0,0,0,0}}, "IV - V - vi - iii (Shell)" });
            dict.push_back({ "3. J-POP: Anime & Modern", "Secondary J-Ballad", 4, {{0,4,0,4,0,0,0,0}, {4,4,2,4,0,1,0,0}, {8,4,5,4,0,0,0,0}, {12,4,3,4,0,0,0,0}}, "I - III7 - vi - IV" });
            dict.push_back({ "3. J-POP: Anime & Modern", "Modern J-Rock", 4, {{0,4,3,9,0,0,0,0}, {4,4,4,9,0,0,0,0}, {8,4,0,9,0,0,0,0}, {12,4,5,9,0,0,0,0}}, "IV - V - I - vi (Shell)" });
            dict.push_back({ "3. J-POP: Anime & Modern", "Chromatic J-Pop", 4, {{0,4,3,0,0,0,0,0}, {4,4,3,0,1,0,0,0}, {8,4,4,0,0,0,0,0}, {12,4,5,0,-1,-1,-1,-1}}, "IV - #IVdim - V - bVI" });
            dict.push_back({ "3. J-POP: Anime & Modern", "Jazz-Pop Fusion", 4, {{0,4,1,4,0,0,0,0}, {4,4,4,4,0,0,0,0}, {8,2,2,4,0,1,0,0}, {10,2,5,4,0,0,0,0}, {12,2,1,4,0,0,0,0}, {14,2,1,4,-1,-1,-1,-1}}, "ii - V - III - vi - ii - bII" });

            // =========================================================
            // 4. K-POP: Maximalism & Neo-Soul (コントラストと転調の美学)
            // =========================================================
            dict.push_back({ "4. K-POP: Maximalism & Neo-Soul", "Lush Neo-Soul", 4, {{0,4,0,4,0,0,0,0}, {4,4,5,4,-1,0,0,0}, {8,4,1,4,0,0,0,0}, {12,4,4,4,0,0,0,0}}, "i - bVI - ii - V" });
            dict.push_back({ "4. K-POP: Maximalism & Neo-Soul", "Dramatic Bridge", 4, {{0,4,5,8,-1,-1,-1,-1}, {4,4,6,8,-1,-1,-1,-1}, {8,4,0,8,0,0,0,0}, {12,4,4,8,0,0,0,0}}, "bVI - bVII - i - v (Quartal)" });
            dict.push_back({ "4. K-POP: Maximalism & Neo-Soul", "Sophisticated Funk", 4, {{0,8,0,12,0,0,0,0}, {8,8,3,12,0,0,0,0}}, "i - IV (So What)" });
            dict.push_back({ "4. K-POP: Maximalism & Neo-Soul", "Global Sync Loop", 2, {{0,4,0,13,0,0,0,0}, {4,4,5,13,-1,0,0,0}}, "i - bVI (Cluster)" });
            dict.push_back({ "4. K-POP: Maximalism & Neo-Soul", "Aesthetic Pad", 8, {{0,8,0,1,0,0,0,0}, {8,8,2,1,0,0,0,0}, {16,8,3,1,0,0,0,0}, {24,8,3,1,0,-1,0,0}}, "I - iii - IV - iv (Drop 2)" });
            dict.push_back({ "4. K-POP: Maximalism & Neo-Soul", "K-Pop Turnaround", 4, {{0,4,3,4,0,0,0,0}, {4,4,2,4,0,1,0,0}, {8,4,5,4,0,0,0,0}, {12,4,6,4,-1,-1,-1,-1}}, "IV - V/vi - vi - bVII" });
            dict.push_back({ "4. K-POP: Maximalism & Neo-Soul", "Dreamy Future R&B", 4, {{0,4,0,4,0,0,0,0}, {4,4,1,4,0,0,0,0}, {8,4,1,4,-1,-1,-1,-1}, {12,4,0,4,0,0,0,0}}, "I - ii - bII - I" });
            dict.push_back({ "4. K-POP: Maximalism & Neo-Soul", "Melancholy K-Ballad", 4, {{0,4,0,4,0,0,0,0}, {4,4,2,4,-1,0,0,0}, {8,4,5,4,-1,0,0,0}, {12,4,1,4,0,1,0,0}}, "i - bIII - bVI - II" });
            dict.push_back({ "4. K-POP: Maximalism & Neo-Soul", "High Energy Pop", 4, {{0,4,0,9,0,0,0,0}, {4,4,5,9,0,0,0,0}, {8,4,3,9,0,0,0,0}, {12,4,4,9,0,0,0,0}}, "I - vi - IV - V (Shell)" });
            dict.push_back({ "4. K-POP: Maximalism & Neo-Soul", "Experimental Bridge", 4, {{0,4,5,8,-1,-1,-1,-1}, {4,4,6,8,-1,-1,-1,-1}, {8,4,0,8,0,0,0,0}, {12,4,1,8,-1,-1,-1,-1}}, "bVI - bVII - i - bII (Quartal)" });
            dict.push_back({ "4. K-POP: Maximalism & Neo-Soul", "Neo-Vintage Soul", 4, {{0,4,0,12,0,0,0,0}, {4,4,1,12,-1,-1,-1,-1}, {8,4,6,12,-1,-1,-1,-1}, {12,4,0,12,0,0,0,0}}, "i - bII - bVII - i (So What)" });

            // =========================================================
            // 5. Pop & Rock Basics (レガシー保存領域)
            // =========================================================
            dict.push_back({ "5. Pop & Rock Basics", "I - IV (Plagal)", 2, {{0,4,0,0,0,0,0,0}, {4,4,3,0,0,0,0,0}}, "Iadd9 - IVM9" });
            dict.push_back({ "5. Pop & Rock Basics", "I - V (Authentic)", 2, {{0,4,0,0,0,0,0,0}, {4,4,4,0,0,0,0,0}}, "Iadd9 - V11" });
            dict.push_back({ "5. Pop & Rock Basics", "I - IV - V (Blues/Rock Base)", 4, {{0,4,0,0,0,0,0,0}, {4,4,3,0,0,0,0,0}, {8,8,4,0,0,0,0,0}}, "I - IV - V" });
            dict.push_back({ "5. Pop & Rock Basics", "The Pop Progression (I-V-vi-IV)", 4, {{0,4,0,0,0,0,0,0}, {4,4,4,0,0,0,0,0}, {8,4,5,0,0,0,0,0}, {12,4,3,0,0,0,0,0}}, "I - V - vim - IV" });
            dict.push_back({ "5. Pop & Rock Basics", "Pop Chorus (I-IV-vi-V)", 4, {{0,4,0,0,0,0,0,0}, {4,4,3,0,0,0,0,0}, {8,4,5,0,0,0,0,0}, {12,4,4,0,0,0,0,0}}, "I - IV - vim - V" });
            dict.push_back({ "5. Pop & Rock Basics", "Minor Rotation (vi-IV-I-V)", 4, {{0,4,5,0,0,0,0,0}, {4,4,3,0,0,0,0,0}, {8,4,0,0,0,0,0,0}, {12,4,4,0,0,0,0,0}}, "vim - IV - I - V" });
            dict.push_back({ "5. Pop & Rock Basics", "Doo-Wop (I-vi-IV-V)", 4, {{0,4,0,0,0,0,0,0}, {4,4,5,0,0,0,0,0}, {8,4,3,0,0,0,0,0}, {12,4,4,0,0,0,0,0}}, "I - vim - IV - V" });
            dict.push_back({ "5. Pop & Rock Basics", "I - vi - ii - V", 4, {{0,4,0,0,0,0,0,0}, {4,4,5,0,0,0,0,0}, {8,4,1,0,0,0,0,0}, {12,4,4,0,0,0,0,0}}, "I - vim - iim - V" });
            dict.push_back({ "5. Pop & Rock Basics", "I - IV - ii - V", 4, {{0,4,0,0,0,0,0,0}, {4,4,3,0,0,0,0,0}, {8,4,1,0,0,0,0,0}, {12,4,4,0,0,0,0,0}}, "I - IV - iim - V" });
            dict.push_back({ "5. Pop & Rock Basics", "I - iii - IV - V", 4, {{0,4,0,0,0,0,0,0}, {4,4,2,0,0,0,0,0}, {8,4,3,0,0,0,0,0}, {12,4,4,0,0,0,0,0}}, "I - iiim - IV - V" });
            dict.push_back({ "5. Pop & Rock Basics", "I - iii - vi - IV", 4, {{0,4,0,0,0,0,0,0}, {4,4,2,0,0,0,0,0}, {8,4,5,0,0,0,0,0}, {12,4,3,0,0,0,0,0}}, "I - iiim - vim - IV" });
            dict.push_back({ "5. Pop & Rock Basics", "Rock Mixolydian", 4, {{0,4,0,0,0,0,0,0}, {4,4,2,0,-1,0,-1,-1}, {8,4,6,0,-1,0,-1,-1}, {12,4,3,0,0,0,0,0}}, "I - bIII - bVII - IV" });
            dict.push_back({ "5. Pop & Rock Basics", "Stadium Rock", 4, {{0,4,0,0,0,0,0,0}, {4,4,4,0,0,0,0,0}, {8,4,6,0,-1,0,-1,-1}, {12,4,3,0,0,0,0,0}}, "I - V - bVII - IV" });

            // =========================================================
            // 6. J-POP: Classic Royal Road (レガシー保存領域)
            // =========================================================
            dict.push_back({ "6. J-POP: Classic Royal Road", "Royal Road Basic", 4, {{0,4,3,4,0,0,0,0}, {4,4,4,4,0,0,0,0}, {8,4,2,4,0,0,0,0}, {12,4,5,4,0,0,0,0}}, "IVmaj7 - V7 - iiim7 - vim7" });
            dict.push_back({ "6. J-POP: Classic Royal Road", "Royal Road with Sec Dom", 4, {{0,4,3,4,0,0,0,0}, {4,4,4,4,0,0,0,0}, {8,4,2,4,0,1,0,0}, {12,4,5,4,0,0,0,0}}, "IVmaj7 - V7 - III7 - vim7" });
            dict.push_back({ "6. J-POP: Classic Royal Road", "Royal Road with Passing Dim", 4, {{0,4,3,4,0,0,0,0}, {4,4,4,4,0,0,0,0}, {8,4,4,0,1,-1,-1,-2}, {12,4,5,4,0,0,0,0}}, "IVmaj7 - V7 - #Vdim7 - vim7" });
            dict.push_back({ "6. J-POP: Classic Royal Road", "Royal Road to ii-V-I", 8, {{0,4,3,4,0,0,0,0}, {4,4,4,4,0,0,0,0}, {8,4,2,4,0,0,0,0}, {12,4,5,4,0,0,0,0}, {16,4,1,4,0,0,0,0}, {20,4,4,4,0,0,0,0}, {24,8,0,4,0,0,0,0}}, "IV-V-iii-vi-ii-V-I" });
            dict.push_back({ "6. J-POP: Classic Royal Road", "Royal Road Bass Descending", 4, {{0,4,3,4,0,0,0,0}, {4,4,4,0,-2,0,0,0}, {8,4,2,4,0,0,0,0}, {12,4,5,4,0,0,0,0}}, "IVmaj7 - V/IV - iiim7 - vim7" });
            dict.push_back({ "6. J-POP: Classic Royal Road", "Minor Perspective", 4, {{0,4,5,4,-1,0,-1,-1}, {4,4,6,4,-1,0,-1,-1}, {8,4,4,4,0,-1,0,-1}, {12,4,0,4,0,-1,0,-1}}, "bVImaj7 - bVII7 - vm7 - im7" });
            dict.push_back({ "6. J-POP: Classic Royal Road", "TK Progression", 4, {{0,4,5,4,0,0,0,0}, {4,4,3,4,0,0,0,0}, {8,4,4,4,0,0,0,0}, {12,4,0,4,0,0,0,0}}, "vim7 - IVmaj7 - V7 - Imaj7" });
            dict.push_back({ "6. J-POP: Classic Royal Road", "Anime Standard", 4, {{0,4,5,4,0,0,0,0}, {4,4,4,4,0,0,0,0}, {8,4,3,4,0,0,0,0}, {12,4,4,4,0,0,0,0}}, "vim7 - V7 - IVmaj7 - V7" });

            // =========================================================
            // 7. J-POP: Classic Marunouchi (レガシー保存領域)
            // =========================================================
            dict.push_back({ "7. J-POP: Classic Marunouchi", "Basic (IV-III-vi-I)", 4, {{0,4,3,4,0,0,0,0}, {4,4,2,4,0,0,0,0}, {8,4,5,4,0,0,0,0}, {12,4,0,4,0,0,0,0}}, "IVM7 - iiim7 - vim7 - IM7" });
            dict.push_back({ "7. J-POP: Classic Marunouchi", "With Sec Dom", 4, {{0,4,3,4,0,0,0,0}, {4,4,2,4,0,1,0,0}, {8,4,5,4,0,0,0,0}, {12,4,0,4,0,0,0,0}}, "IVM7 - III7 - vim7 - IM7" });
            dict.push_back({ "7. J-POP: Classic Marunouchi", "Chromatic Descending", 4, {{0,4,3,4,0,0,0,0}, {4,4,2,4,0,1,0,0}, {8,4,2,0,-1,-1,-2,-3}, {12,4,1,4,0,0,0,0}}, "IVM7 - III7 - bIIIdim7 - iim7" });
            dict.push_back({ "7. J-POP: Classic Marunouchi", "J-Pop Modernized", 4, {{0,4,3,4,0,0,0,0}, {4,4,2,4,0,1,0,0}, {8,2,5,4,0,0,0,0}, {10,2,4,4,0,-1,0,-1}, {12,4,0,4,0,0,0,-1}}, "IVM7 - III7 - vim7 - vm7 - I7" });
            dict.push_back({ "7. J-POP: Classic Marunouchi", "Minor Perspective", 4, {{0,4,5,4,-1,0,-1,-1}, {4,4,4,4,0,1,0,0}, {8,4,0,4,0,-1,0,-1}, {12,4,1,4,0,1,0,0}}, "bVIM7 - V7 - im7 - II7" });
            dict.push_back({ "7. J-POP: Classic Marunouchi", "Parallel Shift", 8, {{0,4,5,4,0,0,0,0}, {4,4,2,4,0,0,0,0}, {8,4,5,4,0,0,0,0}, {12,4,1,4,0,0,0,0}, {16,4,0,4,0,0,0,0}, {20,4,2,4,0,0,0,0}, {24,8,5,4,0,0,0,0}}, "vim - iiim - vim - iim - I - iiim" });

            // =========================================================
            // 8. Emotional / Modals (レガシー保存領域)
            // =========================================================
            dict.push_back({ "8. Emotional / Modals", "Subdominant Minor (IV-iv-I)", 4, {{0,4,3,4,0,0,0,0}, {4,4,3,4,0,-1,0,-1}, {8,8,0,4,0,0,0,0}}, "IVmaj7 - ivm7 - Imaj7" });
            dict.push_back({ "8. Emotional / Modals", "Subdominant Minor 6th", 4, {{0,4,3,4,0,0,0,0}, {4,4,3,0,0,-1,0,-2}, {8,8,0,4,0,0,0,0}}, "IVmaj7 - iv6 - Imaj7" });
            dict.push_back({ "8. Emotional / Modals", "Descending 4-3-2-1", 4, {{0,4,3,4,0,0,0,0}, {4,4,2,4,0,0,0,0}, {8,4,1,4,0,0,0,0}, {12,4,0,4,0,0,0,0}}, "IVmaj7 - iiim7 - iim7 - Imaj7" });
            dict.push_back({ "8. Emotional / Modals", "Mario Cadence (Classic)", 4, {{0,4,5,4,-1,0,-1,-1}, {4,4,6,4,-1,0,-1,-1}, {8,8,0,4,0,0,0,0}}, "bVImaj7 - bVIImaj7 - Imaj7" });
            dict.push_back({ "8. Emotional / Modals", "Pedal Point", 4, {{0,4,0,0,4,0,0,0}, {4,4,3,4,0,0,0,0}, {8,8,4,0,-2,0,0,0}}, "I/III - IVM7 - V/IV" });
            dict.push_back({ "8. Emotional / Modals", "Line Cliche Minor", 4, {{0,4,5,0,0,0,0,-128}, {4,4,5,0,0,0,0,1}, {8,4,5,0,0,0,0,0}, {12,4,5,0,0,0,0,-1}}, "vim - vim(M7) - vim7 - vim6" });
            dict.push_back({ "8. Emotional / Modals", "Line Cliche Major", 4, {{0,4,0,0,0,0,0,-128}, {4,4,0,0,0,0,1,-128}, {8,4,0,0,0,0,2,-128}, {12,4,0,0,0,0,0,-1}}, "I - Iaug - I6 - I7" });

            // =========================================================
            // 9. Jazz & Blues (レガシー保存領域)
            // =========================================================
            dict.push_back({ "9. Jazz & Blues", "Major 2-5-1", 4, {{0,4,1,4,0,0,0,0}, {4,4,4,4,0,0,0,0}, {8,8,0,4,0,0,0,0}}, "iim9 - V13 - Imaj9" });
            dict.push_back({ "9. Jazz & Blues", "Minor 2-5-1 (Half Dim to Alt)", 4, {{0,4,1,0,0,0,-1,0}, {4,4,4,0,0,1,0,0}, {8,8,5,4,0,0,0,0}}, "iim7b5 - V7alt - vim9" });
            dict.push_back({ "9. Jazz & Blues", "Rhythm Changes Base", 8, {{0,2,0,4,0,0,0,0}, {2,2,5,4,0,1,0,0}, {4,2,1,4,0,1,0,0}, {6,2,4,4,0,0,0,0}, {8,2,2,4,0,0,0,0}, {10,2,5,4,0,1,0,0}, {12,2,1,4,0,0,0,0}, {14,2,4,4,0,0,0,0}, {16,8,0,4,0,0,0,0}, {24,8,0,4,0,0,0,0}}, "I-VI7-II7-V7-iii-VI7-ii-V7" });
            dict.push_back({ "9. Jazz & Blues", "Autumn Leaves", 8, {{0,4,1,4,0,0,0,0}, {4,4,4,4,0,0,0,0}, {8,4,0,4,0,0,0,0}, {12,4,3,4,0,0,0,0}, {16,4,6,0,0,0,-1,0}, {20,4,2,4,0,1,0,0}, {24,8,5,4,0,0,0,0}}, "iim7-V7-Imaj7-IVmaj7-viim7b5-III7-vim7" });
            dict.push_back({ "9. Jazz & Blues", "Dominant Chain", 4, {{0,4,2,4,0,1,0,0}, {4,4,5,4,0,1,0,0}, {8,4,1,4,0,1,0,0}, {12,4,4,4,0,0,0,0}}, "III7 - VI7 - II7 - V7" });
            dict.push_back({ "9. Jazz & Blues", "Altered Dominant", 4, {{0,4,1,4,0,0,0,0}, {4,4,4,4,0,0,1,0}, {8,8,0,4,0,0,0,0}}, "iim7 - V7(#5) - Imaj7" });
            dict.push_back({ "9. Jazz & Blues", "Coltrane Giant Steps Matrix", 8, {{0,4,0,0,0,0,0,0}, {4,4,2,0,-1,0,-1,-1}, {8,4,5,0,-1,0,-1,-1}, {12,4,6,0,-1,0,-1,-1}, {16,4,2,0,0,1,0,0}, {20,4,4,0,0,0,0,0}, {24,8,0,0,0,0,0,0}}, "I - bIII7 - bVI - VII7 - III - V7 - I" });
            dict.push_back({ "9. Jazz & Blues", "Charlie Parker Blues", 12, {
                {0,4,0,4,0,0,0,-1}, {4,4,6,0,0,0,-1,0}, {8,4,2,4,0,1,0,0}, {12,4,5,4,0,1,0,0}, {16,4,1,4,0,1,0,0}, {20,4,4,4,0,0,0,0},
                {24,4,0,4,0,0,0,-1}, {28,4,5,4,0,1,0,0}, {32,4,1,4,0,0,0,0}, {36,4,4,4,0,0,0,0}, {40,2,0,4,0,0,0,0}, {42,2,5,4,0,1,0,0}, {44,2,1,4,0,0,0,0}, {46,2,4,4,0,0,0,0}
            }, "I7 - viim7b5 - III7 - VI7 - II7 - V7 - I7 - VI7 - iim7 - V7 - I-VI-ii-V" });
            dict.push_back({ "9. Jazz & Blues", "Andalusian Cadence", 4, {{0,4,5,3,0,0,0,-128}, {4,4,4,3,0,0,0,-128}, {8,4,3,3,0,0,0,-128}, {12,4,2,3,0,1,0,-128}}, "vim - V - IV - III" });
            dict.push_back({ "9. Jazz & Blues", "Passing Diminished Ascent", 4, {{0,4,0,4,0,0,0,0}, {4,4,0,0,1,-1,-1,-2}, {8,4,1,4,0,0,0,0}, {12,4,4,4,0,0,0,0}}, "IM7 - #Idim7 - iim7 - V7" });

            // =========================================================
            // 10. Advanced Theory (レガシー保存領域)
            // =========================================================
            dict.push_back({ "10. Advanced Theory", "IVmaj7 as ii Sub", 4, {{0,4,3,4,0,0,0,0}, {4,4,4,4,0,0,0,0}, {8,8,0,4,0,0,0,0}}, "IVM7 - V7 - IM7" });
            dict.push_back({ "10. Advanced Theory", "Related ii-V to vi", 4, {{0,4,6,0,0,0,-1,0}, {4,4,2,4,0,1,0,0}, {8,8,5,4,0,0,0,0}}, "viim7b5 - III7 - vim7" });
            dict.push_back({ "10. Advanced Theory", "Classic Deceptive (V7 -> vi)", 4, {{0,4,1,4,0,0,0,0}, {4,4,4,4,0,0,0,0}, {8,8,5,4,0,0,0,0}}, "iim7 - V7 - vim7" });
            dict.push_back({ "10. Advanced Theory", "Tonic Sub Deceptive (V7 -> iii)", 4, {{0,4,1,4,0,0,0,0}, {4,4,4,4,0,0,0,0}, {8,8,2,4,0,0,0,0}}, "iim7 - V7 - iiim7" });
            dict.push_back({ "10. Advanced Theory", "Modal Deceptive (V7 -> bVIM7)", 4, {{0,4,1,4,0,0,0,0}, {4,4,4,4,0,0,0,0}, {8,8,5,4,-1,0,-1,-1}}, "iim7 - V7 - bVImaj7" });
            dict.push_back({ "10. Advanced Theory", "Neapolitan Deceptive (V7 -> bIIM7)", 4, {{0,4,1,4,0,0,0,0}, {4,4,4,4,0,0,0,0}, {8,8,1,4,-1,0,-1,-1}}, "iim7 - V7 - Dbmaj7" });
            dict.push_back({ "10. Advanced Theory", "Stella Deceptive (#ivm7b5)", 4, {{0,4,3,0,1,0,0,0}, {4,4,6,4,0,1,0,0}, {8,8,2,4,0,0,0,0}}, "#ivm7b5 - VII7 - iiim7" });
            dict.push_back({ "10. Advanced Theory", "Minor Tonic 6th (Im6)", 4, {{0,4,1,0,0,0,-1,0}, {4,4,4,0,0,1,0,0}, {8,8,0,0,0,0,0,-2}}, "iim7b5 - V7alt - Im6" });
            dict.push_back({ "10. Advanced Theory", "Minor Tonic maj7", 4, {{0,4,1,0,0,0,-1,0}, {4,4,4,0,0,1,0,0}, {8,8,0,0,0,0,0,-1}}, "iim7b5 - V7alt - Im(maj7)" });
            dict.push_back({ "10. Advanced Theory", "Minor Tonic 9th (Im9)", 4, {{0,4,1,0,0,0,-1,0}, {4,4,4,0,0,1,0,0}, {8,8,0,4,0,0,0,0}}, "iim7b5 - V7alt - Im9" });
            dict.push_back({ "10. Advanced Theory", "Altered Sec Dom (V7#5/vi)", 4, {{0,4,2,0,0,0,-1,0}, {4,4,2,0,0,1,1,0}, {8,8,5,4,0,0,0,0}}, "iiim7b5 - III7(#5) - vim9" });
            dict.push_back({ "10. Advanced Theory", "Harmonic Major 2-5-1", 4, {{0,4,1,4,0,0,-1,0}, {4,4,4,4,0,0,0,0}, {8,8,0,4,0,0,0,0}}, "iim7b5 - V7(b9,13) - Imaj7" });
        }
        return dict;
    }
}
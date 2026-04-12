#include "ProgressionEngine.h"
#include "MusicTheory.h"
#include "VoicingEngine.h"
#include <algorithm>
#include <cmath>
#include <map>
#include <string>
#include <tuple>

namespace ChordMatrix
{
    namespace AI_Models {

        // --- 1. Pop Bigram (McGill) ---
        static float getPopBigram(int fromDeg, int toDeg) {
            static const float matrix[7][7] = {
                { 0.050f, 0.089f, 0.030f, 0.347f, 0.227f, 0.107f, 0.010f }, // I
                { 0.228f, 0.030f, 0.056f, 0.125f, 0.465f, 0.050f, 0.010f }, // ii
                { 0.060f, 0.162f, 0.020f, 0.279f, 0.084f, 0.346f, 0.010f }, // iii
                { 0.504f, 0.052f, 0.039f, 0.040f, 0.268f, 0.020f, 0.010f }, // IV
                { 0.618f, 0.040f, 0.030f, 0.196f, 0.050f, 0.053f, 0.010f }, // V
                { 0.153f, 0.273f, 0.094f, 0.286f, 0.132f, 0.040f, 0.010f }, // vi
                { 0.209f, 0.010f, 0.414f, 0.092f, 0.030f, 0.046f, 0.020f }  // vii
            };
            if (fromDeg >= 0 && fromDeg < 7 && toDeg >= 0 && toDeg < 7) return matrix[fromDeg][toDeg];
            return 0.01f;
        }

        // --- 2. Trigram Bonus ---
        static float getTrigramBonus(int deg_n2, int deg_n1, int deg_n) {
            float bonus = 1.0f; // 乗算ボーナス

            // J-POP: IV-V-iii (433)
            if (deg_n2 == 3 && deg_n1 == 4 && deg_n == 2) bonus = 1.5f;
            // J-POP: V-iii-vi (536)
            else if (deg_n2 == 4 && deg_n1 == 2 && deg_n == 5) bonus = 1.5f;
            // Jazz: ii-V-I (251)
            else if (deg_n2 == 1 && deg_n1 == 4 && deg_n == 0) bonus = 1.2f;
            // Pop: I-V-vi (156)
            else if (deg_n2 == 0 && deg_n1 == 4 && deg_n == 5) bonus = 1.4f;
            // Pop: V-vi-IV (564)
            else if (deg_n2 == 4 && deg_n1 == 5 && deg_n == 3) bonus = 1.4f;

            return bonus;
        }

        // --- 3. Metrical Position Weight ---
        static float getMetricalWeight(int beatIdx, int deg) {
            // 強拍(1,3拍目=0,2), 弱拍(2,4拍目=1,3)
            bool isStrong = (beatIdx == 0 || beatIdx == 2);
            int func = (deg == 0 || deg == 5) ? 0 : // Tonic
                (deg == 4 || deg == 6) ? 2 : // Dominant
                1; // Subdominant / Other

            if (func == 0) return isStrong ? 1.2f : 0.8f;
            if (func == 2) return isStrong ? 0.8f : 1.2f;
            return 1.0f;
        }

        // --- 4. Tonnetz Distance (Common Tones & Circle of Fifths) ---
        static float getTonnetzDistance(const StepData& src, const StepData& dst) {
            std::array<int, 7> pA = { 0 }, pB = { 0 };
            int cA = VoicingEngine::getVoicedPitches(src, pA);
            int cB = VoicingEngine::getVoicedPitches(dst, pB);

            int common = 0;
            bool usedB[7] = { false };
            for (int i = 0; i < cA; ++i) {
                for (int j = 0; j < cB; ++j) {
                    if (!usedB[j] && (pA[i] % 12) == (pB[j] % 12)) {
                        common++;
                        usedB[j] = true;
                        break;
                    }
                }
            }
            common = std::min(3, common);

            auto getRootPC = [](const StepData& s) {
                return (((MusicTheory::getBasePitch(s, 0) + s.shift) % 12) + 12) % 12;
                };
            int rA = getRootPC(src);
            int rB = getRootPC(dst);

            static constexpr int FIFTH_POS[12] = { 0, 7, 2, 9, 4, 11, 6, 1, 8, 3, 10, 5 };
            int fd = std::abs(FIFTH_POS[rA] - FIFTH_POS[rB]);
            int circleSteps = std::min(fd, 12 - fd);

            // D = α*(3 - common) + β*circleDist
            return 1.0f * (3.0f - static_cast<float>(common)) + 0.5f * static_cast<float>(circleSteps);
        }

        // --- 5. Modal Interchange Emotions (Pink Category) ---
        struct Emotion { int deg; int scale; float bright; float surprise; std::string name; };
        static const std::vector<Emotion> BORROWED_EMOTIONS = {
            { 1, 0, 0.3f, 0.9f, "bII (Neapolitan)" },
            { 2, 0, 0.6f, 0.5f, "bIII (Mediant)" },
            { 5, 0, 0.4f, 0.7f, "bVI (Submediant)" },
            { 6, 0, 0.8f, 0.6f, "bVII (Subtonic)" }
        };

        struct Cand {
            int deg; int scale; bool isBorrowed;
            float cyanScore = 0; float yellowScore = 0; float pinkScore = 0;
            float tonnetzDist = 0; std::string tag;
        };
    }

    std::vector<ChordSuggestion> ProgressionEngine::suggestNextChords(const std::array<StepData, TotalSteps>& seq, int currentStep, float ppqPerStep) {
        std::vector<ChordSuggestion> suggestions;

        // 1. 文脈の取得
        int prevIdx = -1, prev2Idx = -1, nextIdx = -1;
        for (int s = currentStep; s >= 0; --s) {
            bool act = false; for (int v = 0; v < 7; ++v) if (seq[s].voices[v].isActive) act = true;
            if (act) { prevIdx = s; break; }
        }
        if (prevIdx > 0) {
            for (int s = prevIdx - 1; s >= 0; --s) {
                bool act = false; for (int v = 0; v < 7; ++v) if (seq[s].voices[v].isActive) act = true;
                if (act) { prev2Idx = s; break; }
            }
        }
        for (int s = currentStep + 1; s < TotalSteps; ++s) {
            bool act = false; for (int v = 0; v < 7; ++v) if (seq[s].voices[v].isActive) act = true;
            if (act) { nextIdx = s; break; }
        }

        const StepData& prev = (prevIdx >= 0) ? seq[prevIdx] : StepData{};
        int deg_n1 = (prevIdx >= 0) ? prev.chordDegree : -1;
        int deg_n2 = (prev2Idx >= 0) ? seq[prev2Idx].chordDegree : -1;
        int curScale = (prevIdx >= 0) ? prev.scaleType : 0;

        // 拍位置の計算
        int beatIdx = static_cast<int>(std::fmod(currentStep * 0.25f, 4.0f));

        std::vector<AI_Models::Cand> candidates;

        auto createDummyStep = [&](int d, int sc) {
            StepData st = prev; st.chordDegree = d; st.scaleType = sc; st.voicingMode = 4;
            for (int i = 0; i < 4; ++i) st.voices[i].isActive = true;
            return st;
            };

        // --- A. ダイアトニック候補 (Cyan / Yellow) ---
        for (int deg = 0; deg < 7; ++deg) {
            AI_Models::Cand c; c.deg = deg; c.scale = curScale; c.isBorrowed = false;

            float score = (deg_n1 >= 0) ? AI_Models::getPopBigram(deg_n1, deg) : 1.0f;
            score *= AI_Models::getTrigramBonus(deg_n2, deg_n1, deg);
            score *= AI_Models::getMetricalWeight(beatIdx, deg);

            c.tonnetzDist = (prevIdx >= 0) ? AI_Models::getTonnetzDistance(prev, createDummyStep(deg, curScale)) : 0.0f;

            // 距離ペナルティ (Dが大きいほどスコア低下)
            c.cyanScore = score * std::exp(-c.tonnetzDist * 0.2f);
            c.tag = "Diatonic (Markov+Trigram)";

            if (nextIdx >= 0) {
                const auto& target = seq[nextIdx];
                if (deg == (target.chordDegree + 3) % 7) {
                    c.yellowScore = score * 1.5f; c.tag = "Target: Sec. Dominant";
                }
                else if (std::abs(deg - target.chordDegree) == 1) {
                    c.yellowScore = score * 1.3f; c.tag = "Target: Chromatic Approach";
                }
            }
            candidates.push_back(c);
        }

        // --- B. モーダル・インターチェンジ候補 (Pink) ---
        for (const auto& mi : AI_Models::BORROWED_EMOTIONS) {
            AI_Models::Cand c; c.deg = mi.deg; c.scale = mi.scale; c.isBorrowed = true;

            c.tonnetzDist = (prevIdx >= 0) ? AI_Models::getTonnetzDistance(prev, createDummyStep(mi.deg, mi.scale)) : 0.0f;

            // SurpriseとBrightのバランスでエモーションスコアを計算
            float emotionScore = (mi.surprise * 0.6f) + (mi.bright * 0.4f);
            c.pinkScore = emotionScore * std::exp(-c.tonnetzDist * 0.1f);
            c.tag = mi.name;

            if (nextIdx >= 0) {
                const auto& target = seq[nextIdx];
                if (std::abs(mi.deg - target.chordDegree) == 1) {
                    c.yellowScore = emotionScore * 1.2f; c.tag = "Target: Tritone / Chromatic";
                }
            }
            candidates.push_back(c);
        }

        // ============================================================
        // 10スロット・ポートフォリオ配分 (Cyan:4, Yellow:3, Pink:3)
        // ============================================================
        auto sortAndFilter = [](std::vector<AI_Models::Cand>& cands, int mode, int maxSlots) {
            std::vector<AI_Models::Cand> result;

            std::sort(cands.begin(), cands.end(), [mode](const AI_Models::Cand& a, const AI_Models::Cand& b) {
                if (mode == 0) return a.cyanScore > b.cyanScore;
                if (mode == 1) return a.yellowScore > b.yellowScore;
                return a.pinkScore > b.pinkScore;
                });

            for (const auto& c : cands) {
                if (result.size() >= maxSlots) break;
                float score = (mode == 0) ? c.cyanScore : (mode == 1) ? c.yellowScore : c.pinkScore;
                if (score <= 0.01f) continue;

                // 多様性フィルタ: 同じルートがカテゴリ内に連続しないかチェック
                bool similar = false;
                for (const auto& r : result) {
                    if (r.deg == c.deg) { similar = true; break; }
                }
                if (!similar) result.push_back(c);
            }
            return result;
            };

        auto cyanList = sortAndFilter(candidates, 0, 4);
        auto yellowList = sortAndFilter(candidates, 1, nextIdx >= 0 ? 3 : 0);
        auto pinkList = sortAndFilter(candidates, 2, 3);

        // 不足分を補填して最大10枠へ
        int total = cyanList.size() + yellowList.size() + pinkList.size();
        if (total < 10) {
            auto extraCyan = sortAndFilter(candidates, 0, 4 + (10 - total));
            cyanList = extraCyan;
        }

        auto addSuggestions = [&](const std::vector<AI_Models::Cand>& list) {
            for (const auto& c : list) {
                // すでに追加されているかチェック
                bool exists = false;
                for (const auto& s : suggestions) {
                    if (s.targetDegree == c.deg && s.targetScale == c.scale) { exists = true; break; }
                }
                if (!exists) {
                    float finalScore = std::max({ c.cyanScore, c.yellowScore, c.pinkScore });
                    suggestions.push_back({ c.deg, c.scale, 0, finalScore, c.tag });
                }
            }
            };

        addSuggestions(cyanList);
        addSuggestions(yellowList);
        addSuggestions(pinkList);

        // UI表示のために最大10枠で切り捨て
        if (suggestions.size() > 10) suggestions.resize(10);

        return suggestions;
    }

    juce::StringArray ProgressionEngine::getModulationNames() {
        return {
            "Direct (V7 -> I)", "Standard ii-V-I", "Tritone Sub (bII7)", "Minor ii-V-i",
            "Backdoor (IVm7-bVII7)", "Passing Diminished", "Secondary Dominant",
            "Double ii-V", "Coltrane Matrix", "Extended Dominant", "Chromatic Up",
            "Chromatic Down", "Deceptive (V7->vi)", "Constant Structure",
            "Pedal Point", "Neo-Riemannian P", "Neo-Riemannian L", "Neo-Riemannian R"
        };
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
            // 1. US: Trap & Neo-Soul
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
            // 2. UK: Garage & Indie Pop
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
            // 3. J-POP: Anime & Modern
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
            // 4. K-POP: Maximalism & Neo-Soul
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
            // 5. Pop & Rock Basics
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
            // 6. J-POP: Classic Royal Road
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
            // 7. J-POP: Classic Marunouchi
            // =========================================================
            dict.push_back({ "7. J-POP: Classic Marunouchi", "Basic (IV-III-vi-I)", 4, {{0,4,3,4,0,0,0,0}, {4,4,2,4,0,0,0,0}, {8,4,5,4,0,0,0,0}, {12,4,0,4,0,0,0,0}}, "IVM7 - iiim7 - vim7 - IM7" });
            dict.push_back({ "7. J-POP: Classic Marunouchi", "With Sec Dom", 4, {{0,4,3,4,0,0,0,0}, {4,4,2,4,0,1,0,0}, {8,4,5,4,0,0,0,0}, {12,4,0,4,0,0,0,0}}, "IVM7 - III7 - vim7 - IM7" });
            dict.push_back({ "7. J-POP: Classic Marunouchi", "Chromatic Descending", 4, {{0,4,3,4,0,0,0,0}, {4,4,2,4,0,1,0,0}, {8,4,2,0,-1,-1,-2,-3}, {12,4,1,4,0,0,0,0}}, "IVM7 - III7 - bIIIdim7 - iim7" });
            dict.push_back({ "7. J-POP: Classic Marunouchi", "J-Pop Modernized", 4, {{0,4,3,4,0,0,0,0}, {4,4,2,4,0,1,0,0}, {8,2,5,4,0,0,0,0}, {10,2,4,4,0,-1,0,-1}, {12,4,0,4,0,0,0,-1}}, "IVM7 - III7 - vim7 - vm7 - I7" });
            dict.push_back({ "7. J-POP: Classic Marunouchi", "Minor Perspective", 4, {{0,4,5,4,-1,0,-1,-1}, {4,4,4,4,0,1,0,0}, {8,4,0,4,0,-1,0,-1}, {12,4,1,4,0,1,0,0}}, "bVIM7 - V7 - im7 - II7" });
            dict.push_back({ "7. J-POP: Classic Marunouchi", "Parallel Shift", 8, {{0,4,5,4,0,0,0,0}, {4,4,2,4,0,0,0,0}, {8,4,5,4,0,0,0,0}, {12,4,1,4,0,0,0,0}, {16,4,0,4,0,0,0,0}, {20,4,2,4,0,0,0,0}, {24,8,5,4,0,0,0,0}}, "vim - iiim - vim - iim - I - iiim" });

            // =========================================================
            // 8. Emotional / Modals
            // =========================================================
            dict.push_back({ "8. Emotional / Modals", "Subdominant Minor (IV-iv-I)", 4, {{0,4,3,4,0,0,0,0}, {4,4,3,4,0,-1,0,-1}, {8,8,0,4,0,0,0,0}}, "IVmaj7 - ivm7 - Imaj7" });
            dict.push_back({ "8. Emotional / Modals", "Subdominant Minor 6th", 4, {{0,4,3,4,0,0,0,0}, {4,4,3,0,0,-1,0,-2}, {8,8,0,4,0,0,0,0}}, "IVmaj7 - iv6 - Imaj7" });
            dict.push_back({ "8. Emotional / Modals", "Descending 4-3-2-1", 4, {{0,4,3,4,0,0,0,0}, {4,4,2,4,0,0,0,0}, {8,4,1,4,0,0,0,0}, {12,4,0,4,0,0,0,0}}, "IVmaj7 - iiim7 - iim7 - Imaj7" });
            dict.push_back({ "8. Emotional / Modals", "Mario Cadence (Classic)", 4, {{0,4,5,4,-1,0,-1,-1}, {4,4,6,4,-1,0,-1,-1}, {8,8,0,4,0,0,0,0}}, "bVImaj7 - bVIImaj7 - Imaj7" });
            dict.push_back({ "8. Emotional / Modals", "Pedal Point", 4, {{0,4,0,0,4,0,0,0}, {4,4,3,4,0,0,0,0}, {8,8,4,0,-2,0,0,0}}, "I/III - IVM7 - V/IV" });
            dict.push_back({ "8. Emotional / Modals", "Line Cliche Minor", 4, {{0,4,5,0,0,0,0,-128}, {4,4,5,0,0,0,0,1}, {8,4,5,0,0,0,0,0}, {12,4,5,0,0,0,0,-1}}, "vim - vim(M7) - vim7 - vim6" });
            dict.push_back({ "8. Emotional / Modals", "Line Cliche Major", 4, {{0,4,0,0,0,0,0,-128}, {4,4,0,0,0,0,1,-128}, {8,4,0,0,0,0,2,-128}, {12,4,0,0,0,0,0,-1}}, "I - Iaug - I6 - I7" });

            // =========================================================
            // 9. Jazz & Blues
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
            // 10. Advanced Theory
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
#include "VoicingEngine.h"
#include "MusicTheory.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace ChordMatrix
{
    bool VoicingEngine::isAutoPattern(int mode) {
        return mode == 4 || mode == 5 || mode == 6 || mode == 7 || mode == 8 ||
            mode == 9 || mode == 12 || mode == 13 || mode == 14 ||
            (mode >= 16 && mode <= 19);
    }

    int VoicingEngine::getPatternBPitches(const StepData& step, std::array<int, 7>& outPitches) {
        int rootPitch = MusicTheory::getBasePitch(step, 0) + step.shift;

        if (step.voicingMode == 6 || step.voicingMode == 7 || (step.voicingMode >= 16 && step.voicingMode <= 19)) {
            int ustRootOffset = 0;
            if (step.voicingMode == 6) ustRootOffset = 1;
            else if (step.voicingMode == 7) ustRootOffset = 8;
            else if (step.voicingMode == 16) ustRootOffset = 3;
            else if (step.voicingMode == 17) ustRootOffset = 6;
            else if (step.voicingMode == 18) ustRootOffset = 9;
            else if (step.voicingMode == 19) ustRootOffset = 2;

            int ustBase = rootPitch + 12 + ustRootOffset;

            outPitches[0] = rootPitch + step.voices[0].accidental;
            outPitches[1] = rootPitch + 4 + step.voices[1].accidental;
            outPitches[2] = rootPitch + 10 + step.voices[2].accidental;

            outPitches[3] = ustBase + step.voices[3].accidental;
            outPitches[4] = ustBase + 4 + step.voices[4].accidental;
            outPitches[5] = ustBase + 7 + step.voices[5].accidental;
            return 6;
        }
        else if (step.voicingMode == 4 || step.voicingMode == 5) {
            int r = rootPitch;
            int p3 = MusicTheory::getBasePitch(step, 1);
            int p5 = MusicTheory::getBasePitch(step, 2);
            int p7 = MusicTheory::getBasePitch(step, 3);
            int p9 = MusicTheory::getBasePitch(step, 4);
            int p13 = MusicTheory::getBasePitch(step, 6);

            int int3 = (((p3 - r) % 12) + 12) % 12;
            int int5 = (((p5 - r) % 12) + 12) % 12;
            int int7 = (((p7 - r) % 12) + 12) % 12;

            bool isDom = (int3 == 4 && int7 == 10);
            bool isHalfDim = (int3 == 3 && int5 == 6 && int7 == 10);

            int n1 = 0, n2 = 0, n3 = 0, n4 = 0;
            if (isDom) {
                n1 = p3; n2 = p13; n3 = p7; n4 = p9;
            }
            else if (isHalfDim) {
                n1 = p3; n2 = p5; n3 = p7; n4 = r + 12;
            }
            else {
                n1 = p3; n2 = p5; n3 = p7; n4 = p9;
            }

            n1 = r + (((n1 - r) % 12) + 12) % 12;
            n2 = n1 + (((n2 - n1) % 12) + 12) % 12; if (n2 == n1) n2 += 12;
            n3 = n2 + (((n3 - n2) % 12) + 12) % 12; if (n3 == n2) n3 += 12;
            n4 = n3 + (((n4 - n3) % 12) + 12) % 12; if (n4 == n3) n4 += 12;

            if (step.voicingMode == 4) {
                outPitches[0] = n1 + step.voices[0].accidental;
                outPitches[1] = n2 + step.voices[1].accidental;
                outPitches[2] = n3 + step.voices[2].accidental;
                outPitches[3] = n4 + step.voices[3].accidental;
            }
            else {
                outPitches[0] = n3 - 12 + step.voices[0].accidental;
                outPitches[1] = n4 - 12 + step.voices[1].accidental;
                outPitches[2] = n1 + step.voices[2].accidental;
                outPitches[3] = n2 + step.voices[3].accidental;
            }
            return 4;
        }
        else if (step.voicingMode == 8) {
            int r = rootPitch;
            int p11 = MusicTheory::getBasePitch(step, 5);
            int p7 = MusicTheory::getBasePitch(step, 3);
            int p3 = MusicTheory::getBasePitch(step, 1);

            int n1 = r;
            int n2 = n1 + (((p11 - n1) % 12) + 12) % 12; if (n2 == n1) n2 += 12;
            int n3 = n2 + (((p7 - n2) % 12) + 12) % 12; if (n3 == n2) n3 += 12;
            int n4 = n3 + (((p3 - n3) % 12) + 12) % 12; if (n4 == n3) n4 += 12;

            outPitches[0] = n1 + step.voices[0].accidental;
            outPitches[1] = n2 + step.voices[1].accidental;
            outPitches[2] = n3 + step.voices[2].accidental;
            outPitches[3] = n4 + step.voices[3].accidental;
            return 4;
        }
        else if (step.voicingMode == 9) {
            outPitches[0] = rootPitch + step.voices[0].accidental;
            outPitches[1] = MusicTheory::getBasePitch(step, 1) + step.voices[1].accidental;
            outPitches[2] = MusicTheory::getBasePitch(step, 3) + step.voices[3].accidental;
            return 3;
        }
        else if (step.voicingMode == 12) {
            outPitches[0] = rootPitch + step.voices[0].accidental;
            outPitches[1] = MusicTheory::getBasePitch(step, 5) - 12 + step.voices[1].accidental;
            outPitches[2] = MusicTheory::getBasePitch(step, 3) - 12 + step.voices[2].accidental;
            outPitches[3] = MusicTheory::getBasePitch(step, 1) + step.voices[3].accidental;
            outPitches[4] = MusicTheory::getBasePitch(step, 2) + step.voices[4].accidental;

            std::sort(outPitches.begin(), outPitches.begin() + 5);
            return 5;
        }
        else if (step.voicingMode == 13) {
            outPitches[0] = rootPitch + step.voices[0].accidental;
            outPitches[1] = rootPitch + 2 + step.voices[1].accidental;
            outPitches[2] = rootPitch + 4 + step.voices[2].accidental;
            outPitches[3] = rootPitch + 7 + step.voices[3].accidental;
            return 4;
        }
        else if (step.voicingMode == 14) {
            outPitches[0] = rootPitch + step.voices[0].accidental;
            outPitches[1] = rootPitch + 7 + step.voices[1].accidental;
            outPitches[2] = rootPitch + 15 + step.voices[2].accidental;
            outPitches[3] = rootPitch + 22 + step.voices[3].accidental;
            return 4;
        }
        return 0;
    }

    int VoicingEngine::getVoicedPitches(const StepData& step, std::array<int, 7>& outPitches) {
        if (isAutoPattern(step.voicingMode)) {
            std::array<int, 7> temp = { 0 };
            int maxV = getPatternBPitches(step, temp);
            int count = 0;
            for (int i = 0; i < maxV; ++i) {
                if (step.voices[i].isActive) {
                    outPitches[count++] = temp[i];
                }
            }
            return count;
        }

        int count = 0;
        for (int v = 0; v < NumVoices; ++v) {
            if (step.voices[v].isActive) {
                outPitches[count++] = MusicTheory::getBasePitch(step, v) +
                    step.voices[v].accidental +
                    (step.voices[v].octaveShift * 12) +
                    step.shift;
            }
        }
        if (count == 0) return 0;

        std::sort(outPitches.begin(), outPitches.begin() + count);

        int inv = step.inversion % count;
        for (int i = 0; i < inv; ++i) {
            outPitches[i] += 12;
        }
        std::sort(outPitches.begin(), outPitches.begin() + count);

        if (step.voicingMode == 1 && count >= 4) {
            outPitches[count - 2] -= 12;
            std::sort(outPitches.begin(), outPitches.begin() + count);
        }
        else if (step.voicingMode == 2 && count >= 4) {
            outPitches[count - 3] -= 12;
            std::sort(outPitches.begin(), outPitches.begin() + count);
        }
        else if (step.voicingMode == 10 && count >= 4) {
            outPitches[count - 2] -= 12;
            outPitches[count - 4] -= 12;
            std::sort(outPitches.begin(), outPitches.begin() + count);
        }
        else if (step.voicingMode == 11 && count >= 4) {
            outPitches[count - 2] -= 12;
            outPitches[count - 3] -= 12;
            std::sort(outPitches.begin(), outPitches.begin() + count);
        }
        else if (step.voicingMode == 15 && count >= 4) {
            outPitches[0] = outPitches[count - 1] - 12;
            std::sort(outPitches.begin(), outPitches.begin() + count);
        }
        else if (step.voicingMode == 3 && count >= 1) {
            int lowest = outPitches[0];
            for (int i = count; i > 0; --i) {
                outPitches[i] = outPitches[i - 1];
            }
            outPitches[0] = lowest - 12;
            count++;
            if (count > 3) outPitches[2] += 12;
            std::sort(outPitches.begin(), outPitches.begin() + count);
        }
        return count;
    }

    int VoicingEngine::getPitchForVoice(const StepData& step, int voiceIdx) {
        if (isAutoPattern(step.voicingMode)) {
            std::array<int, 7> temp = { 0 };
            getPatternBPitches(step, temp);
            return temp[voiceIdx];
        }

        std::array<std::pair<int, int>, 7> pitches;
        int count = 0;
        for (int v = 0; v < NumVoices; ++v) {
            if (step.voices[v].isActive) {
                pitches[count++] = { MusicTheory::getBasePitch(step, v) + step.voices[v].accidental + (step.voices[v].octaveShift * 12) + step.shift, v };
            }
        }
        if (count == 0) return 60;

        std::sort(pitches.begin(), pitches.begin() + count, [](auto& a, auto& b) { return a.first < b.first; });
        int inv = step.inversion % count;
        for (int i = 0; i < inv; ++i) pitches[i].first += 12;
        std::sort(pitches.begin(), pitches.begin() + count, [](auto& a, auto& b) { return a.first < b.first; });

        if (step.voicingMode == 1 && count >= 4) pitches[count - 2].first -= 12;
        else if (step.voicingMode == 2 && count >= 4) pitches[count - 3].first -= 12;
        else if (step.voicingMode == 10 && count >= 4) { pitches[count - 2].first -= 12; pitches[count - 4].first -= 12; }
        else if (step.voicingMode == 11 && count >= 4) { pitches[count - 2].first -= 12; pitches[count - 3].first -= 12; }
        else if (step.voicingMode == 15 && count >= 4) { pitches[0].first = pitches[count - 1].first - 12; }
        else if (step.voicingMode == 3 && count >= 3) pitches[1].first += 12;

        for (int i = 0; i < count; ++i) {
            if (pitches[i].second == voiceIdx) return pitches[i].first;
        }
        return 60;
    }

    juce::String VoicingEngine::getRecognizedChordName(const std::array<StepData, TotalSteps>& seq, int targetStep, float ppqPerStep) {
        int effS = targetStep;
        bool anyActive = false;

        for (int s = targetStep; s >= 0; --s) {
            float distBeats = static_cast<float>(targetStep - s) * ppqPerStep;
            bool stepCovers = false;
            for (int v = 0; v < NumVoices; ++v) {
                if (seq[s].voices[v].isActive && seq[s].gateLength > distBeats + 0.001f) {
                    stepCovers = true;
                    anyActive = true;
                }
            }
            if (stepCovers) {
                effS = s;
                break;
            }
        }

        if (!anyActive) return "-";

        const auto& step = seq[effS];
        std::array<int, 7> voicedPitches = { 0 };
        int activeCount = getVoicedPitches(step, voicedPitches);

        if (activeCount == 0) return "-";

        int theoreticalRoot = (((MusicTheory::getBasePitch(step, 0) + step.shift) % 12) + 12) % 12;

        if (isAutoPattern(step.voicingMode)) {
            juce::String rootName = MusicTheory::getNoteName(theoreticalRoot);

            if (step.voicingMode == 6) return rootName + "7 (UST bII)\n" + MusicTheory::getNoteName((theoreticalRoot + 1) % 12) + " / " + rootName + "7";
            if (step.voicingMode == 7) return rootName + "7 (UST bVI)\n" + MusicTheory::getNoteName((theoreticalRoot + 8) % 12) + " / " + rootName + "7";
            if (step.voicingMode == 16) return rootName + "7 (UST bIII)\n" + MusicTheory::getNoteName((theoreticalRoot + 3) % 12) + " / " + rootName + "7";
            if (step.voicingMode == 17) return rootName + "7 (UST bV)\n" + MusicTheory::getNoteName((theoreticalRoot + 6) % 12) + " / " + rootName + "7";
            if (step.voicingMode == 18) return rootName + "7 (UST VI)\n" + MusicTheory::getNoteName((theoreticalRoot + 9) % 12) + " / " + rootName + "7";
            if (step.voicingMode == 19) return rootName + "7 (UST II)\n" + MusicTheory::getNoteName((theoreticalRoot + 2) % 12) + " / " + rootName + "7";

            if (step.voicingMode == 8) return rootName + "7sus4 (Quartal)\n4ths Stack";
            if (step.voicingMode == 9) return rootName + "7 (Shell)\nRoot-3rd-7th";
            if (step.voicingMode == 12) return rootName + "m11 (So What)\n4th-4th-4th-M3";
            if (step.voicingMode == 13) return rootName + " (Cluster)\n2nds";
            if (step.voicingMode == 14) return rootName + "m11 (Kenny B.)\nMin11ths";

            if (step.voicingMode == 4 || step.voicingMode == 5) {
                bool hasChanges = false;
                for (int i = 0; i < 4; ++i) {
                    if (step.voices[i].accidental != 0) hasChanges = true;
                }

                juce::String type = (step.chordDegree == 1) ? "m9" : (step.chordDegree == 4) ? "13" : "Maj9";
                juce::String modeName = (step.voicingMode == 4) ? "Type A" : "Type B";

                if (!hasChanges) return rootName + type + " (Rootless)\n" + modeName;

                bool hasRel[12] = { false };
                for (int i = 0; i < activeCount; ++i) {
                    hasRel[(((voicedPitches[i] - theoreticalRoot) % 12) + 12) % 12] = true;
                }
                bool isMinor = hasRel[3];
                bool isMajor = hasRel[4];
                bool hasDom7 = hasRel[10];
                bool hasMaj7 = hasRel[11];

                if ((isMinor || isMajor) && (hasDom7 || hasMaj7)) {
                    juce::String ext = "";
                    if (hasRel[1] || hasRel[2]) ext += "9 ";
                    if (hasRel[5] || hasRel[6]) ext += "11 ";
                    if (hasRel[8] || hasRel[9]) ext += "13 ";

                    juce::String customType = "";
                    if (isMinor && hasDom7) customType = "m";
                    else if (isMajor && hasDom7) customType = "7";
                    else if (isMajor && hasMaj7) customType = "Maj7";

                    return rootName + customType + " (Rootless)\nadd " + ext.trim();
                }
            }
        }

        bool has[12] = { false };
        int lowestRaw = 99999;

        for (int i = 0; i < activeCount; ++i) {
            if (voicedPitches[i] < lowestRaw) lowestRaw = voicedPitches[i];
            has[(((voicedPitches[i] % 12) + 12) % 12)] = true;
        }

        if (lowestRaw == 99999) return "-";

        int bestRoot = -1;
        juce::String bestType = "??";
        int bestScore = -1;

        for (int testRoot = 0; testRoot < 12; ++testRoot) {
            if (!has[testRoot]) continue;

            bool rHas[12] = { false };
            for (int i = 0; i < 12; ++i) {
                if (has[i]) rHas[(((i - testRoot) % 12) + 12) % 12] = true;
            }

            juce::String type = "??";
            int score = 0;

            if (rHas[0] && rHas[4] && rHas[7]) { type = "Maj"; score = 10; if (rHas[10]) { type = "7"; score = 12; } if (rHas[11]) { type = "Maj7"; score = 12; } }
            else if (rHas[0] && rHas[3] && rHas[7]) { type = "m"; score = 10; if (rHas[10]) { type = "m7"; score = 12; } if (rHas[11]) { type = "m(Maj7)"; score = 12; } }
            else if (rHas[0] && rHas[3] && rHas[6]) { type = "dim"; score = 10; if (rHas[9]) { type = "dim7"; score = 12; } if (rHas[10]) { type = "m7b5"; score = 12; } }
            else if (rHas[0] && rHas[4] && rHas[8]) { type = "aug"; score = 9; }
            else if (rHas[0] && rHas[5] && rHas[7]) { type = "sus4"; score = 9; }
            else if (rHas[0] && rHas[5] && rHas[10]) { type = "7sus4"; score = 11; }
            else if (rHas[0] && rHas[4] && !rHas[7]) { type = "Maj(no5)"; score = 5; }
            else if (rHas[0] && rHas[3] && !rHas[7]) { type = "m(no5)"; score = 5; }
            else if (rHas[0] && !rHas[3] && !rHas[4] && rHas[7]) { type = "5"; score = 4; }

            if (type != "??") {
                if (rHas[2]) type += "(9)";
                if (rHas[5] && type != "sus4" && type != "7sus4") type += "(11)";
                if (rHas[9]) type += "(13)";

                if (testRoot == theoreticalRoot) score += 5;
                if (testRoot == ((((lowestRaw % 12) + 12) % 12))) score += 2;

                if (score > bestScore) {
                    bestScore = score;
                    bestType = type;
                    bestRoot = testRoot;
                }
            }
        }

        if (bestType == "??") {
            bestRoot = (((lowestRaw % 12) + 12) % 12);
            bestType = (activeCount == 1) ? "" : "Custom";
        }

        juce::String absName = MusicTheory::getNoteName(bestRoot) + bestType;
        int keyRoot = (((step.keyRoot + step.shift) % 12) + 12) % 12;
        int diff = (((bestRoot - keyRoot) % 12) + 12) % 12;
        static const char* const romanNames[] = { "I", "bII", "II", "bIII", "III", "IV", "bV", "V", "bVI", "VI", "bVII", "VII" };
        juce::String relName = juce::String(romanNames[diff]) + bestType;

        int lowestVoicedAbs = (((lowestRaw % 12) + 12) % 12);
        if (lowestVoicedAbs != bestRoot && bestType != "Custom" && bestType != "") {
            juce::String slash = "on" + MusicTheory::getNoteName(lowestVoicedAbs);
            relName += slash;
            absName += slash;
        }

        if (step.voicingMode == 1) { relName += " (Drop 2)"; absName += " (Drop 2)"; }
        else if (step.voicingMode == 2) { relName += " (Drop 3)"; absName += " (Drop 3)"; }
        else if (step.voicingMode == 10) { relName += " (Drop 2&4)"; absName += " (Drop 2&4)"; }
        else if (step.voicingMode == 11) { relName += " (Drop 2&3)"; absName += " (Drop 2&3)"; }
        else if (step.voicingMode == 15) { relName += " (Block)"; absName += " (Block)"; }
        else if (step.voicingMode == 3) { relName += " (Spread)"; absName += " (Spread)"; }

        return relName + "\n(" + absName + ")";
    }

    // =========================================================================
        // ★ 究極AI最適化エンジン: Viterbi + 5つの音楽的バリエーション (altIndex対応)
        // =========================================================================
    void VoicingEngine::optimizeStep(std::array<StepData, TotalSteps>& seq, int targetStep, float ppqPerStep, int altIndex) {
        std::vector<int> activeSteps;
        for (int s = 0; s < TotalSteps; ++s) {
            bool hasActive = false;
            for (int v = 0; v < 7; ++v) {
                if (seq[s].voices[v].isActive) { hasActive = true; break; }
            }
            if (hasActive) activeSteps.push_back(s);
        }

        if (activeSteps.size() < 2) return;

        struct State {
            int voicingMode;
            int inversion;
            int mutatedVoiceIdx;
            int mutatedAcc;
            std::array<int, 7> pitches;
            int count;
            float cost;
            int bestPrevState;
        };

        std::vector<int> allowedModes = { 0, 1, 4, 5, 8, 12 };
        std::vector<std::vector<State>> dp(activeSteps.size());

        // バリエーション(altIndex)に応じたコスト重みの設定
        int safeAlt = altIndex % 5;
        float topNoteWeight = (safeAlt == 1) ? 15.0f : 3.0f; // Alt 1: メロディ重視
        float bassNoteWeight = (safeAlt == 2) ? 20.0f : 0.0f; // Alt 2: ベース重視
        float centerGravity = (safeAlt == 3) ? 10.0f : 0.0f; // Alt 3: 中央密集重視

        for (size_t t = 0; t < activeSteps.size(); ++t) {
            int s = activeSteps[t];

            if (seq[s].isLocked) {
                std::array<int, 7> testPitches = { 0 };
                int count = getVoicedPitches(seq[s], testPitches);
                dp[t].push_back({ seq[s].voicingMode, seq[s].inversion, -1, 0, testPitches, count, 0.0f, -1 });
                continue;
            }

            std::vector<StepData> variations;
            variations.push_back(seq[s]);

            if (seq[s].chordDegree == 4 || (seq[s].voices[1].isActive && seq[s].voices[3].accidental == -1)) {
                if (seq[s].voicingMode == 4 || seq[s].voicingMode == 5) {
                    StepData alt1 = seq[s]; alt1.voices[1].accidental = -1;
                    variations.push_back(alt1);
                    StepData alt2 = seq[s]; alt2.voices[3].accidental = -1;
                    variations.push_back(alt2);
                }
            }

            for (size_t varIdx = 0; varIdx < variations.size(); ++varIdx) {
                StepData& baseVar = variations[varIdx];
                for (int mode : allowedModes) {
                    int maxInv = isAutoPattern(mode) ? 1 : 4;
                    for (int inv = 0; inv < maxInv; ++inv) {
                        StepData tempStep = baseVar;
                        tempStep.voicingMode = mode;
                        tempStep.inversion = inv;
                        std::array<int, 7> testPitches = { 0 };
                        int count = getVoicedPitches(tempStep, testPitches);

                        if (count > 0) {
                            int mutIdx = -1, mutAcc = 0;
                            if (varIdx == 1) { mutIdx = 1; mutAcc = -1; }
                            if (varIdx == 2) { mutIdx = 3; mutAcc = -1; }
                            dp[t].push_back({ mode, inv, mutIdx, mutAcc, testPitches, count, 0.0f, -1 });
                        }
                    }
                }
            }
        }

        for (size_t t = 1; t < activeSteps.size(); ++t) {
            for (size_t v = 0; v < dp[t].size(); ++v) {
                float minCost = 9999999.0f;
                int bestU = -1;

                for (size_t u = 0; u < dp[t - 1].size(); ++u) {
                    float transitionCost = 0.0f;
                    int compCount = std::min(dp[t - 1][u].count, dp[t][v].count);
                    int maxJump = 0;

                    for (int i = 0; i < compCount; ++i) {
                        int dist = std::abs(dp[t][v].pitches[i] - dp[t - 1][u].pitches[i]);
                        transitionCost += static_cast<float>(dist);
                        if (dist > maxJump) maxJump = dist;
                        if (dist == 0) transitionCost -= 3.0f;
                    }

                    if (maxJump > 5) transitionCost += static_cast<float>(maxJump) * 10.0f;
                    if (maxJump > 10) transitionCost += 500.0f;

                    // ★バリエーション重みの適用
                    int topDist = std::abs(dp[t][v].pitches[dp[t][v].count - 1] - dp[t - 1][u].pitches[dp[t - 1][u].count - 1]);
                    transitionCost += static_cast<float>(topDist) * topNoteWeight;

                    int bassDist = std::abs(dp[t][v].pitches[0] - dp[t - 1][u].pitches[0]);
                    transitionCost += static_cast<float>(bassDist) * bassNoteWeight;

                    float stateCost = 0.0f;
                    int topPitch = dp[t][v].pitches[dp[t][v].count - 1];
                    int bottomPitch = dp[t][v].pitches[0];
                    if (topPitch > 84) stateCost += static_cast<float>(topPitch - 84) * 5.0f;
                    if (bottomPitch < 40) stateCost += static_cast<float>(40 - bottomPitch) * 5.0f;

                    // Alt 3 用の中央密集ペナルティ
                    if (safeAlt == 3) {
                        float distFromCenter = std::abs(((float)topPitch + (float)bottomPitch) / 2.0f - 60.0f);
                        stateCost += distFromCenter * centerGravity;
                    }

                    float totalCost = dp[t - 1][u].cost + transitionCost + stateCost;

                    if (totalCost < minCost) {
                        minCost = totalCost;
                        bestU = static_cast<int>(u);
                    }
                }
                dp[t][v].cost = minCost;
                dp[t][v].bestPrevState = bestU;
            }
        }

        // バックトラック（Alt 4 の場合は、あえて2番目に良い着地点を選ぶ）
        std::vector<std::pair<float, int>> finalStates;
        for (size_t v = 0; v < dp.back().size(); ++v) {
            finalStates.push_back({ dp.back()[v].cost, static_cast<int>(v) });
        }
        std::sort(finalStates.begin(), finalStates.end(), [](const auto& a, const auto& b) { return a.first < b.first; });

        int currState = finalStates[0].second;
        if (safeAlt == 4 && finalStates.size() > 1) {
            currState = finalStates[1].second; // Alt 4: アナザー・エンディング
        }

        for (int t = static_cast<int>(activeSteps.size()) - 1; t >= 0; --t) {
            int s = activeSteps[t];
            if (!seq[s].isLocked) {
                seq[s].voicingMode = dp[t][currState].voicingMode;
                seq[s].inversion = dp[t][currState].inversion;
                if (dp[t][currState].mutatedVoiceIdx != -1) {
                    seq[s].voices[dp[t][currState].mutatedVoiceIdx].accidental = dp[t][currState].mutatedAcc;
                }
            }
            currState = dp[t][currState].bestPrevState;
        }
    }
}

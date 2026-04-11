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

        // =====================================================================
        // UST ボイシング (Upper Structure Triads)
        // =====================================================================
        if (step.voicingMode == 6 || step.voicingMode == 7 || (step.voicingMode >= 16 && step.voicingMode <= 19)) {
            int ustRootOffset = 0;
            if (step.voicingMode == 6) ustRootOffset = 1;
            else if (step.voicingMode == 7) ustRootOffset = 8;
            else if (step.voicingMode == 16) ustRootOffset = 3;
            else if (step.voicingMode == 17) ustRootOffset = 6;
            else if (step.voicingMode == 18) ustRootOffset = 9;
            else if (step.voicingMode == 19) ustRootOffset = 2;

            int ustBase = rootPitch + 12 + ustRootOffset;

            // 左手骨格 (Root, 3rd, 7th)
            outPitches[0] = rootPitch + step.voices[0].accidental;
            outPitches[1] = rootPitch + 4 + step.voices[1].accidental;
            outPitches[2] = rootPitch + 10 + step.voices[2].accidental;

            // 右手トライアド
            outPitches[3] = ustBase + step.voices[3].accidental;
            outPitches[4] = ustBase + 4 + step.voices[4].accidental;
            outPitches[5] = ustBase + 7 + step.voices[5].accidental;
            return 6;
        }
        // =====================================================================
        // スケール完全追従型 ルートレス・ボイシング (Type A / Type B)
        // =====================================================================
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

            int n1, n2, n3, n4;
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
        // =====================================================================
        // スケール追従型 Quartal ボイシング
        // =====================================================================
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
        // =====================================================================
        // Shell (1-3-7)
        // =====================================================================
        else if (step.voicingMode == 9) {
            outPitches[0] = rootPitch + step.voices[0].accidental;
            outPitches[1] = MusicTheory::getBasePitch(step, 1) + step.voices[1].accidental;
            outPitches[2] = MusicTheory::getBasePitch(step, 3) + step.voices[3].accidental;
            return 3;
        }
        // =====================================================================
        // スケール追従型 So What ボイシング
        // =====================================================================
        else if (step.voicingMode == 12) {
            outPitches[0] = rootPitch + step.voices[0].accidental;
            outPitches[1] = MusicTheory::getBasePitch(step, 5) - 12 + step.voices[1].accidental;
            outPitches[2] = MusicTheory::getBasePitch(step, 3) - 12 + step.voices[2].accidental;
            outPitches[3] = MusicTheory::getBasePitch(step, 1) + step.voices[3].accidental;
            outPitches[4] = MusicTheory::getBasePitch(step, 2) + step.voices[4].accidental;

            std::sort(outPitches.begin(), outPitches.begin() + 5);
            return 5;
        }
        // =====================================================================
        // Cluster (2nds)
        // =====================================================================
        else if (step.voicingMode == 13) {
            outPitches[0] = rootPitch + step.voices[0].accidental;
            outPitches[1] = rootPitch + 2 + step.voices[1].accidental;
            outPitches[2] = rootPitch + 4 + step.voices[2].accidental;
            outPitches[3] = rootPitch + 7 + step.voices[3].accidental;
            return 4;
        }
        // =====================================================================
        // Kenny Barron (Min11ths)
        // =====================================================================
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

        // インバージョン処理
        int inv = step.inversion % count;
        for (int i = 0; i < inv; ++i) {
            outPitches[i] += 12;
        }
        std::sort(outPitches.begin(), outPitches.begin() + count);

        // ドロップおよびスプレッド処理
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
    // ★新AI最適化エンジン: 幾何学的音楽理論、ジャズガイドトーン、対位法禁則の統合
    // =========================================================================
    void VoicingEngine::optimizeStep(std::array<StepData, TotalSteps>& seq, int targetStep, float ppqPerStep, int altIndex) {
        int prevActiveStep = -1;
        for (int s = targetStep - 1; s >= 0; --s) {
            bool hasActive = false;
            for (int v = 0; v < NumVoices; ++v) {
                if (seq[s].voices[v].isActive) { hasActive = true; break; }
            }
            if (hasActive) { prevActiveStep = s; break; }
        }

        if (prevActiveStep < 0) return; // 前和音が存在しない場合は最適化不可

        std::array<int, 7> prevPitches = { 0 };
        int prevCount = getVoicedPitches(seq[prevActiveStep], prevPitches);
        if (prevCount == 0) return;

        struct Candidate {
            int voicingMode;
            int inversion;
            float cost;
        };
        std::vector<Candidate> candidates;

        StepData baseStep = seq[targetStep];
        bool autoPat = isAutoPattern(baseStep.voicingMode);

        std::vector<int> modesToTry;
        // Rootlessの場合はAとB両方を試行し、クロストレスな方を自動選択する
        if (baseStep.voicingMode == 4 || baseStep.voicingMode == 5) {
            modesToTry = { 4, 5 };
        }
        else {
            modesToTry = { baseStep.voicingMode };
        }

        int maxInv = autoPat ? 1 : 7; // オートパターンの場合はインバージョン不要

        // ★修正: 「shift」および各voiceの「octaveShift」は一切変更せず、転回形とA/Bモードのみを探索する
        for (int mode : modesToTry) {
            for (int inv = 0; inv < maxInv; ++inv) {
                Candidate c;
                c.voicingMode = mode;
                c.inversion = inv;
                c.cost = 0.0f;
                candidates.push_back(c);
            }
        }

        // --- 事前準備：ジャズガイドトーン (3rd & 7th) の特定 ---
        int prev7thPC = -1;
        if (seq[prevActiveStep].chordDegree == 1 || seq[prevActiveStep].chordDegree == 4) {
            prev7thPC = (((MusicTheory::getBasePitch(seq[prevActiveStep], 3) + seq[prevActiveStep].shift) % 12) + 12) % 12;
        }
        int curr3rdPC = (((MusicTheory::getBasePitch(baseStep, 1) + baseStep.shift) % 12) + 12) % 12;
        bool isGuideToneResolution = (seq[prevActiveStep].chordDegree == 1 && baseStep.chordDegree == 4) ||
            (seq[prevActiveStep].chordDegree == 4 && baseStep.chordDegree == 0);

        for (auto& c : candidates) {
            StepData testStep = baseStep;
            testStep.voicingMode = c.voicingMode;
            testStep.inversion = c.inversion;
            // shiftは元の値を維持

            std::array<int, 7> testPitches = { 0 };
            int testCount = getVoicedPitches(testStep, testPitches);

            if (testCount == 0) { c.cost = 999999.0f; continue; }

            // ① L1ノルム (Taxicab Metric) + L∞ノルム (Chebyshev Metric)
            float l1Cost = 0.0f;
            int maxJump = 0;

            for (int i = 0; i < testCount; ++i) {
                int minDist = 9999;
                for (int j = 0; j < prevCount; ++j) {
                    int dist = std::abs(testPitches[i] - prevPitches[j]);
                    if (dist < minDist) minDist = dist;
                }
                l1Cost += static_cast<float>(minDist);
                if (minDist > maxJump) maxJump = minDist;

                // ネオ・リーマン的パーシモニアス評価 (共通音の保持にボーナス)
                if (minDist == 0) c.cost -= 10.0f;
            }
            c.cost += l1Cost * 2.0f; // 基本の移動距離コスト
            if (maxJump > 6) c.cost += static_cast<float>(maxJump) * 5.0f; // 大跳躍には指数的ペナルティ

            // ② ジャズ和声：ガイドトーンの解決 (7th -> 3rd by semitone/tone)
            if (isGuideToneResolution && prev7thPC != -1) {
                for (int i = 0; i < prevCount; ++i) {
                    if ((((prevPitches[i] % 12) + 12) % 12) == prev7thPC) {
                        for (int j = 0; j < testCount; ++j) {
                            if ((((testPitches[j] % 12) + 12) % 12) == curr3rdPC) {
                                int dist = testPitches[j] - prevPitches[i];
                                // 7度が半音か全音下がって3度になる美しい連結を発見
                                if (dist == -1 || dist == -2) {
                                    c.cost -= 50.0f; // 圧倒的な優位性を与える
                                }
                            }
                        }
                    }
                }
            }

            // ③ 伝統的対位法：連続5度・連続8度の厳格な禁止
            if (testCount == prevCount) {
                for (int i = 0; i < testCount - 1; ++i) {
                    for (int j = i + 1; j < testCount; ++j) {
                        int prevInt = (prevPitches[j] - prevPitches[i]) % 12;
                        int currInt = (testPitches[j] - testPitches[i]) % 12;

                        if ((prevInt == 7 || prevInt == 0) && currInt == prevInt) {
                            // 同じ方向に移動しているか（並行）をチェック
                            if ((testPitches[i] - prevPitches[i]) * (testPitches[j] - prevPitches[j]) > 0) {
                                c.cost += 1000.0f; // 禁則違反（即座に枝刈り相当）
                            }
                        }
                    }
                }
            }

            // ④ 親指の法則 (Rule of Thumb: ピアニストの最高音は C4~G5 に収めるべき)
            int highestPitch = testPitches[testCount - 1];
            if (highestPitch > 79) c.cost += static_cast<float>(highestPitch - 79) * 5.0f;
            if (highestPitch < 60) c.cost += static_cast<float>(60 - highestPitch) * 5.0f;

            // ⑤ Low Interval Limit (低音域の濁り防止)
            int lowestPitch = testPitches[0];
            if (lowestPitch < 40) c.cost += static_cast<float>(40 - lowestPitch) * 3.0f;
        }

        // コスト順にソート (動的計画法におけるローカル最適パスのランキング)
        std::sort(candidates.begin(), candidates.end(), [](const Candidate& a, const Candidate& b) {
            return a.cost < b.cost;
            });

        if (!candidates.empty()) {
            std::vector<Candidate> uniqueCandidates;
            uniqueCandidates.push_back(candidates[0]);

            // 重複解を除外（同じ響きを排除）
            for (size_t i = 1; i < candidates.size(); ++i) {
                bool isUnique = true;
                for (const auto& uc : uniqueCandidates) {
                    if (candidates[i].voicingMode == uc.voicingMode &&
                        candidates[i].inversion == uc.inversion) {
                        isUnique = false; break;
                    }
                }
                if (isUnique) uniqueCandidates.push_back(candidates[i]);
            }

            // ユーザーがボタンを押すたびに、2位、3位のオルタナティヴな最適解に切り替わる
            int selectedIdx = altIndex % uniqueCandidates.size();
            auto& best = uniqueCandidates[selectedIdx];

            seq[targetStep].voicingMode = best.voicingMode;
            seq[targetStep].inversion = best.inversion;
        }
    }
}
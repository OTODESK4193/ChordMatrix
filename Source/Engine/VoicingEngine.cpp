#include "VoicingEngine.h"
#include "MusicTheory.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace ChordMatrix
{
    bool VoicingEngine::isAutoPattern(int mode) {
        // Rootless(4,5), UST(6,7), Quartal(8), Shell(9), SoWhat(12), Cluster(13), KennyBarron(14)
        return mode == 4 || mode == 5 || mode == 6 || mode == 7 || mode == 8 ||
            mode == 9 || mode == 12 || mode == 13 || mode == 14;
    }

    int VoicingEngine::getPatternBPitches(const StepData& step, std::array<int, 7>& outPitches) {
        int rootPitch = MusicTheory::getBasePitch(step, 0) + step.shift;

        if (step.voicingMode == 6 || step.voicingMode == 7) {
            int ustRootOffset = (step.voicingMode == 6) ? 1 : 8;
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
            int r = rootPitch; int typeA[4] = { r, r, r, r };
            if (step.chordDegree == 1) { typeA[0] = r + 3; typeA[1] = r + 7; typeA[2] = r + 10; typeA[3] = r + 14; }
            else if (step.chordDegree == 4) { typeA[0] = r + 4; typeA[1] = r + 9; typeA[2] = r + 10; typeA[3] = r + 14; }
            else { typeA[0] = r + 4; typeA[1] = r + 7; typeA[2] = r + 11; typeA[3] = r + 14; }

            if (step.voicingMode == 4) { for (int i = 0; i < 4; ++i) outPitches[i] = typeA[i] + step.voices[i].accidental; }
            else {
                outPitches[0] = typeA[2] - 12 + step.voices[0].accidental;
                outPitches[1] = typeA[3] - 12 + step.voices[1].accidental;
                outPitches[2] = typeA[0] + step.voices[2].accidental;
                outPitches[3] = typeA[1] + step.voices[3].accidental;
            }
            return 4;
        }
        else if (step.voicingMode == 8) {
            outPitches[0] = rootPitch + step.voices[0].accidental;
            outPitches[1] = rootPitch + 5 + step.voices[1].accidental;
            outPitches[2] = rootPitch + 10 + step.voices[2].accidental;
            outPitches[3] = rootPitch + 15 + step.voices[3].accidental;
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
            outPitches[1] = rootPitch + 5 + step.voices[1].accidental;
            outPitches[2] = rootPitch + 10 + step.voices[2].accidental;
            outPitches[3] = rootPitch + 15 + step.voices[3].accidental;
            outPitches[4] = rootPitch + 19 + step.voices[4].accidental;
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
                // isActive を絶対基準としてピッチを抽出
                if (step.voices[i].isActive) outPitches[count++] = temp[i];
            }
            return count;
        }

        int count = 0;
        for (int v = 0; v < NumVoices; ++v) {
            if (step.voices[v].isActive) {
                outPitches[count++] = MusicTheory::getBasePitch(step, v) + step.voices[v].accidental + (step.voices[v].octaveShift * 12) + step.shift;
            }
        }
        if (count == 0) return 0;

        std::sort(outPitches.begin(), outPitches.begin() + count);
        int inv = step.inversion % count;
        for (int i = 0; i < inv; ++i) outPitches[i] += 12;
        std::sort(outPitches.begin(), outPitches.begin() + count);

        if (step.voicingMode == 1 && count >= 4) { outPitches[count - 2] -= 12; std::sort(outPitches.begin(), outPitches.begin() + count); }
        else if (step.voicingMode == 2 && count >= 4) { outPitches[count - 3] -= 12; std::sort(outPitches.begin(), outPitches.begin() + count); }
        else if (step.voicingMode == 10 && count >= 4) { outPitches[count - 2] -= 12; outPitches[count - 4] -= 12; std::sort(outPitches.begin(), outPitches.begin() + count); }
        else if (step.voicingMode == 11 && count >= 4) { outPitches[count - 2] -= 12; outPitches[count - 3] -= 12; std::sort(outPitches.begin(), outPitches.begin() + count); }
        else if (step.voicingMode == 15 && count >= 4) { outPitches[0] = outPitches[count - 1] - 12; std::sort(outPitches.begin(), outPitches.begin() + count); }
        else if (step.voicingMode == 3 && count >= 1) {
            int lowest = outPitches[0];
            for (int i = count; i > 0; --i) outPitches[i] = outPitches[i - 1];
            outPitches[0] = lowest - 12; count++;
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
            if (step.voices[v].isActive) pitches[count++] = { MusicTheory::getBasePitch(step, v) + step.voices[v].accidental + (step.voices[v].octaveShift * 12) + step.shift, v };
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

        for (int i = 0; i < count; ++i) { if (pitches[i].second == voiceIdx) return pitches[i].first; }
        return 60;
    }

    juce::String VoicingEngine::getRecognizedChordName(const std::array<StepData, TotalSteps>& seq, int targetStep, float ppqPerStep) {
        int effS = targetStep;
        bool anyActive = false;

        // 統一された安全なアクティビティ判定
        for (int s = targetStep; s >= 0; --s) {
            float distBeats = static_cast<float>(targetStep - s) * ppqPerStep;
            bool stepCovers = false;
            for (int v = 0; v < NumVoices; ++v) {
                if (seq[s].voices[v].isActive && seq[s].gateLength > distBeats + 0.001f) {
                    stepCovers = true; anyActive = true;
                }
            }
            if (stepCovers) { effS = s; break; }
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
            if (step.voicingMode == 8) return rootName + "7sus4 (Quartal)\n4ths Stack";
            if (step.voicingMode == 9) return rootName + "7 (Shell)\nRoot-3rd-7th";
            if (step.voicingMode == 12) return rootName + "m11 (So What)\n4th-4th-4th-M3";
            if (step.voicingMode == 13) return rootName + " (Cluster)\n2nds";
            if (step.voicingMode == 14) return rootName + "m11 (Kenny B.)\nMin11ths";

            if (step.voicingMode == 4 || step.voicingMode == 5) {
                bool hasChanges = false;
                for (int i = 0; i < 4; ++i) if (step.voices[i].accidental != 0) hasChanges = true;
                juce::String type = (step.chordDegree == 1) ? "m9" : (step.chordDegree == 4) ? "13" : "Maj9";
                juce::String modeName = (step.voicingMode == 4) ? "Type A" : "Type B";
                if (!hasChanges) return rootName + type + " (Rootless)\n" + modeName;

                bool hasRel[12] = { false };
                for (int i = 0; i < activeCount; ++i) hasRel[(((voicedPitches[i] - theoreticalRoot) % 12) + 12) % 12] = true;
                bool isMinor = hasRel[3]; bool isMajor = hasRel[4];
                bool hasDom7 = hasRel[10]; bool hasMaj7 = hasRel[11];

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
        int bestRoot = -1; juce::String bestType = "??"; int bestScore = -1;
        for (int testRoot = 0; testRoot < 12; ++testRoot) {
            if (!has[testRoot]) continue;
            bool rHas[12] = { false };
            for (int i = 0; i < 12; ++i) if (has[i]) rHas[(((i - testRoot) % 12) + 12) % 12] = true;

            juce::String type = "??"; int score = 0;
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
                if (score > bestScore) { bestScore = score; bestType = type; bestRoot = testRoot; }
            }
        }

        if (bestType == "??") { bestRoot = (((lowestRaw % 12) + 12) % 12); bestType = (activeCount == 1) ? "" : "Custom"; }

        juce::String absName = MusicTheory::getNoteName(bestRoot) + bestType;
        int keyRoot = (((step.keyRoot + step.shift) % 12) + 12) % 12;
        int diff = (((bestRoot - keyRoot) % 12) + 12) % 12;
        static const char* const romanNames[] = { "I", "bII", "II", "bIII", "III", "IV", "bV", "V", "bVI", "VI", "bVII", "VII" };
        juce::String relName = juce::String(romanNames[diff]) + bestType;

        int lowestVoicedAbs = (((lowestRaw % 12) + 12) % 12);
        if (lowestVoicedAbs != bestRoot && bestType != "Custom" && bestType != "") {
            juce::String slash = "on" + MusicTheory::getNoteName(lowestVoicedAbs);
            relName += slash; absName += slash;
        }

        if (step.voicingMode == 1) { relName += " (Drop 2)"; absName += " (Drop 2)"; }
        else if (step.voicingMode == 2) { relName += " (Drop 3)"; absName += " (Drop 3)"; }
        else if (step.voicingMode == 10) { relName += " (Drop 2&4)"; absName += " (Drop 2&4)"; }
        else if (step.voicingMode == 11) { relName += " (Drop 2&3)"; absName += " (Drop 2&3)"; }
        else if (step.voicingMode == 15) { relName += " (Block)"; absName += " (Block)"; }
        else if (step.voicingMode == 3) { relName += " (Spread)"; absName += " (Spread)"; }

        return relName + "\n(" + absName + ")";
    }

    void VoicingEngine::optimizeStep(std::array<StepData, TotalSteps>& seq, int targetStep, float ppqPerStep) {
        if (isAutoPattern(seq[targetStep].voicingMode)) return;

        int prevActiveStep = -1;
        for (int s = targetStep - 1; s >= 0; --s) {
            bool hasActive = false;
            for (int v = 0; v < NumVoices; ++v) { if (seq[s].voices[v].isActive) { hasActive = true; break; } }
            if (hasActive) { prevActiveStep = s; break; }
        }

        if (prevActiveStep < 0) return;

        std::vector<int> prevPitches;
        for (int pv = 0; pv < NumVoices; ++pv) {
            if (seq[prevActiveStep].voices[pv].isActive) {
                prevPitches.push_back(MusicTheory::getBasePitch(seq[prevActiveStep], pv) + seq[prevActiveStep].voices[pv].accidental + (seq[prevActiveStep].voices[pv].octaveShift * 12) + seq[prevActiveStep].shift);
            }
        }

        for (int v = 0; v < NumVoices; ++v) {
            if (seq[targetStep].voices[v].isActive) {
                int basePitch = MusicTheory::getBasePitch(seq[targetStep], v) + seq[targetStep].voices[v].accidental + seq[targetStep].shift;
                int bestOct = seq[targetStep].voices[v].octaveShift;
                float lowestCost = 999999.0f;

                for (int oct = -2; oct <= 2; ++oct) {
                    int testPitch = basePitch + oct * 12;
                    float cost = 0.0f;

                    int minDist = 9999;
                    for (int pPitch : prevPitches) {
                        int dist = std::abs(pPitch - testPitch);
                        if (dist < minDist) minDist = dist;
                    }

                    cost += static_cast<float>(minDist);
                    if (minDist == 0) cost -= 3.0f; else if (minDist == 1) cost -= 1.5f; else if (minDist == 2) cost -= 0.5f;

                    if (v == 0) cost -= static_cast<float>(minDist) * 0.5f;
                    else if (minDist > 4) cost += static_cast<float>(minDist - 4) * 2.0f;

                    for (int v_prev = 0; v_prev < v; ++v_prev) {
                        if (seq[targetStep].voices[v_prev].isActive && seq[prevActiveStep].voices[v_prev].isActive && seq[prevActiveStep].voices[v].isActive) {
                            int currP1 = MusicTheory::getBasePitch(seq[targetStep], v_prev) + seq[targetStep].voices[v_prev].accidental + (seq[targetStep].voices[v_prev].octaveShift * 12) + seq[targetStep].shift;
                            int currP2 = testPitch;
                            int prevP1 = MusicTheory::getBasePitch(seq[prevActiveStep], v_prev) + seq[prevActiveStep].voices[v_prev].accidental + (seq[prevActiveStep].voices[v_prev].octaveShift * 12) + seq[prevActiveStep].shift;
                            int pP2 = MusicTheory::getBasePitch(seq[prevActiveStep], v) + seq[prevActiveStep].voices[v].accidental + (seq[prevActiveStep].voices[v].octaveShift * 12) + seq[prevActiveStep].shift;

                            int prevInt = std::abs(pP2 - prevP1) % 12;
                            int currInt = std::abs(currP2 - currP1) % 12;

                            if ((currP1 - prevP1) == (currP2 - pP2) && currP1 != prevP1) {
                                if (prevInt == 7 && currInt == 7) cost += 50.0f;
                                if (prevInt == 0 && currInt == 0) cost += 50.0f;
                            }
                        }
                    }

                    bool isDominantResolution = (seq[prevActiveStep].chordDegree == 4 && seq[targetStep].chordDegree == 0);
                    if (isDominantResolution) {
                        int pP2 = MusicTheory::getBasePitch(seq[prevActiveStep], v) + seq[prevActiveStep].voices[v].accidental + (seq[prevActiveStep].voices[v].octaveShift * 12) + seq[prevActiveStep].shift;
                        int leadingToneClass = (((seq[prevActiveStep].keyRoot + seq[prevActiveStep].shift) % 12) + 11) % 12;

                        if ((((pP2 % 12) + 12) % 12) == leadingToneClass) {
                            if (testPitch == pP2 + 1) cost -= 10.0f;
                            else cost += 20.0f;
                        }
                    }

                    if (testPitch < 36) cost += static_cast<float>(36 - testPitch) * 2.0f;
                    if (testPitch > 84) cost += static_cast<float>(testPitch - 84) * 2.0f;

                    if (cost < lowestCost) { lowestCost = cost; bestOct = oct; }
                }
                seq[targetStep].voices[v].octaveShift = static_cast<int8_t>(bestOct);
            }
        }
    }
}
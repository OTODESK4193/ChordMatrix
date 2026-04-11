#include "VoicingEngine.h"
#include "MusicTheory.h"
#include <algorithm>

namespace ChordMatrix
{
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
            int r = rootPitch;
            int typeA[4] = { r, r, r, r };

            if (step.chordDegree == 1) { typeA[0] = r + 3; typeA[1] = r + 7; typeA[2] = r + 10; typeA[3] = r + 14; }
            else if (step.chordDegree == 4) { typeA[0] = r + 4; typeA[1] = r + 9; typeA[2] = r + 10; typeA[3] = r + 14; }
            else { typeA[0] = r + 4; typeA[1] = r + 7; typeA[2] = r + 11; typeA[3] = r + 14; }

            if (step.voicingMode == 4) {
                for (int i = 0; i < 4; ++i) outPitches[i] = typeA[i] + step.voices[i].accidental;
            }
            else {
                outPitches[0] = typeA[2] - 12 + step.voices[0].accidental;
                outPitches[1] = typeA[3] - 12 + step.voices[1].accidental;
                outPitches[2] = typeA[0] + step.voices[2].accidental;
                outPitches[3] = typeA[1] + step.voices[3].accidental;
            }
            return 4;
        }
        return 0;
    }

    int VoicingEngine::getVoicedPitches(const StepData& step, std::array<int, 7>& outPitches) {
        // =========================================================
        // パターンB: 自動生成 + ミュート除外
        // =========================================================
        if (step.voicingMode >= 4) {
            std::array<int, 7> temp = { 0 };
            int maxV = getPatternBPitches(step, temp);
            int count = 0;
            for (int i = 0; i < maxV; ++i) {
                // -128はミュートフラグとして扱う
                if (step.voices[i].octaveShift != -128) outPitches[count++] = temp[i];
            }
            return count;
        }

        // =========================================================
        // パターンA: 通常ボイシング (Close, Drop, Spread)
        // =========================================================
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

        if (step.voicingMode == 1 && count >= 4) {
            outPitches[count - 2] -= 12;
            std::sort(outPitches.begin(), outPitches.begin() + count);
        }
        else if (step.voicingMode == 2 && count >= 4) {
            outPitches[count - 3] -= 12;
            std::sort(outPitches.begin(), outPitches.begin() + count);
        }
        else if (step.voicingMode == 3 && count >= 1) {
            int lowest = outPitches[0];
            for (int i = count; i > 0; --i) outPitches[i] = outPitches[i - 1];
            outPitches[0] = lowest - 12; // 左手ベース
            count++;
            if (count > 3) outPitches[2] += 12; // 元の1stをさらにオクターブ上げて空間を広げる
            std::sort(outPitches.begin(), outPitches.begin() + count);
        }

        return count;
    }

    int VoicingEngine::getPitchForVoice(const StepData& step, int voiceIdx) {
        if (step.voicingMode >= 4) {
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
        else if (step.voicingMode == 3 && count >= 3) pitches[1].first += 12; // Spreadのシフト対象(インサート前のインデックス1)

        for (int i = 0; i < count; ++i) {
            if (pitches[i].second == voiceIdx) return pitches[i].first;
        }
        return 60;
    }

    juce::String VoicingEngine::getRecognizedChordName(const std::array<StepData, TotalSteps>& seq, int targetStep, float ppqPerStep) {
        int effS = targetStep;
        bool anyActive = false;

        const auto& targetStepData = seq[targetStep];
        if (targetStepData.voicingMode >= 4) {
            anyActive = true; effS = targetStep;
        }
        else {
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
        }

        if (!anyActive) return juce::String("-");

        const auto& step = seq[effS];
        int theoreticalRoot = (MusicTheory::getBasePitch(step, 0) + step.shift) % 12;
        if (theoreticalRoot < 0) theoreticalRoot += 12;

        if (step.voicingMode >= 4 && step.voicingMode <= 7) {
            juce::String rootName = MusicTheory::getNoteName(theoreticalRoot);

            if (step.voicingMode == 6) {
                juce::String ustName = MusicTheory::getNoteName(theoreticalRoot + 1);
                return rootName + "7 (UST bII)\n" + ustName + " / " + rootName + "7";
            }
            if (step.voicingMode == 7) {
                juce::String ustName = MusicTheory::getNoteName(theoreticalRoot + 8);
                return rootName + "7 (UST bVI)\n" + ustName + " / " + rootName + "7";
            }
            if (step.voicingMode == 4 || step.voicingMode == 5) {
                bool hasChanges = false;
                for (int i = 0; i < 4; ++i) if (step.voices[i].accidental != 0 || step.voices[i].octaveShift == -128) hasChanges = true;

                juce::String type = (step.chordDegree == 1) ? "m9" : (step.chordDegree == 4) ? "13" : "Maj9";
                juce::String modeName = (step.voicingMode == 4) ? "Type A" : "Type B";

                if (!hasChanges) {
                    return rootName + type + " (Rootless)\n" + modeName;
                }
                else {
                    std::array<int, 7> vps = { 0 };
                    int count = getVoicedPitches(step, vps);
                    bool hasRel[12] = { false };
                    for (int i = 0; i < count; ++i) hasRel[(vps[i] - theoreticalRoot + 120) % 12] = true;

                    bool isMinor = hasRel[3];
                    bool isMajor = hasRel[4];
                    bool hasDom7 = hasRel[10];
                    bool hasMaj7 = hasRel[11];

                    // 骨格が維持されていればB案（テンション追記）
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
                    // 維持されていなければ下の純粋解析(A案)へフォールスルー
                }
            }
        }

        std::array<int, 7> voicedPitches = { 0 };
        int activeCount = getVoicedPitches(step, voicedPitches);

        bool has[12] = { false };
        for (int i = 0; i < activeCount; ++i) has[(voicedPitches[i] % 12 + 12) % 12] = true;

        bool relHas[12] = { false };
        for (int i = 0; i < 12; ++i) if (has[i]) relHas[(i - theoreticalRoot + 12) % 12] = true;

        juce::String type = "??";
        if (relHas[0] && relHas[4] && relHas[7]) {
            type = "Maj"; if (relHas[10]) type = "7"; if (relHas[11]) type = "Maj7";
        }
        else if (relHas[0] && relHas[3] && relHas[7]) {
            type = "m"; if (relHas[10]) type = "m7"; if (relHas[11]) type = "m(Maj7)";
        }
        else if (relHas[0] && relHas[3] && relHas[6]) {
            type = "dim"; if (relHas[9]) type = "dim7"; if (relHas[10]) type = "m7b5";
        }
        else if (relHas[0] && relHas[4] && relHas[8]) type = "aug";
        else if (relHas[0] && relHas[5] && relHas[7]) type = "sus4";
        else if (relHas[0] && relHas[4] && !relHas[7]) type = "Maj(no5)";
        else if (relHas[0] && relHas[3] && !relHas[7]) type = "m(no5)";
        else if (relHas[0] && !relHas[3] && !relHas[4] && relHas[7]) type = "5";
        else if (relHas[0]) type = "Custom";

        if (type != "??" && type != "Custom") {
            if (relHas[2]) type += "(9)"; if (relHas[5]) type += "(11)"; if (relHas[9]) type += "(13)";
        }
        if (type == "??" || type == "Custom") return type;

        juce::String absName = MusicTheory::getNoteName(theoreticalRoot) + type;
        int keyRoot = (step.keyRoot + step.shift) % 12;
        if (keyRoot < 0) keyRoot += 12;

        int diff = (theoreticalRoot - keyRoot + 12) % 12;
        const char* romanNames[] = { "I", "bII", "II", "bIII", "III", "IV", "bV", "V", "bVI", "VI", "bVII", "VII" };
        juce::String relName = juce::String(romanNames[diff]) + type;

        if (activeCount > 0) {
            int lowestVoicedAbs = voicedPitches[0] % 12;
            if (lowestVoicedAbs != theoreticalRoot) {
                juce::String slash = "on" + MusicTheory::getNoteName(lowestVoicedAbs);
                relName += slash; absName += slash;
            }
        }

        // ★追加: Drop/Spread表記の明示
        if (step.voicingMode == 1) { relName += " (Drop 2)"; absName += " (Drop 2)"; }
        else if (step.voicingMode == 2) { relName += " (Drop 3)"; absName += " (Drop 3)"; }
        else if (step.voicingMode == 3) { relName += " (Spread)"; absName += " (Spread)"; }

        return relName + "\n(" + absName + ")";
    }

    void VoicingEngine::optimizeVoiceLeading(std::array<StepData, TotalSteps>& seq, int totalSteps) {
        int prevActiveStep = -1;
        for (int s = 0; s < totalSteps; ++s) {
            if (seq[s].voicingMode >= 4) continue;

            bool hasActive = false;
            for (int v = 0; v < NumVoices; ++v) {
                if (seq[s].voices[v].isActive) {
                    hasActive = true;
                    if (prevActiveStep >= 0) {
                        int bestDist = 9999;
                        int targetPitch = 60;
                        for (int pv = 0; pv < NumVoices; ++pv) {
                            if (seq[prevActiveStep].voices[pv].isActive) {
                                int pPitch = MusicTheory::getBasePitch(seq[prevActiveStep], pv) + seq[prevActiveStep].voices[pv].accidental + (seq[prevActiveStep].voices[pv].octaveShift * 12);
                                int currentBase = MusicTheory::getBasePitch(seq[s], v);
                                int dist = std::abs(pPitch - currentBase);
                                if (dist < bestDist) { bestDist = dist; targetPitch = pPitch; }
                            }
                        }

                        int basePitch = MusicTheory::getBasePitch(seq[s], v) + seq[s].voices[v].accidental;
                        int bestOct = seq[s].voices[v].octaveShift;
                        int minD = 9999;

                        for (int oct = -2; oct <= 2; ++oct) {
                            int d = std::abs((basePitch + oct * 12) - targetPitch);
                            if (d < minD) { minD = d; bestOct = oct; }
                        }
                        seq[s].voices[v].octaveShift = static_cast<int8_t>(bestOct);
                    }
                }
            }
            if (hasActive) prevActiveStep = s;
        }
    }
}
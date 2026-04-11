#include "VoicingEngine.h"
#include "MusicTheory.h"
#include <algorithm>

namespace ChordMatrix
{
    int VoicingEngine::getVoicedPitches(const StepData& step, std::array<int, 7>& outPitches) {
        int count = 0;
        for (int v = 0; v < NumVoices; ++v) {
            if (step.voices[v].isActive) {
                outPitches[count++] = MusicTheory::getBasePitch(step, v) + step.voices[v].accidental + (step.voices[v].octaveShift * 12);
            }
        }
        if (count == 0) return 0;

        std::sort(outPitches.begin(), outPitches.begin() + count);

        if (step.voicingMode == 3 && count >= 2) {
            int originalRoot = outPitches[0];
            outPitches[0] = originalRoot - 12;
            for (int i = 1; i < count; ++i) outPitches[i] += 12;
            if (count < 7) outPitches[count++] = originalRoot;
        }
        else {
            int inv = step.inversion % count;
            for (int i = 0; i < inv; ++i) outPitches[i] += 12;
            std::sort(outPitches.begin(), outPitches.begin() + count);

            if (step.voicingMode == 1 && count >= 3) {
                outPitches[count - 2] -= 12;
            }
            else if (step.voicingMode == 2 && count >= 4) {
                outPitches[count - 3] -= 12;
            }
        }

        std::sort(outPitches.begin(), outPitches.begin() + count);
        return count;
    }

    juce::String VoicingEngine::getRecognizedChordName(const std::array<StepData, TotalSteps>& seq, int targetStep, float ppqPerStep) {
        int effS = targetStep;
        bool anyActive = false;

        for (int s = targetStep; s >= 0; --s) {
            float distBeats = (targetStep - s) * ppqPerStep;
            bool stepCovers = false;
            for (int v = 0; v < NumVoices; ++v) {
                if (seq[s].voices[v].isActive && seq[s].gateLength > distBeats + 0.001f) {
                    stepCovers = true;
                    anyActive = true;
                }
            }
            if (stepCovers) { effS = s; break; }
        }

        if (!anyActive) return juce::String("-");

        const auto& step = seq[effS];
        bool has[12] = { false };
        int lowestRaw = 9999;

        for (int v = 0; v < NumVoices; ++v) {
            if (step.voices[v].isActive) {
                int p = MusicTheory::getBasePitch(step, v) + step.voices[v].accidental + (step.voices[v].octaveShift * 12);
                if (p < lowestRaw) lowestRaw = p;
                has[(p % 12 + 12) % 12] = true;
            }
        }

        int absRoot = lowestRaw % 12;
        bool relHas[12] = { false };
        for (int i = 0; i < 12; ++i) {
            if (has[i]) relHas[(i - absRoot + 12) % 12] = true;
        }

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

        juce::String absName = MusicTheory::getNoteName(absRoot) + type;
        int keyRoot = (step.keyRoot + step.shift) % 12;
        if (keyRoot < 0) keyRoot += 12;

        int diff = (absRoot - keyRoot + 12) % 12;
        const char* romanNames[] = { "I", "bII", "II", "bIII", "III", "IV", "bV", "V", "bVI", "VI", "bVII", "VII" };
        juce::String relName = juce::String(romanNames[diff]) + type;

        std::array<int, 7> voicedPitches;
        int activeCount = getVoicedPitches(step, voicedPitches);
        if (activeCount > 0) {
            int lowestVoicedAbs = voicedPitches[0] % 12;
            if (lowestVoicedAbs != absRoot) {
                juce::String slash = "on" + MusicTheory::getNoteName(lowestVoicedAbs);
                relName += slash;
                absName += slash;
            }
        }
        return relName + "\n(" + absName + ")";
    }

    void VoicingEngine::optimizeVoiceLeading(std::array<StepData, TotalSteps>& seq, int totalSteps) {
        int prevActiveStep = -1;
        for (int s = 0; s < totalSteps; ++s) {
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
                                if (dist < bestDist) {
                                    bestDist = dist;
                                    targetPitch = pPitch;
                                }
                            }
                        }

                        int basePitch = MusicTheory::getBasePitch(seq[s], v) + seq[s].voices[v].accidental;
                        int bestOct = seq[s].voices[v].octaveShift;
                        int minD = 9999;

                        for (int oct = -2; oct <= 2; ++oct) {
                            int d = std::abs((basePitch + oct * 12) - targetPitch);
                            if (d < minD) {
                                minD = d;
                                bestOct = oct;
                            }
                        }
                        seq[s].voices[v].octaveShift = static_cast<int8_t>(bestOct);
                    }
                }
            }
            if (hasActive) prevActiveStep = s;
        }
    }
}
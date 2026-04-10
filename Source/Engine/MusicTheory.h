// Source/Engine/MusicTheory.h
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <algorithm>
#include "../Data/StepData.h"

namespace ChordMatrix
{
    class MusicTheory
    {
    public:
        static juce::StringArray getDegreeNames()
        {
            juce::StringArray arr;
            const char* names[] = { "I", "II", "III", "IV", "V", "VI", "VII" };
            for (int i = 0; i < 7; ++i) arr.add(juce::String(names[i]));
            return arr;
        }

        static juce::StringArray getScaleNames()
        {
            juce::StringArray arr;
            const char* names[] = {
                "Major (Ionian)", "Natural Minor", "Harmonic Minor", "Melodic Minor",
                "Dorian", "Phrygian", "Lydian", "Mixolydian", "Locrian"
            };
            for (int i = 0; i < 9; ++i) arr.add(juce::String(names[i]));
            return arr;
        }

        static std::array<int, 7> getScaleIntervals(int scaleType)
        {
            int safeType = juce::jlimit(0, 8, scaleType);
            switch (safeType)
            {
            case 0: return { 0, 2, 4, 5, 7, 9, 11 };
            case 1: return { 0, 2, 3, 5, 7, 8, 10 };
            case 2: return { 0, 2, 3, 5, 7, 8, 11 };
            case 3: return { 0, 2, 3, 5, 7, 9, 11 };
            case 4: return { 0, 2, 3, 5, 7, 9, 10 };
            case 5: return { 0, 1, 3, 5, 7, 8, 10 };
            case 6: return { 0, 2, 4, 6, 7, 9, 11 };
            case 7: return { 0, 2, 4, 5, 7, 9, 10 };
            case 8: return { 0, 1, 3, 5, 6, 8, 10 };
            default: return { 0, 2, 4, 5, 7, 9, 11 };
            }
        }

        static juce::String getNoteName(int n)
        {
            static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
            int index = n % 12;
            if (index < 0) index += 12;
            return juce::String(names[juce::jlimit(0, 11, index)]);
        }

        static int getBasePitch(const StepData& step, int voiceIndex)
        {
            auto scale = getScaleIntervals(step.scaleType);
            int baseRoot = 60 + step.keyRoot;
            int degreeIndex = juce::jlimit(0, 6, step.chordDegree);

            int scaleIndex = (degreeIndex + voiceIndex * 2) % 7;
            int octaveOffset = (degreeIndex + voiceIndex * 2) / 7;

            return baseRoot + scale[scaleIndex] + (octaveOffset * 12);
        }

        static int getVoicedPitches(const StepData& step, int voicingMode, std::array<int, 7>& outPitches)
        {
            int count = 0;
            for (int v = 0; v < NumVoices; ++v)
            {
                if (step.voices[v].isActive)
                {
                    outPitches[count++] = getBasePitch(step, v) + step.voices[v].accidental + (step.voices[v].octaveShift * 12);
                }
            }
            if (count == 0) return 0;

            std::sort(outPitches.begin(), outPitches.begin() + count);

            int inv = step.inversion % count;
            for (int i = 0; i < inv; ++i) outPitches[i] += 12;
            std::sort(outPitches.begin(), outPitches.begin() + count);

            if (voicingMode == 1 && count >= 3) {
                outPitches[count - 2] -= 12;
            }
            else if (voicingMode == 2 && count >= 4) {
                outPitches[count - 3] -= 12;
            }
            else if (voicingMode == 3 && count >= 4) {
                outPitches[0] -= 12;
                outPitches[count - 1] += 12;
            }

            std::sort(outPitches.begin(), outPitches.begin() + count);
            return count;
        }

        static juce::String getRecognizedChordName(const std::array<StepData, TotalSteps>& seq, int targetStep, float ppqPerStep, int voicingMode)
        {
            int effS = targetStep;
            bool anyActive = false;

            for (int s = targetStep; s >= 0; --s)
            {
                float distBeats = (targetStep - s) * ppqPerStep;
                bool stepCovers = false;
                for (int v = 0; v < NumVoices; ++v)
                {
                    if (seq[s].voices[v].isActive && seq[s].gateLength > distBeats + 0.001f)
                    {
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

            for (int v = 0; v < NumVoices; ++v)
            {
                if (step.voices[v].isActive)
                {
                    int p = getBasePitch(step, v) + step.voices[v].accidental + (step.voices[v].octaveShift * 12);
                    if (p < lowestRaw) lowestRaw = p;
                    has[(p % 12 + 12) % 12] = true;
                }
            }

            int absRoot = lowestRaw % 12;
            bool relHas[12] = { false };
            for (int i = 0; i < 12; ++i)
            {
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

            juce::String absName = getNoteName(absRoot) + type;
            int keyRoot = step.keyRoot % 12;
            int diff = (absRoot - keyRoot + 12) % 12;
            const char* romanNames[] = { "I", "bII", "II", "bIII", "III", "IV", "bV", "V", "bVI", "VI", "bVII", "VII" };
            juce::String relName = juce::String(romanNames[diff]) + type;

            std::array<int, 7> voicedPitches;
            int activeCount = getVoicedPitches(step, voicingMode, voicedPitches);
            if (activeCount > 0)
            {
                int lowestVoicedAbs = voicedPitches[0] % 12;
                if (lowestVoicedAbs != absRoot)
                {
                    juce::String slash = "on" + getNoteName(lowestVoicedAbs);
                    relName += slash;
                    absName += slash;
                }
            }

            return relName + "\n(" + absName + ")";
        }

        static void optimizeVoiceLeading(std::array<StepData, TotalSteps>& seq, int totalSteps)
        {
            int prevActiveStep = -1;
            for (int s = 0; s < totalSteps; ++s)
            {
                bool hasActive = false;
                for (int v = 0; v < NumVoices; ++v)
                {
                    if (seq[s].voices[v].isActive)
                    {
                        hasActive = true;
                        if (prevActiveStep >= 0)
                        {
                            int bestDist = 9999;
                            int targetPitch = 60;

                            for (int pv = 0; pv < NumVoices; ++pv)
                            {
                                if (seq[prevActiveStep].voices[pv].isActive)
                                {
                                    int pPitch = getBasePitch(seq[prevActiveStep], pv) + seq[prevActiveStep].voices[pv].accidental + (seq[prevActiveStep].voices[pv].octaveShift * 12);
                                    int currentBase = getBasePitch(seq[s], v);
                                    int dist = std::abs(pPitch - currentBase);
                                    if (dist < bestDist)
                                    {
                                        bestDist = dist;
                                        targetPitch = pPitch;
                                    }
                                }
                            }

                            int basePitch = getBasePitch(seq[s], v) + seq[s].voices[v].accidental;
                            int bestOct = seq[s].voices[v].octaveShift;
                            int minD = 9999;

                            for (int oct = -2; oct <= 2; ++oct)
                            {
                                int d = std::abs((basePitch + oct * 12) - targetPitch);
                                if (d < minD)
                                {
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
    };
}
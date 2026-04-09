// Source/Engine/MusicTheory.h
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <cmath>
#include "../Data/StepData.h"

namespace ChordMatrix
{
    class MusicTheory
    {
    public:
        static juce::StringArray getDegreeNames() { return { "I", "bII", "II", "bIII", "III", "IV", "bV", "V", "bVI", "VI", "bVII", "VII" }; }
        static juce::StringArray getChordTypeNames() { return { "Maj", "min", "7", "dim", "aug", "sus4" }; }
        static juce::StringArray getTensionNames() { return { "Triad", "7th", "9th", "11th", "13th" }; }

        static int getDegreeInterval(int degree)
        {
            static constexpr int scale[] = { 0, 2, 4, 5, 7, 9, 11 };
            return scale[juce::jlimit(0, 6, degree)];
        }

        static std::array<int, 7> getChordIntervals(int type, int tension)
        {
            std::array<int, 7> intervals = { 0, 4, 7, 11, 14, 17, 21 };
            if (type == 1)      intervals = { 0, 3, 7, 10, 14, 17, 21 };
            else if (type == 2) intervals = { 0, 4, 7, 10, 14, 17, 21 };
            else if (type == 3) intervals = { 0, 3, 6, 9, 13, 16, 20 };
            return intervals;
        }

        static juce::String getNoteName(int n) {
            static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
            int index = n % 12;
            if (index < 0) index += 12;
            return names[juce::jlimit(0, 11, index)];
        }

        static int getBasePitch(const StepData& step, int voiceIndex) {
            int base = 60 + step.keyRoot + getDegreeInterval(step.chordDegree);
            auto intervals = getChordIntervals(step.chordType, step.tensionType);
            return base + intervals[voiceIndex];
        }

        // --- 要件: Gate長を考慮したルックバック・コード判定エンジン ---
        static juce::String getRecognizedChordName(const std::array<StepData, TotalSteps>& seq, int targetStep, float ppqPerStep) {
            bool has[12] = { false };
            bool anyActive = false;
            int lowestPitch = 9999;
            int effectiveKeyRoot = seq[targetStep].keyRoot; // デフォルトは現在のステップ

            // 過去から現在まで走査し、Gateが伸びて現在鳴り続けている音符をすべて集める
            for (int s = targetStep; s >= 0; --s) {
                float distBeats = (targetStep - s) * ppqPerStep;
                bool stepCovers = false;

                for (int v = 0; v < NumVoices; ++v) {
                    if (seq[s].voices[v].isActive && seq[s].gateLength > distBeats - 0.001f) {
                        anyActive = true;
                        stepCovers = true;
                        int p = getBasePitch(seq[s], v) + seq[s].voices[v].accidental + (seq[s].voices[v].octaveShift * 12);
                        if (p < lowestPitch) lowestPitch = p;

                        int note = p % 12;
                        if (note < 0) note += 12;
                        has[note] = true;
                    }
                }
                // もし過去のStepのGateがここまで伸びていれば、そのStepのKeyRootを相対度数の計算基準とする
                if (stepCovers) {
                    effectiveKeyRoot = seq[s].keyRoot;
                    break;
                }
            }

            if (!anyActive) return "-";

            int absRoot = lowestPitch % 12;
            if (absRoot < 0) absRoot += 12;

            bool relHas[12] = { false };
            for (int i = 0; i < 12; ++i) {
                if (has[i]) {
                    int rel = (i - absRoot) % 12;
                    if (rel < 0) rel += 12;
                    relHas[rel] = true;
                }
            }

            juce::String type = "??";
            if (relHas[0] && relHas[4] && relHas[7]) {
                type = "Maj";
                if (relHas[10]) type = "7";
                if (relHas[11]) type = "Maj7";
                if (relHas[2]) type += "(9)";
            }
            else if (relHas[0] && relHas[3] && relHas[7]) {
                type = "m";
                if (relHas[10]) type = "m7";
                if (relHas[11]) type = "m(Maj7)";
                if (relHas[2]) type += "(9)";
            }
            else if (relHas[0] && relHas[3] && relHas[6]) {
                type = "dim";
                if (relHas[9]) type = "dim7";
                if (relHas[10]) type = "m7b5";
            }
            else if (relHas[0] && relHas[4] && relHas[8]) {
                type = "aug";
            }
            else if (relHas[0] && relHas[5] && relHas[7]) {
                type = "sus4";
            }
            else if (relHas[0] && relHas[4] && !relHas[7]) {
                type = "Maj(no5)";
            }
            else if (relHas[0] && relHas[3] && !relHas[7]) {
                type = "m(no5)";
            }
            else if (relHas[0] && !relHas[3] && !relHas[4] && relHas[7]) {
                type = "5";
            }
            else if (relHas[0]) {
                type = "Custom";
            }

            if (type == "??" || type == "Custom") return type;

            juce::String absName = getNoteName(absRoot) + type;

            int keyRoot = effectiveKeyRoot % 12;
            if (keyRoot < 0) keyRoot += 12;
            int diff = (absRoot - keyRoot) % 12;
            if (diff < 0) diff += 12;

            auto romanNames = getDegreeNames();
            juce::String relName = romanNames[diff] + type;

            return relName + "\n(" + absName + ")";
        }

        static void optimizeVoiceLeading(std::array<StepData, TotalSteps>& seq, int totalSteps) {
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
                                    int pPitch = getBasePitch(seq[prevActiveStep], pv) + seq[prevActiveStep].voices[pv].accidental + (seq[prevActiveStep].voices[pv].octaveShift * 12);
                                    int currentBase = getBasePitch(seq[s], v);
                                    int dist = std::abs(pPitch - currentBase);
                                    if (dist < bestDist) { bestDist = dist; targetPitch = pPitch; }
                                }
                            }
                            int basePitch = getBasePitch(seq[s], v) + seq[s].voices[v].accidental;
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
    };
}
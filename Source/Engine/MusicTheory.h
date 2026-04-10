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
        // Degreeはスケール内の純粋な7音（I 〜 VII）に限定
        static juce::StringArray getDegreeNames() {
            juce::StringArray arr;
            const char* names[] = { "I", "II", "III", "IV", "V", "VI", "VII" };
            for (int i = 0; i < 7; ++i) arr.add(juce::String(names[i]));
            return arr;
        }

        // 音楽的に最も重要な9つのスケールを完全網羅
        static juce::StringArray getScaleNames() {
            juce::StringArray arr;
            const char* names[] = {
                "Major (Ionian)", "Natural Minor", "Harmonic Minor", "Melodic Minor",
                "Dorian", "Phrygian", "Lydian", "Mixolydian", "Locrian"
            };
            for (int i = 0; i < 9; ++i) arr.add(juce::String(names[i]));
            return arr;
        }

        // 選択されたスケールの1オクターブ（7音）のインターバル（半音数）を返す
        static std::array<int, 7> getScaleIntervals(int scaleType)
        {
            int safeType = juce::jlimit(0, 8, scaleType);
            switch (safeType) {
            case 0: return { 0, 2, 4, 5, 7, 9, 11 }; // Major (Ionian)
            case 1: return { 0, 2, 3, 5, 7, 8, 10 }; // Natural Minor (Aeolian)
            case 2: return { 0, 2, 3, 5, 7, 8, 11 }; // Harmonic Minor
            case 3: return { 0, 2, 3, 5, 7, 9, 11 }; // Melodic Minor
            case 4: return { 0, 2, 3, 5, 7, 9, 10 }; // Dorian
            case 5: return { 0, 1, 3, 5, 7, 8, 10 }; // Phrygian
            case 6: return { 0, 2, 4, 6, 7, 9, 11 }; // Lydian
            case 7: return { 0, 2, 4, 5, 7, 9, 10 }; // Mixolydian
            case 8: return { 0, 1, 3, 5, 6, 8, 10 }; // Locrian
            default: return { 0, 2, 4, 5, 7, 9, 11 };
            }
        }

        static juce::String getNoteName(int n) {
            static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
            int index = n % 12;
            if (index < 0) index += 12;
            return juce::String(names[juce::jlimit(0, 11, index)]);
        }

        // 究極のダイアトニック計算アルゴリズム：ScaleとDegreeから各Voice(1,3,5,7,9,11,13)の絶対ピッチを数学的に導出
        static int getBasePitch(const StepData& step, int voiceIndex) {
            auto scale = getScaleIntervals(step.scaleType);
            int baseRoot = 60 + step.keyRoot;
            int degreeIndex = juce::jlimit(0, 6, step.chordDegree);

            // 1個飛ばし（3度積み）でスケール内の音を拾う
            int scaleIndex = (degreeIndex + voiceIndex * 2) % 7;
            // スケールを一周した場合はオクターブ(+12半音)を加算
            int octaveOffset = (degreeIndex + voiceIndex * 2) / 7;

            return baseRoot + scale[scaleIndex] + (octaveOffset * 12);
        }

        // 実際に配置されたノート集合からリアルタイムに和音名を逆算するルックバック解析
        static juce::String getRecognizedChordName(const std::array<StepData, TotalSteps>& seq, int targetStep, float ppqPerStep) {
            bool has[12] = { false };
            bool anyActive = false;
            int lowestPitch = 9999;
            int effectiveKeyRoot = seq[targetStep].keyRoot;
            int effectiveDegree = seq[targetStep].chordDegree;

            for (int s = targetStep; s >= 0; --s) {
                float distBeats = (targetStep - s) * ppqPerStep;
                bool stepCovers = false;

                for (int v = 0; v < NumVoices; ++v) {
                    if (seq[s].voices[v].isActive && seq[s].gateLength > distBeats + 0.001f) {
                        anyActive = true;
                        stepCovers = true;
                        int p = getBasePitch(seq[s], v) + seq[s].voices[v].accidental + (seq[s].voices[v].octaveShift * 12);
                        if (p < lowestPitch) lowestPitch = p;

                        int note = p % 12;
                        if (note < 0) note += 12;
                        has[note] = true;
                    }
                }
                if (stepCovers) {
                    effectiveKeyRoot = seq[s].keyRoot;
                    effectiveDegree = seq[s].chordDegree;
                    break;
                }
            }

            if (!anyActive) return juce::String("-");

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
            }
            else if (relHas[0] && relHas[3] && relHas[7]) {
                type = "m";
                if (relHas[10]) type = "m7";
                if (relHas[11]) type = "m(Maj7)";
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

            if (type != "??" && type != "Custom") {
                if (relHas[2]) type += "(9)";
                if (relHas[5]) type += "(11)";
                if (relHas[9]) type += "(13)";
            }

            if (type == "??" || type == "Custom") return type;

            juce::String absName = getNoteName(absRoot) + type;

            // ルートからの相対的な度数を計算
            int keyRoot = effectiveKeyRoot % 12;
            if (keyRoot < 0) keyRoot += 12;
            int diff = (absRoot - keyRoot) % 12;
            if (diff < 0) diff += 12;

            // ローマ数字の生成 (I, bII, II, bIII...)
            const char* romanNames[] = { "I", "bII", "II", "bIII", "III", "IV", "bV", "V", "bVI", "VI", "bVII", "VII" };
            juce::String relName = juce::String(romanNames[diff]) + type;

            // 度数表記と絶対コード名を2段組みで返す
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
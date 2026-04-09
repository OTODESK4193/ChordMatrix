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
        static juce::StringArray getDegreeNames() { return { "I", "II", "III", "IV", "V", "VI", "VII" }; }
        static juce::StringArray getChordTypeNames() { return { "Maj", "min", "7", "dim", "aug", "sus4" }; }
        static juce::StringArray getTensionNames() { return { "Triad", "7th", "9th", "11th", "13th" }; }

        // 要件④: 狭い幅でも見切れないよう、相対表記と絶対表記の間に改行コード (\n) を挿入
        static juce::String getChordFullName(int keyRoot, int deg, int type, int tens)
        {
            auto degs = getDegreeNames();
            auto typs = getChordTypeNames();
            auto tsns = getTensionNames();

            juce::String d = degs[juce::jlimit(0, 6, deg)];
            juce::String t = (typs[juce::jlimit(0, 5, type)] == "Maj") ? "" : typs[juce::jlimit(0, 5, type)];
            juce::String s = (tsns[juce::jlimit(0, 4, tens)] == "Triad") ? "" : tsns[juce::jlimit(0, 4, tens)];

            juce::String relName = d + t + s;

            int absRoot = (keyRoot + getDegreeInterval(deg)) % 12;
            if (absRoot < 0) absRoot += 12;
            juce::String absName = getNoteName(absRoot) + t + s;

            return relName + "\n(" + absName + ")"; // 改行を挟む
        }

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

        static int getBasePitch(const BeatData& b, int voiceIndex) {
            int base = 60 + b.keyRoot + getDegreeInterval(b.chordDegree);
            auto intervals = getChordIntervals(b.chordType, b.tensionType);
            return base + intervals[voiceIndex];
        }

        static void optimizeVoiceLeading(std::array<StepData, TotalSteps>& seq, const std::array<BeatData, TotalBeats>& beats, int totalSteps)
        {
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
                                    int pPitch = getBasePitch(beats[prevActiveStep / 4], pv) +
                                        seq[prevActiveStep].voices[pv].accidental +
                                        (seq[prevActiveStep].voices[pv].octaveShift * 12);
                                    int currentBase = getBasePitch(beats[s / 4], v);
                                    int dist = std::abs(pPitch - currentBase);
                                    if (dist < bestDist) { bestDist = dist; targetPitch = pPitch; }
                                }
                            }
                            int basePitch = getBasePitch(beats[s / 4], v) + seq[s].voices[v].accidental;
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
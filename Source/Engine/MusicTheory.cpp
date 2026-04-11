#include "MusicTheory.h"

namespace ChordMatrix
{
    juce::StringArray MusicTheory::getDegreeNames() {
        juce::StringArray arr;
        const char* names[] = { "I", "II", "III", "IV", "V", "VI", "VII" };
        for (int i = 0; i < 7; ++i) arr.add(juce::String(names[i]));
        return arr;
    }

    juce::StringArray MusicTheory::getScaleNames() {
        juce::StringArray arr;
        const char* names[] = {
            "Major (Ionian)", "Natural Minor", "Harmonic Minor", "Melodic Minor",
            "Dorian", "Phrygian", "Lydian", "Mixolydian", "Locrian"
        };
        for (int i = 0; i < 9; ++i) arr.add(juce::String(names[i]));
        return arr;
    }

    std::array<int, 7> MusicTheory::getScaleIntervals(int scaleType) {
        int safeType = juce::jlimit(0, 8, scaleType);
        switch (safeType) {
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

    juce::String MusicTheory::getNoteName(int n) {
        static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        int index = n % 12;
        if (index < 0) index += 12;
        return juce::String(names[juce::jlimit(0, 11, index)]);
    }

    int MusicTheory::getBasePitch(const StepData& step, int voiceIndex) {
        auto scale = getScaleIntervals(step.scaleType);
        int baseRoot = 60 + step.keyRoot;
        int degreeIndex = juce::jlimit(0, 6, step.chordDegree);
        int scaleIndex = (degreeIndex + voiceIndex * 2) % 7;
        int octaveOffset = (degreeIndex + voiceIndex * 2) / 7;
        return baseRoot + scale[scaleIndex] + (octaveOffset * 12) + step.shift;
    }
}
#include "MusicTheory.h"

namespace ChordMatrix
{
    juce::String MusicTheory::getNoteName(int pitchClass) {
        const char* names[] = { "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B" };
        return names[(((pitchClass % 12) + 12) % 12)];
    }

    std::vector<juce::String> MusicTheory::getScaleNames() {
        return {
            "Major (Ionian)", "Natural Minor", "Harmonic Minor", "Melodic Minor",
            "Dorian", "Phrygian", "Lydian", "Mixolydian", "Locrian",
            "Harmonic Major", "Altered", "Lydian Dominant", "HW Diminished", "Whole Tone"
        };
    }

    std::vector<int> MusicTheory::getScaleIntervals(int scaleType) {
        switch (scaleType) {
        case 0: return { 0, 2, 4, 5, 7, 9, 11 }; // Major
        case 1: return { 0, 2, 3, 5, 7, 8, 10 }; // Natural Minor
        case 2: return { 0, 2, 3, 5, 7, 8, 11 }; // Harmonic Minor
        case 3: return { 0, 2, 3, 5, 7, 9, 11 }; // Melodic Minor
        case 4: return { 0, 2, 3, 5, 7, 9, 10 }; // Dorian
        case 5: return { 0, 1, 3, 5, 7, 8, 10 }; // Phrygian
        case 6: return { 0, 2, 4, 6, 7, 9, 11 }; // Lydian
        case 7: return { 0, 2, 4, 5, 7, 9, 10 }; // Mixolydian
        case 8: return { 0, 1, 3, 5, 6, 8, 10 }; // Locrian
        case 9: return { 0, 2, 4, 5, 7, 8, 11 }; // Harmonic Major
        case 10: return { 0, 1, 3, 4, 6, 8, 10 }; // Altered
        case 11: return { 0, 2, 4, 6, 7, 9, 10 }; // Lydian Dominant
        case 12: return { 0, 1, 3, 4, 6, 7, 9 };  // HW Diminished
        case 13: return { 0, 2, 4, 6, 8, 10, 12 }; // Whole Tone
        default: return { 0, 2, 4, 5, 7, 9, 11 };
        }
    }

    std::vector<juce::String> MusicTheory::getDegreeNames() {
        return { "I", "II", "III", "IV", "V", "VI", "VII" };
    }

    int MusicTheory::getBasePitch(const StepData& step, int voiceIdx) {
        auto intervals = getScaleIntervals(step.scaleType);
        int rootPitch = 60 + step.keyRoot;

        int offsetDegrees = 0;
        if (voiceIdx == 0) offsetDegrees = 0;
        else if (voiceIdx == 1) offsetDegrees = 2;
        else if (voiceIdx == 2) offsetDegrees = 4;
        else if (voiceIdx == 3) offsetDegrees = 6;
        else if (voiceIdx == 4) offsetDegrees = 8;
        else if (voiceIdx == 5) offsetDegrees = 10;
        else if (voiceIdx == 6) offsetDegrees = 12;

        int totalDegrees = step.chordDegree + offsetDegrees;
        int octaves = totalDegrees / 7;
        int scaleDegree = totalDegrees % 7;

        return rootPitch + intervals[scaleDegree] + (octaves * 12);
    }

    // ★新規追加: UI描画用に現在のKey/Scaleの構成音を取得
    std::array<juce::String, 7> MusicTheory::getScaleNoteNames(int keyRoot, int scaleType) {
        std::array<juce::String, 7> names;
        auto intervals = getScaleIntervals(scaleType);
        for (int i = 0; i < 7; ++i) {
            int pc = (keyRoot + intervals[i]) % 12;
            names[i] = getNoteName(pc);
        }
        return names;
    }
}
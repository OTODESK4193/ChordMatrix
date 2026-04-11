#include "MusicTheory.h"

namespace ChordMatrix
{
    juce::String MusicTheory::getNoteName(int pitchClass) {
        const char* names[] = { "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B" };
        return names[(((pitchClass % 12) + 12) % 12)];
    }

    // ★完全網羅: 55種類のスケールデータベース（DSPセーフティのための静的定義）
    static constexpr std::array<ScaleDefinition, 55> scaleDatabase = { {
            // --- Diatonic / Church Modes (0-6) ---
            {"Major (Ionian)",         7, {0, 2, 4, 5, 7, 9, 11, 0,0,0,0,0}},
            {"Dorian",                 7, {0, 2, 3, 5, 7, 9, 10, 0,0,0,0,0}},
            {"Phrygian",               7, {0, 1, 3, 5, 7, 8, 10, 0,0,0,0,0}},
            {"Lydian",                 7, {0, 2, 4, 6, 7, 9, 11, 0,0,0,0,0}},
            {"Mixolydian",             7, {0, 2, 4, 5, 7, 9, 10, 0,0,0,0,0}},
            {"Natural Minor (Aeolian)",7, {0, 2, 3, 5, 7, 8, 10, 0,0,0,0,0}},
            {"Locrian",                7, {0, 1, 3, 5, 6, 8, 10, 0,0,0,0,0}},

            // --- Melodic Minor Modes (7-13) ---
            {"Melodic Minor (Jazz)",   7, {0, 2, 3, 5, 7, 9, 11, 0,0,0,0,0}},
            {"Dorian b2",              7, {0, 1, 3, 5, 7, 9, 10, 0,0,0,0,0}},
            {"Lydian Augmented",       7, {0, 2, 4, 6, 8, 9, 11, 0,0,0,0,0}},
            {"Lydian Dominant",        7, {0, 2, 4, 6, 7, 9, 10, 0,0,0,0,0}},
            {"Mixolydian b6",          7, {0, 2, 4, 5, 7, 8, 10, 0,0,0,0,0}},
            {"Locrian Natural 2",      7, {0, 2, 3, 5, 6, 8, 10, 0,0,0,0,0}},
            {"Altered (Super Locrian)",7, {0, 1, 3, 4, 6, 8, 10, 0,0,0,0,0}},

            // --- Harmonic Minor Modes (14-20) ---
            {"Harmonic Minor",         7, {0, 2, 3, 5, 7, 8, 11, 0,0,0,0,0}},
            {"Locrian Natural 6",      7, {0, 1, 3, 5, 6, 9, 10, 0,0,0,0,0}},
            {"Ionian #5",              7, {0, 2, 4, 5, 8, 9, 11, 0,0,0,0,0}},
            {"Dorian #4 (Romanian)",   7, {0, 2, 3, 6, 7, 9, 10, 0,0,0,0,0}},
            {"Phrygian Dominant",      7, {0, 1, 4, 5, 7, 8, 10, 0,0,0,0,0}},
            {"Lydian #2",              7, {0, 3, 4, 6, 7, 9, 11, 0,0,0,0,0}},
            {"Ultra Locrian",          7, {0, 1, 3, 4, 6, 8, 9,  0,0,0,0,0}},

            // --- Harmonic Major Modes (21-27) ---
            {"Harmonic Major",         7, {0, 2, 4, 5, 7, 8, 11, 0,0,0,0,0}},
            {"Dorian b5",              7, {0, 2, 3, 5, 6, 9, 10, 0,0,0,0,0}},
            {"Phrygian b4",            7, {0, 1, 3, 4, 7, 8, 10, 0,0,0,0,0}},
            {"Lydian b3",              7, {0, 2, 3, 6, 7, 9, 11, 0,0,0,0,0}},
            {"Mixolydian b2",          7, {0, 1, 4, 5, 7, 9, 10, 0,0,0,0,0}},
            {"Lydian Augmented #2",    7, {0, 3, 4, 6, 8, 9, 11, 0,0,0,0,0}},
            {"Locrian bb7",            7, {0, 1, 3, 5, 6, 8, 9,  0,0,0,0,0}},

            // --- Symmetrical Scales (28-33) ---
            {"Whole Tone",             6, {0, 2, 4, 6, 8, 10, 0,  0,0,0,0,0}},
            {"Diminished (HW)",        8, {0, 1, 3, 4, 6, 7, 9, 10,0,0,0,0}},
            {"Diminished (WH)",        8, {0, 2, 3, 5, 6, 8, 9, 11,0,0,0,0}},
            {"Augmented",              6, {0, 3, 4, 7, 8, 11, 0,  0,0,0,0,0}},
            {"Chromatic",             12, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}},
            {"Tritone Scale",          6, {0, 1, 4, 6, 7, 10, 0,  0,0,0,0,0}},

            // --- Bebop Scales (34-38) ---
            {"Bebop Major",            8, {0, 2, 4, 5, 7, 8, 9, 11, 0,0,0,0}},
            {"Bebop Dominant",         8, {0, 2, 4, 5, 7, 9, 10, 11,0,0,0,0}},
            {"Bebop Dorian",           8, {0, 2, 3, 4, 5, 7, 9, 10, 0,0,0,0}},
            {"Bebop Melodic Minor",    8, {0, 2, 3, 5, 7, 8, 9, 11, 0,0,0,0}},
            {"Bebop Harmonic Minor",   8, {0, 2, 3, 5, 7, 8, 10, 11,0,0,0,0}},

            // --- Pentatonic / Ethnic / World (39-54) ---
            {"Yonanuki Major",         5, {0, 2, 4, 7, 9, 0,0, 0,0,0,0,0}},
            {"Yonanuki Minor",         5, {0, 2, 3, 7, 8, 0,0, 0,0,0,0,0}},
            {"Ryukyu (Okinawa)",       5, {0, 4, 5, 7, 11,0,0, 0,0,0,0,0}},
            {"Miyakobushi (In)",       5, {0, 1, 5, 7, 8, 0,0, 0,0,0,0,0}},
            {"Ritsu",                  5, {0, 2, 5, 7, 9, 0,0, 0,0,0,0,0}},
            {"Iwato",                  5, {0, 1, 5, 6, 10,0,0, 0,0,0,0,0}},
            {"Gong (China)",           5, {0, 2, 4, 7, 9, 0,0, 0,0,0,0,0}},
            {"Yo (Japan)",             5, {0, 2, 5, 7, 9, 0,0, 0,0,0,0,0}},
            {"Hungarian Minor",        7, {0, 2, 3, 6, 7, 8, 11, 0,0,0,0,0}},
            {"Double Harmonic Major",  7, {0, 1, 4, 5, 7, 8, 11, 0,0,0,0,0}},
            {"Neapolitan Major",       7, {0, 1, 3, 5, 7, 9, 11, 0,0,0,0,0}},
            {"Neapolitan Minor",       7, {0, 1, 3, 5, 7, 8, 11, 0,0,0,0,0}},
            {"Pelog",                  5, {0, 1, 3, 7, 8, 0,0, 0,0,0,0,0}},
            {"Hirajoshi",              5, {0, 2, 3, 7, 8, 0,0, 0,0,0,0,0}},
            {"Kumoi",                  5, {0, 1, 5, 7, 8, 0,0, 0,0,0,0,0}},
            {"Insen",                  5, {0, 1, 5, 7, 10,0,0, 0,0,0,0,0}}
        } };

    std::vector<juce::String> MusicTheory::getScaleNames() {
        std::vector<juce::String> names;
        names.reserve(scaleDatabase.size());
        for (const auto& s : scaleDatabase) names.push_back(s.name);
        return names;
    }

    std::vector<juce::String> MusicTheory::getDegreeNames() {
        return { "I", "II", "III", "IV", "V", "VI", "VII" };
    }

    const ScaleDefinition& MusicTheory::getScaleDefinition(int scaleType) {
        int index = juce::jlimit(0, (int)scaleDatabase.size() - 1, scaleType);
        return scaleDatabase[index];
    }

    int MusicTheory::getScaleNoteCount(int scaleType) {
        return getScaleDefinition(scaleType).numNotes;
    }

    // ★修正: 5音/8音スケール等に完全対応。N音ごとにオクターブを繰り上げる数学的処理
    int MusicTheory::getBasePitch(const StepData& step, int voiceIdx) {
        const auto& scaleDef = getScaleDefinition(step.scaleType);
        int numNotes = scaleDef.numNotes;
        int rootPitch = 60 + step.keyRoot; // C4 Base

        // 3度堆積(Tertian)を維持したまま、N音スケール上でインデックスを進める
        int offsetDegrees = voiceIdx * 2;
        int totalDegrees = step.chordDegree + offsetDegrees;

        int octaves = totalDegrees / numNotes;
        int scaleDegree = totalDegrees % numNotes;

        return rootPitch + scaleDef.intervals[scaleDegree] + (octaves * 12);
    }

    std::vector<juce::String> MusicTheory::getScaleNoteNames(int keyRoot, int scaleType) {
        const auto& scaleDef = getScaleDefinition(scaleType);
        std::vector<juce::String> names(scaleDef.numNotes);
        for (int i = 0; i < scaleDef.numNotes; ++i) {
            int pc = (keyRoot + scaleDef.intervals[i]) % 12;
            names[i] = getNoteName(pc);
        }
        return names;
    }

    std::vector<juce::String> MusicTheory::getScaleIntervalNames(int scaleType) {
        const auto& scaleDef = getScaleDefinition(scaleType);
        std::vector<juce::String> names(scaleDef.numNotes);

        // 12音のインターバル名マッピング
        const char* intNames[] = { "R", "b2", "2", "m3", "3", "4", "b5", "5", "b6", "6", "b7", "7" };

        for (int i = 0; i < scaleDef.numNotes; ++i) {
            names[i] = intNames[scaleDef.intervals[i]];
        }
        return names;
    }
}
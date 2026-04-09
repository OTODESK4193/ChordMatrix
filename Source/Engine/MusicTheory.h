#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>

namespace ChordMatrix
{
    class MusicTheory
    {
    public:
        static juce::StringArray getDegreeNames() { return { "I", "II", "III", "IV", "V", "VI", "VII" }; }
        static juce::StringArray getChordTypeNames() { return { "Maj", "min", "7", "dim", "aug", "sus4" }; }
        static juce::StringArray getTensionNames() { return { "Triad", "7th", "9th", "11th", "13th" }; }

        static juce::String getChordFullName(int deg, int type, int tens)
        {
            auto degs = getDegreeNames();
            auto typs = getChordTypeNames();
            auto tsns = getTensionNames();

            juce::String d = degs[juce::jlimit(0, 6, deg)];
            juce::String t = (typs[juce::jlimit(0, 5, type)] == "Maj") ? "" : typs[juce::jlimit(0, 5, type)];
            juce::String s = (tsns[juce::jlimit(0, 4, tens)] == "Triad") ? "" : tsns[juce::jlimit(0, 4, tens)];

            return d + t + s;
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
            return names[juce::jlimit(0, 11, n % 12)];
        }
    };
}
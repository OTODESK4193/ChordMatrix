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

        static juce::String getChordFullName(int degreeIdx, int typeIdx, int tensionIdx)
        {
            auto degs = getDegreeNames();
            auto typs = getChordTypeNames();
            auto tens = getTensionNames();

            juce::String d = degs[juce::jlimit(0, 6, degreeIdx)];
            juce::String t = typs[juce::jlimit(0, 5, typeIdx)];
            juce::String ts = tens[juce::jlimit(0, 4, tensionIdx)];

            // 表記の微調整 (Majorの時は空文字にする、Triadの時は数字を書かない等)
            juce::String typeStr = (t == "Maj") ? "" : t;
            juce::String tensionStr = (ts == "Triad") ? "" : ts;

            return d + typeStr + tensionStr;
        }

        static int getDegreeInterval(int degree)
        {
            static constexpr int scale[] = { 0, 2, 4, 5, 7, 9, 11 };
            return scale[juce::jlimit(0, 6, degree)];
        }

        static std::array<int, 7> getChordIntervals(int type, int tension)
        {
            std::array<int, 7> intervals = { 0, 4, 7, 11, 14, 17, 21 };
            if (type == 1)      intervals = { 0, 3, 7, 10, 14, 17, 21 }; // minor
            else if (type == 2) intervals = { 0, 4, 7, 10, 14, 17, 21 }; // Dominant
            else if (type == 3) intervals = { 0, 3, 6, 9, 13, 16, 20 };  // dim
            else if (type == 4) intervals = { 0, 4, 8, 10, 14, 18, 21 }; // aug
            else if (type == 5) intervals = { 0, 5, 7, 10, 14, 17, 21 }; // sus4
            return intervals;
        }

        static juce::String getNoteName(int n) {
            static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
            return names[juce::jlimit(0, 11, n % 12)];
        }
    };
}
#pragma once
#include <juce_core/juce_core.h>
#include <vector>
#include <array>
#include "../Data/StepData.h"

namespace ChordMatrix
{
    class MusicTheory
    {
    public:
        static juce::String getNoteName(int pitchClass);
        static std::vector<juce::String> getScaleNames();
        static std::vector<int> getScaleIntervals(int scaleType);
        static std::vector<juce::String> getDegreeNames();

        static int getBasePitch(const StepData& step, int voiceIdx);

        // ★新規追加: 現在のKeyとScaleから7つの構成音の音名リストを取得する
        static std::array<juce::String, 7> getScaleNoteNames(int keyRoot, int scaleType);
    };
}
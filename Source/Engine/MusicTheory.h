#pragma once
#include <juce_core/juce_core.h>
#include <vector>
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
    };
}
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include "Data/StepData.h"

namespace ChordMatrix
{
    class MusicTheory
    {
    public:
        static juce::StringArray getDegreeNames();
        static juce::StringArray getScaleNames();
        static std::array<int, 7> getScaleIntervals(int scaleType);
        static juce::String getNoteName(int n);
        static int getBasePitch(const StepData& step, int voiceIndex);
    };
}
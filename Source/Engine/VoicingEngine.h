#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include "../Data/StepData.h"

namespace ChordMatrix
{
    class VoicingEngine
    {
    public:
        static int getVoicedPitches(const StepData& step, std::array<int, 7>& outPitches);
        static juce::String getRecognizedChordName(const std::array<StepData, TotalSteps>& seq, int targetStep, float ppqPerStep);
        static void optimizeVoiceLeading(std::array<StepData, TotalSteps>& seq, int totalSteps);
    };
}
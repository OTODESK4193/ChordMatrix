#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include "../Data/StepData.h"

namespace ChordMatrix
{
    class VoicingEngine
    {
    public:
        static bool isAutoPattern(int voicingMode);

        static int getVoicedPitches(const StepData& step, std::array<int, 7>& outPitches);
        static juce::String getRecognizedChordName(const std::array<StepData, TotalSteps>& seq, int targetStep, float ppqPerStep);
        // ------------------------------------------------------------
        // 変更後 (VoicingEngine.h)
        // ------------------------------------------------------------
        static void optimizeStep(std::array<StepData, TotalSteps>& seq, int targetStep, float ppqPerStep, int altIndex = 0);
        static int getPatternBPitches(const StepData& step, std::array<int, 7>& outPitches);
        static int getPitchForVoice(const StepData& step, int voiceIdx);
    };
}
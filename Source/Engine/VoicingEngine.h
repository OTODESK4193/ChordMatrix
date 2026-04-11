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

        // ★修正: 単一ステップを最適化する関数に変更
        static void optimizeStep(std::array<StepData, TotalSteps>& seq, int targetStep, float ppqPerStep);

        static int getPatternBPitches(const StepData& step, std::array<int, 7>& outPitches);
        static int getPitchForVoice(const StepData& step, int voiceIdx);
    };
}
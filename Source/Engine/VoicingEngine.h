#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include "../Data/StepData.h"

namespace ChordMatrix
{
    class VoicingEngine
    {
    public:
        // 自動生成パターン（Rootless, UST, Quartal等）か、手動配置パターン（Drop, Block等）かを判定
        static bool isAutoPattern(int voicingMode);

        static int getVoicedPitches(const StepData& step, std::array<int, 7>& outPitches);
        static juce::String getRecognizedChordName(const std::array<StepData, TotalSteps>& seq, int targetStep, float ppqPerStep);
        static void optimizeStep(std::array<StepData, TotalSteps>& seq, int targetStep, float ppqPerStep);

        static int getPatternBPitches(const StepData& step, std::array<int, 7>& outPitches);
        static int getPitchForVoice(const StepData& step, int voiceIdx);
    };
}
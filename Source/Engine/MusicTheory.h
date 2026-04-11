#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include "../Data/StepData.h"

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

        // ★追加: 転調の架け橋となるコード進行を計算し、プレビューバッファ(dest)へ書き込む枠組み関数
        static void applyModulation(const std::array<StepData, TotalSteps>& source,
            std::array<StepData, TotalSteps>& dest,
            int targetBar, int targetKey, int targetScale, int method, int stepsPerBar);
    };
}
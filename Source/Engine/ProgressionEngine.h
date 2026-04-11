#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include "../Data/StepData.h"
#include "MusicTheory.h"

namespace ChordMatrix
{
    class ProgressionEngine
    {
    public:
        enum ModulationMethod {
            DirectDominant = 0, // V -> I
            TwoFiveOne = 1,     // ii -> V -> I
            TritoneSub = 2      // SubV7 -> I (裏コード)
        };

        // 拍子やステップ設定に完全対応したシグネチャに変更
        static void applyModulation(const std::array<StepData, TotalSteps>& source,
            std::array<StepData, TotalSteps>& dest,
            int targetBar, int targetKey, int targetScale, int method,
            int stepsPerBar, int stepsPerBeat, float ppqPerStep);
    };
}
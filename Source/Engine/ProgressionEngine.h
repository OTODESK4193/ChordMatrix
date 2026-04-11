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

        // ターゲット小節に向けて、直前の小節に転調用のコード進行を自動生成しプレビューバッファに書き込む
        static void applyModulation(const std::array<StepData, TotalSteps>& source,
            std::array<StepData, TotalSteps>& dest,
            int targetBar, int targetKey, int targetScale, int method, int stepsPerBar);
    };
}
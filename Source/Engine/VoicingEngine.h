#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include "../Data/StepData.h"

namespace ChordMatrix
{
    class VoicingEngine
    {
    public:
        // 構成音の算出 (パターンA/B両対応)
        static int getVoicedPitches(const StepData& step, std::array<int, 7>& outPitches);

        // 音楽理論に基づく高度なコード推論
        static juce::String getRecognizedChordName(const std::array<StepData, TotalSteps>& seq, int targetStep, float ppqPerStep);

        // 多目的コスト関数によるボイスリーディング最適化（ネオ・リーマン理論等）
        static void optimizeStep(std::array<StepData, TotalSteps>& seq, int targetStep, float ppqPerStep);

        // 内部処理・UI描画用ヘルパー
        static int getPatternBPitches(const StepData& step, std::array<int, 7>& outPitches);
        static int getPitchForVoice(const StepData& step, int voiceIdx);
    };
}
#pragma once
#include <juce_core/juce_core.h>
#include <vector>
#include <array>
#include "../Data/StepData.h"

namespace ChordMatrix
{
    // ★新規追加: コンパイル時定数として扱うためのスケール定義構造体
    struct ScaleDefinition {
        const char* name;
        int numNotes;
        std::array<int, 12> intervals;
    };

    class MusicTheory
    {
    public:
        static juce::String getNoteName(int pitchClass);
        static std::vector<juce::String> getScaleNames();
        static std::vector<juce::String> getDegreeNames();

        // ★修正: 動的メモリ確保(std::vector)を排除し、安全に情報を取得するAPI
        static const ScaleDefinition& getScaleDefinition(int scaleType);
        static int getScaleNoteCount(int scaleType);

        static int getBasePitch(const StepData& step, int voiceIdx);

        // UI描画用 (N音スケールの音名リストを取得)
        static std::vector<juce::String> getScaleNoteNames(int keyRoot, int scaleType);
        // UI描画用 (度数表記リストを取得 例: "Root", "b3", "#4")
        static std::vector<juce::String> getScaleIntervalNames(int scaleType);
    };
}
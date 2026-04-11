#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <vector>
#include "../Data/StepData.h"

namespace ChordMatrix
{
    // ★追加: 音楽理論上のプリセット構造体をEngine側で定義
    struct PresetChord {
        int startBeat;       // 0-based
        int lengthBeats;     // 持続拍数
        int chordDegree;     // 0=I, 1=II...
        int voicingMode;     // 推奨ボイシング
        int accRoot = 0;     // 臨時記号（-128でミュート）
        int acc3rd = 0;
        int acc5th = 0;
        int acc7th = 0;
    };

    struct ProgressionPreset {
        juce::String category;
        juce::String name;
        int numBars;
        std::vector<PresetChord> chords;
        juce::String previewText;
    };

    class ProgressionEngine
    {
    public:
        enum ModulationMethod {
            DirectDominant = 0,
            TwoFiveOne = 1,
            TritoneSub = 2
        };

        static void applyModulation(const std::array<StepData, TotalSteps>& source,
            std::array<StepData, TotalSteps>& dest,
            int targetBar, int targetKey, int targetScale, int method,
            int stepsPerBar, int stepsPerBeat, float ppqPerStep);

        // ★追加: 全てのコード進行パターンを保持する辞書
        static const std::vector<ProgressionPreset>& getProgressionDictionary();
    };
}
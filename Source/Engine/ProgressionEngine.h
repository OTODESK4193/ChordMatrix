#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <vector>
#include "../Data/StepData.h"

namespace ChordMatrix
{
    // 音楽理論上のプリセット構造体
    struct PresetChord {
        int startBeat;       // 0-based
        int lengthBeats;     // 持続拍数
        int chordDegree;     // 0=I, 1=II...
        int voicingMode;     // 推奨ボイシング (0:Close, 4:RootlessA, 8:Quartal等)
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
            DirectDominant = 0, // V7 -> I
            TwoFiveOne = 1,     // IIm7 -> V7 -> I
            TritoneSub = 2,     // bII7 -> I (裏コード)
            MinorTwoFive = 3,   // IIm7b5 -> V7alt -> I (マイナー・ツーファイブ)
            Backdoor = 4        // IVm7 -> bVII7 -> I (サブドミナントマイナー終止)
        };

        // 転調・アプローチコードの自動生成
        static void applyModulation(const std::array<StepData, TotalSteps>& source,
            std::array<StepData, TotalSteps>& dest,
            int targetBar, int targetKey, int targetScale, int method,
            int stepsPerBar, int stepsPerBeat, float ppqPerStep);

        // 高度な音楽理論に基づく全コード進行辞書の取得
        static const std::vector<ProgressionPreset>& getProgressionDictionary();
    };
}
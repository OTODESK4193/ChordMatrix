#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <vector>
#include "../Data/StepData.h"

namespace ChordMatrix
{
    struct PresetChord {
        int startBeat;
        int lengthBeats;
        int chordDegree;
        int voicingMode;
        int accRoot = 0;
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

    // ★新規追加: AIコードサジェスション機能のためのデータ構造
    struct ChordSuggestion {
        int targetDegree;       // 提案する次コードのディグリー (0:I, 1:II ...)
        int targetScale;        // 推奨スケール (0:Major, 1:Minor ...)
        int relativeKeyOffset;  // 現在のキーからの相対シフト (-12 to +12)
        float probability;      // 遷移確率の重み付け (0.0 to 1.0)
        juce::String reasoning; // 提案の理論的根拠 (UI表示用)
    };

    class ProgressionEngine
    {
    public:
        enum ModulationMethod {
            DirectDominant = 0,
            TwoFiveOne = 1,
            TritoneSub = 2,
            MinorTwoFive = 3,
            Backdoor = 4,
            PassingDiminished = 5,
            SecondaryDominant = 6,
            DoubleTwoFive = 7,
            ColtraneApproach = 8,
            ExtendedDominant = 9,
            ChromaticApproachUp = 10,
            ChromaticApproachDown = 11,
            DeceptiveCadence = 12,
            ConstantStructure = 13,
            PedalPointApproach = 14,
            NeoRiemannianP = 15,
            NeoRiemannianL = 16,
            NeoRiemannianR = 17
        };

        static void applyModulation(const std::array<StepData, TotalSteps>& source,
            std::array<StepData, TotalSteps>& dest,
            int targetBar, int targetKey, int targetScale, int method,
            int stepsPerBar, int stepsPerBeat, float ppqPerStep);

        static const std::vector<ProgressionPreset>& getProgressionDictionary();
        static juce::StringArray getModulationNames();

        // ★修正: マルコフ連鎖に基づく構造的なサジェスト予測API
        static std::vector<ChordSuggestion> suggestNextChords(int currentDegree, int scaleType);
    };
}
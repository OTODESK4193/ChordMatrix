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

    class ProgressionEngine
    {
    public:
        // 転調・アプローチアルゴリズム（15種類に拡張）
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
            ExtendedDominant = 9,      // V7/V/V -> V7/V -> V7
            ChromaticApproachUp = 10,  // 半音下からのアプローチ
            ChromaticApproachDown = 11,// 半音上からのアプローチ
            DeceptiveCadence = 12,     // 偽終止アプローチ
            ConstantStructure = 13,    // 同形進行（平行移動）
            PedalPointApproach = 14    // ベース保留アプローチ
        };

        static void applyModulation(const std::array<StepData, TotalSteps>& source,
            std::array<StepData, TotalSteps>& dest,
            int targetBar, int targetKey, int targetScale, int method,
            int stepsPerBar, int stepsPerBeat, float ppqPerStep);

        // ジャズ、ポップス、クラシックの100種類以上の進行を網羅した辞書
        static const std::vector<ProgressionPreset>& getProgressionDictionary();
        static juce::StringArray getModulationNames();

        // AIサジェスション機能への布石（機能和声的推論）
        static std::vector<int> suggestNextChords(int currentDegree, int scaleType);
    };
}
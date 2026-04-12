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
        int recommendedScale = 0;
    };

    struct ChordSuggestion {
        int targetDegree;
        int targetScale;
        int shift = 0;
        float probability;
        juce::String reasoning;
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
            NeoRiemannianR = 17,
            PivotModulation = 18 // ★追加: 共通和音による究極の滑らかな転調
        };

        static void applyModulation(const std::array<StepData, TotalSteps>& source,
            std::array<StepData, TotalSteps>& dest,
            int targetBar, int targetKey, int targetScale, int method,
            int stepsPerBar, int stepsPerBeat, float ppqPerStep);

        static const std::vector<ProgressionPreset>& getProgressionDictionary();
        static juce::StringArray getModulationNames();

        static std::vector<ChordSuggestion> suggestNextChords(const std::array<StepData, TotalSteps>& seq, int currentStep, float ppqPerStep);
    };
}
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>

namespace ChordMatrix
{
    struct VoiceState {
        bool isActive = false;
        int8_t octaveShift = 0;
        int8_t accidental = 0; // # / b
    };

    struct StepData {
        std::array<VoiceState, 7> voices;
        uint8_t velocity = 100;
        float gateLength = 0.8f;
    };

    struct BeatData {
        int keyRoot = 0;       // 0:C, 1:C#...
        int chordDegree = 0;   // 0:I, 1:II...
        int chordType = 0;     // 0:Maj, 1:min, 2:7...
        int tensionType = 0;   // 0:None, 1:7, 2:9, 3:11, 4:13
    };

    static constexpr int NumVoices = 7;
    static constexpr int NumStepsPerBar = 16;
    static constexpr int MaxBars = 16;
    static constexpr int TotalSteps = NumStepsPerBar * MaxBars; // 256
    static constexpr int TotalBeats = TotalSteps / 4;           // 64
}
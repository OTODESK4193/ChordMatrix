// Source/Data/StepData.h
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>

namespace ChordMatrix
{
    struct VoiceState
    {
        bool isActive = false;
        int8_t octaveShift = 0;
        int8_t accidental = 0;
    };

    struct StepData
    {
        std::array<VoiceState, 7> voices;
        uint8_t velocity = 100;
        float gateLength = 0.25f;

        int keyRoot = 0;
        int chordDegree = 0;
        int scaleType = 0;
        int inversion = 0;
    };

    static constexpr int NumVoices = 7;
    static constexpr int MaxBars = 16;
    static constexpr int TotalSteps = 1024;
}
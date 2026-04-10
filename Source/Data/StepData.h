// Source/Data/StepData.h
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>

namespace ChordMatrix
{
    struct VoiceState {
        bool isActive = false;
        int8_t octaveShift = 0;
        int8_t accidental = 0;
    };

    struct StepData {
        std::array<VoiceState, 7> voices;
        uint8_t velocity = 100;
        float gateLength = 0.25f;

        // 最小Stepごとに独立したコード情報を保持
        int keyRoot = 0;
        int chordDegree = 0; // 0〜6 (I 〜 VII)
        int scaleType = 0;   // TypeではなくScaleを保持
    };

    static constexpr int NumVoices = 7;
    static constexpr int MaxBars = 16;
    static constexpr int TotalSteps = 1024;
}
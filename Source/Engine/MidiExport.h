#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../Data/StepData.h"

namespace ChordMatrix
{
    class MidiExport
    {
    public:
        // targetBar に -1 を渡すと全小節、0〜15 を渡すとその小節のみを書き出す
        static void exportAndDrag(const std::array<StepData, TotalSteps>& sequenceData, int targetBar, int numBars, int stepsPerBar, float ppqPerStep, juce::DragAndDropContainer* container);
    };
}
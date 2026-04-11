#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "Data/StepData.h"

namespace ChordMatrix
{
    class MidiExport
    {
    public:
        static void exportAndDrag(const std::array<StepData, TotalSteps>& sequenceData, int numBars, int stepsPerBar, float ppqPerStep, juce::DragAndDropContainer* container);
    };
}
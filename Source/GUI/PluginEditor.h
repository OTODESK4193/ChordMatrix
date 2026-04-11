#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../PluginProcessor.h"
#include "HeaderComponent.h"
#include "InspectorComponent.h"
#include "MatrixGridComponent.h"

class ChordMatrixAudioProcessorEditor : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    ChordMatrixAudioProcessorEditor(ChordMatrixAudioProcessor&);
    ~ChordMatrixAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    ChordMatrixAudioProcessor& audioProcessor;

    ChordMatrix::HeaderComponent header;
    ChordMatrix::InspectorComponent inspector;
    ChordMatrix::MatrixGridComponent matrixGrid;

    bool isFollowMode = false; // Follow状態の保持

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChordMatrixAudioProcessorEditor)
};
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "GUI/HeaderComponent.h"
#include "GUI/InspectorComponent.h"
#include "GUI/MatrixGridComponent.h"

class ChordMatrixAudioProcessorEditor : public juce::AudioProcessorEditor, public juce::Timer, public juce::DragAndDropContainer
{
public:
    ChordMatrixAudioProcessorEditor(ChordMatrixAudioProcessor&);
    ~ChordMatrixAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;

private:
    ChordMatrixAudioProcessor& audioProcessor;

    ChordMatrix::HeaderComponent header;
    ChordMatrix::InspectorComponent inspector;
    ChordMatrix::MatrixGridComponent matrixGrid;

    juce::Rectangle<int> progBtnBounds;
    juce::Rectangle<int> allClearBtnBounds;
    juce::Rectangle<int> dragMidiBtnBounds;

    bool isProgressionMode = false;
    bool isDraggingMidi = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChordMatrixAudioProcessorEditor)
};
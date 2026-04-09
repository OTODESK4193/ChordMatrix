#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

class ChordMatrixAudioProcessorEditor : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    ChordMatrixAudioProcessorEditor(ChordMatrixAudioProcessor&);
    ~ChordMatrixAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override; // ← 宣言追加

private:
    ChordMatrixAudioProcessor& audioProcessor;
    juce::Rectangle<float> getCellBounds(int step, int voice);
    juce::Rectangle<float> getBarButtonBounds(int barIndex);
    juce::Rectangle<float> getBeatLaneBounds(int beatIdx, int laneY);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChordMatrixAudioProcessorEditor)
};
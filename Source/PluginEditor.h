// Source/PluginEditor.h
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
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseMove(const juce::MouseEvent& event) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;

private:
    ChordMatrixAudioProcessor& audioProcessor;

    juce::ComboBox timeSigNumMenu, timeSigDenMenu, stepSizeMenu, loopBarsMenu;
    juce::Slider tempoSlider;
    juce::Label timeSigLabel{ "", "Sig:" }, stepSizeLabel{ "", "Step:" }, barsLabel{ "", "Bars:" }, tempoLabel{ "", "BPM:" };

    juce::Rectangle<float> getCellBounds(int step, int voice, int stepsPerBar);
    juce::Rectangle<float> getBarButtonBounds(int barIndex, int numBars);
    juce::Rectangle<float> getBeatLaneBounds(int beatIdx, int y, int stepsPerBar, float ppqPerStep);

    int getStepsPerBar() const;
    float getPpqPerStep() const;

    bool isDraggingGate = false;
    int dragStep = -1;
    float dragStartGate = 0.0f;
    float dragStartX = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChordMatrixAudioProcessorEditor)
};
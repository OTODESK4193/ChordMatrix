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

    // Header Components
    juce::ComboBox timeSigNumMenu, timeSigDenMenu, stepSizeMenu, loopBarsMenu;
    juce::Slider tempoSlider;
    juce::Label timeSigLabel{ "", "SIG:" }, stepSizeLabel{ "", "STEP:" }, barsLabel{ "", "BARS:" }, tempoLabel{ "", "BPM:" };
    juce::Label timeSigSlashLabel{ "", "/" }; // メモリリーク修正済

    // Inspector Components (Left Panel)
    int selectedStep = 0;
    juce::ComboBox stepKeyMenu, stepDegreeMenu, stepTypeMenu, stepTensionMenu;
    juce::Label stepKeyLabel{ "", "KEY" }, stepDegreeLabel{ "", "DEGREE" }, stepTypeLabel{ "", "TYPE" }, stepTensionLabel{ "", "TENSION" };

    void updateTimeSigLimits();
    void updateInspector();
    int getStepsPerBar() const;
    float getPpqPerStep() const;

    juce::Rectangle<float> getCellBounds(int step, int voiceRow, int stepsPerBar);
    juce::Rectangle<float> getStepHeaderBounds(int step, int y, int stepsPerBar);
    juce::Rectangle<float> getBarButtonBounds(int barIndex, int numBars);

    bool isDraggingGate = false;
    int dragStep = -1;
    float dragStartGate = 0.0f;
    float dragStartX = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChordMatrixAudioProcessorEditor)
};
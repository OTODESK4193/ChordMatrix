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
    juce::Label timeSigLabel{ "", "SIG:" }, stepSizeLabel{ "", "STEP:" }, barsLabel{ "", "BARS:" }, tempoLabel{ "", "BPM:" };
    juce::Label timeSigSlashLabel{ "", "/" };

    int selectedStep = 0;
    // Scaleベースに変更
    juce::ComboBox stepKeyMenu, stepScaleMenu, stepDegreeMenu;
    juce::Label stepKeyLabel{ "", "KEY" }, stepScaleLabel{ "", "SCALE" }, stepDegreeLabel{ "", "DEGREE" };

    bool isFollowMode = false;

    // UI大改修: 1STEPを大型正方形(50px)とし、全体の座標もシフト
    const float cellHeight = 50.0f;
    const float stepW = 50.0f;
    const float gridX = 240.0f;
    const float gridY = 160.0f;
    // コードネームの2段組みを余裕で表示するためにヘッダーを60pxに拡大
    const float headerHeight = 60.0f;

    void updateTimeSigLimits();
    void updateInspector();
    int getStepsPerBar() const;
    float getPpqPerStep() const;
    int getEffectiveStep(int targetS) const;

    juce::Rectangle<float> getCellBounds(int step, int voiceRow);
    juce::Rectangle<float> getStepHeaderBounds(int step);
    juce::Rectangle<float> getBarButtonBounds(int barIndex, int numBars, int stepsPerBar);

    bool isDraggingGate = false;
    int dragStep = -1;
    float dragStartGate = 0.0f;
    float dragStartX = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChordMatrixAudioProcessorEditor)
};
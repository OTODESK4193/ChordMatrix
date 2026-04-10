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

    juce::ComboBox stepKeyMenu, stepScaleMenu, stepDegreeMenu, voicingMenu;
    juce::Label stepKeyLabel{ "", "KEY" }, stepScaleLabel{ "", "SCALE" }, stepDegreeLabel{ "", "DEGREE" }, voicingLabel{ "", "VOICING" };

    bool isFollowMode = false;
    bool isProgressionMode = false; // Progressionブラウザ切り替え用フラグ

    int selectedStep = 0;
    int selectedVoice = -1;

    // UI大改修: SEQ全体の幅を固定し、ステップ解像度に応じて動的に拡大・縮小を防ぐ
    const float cellHeight = 50.0f;
    const float seqTotalWidth = 800.0f; // SEQの全体幅を800pxに完全固定
    const float gridX = 300.0f;         // Inspectorとの重なりを回避するため右に大きくシフト
    const float gridY = 220.0f;
    const float headerHeight = 60.0f;

    void updateTimeSigLimits();
    void updateInspector();
    int getStepsPerBar() const;
    float getPpqPerStep() const;
    int getEffectiveStep(int targetS) const;

    juce::Rectangle<float> getCellBounds(int step, int voiceRow, float stepW);
    juce::Rectangle<float> getStepHeaderBounds(int step, float stepW);
    juce::Rectangle<float> getBarButtonBounds(int barIndex);

    bool isDraggingGate = false;
    int dragStep = -1;
    float dragStartGate = 0.0f;
    float dragStartX = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChordMatrixAudioProcessorEditor)
};
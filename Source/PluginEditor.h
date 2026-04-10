// Source/PluginEditor.h
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

// ドラッグ＆ドロップ機能（DRAG MIDI）を使用するため DragAndDropContainer を継承
class ChordMatrixAudioProcessorEditor : public juce::AudioProcessorEditor, public juce::Timer, public juce::DragAndDropContainer
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

    // 縦5行のインスペクター設定（Shiftは数値入力用のSliderを使用）
    juce::ComboBox stepKeyMenu, stepScaleMenu, stepDegreeMenu, voicingMenu;
    juce::Slider stepShiftSlider;
    juce::Label stepKeyLabel{ "", "KEY" }, stepScaleLabel{ "", "SCALE" }, stepDegreeLabel{ "", "DEGREE" }, voicingLabel{ "", "VOICING" }, stepShiftLabel{ "", "SHIFT" };

    bool isFollowMode = false;
    bool isProgressionMode = false;

    int selectedStep = 0;
    int selectedVoice = -1;

    // 各設定専用のエディットスコープ (0:STEP, 1:BAR, 2:ALL)
    int scopeKey = 0;
    int scopeScale = 0;
    int scopeDegree = 0;
    int scopeVoicing = 0;
    int scopeShift = 0;

    const float cellHeight = 50.0f;
    const float seqTotalWidth = 800.0f;
    const float gridX = 380.0f; // 重なりを完全に回避するため右に大きくシフト
    const float gridY = 220.0f;
    const float headerHeight = 60.0f;

    // ボタンのバウンディングボックス
    juce::Rectangle<int> progBtnBounds;
    juce::Rectangle<int> allClearBtnBounds;
    juce::Rectangle<int> dragMidiBtnBounds;

    void updateTimeSigLimits();
    void updateStepSizeMenu();
    void updateInspector();
    int getStepsPerBar() const;
    float getPpqPerStep() const;
    int getEffectiveStep(int targetS) const;
    void exportMidiAndDrag(); // DRAG MIDIのコアロジック

    juce::Rectangle<float> getCellBounds(int step, int voiceRow, float stepW) const;
    juce::Rectangle<float> getStepHeaderBounds(int step, float stepW) const;
    juce::Rectangle<float> getBarButtonBounds(int barIndex) const;

    bool isDraggingGate = false;
    bool isDraggingChord = false;
    bool isDraggingMidi = false; // MIDIエクスポートフラグ
    int dragStep = -1;
    float dragStartGate = 0.0f;
    float dragStartX = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChordMatrixAudioProcessorEditor)
};
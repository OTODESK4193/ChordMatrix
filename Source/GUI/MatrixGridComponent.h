#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../PluginProcessor.h"

namespace ChordMatrix {

    class MatrixGridComponent : public juce::Component, public juce::DragAndDropContainer
    {
    public:
        MatrixGridComponent(ChordMatrixAudioProcessor& p);
        ~MatrixGridComponent() override;

        void paint(juce::Graphics& g) override;
        void resized() override;
        void mouseDown(const juce::MouseEvent& e) override;
        void mouseDrag(const juce::MouseEvent& e) override;
        void mouseMove(const juce::MouseEvent& e) override;
        void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;

        std::function<void(int)> onStepSelected;
        std::function<void()> onRepaintRequest;

        void setProgressionMode(bool isProg);
        int getStepsPerBar() const;
        float getPpqPerStep() const;

    private:
        ChordMatrixAudioProcessor& audioProcessor;

        bool isProgressionMode = false;
        int selectedStep = 0;
        int selectedVoice = -1;

        const float leftMargin = 60.0f;
        const float cellHeight = 50.0f;
        const float headerHeight = 60.0f;
        const float seqTotalWidth = 740.0f;

        bool isDraggingGate = false;
        bool isDraggingChord = false;
        bool isDraggingMidi = false;
        int dragStep = -1;
        float dragStartGate = 0.0f;
        float dragStartX = 0.0f;

        juce::Rectangle<int> progBtnBounds;
        juce::Rectangle<int> allClearBtnBounds;
        juce::Rectangle<int> dragMidiBtnBounds;
        juce::Rectangle<int> modulationBtnBounds;

        bool isModulationPanelOpen = false;
        juce::ComboBox modTargetBarMenu, modKeyMenu, modScaleMenu, modMethodMenu;
        juce::TextButton btnModPreview{ "PREVIEW" }, btnModApply{ "APPLY" }, btnModCancel{ "CANCEL" };
        juce::Label modTitleLabel{ "", "MODULATION ASSISTANT" };

        int getEffectiveStep(int targetS) const;

        juce::Rectangle<float> getCellBounds(int step, int voiceRow, float stepW) const;
        juce::Rectangle<float> getStepHeaderBounds(int step, float stepW) const;
        juce::Rectangle<float> getBarButtonBounds(int barIndex) const;

        void setupModulationPanel();

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MatrixGridComponent)
    };

} // namespace ChordMatrix
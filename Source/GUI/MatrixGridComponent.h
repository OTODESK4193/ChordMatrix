#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include "../PluginProcessor.h"
#include "../Engine/ProgressionEngine.h"

namespace ChordMatrix {

    // UIでリスト表示するためのカテゴリ・ヘルパー
    struct ProgressionCategory {
        juce::String name;
        std::vector<ProgressionPreset> presets;
    };

    class ProgressionBrowserComponent : public juce::Component, public juce::ListBoxModel
    {
    public:
        ProgressionBrowserComponent(ChordMatrixAudioProcessor& p);
        ~ProgressionBrowserComponent() override;

        void paint(juce::Graphics& g) override;
        void resized() override;

        int getNumRows() override;
        void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
        void listBoxItemClicked(int row, const juce::MouseEvent&) override;

        std::function<void()> onApplyPreset;
        std::function<void()> onCancel;

    private:
        ChordMatrixAudioProcessor& audioProcessor;

        juce::ListBox categoryList;
        juce::ListBox presetList;

        juce::TextButton btnApply{ "APPLY TO BAR" };
        juce::TextButton btnCancel{ "CANCEL" };

        std::vector<ProgressionCategory> categories;
        int selectedCategory = 0;
        int selectedPreset = -1;

        void loadPresets();
        void applyPresetToProcessor();

        class CategoryListModel : public juce::ListBoxModel {
            ProgressionBrowserComponent& owner;
        public:
            CategoryListModel(ProgressionBrowserComponent& o) : owner(o) {}
            int getNumRows() override { return (int)owner.categories.size(); }
            void paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool isSelected) override;
            void listBoxItemClicked(int row, const juce::MouseEvent&) override;
        } categoryModel;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProgressionBrowserComponent)
    };

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
        ProgressionBrowserComponent progressionBrowser;

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
        juce::Rectangle<int> modulationBtnBounds;
        juce::Rectangle<int> memoryBtnBounds;
        juce::Rectangle<int> dragMidiBtnBounds;
        juce::Rectangle<int> allClearBtnBounds;

        bool isModulationPanelOpen = false;
        juce::ComboBox modTargetBarMenu, modKeyMenu, modScaleMenu, modMethodMenu;
        juce::TextButton btnModPreview{ "PREVIEW" }, btnModApply{ "APPLY" }, btnModCancel{ "CANCEL" };

        bool isMemoryModeOpen = false;

        int getEffectiveStep(int targetS) const;
        juce::Rectangle<float> getCellBounds(int step, int voiceRow, float stepW) const;
        juce::Rectangle<float> getStepHeaderBounds(int step, float stepW) const;
        juce::Rectangle<float> getBarButtonBounds(int barIndex) const;

        void setupModulationPanel();

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MatrixGridComponent)
    };

} // namespace ChordMatrix
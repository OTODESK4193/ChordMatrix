#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <array>
#include "../PluginProcessor.h"
#include "../Engine/ProgressionEngine.h"

namespace ChordMatrix {

    struct ProgressionCategory {
        juce::String name;
        std::vector<ProgressionPreset> presets;
    };

    class ProgressionBrowserComponent : public juce::Component, public juce::ListBoxModel, public juce::Timer
    {
    public:
        ProgressionBrowserComponent(ChordMatrixAudioProcessor& p);
        ~ProgressionBrowserComponent() override;

        void paint(juce::Graphics& g) override;
        void resized() override;
        void timerCallback() override;

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
        juce::TextButton btnPreview{ "PREVIEW" };

        std::vector<ProgressionCategory> categories;
        int selectedCategory = 0;
        int selectedPreset = -1;

        void loadPresets();
        void applyPresetToProcessor();
        void applyPresetToArray(std::array<StepData, TotalSteps>& destArray);
        void previewPreset();

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

} // namespace ChordMatrix
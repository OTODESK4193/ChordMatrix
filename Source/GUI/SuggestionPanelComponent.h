#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <memory>
#include "../PluginProcessor.h"
#include "../Engine/ProgressionEngine.h"

namespace ChordMatrix {

    // ★新規追加: 右クリック（プレビュー）と左クリック（適用）を区別するカスタムボタン
    class SuggestionButton : public juce::TextButton {
    public:
        SuggestionButton(const juce::String& name) : juce::TextButton(name) {}

        std::function<void()> onLeftClick;
        std::function<void()> onRightClick;

        void clicked(const juce::ModifierKeys& modifiers) override {
            if (modifiers.isPopupMenu()) {
                if (onRightClick) onRightClick(); // 右クリックでプレビュー
            }
            else {
                if (onLeftClick) onLeftClick();   // 左クリックで適用
            }
        }
    };

    class SuggestionPanelComponent : public juce::Component
    {
    public:
        SuggestionPanelComponent(ChordMatrixAudioProcessor& p);
        ~SuggestionPanelComponent() override;

        void paint(juce::Graphics& g) override;
        void resized() override;

        void updateSuggestions(int targetStep, float ppqPerStep, int stepsPerBar);

        std::function<void(int newSelectedStep)> onSuggestionApplied;

    private:
        ChordMatrixAudioProcessor& audioProcessor;

        int currentStep = -1;
        float currentPpqPerStep = 1.0f;
        int currentStepsPerBar = 16;

        std::vector<ChordSuggestion> currentSuggestions;

        // ★修正: カスタムボタンクラスに変更
        juce::OwnedArray<SuggestionButton> suggestionButtons;

        void buildButtons();
        void applySuggestion(const ChordSuggestion& sug);
        void previewSuggestion(const ChordSuggestion& sug);

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SuggestionPanelComponent)
    };

} // namespace ChordMatrix
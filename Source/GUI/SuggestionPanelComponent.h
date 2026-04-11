#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <memory>
#include "../PluginProcessor.h"
#include "../Engine/ProgressionEngine.h"

namespace ChordMatrix {

    class SuggestionPanelComponent : public juce::Component
    {
    public:
        SuggestionPanelComponent(ChordMatrixAudioProcessor& p);
        ~SuggestionPanelComponent() override;

        void paint(juce::Graphics& g) override;
        void resized() override;

        // 選択されたステップに基づきサジェストリストを更新
        void updateSuggestions(int targetStep, float ppqPerStep, int stepsPerBar);

        // サジェストが選択・適用された際、親(MatrixGrid)へ通知するコールバック
        std::function<void(int newSelectedStep)> onSuggestionApplied;

    private:
        ChordMatrixAudioProcessor& audioProcessor;

        int currentStep = -1;
        float currentPpqPerStep = 1.0f;
        int currentStepsPerBar = 16;

        std::vector<ChordSuggestion> currentSuggestions;

        // メッセージスレッドで安全にボタンを再生成して保持
        juce::OwnedArray<juce::TextButton> suggestionButtons;

        void buildButtons();
        void applySuggestion(const ChordSuggestion& sug);

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SuggestionPanelComponent)
    };

} // namespace ChordMatrix
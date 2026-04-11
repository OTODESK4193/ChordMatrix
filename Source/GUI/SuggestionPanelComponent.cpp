#include "SuggestionPanelComponent.h"
#include "../Engine/MusicTheory.h"

namespace ChordMatrix {

    SuggestionPanelComponent::SuggestionPanelComponent(ChordMatrixAudioProcessor& p)
        : audioProcessor(p)
    {
    }

    SuggestionPanelComponent::~SuggestionPanelComponent() {}

    void SuggestionPanelComponent::updateSuggestions(int targetStep, float ppqPerStep, int stepsPerBar) {
        currentStep = targetStep;
        currentPpqPerStep = ppqPerStep;
        currentStepsPerBar = stepsPerBar;

        if (currentStep < 0 || currentStep >= TotalSteps) {
            currentSuggestions.clear();
            buildButtons();
            return;
        }

        const auto& sData = audioProcessor.sequenceData[currentStep];

        // ProgressionEngine の AIサジェストロジックを呼び出し
        currentSuggestions = ProgressionEngine::suggestNextChords(sData.chordDegree, sData.scaleType);

        // 確率(重み)の高い順にソート
        std::sort(currentSuggestions.begin(), currentSuggestions.end(),
            [](const ChordSuggestion& a, const ChordSuggestion& b) {
                return a.probability > b.probability;
            });

        buildButtons();
    }

    void SuggestionPanelComponent::buildButtons() {
        suggestionButtons.clear();

        for (int i = 0; i < (int)currentSuggestions.size(); ++i) {
            const auto& sug = currentSuggestions[i];

            // UI表示用の度数とスケール名の生成
            juce::String degName = MusicTheory::getDegreeNames()[sug.targetDegree];
            juce::String labelText = degName + " (" + sug.reasoning + ")";

            auto* btn = new juce::TextButton(labelText);

            // グループ（確率・意味合い）に応じて色分け
            juce::Colour btnColor = juce::Colours::cyan;
            if (sug.reasoning.contains("Neo-Riemannian")) btnColor = juce::Colours::hotpink;
            else if (sug.reasoning.contains("Deceptive") || sug.reasoning.contains("Sub")) btnColor = juce::Colours::yellow;

            btn->setColour(juce::TextButton::buttonColourId, btnColor.withAlpha(0.2f));
            btn->setColour(juce::TextButton::textColourOffId, btnColor);

            btn->onClick = [this, sug] { applySuggestion(sug); };

            addAndMakeVisible(btn);
            suggestionButtons.add(btn);
        }
        resized();
        repaint();
    }

    void SuggestionPanelComponent::applySuggestion(const ChordSuggestion& sug) {
        if (currentStep < 0) return;

        // デフォルトでは次のステップへ（必要に応じて拍単位にジャンプさせる計算も可能）
        int nextStep = currentStep + 1;

        // 既存のノートと被らないように空きステップを探す簡易ロジック
        bool foundEmpty = false;
        while (nextStep < TotalSteps) {
            bool hasNotes = false;
            for (int v = 0; v < 7; ++v) {
                if (audioProcessor.sequenceData[nextStep].voices[v].isActive) hasNotes = true;
            }
            if (!hasNotes) {
                foundEmpty = true;
                break;
            }
            nextStep++;
        }

        if (foundEmpty && nextStep < TotalSteps) {
            auto& sData = audioProcessor.sequenceData[nextStep];

            // 直前のステップからKeyを引き継ぎつつ、提案されたDegreeとScaleをセット
            sData.keyRoot = audioProcessor.sequenceData[currentStep].keyRoot;
            sData.chordDegree = sug.targetDegree;
            sData.scaleType = sug.targetScale;

            // デフォルトボイシング（Rootless Aなど無難なものをセット）
            sData.voicingMode = 4;
            for (int v = 0; v < 4; ++v) sData.voices[v].isActive = true;

            audioProcessor.previewSequenceData[nextStep] = sData;

            if (onSuggestionApplied) onSuggestionApplied(nextStep);
        }
    }

    void SuggestionPanelComponent::paint(juce::Graphics& g) {
        g.fillAll(juce::Colour(0xff1c1c1c));
        g.setColour(juce::Colour(0xff333333));
        g.drawRect(getLocalBounds(), 1);

        g.setColour(juce::Colours::grey);
        g.setFont(14.0f);
        g.drawText("AI CHORD SUGGESTIONS (Next Move)", 10, 5, 300, 20, juce::Justification::centredLeft);

        if (currentSuggestions.empty()) {
            g.drawText("Select a valid chord step to see suggestions.", 10, 30, 300, 20, juce::Justification::centredLeft);
        }
    }

    void SuggestionPanelComponent::resized() {
        if (suggestionButtons.isEmpty()) return;

        // 全3行でフレックスに配置する簡易グリッド計算
        int startX = 10;
        int startY = 30;
        int btnHeight = 25;
        int spacing = 10;
        int maxRows = 3;

        int currentX = startX;
        int currentY = startY;
        int rowCount = 0;

        for (auto* btn : suggestionButtons) {
            // 文字列長からボタン幅を推測
            int btnWidth = 120 + btn->getButtonText().length() * 5;

            if (currentX + btnWidth > getWidth() - 10) {
                currentX = startX;
                currentY += btnHeight + spacing;
                rowCount++;
                if (rowCount >= maxRows) break; // 3行までに制限
            }

            btn->setBounds(currentX, currentY, btnWidth, btnHeight);
            currentX += btnWidth + spacing;
        }
    }

} // namespace ChordMatrix
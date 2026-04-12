#include "SuggestionPanelComponent.h"
#include "../Engine/MusicTheory.h"
#include "../Engine/VoicingEngine.h"

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

        // ★修正: 対象のステップだけでなく、シーケンス全体を渡してコンテキストを解析させる
        currentSuggestions = ProgressionEngine::suggestNextChords(audioProcessor.sequenceData, currentStep, ppqPerStep);

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

            juce::String degName = MusicTheory::getDegreeNames()[sug.targetDegree];
            juce::String labelText = degName + " (" + sug.reasoning + ")";

            auto* btn = new SuggestionButton(labelText);

            juce::Colour btnColor = juce::Colours::cyan;
            if (sug.reasoning.contains("Neo-Riemannian")) btnColor = juce::Colours::hotpink;
            else if (sug.reasoning.contains("Target") || sug.reasoning.contains("Approach")) btnColor = juce::Colours::yellow;

            btn->setColour(juce::TextButton::buttonColourId, btnColor.withAlpha(0.2f));
            btn->setColour(juce::TextButton::textColourOffId, btnColor);

            // ★修正: 左右のクリックイベントを個別にバインド
            btn->onLeftClick = [this, sug] { applySuggestion(sug); };
            btn->onRightClick = [this, sug] { previewSuggestion(sug); };

            // ツールチップで操作ヒントを表示
            btn->setTooltip("Left Click: Apply & Next Step | Right Click: Preview Chord");

            addAndMakeVisible(btn);
            suggestionButtons.add(btn);
        }
        resized();
        repaint();
    }

    // ★新規追加: 右クリック時の和音プレビュー機能
    void SuggestionPanelComponent::previewSuggestion(const ChordSuggestion& sug) {
        if (currentStep < 0) return;

        // 仮想のステップデータを構築してシミュレーションする
        StepData dummyStep;
        dummyStep.keyRoot = audioProcessor.sequenceData[currentStep].keyRoot;
        dummyStep.scaleType = sug.targetScale;
        dummyStep.chordDegree = sug.targetDegree;
        dummyStep.voicingMode = 4; // Rootless A で最も無難なプレビュー
        dummyStep.shift = audioProcessor.sequenceData[currentStep].shift;

        for (int v = 0; v < 4; ++v) dummyStep.voices[v].isActive = true;

        std::array<int, 7> vps = { 0 };
        int count = VoicingEngine::getVoicedPitches(dummyStep, vps);

        for (int i = 0; i < 7; ++i) {
            audioProcessor.previewNotes[i].store(i < count ? juce::jlimit(0, 127, vps[i]) : -1);
        }
        audioProcessor.triggerPreview.store(true);
    }

    // ★修正: 左クリック時、ステップ幅に合わせて「次へ」進みながら挿入・上書きする連続配置ロジック
    void SuggestionPanelComponent::applySuggestion(const ChordSuggestion& sug) {
        if (currentStep < 0) return;

        // 現在のUI解像度（ステップ幅）の分だけ内部ステップを進める
        int internalStepMultiplier = juce::roundToInt(currentPpqPerStep / 0.25f);
        int nextStep = currentStep + internalStepMultiplier;

        if (nextStep < TotalSteps) {
            auto& sData = audioProcessor.sequenceData[nextStep];

            // ロックされていなければ上書き挿入する
            if (!sData.isLocked) {
                sData.keyRoot = audioProcessor.sequenceData[currentStep].keyRoot;
                sData.chordDegree = sug.targetDegree;
                sData.scaleType = sug.targetScale;

                sData.voicingMode = 4; // デフォルトは Rootless A

                // ★修正: 挿入時の長さをUIのStep設定に完全に準拠させる
                sData.gateLength = currentPpqPerStep;

                for (int v = 0; v < 7; ++v) {
                    sData.voices[v].isActive = false;
                    sData.voices[v].octaveShift = 0;
                    sData.voices[v].accidental = 0;
                }
                for (int v = 0; v < 4; ++v) sData.voices[v].isActive = true;

                // 挿入直後に1度最適化をかけ、直前の和音とボイスリーディングを滑らかに繋ぐ
                VoicingEngine::optimizeStep(audioProcessor.sequenceData, nextStep, 0.25f, 0);

                audioProcessor.previewSequenceData[nextStep] = sData;

                // 適用完了後、UIのフォーカスを新しいステップに移動させる（連続打鍵が可能に）
                if (onSuggestionApplied) onSuggestionApplied(nextStep);
            }
        }
    }

    void SuggestionPanelComponent::paint(juce::Graphics& g) {
        g.fillAll(juce::Colour(0xff1c1c1c));
        g.setColour(juce::Colour(0xff333333));
        g.drawRect(getLocalBounds(), 1);

        g.setColour(juce::Colours::grey);
        g.setFont(14.0f);

        // ヘッダーテキストに操作ヒントを追記
        g.drawText("AI CHORD SUGGESTIONS (L-Click: Insert & Next | R-Click: Preview)",
            10, 5, 500, 20, juce::Justification::centredLeft);

        if (currentSuggestions.empty()) {
            g.drawText("Select a valid chord step to see suggestions.", 10, 30, 300, 20, juce::Justification::centredLeft);
        }
    }

    void SuggestionPanelComponent::resized() {
        if (suggestionButtons.isEmpty()) return;

        int startX = 10;
        int startY = 30;
        int btnHeight = 25;
        int spacing = 10;
        int maxRows = 4;

        int currentX = startX;
        int currentY = startY;
        int rowCount = 0;

        for (auto* btn : suggestionButtons) {
            int btnWidth = 100 + btn->getButtonText().length() * 5;

            if (currentX + btnWidth > getWidth() - 10) {
                currentX = startX;
                currentY += btnHeight + spacing;
                rowCount++;
                if (rowCount >= maxRows) break;
            }

            btn->setBounds(currentX, currentY, btnWidth, btnHeight);
            currentX += btnWidth + spacing;
        }
    }

} // namespace ChordMatrix
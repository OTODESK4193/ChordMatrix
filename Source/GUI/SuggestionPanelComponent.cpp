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

        currentSuggestions = ProgressionEngine::suggestNextChords(audioProcessor.sequenceData, currentStep, ppqPerStep);

        // 生成されたスコアに従ってソート (10枠はProgressionEngine側で保証済み)
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
            // bII 等の独自名称がある場合はスケールと度数から表示を上書き
            if (sug.reasoning.contains("bII")) degName = "bII";
            else if (sug.reasoning.contains("bIII")) degName = "bIII";
            else if (sug.reasoning.contains("bVI")) degName = "bVI";
            else if (sug.reasoning.contains("bVII")) degName = "bVII";

            juce::String labelText = degName + " (" + sug.reasoning + ")";

            auto* btn = new SuggestionButton(labelText);

            juce::Colour btnColor = juce::Colours::cyan; // 王道
            if (sug.reasoning.contains("Neo-Riemannian") || sug.reasoning.contains("Mediant") || sug.reasoning.contains("Neapolitan") || sug.reasoning.contains("Subtonic") || sug.reasoning.contains("Submediant") || sug.reasoning.contains("SD Minor") || sug.reasoning.contains("Dom Minor")) {
                btnColor = juce::Colours::hotpink; // 独創・借用
            }
            else if (sug.reasoning.contains("Target") || sug.reasoning.contains("Approach")) {
                btnColor = juce::Colours::yellow; // ターゲット逆算
            }

            btn->setColour(juce::TextButton::buttonColourId, btnColor.withAlpha(0.2f));
            btn->setColour(juce::TextButton::textColourOffId, btnColor);

            btn->onLeftClick = [this, sug] { applySuggestion(sug); };
            btn->onRightClick = [this, sug] { previewSuggestion(sug); };

            btn->setTooltip("Left Click: Apply | Right Click: Preview Chord");

            addAndMakeVisible(btn);
            suggestionButtons.add(btn);
        }
        resized();
        repaint();
    }

    void SuggestionPanelComponent::previewSuggestion(const ChordSuggestion& sug) {
        if (currentStep < 0) return;

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

    void SuggestionPanelComponent::applySuggestion(const ChordSuggestion& sug) {
        if (currentStep < 0) return;

        // =====================================================================
        // ★スマート挿入ロジック：現在のマスが「空」か「埋まっているか」をチェック
        // =====================================================================
        bool isCurrentEmpty = true;
        for (int v = 0; v < 7; ++v) {
            if (audioProcessor.sequenceData[currentStep].voices[v].isActive) {
                isCurrentEmpty = false;
                break;
            }
        }

        int internalStepMultiplier = juce::roundToInt(currentPpqPerStep / 0.25f);
        int targetStep = currentStep; // デフォルトは「現在選択中のマス」

        if (!isCurrentEmpty) {
            // 埋まっていれば「次のマス」へ進む
            targetStep = currentStep + internalStepMultiplier;

            // ついでに元のコードが長すぎる場合は切り詰める
            float maxAllowedGate = static_cast<float>(internalStepMultiplier) * 0.25f;
            if (audioProcessor.sequenceData[currentStep].gateLength > maxAllowedGate) {
                audioProcessor.sequenceData[currentStep].gateLength = maxAllowedGate;
            }
        }

        if (targetStep < TotalSteps) {
            auto& sData = audioProcessor.sequenceData[targetStep];

            if (!sData.isLocked) {
                // ベースのキーは前のコードから引き継ぐ
                sData.keyRoot = audioProcessor.sequenceData[currentStep].keyRoot;
                sData.chordDegree = sug.targetDegree;
                sData.scaleType = sug.targetScale;

                sData.voicingMode = 4; // デフォルトは Rootless A
                sData.gateLength = currentPpqPerStep;

                for (int v = 0; v < 7; ++v) {
                    sData.voices[v].isActive = false;
                    sData.voices[v].octaveShift = 0;
                    sData.voices[v].accidental = 0;
                }
                for (int v = 0; v < 4; ++v) sData.voices[v].isActive = true;

                // 新しく挿入したコードの長さに被っている未来のゴミデータを消去
                for (int i = 1; i < internalStepMultiplier; ++i) {
                    if (targetStep + i < TotalSteps && !audioProcessor.sequenceData[targetStep + i].isLocked) {
                        audioProcessor.sequenceData[targetStep + i] = {};
                    }
                }

                // ボイスリーディング最適化
                VoicingEngine::optimizeStep(audioProcessor.sequenceData, targetStep, 0.25f, 0);

                audioProcessor.previewSequenceData[targetStep] = sData;

                // 適用完了後、UIのフォーカスを新しいステップに移動させる
                if (onSuggestionApplied) onSuggestionApplied(targetStep);
            }
        }
    }

    void SuggestionPanelComponent::paint(juce::Graphics& g) {
        g.fillAll(juce::Colour(0xff1c1c1c));
        g.setColour(juce::Colour(0xff333333));
        g.drawRect(getLocalBounds(), 1);

        g.setColour(juce::Colours::grey);
        g.setFont(14.0f);

        g.drawText("AI CHORD SUGGESTIONS (L-Click: Insert | R-Click: Preview)",
            10, 5, 500, 20, juce::Justification::centredLeft);

        if (currentSuggestions.empty()) {
            g.drawText("Select a valid chord step to see suggestions.", 10, 30, 300, 20, juce::Justification::centredLeft);
        }
    }

    void SuggestionPanelComponent::resized() {
        if (suggestionButtons.isEmpty()) return;

        // =====================================================================
        // ★4行表示への最適化：ボタンの縦幅と余白を詰めて最大10〜12個を画面内に収める
        // =====================================================================
        int startX = 10;
        int startY = 30;
        int btnHeight = 22; // 以前の25pxから少しスリム化
        int spacingX = 8;
        int spacingY = 6;   // 縦の余白も少し詰める
        int maxRows = 4;

        int currentX = startX;
        int currentY = startY;
        int rowCount = 0;

        for (auto* btn : suggestionButtons) {
            int btnWidth = 90 + btn->getButtonText().length() * 4;

            // 右端にぶつかったら改行
            if (currentX + btnWidth > getWidth() - 10) {
                currentX = startX;
                currentY += btnHeight + spacingY;
                rowCount++;
                if (rowCount >= maxRows) break; // 4行を超えたら描画ストップ
            }

            btn->setBounds(currentX, currentY, btnWidth, btnHeight);
            currentX += btnWidth + spacingX;
        }
    }

} // namespace ChordMatrix
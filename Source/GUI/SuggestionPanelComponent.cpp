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

        // ★スマート挿入ロジック：現在地が空ならそのまま、埋まっていれば次へ
        bool isCurrentEmpty = true;
        for (int v = 0; v < 7; ++v) {
            if (audioProcessor.sequenceData[currentStep].voices[v].isActive) {
                isCurrentEmpty = false; break;
            }
        }
        int insertionStep = currentStep;
        if (!isCurrentEmpty) {
            int internalStepMultiplier = juce::roundToInt(currentPpqPerStep / 0.25f);
            insertionStep = currentStep + internalStepMultiplier;
        }

        currentSuggestions = ProgressionEngine::suggestNextChords(audioProcessor.sequenceData, insertionStep, ppqPerStep);
        buildButtons();
    }

    void SuggestionPanelComponent::buildButtons() {
        suggestionButtons.clear();

        for (int i = 0; i < (int)currentSuggestions.size(); ++i) {
            const auto& sug = currentSuggestions[i];

            juce::String degName = MusicTheory::getDegreeNames()[sug.targetDegree];
            // bII や bVI 等の独自名称がある場合はスケールと度数から表示を上書き
            if (sug.reasoning.contains("bII")) degName = "bII";
            else if (sug.reasoning.contains("bIII")) degName = "bIII";
            else if (sug.reasoning.contains("bVI")) degName = "bVI";
            else if (sug.reasoning.contains("bVII")) degName = "bVII";

            juce::String labelText = degName + " (" + sug.reasoning + ")";

            auto* btn = new SuggestionButton(labelText);

            juce::Colour btnColor = juce::Colours::cyan;
            if (sug.reasoning.contains("Neapolitan") || sug.reasoning.contains("Mediant") || sug.reasoning.contains("Subtonic") || sug.reasoning.contains("Submediant") || sug.reasoning.contains("Neo-Riemannian")) {
                btnColor = juce::Colours::hotpink;
            }
            else if (sug.reasoning.contains("Target") || sug.reasoning.contains("Proactive")) {
                btnColor = juce::Colours::yellow;
            }

            btn->setColour(juce::TextButton::buttonColourId, btnColor.withAlpha(0.2f));
            btn->setColour(juce::TextButton::textColourOffId, btnColor);

            btn->onLeftClick = [this, sug] { applySuggestion(sug); };
            btn->onRightClick = [this, sug] { previewSuggestion(sug); };

            btn->setTooltip("L-Click: Insert | R-Click: Preview Chord");

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
        dummyStep.voicingMode = 4; // Rootless A でプレビュー
        dummyStep.shift = sug.shift; // ★バグ修正: 臨時記号(shift)を反映

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

        bool isCurrentEmpty = true;
        for (int v = 0; v < 7; ++v) {
            if (audioProcessor.sequenceData[currentStep].voices[v].isActive) {
                isCurrentEmpty = false; break;
            }
        }

        int internalStepMultiplier = juce::roundToInt(currentPpqPerStep / 0.25f);
        int targetStep = currentStep;

        if (!isCurrentEmpty) {
            targetStep = currentStep + internalStepMultiplier;
            float maxAllowedGate = static_cast<float>(internalStepMultiplier) * 0.25f;
            if (audioProcessor.sequenceData[currentStep].gateLength > maxAllowedGate) {
                audioProcessor.sequenceData[currentStep].gateLength = maxAllowedGate;
            }
        }

        if (targetStep < TotalSteps) {
            auto& sData = audioProcessor.sequenceData[targetStep];

            if (!sData.isLocked) {
                sData.keyRoot = audioProcessor.sequenceData[currentStep].keyRoot;
                sData.chordDegree = sug.targetDegree;
                sData.scaleType = sug.targetScale;
                sData.shift = sug.shift; // ★バグ修正: 臨時記号(shift)を反映

                sData.voicingMode = 4;
                sData.gateLength = currentPpqPerStep;

                for (int v = 0; v < 7; ++v) {
                    sData.voices[v].isActive = false;
                    sData.voices[v].octaveShift = 0;
                    sData.voices[v].accidental = 0;
                }
                for (int v = 0; v < 4; ++v) sData.voices[v].isActive = true;

                for (int i = 1; i < internalStepMultiplier; ++i) {
                    if (targetStep + i < TotalSteps && !audioProcessor.sequenceData[targetStep + i].isLocked) {
                        audioProcessor.sequenceData[targetStep + i] = {};
                    }
                }

                VoicingEngine::optimizeStep(audioProcessor.sequenceData, targetStep, 0.25f, 0);

                audioProcessor.previewSequenceData[targetStep] = sData;

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

        // ★UI最適化：ボタンの縦幅と余白を詰めて最大10〜12個を画面内に収める
        int startX = 10;
        int startY = 30;
        int btnHeight = 22; // 25pxから少しスリム化
        int spacingX = 8;
        int spacingY = 6;
        int maxRows = 4;

        int currentX = startX;
        int currentY = startY;
        int rowCount = 0;

        for (auto* btn : suggestionButtons) {
            int btnWidth = 105 + btn->getButtonText().length() * 4;

            if (currentX + btnWidth > getWidth() - 10) {
                currentX = startX;
                currentY += btnHeight + spacingY;
                rowCount++;
                if (rowCount >= maxRows) break;
            }

            btn->setBounds(currentX, currentY, btnWidth, btnHeight);
            currentX += btnWidth + spacingX;
        }
    }

} // namespace ChordMatrix
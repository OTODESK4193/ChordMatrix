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

            btn->onLeftClick = [this, sug] { applySuggestion(sug); };
            btn->onRightClick = [this, sug] { previewSuggestion(sug); };

            btn->setTooltip("Left Click: Apply & Next Step | Right Click: Preview Chord");

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

        int internalStepMultiplier = juce::roundToInt(currentPpqPerStep / 0.25f);
        int nextStep = currentStep + internalStepMultiplier;

        // =====================================================================
        // ★修正1: 現在のコードが「次に挿入する場所」に被る場合、長さを切り詰める
        // =====================================================================
        float maxAllowedGate = static_cast<float>(internalStepMultiplier) * 0.25f;
        if (audioProcessor.sequenceData[currentStep].gateLength > maxAllowedGate) {
            audioProcessor.sequenceData[currentStep].gateLength = maxAllowedGate;
        }

        if (nextStep < TotalSteps) {
            auto& sData = audioProcessor.sequenceData[nextStep];

            if (!sData.isLocked) {
                sData.keyRoot = audioProcessor.sequenceData[currentStep].keyRoot;
                sData.chordDegree = sug.targetDegree;
                sData.scaleType = sug.targetScale;

                sData.voicingMode = 4;
                sData.gateLength = currentPpqPerStep;

                for (int v = 0; v < 7; ++v) {
                    sData.voices[v].isActive = false;
                    sData.voices[v].octaveShift = 0;
                    sData.voices[v].accidental = 0;
                }
                for (int v = 0; v < 4; ++v) sData.voices[v].isActive = true;

                // =====================================================================
                // ★修正2: 新しく挿入したコードの長さに被っている未来のゴミデータを消去
                // =====================================================================
                for (int i = 1; i < internalStepMultiplier; ++i) {
                    if (nextStep + i < TotalSteps && !audioProcessor.sequenceData[nextStep + i].isLocked) {
                        audioProcessor.sequenceData[nextStep + i] = {};
                    }
                }

                VoicingEngine::optimizeStep(audioProcessor.sequenceData, nextStep, 0.25f, 0);
                audioProcessor.previewSequenceData[nextStep] = sData;

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
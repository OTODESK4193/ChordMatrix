#include "MatrixGridComponent.h"
#include "../Engine/VoicingEngine.h"
#include "../Engine/MidiExport.h"
#include "../Engine/ProgressionEngine.h"

namespace ChordMatrix {

    MatrixGridComponent::MatrixGridComponent(ChordMatrixAudioProcessor& p)
        : audioProcessor(p), progressionBrowser(p), suggestionPanel(p)
    {
        progBtnBounds = juce::Rectangle<int>(leftMargin, 15, 110, 30);
        modulationBtnBounds = juce::Rectangle<int>(leftMargin + 120, 15, 110, 30);
        memoryBtnBounds = juce::Rectangle<int>(leftMargin + 240, 15, 80, 30);
        dragMidiBtnBounds = juce::Rectangle<int>(leftMargin + 330, 15, 100, 30);
        allClearBtnBounds = juce::Rectangle<int>(leftMargin + 440, 15, 100, 30);

        setupModulationPanel();

        addAndMakeVisible(progressionBrowser);
        progressionBrowser.onApplyPreset = [this]() {
            isProgressionMode = false;
            progressionBrowser.setVisible(false);
            if (onRepaintRequest) onRepaintRequest();
            };
        progressionBrowser.onCancel = [this]() {
            isProgressionMode = false;
            progressionBrowser.setVisible(false);
            repaint();
            };
        progressionBrowser.setVisible(false);

        addAndMakeVisible(suggestionPanel);
        suggestionPanel.onSuggestionApplied = [this](int newSelectedStep) {
            if (onStepSelected) onStepSelected(newSelectedStep);
            if (onRepaintRequest) onRepaintRequest();

            int tsNum = (int)*audioProcessor.apvts.getRawParameterValue("timeSigNum");
            int tsDenIdx = (int)*audioProcessor.apvts.getRawParameterValue("timeSigDen");
            int tsDen = (tsDenIdx == 0) ? 4 : (tsDenIdx == 1) ? 8 : 16;
            int internalStepsPerBar = juce::roundToInt((static_cast<float>(tsNum) * (4.0f / static_cast<float>(tsDen))) / 0.25f);

            suggestionPanel.updateSuggestions(newSelectedStep, 0.25f, std::max(1, internalStepsPerBar));
            };
    }

    MatrixGridComponent::~MatrixGridComponent() {}

    void MatrixGridComponent::setupModulationPanel() {
        addAndMakeVisible(modTargetBarMenu);
        addAndMakeVisible(modKeyMenu);
        addAndMakeVisible(modScaleMenu);
        addAndMakeVisible(modMethodMenu);
        addAndMakeVisible(btnModPreview);
        addAndMakeVisible(btnModApply);
        addAndMakeVisible(btnModCancel);

        for (int i = 1; i <= 16; ++i) {
            modTargetBarMenu.addItem("Bar " + juce::String(i), i);
        }
        for (int i = 0; i < 12; ++i) {
            modKeyMenu.addItem(MusicTheory::getNoteName(i), i + 1);
        }
        auto scales = MusicTheory::getScaleNames();
        for (int i = 0; i < (int)scales.size(); ++i) {
            modScaleMenu.addItem(scales[i], i + 1);
        }

        auto modNames = ProgressionEngine::getModulationNames();
        for (int i = 0; i < (int)modNames.size(); ++i) {
            modMethodMenu.addItem(modNames[i], i + 1);
        }

        modTargetBarMenu.setSelectedId(5, juce::dontSendNotification);
        modKeyMenu.setSelectedId(4, juce::dontSendNotification);
        modScaleMenu.setSelectedId(2, juce::dontSendNotification);
        modMethodMenu.setSelectedId(2, juce::dontSendNotification);

        auto setBtnStyle = [](juce::TextButton& btn, juce::Colour c) {
            btn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2a2a2a));
            btn.setColour(juce::TextButton::textColourOffId, c);
            };

        setBtnStyle(btnModPreview, juce::Colours::cyan);
        setBtnStyle(btnModApply, juce::Colours::hotpink);
        setBtnStyle(btnModCancel, juce::Colours::grey);

        btnModPreview.onClick = [this] {
            int targetBar = modTargetBarMenu.getSelectedId() - 1;
            int targetKey = modKeyMenu.getSelectedId() - 1;
            int targetScale = modScaleMenu.getSelectedId() - 1;
            int methodIdx = modMethodMenu.getSelectedId() - 1;

            int tsDen = (int)*audioProcessor.apvts.getRawParameterValue("timeSigDen");
            float beatLengthPpq = (tsDen == 0) ? 1.0f : (tsDen == 1) ? 0.5f : 0.25f;

            // ★警告(C26451)抑止のためキャストを強化
            int tsNum = (int)*audioProcessor.apvts.getRawParameterValue("timeSigNum");
            int internalStepsPerBar = juce::roundToInt(beatLengthPpq * static_cast<float>(tsNum) / 0.25f);
            int internalStepsPerBeat = std::max(1, juce::roundToInt(beatLengthPpq / 0.25f));

            ProgressionEngine::applyModulation(audioProcessor.sequenceData, audioProcessor.previewSequenceData, targetBar, targetKey, targetScale, methodIdx, internalStepsPerBar, internalStepsPerBeat, 0.25f);

            audioProcessor.isPlayingModulationPreview.store(true);
            audioProcessor.internalPPQ = static_cast<double>(std::max(0, targetBar - 1)) * static_cast<double>(internalStepsPerBar) * 0.25;
            audioProcessor.isInternalPlaying = true;

            suggestionPanel.updateSuggestions(selectedStep, 0.25f, internalStepsPerBar);
            repaint();
            };

        btnModApply.onClick = [this] {
            audioProcessor.sequenceData = audioProcessor.previewSequenceData;
            audioProcessor.isPlayingModulationPreview.store(false);
            audioProcessor.isInternalPlaying = false;
            isModulationPanelOpen = false;
            resized();

            int tsDen = (int)*audioProcessor.apvts.getRawParameterValue("timeSigDen");
            float beatLengthPpq = (tsDen == 0) ? 1.0f : (tsDen == 1) ? 0.5f : 0.25f;
            int tsNum = (int)*audioProcessor.apvts.getRawParameterValue("timeSigNum");
            int internalStepsPerBar = juce::roundToInt(beatLengthPpq * static_cast<float>(tsNum) / 0.25f);

            suggestionPanel.updateSuggestions(selectedStep, 0.25f, internalStepsPerBar);
            if (onRepaintRequest) onRepaintRequest();
            };

        btnModCancel.onClick = [this] {
            audioProcessor.isPlayingModulationPreview.store(false);
            audioProcessor.isInternalPlaying = false;
            isModulationPanelOpen = false;
            resized();
            repaint();
            };

        modTargetBarMenu.setVisible(false);
        modKeyMenu.setVisible(false);
        modScaleMenu.setVisible(false);
        modMethodMenu.setVisible(false);
        btnModPreview.setVisible(false);
        btnModApply.setVisible(false);
        btnModCancel.setVisible(false);
    }

    void MatrixGridComponent::setProgressionMode(bool isProg) {
        isProgressionMode = isProg;
        progressionBrowser.setVisible(isProgressionMode);
        repaint();
    }

    int MatrixGridComponent::getStepsPerBar() const {
        int tsNum = (int)*audioProcessor.apvts.getRawParameterValue("timeSigNum");
        int tsDenIdx = (int)*audioProcessor.apvts.getRawParameterValue("timeSigDen");
        int tsDen = (tsDenIdx == 0) ? 4 : (tsDenIdx == 1) ? 8 : 16;
        return std::max(1, juce::roundToInt((static_cast<float>(tsNum) * (4.0f / static_cast<float>(tsDen))) / getPpqPerStep()));
    }

    float MatrixGridComponent::getPpqPerStep() const {
        int stepSizeIdx = (int)*audioProcessor.apvts.getRawParameterValue("stepSize");
        return (stepSizeIdx == 0) ? 1.0f : (stepSizeIdx == 1) ? 0.5f : 0.25f;
    }

    int MatrixGridComponent::getInternalStep(int bar, int uiStep) const {
        int tsNum = (int)*audioProcessor.apvts.getRawParameterValue("timeSigNum");
        int tsDenIdx = (int)*audioProcessor.apvts.getRawParameterValue("timeSigDen");
        int tsDen = (tsDenIdx == 0) ? 4 : (tsDenIdx == 1) ? 8 : 16;
        float beatsPerBar = static_cast<float>(tsNum) * (4.0f / static_cast<float>(tsDen));

        int internalStepsPerBar = juce::roundToInt(beatsPerBar / 0.25f);
        int internalStepMultiplier = juce::roundToInt(getPpqPerStep() / 0.25f);

        // ★警告(C26451)抑止のためキャストを強化
        return static_cast<int>(static_cast<int64_t>(bar) * internalStepsPerBar) + (uiStep * internalStepMultiplier);
    }

    int MatrixGridComponent::getEffectiveStep(int targetInternalS) const {
        int eff = targetInternalS;
        for (int prevS = targetInternalS; prevS >= 0; --prevS) {
            float dist = static_cast<float>(targetInternalS - prevS) * 0.25f;
            const auto& sData = audioProcessor.isPlayingModulationPreview.load() ? audioProcessor.previewSequenceData[prevS] : audioProcessor.sequenceData[prevS];

            bool hasNotes = false;
            for (int v = 0; v < 7; ++v) {
                if (sData.voices[v].isActive) {
                    hasNotes = true;
                    break;
                }
            }

            if (hasNotes && sData.gateLength > dist + 0.001f) {
                eff = prevS;
                break;
            }
        }
        return eff;
    }

    juce::Rectangle<float> MatrixGridComponent::getCellBounds(int s, int v, float stepW) const {
        return { leftMargin + static_cast<float>(s) * stepW, 155.0f + static_cast<float>(v) * cellHeight, stepW, cellHeight - 2.0f };
    }

    juce::Rectangle<float> MatrixGridComponent::getStepHeaderBounds(int s, float stepW) const {
        return { leftMargin + static_cast<float>(s) * stepW, 155.0f - headerHeight - 35.0f, stepW, headerHeight - 5.0f };
    }

    juce::Rectangle<float> MatrixGridComponent::getBarButtonBounds(int i) const {
        float w = seqTotalWidth / 8.0f;
        return { leftMargin + static_cast<float>(i % 8) * w, 555.0f + static_cast<float>(i / 8) * 35.0f, w - 8.0f, 30.0f };
    }

    void MatrixGridComponent::resized() {
        bool showMod = isModulationPanelOpen && !isMemoryModeOpen && !isProgressionMode;

        if (showMod) {
            int py = 675;
            modTargetBarMenu.setBounds(static_cast<int>(leftMargin) + 20, py, 85, 30);
            modKeyMenu.setBounds(static_cast<int>(leftMargin) + 115, py, 65, 30);
            modScaleMenu.setBounds(static_cast<int>(leftMargin) + 190, py, 130, 30);
            modMethodMenu.setBounds(static_cast<int>(leftMargin) + 330, py, 160, 30);
            btnModPreview.setBounds(static_cast<int>(leftMargin) + 500, py, 70, 30);
            btnModApply.setBounds(static_cast<int>(leftMargin) + 580, py, 70, 30);
            btnModCancel.setBounds(static_cast<int>(leftMargin) + 660, py, 70, 30);
        }

        modTargetBarMenu.setVisible(showMod);
        modKeyMenu.setVisible(showMod);
        modScaleMenu.setVisible(showMod);
        modMethodMenu.setVisible(showMod);
        btnModPreview.setVisible(showMod);
        btnModApply.setVisible(showMod);
        btnModCancel.setVisible(showMod);

        progressionBrowser.setBounds(leftMargin, 155.0f - headerHeight - 35.0f, seqTotalWidth, 7.0f * cellHeight + headerHeight + 65.0f);

        bool showSuggest = !isModulationPanelOpen && !isProgressionMode && !isMemoryModeOpen;
        suggestionPanel.setVisible(showSuggest);
        suggestionPanel.setBounds(leftMargin, 625.0f, seqTotalWidth, 150.0f);
    }

    void MatrixGridComponent::paint(juce::Graphics& g) {
        const auto bg = juce::Colour(0xff1a1a1a);
        const auto activeColor = juce::Colour(0xffffa500);
        const auto panelBg = juce::Colour(0xff252525);
        const auto textLight = juce::Colours::white;
        const auto gridLine = juce::Colour(0xff333333);

        g.fillAll(bg);

        auto drawBtn = [&](int x, int y, int w, int h, const char* txt, bool active, juce::Colour c) {
            g.setColour(active ? c : juce::Colour(0xff3a3a3a));
            g.fillRoundedRectangle(static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h), 4.0f);

            g.setColour(active ? juce::Colour(0xff1a1a1a) : textLight);
            g.setFont(12.0f);
            juce::String labelText(txt);
            if (labelText.isNotEmpty()) {
                g.drawText(labelText, x, y, w, h, juce::Justification::centred);
            }
            };

        drawBtn(progBtnBounds.getX(), progBtnBounds.getY(), progBtnBounds.getWidth(), progBtnBounds.getHeight(), "PROGRESSION", isProgressionMode, juce::Colours::hotpink.withAlpha(0.8f));
        drawBtn(modulationBtnBounds.getX(), modulationBtnBounds.getY(), modulationBtnBounds.getWidth(), modulationBtnBounds.getHeight(), "MODULATION", isModulationPanelOpen, juce::Colours::yellow.withAlpha(0.8f));
        drawBtn(memoryBtnBounds.getX(), memoryBtnBounds.getY(), memoryBtnBounds.getWidth(), memoryBtnBounds.getHeight(), "MEMORY", isMemoryModeOpen, juce::Colours::cyan.withAlpha(0.8f));
        drawBtn(dragMidiBtnBounds.getX(), dragMidiBtnBounds.getY(), dragMidiBtnBounds.getWidth(), dragMidiBtnBounds.getHeight(), "DRAG MIDI", false, juce::Colours::cyan.withAlpha(0.6f));
        drawBtn(allClearBtnBounds.getX(), allClearBtnBounds.getY(), allClearBtnBounds.getWidth(), allClearBtnBounds.getHeight(), "ALL CLEAR", false, juce::Colours::indianred.withAlpha(0.8f));

        if (isProgressionMode) return;

        if (isMemoryModeOpen) {
            g.setColour(panelBg);
            g.fillRoundedRectangle(leftMargin, 155.0f - headerHeight - 35.0f, seqTotalWidth, 7.0f * cellHeight + headerHeight + 65.0f, 8.0f);
            g.setColour(textLight);
            g.setFont(juce::Font(24.0f, juce::Font::bold));
            g.drawText("MEMORY SLOTS", static_cast<int>(leftMargin), 130, static_cast<int>(seqTotalWidth), 30, juce::Justification::centred);

            float slotW = (seqTotalWidth - 120.0f) / 4.0f;
            float startX = leftMargin + 30.0f;

            for (int i = 0; i < 4; ++i) {
                juce::Rectangle<float> r(startX + i * (slotW + 20.0f), 220.0f, slotW, 150.0f);
                bool isUsed = audioProcessor.isSlotUsed[i];
                g.setColour(isUsed ? activeColor.withAlpha(0.8f) : juce::Colour(0xff3a3a3a));
                g.fillRoundedRectangle(r, 8.0f);

                g.setColour(isUsed ? bg : juce::Colours::grey);
                g.setFont(juce::Font(24.0f, juce::Font::bold));

                juce::String memTitle = "MEMORY " + juce::String(i + 1);
                if (memTitle.isNotEmpty()) {
                    g.drawText(memTitle, r, juce::Justification::centred);
                }

                g.setFont(juce::Font(12.0f, juce::Font::plain));
                juce::Rectangle<float> textR = r.translated(0, 45.0f);
                if (isUsed) {
                    g.drawText("L-Click: LOAD", textR.removeFromTop(15.0f), juce::Justification::centred);
                    g.drawText("Shift+Click: OVRWRT", textR.removeFromTop(15.0f), juce::Justification::centred);
                    g.drawText("R-Click: CLEAR", textR.removeFromTop(15.0f), juce::Justification::centred);
                }
                else {
                    g.drawText("L-Click: SAVE", textR.removeFromTop(15.0f), juce::Justification::centred);
                }
            }
            return;
        }

        g.setFont(12.0f);
        g.setColour(juce::Colours::grey);
        g.drawText("INV", 0, 155 - 30, static_cast<int>(leftMargin) - 10, 25, juce::Justification::centredRight);

        int currentScaleType = audioProcessor.sequenceData[selectedStep].scaleType;
        int numNotes = MusicTheory::getScaleNoteCount(currentScaleType);
        auto intNames = MusicTheory::getScaleIntervalNames(currentScaleType);

        for (int v = 0; v < 7; ++v) {
            int voiceIdx = 6 - v;
            int offsetDegrees = voiceIdx * 2;
            int octaves = offsetDegrees / numNotes;
            int scaleDegree = offsetDegrees % numNotes;

            juce::String vn = intNames[scaleDegree];
            if (octaves > 0) {
                vn += " (+" + juce::String(octaves) + "8va)";
            }

            g.drawText(vn, 0, 155 + static_cast<int>(static_cast<float>(v) * cellHeight), static_cast<int>(leftMargin) - 10, static_cast<int>(cellHeight), juce::Justification::centredRight);
        }

        g.setColour(juce::Colours::indianred);
        g.drawText("DEL", 0, 155 + static_cast<int>(7.0f * cellHeight), static_cast<int>(leftMargin) - 10, 30, juce::Justification::centredRight);

        int editBar = (int)*audioProcessor.apvts.getRawParameterValue("editBar");
        int uiStepsPerBar = getStepsPerBar();
        float uiPpqPerStep = getPpqPerStep();
        float stepW = seqTotalWidth / static_cast<float>(uiStepsPerBar);
        int uiStepsPerBeat = std::max(1, juce::roundToInt(((int)*audioProcessor.apvts.getRawParameterValue("timeSigDen") == 0 ? 1.0f : 0.5f) / uiPpqPerStep));

        const auto& activeSeqData = audioProcessor.isPlayingModulationPreview.load() ? audioProcessor.previewSequenceData : audioProcessor.sequenceData;

        for (int s = 0; s < uiStepsPerBar; ) {
            int internalS = getInternalStep(editBar, s);
            int effS = getEffectiveStep(internalS);
            auto& effStep = activeSeqData[effS];

            int runLength = 1;
            for (int nextS = s + 1; nextS < uiStepsPerBar; ++nextS) {
                if (getEffectiveStep(getInternalStep(editBar, nextS)) == effS) runLength++;
                else break;
            }

            float startX = leftMargin + static_cast<float>(s) * stepW;
            float runW = static_cast<float>(runLength) * stepW;
            bool isSelected = false;

            for (int i = 0; i < runLength; ++i) {
                if (getEffectiveStep(getInternalStep(editBar, s + i)) == getEffectiveStep(selectedStep)) {
                    isSelected = true;
                }
            }

            juce::Rectangle<float> rHeader = getStepHeaderBounds(s, stepW).withWidth(runW);
            g.setColour(isSelected ? activeColor.withAlpha(0.3f) : panelBg);
            g.fillRoundedRectangle(rHeader.reduced(1.0f), 4.0f);

            if (effStep.isLocked) {
                juce::Rectangle<float> lockRect(startX, 155.0f - headerHeight - 35.0f, runW, 7.0f * cellHeight + headerHeight + 35.0f);
                g.setColour(juce::Colours::red.withAlpha(0.6f));
                g.drawRect(lockRect, 2.0f);

                g.setColour(juce::Colours::red.withAlpha(0.9f));
                g.setFont(juce::Font(10.0f, juce::Font::bold));
                g.drawText("[LOCKED]", rHeader.reduced(3.0f), juce::Justification::topRight);
            }

            juce::String recognizedName = VoicingEngine::getRecognizedChordName(activeSeqData, effS, 0.25f);
            juce::Rectangle<int> textRect = rHeader.reduced(2.0f).toNearestInt();

            if (recognizedName.contains("\n")) {
                juce::String part1 = recognizedName.upToFirstOccurrenceOf("\n", false, false).trim();
                juce::String part2 = recognizedName.fromFirstOccurrenceOf("\n", false, false).replaceCharacter('(', ' ').replaceCharacter(')', ' ').trim();

                if (part1.isNotEmpty()) {
                    g.setFont(juce::Font(13.0f, juce::Font::bold));
                    g.setColour(juce::Colour(0xffffa500));
                    g.drawFittedText(part1, textRect.removeFromTop(textRect.getHeight() / 2), juce::Justification::centredBottom, 1, 0.1f);
                }
                if (part2.isNotEmpty()) {
                    g.setFont(juce::Font(12.0f, juce::Font::plain));
                    g.setColour(juce::Colours::white);
                    g.drawFittedText(part2, textRect, juce::Justification::centredTop, 1, 0.1f);
                }
            }
            else {
                if (recognizedName.isNotEmpty()) {
                    g.setFont(juce::Font(13.0f, juce::Font::bold));
                    g.setColour(juce::Colour(0xffffa500));
                    g.drawFittedText(recognizedName, textRect, juce::Justification::centred, 2, 0.1f);
                }
            }

            juce::Rectangle<float> rInv(startX, 155.0f - 30.0f, runW, 25.0f);
            g.setColour(juce::Colour(0xff2a2a2a));
            g.fillRoundedRectangle(rInv.reduced(1.0f), 4.0f);
            g.setFont(12.0f);

            if (VoicingEngine::isAutoPattern(effStep.voicingMode)) {
                g.setColour(juce::Colours::grey.withAlpha(0.5f));
                g.drawText("INV: --", rInv, juce::Justification::centred);
            }
            else {
                g.setColour(juce::Colours::lightgreen);
                juce::String invStr = "INV: " + juce::String(effStep.inversion);
                if (invStr.isNotEmpty()) {
                    g.drawText(invStr, rInv, juce::Justification::centred);
                }
            }

            juce::Rectangle<float> rDel(startX, 155.0f + (7.0f * cellHeight), runW, 30.0f);
            g.setColour(juce::Colour(0xff331111));
            g.fillRoundedRectangle(rDel.reduced(1.0f), 4.0f);
            g.setColour(juce::Colours::red.withAlpha(0.7f));
            g.drawText("X", rDel, juce::Justification::centred);

            s += runLength;
        }

        for (int s = 0; s <= uiStepsPerBar; ++s) {
            g.setColour((s % uiStepsPerBeat == 0) ? juce::Colour(0xff555555) : gridLine);
            g.drawLine(leftMargin + static_cast<float>(s) * stepW, 155.0f - 5.0f, leftMargin + static_cast<float>(s) * stepW, 155.0f + (8.0f * cellHeight), (s % uiStepsPerBeat == 0) ? 2.0f : 1.0f);
        }

        int globalStep = audioProcessor.currentGlobalStep;
        int loopBarsIndex = juce::jlimit(0, 4, (int)*audioProcessor.apvts.getRawParameterValue("loopBars"));
        int tsNum = (int)*audioProcessor.apvts.getRawParameterValue("timeSigNum");
        int tsDenIdx = (int)*audioProcessor.apvts.getRawParameterValue("timeSigDen");
        int tsDen = (tsDenIdx == 0) ? 4 : (tsDenIdx == 1) ? 8 : 16;
        float beatsPerBar = static_cast<float>(tsNum) * (4.0f / static_cast<float>(tsDen));
        int internalStepsPerBar = juce::roundToInt(beatsPerBar / 0.25f);

        // ★警告(C26451)抑止のためキャストを強化
        int totalStepsInLoop = std::array<int, 5>{ 1, 4, 8, 12, 16 } [loopBarsIndex] * internalStepsPerBar;

        if (audioProcessor.isPlaying && globalStep >= 0 && totalStepsInLoop > 0 && ((globalStep % totalStepsInLoop) / internalStepsPerBar) == editBar) {
            int stepInBar = (globalStep % totalStepsInLoop) % internalStepsPerBar;
            float playX = leftMargin + (static_cast<float>(stepInBar) * (0.25f / uiPpqPerStep)) * stepW;
            g.setColour(juce::Colours::white.withAlpha(0.15f));
            g.fillRect(playX, 155.0f - 5.0f, stepW * (0.25f / uiPpqPerStep), 8.0f * cellHeight + 10.0f);
        }

        for (int s = 0; s < uiStepsPerBar; ++s) {
            for (int v = 0; v < 7; ++v) {
                g.setColour(juce::Colour(0xff121212));
                g.fillRect(getCellBounds(s, v, stepW).reduced(1.0f, 1.0f));
            }
        }

        for (int s = 0; s < uiStepsPerBar; ++s) {
            int internalS = getInternalStep(editBar, s);
            auto& effStep = activeSeqData[internalS];

            bool hasNotes = false;
            for (int v = 0; v < 7; ++v) {
                if (effStep.voices[v].isActive) {
                    hasNotes = true;
                    break;
                }
            }
            if (!hasNotes) continue;

            float safeGate = juce::jlimit(0.25f, 16.0f, effStep.gateLength);
            float desiredWidth = stepW * (safeGate / uiPpqPerStep);
            float maxWidth = seqTotalWidth - static_cast<float>(s) * stepW;

            if (VoicingEngine::isAutoPattern(effStep.voicingMode)) {
                // ★エラー修正(C4700): 変数を確実にゼロ初期化
                std::array<int, 7> dummy = { 0 };
                int maxV = VoicingEngine::getPatternBPitches(effStep, dummy);

                for (int i = 0; i < maxV; ++i) {
                    int v = 6 - i;
                    auto cell = getCellBounds(s, v, stepW).withWidth(juce::jmin(desiredWidth, maxWidth));

                    if (!effStep.voices[i].isActive) {
                        g.setColour(activeColor.withAlpha(0.2f));
                        g.drawRect(cell.reduced(1.0f, 1.0f), 1.0f);
                    }
                    else {
                        g.setColour(activeColor);
                        g.fillRect(cell.reduced(1.0f, 1.0f));

                        if (audioProcessor.isPlaying && globalStep >= 0 && totalStepsInLoop > 0 && internalS == (globalStep % totalStepsInLoop)) {
                            g.setColour(textLight);
                            g.drawRect(cell.reduced(1.0f, 1.0f), 2.0f);
                        }

                        int p = dummy[i];
                        juce::String noteName = MusicTheory::getNoteName(p) + juce::String((p / 12) - 1);
                        if (noteName.isNotEmpty()) {
                            g.setColour(bg);
                            g.setFont(juce::Font(14.0f, juce::Font::bold));
                            g.drawFittedText(noteName, cell.reduced(2.0f).toNearestInt(), juce::Justification::centred, 1, 0.1f);
                        }
                    }
                }
            }
            else {
                for (int v = 0; v < 7; ++v) {
                    int voiceIdx = 6 - v;
                    auto& voice = effStep.voices[voiceIdx];

                    if (voice.isActive) {
                        auto cell = getCellBounds(s, v, stepW).withWidth(juce::jmin(desiredWidth, maxWidth));
                        g.setColour(activeColor);
                        g.fillRect(cell.reduced(1.0f, 1.0f));

                        if (audioProcessor.isPlaying && globalStep >= 0 && totalStepsInLoop > 0 && internalS == (globalStep % totalStepsInLoop)) {
                            g.setColour(textLight);
                            g.drawRect(cell.reduced(1.0f, 1.0f), 2.0f);
                        }

                        int finalPitch = VoicingEngine::getPitchForVoice(effStep, voiceIdx);
                        int basePitch = MusicTheory::getBasePitch(effStep, voiceIdx) + voice.accidental + (voice.octaveShift * 12) + effStep.shift;
                        int engineOctaveShift = (finalPitch - basePitch) / 12;
                        int totalOct = voice.octaveShift + engineOctaveShift;

                        juce::String label;
                        if (totalOct != 0) {
                            label << (totalOct > 0 ? "+" : "") << totalOct;
                        }

                        if (voice.accidental > 0) {
                            for (int i = 0; i < voice.accidental; ++i) {
                                if (label.isNotEmpty()) label << " ";
                                label << "#";
                            }
                        }
                        else if (voice.accidental < 0 && voice.accidental > -128) {
                            for (int i = 0; i < std::abs((int)voice.accidental); ++i) {
                                if (label.isNotEmpty()) label << " ";
                                label << "b";
                            }
                        }

                        if (label.isNotEmpty()) {
                            g.setColour(bg);
                            g.setFont(juce::Font(14.0f, juce::Font::bold));
                            g.drawFittedText(label, cell.reduced(2.0f).toNearestInt(), juce::Justification::centred, 1, 0.1f);
                        }
                    }
                }
            }
        }

        for (int i = 0; i < 16; ++i) {
            auto r = getBarButtonBounds(i);
            bool inLoop = (i < std::array<int, 5>{1, 4, 8, 12, 16}[loopBarsIndex]);
            g.setColour(i == editBar ? activeColor : (inLoop ? panelBg : juce::Colour(0xff121212)));
            g.fillRoundedRectangle(r, 4.0f);
            g.setColour(i == editBar ? bg : (inLoop ? textLight : juce::Colours::grey));
            g.setFont(juce::Font(16.0f, juce::Font::bold));

            juce::String barText = "BAR " + juce::String(i + 1);
            if (barText.isNotEmpty()) {
                g.drawText(barText, r, juce::Justification::centred);
            }
        }

        if (isModulationPanelOpen) {
            juce::Rectangle<float> modArea(leftMargin, 625.0f, seqTotalWidth, 150.0f);

            g.setColour(juce::Colour(0xff222222));
            g.fillRoundedRectangle(modArea, 8.0f);
            g.setColour(juce::Colours::yellow.withAlpha(0.6f));
            g.drawRoundedRectangle(modArea, 8.0f, 1.0f);

            g.setColour(juce::Colours::yellow);
            g.setFont(juce::Font(14.0f, juce::Font::bold));

            juce::String modText = "MODULATION ASSISTANT (" + modMethodMenu.getText() + ")";
            if (modText.isNotEmpty()) {
                g.drawFittedText(modText, static_cast<int>(leftMargin) + 20, 635, 350, 20, juce::Justification::centredLeft, 1, 0.5f);
            }

            g.setColour(juce::Colours::grey);
            g.setFont(juce::Font(12.0f, juce::Font::plain));
            g.drawText("Target Bar and Key/Scale will be modulated using the selected method.",
                static_cast<int>(leftMargin) + 20, 715, 400, 20, juce::Justification::centredLeft);
        }
    }

    void MatrixGridComponent::mouseDown(const juce::MouseEvent& e) {
        isDraggingMidi = false;

        if (progBtnBounds.contains(e.getPosition())) { setProgressionMode(!isProgressionMode); isModulationPanelOpen = false; isMemoryModeOpen = false; resized(); return; }
        if (modulationBtnBounds.contains(e.getPosition())) { isModulationPanelOpen = !isModulationPanelOpen; isProgressionMode = false; isMemoryModeOpen = false; progressionBrowser.setVisible(false); resized(); repaint(); return; }
        if (memoryBtnBounds.contains(e.getPosition())) { isMemoryModeOpen = !isMemoryModeOpen; isProgressionMode = false; isModulationPanelOpen = false; progressionBrowser.setVisible(false); resized(); repaint(); return; }

        if (isMemoryModeOpen) {
            float startX = leftMargin + 30.0f;
            for (int i = 0; i < 4; ++i) {
                if (juce::Rectangle<float>(startX + i * ((seqTotalWidth - 120.0f) / 4.0f + 20.0f), 220.0f, (seqTotalWidth - 120.0f) / 4.0f, 150.0f).contains(e.position)) {
                    if (e.mods.isRightButtonDown()) audioProcessor.isSlotUsed[i] = false;
                    else if (e.mods.isShiftDown() || e.mods.isCommandDown()) { audioProcessor.memorySlots[i] = audioProcessor.sequenceData; audioProcessor.isSlotUsed[i] = true; }
                    else if (e.mods.isLeftButtonDown()) {
                        if (audioProcessor.isSlotUsed[i]) { audioProcessor.sequenceData = audioProcessor.memorySlots[i]; audioProcessor.previewSequenceData = audioProcessor.sequenceData; if (onRepaintRequest) onRepaintRequest(); }
                        else { audioProcessor.memorySlots[i] = audioProcessor.sequenceData; audioProcessor.isSlotUsed[i] = true; }
                    }
                    repaint(); return;
                }
            }
            return;
        }

        int tsNum = (int)*audioProcessor.apvts.getRawParameterValue("timeSigNum");
        int tsDenIdx = (int)*audioProcessor.apvts.getRawParameterValue("timeSigDen");
        int tsDen = (tsDenIdx == 0) ? 4 : (tsDenIdx == 1) ? 8 : 16;
        int internalStepsPerBar = juce::roundToInt(((float)tsNum * (4.0f / (float)tsDen)) / 0.25f);

        if (allClearBtnBounds.contains(e.getPosition())) {
            juce::Component::SafePointer<MatrixGridComponent> safeThis(this);
            juce::NativeMessageBox::showAsync(juce::MessageBoxOptions().withTitle("All Clear").withMessage("Clear ALL sequences and initialize?").withButton("Yes").withButton("No"),
                [safeThis, internalStepsPerBar](int result) {
                    if (safeThis != nullptr && result == 0) {
                        for (int s = 0; s < ChordMatrix::TotalSteps; ++s) {
                            safeThis->audioProcessor.sequenceData[s] = {};
                            safeThis->audioProcessor.previewSequenceData[s] = {};
                        }
                        if (safeThis->onStepSelected) safeThis->onStepSelected(safeThis->selectedStep);
                        safeThis->suggestionPanel.updateSuggestions(-1, 0.25f, internalStepsPerBar);
                        if (safeThis->onRepaintRequest) safeThis->onRepaintRequest();
                    }
                }
            ); return;
        }

        if (isProgressionMode || isModulationPanelOpen || audioProcessor.isPlayingModulationPreview.load()) return;

        isDraggingGate = false; isDraggingChord = false; dragStep = -1;
        int editBar = (int)*audioProcessor.apvts.getRawParameterValue("editBar");
        int uiStepsPerBar = getStepsPerBar();
        float stepW = seqTotalWidth / static_cast<float>(uiStepsPerBar);

        for (int i = 0; i < 16; ++i) {
            if (getBarButtonBounds(i).contains(e.position)) {
                if (e.mods.isLeftButtonDown()) {
                    audioProcessor.apvts.getParameter("editBar")->setValueNotifyingHost(static_cast<float>(i) / 15.0f);
                    if (onRepaintRequest) onRepaintRequest();
                }
                else if (e.mods.isRightButtonDown()) {
                    juce::Component::SafePointer<MatrixGridComponent> safeThis(this);
                    juce::NativeMessageBox::showAsync(juce::MessageBoxOptions().withTitle("Clear Bar").withMessage("Clear Bar " + juce::String(i + 1) + "?").withButton("Yes").withButton("No"),
                        [safeThis, i, internalStepsPerBar](int result) {
                            if (safeThis != nullptr && result == 0) {
                                for (int s = i * internalStepsPerBar; s < (i + 1) * internalStepsPerBar; ++s) safeThis->audioProcessor.sequenceData[s] = {};
                                if (safeThis->onRepaintRequest) safeThis->onRepaintRequest();
                            }
                        }
                    );
                }
                return;
            }
        }

        float localX = static_cast<float>(e.x) - leftMargin;
        if (localX < 0) return;

        int localUiStep = static_cast<int>(localX / stepW);
        if (localUiStep >= uiStepsPerBar) return;

        int internalS = getInternalStep(editBar, localUiStep);
        int effS = getEffectiveStep(internalS);

        if (e.y < 155.0f + 7.0f * cellHeight) {
            if (e.mods.isCtrlDown() || e.mods.isCommandDown()) {
                audioProcessor.sequenceData[effS].isLocked = !audioProcessor.sequenceData[effS].isLocked;
                repaint();
                return;
            }
        }

        if (e.y >= 155.0f - 30.0f && e.y < 155.0f - 5.0f) {
            if (!audioProcessor.sequenceData[effS].isLocked) {
                if (!VoicingEngine::isAutoPattern(audioProcessor.sequenceData[effS].voicingMode) && audioProcessor.sequenceData[effS].voicingMode != 3) {
                    int activeNotesCount = 0;
                    for (int v = 0; v < 7; ++v) {
                        if (audioProcessor.sequenceData[effS].voices[v].isActive) activeNotesCount++;
                    }

                    if (activeNotesCount > 0) {
                        if (e.mods.isLeftButtonDown()) audioProcessor.sequenceData[effS].inversion = (audioProcessor.sequenceData[effS].inversion + 1) % activeNotesCount;
                        else if (e.mods.isRightButtonDown()) audioProcessor.sequenceData[effS].inversion = (audioProcessor.sequenceData[effS].inversion - 1 + activeNotesCount) % activeNotesCount;

                        std::array<int, 7> vps = { 0 };
                        int count = VoicingEngine::getVoicedPitches(audioProcessor.sequenceData[effS], vps);
                        for (int i = 0; i < 7; ++i) audioProcessor.previewNotes[i].store(i < count ? vps[i] : -1);
                        audioProcessor.triggerPreview.store(true);
                    }
                }
            }
            suggestionPanel.updateSuggestions(effS, 0.25f, internalStepsPerBar);
            if (onStepSelected) onStepSelected(selectedStep = effS);
            return;
        }

        if (e.y >= 155.0f + 7.0f * cellHeight && e.y < 155.0f + 7.0f * cellHeight + 30.0f) {
            if (e.mods.isLeftButtonDown()) {
                audioProcessor.sequenceData[effS] = {};
                suggestionPanel.updateSuggestions(effS, 0.25f, internalStepsPerBar);
                if (onStepSelected) onStepSelected(selectedStep = effS);
                if (onRepaintRequest) onRepaintRequest();
            }
            return;
        }

        if (e.y >= 155.0f - headerHeight - 35.0f && e.y < 155.0f - 35.0f) {
            std::array<int, 7> vps = { 0 };
            int count = VoicingEngine::getVoicedPitches(audioProcessor.sequenceData[effS], vps);
            for (int i = 0; i < 7; ++i) audioProcessor.previewNotes[i].store(i < count ? vps[i] : -1);
            audioProcessor.triggerPreview.store(true);

            suggestionPanel.updateSuggestions(effS, 0.25f, internalStepsPerBar);
            if (onStepSelected) onStepSelected(selectedStep = effS);
            return;
        }

        if (e.y >= 155.0f && e.y < 155.0f + 7.0f * cellHeight) {
            auto& effStep = audioProcessor.sequenceData[internalS];

            float safeGate = juce::jlimit(0.25f, 16.0f, effStep.gateLength);
            float desiredWidth = stepW * (safeGate / getPpqPerStep());
            float maxWidth = seqTotalWidth - static_cast<float>(localUiStep) * stepW;

            for (int v = 0; v < 7; ++v) {
                int voiceIdx = 6 - v;
                auto cell = getCellBounds(localUiStep, v, stepW).withWidth(juce::jmin(desiredWidth, maxWidth));

                bool isCellActive = false;
                int maxV = 0;
                if (VoicingEngine::isAutoPattern(effStep.voicingMode)) {
                    // ★エラー修正(C4700): 変数を確実にゼロ初期化
                    std::array<int, 7> dummy = { 0 };
                    maxV = VoicingEngine::getPatternBPitches(effStep, dummy);
                    isCellActive = (voiceIdx < maxV && effStep.voices[voiceIdx].isActive);
                }
                else {
                    isCellActive = effStep.voices[voiceIdx].isActive;
                }

                if (isCellActive) {
                    if (cell.contains(e.position)) {
                        if (effStep.isLocked) return;

                        if (e.mods.isRightButtonDown()) {
                            effStep.voices[voiceIdx].isActive = false;
                            effStep.voices[voiceIdx].octaveShift = 0;
                            effStep.voices[voiceIdx].accidental = 0;

                            bool anyActive = false;
                            for (int i = 0; i < 7; ++i) {
                                if (effStep.voices[i].isActive) anyActive = true;
                            }
                            if (!anyActive) effStep = {};

                            suggestionPanel.updateSuggestions(internalS, 0.25f, internalStepsPerBar);
                            if (onStepSelected) onStepSelected(selectedStep = internalS);
                            return;
                        }

                        if (e.mods.isLeftButtonDown()) {
                            if (e.position.x > cell.getRight() - 10.0f) {
                                isDraggingGate = true; dragStep = internalS; dragStartGate = safeGate; dragStartX = static_cast<float>(e.position.x); return;
                            }
                            else {
                                isDraggingChord = true; dragStep = internalS; dragStartX = static_cast<float>(e.position.x);
                                int targetPitch = VoicingEngine::getPitchForVoice(effStep, voiceIdx);
                                audioProcessor.previewNotes[0].store(juce::jlimit(0, 127, targetPitch));

                                bool isLowestActive = true;
                                for (int iv = voiceIdx - 1; iv >= 0; --iv) {
                                    if (effStep.voices[iv].isActive) { isLowestActive = false; break; }
                                }
                                audioProcessor.previewNotes[1].store((effStep.voicingMode == 3 && isLowestActive) ? juce::jlimit(0, 127, targetPitch - 12) : -1);
                                for (int i = 2; i < 7; ++i) audioProcessor.previewNotes[i].store(-1);
                                audioProcessor.triggerPreview.store(true);

                                suggestionPanel.updateSuggestions(internalS, 0.25f, internalStepsPerBar);
                                if (onStepSelected) onStepSelected(selectedStep = internalS);
                                return;
                            }
                        }
                    }
                }
                else if (cell.contains(e.position) && e.mods.isLeftButtonDown()) {
                    if (effStep.isLocked) return;

                    if (VoicingEngine::isAutoPattern(effStep.voicingMode)) {
                        // ★エラー修正(C4700): 変数を確実にゼロ初期化
                        std::array<int, 7> dummy = { 0 };
                        int currentMaxV = VoicingEngine::getPatternBPitches(effStep, dummy);

                        if (voiceIdx < currentMaxV) {
                            bool wasEmpty = true;
                            for (int i = 0; i < 7; ++i) {
                                if (effStep.voices[i].isActive) wasEmpty = false;
                            }
                            if (wasEmpty) {
                                for (int i = 0; i < currentMaxV; ++i) effStep.voices[i].isActive = true;
                            }
                            else {
                                effStep.voices[voiceIdx].isActive = true;
                            }

                            effStep.voices[voiceIdx].octaveShift = 0;
                            audioProcessor.previewNotes[0].store(juce::jlimit(0, 127, VoicingEngine::getPitchForVoice(effStep, voiceIdx)));
                            for (int i = 1; i < 7; ++i) audioProcessor.previewNotes[i].store(-1);
                            audioProcessor.triggerPreview.store(true);

                            suggestionPanel.updateSuggestions(internalS, 0.25f, internalStepsPerBar);
                            if (onStepSelected) onStepSelected(selectedStep = internalS);
                            return;
                        }
                    }
                    else {
                        bool isCovered = false;
                        for (int prevS = internalS - 1; prevS >= 0; --prevS) {
                            if (audioProcessor.sequenceData[prevS].voices[voiceIdx].isActive && audioProcessor.sequenceData[prevS].gateLength > static_cast<float>(internalS - prevS) * 0.25f + 0.001f) {
                                isCovered = true;
                                break;
                            }
                        }
                        if (!isCovered) {
                            effStep.voices[voiceIdx].isActive = true;
                            audioProcessor.previewNotes[0].store(juce::jlimit(0, 127, VoicingEngine::getPitchForVoice(effStep, voiceIdx)));
                            for (int i = 1; i < 7; ++i) audioProcessor.previewNotes[i].store(-1);
                            audioProcessor.triggerPreview.store(true);

                            suggestionPanel.updateSuggestions(internalS, 0.25f, internalStepsPerBar);
                            if (onStepSelected) onStepSelected(selectedStep = internalS);
                            return;
                        }
                    }
                }
            }
        }
    }

    void MatrixGridComponent::mouseMove(const juce::MouseEvent& e) {
        if (isProgressionMode || isModulationPanelOpen || isMemoryModeOpen || audioProcessor.isPlayingModulationPreview.load()) {
            setMouseCursor(juce::MouseCursor::NormalCursor);
            return;
        }

        int editBar = (int)*audioProcessor.apvts.getRawParameterValue("editBar");
        int uiStepsPerBar = getStepsPerBar();
        float stepW = seqTotalWidth / static_cast<float>(uiStepsPerBar);
        bool hoveringEdge = false;

        if (float localX = static_cast<float>(e.x) - leftMargin; localX >= 0 && e.y >= 155.0f && e.y < 155.0f + 7.0f * cellHeight) {
            int localUiStep = static_cast<int>(localX / stepW);
            if (localUiStep < uiStepsPerBar) {
                int internalS = getInternalStep(editBar, localUiStep);
                auto& effStep = audioProcessor.sequenceData[internalS];

                if (!effStep.isLocked) {
                    int maxV = 0;
                    if (VoicingEngine::isAutoPattern(effStep.voicingMode)) {
                        // ★エラー修正(C4700): 変数を確実にゼロ初期化
                        std::array<int, 7> dummy = { 0 };
                        maxV = VoicingEngine::getPatternBPitches(effStep, dummy);
                    }

                    for (int v = 0; v < 7; ++v) {
                        int voiceIdx = 6 - v;
                        bool isCellActive = VoicingEngine::isAutoPattern(effStep.voicingMode) ? (voiceIdx < maxV && effStep.voices[voiceIdx].isActive) : effStep.voices[voiceIdx].isActive;

                        if (isCellActive) {
                            float safeGate = juce::jlimit(0.25f, 16.0f, effStep.gateLength);
                            float desiredWidth = stepW * (safeGate / getPpqPerStep());
                            float maxWidth = seqTotalWidth - static_cast<float>(localUiStep) * stepW;
                            auto cell = getCellBounds(localUiStep, v, stepW).withWidth(juce::jmin(desiredWidth, maxWidth));

                            if (cell.contains(e.position) && e.position.x > cell.getRight() - 10.0f) {
                                hoveringEdge = true;
                                break;
                            }
                        }
                    }
                }
            }
        }
        setMouseCursor(hoveringEdge ? juce::MouseCursor::LeftRightResizeCursor : juce::MouseCursor::NormalCursor);
    }

    void MatrixGridComponent::mouseDrag(const juce::MouseEvent& e) {
        int tsNum = (int)*audioProcessor.apvts.getRawParameterValue("timeSigNum");
        int tsDenIdx = (int)*audioProcessor.apvts.getRawParameterValue("timeSigDen");
        int tsDen = (tsDenIdx == 0) ? 4 : (tsDenIdx == 1) ? 8 : 16;
        int internalStepsPerBar = juce::roundToInt(((float)tsNum * (4.0f / (float)tsDen)) / 0.25f);

        int uiStepsPerBar = getStepsPerBar();
        float uiPpqPerStep = getPpqPerStep();

        if (!isDraggingMidi && dragMidiBtnBounds.contains(e.getMouseDownPosition()) && e.mouseWasDraggedSinceMouseDown()) {
            isDraggingMidi = true;
            int loopIdx = juce::jlimit(0, 4, (int)*audioProcessor.apvts.getRawParameterValue("loopBars"));
            ChordMatrix::MidiExport::exportAndDrag(audioProcessor.sequenceData, -1, std::array<int, 5>{1, 4, 8, 12, 16}[loopIdx], internalStepsPerBar, 0.25f, this);
            return;
        }
        if (!isDraggingMidi && e.mouseWasDraggedSinceMouseDown()) {
            for (int i = 0; i < 16; ++i) {
                if (getBarButtonBounds(i).contains(e.getMouseDownPosition().toFloat())) {
                    isDraggingMidi = true;
                    ChordMatrix::MidiExport::exportAndDrag(audioProcessor.sequenceData, i, 16, internalStepsPerBar, 0.25f, this);
                    return;
                }
            }
        }

        if (isProgressionMode || isModulationPanelOpen || isMemoryModeOpen || audioProcessor.isPlayingModulationPreview.load()) return;

        float stepW = seqTotalWidth / static_cast<float>(uiStepsPerBar);

        if (isDraggingGate && dragStep >= 0) {
            if (audioProcessor.sequenceData[dragStep].isLocked) return;

            float deltaX = static_cast<float>(e.position.x) - dragStartX;
            float maxGate = 16.0f;

            float newGate = juce::jlimit(0.25f, maxGate, std::round((dragStartGate + (deltaX / stepW) * uiPpqPerStep) / 0.25f) * 0.25f);

            audioProcessor.sequenceData[dragStep].gateLength = newGate;
            if (onRepaintRequest) onRepaintRequest();
        }
        else if (isDraggingChord && dragStep >= 0) {
            if (audioProcessor.sequenceData[dragStep].isLocked) return;

            if (int deltaUiSteps = std::round((static_cast<float>(e.position.x) - dragStartX) / stepW); deltaUiSteps != 0) {
                int deltaInternalSteps = juce::roundToInt(deltaUiSteps * (uiPpqPerStep / 0.25f));
                int targetStep = juce::jlimit(0, ChordMatrix::TotalSteps - 1, dragStep + deltaInternalSteps);

                if (audioProcessor.sequenceData[targetStep].isLocked) return;

                if (targetStep != dragStep) {
                    bool targetEmpty = true;
                    for (int v = 0; v < 7; ++v) {
                        if (audioProcessor.sequenceData[targetStep].voices[v].isActive) targetEmpty = false;
                    }

                    if (targetEmpty) {
                        audioProcessor.sequenceData[targetStep] = audioProcessor.sequenceData[dragStep];
                        audioProcessor.sequenceData[dragStep] = {};
                        dragStep = targetStep;
                        dragStartX += static_cast<float>(deltaUiSteps) * stepW;

                        suggestionPanel.updateSuggestions(targetStep, 0.25f, internalStepsPerBar);
                        if (onStepSelected) onStepSelected(selectedStep = targetStep);
                    }
                }
            }
        }
    }

    void MatrixGridComponent::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) {
        if (isProgressionMode || isModulationPanelOpen || isMemoryModeOpen || audioProcessor.isPlayingModulationPreview.load()) return;

        int editBar = (int)*audioProcessor.apvts.getRawParameterValue("editBar");
        int uiStepsPerBar = getStepsPerBar();
        float stepW = seqTotalWidth / static_cast<float>(uiStepsPerBar);

        if (float localX = static_cast<float>(e.x) - leftMargin; localX >= 0 && e.y >= 155.0f && e.y < 155.0f + 7.0f * cellHeight) {
            int localUiStep = static_cast<int>(localX / stepW);
            if (localUiStep < uiStepsPerBar) {
                int internalS = getInternalStep(editBar, localUiStep);
                auto& effStep = audioProcessor.sequenceData[internalS];

                int maxV = 0;
                if (VoicingEngine::isAutoPattern(effStep.voicingMode)) {
                    // ★エラー修正(C4700): 変数を確実にゼロ初期化
                    std::array<int, 7> dummy = { 0 };
                    maxV = VoicingEngine::getPatternBPitches(effStep, dummy);
                }

                for (int v = 0; v < 7; ++v) {
                    int voiceIdx = 6 - v;
                    float safeGate = juce::jlimit(0.25f, 16.0f, effStep.gateLength);
                    float desiredWidth = stepW * (safeGate / getPpqPerStep());
                    float maxWidth = seqTotalWidth - static_cast<float>(localUiStep) * stepW;
                    auto cell = getCellBounds(localUiStep, v, stepW).withWidth(juce::jmin(desiredWidth, maxWidth));

                    bool isCellActive = VoicingEngine::isAutoPattern(effStep.voicingMode) ? (voiceIdx < maxV && effStep.voices[voiceIdx].isActive) : effStep.voices[voiceIdx].isActive;

                    if (cell.contains(e.position) && isCellActive) {

                        if (effStep.isLocked) return;

                        auto& voice = effStep.voices[voiceIdx];

                        if (VoicingEngine::isAutoPattern(effStep.voicingMode)) {
                            voice.accidental = juce::jlimit<int8_t>(-24, 24, voice.accidental + (wheel.deltaY > 0 ? 1 : -1));
                        }
                        else if (e.mods.isCtrlDown() || e.mods.isCommandDown()) {
                            if (voiceIdx != 0) voice.accidental = juce::jlimit<int8_t>(-2, 2, voice.accidental + (wheel.deltaY > 0 ? 1 : -1));
                        }
                        else {
                            voice.octaveShift = juce::jlimit<int8_t>(-2, 2, voice.octaveShift + (wheel.deltaY > 0 ? 1 : -1));
                        }

                        selectedStep = internalS;
                        int previewPitch = VoicingEngine::getPitchForVoice(effStep, voiceIdx);
                        audioProcessor.previewNotes[0].store(juce::jlimit(0, 127, previewPitch));

                        bool isLowestActive = true;
                        for (int iv = voiceIdx - 1; iv >= 0; --iv) {
                            if (effStep.voices[iv].isActive) { isLowestActive = false; break; }
                        }

                        audioProcessor.previewNotes[1].store((effStep.voicingMode == 3 && isLowestActive) ? juce::jlimit(0, 127, previewPitch - 12) : -1);
                        for (int i = 2; i < 7; ++i) audioProcessor.previewNotes[i].store(-1);
                        audioProcessor.triggerPreview.store(true);

                        int tsNum = (int)*audioProcessor.apvts.getRawParameterValue("timeSigNum");
                        int tsDenIdx = (int)*audioProcessor.apvts.getRawParameterValue("timeSigDen");
                        int tsDen = (tsDenIdx == 0) ? 4 : (tsDenIdx == 1) ? 8 : 16;
                        int internalStepsPerBar = juce::roundToInt(((float)tsNum * (4.0f / (float)tsDen)) / 0.25f);

                        suggestionPanel.updateSuggestions(internalS, 0.25f, internalStepsPerBar);
                        if (onStepSelected) onStepSelected(selectedStep = internalS);
                        return;
                    }
                }
            }
        }
    }

} // namespace ChordMatrix
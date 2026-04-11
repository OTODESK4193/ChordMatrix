#include "MatrixGridComponent.h"
#include "../Engine/VoicingEngine.h"
#include "../Engine/MidiExport.h"
#include "../Engine/ProgressionEngine.h"

namespace ChordMatrix {

    MatrixGridComponent::MatrixGridComponent(ChordMatrixAudioProcessor& p) : audioProcessor(p) {
        progBtnBounds = juce::Rectangle<int>(leftMargin, 15, 110, 30);
        allClearBtnBounds = juce::Rectangle<int>(leftMargin + 120, 15, 110, 30);
        dragMidiBtnBounds = juce::Rectangle<int>(leftMargin + 240, 15, 110, 30);
        modulationBtnBounds = juce::Rectangle<int>(leftMargin + 360, 15, 110, 30);

        setupModulationPanel();
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

        for (int i = 1; i <= 16; ++i) modTargetBarMenu.addItem("Bar " + juce::String(i), i);
        for (int i = 0; i < 12; ++i) modKeyMenu.addItem(MusicTheory::getNoteName(i), i + 1);
        auto scales = MusicTheory::getScaleNames();
        for (int i = 0; i < scales.size(); ++i) modScaleMenu.addItem(scales[i], i + 1);

        modMethodMenu.addItem("Direct (V - I)", 1);
        modMethodMenu.addItem("Standard (ii - V - I)", 2);
        modMethodMenu.addItem("Tritone Sub (SubV - I)", 3);

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
            int stepsPerBeat = juce::roundToInt(beatLengthPpq / getPpqPerStep());
            if (stepsPerBeat < 1) stepsPerBeat = 1;

            ProgressionEngine::applyModulation(audioProcessor.sequenceData, audioProcessor.previewSequenceData,
                targetBar, targetKey, targetScale, methodIdx,
                getStepsPerBar(), stepsPerBeat, getPpqPerStep());

            audioProcessor.isPlayingModulationPreview.store(true);

            float ppq = getPpqPerStep();
            audioProcessor.internalPPQ = static_cast<double>(std::max(0, targetBar - 1)) * static_cast<double>(getStepsPerBar()) * static_cast<double>(ppq);
            audioProcessor.isInternalPlaying = true;

            repaint();
            };

        btnModApply.onClick = [this] {
            audioProcessor.sequenceData = audioProcessor.previewSequenceData;
            audioProcessor.isPlayingModulationPreview.store(false);
            audioProcessor.isInternalPlaying = false;
            isModulationPanelOpen = false;
            resized();
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
        repaint();
    }

    int MatrixGridComponent::getStepsPerBar() const {
        int tsNum = (int)*audioProcessor.apvts.getRawParameterValue("timeSigNum");
        int tsDenIdx = (int)*audioProcessor.apvts.getRawParameterValue("timeSigDen");
        int tsDen = (tsDenIdx == 0) ? 4 : (tsDenIdx == 1) ? 8 : 16;
        float ppqPerStep = getPpqPerStep();
        return juce::roundToInt((static_cast<float>(tsNum) * (4.0f / static_cast<float>(tsDen))) / ppqPerStep) < 1 ? 1 : juce::roundToInt((static_cast<float>(tsNum) * (4.0f / static_cast<float>(tsDen))) / ppqPerStep);
    }

    float MatrixGridComponent::getPpqPerStep() const {
        int stepSizeIdx = (int)*audioProcessor.apvts.getRawParameterValue("stepSize");
        return (stepSizeIdx == 0) ? 1.0f : (stepSizeIdx == 1) ? 0.5f : 0.25f;
    }

    int MatrixGridComponent::getEffectiveStep(int targetS) const {
        int eff = targetS;
        float ppq = getPpqPerStep();
        for (int prevS = targetS; prevS >= 0; --prevS) {
            float dist = static_cast<float>(targetS - prevS) * ppq;
            bool covers = false;
            for (int v = 0; v < 7; ++v) {
                const auto& sData = audioProcessor.isPlayingModulationPreview.load() ? audioProcessor.previewSequenceData[prevS] : audioProcessor.sequenceData[prevS];
                if (sData.voices[v].isActive && sData.gateLength > dist + 0.001f) {
                    covers = true; break;
                }
            }
            if (covers) { eff = prevS; break; }
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
        if (isModulationPanelOpen) {
            int py = 665;
            modTargetBarMenu.setBounds(static_cast<int>(leftMargin) + 10, py, 85, 30);
            modKeyMenu.setBounds(static_cast<int>(leftMargin) + 100, py, 65, 30);
            modScaleMenu.setBounds(static_cast<int>(leftMargin) + 170, py, 130, 30);
            modMethodMenu.setBounds(static_cast<int>(leftMargin) + 305, py, 160, 30);
            btnModPreview.setBounds(static_cast<int>(leftMargin) + 475, py, 80, 30);
            btnModApply.setBounds(static_cast<int>(leftMargin) + 560, py, 80, 30);
            btnModCancel.setBounds(static_cast<int>(leftMargin) + 645, py, 80, 30);
        }
        modTargetBarMenu.setVisible(isModulationPanelOpen);
        modKeyMenu.setVisible(isModulationPanelOpen);
        modScaleMenu.setVisible(isModulationPanelOpen);
        modMethodMenu.setVisible(isModulationPanelOpen);
        btnModPreview.setVisible(isModulationPanelOpen);
        btnModApply.setVisible(isModulationPanelOpen);
        btnModCancel.setVisible(isModulationPanelOpen);
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
            g.setColour(active ? juce::Colour(0xff1a1a1a) : textLight); g.setFont(12.0f);
            g.drawText(txt, x, y, w, h, juce::Justification::centred);
            };

        drawBtn(progBtnBounds.getX(), progBtnBounds.getY(), progBtnBounds.getWidth(), progBtnBounds.getHeight(), "PROGRESSION", isProgressionMode, juce::Colours::hotpink.withAlpha(0.8f));
        drawBtn(allClearBtnBounds.getX(), allClearBtnBounds.getY(), allClearBtnBounds.getWidth(), allClearBtnBounds.getHeight(), "ALL CLEAR", false, juce::Colours::indianred.withAlpha(0.8f));
        drawBtn(dragMidiBtnBounds.getX(), dragMidiBtnBounds.getY(), dragMidiBtnBounds.getWidth(), dragMidiBtnBounds.getHeight(), "DRAG MIDI", false, juce::Colours::cyan.withAlpha(0.6f));
        drawBtn(modulationBtnBounds.getX(), modulationBtnBounds.getY(), modulationBtnBounds.getWidth(), modulationBtnBounds.getHeight(), "MODULATION", isModulationPanelOpen, juce::Colours::yellow.withAlpha(0.8f));

        if (isProgressionMode) {
            g.setColour(panelBg);
            g.fillRoundedRectangle(leftMargin, 155.0f - headerHeight - 35.0f, seqTotalWidth, 7.0f * cellHeight + headerHeight + 65.0f, 8.0f);
            g.setColour(juce::Colours::grey);
            g.setFont(juce::Font(24.0f, juce::Font::bold));
            g.drawText("PROGRESSION BROWSER (Coming Soon)", static_cast<int>(leftMargin), 155, static_cast<int>(seqTotalWidth), 200, juce::Justification::centred);
            return;
        }

        g.setFont(12.0f); g.setColour(juce::Colours::grey);
        g.drawText("INV", 0, 155 - 30, static_cast<int>(leftMargin) - 10, 25, juce::Justification::centredRight);

        const char* vNames[] = { "6/13th", "4/11th", "2/9th", "7th", "5th", "3rd", "Root" };
        for (int i = 0; i < 7; ++i) {
            int textY = 155 + static_cast<int>(static_cast<float>(i) * cellHeight);
            g.drawText(vNames[i], 0, textY, static_cast<int>(leftMargin) - 10, static_cast<int>(cellHeight), juce::Justification::centredRight);
        }

        g.setColour(juce::Colours::indianred);
        int delY = 155 + static_cast<int>(7.0f * cellHeight);
        g.drawText("DEL", 0, delY, static_cast<int>(leftMargin) - 10, 30, juce::Justification::centredRight);

        int editBar = (int)*audioProcessor.apvts.getRawParameterValue("editBar");
        int stepsPerBar = getStepsPerBar();
        float ppqPerStep = getPpqPerStep();
        float stepW = seqTotalWidth / static_cast<float>(stepsPerBar);

        int tsDen = (int)*audioProcessor.apvts.getRawParameterValue("timeSigDen");
        float beatLengthPpq = (tsDen == 0) ? 1.0f : (tsDen == 1) ? 0.5f : 0.25f;
        int stepsPerBeat = juce::roundToInt(beatLengthPpq / ppqPerStep);
        if (stepsPerBeat < 1) stepsPerBeat = 1;

        const auto& activeSeqData = audioProcessor.isPlayingModulationPreview.load() ? audioProcessor.previewSequenceData : audioProcessor.sequenceData;

        for (int s = 0; s < stepsPerBar; ) {
            int actS = (editBar * stepsPerBar) + s;
            int effS = getEffectiveStep(actS);
            auto& effStep = activeSeqData[effS];

            int runLength = 1;
            for (int nextS = s + 1; nextS < stepsPerBar; ++nextS) {
                int nextActS = (editBar * stepsPerBar) + nextS;
                if (getEffectiveStep(nextActS) == effS) runLength++;
                else break;
            }

            float startX = leftMargin + static_cast<float>(s) * stepW;
            float runW = static_cast<float>(runLength) * stepW;
            bool isSelected = false;
            for (int i = 0; i < runLength; ++i) {
                if (getEffectiveStep((editBar * stepsPerBar) + s + i) == getEffectiveStep(selectedStep)) isSelected = true;
            }

            juce::Rectangle<float> rHeader = getStepHeaderBounds(s, stepW).withWidth(runW);
            g.setColour(isSelected ? activeColor.withAlpha(0.3f) : panelBg);
            g.fillRoundedRectangle(rHeader.reduced(1.0f), 4.0f);

            juce::String recognizedName = VoicingEngine::getRecognizedChordName(activeSeqData, effS, ppqPerStep);
            juce::Rectangle<int> textRect = rHeader.reduced(2.0f).toNearestInt();

            if (recognizedName.contains("\n")) {
                juce::String relName = recognizedName.upToFirstOccurrenceOf("\n", false, false).trim();
                juce::String absName = recognizedName.fromFirstOccurrenceOf("\n", false, false).replaceCharacter('(', ' ').replaceCharacter(')', ' ').trim();

                g.setFont(juce::Font(13.0f, juce::Font::bold));
                g.setColour(juce::Colour(0xffffa500));
                g.drawFittedText(relName, textRect.removeFromTop(textRect.getHeight() / 2), juce::Justification::centredBottom, 1, 0.7f);

                g.setFont(juce::Font(12.0f, juce::Font::plain));
                g.setColour(juce::Colours::white);
                g.drawFittedText(absName, textRect, juce::Justification::centredTop, 1, 0.7f);
            }
            else {
                g.setFont(juce::Font(13.0f, juce::Font::bold));
                g.setColour(juce::Colour(0xffffa500));
                g.drawFittedText(recognizedName, textRect, juce::Justification::centred, 2, 0.7f);
            }

            juce::Rectangle<float> rInv(startX, 155.0f - 30.0f, runW, 25.0f);
            g.setColour(juce::Colour(0xff2a2a2a));
            g.fillRoundedRectangle(rInv.reduced(1.0f), 4.0f);
            g.setFont(12.0f);

            if (effStep.voicingMode == 3) {
                g.setColour(juce::Colours::grey.withAlpha(0.5f));
                g.drawText("INV: --", rInv, juce::Justification::centred);
            }
            else {
                g.setColour(juce::Colours::lightgreen);
                g.drawText("INV: " + juce::String(effStep.inversion), rInv, juce::Justification::centred);
            }

            juce::Rectangle<float> rDel(startX, 155.0f + (7.0f * cellHeight), runW, 30.0f);
            g.setColour(juce::Colour(0xff331111));
            g.fillRoundedRectangle(rDel.reduced(1.0f), 4.0f);
            g.setColour(juce::Colours::red.withAlpha(0.7f));
            g.drawText("X", rDel, juce::Justification::centred);

            s += runLength;
        }

        for (int s = 0; s <= stepsPerBar; ++s) {
            g.setColour((s % stepsPerBeat == 0) ? juce::Colour(0xff555555) : gridLine);
            g.drawLine(leftMargin + static_cast<float>(s) * stepW, 155.0f - 5.0f, leftMargin + static_cast<float>(s) * stepW, 155.0f + (8.0f * cellHeight), (s % stepsPerBeat == 0) ? 2.0f : 1.0f);
        }

        int loopIdx = (int)*audioProcessor.apvts.getRawParameterValue("loopBars");
        constexpr std::array<int, 5> barsMap = { 1, 4, 8, 12, 16 };
        int numBars = barsMap[loopIdx];
        int globalStep = audioProcessor.currentGlobalStep;
        int totalStepsInLoop = numBars * stepsPerBar;

        if (audioProcessor.isPlaying && globalStep >= 0) {
            int currentStepInLoop = globalStep % totalStepsInLoop;
            int playingBar = currentStepInLoop / stepsPerBar;
            if (playingBar == editBar) {
                int localStep = currentStepInLoop % stepsPerBar;
                g.setColour(juce::Colours::white.withAlpha(0.15f));
                g.fillRect(leftMargin + static_cast<float>(localStep) * stepW, 155.0f - 5.0f, stepW, 8.0f * cellHeight + 10.0f);
            }
        }

        for (int s = 0; s < stepsPerBar; ++s) {
            for (int v = 0; v < 7; ++v) {
                auto cell = getCellBounds(s, v, stepW);
                g.setColour(juce::Colour(0xff121212));
                g.fillRect(cell.reduced(1.0f, 1.0f));
            }
        }

        for (int s = 0; s < stepsPerBar; ++s) {
            int actS = (editBar * stepsPerBar) + s;
            for (int v = 0; v < 7; ++v) {
                int voiceIdx = 6 - v;
                auto& voice = activeSeqData[actS].voices[voiceIdx];

                if (voice.isActive) {
                    auto cell = getCellBounds(s, v, stepW);
                    float safeGate = juce::jlimit(ppqPerStep, 16.0f, activeSeqData[actS].gateLength);
                    cell.setWidth(cell.getWidth() * (safeGate / ppqPerStep));

                    g.setColour(activeColor);
                    g.fillRect(cell.reduced(1.0f, 1.0f));

                    if (audioProcessor.isPlaying && globalStep >= 0 && actS == (globalStep % totalStepsInLoop)) {
                        g.setColour(textLight); g.drawRect(cell.reduced(1.0f, 1.0f), 2.0f);
                    }

                    // ==============================================================================
                    // ★修正: ♯、♭、オクターブの描画。セルの中心に正確に描画する
                    // ==============================================================================
                    juce::String label;
                    if (voice.octaveShift != 0) {
                        label << (voice.octaveShift > 0 ? "+" : "") << (int)voice.octaveShift;
                    }
                    if (voice.accidental > 0) {
                        for (int i = 0; i < voice.accidental; ++i) { if (label.isNotEmpty()) label << " "; label << "#"; }
                    }
                    else if (voice.accidental < 0) {
                        for (int i = 0; i < std::abs(voice.accidental); ++i) { if (label.isNotEmpty()) label << " "; label << "b"; }
                    }

                    if (label.isNotEmpty()) {
                        g.setColour(bg);
                        g.setFont(juce::Font(14.0f, juce::Font::bold));
                        // セルの幅が狭くてもはみ出さないよう drawFittedText に変更
                        g.drawFittedText(label, cell.reduced(2.0f).toNearestInt(), juce::Justification::centred, 1);
                    }
                }
            }
        }

        for (int i = 0; i < 16; ++i) {
            auto r = getBarButtonBounds(i);
            bool inLoop = (i < numBars);
            g.setColour(i == editBar ? activeColor : (inLoop ? panelBg : juce::Colour(0xff121212)));
            g.fillRoundedRectangle(r, 4.0f);
            g.setColour(i == editBar ? bg : (inLoop ? textLight : juce::Colours::grey));
            g.setFont(juce::Font(16.0f, juce::Font::bold));
            g.drawText("BAR " + juce::String(i + 1), r, juce::Justification::centred);
        }

        if (isModulationPanelOpen) {
            g.setColour(juce::Colour(0xff222222));
            g.fillRoundedRectangle(leftMargin, 635, seqTotalWidth, 75, 8.0f);

            juce::String approachName = modMethodMenu.getText();
            g.setColour(juce::Colours::yellow);
            g.setFont(juce::Font(14.0f, juce::Font::bold));
            g.drawText("MODULATION ASSISTANT (" + approachName + ")", static_cast<int>(leftMargin) + 10, 640, 350, 20, juce::Justification::centredLeft);

            if (audioProcessor.isPlayingModulationPreview.load()) {
                g.setColour(juce::Colours::red);
                g.drawText("PREVIEWING...", static_cast<int>(leftMargin) + 380, 640, 100, 20, juce::Justification::centredLeft);
            }
        }
    }

    void MatrixGridComponent::mouseDown(const juce::MouseEvent& e) {
        isDraggingMidi = false;

        if (progBtnBounds.contains(e.getPosition())) {
            isProgressionMode = !isProgressionMode;
            repaint();
            return;
        }

        if (modulationBtnBounds.contains(e.getPosition())) {
            isModulationPanelOpen = !isModulationPanelOpen;
            resized();
            repaint();
            return;
        }

        if (allClearBtnBounds.contains(e.getPosition())) {
            juce::Component::SafePointer<MatrixGridComponent> safeThis(this);
            juce::NativeMessageBox::showAsync(
                juce::MessageBoxOptions().withTitle("All Clear").withMessage("Clear ALL sequences and initialize?").withButton("Yes").withButton("No"),
                [safeThis](int result) {
                    if (safeThis != nullptr && result == 0) {
                        for (int s = 0; s < ChordMatrix::TotalSteps; ++s) {
                            safeThis->audioProcessor.sequenceData[s].inversion = 0;
                            safeThis->audioProcessor.sequenceData[s].shift = 0;
                            for (int v = 0; v < 7; ++v) {
                                safeThis->audioProcessor.sequenceData[s].voices[v].isActive = false;
                                safeThis->audioProcessor.sequenceData[s].voices[v].octaveShift = 0;
                                safeThis->audioProcessor.sequenceData[s].voices[v].accidental = 0;
                            }
                            safeThis->audioProcessor.previewSequenceData[s] = safeThis->audioProcessor.sequenceData[s];
                        }
                        if (safeThis->onRepaintRequest) safeThis->onRepaintRequest();
                        safeThis->repaint();
                    }
                }
            );
            return;
        }

        // ==============================================================================
        // ★修正: パネルが開いている時やプレビュー中はグリッド操作を完全にブロックする
        // ==============================================================================
        if (isProgressionMode || isModulationPanelOpen || audioProcessor.isPlayingModulationPreview.load()) return;

        isDraggingGate = false;
        isDraggingChord = false;
        dragStep = -1;

        int editBar = (int)*audioProcessor.apvts.getRawParameterValue("editBar");
        int stepsPerBar = getStepsPerBar();
        float ppqPerStep = getPpqPerStep();
        float stepW = seqTotalWidth / static_cast<float>(stepsPerBar);

        bool isLeftClick = e.mods.isLeftButtonDown();
        bool isRightClick = e.mods.isRightButtonDown();

        float localX = static_cast<float>(e.x) - leftMargin;
        if (localX < 0) {
            for (int i = 0; i < 16; ++i) {
                if (getBarButtonBounds(i).contains(e.position)) goto HandleBarButtonClick;
            }
            return;
        }

        if (e.y >= 155.0f - 30.0f && e.y < 155.0f - 5.0f) {
            int localStep = static_cast<int>(localX / stepW);
            if (localStep < stepsPerBar) {
                int clickedActS = (editBar * stepsPerBar) + localStep;
                int effS = getEffectiveStep(clickedActS);
                if (audioProcessor.sequenceData[effS].voicingMode == 3) return;

                int activeNotesCount = 0;
                for (int v = 0; v < 7; ++v) {
                    if (audioProcessor.sequenceData[effS].voices[v].isActive) activeNotesCount++;
                }

                if (activeNotesCount > 0) {
                    if (isLeftClick) {
                        audioProcessor.sequenceData[effS].inversion = (audioProcessor.sequenceData[effS].inversion + 1) % activeNotesCount;
                    }
                    else if (isRightClick) {
                        audioProcessor.sequenceData[effS].inversion = (audioProcessor.sequenceData[effS].inversion - 1 + activeNotesCount) % activeNotesCount;
                    }
                }
                selectedStep = effS;
                if (onStepSelected) onStepSelected(selectedStep);
            }
            return;
        }

        if (e.y >= 155.0f + 7.0f * cellHeight && e.y < 155.0f + 7.0f * cellHeight + 30.0f) {
            if (isLeftClick) {
                int localStep = static_cast<int>(localX / stepW);
                if (localStep < stepsPerBar) {
                    int clickedActS = (editBar * stepsPerBar) + localStep;
                    int effS = getEffectiveStep(clickedActS);
                    for (int v = 0; v < 7; ++v) {
                        audioProcessor.sequenceData[effS].voices[v].isActive = false;
                        audioProcessor.sequenceData[effS].voices[v].octaveShift = 0;
                        audioProcessor.sequenceData[effS].voices[v].accidental = 0;
                    }
                    selectedStep = effS;
                    if (onStepSelected) onStepSelected(selectedStep);
                }
            }
            return;
        }

        if (e.y >= 155.0f - headerHeight - 35.0f && e.y < 155.0f - 35.0f) {
            int localStep = static_cast<int>(localX / stepW);
            if (localStep < stepsPerBar) {
                int clickedActS = (editBar * stepsPerBar) + localStep;
                int effS = getEffectiveStep(clickedActS);
                selectedStep = effS;

                std::array<int, 7> vps;
                int count = VoicingEngine::getVoicedPitches(audioProcessor.sequenceData[effS], vps);
                for (int i = 0; i < 7; ++i) {
                    audioProcessor.previewNotes[i].store(i < count ? vps[i] : -1);
                }
                audioProcessor.triggerPreview.store(true);

                if (onStepSelected) onStepSelected(selectedStep);
            }
            return;
        }

        if (e.y >= 155.0f && e.y < 155.0f + 7.0f * cellHeight) {
            for (int s = 0; s < stepsPerBar; ++s) {
                int actS = (editBar * stepsPerBar) + s;
                for (int v = 0; v < 7; ++v) {
                    int voiceIdx = 6 - v;
                    auto cell = getCellBounds(s, v, stepW);
                    float safeGate = juce::jlimit(ppqPerStep, 16.0f, audioProcessor.sequenceData[actS].gateLength);
                    if (audioProcessor.sequenceData[actS].voices[voiceIdx].isActive) {
                        cell.setWidth(cell.getWidth() * (safeGate / ppqPerStep));
                    }

                    if (cell.contains(e.position)) {
                        if (audioProcessor.sequenceData[actS].voices[voiceIdx].isActive) {
                            if (isRightClick) {
                                audioProcessor.sequenceData[actS].voices[voiceIdx].isActive = false;
                                audioProcessor.sequenceData[actS].voices[voiceIdx].octaveShift = 0;
                                audioProcessor.sequenceData[actS].voices[voiceIdx].accidental = 0;
                                selectedStep = actS;
                                if (onStepSelected) onStepSelected(selectedStep);
                                return;
                            }

                            if (isLeftClick) {
                                if (e.position.x > cell.getRight() - 10.0f) {
                                    isDraggingGate = true; dragStep = actS; dragStartGate = safeGate; dragStartX = static_cast<float>(e.position.x);
                                    return;
                                }
                                else {
                                    isDraggingChord = true; dragStep = actS; dragStartX = static_cast<float>(e.position.x); selectedStep = actS;
                                    if (onStepSelected) onStepSelected(selectedStep);
                                    return;
                                }
                            }
                        }
                        else {
                            bool isCovered = false;
                            for (int prevS = actS - 1; prevS >= 0; --prevS) {
                                float dist = static_cast<float>(actS - prevS) * ppqPerStep;
                                if (audioProcessor.sequenceData[prevS].voices[voiceIdx].isActive && audioProcessor.sequenceData[prevS].gateLength > dist + 0.001f) {
                                    isCovered = true; break;
                                }
                            }

                            if (isLeftClick && !isCovered) {
                                audioProcessor.sequenceData[actS].voices[voiceIdx].isActive = true;
                                selectedStep = actS;

                                auto& stepData = audioProcessor.sequenceData[actS];
                                int pitch = MusicTheory::getBasePitch(stepData, voiceIdx) + stepData.voices[voiceIdx].accidental + (stepData.voices[voiceIdx].octaveShift * 12);
                                audioProcessor.previewNotes[0].store(juce::jlimit(0, 127, pitch));
                                for (int i = 1; i < 7; ++i) audioProcessor.previewNotes[i].store(-1);
                                audioProcessor.triggerPreview.store(true);

                                if (onStepSelected) onStepSelected(selectedStep);
                                return;
                            }
                        }
                    }
                }
            }
        }

    HandleBarButtonClick:
        for (int i = 0; i < 16; ++i) {
            if (getBarButtonBounds(i).contains(e.position)) {
                if (isLeftClick) {
                    audioProcessor.apvts.getParameter("editBar")->setValueNotifyingHost(static_cast<float>(i) / 15.0f);
                    if (onRepaintRequest) onRepaintRequest();
                }
                else if (isRightClick) {
                    juce::Component::SafePointer<MatrixGridComponent> safeThis(this);
                    juce::NativeMessageBox::showAsync(
                        juce::MessageBoxOptions().withTitle("Clear Bar").withMessage("Clear all steps in Bar " + juce::String(i + 1) + "?").withButton("Yes").withButton("No"),
                        [safeThis, i, stepsPerBar](int result) {
                            if (safeThis != nullptr && result == 0) {
                                for (int s = i * stepsPerBar; s < (i + 1) * stepsPerBar; ++s) {
                                    safeThis->audioProcessor.sequenceData[s].inversion = 0;
                                    for (int v = 0; v < 7; ++v) {
                                        safeThis->audioProcessor.sequenceData[s].voices[v].isActive = false;
                                        safeThis->audioProcessor.sequenceData[s].voices[v].octaveShift = 0;
                                        safeThis->audioProcessor.sequenceData[s].voices[v].accidental = 0;
                                    }
                                }
                                if (safeThis->onRepaintRequest) safeThis->onRepaintRequest();
                            }
                        }
                    );
                }
                return;
            }
        }
    }

    void MatrixGridComponent::mouseMove(const juce::MouseEvent& e) {
        // ★修正: パネルが開いている時やプレビュー中はグリッド操作ブロック
        if (isProgressionMode || isModulationPanelOpen || audioProcessor.isPlayingModulationPreview.load()) {
            setMouseCursor(juce::MouseCursor::NormalCursor); return;
        }

        int editBar = (int)*audioProcessor.apvts.getRawParameterValue("editBar");
        int stepsPerBar = getStepsPerBar();
        float ppqPerStep = getPpqPerStep();
        float stepW = seqTotalWidth / static_cast<float>(stepsPerBar);
        bool hoveringEdge = false;

        float localX = static_cast<float>(e.x) - leftMargin;
        if (localX < 0) { setMouseCursor(juce::MouseCursor::NormalCursor); return; }

        if (e.y >= 155.0f && e.y < 155.0f + 7.0f * cellHeight) {
            for (int s = 0; s < stepsPerBar; ++s) {
                int actS = (editBar * stepsPerBar) + s;
                for (int v = 0; v < 7; ++v) {
                    int voiceIdx = 6 - v;
                    if (audioProcessor.sequenceData[actS].voices[voiceIdx].isActive) {
                        auto cell = getCellBounds(s, v, stepW);
                        float safeGate = juce::jlimit(ppqPerStep, 16.0f, audioProcessor.sequenceData[actS].gateLength);
                        cell.setWidth(cell.getWidth() * (safeGate / ppqPerStep));
                        if (cell.contains(e.position) && e.position.x > cell.getRight() - 10.0f) {
                            hoveringEdge = true; break;
                        }
                    }
                }
            }
        }
        setMouseCursor(hoveringEdge ? juce::MouseCursor::LeftRightResizeCursor : juce::MouseCursor::NormalCursor);
    }

    void MatrixGridComponent::mouseDrag(const juce::MouseEvent& e) {
        if (!isDraggingMidi && dragMidiBtnBounds.contains(e.getMouseDownPosition()) && e.mouseWasDraggedSinceMouseDown()) {
            isDraggingMidi = true;

            int tsNum = (int)*audioProcessor.apvts.getRawParameterValue("timeSigNum");
            int tsDenIdx = (int)*audioProcessor.apvts.getRawParameterValue("timeSigDen");
            int tsDen = (tsDenIdx == 0) ? 4 : (tsDenIdx == 1) ? 8 : 16;
            int stepSizeIdx = (int)*audioProcessor.apvts.getRawParameterValue("stepSize");
            float ppqPerStep = (stepSizeIdx == 0) ? 1.0f : (stepSizeIdx == 1) ? 0.5f : 0.25f;
            int stepsPerBar = juce::roundToInt((static_cast<float>(tsNum) * (4.0f / static_cast<float>(tsDen))) / ppqPerStep);
            if (stepsPerBar < 1) stepsPerBar = 1;

            int loopIdx = (int)*audioProcessor.apvts.getRawParameterValue("loopBars");
            constexpr std::array<int, 5> barsMap = { 1, 4, 8, 12, 16 };

            ChordMatrix::MidiExport::exportAndDrag(audioProcessor.sequenceData, -1, barsMap[loopIdx], stepsPerBar, ppqPerStep, this);
            return;
        }

        if (!isDraggingMidi && e.mouseWasDraggedSinceMouseDown()) {
            int stepsPerBar = getStepsPerBar();
            for (int i = 0; i < 16; ++i) {
                if (getBarButtonBounds(i).contains(e.getMouseDownPosition().toFloat())) {
                    isDraggingMidi = true;
                    float ppqPerStep = (static_cast<int>(*audioProcessor.apvts.getRawParameterValue("stepSize")) == 0) ? 1.0f :
                        (static_cast<int>(*audioProcessor.apvts.getRawParameterValue("stepSize")) == 1) ? 0.5f : 0.25f;
                    ChordMatrix::MidiExport::exportAndDrag(audioProcessor.sequenceData, i, 16, stepsPerBar, ppqPerStep, this);
                    return;
                }
            }
        }

        // ★修正: パネルが開いている時やプレビュー中はグリッド操作ブロック
        if (isProgressionMode || isModulationPanelOpen || audioProcessor.isPlayingModulationPreview.load()) return;

        int stepsPerBar = getStepsPerBar();
        float ppqPerStep = getPpqPerStep();
        float stepW = seqTotalWidth / static_cast<float>(stepsPerBar);

        if (isDraggingGate && dragStep >= 0) {
            float deltaX = static_cast<float>(e.position.x) - dragStartX;
            float maxGate = 16.0f;
            int editBar = (int)*audioProcessor.apvts.getRawParameterValue("editBar");
            int localS = dragStep % stepsPerBar;

            for (int nextS = localS + 1; nextS < stepsPerBar; ++nextS) {
                int actNext = (editBar * stepsPerBar) + nextS;
                for (int v = 0; v < 7; ++v) {
                    if (audioProcessor.sequenceData[actNext].voices[v].isActive) {
                        maxGate = static_cast<float>(nextS - localS) * ppqPerStep;
                        goto ExitLoopGate;
                    }
                }
            }
        ExitLoopGate:
            float newGate = dragStartGate + (deltaX / stepW) * ppqPerStep;
            newGate = std::round(newGate / ppqPerStep) * ppqPerStep;
            audioProcessor.sequenceData[dragStep].gateLength = juce::jlimit(ppqPerStep, maxGate, newGate);
            if (onRepaintRequest) onRepaintRequest();
        }
        else if (isDraggingChord && dragStep >= 0) {
            int deltaSteps = std::round((static_cast<float>(e.position.x) - dragStartX) / stepW);
            if (deltaSteps != 0) {
                int targetStep = juce::jlimit(0, ChordMatrix::TotalSteps - 1, dragStep + deltaSteps);
                if (targetStep != dragStep) {
                    bool targetEmpty = true;
                    for (int v = 0; v < 7; ++v) {
                        if (audioProcessor.sequenceData[targetStep].voices[v].isActive) targetEmpty = false;
                    }
                    if (targetEmpty) {
                        audioProcessor.sequenceData[targetStep] = audioProcessor.sequenceData[dragStep];
                        for (int v = 0; v < 7; ++v) audioProcessor.sequenceData[dragStep].voices[v].isActive = false;
                        dragStep = targetStep;
                        dragStartX += static_cast<float>(deltaSteps) * stepW;
                        selectedStep = targetStep;
                        if (onStepSelected) onStepSelected(selectedStep);
                    }
                }
            }
        }
    }

    void MatrixGridComponent::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) {
        // ★修正: パネルが開いている時やプレビュー中はグリッド操作ブロック
        if (isProgressionMode || isModulationPanelOpen || audioProcessor.isPlayingModulationPreview.load()) return;

        int editBar = (int)*audioProcessor.apvts.getRawParameterValue("editBar");
        int stepsPerBar = getStepsPerBar();
        float stepW = seqTotalWidth / static_cast<float>(stepsPerBar);

        float localX = static_cast<float>(e.x) - leftMargin;
        if (localX < 0) return;

        if (e.y >= 155.0f && e.y < 155.0f + 7.0f * cellHeight) {
            for (int s = 0; s < stepsPerBar; ++s) {
                int actS = (editBar * stepsPerBar) + s;
                for (int v = 0; v < 7; ++v) {
                    int voiceIdx = 6 - v;
                    if (getCellBounds(s, v, stepW).contains(e.position) && audioProcessor.sequenceData[actS].voices[voiceIdx].isActive) {
                        auto& voice = audioProcessor.sequenceData[actS].voices[voiceIdx];
                        if (e.mods.isCtrlDown() || e.mods.isCommandDown()) {
                            voice.accidental = juce::jlimit<int8_t>(-2, 2, voice.accidental + (wheel.deltaY > 0 ? 1 : -1));
                        }
                        else {
                            voice.octaveShift = juce::jlimit<int8_t>(-2, 2, voice.octaveShift + (wheel.deltaY > 0 ? 1 : -1));
                        }
                        selectedStep = actS;
                        if (onStepSelected) onStepSelected(selectedStep);
                        return;
                    }
                }
            }
        }
    }

} // namespace ChordMatrix
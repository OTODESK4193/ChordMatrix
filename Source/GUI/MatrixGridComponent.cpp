#include "MatrixGridComponent.h"
#include "../Engine/VoicingEngine.h"

namespace ChordMatrix {

    MatrixGridComponent::MatrixGridComponent(ChordMatrixAudioProcessor& p) : audioProcessor(p) {}
    MatrixGridComponent::~MatrixGridComponent() {}

    void MatrixGridComponent::setProgressionMode(bool isProg) {
        isProgressionMode = isProg;
        repaint();
    }

    int MatrixGridComponent::getStepsPerBar() const {
        int tsNum = (int)*audioProcessor.apvts.getRawParameterValue("timeSigNum");
        int tsDenIdx = (int)*audioProcessor.apvts.getRawParameterValue("timeSigDen");
        int tsDen = (tsDenIdx == 0) ? 4 : (tsDenIdx == 1) ? 8 : 16;
        float ppqPerStep = getPpqPerStep();
        return juce::roundToInt(((float)tsNum * (4.0f / (float)tsDen)) / ppqPerStep) < 1 ? 1 : juce::roundToInt(((float)tsNum * (4.0f / (float)tsDen)) / ppqPerStep);
    }

    float MatrixGridComponent::getPpqPerStep() const {
        int stepSizeIdx = (int)*audioProcessor.apvts.getRawParameterValue("stepSize");
        return (stepSizeIdx == 0) ? 1.0f : (stepSizeIdx == 1) ? 0.5f : 0.25f;
    }

    int MatrixGridComponent::getEffectiveStep(int targetS) const {
        int eff = targetS;
        float ppq = getPpqPerStep();
        for (int prevS = targetS; prevS >= 0; --prevS) {
            float dist = (float)(targetS - prevS) * ppq;
            bool covers = false;
            for (int v = 0; v < 7; ++v) {
                if (audioProcessor.sequenceData[prevS].voices[v].isActive && audioProcessor.sequenceData[prevS].gateLength > dist + 0.001f) {
                    covers = true; break;
                }
            }
            if (covers) { eff = prevS; break; }
        }
        return eff;
    }

    juce::Rectangle<float> MatrixGridComponent::getCellBounds(int s, int v, float stepW) const {
        return { (float)s * stepW, 155.0f + (float)v * cellHeight, stepW, cellHeight - 2.0f };
    }

    juce::Rectangle<float> MatrixGridComponent::getStepHeaderBounds(int s, float stepW) const {
        return { (float)s * stepW, 155.0f - headerHeight - 35.0f, stepW, headerHeight - 5.0f };
    }

    juce::Rectangle<float> MatrixGridComponent::getBarButtonBounds(int i) const {
        float w = seqTotalWidth / 8.0f;
        return { (float)(i % 8) * w, 555.0f + (float)(i / 8) * 35.0f, w - 8.0f, 30.0f };
    }

    void MatrixGridComponent::resized() {}

    void MatrixGridComponent::paint(juce::Graphics& g) {
        const auto bg = juce::Colour(0xff1a1a1a); // ★修正：ここで背景色を定義
        const auto activeColor = juce::Colour(0xffffa500);
        const auto panelBg = juce::Colour(0xff252525);
        const auto textLight = juce::Colours::white;
        const auto gridLine = juce::Colour(0xff333333);

        g.fillAll(bg);

        if (isProgressionMode) {
            g.setColour(panelBg);
            g.fillRoundedRectangle(0.0f, 155.0f - headerHeight - 35.0f, seqTotalWidth, 7.0f * cellHeight + headerHeight + 65.0f, 8.0f);
            g.setColour(juce::Colours::grey);
            g.setFont(juce::Font(24.0f, juce::Font::bold));
            g.drawText("PROGRESSION BROWSER (Coming Soon)", 0, 155, (int)seqTotalWidth, 200, juce::Justification::centred);
            return;
        }

        g.setFont(12.0f); g.setColour(juce::Colours::grey);
        g.drawText("INV", -60, 155 - 30, 50, 25, juce::Justification::centredRight);

        const char* vNames[] = { "6/13th", "4/11th", "2/9th", "7th", "5th", "3rd", "Root" };
        for (int i = 0; i < 7; ++i) g.drawText(vNames[i], -60, 155 + (int)(i * cellHeight), 50, (int)cellHeight, juce::Justification::centredRight);

        g.setColour(juce::Colours::indianred);
        g.drawText("DEL", -60, 155 + (int)(7 * cellHeight), 50, 30, juce::Justification::centredRight);

        int editBar = (int)*audioProcessor.apvts.getRawParameterValue("editBar");
        int stepsPerBar = getStepsPerBar();
        float ppqPerStep = getPpqPerStep();
        float stepW = seqTotalWidth / (float)stepsPerBar;

        int tsDen = (int)*audioProcessor.apvts.getRawParameterValue("timeSigDen");
        float beatLengthPpq = (tsDen == 0) ? 1.0f : (tsDen == 1) ? 0.5f : 0.25f;
        int stepsPerBeat = juce::roundToInt(beatLengthPpq / ppqPerStep);
        if (stepsPerBeat < 1) stepsPerBeat = 1;

        for (int s = 0; s < stepsPerBar; ) {
            int actS = (editBar * stepsPerBar) + s;
            int effS = getEffectiveStep(actS);
            auto& effStep = audioProcessor.sequenceData[effS];

            int runLength = 1;
            for (int nextS = s + 1; nextS < stepsPerBar; ++nextS) {
                int nextActS = (editBar * stepsPerBar) + nextS;
                if (getEffectiveStep(nextActS) == effS) runLength++;
                else break;
            }

            float startX = (float)s * stepW;
            float runW = (float)runLength * stepW;
            bool isSelected = false;
            for (int i = 0; i < runLength; ++i) {
                if (getEffectiveStep((editBar * stepsPerBar) + s + i) == getEffectiveStep(selectedStep)) isSelected = true;
            }

            juce::Rectangle<float> rHeader = getStepHeaderBounds(s, stepW).withWidth(runW);
            g.setColour(isSelected ? activeColor.withAlpha(0.3f) : panelBg);
            g.fillRoundedRectangle(rHeader.reduced(1.0f), 4.0f);
            g.setColour(isSelected ? activeColor : textLight);

            juce::String recognizedName = VoicingEngine::getRecognizedChordName(audioProcessor.sequenceData, effS, ppqPerStep);

            if (runW <= 60.0f && recognizedName.contains("(")) {
                juce::String absPart = recognizedName.fromFirstOccurrenceOf("(", false, false).upToFirstOccurrenceOf(")", false, false);
                int splitIdx = -1;
                for (int c = 0; c < absPart.length(); ++c) {
                    if (juce::CharacterFunctions::isDigit(absPart[c])) { splitIdx = c; break; }
                }
                if (splitIdx > 0) {
                    juce::String part1 = absPart.substring(0, splitIdx);
                    juce::String part2 = absPart.substring(splitIdx);
                    g.setFont(juce::Font(12.0f, juce::Font::bold));
                    g.drawText(part1, rHeader.withHeight(rHeader.getHeight() / 2.0f).reduced(2.0f), juce::Justification::centredBottom);
                    g.drawText(part2, rHeader.withY(rHeader.getY() + rHeader.getHeight() / 2.0f).withHeight(rHeader.getHeight() / 2.0f).reduced(2.0f), juce::Justification::centredTop);
                }
                else {
                    g.setFont(juce::Font(12.0f, juce::Font::bold));
                    g.drawFittedText(absPart, rHeader.reduced(2.0f).toNearestInt(), juce::Justification::centred, 1, 0.8f);
                }
            }
            else {
                g.setFont(juce::Font(14.0f, juce::Font::bold));
                g.drawFittedText(recognizedName, rHeader.reduced(2.0f).toNearestInt(), juce::Justification::centred, 2, 0.8f);
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
            g.drawLine((float)s * stepW, 155.0f - 5.0f, (float)s * stepW, 155.0f + (8.0f * cellHeight), (s % stepsPerBeat == 0) ? 2.0f : 1.0f);
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
                g.fillRect((float)localStep * stepW, 155.0f - 5.0f, stepW, 8.0f * cellHeight + 10.0f);
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
                auto& voice = audioProcessor.sequenceData[actS].voices[voiceIdx];

                if (voice.isActive) {
                    auto cell = getCellBounds(s, v, stepW);
                    float safeGate = juce::jlimit(ppqPerStep, 16.0f, audioProcessor.sequenceData[actS].gateLength);
                    cell.setWidth(cell.getWidth() * (safeGate / ppqPerStep));

                    g.setColour(activeColor);
                    g.fillRect(cell.reduced(1.0f, 1.0f));

                    if (audioProcessor.isPlaying && globalStep >= 0 && actS == (globalStep % totalStepsInLoop)) {
                        g.setColour(textLight); g.drawRect(cell.reduced(1.0f, 1.0f), 2.0f);
                    }

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
                        g.setColour(bg); // ★修正：定義した bg を使用
                        g.setFont(juce::Font(14.0f, juce::Font::bold));
                        g.drawText(label, getCellBounds(s, v, stepW), juce::Justification::centred);
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
    }

    void MatrixGridComponent::mouseDown(const juce::MouseEvent& e) {
        if (isProgressionMode) return;

        isDraggingGate = false;
        isDraggingChord = false;
        dragStep = -1;

        int editBar = (int)*audioProcessor.apvts.getRawParameterValue("editBar");
        int stepsPerBar = getStepsPerBar();
        float ppqPerStep = getPpqPerStep();
        float stepW = seqTotalWidth / (float)stepsPerBar;

        bool isLeftClick = e.mods.isLeftButtonDown();
        bool isRightClick = e.mods.isRightButtonDown();

        if (e.y >= 155.0f - 30.0f && e.y < 155.0f - 5.0f) {
            int localStep = (int)((float)e.x / stepW);
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
                int localStep = (int)((float)e.x / stepW);
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
            int localStep = (int)((float)e.x / stepW);
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
                                    isDraggingGate = true; dragStep = actS; dragStartGate = safeGate; dragStartX = (float)e.position.x;
                                    return;
                                }
                                else {
                                    isDraggingChord = true; dragStep = actS; dragStartX = (float)e.position.x; selectedStep = actS;
                                    if (onStepSelected) onStepSelected(selectedStep);
                                    return;
                                }
                            }
                        }
                        else {
                            bool isCovered = false;
                            for (int prevS = actS - 1; prevS >= 0; --prevS) {
                                float dist = (float)(actS - prevS) * ppqPerStep;
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

        for (int i = 0; i < 16; ++i) {
            if (getBarButtonBounds(i).contains(e.position)) {
                if (isLeftClick) {
                    audioProcessor.apvts.getParameter("editBar")->setValueNotifyingHost((float)i / 15.0f);
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
        if (isProgressionMode) { setMouseCursor(juce::MouseCursor::NormalCursor); return; }

        int editBar = (int)*audioProcessor.apvts.getRawParameterValue("editBar");
        int stepsPerBar = getStepsPerBar();
        float ppqPerStep = getPpqPerStep();
        float stepW = seqTotalWidth / (float)stepsPerBar;
        bool hoveringEdge = false;

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
        if (isProgressionMode) return;

        int stepsPerBar = getStepsPerBar();
        float ppqPerStep = getPpqPerStep();
        float stepW = seqTotalWidth / (float)stepsPerBar;

        if (isDraggingGate && dragStep >= 0) {
            float deltaX = (float)e.position.x - dragStartX;
            float maxGate = 16.0f;
            int editBar = (int)*audioProcessor.apvts.getRawParameterValue("editBar");
            int localS = dragStep % stepsPerBar;

            for (int nextS = localS + 1; nextS < stepsPerBar; ++nextS) {
                int actNext = (editBar * stepsPerBar) + nextS;
                for (int v = 0; v < 7; ++v) {
                    if (audioProcessor.sequenceData[actNext].voices[v].isActive) {
                        maxGate = (float)(nextS - localS) * ppqPerStep;
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
            int deltaSteps = std::round(((float)e.position.x - dragStartX) / stepW);
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
                        dragStartX += (float)deltaSteps * stepW;
                        selectedStep = targetStep;
                        if (onStepSelected) onStepSelected(selectedStep);
                    }
                }
            }
        }
    }

    void MatrixGridComponent::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) {
        if (isProgressionMode) return;

        int editBar = (int)*audioProcessor.apvts.getRawParameterValue("editBar");
        int stepsPerBar = getStepsPerBar();
        float stepW = seqTotalWidth / (float)stepsPerBar;

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
// Source/PluginEditor.cpp
#include "PluginProcessor.h"
#include "PluginEditor.h"

ChordMatrixAudioProcessorEditor::ChordMatrixAudioProcessorEditor(ChordMatrixAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(1250, 800);

    auto setupCombo = [this](juce::ComboBox& box, juce::Label& lbl) {
        addAndMakeVisible(box); addAndMakeVisible(lbl);
        box.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff252525));
        box.setColour(juce::ComboBox::textColourId, juce::Colours::white);
        box.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff3a3a3a));
        box.setColour(juce::ComboBox::arrowColourId, juce::Colour(0xffffa500));
        lbl.setColour(juce::Label::textColourId, juce::Colours::grey);
        lbl.attachToComponent(&box, true);
        };

    setupCombo(timeSigNumMenu, timeSigLabel);
    timeSigNumMenu.onChange = [this] { audioProcessor.apvts.getParameter("timeSigNum")->setValueNotifyingHost((timeSigNumMenu.getSelectedId() - 1) / 14.0f); };

    setupCombo(timeSigDenMenu, timeSigSlashLabel);
    timeSigDenMenu.addItem("4", 1); timeSigDenMenu.addItem("8", 2); timeSigDenMenu.addItem("16", 3);
    timeSigDenMenu.onChange = [this] {
        audioProcessor.apvts.getParameter("timeSigDen")->setValueNotifyingHost((timeSigDenMenu.getSelectedId() - 1) / 2.0f);
        updateTimeSigLimits();
        };

    setupCombo(stepSizeMenu, stepSizeLabel);
    stepSizeMenu.addItem("1/4", 1); stepSizeMenu.addItem("1/8", 2); stepSizeMenu.addItem("1/16", 3);
    stepSizeMenu.onChange = [this] { audioProcessor.apvts.getParameter("stepSize")->setValueNotifyingHost((stepSizeMenu.getSelectedId() - 1) / 2.0f); };

    setupCombo(loopBarsMenu, barsLabel);
    for (int i = 1; i <= 16; ++i) loopBarsMenu.addItem(juce::String(i), i);
    loopBarsMenu.onChange = [this] { audioProcessor.apvts.getParameter("loopBars")->setValueNotifyingHost((loopBarsMenu.getSelectedId() - 1) / 15.0f); };

    addAndMakeVisible(tempoSlider); addAndMakeVisible(tempoLabel);
    tempoSlider.setSliderStyle(juce::Slider::LinearBar);
    tempoSlider.setRange(20.0, 300.0, 1.0);
    tempoSlider.setColour(juce::Slider::trackColourId, juce::Colours::transparentBlack);
    tempoSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
    tempoSlider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff252525));
    tempoSlider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff3a3a3a));
    tempoLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
    tempoLabel.attachToComponent(&tempoSlider, true);
    tempoSlider.onValueChange = [this] { audioProcessor.apvts.getParameter("tempo")->setValueNotifyingHost((tempoSlider.getValue() - 20.0f) / 280.0f); };

    auto setupInspector = [this, &setupCombo](juce::ComboBox& box, juce::Label& lbl) {
        setupCombo(box, lbl);
        lbl.attachToComponent(&box, false);
        lbl.setJustificationType(juce::Justification::centred);
        box.onChange = [this, &box, &lbl] {
            if (selectedStep < 0 || selectedStep >= ChordMatrix::TotalSteps) return;
            auto& sData = audioProcessor.sequenceData[selectedStep];
            int val = box.getSelectedId() - 1;
            if (&box == &stepKeyMenu) sData.keyRoot = val;
            else if (&box == &stepDegreeMenu) sData.chordDegree = val;
            else if (&box == &stepTypeMenu) sData.chordType = val;
            else if (&box == &stepTensionMenu) sData.tensionType = val;
            repaint();
            };
        };

    setupInspector(stepKeyMenu, stepKeyLabel);
    for (int i = 0; i < 12; ++i) stepKeyMenu.addItem(ChordMatrix::MusicTheory::getNoteName(i), i + 1);

    setupInspector(stepDegreeMenu, stepDegreeLabel);
    auto degs = ChordMatrix::MusicTheory::getDegreeNames();
    for (int i = 0; i < degs.size(); ++i) stepDegreeMenu.addItem(degs[i], i + 1);

    setupInspector(stepTypeMenu, stepTypeLabel);
    auto typs = ChordMatrix::MusicTheory::getChordTypeNames();
    for (int i = 0; i < typs.size(); ++i) stepTypeMenu.addItem(typs[i], i + 1);

    setupInspector(stepTensionMenu, stepTensionLabel);
    auto tens = ChordMatrix::MusicTheory::getTensionNames();
    for (int i = 0; i < tens.size(); ++i) stepTensionMenu.addItem(tens[i], i + 1);

    timeSigDenMenu.setSelectedId((int)*audioProcessor.apvts.getRawParameterValue("timeSigDen") + 1, juce::dontSendNotification);
    updateTimeSigLimits();
    updateInspector();

    startTimerHz(30);
}

ChordMatrixAudioProcessorEditor::~ChordMatrixAudioProcessorEditor() { stopTimer(); }
void ChordMatrixAudioProcessorEditor::timerCallback() { repaint(); }

void ChordMatrixAudioProcessorEditor::updateTimeSigLimits() {
    int denIdx = timeSigDenMenu.getSelectedId();
    int maxNum = (denIdx == 1) ? 7 : (denIdx == 2) ? 9 : 15;
    int currentSelection = (int)*audioProcessor.apvts.getRawParameterValue("timeSigNum");

    timeSigNumMenu.clear(juce::dontSendNotification);
    for (int i = 1; i <= maxNum; ++i) timeSigNumMenu.addItem(juce::String(i), i);

    if (currentSelection > maxNum) currentSelection = maxNum;
    if (currentSelection < 1) currentSelection = 4;

    timeSigNumMenu.setSelectedId(currentSelection, juce::dontSendNotification);
    audioProcessor.apvts.getParameter("timeSigNum")->setValueNotifyingHost((currentSelection - 1) / 14.0f);
}

void ChordMatrixAudioProcessorEditor::updateInspector() {
    if (selectedStep < 0 || selectedStep >= ChordMatrix::TotalSteps) return;
    auto& sData = audioProcessor.sequenceData[selectedStep];
    stepKeyMenu.setSelectedId(sData.keyRoot + 1, juce::dontSendNotification);
    stepDegreeMenu.setSelectedId(sData.chordDegree + 1, juce::dontSendNotification);
    stepTypeMenu.setSelectedId(sData.chordType + 1, juce::dontSendNotification);
    stepTensionMenu.setSelectedId(sData.tensionType + 1, juce::dontSendNotification);
}

int ChordMatrixAudioProcessorEditor::getStepsPerBar() const {
    int tsNum = (int)*audioProcessor.apvts.getRawParameterValue("timeSigNum");
    int tsDenIdx = (int)*audioProcessor.apvts.getRawParameterValue("timeSigDen");
    int tsDen = (tsDenIdx == 0) ? 4 : (tsDenIdx == 1) ? 8 : 16;
    float ppqPerStep = getPpqPerStep();
    float beatsPerBar = tsNum * (4.0f / tsDen);
    int spb = juce::roundToInt(beatsPerBar / ppqPerStep);
    return spb < 1 ? 1 : spb;
}

float ChordMatrixAudioProcessorEditor::getPpqPerStep() const {
    int stepSizeIdx = (int)*audioProcessor.apvts.getRawParameterValue("stepSize");
    return (stepSizeIdx == 0) ? 1.0f : (stepSizeIdx == 1) ? 0.5f : 0.25f;
}

void ChordMatrixAudioProcessorEditor::resized()
{
    tempoSlider.setBounds(700, 20, 60, 25);
    timeSigNumMenu.setBounds(830, 20, 60, 25);
    timeSigDenMenu.setBounds(905, 20, 60, 25);
    stepSizeMenu.setBounds(1030, 20, 70, 25);
    loopBarsMenu.setBounds(1160, 20, 60, 25);

    int px = 20, py = 120, pw = 160, ph = 30, pySpace = 60;
    stepKeyMenu.setBounds(px, py, pw, ph);
    stepDegreeMenu.setBounds(px, py + pySpace, pw, ph);
    stepTypeMenu.setBounds(px, py + pySpace * 2, pw, ph);
    stepTensionMenu.setBounds(px, py + pySpace * 3, pw, ph);
}

void ChordMatrixAudioProcessorEditor::paint(juce::Graphics& g)
{
    const auto bg = juce::Colour(0xff1a1a1a);
    const auto panelBg = juce::Colour(0xff252525);
    const auto activeColor = juce::Colour(0xffffa500);
    const auto textLight = juce::Colours::white;
    const auto gridLine = juce::Colour(0xff333333);

    g.fillAll(bg);

    g.setColour(panelBg); g.fillRect(0, 0, getWidth(), 65);
    g.setColour(juce::Colour(0xff3a3a3a)); g.drawLine(0, 65, getWidth(), 65, 2.0f);

    g.setColour(textLight); g.setFont(juce::Font(22.0f, juce::Font::bold));
    g.drawText("CHORD MATRIX", 25, 0, 180, 65, juce::Justification::centredLeft);

    timeSigNumMenu.setSelectedId((int)*audioProcessor.apvts.getRawParameterValue("timeSigNum"), juce::dontSendNotification);
    timeSigDenMenu.setSelectedId((int)*audioProcessor.apvts.getRawParameterValue("timeSigDen") + 1, juce::dontSendNotification);
    stepSizeMenu.setSelectedId((int)*audioProcessor.apvts.getRawParameterValue("stepSize") + 1, juce::dontSendNotification);
    loopBarsMenu.setSelectedId((int)*audioProcessor.apvts.getRawParameterValue("loopBars"), juce::dontSendNotification);
    tempoSlider.setValue(*audioProcessor.apvts.getRawParameterValue("tempo"), juce::dontSendNotification);

    g.setColour(juce::Colours::grey); g.setFont(16.0f);
    g.drawText("/", 890, 20, 15, 25, juce::Justification::centred);

    auto drawBtn = [&](int x, int w, juce::String txt, bool active, juce::Colour c) {
        g.setColour(active ? c : juce::Colour(0xff3a3a3a));
        g.fillRoundedRectangle((float)x, 15.0f, (float)w, 35.0f, 4.0f);
        g.setColour(active ? bg : textLight); g.setFont(12.0f);
        g.drawText(txt, x, 15, w, 35, juce::Justification::centred);
        };

    drawBtn(220, 60, "SYNC", audioProcessor.isSyncEnabled, activeColor);
    drawBtn(290, 60, "PLAY", audioProcessor.isInternalPlaying, activeColor);
    drawBtn(360, 60, "STOP", !audioProcessor.isInternalPlaying && !audioProcessor.isSyncEnabled, juce::Colours::red.withAlpha(0.8f));
    drawBtn(430, 80, "OPTIMIZE", false, textLight);

    int editBar = (int)*audioProcessor.apvts.getRawParameterValue("editBar");
    int stepsPerBar = getStepsPerBar();
    float ppqPerStep = getPpqPerStep();
    float stepW = ((float)getWidth() - 260.0f) / stepsPerBar;

    g.setColour(juce::Colour(0xff1c1c1c));
    g.fillRect(0, 65, 200, getHeight() - 65);
    g.setColour(juce::Colours::grey);
    g.setFont(12.0f);
    int currentBarLabel = (selectedStep / stepsPerBar) + 1;
    int currentStepLabel = (selectedStep % stepsPerBar) + 1;
    g.drawText("EDITING: BAR " + juce::String(currentBarLabel) + " / STEP " + juce::String(currentStepLabel),
        20, 80, 160, 20, juce::Justification::centredLeft);

    // 要件: リアルタイム解析されたコードネームを巨大フォントで左下に配置
    juce::String inspectorChordName = ChordMatrix::MusicTheory::getRecognizedChordName(audioProcessor.sequenceData, selectedStep, ppqPerStep);
    g.setColour(activeColor);
    g.setFont(juce::Font(32.0f, juce::Font::bold));
    g.drawFittedText(inspectorChordName, 10, 360, 180, 100, juce::Justification::centred, 2);

    g.setFont(12.0f); g.setColour(juce::Colours::grey);
    juce::String vNames[] = { "13th", "11th", "9th", "7th", "5th", "3rd", "Root" };
    for (int i = 0; i < 7; ++i) g.drawText(vNames[i], 205, 200 + (i * 55), 45, 55, juce::Justification::centredLeft);

    int tsDen = (int)*audioProcessor.apvts.getRawParameterValue("timeSigDen");
    float beatLengthPpq = (tsDen == 0) ? 1.0f : (tsDen == 1) ? 0.5f : 0.25f;
    int stepsPerBeat = juce::roundToInt(beatLengthPpq / ppqPerStep);
    if (stepsPerBeat < 1) stepsPerBeat = 1;

    for (int s = 0; s < stepsPerBar; ++s) {
        if (s % stepsPerBeat == 0) {
            int actS = (editBar * stepsPerBar) + s;

            // 要件: Gateで伸びた和音の上に被っているヘッダーは、元の和音のKEYを継承して表示する
            int effectiveS = actS;
            for (int prevS = actS; prevS >= 0; --prevS) {
                float dist = (actS - prevS) * ppqPerStep;
                bool stepCovers = false;
                for (int v = 0; v < 7; ++v) {
                    if (audioProcessor.sequenceData[prevS].voices[v].isActive &&
                        audioProcessor.sequenceData[prevS].gateLength > dist - 0.001f) {
                        stepCovers = true; break;
                    }
                }
                if (stepCovers) { effectiveS = prevS; break; }
            }

            auto& effectiveStep = audioProcessor.sequenceData[effectiveS];
            auto rK = getStepHeaderBounds(s, 90, stepsPerBar);
            g.setColour(actS == selectedStep ? juce::Colour(0xff444444) : panelBg);
            g.fillRoundedRectangle(rK.reduced(1.0f), 4.0f);
            g.setColour(textLight);
            g.drawText(ChordMatrix::MusicTheory::getNoteName(effectiveStep.keyRoot), rK, juce::Justification::centred);

            auto rC = getStepHeaderBounds(s, 140, stepsPerBar);
            g.setColour(actS == selectedStep ? activeColor.withAlpha(0.3f) : activeColor.withAlpha(0.15f));
            g.fillRoundedRectangle(rC.reduced(1.0f), 4.0f);
            g.setColour(activeColor);

            juce::String recognizedName = ChordMatrix::MusicTheory::getRecognizedChordName(audioProcessor.sequenceData, actS, ppqPerStep);
            g.drawFittedText(recognizedName, rC.reduced(2.0f).toNearestInt(), juce::Justification::centred, 2, 0.7f);
        }

        g.setColour((s % stepsPerBeat == 0) ? juce::Colour(0xff555555) : gridLine);
        g.drawLine(260.0f + (s * stepW), 190.0f, 260.0f + (s * stepW), 600.0f, (s % stepsPerBeat == 0) ? 2.0f : 1.0f);
    }

    int loopIdx = (int)*audioProcessor.apvts.getRawParameterValue("loopBars");
    int numBars = loopIdx;
    int globalStep = audioProcessor.currentGlobalStep;
    int totalStepsInLoop = numBars * stepsPerBar;

    if (audioProcessor.isPlaying && globalStep >= 0) {
        int currentStepInLoop = globalStep % totalStepsInLoop;
        int playingBar = currentStepInLoop / stepsPerBar;
        if (playingBar == editBar) {
            int localStep = currentStepInLoop % stepsPerBar;
            g.setColour(juce::Colours::white.withAlpha(0.15f));
            g.fillRect(260.0f + (localStep * stepW), 190.0f, stepW, 400.0f);
        }
    }

    // 要件: アクティブノートの下には黒いマスを描画しない（黒いフチ・影の完全排除）
    for (int s = 0; s < stepsPerBar; ++s) {
        int actS = (editBar * stepsPerBar) + s;
        for (int v = 0; v < 7; ++v) {
            int voiceIdx = 6 - v;

            bool isCovered = false;
            for (int prevS = actS - 1; prevS >= 0; --prevS) {
                float dist = (actS - prevS) * ppqPerStep;
                if (audioProcessor.sequenceData[prevS].voices[voiceIdx].isActive &&
                    audioProcessor.sequenceData[prevS].gateLength > dist - 0.001f) {
                    isCovered = true; break;
                }
            }

            if (!audioProcessor.sequenceData[actS].voices[voiceIdx].isActive && !isCovered) {
                auto cell = getCellBounds(s, v, stepsPerBar);
                g.setColour(juce::Colour(0xff121212));
                g.fillRect(cell.reduced(1.0f, 1.0f));
            }
        }
    }

    // アクティブなセルを描画（横方向の隙間を0.0fにして背景を完全に覆い隠し、ソリッドな長方形にする）
    for (int s = 0; s < stepsPerBar; ++s) {
        int actS = (editBar * stepsPerBar) + s;
        for (int v = 0; v < 7; ++v) {
            int voiceIdx = 6 - v;
            auto& voice = audioProcessor.sequenceData[actS].voices[voiceIdx];
            if (voice.isActive) {
                auto cell = getCellBounds(s, v, stepsPerBar);
                float gateLen = audioProcessor.sequenceData[actS].gateLength;

                cell.setWidth(cell.getWidth() * (gateLen / ppqPerStep));

                g.setColour(activeColor);
                g.fillRect(cell.reduced(0.0f, 1.0f)); // 横の隙間を消して完全に結合させる

                if (audioProcessor.isPlaying && globalStep >= 0 && actS == (globalStep % totalStepsInLoop)) {
                    g.setColour(textLight); g.drawRect(cell.reduced(0.0f, 1.0f), 2.0f);
                }

                juce::String label = "";
                if (voice.octaveShift != 0) label += (voice.octaveShift > 0 ? "+" : "") + juce::String((int)voice.octaveShift);
                if (voice.accidental == 1) label += (label.isNotEmpty() ? " " : "") + juce::String("#");
                else if (voice.accidental == -1) label += (label.isNotEmpty() ? " " : "") + juce::String("b");

                if (label.isNotEmpty()) {
                    g.setColour(bg); g.setFont(12.0f);
                    g.drawText(label, getCellBounds(s, v, stepsPerBar), juce::Justification::centred);
                }
            }
        }
    }

    for (int i = 0; i < 16; ++i) {
        auto r = getBarButtonBounds(i, numBars);
        bool inLoop = (i < numBars);
        g.setColour(i == editBar ? activeColor : (inLoop ? panelBg : juce::Colour(0xff121212)));
        g.fillRoundedRectangle(r, 4.0f);
        g.setColour(i == editBar ? bg : (inLoop ? textLight : juce::Colours::grey));
        g.drawText("BAR " + juce::String(i + 1), r, juce::Justification::centred);
    }
}

void ChordMatrixAudioProcessorEditor::mouseDown(const juce::MouseEvent& e)
{
    isDraggingGate = false; dragStep = -1;

    if (juce::Rectangle<int>(220, 15, 60, 35).contains(e.getPosition())) audioProcessor.isSyncEnabled = !audioProcessor.isSyncEnabled;
    if (juce::Rectangle<int>(290, 15, 60, 35).contains(e.getPosition())) { audioProcessor.isInternalPlaying = true; audioProcessor.isSyncEnabled = false; }
    if (juce::Rectangle<int>(360, 15, 60, 35).contains(e.getPosition())) {
        if (!audioProcessor.isInternalPlaying && !audioProcessor.isSyncEnabled) {
            audioProcessor.internalPPQ = 0.0;
            audioProcessor.currentGlobalStep = -1;
        }
        else {
            audioProcessor.isInternalPlaying = false;
            audioProcessor.isSyncEnabled = false;
        }
    }

    if (juce::Rectangle<int>(430, 15, 80, 35).contains(e.getPosition())) { audioProcessor.optimizeVoicing(); repaint(); return; }

    int editBar = (int)*audioProcessor.apvts.getRawParameterValue("editBar");
    int stepsPerBar = getStepsPerBar();
    float ppqPerStep = getPpqPerStep();
    float stepW = ((float)getWidth() - 260.0f) / stepsPerBar;

    if (e.x >= 260) {
        if (e.y >= 90 && e.y < 185) {
            int localStep = (int)((e.x - 260.0f) / stepW);
            selectedStep = (editBar * stepsPerBar) + localStep;
            updateInspector();
            repaint();
            return;
        }

        for (int s = 0; s < stepsPerBar; ++s) {
            int actS = (editBar * stepsPerBar) + s;
            for (int v = 0; v < 7; ++v) {
                int voiceIdx = 6 - v;
                auto cell = getCellBounds(s, v, stepsPerBar);

                // Activeな場合のみクリック枠を拡大させる（バグ修正）
                if (audioProcessor.sequenceData[actS].voices[voiceIdx].isActive) {
                    cell.setWidth(cell.getWidth() * (audioProcessor.sequenceData[actS].gateLength / ppqPerStep));
                }

                if (cell.contains(e.position)) {
                    if (audioProcessor.sequenceData[actS].voices[voiceIdx].isActive && e.position.x > cell.getRight() - 10.0f) {
                        isDraggingGate = true; dragStep = actS;
                        dragStartGate = audioProcessor.sequenceData[actS].gateLength;
                        dragStartX = e.position.x; return;
                    }
                    else if (getCellBounds(s, v, stepsPerBar).contains(e.position)) {

                        // 要件: 伸びているGateの「下敷き」になっている場所には新規配置させない
                        bool isCovered = false;
                        for (int prevS = actS - 1; prevS >= 0; --prevS) {
                            float dist = (actS - prevS) * ppqPerStep;
                            if (audioProcessor.sequenceData[prevS].voices[voiceIdx].isActive &&
                                audioProcessor.sequenceData[prevS].gateLength > dist - 0.001f) {
                                isCovered = true; break;
                            }
                        }

                        if (!isCovered) {
                            bool wasActive = audioProcessor.sequenceData[actS].voices[voiceIdx].isActive;
                            audioProcessor.sequenceData[actS].voices[voiceIdx].isActive = !wasActive;

                            // 要件: 新規配置時は完全にナチュラル状態（0）にリセットする
                            if (!wasActive) {
                                audioProcessor.sequenceData[actS].voices[voiceIdx].octaveShift = 0;
                                audioProcessor.sequenceData[actS].voices[voiceIdx].accidental = 0;
                            }

                            selectedStep = actS;
                            updateInspector();
                            repaint();
                            return;
                        }
                    }
                }
            }
        }
    }

    for (int i = 0; i < 16; ++i) if (getBarButtonBounds(i, 16).contains(e.position)) audioProcessor.apvts.getParameter("editBar")->setValueNotifyingHost((float)i / 15.0f);
}

void ChordMatrixAudioProcessorEditor::mouseMove(const juce::MouseEvent& e) {
    int editBar = (int)*audioProcessor.apvts.getRawParameterValue("editBar");
    int stepsPerBar = getStepsPerBar();
    float ppqPerStep = getPpqPerStep();
    bool hoveringEdge = false;

    for (int s = 0; s < stepsPerBar; ++s) {
        int actS = (editBar * stepsPerBar) + s;
        for (int v = 0; v < 7; ++v) {
            int voiceIdx = 6 - v;
            if (audioProcessor.sequenceData[actS].voices[voiceIdx].isActive) {
                auto cell = getCellBounds(s, v, stepsPerBar);
                cell.setWidth(cell.getWidth() * (audioProcessor.sequenceData[actS].gateLength / ppqPerStep));
                if (cell.contains(e.position) && e.position.x > cell.getRight() - 10.0f) {
                    hoveringEdge = true; break;
                }
            }
        }
    }
    setMouseCursor(hoveringEdge ? juce::MouseCursor::LeftRightResizeCursor : juce::MouseCursor::NormalCursor);
}

void ChordMatrixAudioProcessorEditor::mouseDrag(const juce::MouseEvent& e) {
    if (isDraggingGate && dragStep >= 0) {
        int stepsPerBar = getStepsPerBar();
        float stepW = ((float)getWidth() - 260.0f) / stepsPerBar;
        float ppqPerStep = getPpqPerStep();
        float deltaX = e.position.x - dragStartX;

        float maxGate = 16.0f;
        int editBar = (int)*audioProcessor.apvts.getRawParameterValue("editBar");
        int localS = dragStep % stepsPerBar;
        for (int nextS = localS + 1; nextS < stepsPerBar; ++nextS) {
            int actNext = (editBar * stepsPerBar) + nextS;
            for (int v = 0; v < 7; ++v) {
                if (audioProcessor.sequenceData[actNext].voices[v].isActive) {
                    maxGate = (nextS - localS) * ppqPerStep;
                    goto ExitLoop;
                }
            }
        }
    ExitLoop:
        float newGate = dragStartGate + (deltaX / stepW) * ppqPerStep;

        // 要件: ドラッグによるGate長の変更を最小Step単位にスナップさせる
        newGate = std::round(newGate / ppqPerStep) * ppqPerStep;

        audioProcessor.sequenceData[dragStep].gateLength = juce::jlimit(ppqPerStep, maxGate, newGate);
        repaint();
    }
}

void ChordMatrixAudioProcessorEditor::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) {
    int editBar = (int)*audioProcessor.apvts.getRawParameterValue("editBar");
    int stepsPerBar = getStepsPerBar();
    for (int s = 0; s < stepsPerBar; ++s) {
        int actS = (editBar * stepsPerBar) + s;
        for (int v = 0; v < 7; ++v) {
            int voiceIdx = 6 - v;
            if (getCellBounds(s, v, stepsPerBar).contains(e.position) && audioProcessor.sequenceData[actS].voices[voiceIdx].isActive) {
                auto& voice = audioProcessor.sequenceData[actS].voices[voiceIdx];

                if (e.mods.isCtrlDown() || e.mods.isCommandDown()) {
                    if (voiceIdx == 0) return; // Rootはロック
                    voice.accidental = juce::jlimit<int8_t>(-1, 1, voice.accidental + (wheel.deltaY > 0 ? 1 : -1));
                }
                else {
                    voice.octaveShift = juce::jlimit<int8_t>(-2, 2, voice.octaveShift + (wheel.deltaY > 0 ? 1 : -1));
                }

                selectedStep = actS;
                updateInspector();
                repaint();
                return;
            }
        }
    }
}

juce::Rectangle<float> ChordMatrixAudioProcessorEditor::getCellBounds(int s, int v, int spb) {
    return { 260.0f + (float)s * ((getWidth() - 260.0f) / spb), 200.0f + (float)v * 55.0f, (getWidth() - 260.0f) / spb, 52.0f };
}

juce::Rectangle<float> ChordMatrixAudioProcessorEditor::getStepHeaderBounds(int s, int y, int spb) {
    float w = (getWidth() - 260.0f) / spb;
    return { 260.0f + (float)s * w, (float)y, w, 45.0f };
}

juce::Rectangle<float> ChordMatrixAudioProcessorEditor::getBarButtonBounds(int i, int) {
    float w = ((float)getWidth() - 220.0f) / 8.0f;
    return { 220.0f + (float)(i % 8) * w, (float)getHeight() - 120.0f + (float)(i / 8) * 55.0f, w - 8.0f, 48.0f };
}
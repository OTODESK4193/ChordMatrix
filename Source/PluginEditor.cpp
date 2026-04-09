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

    // --- 左パネル ---
    g.setColour(juce::Colour(0xff1c1c1c));
    g.fillRect(0, 65, 200, getHeight() - 65);
    g.setColour(juce::Colours::grey);
    g.setFont(12.0f);
    int currentBarLabel = (selectedStep / stepsPerBar) + 1;
    int currentStepLabel = (selectedStep % stepsPerBar) + 1;
    g.drawText("EDITING: BAR " + juce::String(currentBarLabel) + " / STEP " + juce::String(currentStepLabel),
        20, 80, 160, 20, juce::Justification::centredLeft);

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

    // --- 【修正】キー＆コードレーンの「スマート・グループ化」 ---
    // 隣接するStepでコード設定が同じであれば、結合して1つの広いブロックとして描画する
    for (int s = 0; s < stepsPerBar; ) {
        int actS = (editBar * stepsPerBar) + s;
        auto& step = audioProcessor.sequenceData[actS];

        int runLength = 1;
        for (int nextS = s + 1; nextS < stepsPerBar; ++nextS) {
            int nextActS = (editBar * stepsPerBar) + nextS;
            auto& nextStep = audioProcessor.sequenceData[nextActS];
            // 設定が全く同じならグループ化して幅を広げる
            if (nextStep.keyRoot == step.keyRoot &&
                nextStep.chordDegree == step.chordDegree &&
                nextStep.chordType == step.chordType &&
                nextStep.tensionType == step.tensionType) {
                runLength++;
            }
            else {
                break;
            }
        }

        float startX = 260.0f + (s * stepW);
        float runW = runLength * stepW;

        bool isSelected = false;
        for (int i = 0; i < runLength; ++i) {
            if ((actS + i) == selectedStep) isSelected = true;
        }

        juce::Rectangle<float> rK(startX, 90.0f, runW, 45.0f);
        g.setColour(isSelected ? juce::Colour(0xff444444) : panelBg);
        g.fillRoundedRectangle(rK.reduced(1.0f), 4.0f);
        g.setColour(textLight);
        g.drawText(ChordMatrix::MusicTheory::getNoteName(step.keyRoot), rK, juce::Justification::centred);

        juce::Rectangle<float> rC(startX, 140.0f, runW, 45.0f);
        g.setColour(isSelected ? activeColor.withAlpha(0.3f) : activeColor.withAlpha(0.15f));
        g.fillRoundedRectangle(rC.reduced(1.0f), 4.0f);
        g.setColour(activeColor);

        juce::String recognizedName = ChordMatrix::MusicTheory::getRecognizedChordName(audioProcessor.sequenceData, actS, ppqPerStep);
        g.drawFittedText(recognizedName, rC.reduced(2.0f).toNearestInt(), juce::Justification::centred, 2, 0.7f);

        s += runLength; // 結合した分だけStepを進める
    }

    // グリッド線（背景よりも手前に描画）
    for (int s = 0; s <= stepsPerBar; ++s) {
        g.setColour((s % stepsPerBeat == 0) ? juce::Colour(0xff555555) : gridLine);
        g.drawLine(260.0f + (s * stepW), 190.0f, 260.0f + (s * stepW), 600.0f, (s % stepsPerBeat == 0) ? 2.0f : 1.0f);
    }

    int loopIdx = (int)*audioProcessor.apvts.getRawParameterValue("loopBars");
    int numBars = loopIdx;
    int globalStep = audioProcessor.currentGlobalStep;
    int totalStepsInLoop = numBars * stepsPerBar;

    // 再生位置ハイライト
    if (audioProcessor.isPlaying && globalStep >= 0) {
        int currentStepInLoop = globalStep % totalStepsInLoop;
        int playingBar = currentStepInLoop / stepsPerBar;
        if (playingBar == editBar) {
            int localStep = currentStepInLoop % stepsPerBar;
            g.setColour(juce::Colours::white.withAlpha(0.15f));
            g.fillRect(260.0f + (localStep * stepW), 190.0f, stepW, 400.0f);
        }
    }

    // --- パス1: 全ての非アクティブセルを描画 ---
    for (int s = 0; s < stepsPerBar; ++s) {
        int actS = (editBar * stepsPerBar) + s;
        for (int v = 0; v < 7; ++v) {
            auto cell = getCellBounds(s, v, stepsPerBar);
            g.setColour(juce::Colour(0xff121212));
            // 常に1pxの余白を持たせる
            g.fillRect(cell.reduced(1.0f, 1.0f));
        }
    }

    // --- パス2: アクティブセルを描画 ---
    for (int s = 0; s < stepsPerBar; ++s) {
        int actS = (editBar * stepsPerBar) + s;
        for (int v = 0; v < 7; ++v) {
            int voiceIdx = 6 - v;
            auto& voice = audioProcessor.sequenceData[actS].voices[voiceIdx];
            if (voice.isActive) {
                auto cell = getCellBounds(s, v, stepsPerBar);
                float safeGate = juce::jlimit(ppqPerStep, 16.0f, audioProcessor.sequenceData[actS].gateLength);

                // Gate長に応じて幅を拡張する
                cell.setWidth(cell.getWidth() * (safeGate / ppqPerStep));

                g.setColour(activeColor);

                // 【修正】 拡張された全体の枠に対して 1pxの余白 を適用する
                // これにより「単音は独立して見え、伸ばした音は癒着する」理想の挙動になる
                g.fillRect(cell.reduced(1.0f, 1.0f));

                if (audioProcessor.isPlaying && globalStep >= 0 && actS == (globalStep % totalStepsInLoop)) {
                    g.setColour(textLight); g.drawRect(cell.reduced(1.0f, 1.0f), 2.0f);
                }

                juce::String label = "";
                if (voice.octaveShift != 0) label += (voice.octaveShift > 0 ? "+" : "") + juce::String((int)voice.octaveShift);
                if (voice.accidental == 1) label += (label.isNotEmpty() ? " " : "") + juce::String("#");
                else if (voice.accidental == -1) label += (label.isNotEmpty() ? " " : "") + juce::String("b");

                if (label.isNotEmpty()) {
                    g.setColour(bg); g.setFont(12.0f);
                    // ラベルは最初の1マス分の領域に中央揃えで描画する
                    g.drawText(label, getCellBounds(s, v, stepsPerBar), juce::Justification::centred);
                }
            }
        }
    }

    // Bar Navigation
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

        if (e.y >= 190) {
            for (int s = 0; s < stepsPerBar; ++s) {
                int actS = (editBar * stepsPerBar) + s;
                for (int v = 0; v < 7; ++v) {
                    int voiceIdx = 6 - v;
                    auto cell = getCellBounds(s, v, stepsPerBar);

                    float safeGate = juce::jlimit(ppqPerStep, 16.0f, audioProcessor.sequenceData[actS].gateLength);

                    if (audioProcessor.sequenceData[actS].voices[voiceIdx].isActive) {
                        cell.setWidth(cell.getWidth() * (safeGate / ppqPerStep));
                    }

                    if (cell.contains(e.position)) {
                        if (audioProcessor.sequenceData[actS].voices[voiceIdx].isActive && e.position.x > cell.getRight() - 10.0f) {
                            isDraggingGate = true; dragStep = actS;
                            dragStartGate = safeGate;
                            dragStartX = e.position.x; return;
                        }
                        else {
                            bool isCovered = false;
                            for (int prevS = actS - 1; prevS >= 0; --prevS) {
                                float dist = (actS - prevS) * ppqPerStep;
                                if (audioProcessor.sequenceData[prevS].voices[voiceIdx].isActive &&
                                    audioProcessor.sequenceData[prevS].gateLength > dist + 0.001f) {
                                    isCovered = true; break;
                                }
                            }

                            if (!isCovered) {
                                bool wasActive = audioProcessor.sequenceData[actS].voices[voiceIdx].isActive;
                                audioProcessor.sequenceData[actS].voices[voiceIdx].isActive = !wasActive;

                                if (!wasActive) {
                                    audioProcessor.sequenceData[actS].voices[voiceIdx].octaveShift = 0;
                                    audioProcessor.sequenceData[actS].voices[voiceIdx].accidental = 0;
                                    audioProcessor.sequenceData[actS].gateLength = ppqPerStep;
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
    }

    for (int i = 0; i < 16; ++i) if (getBarButtonBounds(i, 16).contains(e.position)) audioProcessor.apvts.getParameter("editBar")->setValueNotifyingHost((float)i / 15.0f);
}

void ChordMatrixAudioProcessorEditor::mouseMove(const juce::MouseEvent& e) {
    int editBar = (int)*audioProcessor.apvts.getRawParameterValue("editBar");
    int stepsPerBar = getStepsPerBar();
    float ppqPerStep = getPpqPerStep();
    bool hoveringEdge = false;

    if (e.y >= 190 && e.x >= 260) {
        for (int s = 0; s < stepsPerBar; ++s) {
            int actS = (editBar * stepsPerBar) + s;
            for (int v = 0; v < 7; ++v) {
                int voiceIdx = 6 - v;
                if (audioProcessor.sequenceData[actS].voices[voiceIdx].isActive) {
                    auto cell = getCellBounds(s, v, stepsPerBar);
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
        newGate = std::round(newGate / ppqPerStep) * ppqPerStep;
        audioProcessor.sequenceData[dragStep].gateLength = juce::jlimit(ppqPerStep, maxGate, newGate);
        repaint();
    }
}

void ChordMatrixAudioProcessorEditor::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) {
    int editBar = (int)*audioProcessor.apvts.getRawParameterValue("editBar");
    int stepsPerBar = getStepsPerBar();
    if (e.y >= 190 && e.x >= 260) {
        for (int s = 0; s < stepsPerBar; ++s) {
            int actS = (editBar * stepsPerBar) + s;
            for (int v = 0; v < 7; ++v) {
                int voiceIdx = 6 - v;
                if (getCellBounds(s, v, stepsPerBar).contains(e.position) && audioProcessor.sequenceData[actS].voices[voiceIdx].isActive) {
                    auto& voice = audioProcessor.sequenceData[actS].voices[voiceIdx];
                    if (e.mods.isCtrlDown() || e.mods.isCommandDown()) {
                        if (voiceIdx == 0) return;
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
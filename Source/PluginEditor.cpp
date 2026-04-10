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
        lbl.setJustificationType(juce::Justification::centred);
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
    loopBarsMenu.addItem("1", 1); loopBarsMenu.addItem("4", 2); loopBarsMenu.addItem("8", 3); loopBarsMenu.addItem("12", 4); loopBarsMenu.addItem("16", 5);
    loopBarsMenu.onChange = [this] { audioProcessor.apvts.getParameter("loopBars")->setValueNotifyingHost((loopBarsMenu.getSelectedId() - 1) / 4.0f); };

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

    // インスペクターのコンボボックス設定
    setupCombo(stepKeyMenu, stepKeyLabel);
    for (int i = 0; i < 12; ++i) stepKeyMenu.addItem(ChordMatrix::MusicTheory::getNoteName(i), i + 1);
    stepKeyMenu.onChange = [this] {
        if (selectedStep >= 0 && selectedStep < ChordMatrix::TotalSteps) {
            audioProcessor.sequenceData[selectedStep].keyRoot = stepKeyMenu.getSelectedId() - 1;
            repaint();
        }
        };

    setupCombo(stepScaleMenu, stepScaleLabel);
    auto scales = ChordMatrix::MusicTheory::getScaleNames();
    for (int i = 0; i < scales.size(); ++i) stepScaleMenu.addItem(scales[i], i + 1);
    stepScaleMenu.onChange = [this] {
        if (selectedStep >= 0 && selectedStep < ChordMatrix::TotalSteps) {
            audioProcessor.sequenceData[selectedStep].scaleType = stepScaleMenu.getSelectedId() - 1;
            repaint();
        }
        };

    setupCombo(stepDegreeMenu, stepDegreeLabel);
    auto degs = ChordMatrix::MusicTheory::getDegreeNames();
    for (int i = 0; i < degs.size(); ++i) stepDegreeMenu.addItem(degs[i], i + 1);
    stepDegreeMenu.onChange = [this] {
        if (selectedStep >= 0 && selectedStep < ChordMatrix::TotalSteps) {
            audioProcessor.sequenceData[selectedStep].chordDegree = stepDegreeMenu.getSelectedId() - 1;
            repaint();
        }
        };

    setupCombo(voicingMenu, voicingLabel);
    voicingMenu.addItem("Close", 1); voicingMenu.addItem("Drop 2", 2); voicingMenu.addItem("Drop 3", 3); voicingMenu.addItem("Spread", 4);
    voicingMenu.onChange = [this] {
        audioProcessor.apvts.getParameter("voicingMode")->setValueNotifyingHost((voicingMenu.getSelectedId() - 1) / 3.0f);
        repaint();
        };

    timeSigDenMenu.setSelectedId((int)*audioProcessor.apvts.getRawParameterValue("timeSigDen") + 1, juce::dontSendNotification);
    voicingMenu.setSelectedId((int)*audioProcessor.apvts.getRawParameterValue("voicingMode") + 1, juce::dontSendNotification);
    updateTimeSigLimits();
    updateInspector();

    startTimerHz(30);
}

ChordMatrixAudioProcessorEditor::~ChordMatrixAudioProcessorEditor() { stopTimer(); }

void ChordMatrixAudioProcessorEditor::timerCallback() {
    if (audioProcessor.isPlaying && isFollowMode && audioProcessor.currentGlobalStep >= 0) {
        int stepsPerBar = getStepsPerBar();
        int loopIdx = (int)*audioProcessor.apvts.getRawParameterValue("loopBars");
        constexpr std::array<int, 5> barsMap = { 1, 4, 8, 12, 16 };
        int numBars = barsMap[loopIdx];
        int totalStepsInLoop = numBars * stepsPerBar;

        int currentStepInLoop = audioProcessor.currentGlobalStep % totalStepsInLoop;
        int playingBar = currentStepInLoop / stepsPerBar;
        int editBar = (int)*audioProcessor.apvts.getRawParameterValue("editBar");

        if (playingBar != editBar && playingBar < 16) {
            audioProcessor.apvts.getParameter("editBar")->setValueNotifyingHost((float)playingBar / 15.0f);
        }
    }
    repaint();
}

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
    stepScaleMenu.setSelectedId(sData.scaleType + 1, juce::dontSendNotification);
    stepDegreeMenu.setSelectedId(sData.chordDegree + 1, juce::dontSendNotification);
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

int ChordMatrixAudioProcessorEditor::getEffectiveStep(int targetS) const {
    int eff = targetS;
    float ppq = getPpqPerStep();
    for (int prevS = targetS; prevS >= 0; --prevS) {
        float dist = (targetS - prevS) * ppq;
        bool covers = false;
        for (int v = 0; v < 7; ++v) {
            if (audioProcessor.sequenceData[prevS].voices[v].isActive &&
                audioProcessor.sequenceData[prevS].gateLength > dist + 0.001f) {
                covers = true; break;
            }
        }
        if (covers) { eff = prevS; break; }
    }
    return eff;
}

void ChordMatrixAudioProcessorEditor::resized()
{
    tempoSlider.setBounds(700, 20, 60, 25);
    timeSigNumMenu.setBounds(830, 20, 60, 25);
    timeSigDenMenu.setBounds(905, 20, 60, 25);
    stepSizeMenu.setBounds(1030, 20, 70, 25);
    loopBarsMenu.setBounds(1160, 20, 60, 25);

    int px = 80, py = 180, pw = 120, ph = 30, pySpace = 50;
    stepKeyMenu.setBounds(px, py, pw, ph);
    stepScaleMenu.setBounds(px, py + pySpace, pw, ph);
    stepDegreeMenu.setBounds(px, py + pySpace * 2, pw, ph);
    voicingMenu.setBounds(px, py + pySpace * 3 + 20, pw, ph);
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
    g.drawText(juce::String("CHORD MATRIX"), 25, 0, 180, 65, juce::Justification::centredLeft);

    timeSigNumMenu.setSelectedId((int)*audioProcessor.apvts.getRawParameterValue("timeSigNum"), juce::dontSendNotification);
    timeSigDenMenu.setSelectedId((int)*audioProcessor.apvts.getRawParameterValue("timeSigDen") + 1, juce::dontSendNotification);
    stepSizeMenu.setSelectedId((int)*audioProcessor.apvts.getRawParameterValue("stepSize") + 1, juce::dontSendNotification);
    loopBarsMenu.setSelectedId((int)*audioProcessor.apvts.getRawParameterValue("loopBars") + 1, juce::dontSendNotification);
    tempoSlider.setValue(*audioProcessor.apvts.getRawParameterValue("tempo"), juce::dontSendNotification);

    g.setColour(juce::Colours::grey); g.setFont(16.0f);
    g.drawText(juce::String("/"), 890, 20, 15, 25, juce::Justification::centred);

    auto drawBtn = [&](int x, int y, int w, int h, const char* txt, bool active, juce::Colour c) {
        g.setColour(active ? c : juce::Colour(0xff3a3a3a));
        g.fillRoundedRectangle((float)x, (float)y, (float)w, (float)h, 4.0f);
        g.setColour(active ? bg : textLight); g.setFont(12.0f);
        g.drawText(juce::String(txt), x, y, w, h, juce::Justification::centred);
        };

    drawBtn(220, 15, 60, 35, "SYNC", audioProcessor.isSyncEnabled, activeColor);
    drawBtn(290, 15, 60, 35, "PLAY", audioProcessor.isInternalPlaying, activeColor);
    drawBtn(360, 15, 60, 35, "STOP", !audioProcessor.isInternalPlaying && !audioProcessor.isSyncEnabled, juce::Colours::red.withAlpha(0.8f));
    drawBtn(430, 15, 60, 35, "FOLLOW", isFollowMode, activeColor);
    drawBtn(500, 15, 80, 35, "OPTIMIZE", false, textLight);

    drawBtn(gridX, 80, 120, 30, "DRAG MIDI", false, juce::Colours::cyan.withAlpha(0.6f));

    int editBar = (int)*audioProcessor.apvts.getRawParameterValue("editBar");
    int stepsPerBar = getStepsPerBar();
    float ppqPerStep = getPpqPerStep();
    int voicingMode = (int)*audioProcessor.apvts.getRawParameterValue("voicingMode");

    g.setColour(juce::Colour(0xff1c1c1c));
    g.fillRect(0, 65, 220, getHeight() - 65);

    // --- インスペクター上部の現在位置とプレビュー音名表示 ---
    int dispBar = (selectedStep / stepsPerBar) + 1;
    int dispStep = (selectedStep % stepsPerBar) + 1;
    g.setColour(juce::Colours::grey);
    g.setFont(14.0f);
    g.drawText("BAR " + juce::String(dispBar) + " / STEP " + juce::String(dispStep), 20, 80, 180, 20, juce::Justification::centredLeft);

    if (selectedStep >= 0 && selectedVoice >= 0 && selectedVoice < 7) {
        int vIdx = 6 - selectedVoice;
        auto& sData = audioProcessor.sequenceData[selectedStep];
        if (sData.voices[vIdx].isActive) {
            int pitch = ChordMatrix::MusicTheory::getBasePitch(sData, vIdx) + sData.voices[vIdx].accidental + (sData.voices[vIdx].octaveShift * 12);
            juce::String noteName = ChordMatrix::MusicTheory::getNoteName(pitch);
            int oct = (pitch / 12) - 1;
            g.setColour(activeColor);
            g.setFont(juce::Font(16.0f, juce::Font::bold));
            g.drawText("NOTE: " + noteName + juce::String(oct), 20, 105, 180, 20, juce::Justification::centredLeft);
        }
    }

    g.setColour(juce::Colours::grey);
    g.setFont(12.0f);
    g.drawText(juce::String("BASE SCALE SETTINGS"), 20, 140, 180, 20, juce::Justification::centredLeft);

    juce::String inspectorChordName = ChordMatrix::MusicTheory::getRecognizedChordName(audioProcessor.sequenceData, selectedStep, ppqPerStep, voicingMode);
    g.setColour(activeColor);
    g.setFont(juce::Font(32.0f, juce::Font::bold));
    g.drawFittedText(inspectorChordName, 10, 420, 200, 100, juce::Justification::centred, 2);

    g.setFont(12.0f); g.setColour(juce::Colours::grey);
    g.drawText("INV", 180, gridY - 30.0f, 50, 25, juce::Justification::centredRight);

    const char* vNames[] = { "6/13th", "4/11th", "2/9th", "7th", "5th", "3rd", "Root" };
    for (int i = 0; i < 7; ++i) g.drawText(juce::String(vNames[i]), 180, gridY + (i * cellHeight), 50, cellHeight, juce::Justification::centredRight);

    g.setColour(juce::Colours::indianred);
    g.drawText(juce::String("DEL"), 180, gridY + (7 * cellHeight), 50, 30, juce::Justification::centredRight);

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

        float startX = gridX + (s * stepW);
        float runW = runLength * stepW;
        bool isSelected = false;
        for (int i = 0; i < runLength; ++i) {
            if (getEffectiveStep((editBar * stepsPerBar) + s + i) == getEffectiveStep(selectedStep)) isSelected = true;
        }

        // Header (コード名表示)
        juce::Rectangle<float> rHeader(startX, gridY - headerHeight - 35.0f, runW, headerHeight - 5.0f);
        g.setColour(isSelected ? activeColor.withAlpha(0.3f) : panelBg);
        g.fillRoundedRectangle(rHeader.reduced(1.0f), 4.0f);
        g.setColour(isSelected ? activeColor : textLight);

        juce::String recognizedName = ChordMatrix::MusicTheory::getRecognizedChordName(audioProcessor.sequenceData, effS, ppqPerStep, voicingMode);
        g.setFont(juce::Font(14.0f, juce::Font::bold));
        g.drawFittedText(recognizedName, rHeader.reduced(2.0f).toNearestInt(), juce::Justification::centred, 2, 0.8f);

        // Inversion (展開形) ボタンレーン
        juce::Rectangle<float> rInv(startX, gridY - 30.0f, runW, 25.0f);
        g.setColour(juce::Colour(0xff2a2a2a));
        g.fillRoundedRectangle(rInv.reduced(1.0f), 4.0f);
        g.setColour(juce::Colours::lightgreen);
        g.setFont(12.0f);
        g.drawText("INV: " + juce::String(effStep.inversion), rInv, juce::Justification::centred);

        // DEL ボタン
        juce::Rectangle<float> rDel(startX, gridY + (7 * cellHeight), runW, 30.0f);
        g.setColour(juce::Colour(0xff331111));
        g.fillRoundedRectangle(rDel.reduced(1.0f), 4.0f);
        g.setColour(juce::Colours::red.withAlpha(0.7f));
        g.drawText(juce::String("X"), rDel, juce::Justification::centred);

        s += runLength;
    }

    for (int s = 0; s <= stepsPerBar; ++s) {
        g.setColour((s % stepsPerBeat == 0) ? juce::Colour(0xff555555) : gridLine);
        g.drawLine(gridX + (s * stepW), gridY - 5.0f, gridX + (s * stepW), gridY + (8 * cellHeight), (s % stepsPerBeat == 0) ? 2.0f : 1.0f);
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
            g.fillRect(gridX + (localStep * stepW), gridY - 5.0f, stepW, 8 * cellHeight + 10.0f);
        }
    }

    for (int s = 0; s < stepsPerBar; ++s) {
        for (int v = 0; v < 7; ++v) {
            auto cell = getCellBounds(s, v);
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
                auto cell = getCellBounds(s, v);
                float safeGate = juce::jlimit(ppqPerStep, 16.0f, audioProcessor.sequenceData[actS].gateLength);
                cell.setWidth(cell.getWidth() * (safeGate / ppqPerStep));

                g.setColour(activeColor);
                g.fillRect(cell.reduced(1.0f, 1.0f));

                if (audioProcessor.isPlaying && globalStep >= 0 && actS == (globalStep % totalStepsInLoop)) {
                    g.setColour(textLight); g.drawRect(cell.reduced(1.0f, 1.0f), 2.0f);
                }

                juce::String label;
                if (voice.octaveShift != 0) {
                    label << (voice.octaveShift > 0 ? "+" : "");
                    label << (int)voice.octaveShift;
                }
                if (voice.accidental > 0) {
                    for (int i = 0; i < voice.accidental; ++i) {
                        if (label.isNotEmpty()) label << " ";
                        label << "#";
                    }
                }
                else if (voice.accidental < 0) {
                    for (int i = 0; i < std::abs(voice.accidental); ++i) {
                        if (label.isNotEmpty()) label << " ";
                        label << "b";
                    }
                }

                if (label.isNotEmpty()) {
                    g.setColour(bg);
                    g.setFont(juce::Font(14.0f, juce::Font::bold));
                    g.drawText(label, getCellBounds(s, v), juce::Justification::centred);
                }
            }
        }
    }

    for (int i = 0; i < 16; ++i) {
        auto r = getBarButtonBounds(i, numBars, stepsPerBar);
        bool inLoop = (i < numBars);
        g.setColour(i == editBar ? activeColor : (inLoop ? panelBg : juce::Colour(0xff121212)));
        g.fillRoundedRectangle(r, 4.0f);
        g.setColour(i == editBar ? bg : (inLoop ? textLight : juce::Colours::grey));
        g.setFont(juce::Font(18.0f, juce::Font::bold));
        g.drawText(juce::String("BAR ") + juce::String(i + 1), r, juce::Justification::centred);
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

    if (juce::Rectangle<int>(430, 15, 60, 35).contains(e.getPosition())) { isFollowMode = !isFollowMode; repaint(); return; }
    if (juce::Rectangle<int>(500, 15, 80, 35).contains(e.getPosition())) { audioProcessor.optimizeVoicing(); repaint(); return; }

    int editBar = (int)*audioProcessor.apvts.getRawParameterValue("editBar");
    int stepsPerBar = getStepsPerBar();
    float ppqPerStep = getPpqPerStep();

    bool isLeftClick = e.mods.isLeftButtonDown();
    bool isRightClick = e.mods.isRightButtonDown();

    if (e.x >= gridX) {

        // Inversion (展開形) クリック判定
        if (e.y >= gridY - 30.0f && e.y < gridY - 5.0f) {
            int localStep = (int)((e.x - gridX) / stepW);
            if (localStep < stepsPerBar) {
                int clickedActS = (editBar * stepsPerBar) + localStep;
                int effS = getEffectiveStep(clickedActS);
                if (isLeftClick) audioProcessor.sequenceData[effS].inversion++;
                else if (isRightClick) audioProcessor.sequenceData[effS].inversion = std::max(0, audioProcessor.sequenceData[effS].inversion - 1);
                selectedStep = effS;
                updateInspector();
                repaint();
            }
            return;
        }

        // DELボタン（左クリックで削除、Gate長保持）
        if (e.y >= gridY + 7 * cellHeight && e.y < gridY + 7 * cellHeight + 30) {
            if (isLeftClick) {
                int localStep = (int)((e.x - gridX) / stepW);
                if (localStep < stepsPerBar) {
                    int clickedActS = (editBar * stepsPerBar) + localStep;
                    int effS = getEffectiveStep(clickedActS);

                    for (int v = 0; v < 7; ++v) {
                        audioProcessor.sequenceData[effS].voices[v].isActive = false;
                        audioProcessor.sequenceData[effS].voices[v].octaveShift = 0;
                        audioProcessor.sequenceData[effS].voices[v].accidental = 0;
                    }
                    selectedStep = effS;
                    updateInspector();
                    repaint();
                }
            }
            return;
        }

        // Header部分（クリックで選択のみ）
        if (e.y >= gridY - headerHeight - 35.0f && e.y < gridY - 35.0f) {
            int localStep = (int)((e.x - gridX) / stepW);
            if (localStep < stepsPerBar) {
                int clickedActS = (editBar * stepsPerBar) + localStep;
                selectedStep = getEffectiveStep(clickedActS);
                updateInspector();
                repaint();
            }
            return;
        }

        // Grid (Note)
        if (e.y >= gridY && e.y < gridY + 7 * cellHeight) {
            for (int s = 0; s < stepsPerBar; ++s) {
                int actS = (editBar * stepsPerBar) + s;
                for (int v = 0; v < 7; ++v) {
                    int voiceIdx = 6 - v;
                    auto cell = getCellBounds(s, v);

                    float safeGate = juce::jlimit(ppqPerStep, 16.0f, audioProcessor.sequenceData[actS].gateLength);
                    if (audioProcessor.sequenceData[actS].voices[voiceIdx].isActive) {
                        cell.setWidth(cell.getWidth() * (safeGate / ppqPerStep));
                    }

                    if (cell.contains(e.position)) {
                        if (audioProcessor.sequenceData[actS].voices[voiceIdx].isActive && e.position.x > cell.getRight() - 10.0f) {
                            if (isLeftClick || isRightClick) {
                                isDraggingGate = true; dragStep = actS;
                                dragStartGate = safeGate;
                                dragStartX = e.position.x;
                                return;
                            }
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

                            if (isLeftClick) {
                                if (!isCovered) {
                                    audioProcessor.sequenceData[actS].voices[voiceIdx].isActive = true;
                                }
                                selectedStep = actS;
                                selectedVoice = v; // 音名表示用

                                // アトミック変数による安全なプレビュー発音トリガー
                                auto& stepData = audioProcessor.sequenceData[actS];
                                int pitch = ChordMatrix::MusicTheory::getBasePitch(stepData, voiceIdx) + stepData.voices[voiceIdx].accidental + (stepData.voices[voiceIdx].octaveShift * 12);
                                audioProcessor.previewNoteOn.store(juce::jlimit(0, 127, pitch));

                                updateInspector();
                                repaint();
                                return;
                            }
                            else if (isRightClick) {
                                if (!isCovered) {
                                    audioProcessor.sequenceData[actS].voices[voiceIdx].isActive = false;
                                    audioProcessor.sequenceData[actS].voices[voiceIdx].octaveShift = 0;
                                    audioProcessor.sequenceData[actS].voices[voiceIdx].accidental = 0;
                                    selectedStep = actS;
                                    selectedVoice = -1;
                                    updateInspector();
                                    repaint();
                                }
                                return;
                            }
                        }
                    }
                }
            }
        }
    }

    int loopIdx = (int)*audioProcessor.apvts.getRawParameterValue("loopBars");
    constexpr std::array<int, 5> barsMap = { 1, 4, 8, 12, 16 };
    int numBars = barsMap[loopIdx];
    for (int i = 0; i < 16; ++i) if (getBarButtonBounds(i, numBars, stepsPerBar).contains(e.position)) audioProcessor.apvts.getParameter("editBar")->setValueNotifyingHost((float)i / 15.0f);
}

void ChordMatrixAudioProcessorEditor::mouseMove(const juce::MouseEvent& e) {
    int editBar = (int)*audioProcessor.apvts.getRawParameterValue("editBar");
    int stepsPerBar = getStepsPerBar();
    float ppqPerStep = getPpqPerStep();
    bool hoveringEdge = false;

    if (e.y >= gridY && e.y < gridY + 7 * cellHeight && e.x >= gridX) {
        for (int s = 0; s < stepsPerBar; ++s) {
            int actS = (editBar * stepsPerBar) + s;
            for (int v = 0; v < 7; ++v) {
                int voiceIdx = 6 - v;
                if (audioProcessor.sequenceData[actS].voices[voiceIdx].isActive) {
                    auto cell = getCellBounds(s, v);
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
    if (e.y >= gridY && e.y < gridY + 7 * cellHeight && e.x >= gridX) {
        for (int s = 0; s < stepsPerBar; ++s) {
            int actS = (editBar * stepsPerBar) + s;
            for (int v = 0; v < 7; ++v) {
                int voiceIdx = 6 - v;
                if (getCellBounds(s, v).contains(e.position) && audioProcessor.sequenceData[actS].voices[voiceIdx].isActive) {
                    auto& voice = audioProcessor.sequenceData[actS].voices[voiceIdx];
                    if (e.mods.isCtrlDown() || e.mods.isCommandDown()) {
                        voice.accidental = juce::jlimit<int8_t>(-2, 2, voice.accidental + (wheel.deltaY > 0 ? 1 : -1));
                    }
                    else {
                        voice.octaveShift = juce::jlimit<int8_t>(-2, 2, voice.octaveShift + (wheel.deltaY > 0 ? 1 : -1));
                    }
                    selectedStep = actS;
                    selectedVoice = v;
                    updateInspector();
                    repaint();
                    return;
                }
            }
        }
    }
}

juce::Rectangle<float> ChordMatrixAudioProcessorEditor::getCellBounds(int s, int v) {
    return { gridX + (float)s * stepW, gridY + (float)v * cellHeight, stepW, cellHeight - 2.0f };
}

juce::Rectangle<float> ChordMatrixAudioProcessorEditor::getStepHeaderBounds(int s) {
    return { gridX + (float)s * stepW, gridY - headerHeight - 35.0f, stepW, headerHeight - 5.0f };
}

juce::Rectangle<float> ChordMatrixAudioProcessorEditor::getBarButtonBounds(int i, int, int spb) {
    float totalSeqWidth = (float)spb * stepW;
    float w = totalSeqWidth / 8.0f;
    return { gridX + (float)(i % 8) * w, 620.0f + (float)(i / 8) * 80.0f, w - 8.0f, 60.0f };
}
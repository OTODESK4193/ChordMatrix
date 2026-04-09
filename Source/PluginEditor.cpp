// Source/PluginEditor.cpp
#include "PluginProcessor.h"
#include "PluginEditor.h"

ChordMatrixAudioProcessorEditor::ChordMatrixAudioProcessorEditor(ChordMatrixAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(1100, 800);

    auto setupCombo = [this](juce::ComboBox& box, juce::Label& lbl) {
        addAndMakeVisible(box); addAndMakeVisible(lbl);
        // 明るいテーマ用のUIカラー
        box.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xffffffff));
        box.setColour(juce::ComboBox::textColourId, juce::Colours::black);
        box.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xffcccccc));
        lbl.setColour(juce::Label::textColourId, juce::Colours::darkgrey);
        lbl.attachToComponent(&box, true);
        };

    setupCombo(timeSigNumMenu, timeSigLabel);
    for (int i = 4; i <= 15; ++i) timeSigNumMenu.addItem(juce::String(i), i);
    timeSigNumMenu.onChange = [this] { audioProcessor.apvts.getParameter("timeSigNum")->setValueNotifyingHost((timeSigNumMenu.getSelectedId() - 4) / 11.0f); };

    setupCombo(timeSigDenMenu, *new juce::Label());
    timeSigDenMenu.addItem("4", 1); timeSigDenMenu.addItem("8", 2); timeSigDenMenu.addItem("16", 3);
    timeSigDenMenu.onChange = [this] { audioProcessor.apvts.getParameter("timeSigDen")->setValueNotifyingHost((timeSigDenMenu.getSelectedId() - 1) / 2.0f); };

    setupCombo(stepSizeMenu, stepSizeLabel);
    stepSizeMenu.addItem("1/4", 1); stepSizeMenu.addItem("1/8", 2); stepSizeMenu.addItem("1/16", 3);
    stepSizeMenu.onChange = [this] { audioProcessor.apvts.getParameter("stepSize")->setValueNotifyingHost((stepSizeMenu.getSelectedId() - 1) / 2.0f); };

    setupCombo(loopBarsMenu, barsLabel);
    for (int i = 1; i <= 16; ++i) loopBarsMenu.addItem(juce::String(i), i);
    loopBarsMenu.onChange = [this] { audioProcessor.apvts.getParameter("loopBars")->setValueNotifyingHost((loopBarsMenu.getSelectedId() - 1) / 15.0f); };

    addAndMakeVisible(tempoSlider); addAndMakeVisible(tempoLabel);
    tempoSlider.setSliderStyle(juce::Slider::IncDecButtons);
    tempoSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 50, 20);
    tempoSlider.setRange(20.0, 300.0, 1.0);
    tempoSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::black);
    tempoLabel.setColour(juce::Label::textColourId, juce::Colours::darkgrey);
    tempoLabel.attachToComponent(&tempoSlider, true);
    tempoSlider.onValueChange = [this] { audioProcessor.apvts.getParameter("tempo")->setValueNotifyingHost((tempoSlider.getValue() - 20.0f) / 280.0f); };

    startTimerHz(30);
}

ChordMatrixAudioProcessorEditor::~ChordMatrixAudioProcessorEditor() { stopTimer(); }
void ChordMatrixAudioProcessorEditor::timerCallback() { repaint(); }

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
    // ヘッダーの要素を綺麗に整列
    timeSigNumMenu.setBounds(620, 20, 50, 25);
    timeSigDenMenu.setBounds(680, 20, 50, 25);
    stepSizeMenu.setBounds(780, 20, 60, 25);
    loopBarsMenu.setBounds(890, 20, 50, 25);
    tempoSlider.setBounds(500, 20, 70, 25);
}

void ChordMatrixAudioProcessorEditor::paint(juce::Graphics& g)
{
    // ★ 明るいフラットテーマ (Ableton Light風)
    const auto bg = juce::Colour(0xfff4f5f7);         // メイン背景（ライトグレー）
    const auto panelBg = juce::Colour(0xffffffff);    // パネル背景（白）
    const auto activeCyan = juce::Colour(0xff00aadd); // 鮮やかなシアン
    const auto accentOrange = juce::Colour(0xffff7700);// アクセントオレンジ
    const auto textDark = juce::Colours::black;
    const auto lineCol = juce::Colour(0xffdddddd);

    g.fillAll(bg);

    // --- Header ---
    g.setColour(panelBg); g.fillRect(0, 0, getWidth(), 65);
    g.setColour(lineCol); g.drawLine(0, 65, getWidth(), 65, 1.0f);

    g.setColour(textDark); g.setFont(juce::Font(22.0f, juce::Font::bold));
    g.drawText("CHORD MATRIX", 25, 0, 200, 65, juce::Justification::centredLeft);

    // Sync Params
    timeSigNumMenu.setSelectedId((int)*audioProcessor.apvts.getRawParameterValue("timeSigNum"), juce::dontSendNotification);
    timeSigDenMenu.setSelectedId((int)*audioProcessor.apvts.getRawParameterValue("timeSigDen") + 1, juce::dontSendNotification);
    stepSizeMenu.setSelectedId((int)*audioProcessor.apvts.getRawParameterValue("stepSize") + 1, juce::dontSendNotification);
    loopBarsMenu.setSelectedId((int)*audioProcessor.apvts.getRawParameterValue("loopBars"), juce::dontSendNotification);
    tempoSlider.setValue(*audioProcessor.apvts.getRawParameterValue("tempo"), juce::dontSendNotification);

    g.setColour(juce::Colours::grey); g.drawText("/", 670, 20, 10, 25, juce::Justification::centred);

    // Transport Buttons
    auto drawBtn = [&](int x, int w, juce::String txt, bool active, juce::Colour c) {
        g.setColour(active ? c : juce::Colour(0xffe0e0e0));
        g.fillRoundedRectangle((float)x, 15.0f, (float)w, 35.0f, 4.0f);
        g.setColour(active ? juce::Colours::white : textDark); g.setFont(13.0f);
        g.drawText(txt, x, 15, w, 35, juce::Justification::centred);
        };

    drawBtn(190, 60, "SYNC", audioProcessor.isSyncEnabled, accentOrange);
    drawBtn(260, 60, "PLAY", audioProcessor.isInternalPlaying, activeCyan);
    drawBtn(330, 60, "STOP", !audioProcessor.isInternalPlaying && !audioProcessor.isSyncEnabled, juce::Colours::red.withAlpha(0.8f));
    drawBtn(400, 80, "OPTIMIZE", false, textDark);

    int editBar = (int)*audioProcessor.apvts.getRawParameterValue("editBar");
    int stepsPerBar = getStepsPerBar();
    float ppqPerStep = getPpqPerStep();
    float stepW = ((float)getWidth() - 140.0f) / stepsPerBar;

    // --- Grid Area ---
    g.setFont(12.0f); g.setColour(textDark);
    g.drawText("KEY", 15, 90, 70, 45, juce::Justification::centredLeft);
    g.drawText("CHORD", 15, 140, 70, 45, juce::Justification::centredLeft);
    juce::String vNames[] = { "13th", "11th", "9th", "7th", "5th", "3rd", "Root" };
    for (int i = 0; i < 7; ++i) g.drawText(vNames[i], 15, 200 + (i * 55), 70, 55, juce::Justification::centredLeft);

    int tsDen = (int)*audioProcessor.apvts.getRawParameterValue("timeSigDen");
    int stepsPerBeat = (tsDen == 0) ? (1.0f / ppqPerStep) : (tsDen == 1) ? (0.5f / ppqPerStep) : 1;
    if (stepsPerBeat < 1) stepsPerBeat = 1;

    for (int s = 0; s < stepsPerBar; ++s) {
        if (s % stepsPerBeat == 0) {
            float beatPpq = s * ppqPerStep;
            auto& beat = audioProcessor.beatSettings[((editBar * stepsPerBar) + s) * (int)ppqPerStep];

            auto rK = getBeatLaneBounds(s / stepsPerBeat, 90, stepsPerBar, ppqPerStep);
            g.setColour(panelBg); g.fillRoundedRectangle(rK.reduced(2.0f), 4.0f);
            g.setColour(textDark); g.drawText(ChordMatrix::MusicTheory::getNoteName(beat.keyRoot), rK, juce::Justification::centred);

            auto rC = getBeatLaneBounds(s / stepsPerBeat, 140, stepsPerBar, ppqPerStep);
            g.setColour(activeCyan.withAlpha(0.2f)); g.fillRoundedRectangle(rC.reduced(2.0f), 4.0f);
            g.setColour(textDark);
            g.drawText(ChordMatrix::MusicTheory::getChordFullName(beat.keyRoot, beat.chordDegree, beat.chordType, beat.tensionType), rC, juce::Justification::centred);
        }

        // 拍の区切り線 (視認性向上)
        if (s % stepsPerBeat == 0) {
            g.setColour(lineCol);
            g.drawLine(100.0f + (s * stepW), 190.0f, 100.0f + (s * stepW), 600.0f, 1.5f);
        }
    }

    int loopIdx = (int)*audioProcessor.apvts.getRawParameterValue("loopBars");
    int numBars = loopIdx;

    for (int s = 0; s < stepsPerBar; ++s) {
        int actS = (editBar * stepsPerBar) + s;
        for (int v = 0; v < 7; ++v) {
            auto cell = getCellBounds(s, v, stepsPerBar);
            auto& voice = audioProcessor.sequenceData[actS].voices[v];

            if (!voice.isActive) {
                g.setColour(juce::Colour(0xffe8ecef)); // ライトグレーのマス
                g.fillRoundedRectangle(cell.reduced(3.0f), 3.0f);
            }
            else {
                float gateLen = audioProcessor.sequenceData[actS].gateLength;
                cell.setWidth(cell.getWidth() * (gateLen / ppqPerStep));

                g.setColour(activeCyan);
                g.fillRoundedRectangle(cell.reduced(2.0f), 4.0f);

                if (actS == audioProcessor.currentGlobalStep % (numBars * stepsPerBar)) {
                    g.setColour(accentOrange); g.drawRoundedRectangle(cell.reduced(1.0f), 4.0f, 2.0f);
                }

                juce::String label = "";
                if (voice.octaveShift != 0) label += (voice.octaveShift > 0 ? "+" : "") + juce::String(voice.octaveShift);
                if (voice.accidental == 1) label += (label.isNotEmpty() ? " " : "") + juce::String("#");
                else if (voice.accidental == -1) label += (label.isNotEmpty() ? " " : "") + juce::String("b");

                if (label.isNotEmpty()) {
                    g.setColour(panelBg); g.setFont(12.0f);
                    g.drawText(label, getCellBounds(s, v, stepsPerBar), juce::Justification::centred);
                }
            }
        }
    }

    // --- Bar Navigation ---
    for (int i = 0; i < 16; ++i) {
        auto r = getBarButtonBounds(i, numBars);
        bool inLoop = (i < numBars);
        g.setColour(i == editBar ? activeCyan : (inLoop ? panelBg : juce::Colour(0xffe0e0e0)));
        g.fillRoundedRectangle(r, 4.0f);
        g.setColour(i == editBar ? panelBg : (inLoop ? textDark : juce::Colours::grey));
        g.drawText("BAR " + juce::String(i + 1), r, juce::Justification::centred);
    }
}

void ChordMatrixAudioProcessorEditor::mouseDown(const juce::MouseEvent& e)
{
    isDraggingGate = false; dragStep = -1;

    if (juce::Rectangle<int>(190, 15, 70, 35).contains(e.getPosition())) audioProcessor.isSyncEnabled = !audioProcessor.isSyncEnabled;
    if (juce::Rectangle<int>(260, 15, 70, 35).contains(e.getPosition())) { audioProcessor.isInternalPlaying = true; audioProcessor.isSyncEnabled = false; }

    // 要件②: Stopボタンのダブルクリック（2回押し）リセットロジック
    if (juce::Rectangle<int>(330, 15, 70, 35).contains(e.getPosition())) {
        if (!audioProcessor.isInternalPlaying && !audioProcessor.isSyncEnabled) {
            // 既にStop状態なら、再生位置をリセット
            audioProcessor.internalPPQ = 0.0;
            audioProcessor.currentGlobalStep = -1;
        }
        else {
            // 再生中ならStop
            audioProcessor.isInternalPlaying = false;
            audioProcessor.isSyncEnabled = false;
        }
    }

    if (juce::Rectangle<int>(400, 15, 80, 35).contains(e.getPosition())) { audioProcessor.optimizeVoicing(); repaint(); return; }

    int editBar = (int)*audioProcessor.apvts.getRawParameterValue("editBar");
    int stepsPerBar = getStepsPerBar();
    float ppqPerStep = getPpqPerStep();
    float stepW = ((float)getWidth() - 140.0f) / stepsPerBar;

    if (e.y >= 90 && e.y < 135 && e.x >= 100) {
        int bIdx = (editBar * stepsPerBar) * (int)ppqPerStep + (int)((e.x - 100) / stepW) * (int)ppqPerStep;
        juce::PopupMenu m; for (int i = 0; i < 12; ++i) m.addItem(i + 1, ChordMatrix::MusicTheory::getNoteName(i));
        m.showMenuAsync(juce::PopupMenu::Options(), [this, bIdx](int r) { if (r > 0) audioProcessor.beatSettings[bIdx].keyRoot = r - 1; });
    }

    for (int s = 0; s < stepsPerBar; ++s) {
        int actS = (editBar * stepsPerBar) + s;
        for (int v = 0; v < 7; ++v) {
            auto cell = getCellBounds(s, v, stepsPerBar);
            cell.setWidth(cell.getWidth() * (audioProcessor.sequenceData[actS].gateLength / ppqPerStep));
            if (cell.contains(e.position)) {
                if (audioProcessor.sequenceData[actS].voices[v].isActive && e.position.x > cell.getRight() - 10.0f) {
                    isDraggingGate = true; dragStep = actS;
                    dragStartGate = audioProcessor.sequenceData[actS].gateLength;
                    dragStartX = e.position.x; return;
                }
                else if (getCellBounds(s, v, stepsPerBar).contains(e.position)) {
                    audioProcessor.sequenceData[actS].voices[v].isActive = !audioProcessor.sequenceData[actS].voices[v].isActive;
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
            if (audioProcessor.sequenceData[actS].voices[v].isActive) {
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
        float stepW = ((float)getWidth() - 140.0f) / stepsPerBar;
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
        audioProcessor.sequenceData[dragStep].gateLength = juce::jlimit(0.1f, maxGate, newGate);
        repaint();
    }
}

void ChordMatrixAudioProcessorEditor::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) {
    int editBar = (int)*audioProcessor.apvts.getRawParameterValue("editBar");
    int stepsPerBar = getStepsPerBar();
    for (int s = 0; s < stepsPerBar; ++s) {
        int actS = (editBar * stepsPerBar) + s;
        for (int v = 0; v < 7; ++v) {
            if (getCellBounds(s, v, stepsPerBar).contains(e.position) && audioProcessor.sequenceData[actS].voices[v].isActive) {
                auto& voice = audioProcessor.sequenceData[actS].voices[v];
                if (e.mods.isCtrlDown() || e.mods.isCommandDown()) voice.accidental = juce::jlimit<int8_t>(-1, 1, voice.accidental + (wheel.deltaY > 0 ? 1 : -1));
                else voice.octaveShift = juce::jlimit<int8_t>(-2, 2, voice.octaveShift + (wheel.deltaY > 0 ? 1 : -1));
                repaint(); return;
            }
        }
    }
}

juce::Rectangle<float> ChordMatrixAudioProcessorEditor::getCellBounds(int s, int v, int spb) { return { 100.0f + (float)s * ((getWidth() - 140.0f) / spb), 200.0f + (float)v * 55.0f, (getWidth() - 140.0f) / spb, 52.0f }; }
juce::Rectangle<float> ChordMatrixAudioProcessorEditor::getBeatLaneBounds(int b, int y, int spb, float ppq) {
    int stepsPerBeat = 1.0f / ppq; if (stepsPerBeat < 1) stepsPerBeat = 1;
    float w = ((getWidth() - 140.0f) / spb) * stepsPerBeat;
    return { 100.0f + (float)b * w, (float)y, w, 45.0f };
}
juce::Rectangle<float> ChordMatrixAudioProcessorEditor::getBarButtonBounds(int i, int) { float w = ((float)getWidth() - 100.0f) / 8.0f; return { 50.0f + (float)(i % 8) * w, (float)getHeight() - 120.0f + (float)(i / 8) * 55.0f, w - 8.0f, 48.0f }; }
// Source/PluginEditor.cpp
#include "PluginProcessor.h"
#include "PluginEditor.h"

ChordMatrixAudioProcessorEditor::ChordMatrixAudioProcessorEditor(ChordMatrixAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(1100, 800);

    auto setupCombo = [this](juce::ComboBox& box, juce::Label& lbl) {
        addAndMakeVisible(box); addAndMakeVisible(lbl);
        box.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff2a2a2a));
        box.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff444444));
        lbl.setColour(juce::Label::textColourId, juce::Colours::grey);
        lbl.attachToComponent(&box, true);
        };

    setupCombo(timeSigNumMenu, timeSigLabel);
    for (int i = 4; i <= 15; ++i) timeSigNumMenu.addItem(juce::String(i), i);
    timeSigNumMenu.onChange = [this] { audioProcessor.apvts.getParameter("timeSigNum")->setValueNotifyingHost((timeSigNumMenu.getSelectedId() - 4) / 11.0f); };

    setupCombo(timeSigDenMenu, *new juce::Label()); // Slash label managed in paint
    timeSigDenMenu.addItem("4", 1); timeSigDenMenu.addItem("8", 2); timeSigDenMenu.addItem("16", 3);
    timeSigDenMenu.onChange = [this] { audioProcessor.apvts.getParameter("timeSigDen")->setValueNotifyingHost((timeSigDenMenu.getSelectedId() - 1) / 2.0f); };

    setupCombo(stepSizeMenu, stepSizeLabel);
    stepSizeMenu.addItem("1/4", 1); stepSizeMenu.addItem("1/8", 2); stepSizeMenu.addItem("1/16", 3);
    stepSizeMenu.onChange = [this] { audioProcessor.apvts.getParameter("stepSize")->setValueNotifyingHost((stepSizeMenu.getSelectedId() - 1) / 2.0f); };

    startTimerHz(30);
}

ChordMatrixAudioProcessorEditor::~ChordMatrixAudioProcessorEditor() { stopTimer(); }
void ChordMatrixAudioProcessorEditor::timerCallback() { repaint(); }

void ChordMatrixAudioProcessorEditor::resized()
{
    timeSigNumMenu.setBounds(650, 20, 50, 25);
    timeSigDenMenu.setBounds(710, 20, 50, 25);
    stepSizeMenu.setBounds(840, 20, 70, 25);
}

void ChordMatrixAudioProcessorEditor::paint(juce::Graphics& g)
{
    const auto bg = juce::Colour(0xff1c1c1c);
    const auto panelBg = juce::Colour(0xff2d2d2d);
    const auto activeCyan = juce::Colour(0xff00d2ff);
    const auto accentOrange = juce::Colour(0xffff9900);

    g.fillAll(bg);

    // --- Header ---
    g.setColour(panelBg); g.fillRect(0, 0, getWidth(), 65);
    g.setColour(juce::Colours::white); g.setFont(juce::Font(22.0f, juce::Font::bold));
    g.drawText("CHORD MATRIX", 25, 0, 200, 65, juce::Justification::centredLeft);

    // Update Combo boxes from APVTS (Sync)
    timeSigNumMenu.setSelectedId((int)*audioProcessor.apvts.getRawParameterValue("timeSigNum"), juce::dontSendNotification);
    timeSigDenMenu.setSelectedId((int)*audioProcessor.apvts.getRawParameterValue("timeSigDen") + 1, juce::dontSendNotification);
    stepSizeMenu.setSelectedId((int)*audioProcessor.apvts.getRawParameterValue("stepSize") + 1, juce::dontSendNotification);

    g.setColour(juce::Colours::grey); g.drawText("/", 700, 20, 10, 25, juce::Justification::centred);

    // Transport
    auto drawBtn = [&](int x, int w, juce::String txt, bool active, juce::Colour c) {
        g.setColour(active ? c : juce::Colour(0xff444444));
        g.fillRoundedRectangle((float)x, 15.0f, (float)w, 35.0f, 4.0f);
        g.setColour(juce::Colours::white); g.setFont(13.0f);
        g.drawText(txt, x, 15, w, 35, juce::Justification::centred);
        };

    drawBtn(220, 70, "SYNC", audioProcessor.isSyncEnabled, accentOrange);
    drawBtn(300, 70, "PLAY", audioProcessor.isInternalPlaying, activeCyan);
    drawBtn(380, 70, "STOP", !audioProcessor.isInternalPlaying && !audioProcessor.isSyncEnabled, juce::Colours::red.withAlpha(0.8f));
    drawBtn(460, 90, "OPTIMIZE", false, juce::Colours::white);

    int editBar = (int)*audioProcessor.apvts.getRawParameterValue("editBar");
    float stepW = ((float)getWidth() - 140.0f) / 16.0f;

    // --- Grid Area ---
    g.setFont(12.0f); g.setColour(juce::Colours::grey);
    g.drawText("KEY", 15, 90, 70, 45, juce::Justification::centredLeft);
    g.drawText("CHORD", 15, 140, 70, 45, juce::Justification::centredLeft);
    juce::String vNames[] = { "13th", "11th", "9th", "7th", "5th", "3rd", "Root" };
    for (int i = 0; i < 7; ++i) g.drawText(vNames[i], 15, 200 + (i * 55), 70, 55, juce::Justification::centredLeft);

    // Headers & Vertical Beat Lines (要件⑤)
    int tsDen = (int)*audioProcessor.apvts.getRawParameterValue("timeSigDen");
    int stepsPerBeat = (tsDen == 0) ? 4 : (tsDen == 1) ? 2 : 1; // 4=1/4, 8=1/8, 16=1/16

    for (int s = 0; s < 16; ++s) {
        if (s % 4 == 0) { // コードヘッダーは4マス単位で描画
            auto& beat = audioProcessor.beatSettings[((editBar * 16) + s) / 4];
            auto rK = getBeatLaneBounds(s / 4, 90);
            g.setColour(panelBg); g.fillRoundedRectangle(rK.reduced(2.0f), 4.0f);
            g.setColour(juce::Colours::white); g.drawText(ChordMatrix::MusicTheory::getNoteName(beat.keyRoot), rK, juce::Justification::centred);

            auto rC = getBeatLaneBounds(s / 4, 140);
            g.setColour(activeCyan.withAlpha(0.15f)); g.fillRoundedRectangle(rC.reduced(2.0f), 4.0f);
            g.setColour(activeCyan);
            // 要件③: 拡張和音名の表示
            g.drawText(ChordMatrix::MusicTheory::getChordFullName(beat.keyRoot, beat.chordDegree, beat.chordType, beat.tensionType), rC, juce::Justification::centred);
        }

        // 拍の区切り線描画
        if (s % stepsPerBeat == 0) {
            g.setColour(juce::Colours::white.withAlpha(0.1f));
            g.drawLine(100.0f + (s * stepW), 190.0f, 100.0f + (s * stepW), 600.0f, 2.0f);
        }
    }

    // Grid Data
    int loopIdx = (int)*audioProcessor.apvts.getRawParameterValue("loopBars");
    int numBars = (loopIdx + 1) * 4;
    int stepSizeIdx = (int)*audioProcessor.apvts.getRawParameterValue("stepSize");
    float ppqPerStep = (stepSizeIdx == 0) ? 1.0f : (stepSizeIdx == 1) ? 0.5f : 0.25f;

    for (int s = 0; s < 16; ++s) {
        int actS = (editBar * 16) + s;
        for (int v = 0; v < 7; ++v) {
            auto cell = getCellBounds(s, v);
            auto& voice = audioProcessor.sequenceData[actS].voices[v];

            if (!voice.isActive) {
                g.setColour(juce::Colour(0xff222222));
                g.fillRoundedRectangle(cell.reduced(3.0f), 3.0f);
            }
            else {
                float gateLen = audioProcessor.sequenceData[actS].gateLength;
                cell.setWidth(cell.getWidth() * (gateLen / ppqPerStep));

                g.setColour(activeCyan);
                g.fillRoundedRectangle(cell.reduced(2.0f), 4.0f);

                if (actS == audioProcessor.currentGlobalStep % (numBars * 16)) {
                    g.setColour(juce::Colours::white); g.drawRoundedRectangle(cell.reduced(1.0f), 4.0f, 2.0f);
                }

                juce::String label = "";
                if (voice.octaveShift != 0) label += (voice.octaveShift > 0 ? "+" : "") + juce::String(voice.octaveShift);
                if (voice.accidental == 1) label += (label.isNotEmpty() ? " " : "") + juce::String("#");
                else if (voice.accidental == -1) label += (label.isNotEmpty() ? " " : "") + juce::String("b");

                if (label.isNotEmpty()) {
                    g.setColour(bg); g.setFont(12.0f);
                    g.drawText(label, getCellBounds(s, v), juce::Justification::centred);
                }
            }
        }
    }

    // --- Bar Navigation ---
    for (int i = 0; i < 16; ++i) {
        auto r = getBarButtonBounds(i);
        bool inLoop = (i < numBars);
        g.setColour(i == editBar ? activeCyan : (inLoop ? panelBg : juce::Colour(0xff222222)));
        g.fillRoundedRectangle(r, 4.0f);
        g.setColour(i == editBar ? bg : (inLoop ? juce::Colours::white : juce::Colours::grey));
        g.drawText("BAR " + juce::String(i + 1), r, juce::Justification::centred);
    }
}

void ChordMatrixAudioProcessorEditor::mouseMove(const juce::MouseEvent& e)
{
    int editBar = (int)*audioProcessor.apvts.getRawParameterValue("editBar");
    int stepSizeIdx = (int)*audioProcessor.apvts.getRawParameterValue("stepSize");
    float ppqPerStep = (stepSizeIdx == 0) ? 1.0f : (stepSizeIdx == 1) ? 0.5f : 0.25f;
    bool hoveringEdge = false;

    for (int s = 0; s < 16; ++s) {
        int actS = (editBar * 16) + s;
        for (int v = 0; v < 7; ++v) {
            if (audioProcessor.sequenceData[actS].voices[v].isActive) {
                auto cell = getCellBounds(s, v);
                cell.setWidth(cell.getWidth() * (audioProcessor.sequenceData[actS].gateLength / ppqPerStep));
                if (cell.contains(e.position) && e.position.x > cell.getRight() - 10.0f) {
                    hoveringEdge = true; break;
                }
            }
        }
    }
    setMouseCursor(hoveringEdge ? juce::MouseCursor::LeftRightResizeCursor : juce::MouseCursor::NormalCursor);
}

void ChordMatrixAudioProcessorEditor::mouseDown(const juce::MouseEvent& e)
{
    isDraggingGate = false; dragStep = -1;

    if (juce::Rectangle<int>(220, 15, 70, 35).contains(e.getPosition())) audioProcessor.isSyncEnabled = !audioProcessor.isSyncEnabled;
    if (juce::Rectangle<int>(300, 15, 70, 35).contains(e.getPosition())) { audioProcessor.isInternalPlaying = true; audioProcessor.isSyncEnabled = false; }
    if (juce::Rectangle<int>(380, 15, 70, 35).contains(e.getPosition())) { audioProcessor.isInternalPlaying = false; audioProcessor.isSyncEnabled = false; audioProcessor.internalPPQ = 0.0; }
    if (juce::Rectangle<int>(460, 15, 100, 35).contains(e.getPosition())) { audioProcessor.optimizeVoicing(); repaint(); return; }

    int editBar = (int)*audioProcessor.apvts.getRawParameterValue("editBar");
    int stepSizeIdx = (int)*audioProcessor.apvts.getRawParameterValue("stepSize");
    float ppqPerStep = (stepSizeIdx == 0) ? 1.0f : (stepSizeIdx == 1) ? 0.5f : 0.25f;
    float stepW = ((float)getWidth() - 140.0f) / 16.0f;

    if (e.y >= 90 && e.y < 135 && e.x >= 100) {
        int bIdx = (editBar * 4) + (int)((e.x - 100) / (stepW * 4));
        juce::PopupMenu m; for (int i = 0; i < 12; ++i) m.addItem(i + 1, ChordMatrix::MusicTheory::getNoteName(i));
        m.showMenuAsync(juce::PopupMenu::Options(), [this, bIdx](int r) { if (r > 0) audioProcessor.beatSettings[bIdx].keyRoot = r - 1; });
    }
    else if (e.y >= 140 && e.y < 185 && e.x >= 100) {
        int bIdx = (editBar * 4) + (int)((e.x - 100) / (stepW * 4));
        juce::PopupMenu m, degM, typM, tenM;
        auto ds = ChordMatrix::MusicTheory::getDegreeNames(); for (int i = 0; i < ds.size(); ++i) degM.addItem(i + 1, ds[i]);
        auto ts = ChordMatrix::MusicTheory::getChordTypeNames(); for (int i = 0; i < ts.size(); ++i) typM.addItem(i + 100, ts[i]);
        auto ns = ChordMatrix::MusicTheory::getTensionNames(); for (int i = 0; i < ns.size(); ++i) tenM.addItem(i + 200, ns[i]);
        m.addSubMenu("Degree", degM); m.addSubMenu("Type", typM); m.addSubMenu("Tension", tenM);
        m.showMenuAsync(juce::PopupMenu::Options(), [this, bIdx](int r) {
            auto& b = audioProcessor.beatSettings[bIdx];
            if (r >= 1 && r <= 7) b.chordDegree = r - 1; else if (r >= 100 && r < 200) b.chordType = r - 100; else if (r >= 200) b.tensionType = r - 200;
            });
    }

    for (int s = 0; s < 16; ++s) {
        int actS = (editBar * 16) + s;
        for (int v = 0; v < 7; ++v) {
            auto cell = getCellBounds(s, v);
            cell.setWidth(cell.getWidth() * (audioProcessor.sequenceData[actS].gateLength / ppqPerStep));
            if (cell.contains(e.position)) {
                if (audioProcessor.sequenceData[actS].voices[v].isActive && e.position.x > cell.getRight() - 10.0f) {
                    isDraggingGate = true; dragStep = actS;
                    dragStartGate = audioProcessor.sequenceData[actS].gateLength;
                    dragStartX = e.position.x; return;
                }
                else if (getCellBounds(s, v).contains(e.position)) {
                    audioProcessor.sequenceData[actS].voices[v].isActive = !audioProcessor.sequenceData[actS].voices[v].isActive;
                }
            }
        }
    }
    for (int i = 0; i < 16; ++i) if (getBarButtonBounds(i).contains(e.position)) audioProcessor.apvts.getParameter("editBar")->setValueNotifyingHost((float)i / 15.0f);
}

void ChordMatrixAudioProcessorEditor::mouseDrag(const juce::MouseEvent& e) {
    if (isDraggingGate && dragStep >= 0) {
        float stepW = ((float)getWidth() - 140.0f) / 16.0f;
        int stepSizeIdx = (int)*audioProcessor.apvts.getRawParameterValue("stepSize");
        float ppqPerStep = (stepSizeIdx == 0) ? 1.0f : (stepSizeIdx == 1) ? 0.5f : 0.25f;
        float deltaX = e.position.x - dragStartX;

        // 要件②: オーバーラップ防止ロジック
        float maxGate = 16.0f;
        int editBar = (int)*audioProcessor.apvts.getRawParameterValue("editBar");
        int localS = dragStep % 16;
        for (int nextS = localS + 1; nextS < 16; ++nextS) {
            int actNext = (editBar * 16) + nextS;
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
    for (int s = 0; s < 16; ++s) {
        int actS = (editBar * 16) + s;
        for (int v = 0; v < 7; ++v) {
            if (getCellBounds(s, v).contains(e.position) && audioProcessor.sequenceData[actS].voices[v].isActive) {
                auto& voice = audioProcessor.sequenceData[actS].voices[v];
                if (e.mods.isCtrlDown() || e.mods.isCommandDown()) voice.accidental = juce::jlimit<int8_t>(-1, 1, voice.accidental + (wheel.deltaY > 0 ? 1 : -1));
                else voice.octaveShift = juce::jlimit<int8_t>(-2, 2, voice.octaveShift + (wheel.deltaY > 0 ? 1 : -1));
                repaint(); return;
            }
        }
    }
}

juce::Rectangle<float> ChordMatrixAudioProcessorEditor::getCellBounds(int s, int v) { return { 100.0f + (float)s * ((getWidth() - 140.0f) / 16.0f), 200.0f + (float)v * 55.0f, (getWidth() - 140.0f) / 16.0f, 52.0f }; }
juce::Rectangle<float> ChordMatrixAudioProcessorEditor::getBeatLaneBounds(int b, int y) { return { 100.0f + (float)b * (((getWidth() - 140.0f) / 16.0f) * 4.0f), (float)y, ((getWidth() - 140.0f) / 16.0f) * 4.0f, 45.0f }; }
juce::Rectangle<float> ChordMatrixAudioProcessorEditor::getBarButtonBounds(int i) { float w = ((float)getWidth() - 100.0f) / 8.0f; return { 50.0f + (float)(i % 8) * w, (float)getHeight() - 120.0f + (float)(i / 8) * 55.0f, w - 8.0f, 48.0f }; }
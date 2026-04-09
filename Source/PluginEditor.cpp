#include "PluginProcessor.h"
#include "PluginEditor.h"

ChordMatrixAudioProcessorEditor::ChordMatrixAudioProcessorEditor(ChordMatrixAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(1000, 780);
    startTimerHz(30);
}

ChordMatrixAudioProcessorEditor::~ChordMatrixAudioProcessorEditor() { stopTimer(); }
void ChordMatrixAudioProcessorEditor::timerCallback() { repaint(); }

void ChordMatrixAudioProcessorEditor::paint(juce::Graphics& g)
{
    const auto headerBg = juce::Colour(0xff222222);
    const auto activeCyan = juce::Colour(0xff00f0ff);
    const auto accentOrange = juce::Colour(0xfff0a030);

    g.fillAll(juce::Colour(0xff2a2a2a));

    // --- Header ---
    g.setColour(headerBg); g.fillRect(0, 0, getWidth(), 65);
    g.setColour(juce::Colours::white); g.setFont(juce::Font(22.0f, juce::Font::bold));
    g.drawText("ChordMatrix", 25, 0, 200, 65, juce::Justification::left);

    // Transport
    auto drawBtn = [&](int x, juce::String txt, bool active, juce::Colour c) {
        g.setColour(active ? c : juce::Colours::darkgrey.darker());
        g.fillRoundedRectangle((float)x, 15.0f, 70.0f, 35.0f, 5.0f);
        g.setColour(juce::Colours::white); g.setFont(14.0f);
        g.drawText(txt, x, 15, 70, 35, juce::Justification::centred);
        };

    drawBtn(240, "SYNC", audioProcessor.isSyncEnabled, accentOrange);
    drawBtn(320, "PLAY", audioProcessor.isInternalPlaying, juce::Colours::green.withAlpha(0.8f));
    drawBtn(400, "STOP", !audioProcessor.isInternalPlaying && !audioProcessor.isSyncEnabled, juce::Colours::red.withAlpha(0.8f));

    // BPM
    g.setColour(juce::Colours::black); g.fillRoundedRectangle(490.0f, 15.0f, 110.0f, 35.0f, 5.0f);
    g.setColour(juce::Colours::white); g.setFont(16.0f);
    g.drawText(juce::String(audioProcessor.currentBPM, 1) + " BPM", 490, 15, 110, 35, juce::Justification::centred);

    // BARS Selector
    int loopIdx = (int)*audioProcessor.apvts.getRawParameterValue("loopBars");
    int numBars = (loopIdx + 1) * 4;
    g.setColour(juce::Colours::white); g.setFont(14.0f);
    g.drawText("BARS:", 620, 15, 50, 35, juce::Justification::centredLeft);
    for (int i = 0; i < 4; ++i) {
        int bx = 670 + i * 50;
        g.setColour(i == loopIdx ? accentOrange : juce::Colours::darkgrey);
        g.fillRoundedRectangle((float)bx, 15.0f, 40.0f, 35.0f, 4.0f);
        g.setColour(juce::Colours::white);
        g.drawText(juce::String((i + 1) * 4), bx, 15, 40, 35, juce::Justification::centred);
    }

    int editBar = (int)*audioProcessor.apvts.getRawParameterValue("editBar");

    // --- Grid Area ---
    g.setFont(13.0f); g.setColour(juce::Colours::grey);
    g.drawText("KEY", 15, 90, 70, 45, juce::Justification::centredLeft);
    g.drawText("CHORD", 15, 140, 70, 45, juce::Justification::centredLeft);
    juce::String vNames[] = { "13th", "11th", "9th", "7th", "5th", "3rd", "Root" };
    for (int i = 0; i < 7; ++i) g.drawText(vNames[i], 15, 200 + (i * 55), 70, 55, juce::Justification::centredLeft);

    float totalGridWidth = (float)getWidth() - 140.0f;
    float stepW = totalGridWidth / 16.0f;
    for (int s = 0; s < 16; ++s) {
        int actS = (editBar * 16) + s;
        int beatIdx = actS / 4;
        auto& beat = audioProcessor.beatSettings[beatIdx];

        if (s % 4 == 0) {
            auto rK = getBeatLaneBounds(s / 4, 90);
            g.setColour(juce::Colours::white.withAlpha(0.05f)); g.fillRect(rK);
            g.setColour(juce::Colours::white); g.drawText(ChordMatrix::MusicTheory::getNoteName(beat.keyRoot), rK, juce::Justification::centred);
            auto rC = getBeatLaneBounds(s / 4, 140);
            g.setColour(activeCyan.withAlpha(0.1f)); g.fillRect(rC);
            g.setColour(activeCyan); g.drawText(ChordMatrix::MusicTheory::getChordFullName(beat.chordDegree, beat.chordType, beat.tensionType), rC, juce::Justification::centred);
        }

        for (int v = 0; v < 7; ++v) {
            auto cell = getCellBounds(s, v);
            bool active = audioProcessor.sequenceData[actS].voices[v].isActive;
            g.setColour(active ? activeCyan : juce::Colours::black.withAlpha(0.4f));
            g.fillRect(cell.reduced(2.0f));
            if (actS == audioProcessor.currentGlobalStep % (numBars * 16)) {
                g.setColour(juce::Colours::yellow); g.drawRect(cell.reduced(1.0f), 2.0f);
            }
        }
    }

    // --- Bar Navigation ---
    for (int i = 0; i < 16; ++i) {
        auto r = getBarButtonBounds(i);
        bool inLoop = (i < numBars);
        g.setColour(i == editBar ? juce::Colours::white : (inLoop ? juce::Colours::grey : juce::Colours::black));
        g.drawRoundedRectangle(r, 5.0f, 1.0f);
        if (i == editBar) g.fillRoundedRectangle(r, 5.0f);
        g.setColour(i == editBar ? juce::Colours::black : juce::Colours::white);
        g.drawText("BAR " + juce::String(i + 1), r, juce::Justification::centred);
    }
}

void ChordMatrixAudioProcessorEditor::mouseDown(const juce::MouseEvent& e)
{
    // BARS Select
    for (int i = 0; i < 4; ++i) {
        if (juce::Rectangle<int>(670 + i * 50, 15, 40, 35).contains(e.getPosition())) {
            audioProcessor.apvts.getParameter("loopBars")->setValueNotifyingHost((float)i / 3.0f);
            return;
        }
    }
    // Transport
    if (juce::Rectangle<int>(240, 15, 70, 35).contains(e.getPosition())) audioProcessor.isSyncEnabled = !audioProcessor.isSyncEnabled;
    if (juce::Rectangle<int>(320, 15, 70, 35).contains(e.getPosition())) { audioProcessor.isInternalPlaying = true; audioProcessor.isSyncEnabled = false; }
    if (juce::Rectangle<int>(400, 15, 70, 35).contains(e.getPosition())) { audioProcessor.isInternalPlaying = false; audioProcessor.isSyncEnabled = false; audioProcessor.internalPPQ = 0.0; }

    int editBar = (int)*audioProcessor.apvts.getRawParameterValue("editBar");
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
        for (int v = 0; v < 7; ++v) {
            if (getCellBounds(s, v).contains(e.position)) audioProcessor.sequenceData[(editBar * 16) + s].voices[v].isActive = !audioProcessor.sequenceData[(editBar * 16) + s].voices[v].isActive;
        }
    }
    for (int i = 0; i < 16; ++i) if (getBarButtonBounds(i).contains(e.position)) audioProcessor.apvts.getParameter("editBar")->setValueNotifyingHost((float)i / 15.0f);
}

void ChordMatrixAudioProcessorEditor::mouseDrag(const juce::MouseEvent& e) {
    if (juce::Rectangle<int>(490, 15, 110, 35).contains(e.getMouseDownPosition())) {
        float delta = e.getDistanceFromDragStartX() * 0.1f;
        float current = *audioProcessor.apvts.getRawParameterValue("tempo");
        audioProcessor.apvts.getParameter("tempo")->setValueNotifyingHost(audioProcessor.apvts.getParameterRange("tempo").convertTo0to1(juce::jlimit(20.0f, 300.0f, current + delta)));
    }
}

juce::Rectangle<float> ChordMatrixAudioProcessorEditor::getCellBounds(int s, int v) { return { 100.0f + (float)s * ((getWidth() - 140.0f) / 16.0f), 200.0f + (float)v * 55.0f, (getWidth() - 140.0f) / 16.0f, 55.0f }; }
juce::Rectangle<float> ChordMatrixAudioProcessorEditor::getBeatLaneBounds(int b, int y) { return { 100.0f + (float)b * (((getWidth() - 140.0f) / 16.0f) * 4.0f), (float)y, ((getWidth() - 140.0f) / 16.0f) * 4.0f, 45.0f }; }
juce::Rectangle<float> ChordMatrixAudioProcessorEditor::getBarButtonBounds(int i) { float w = ((float)getWidth() - 100.0f) / 8.0f; return { 50.0f + (float)(i % 8) * w, (float)getHeight() - 120.0f + (float)(i / 8) * 55.0f, w - 8.0f, 48.0f }; }
void ChordMatrixAudioProcessorEditor::resized() {}
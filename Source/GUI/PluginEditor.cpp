#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Engine/MidiExport.h"

ChordMatrixAudioProcessorEditor::ChordMatrixAudioProcessorEditor(ChordMatrixAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), header(p), inspector(p), matrixGrid(p)
{
    setSize(1250, 800);

    addAndMakeVisible(header);
    addAndMakeVisible(inspector);
    addAndMakeVisible(matrixGrid);

    // コンポーネント同士の通信（コールバック）を接続
    header.onRepaintRequest = [this] { repaint(); };
    inspector.onSettingsChanged = [this] { matrixGrid.repaint(); };

    matrixGrid.onStepSelected = [this](int step) {
        inspector.setSelectedStep(step);
        repaint();
        };
    matrixGrid.onRepaintRequest = [this] { repaint(); };

    // Editor側のヘッダーボタン配置
    progBtnBounds = juce::Rectangle<int>(380, 80, 110, 30);
    allClearBtnBounds = juce::Rectangle<int>(380 + 120, 80, 110, 30);
    dragMidiBtnBounds = juce::Rectangle<int>(380 + 240, 80, 110, 30);

    startTimerHz(30);
}

ChordMatrixAudioProcessorEditor::~ChordMatrixAudioProcessorEditor() { stopTimer(); }

void ChordMatrixAudioProcessorEditor::timerCallback() {
    if (audioProcessor.isPlaying && audioProcessor.currentGlobalStep >= 0) {
        // DAW再生に追従して画面を再描画する処理
        int tsNum = (int)*audioProcessor.apvts.getRawParameterValue("timeSigNum");
        int tsDenIdx = (int)*audioProcessor.apvts.getRawParameterValue("timeSigDen");
        int tsDen = (tsDenIdx == 0) ? 4 : (tsDenIdx == 1) ? 8 : 16;
        int stepSizeIdx = (int)*audioProcessor.apvts.getRawParameterValue("stepSize");
        float ppqPerStep = (stepSizeIdx == 0) ? 1.0f : (stepSizeIdx == 1) ? 0.5f : 0.25f;
        int stepsPerBar = juce::roundToInt((tsNum * (4.0f / tsDen)) / ppqPerStep);
        if (stepsPerBar < 1) stepsPerBar = 1;

        int loopIdx = (int)*audioProcessor.apvts.getRawParameterValue("loopBars");
        constexpr std::array<int, 5> barsMap = { 1, 4, 8, 12, 16 };
        int totalStepsInLoop = barsMap[loopIdx] * stepsPerBar;

        int currentStepInLoop = audioProcessor.currentGlobalStep % totalStepsInLoop;
        int playingBar = currentStepInLoop / stepsPerBar;
        int editBar = (int)*audioProcessor.apvts.getRawParameterValue("editBar");

        if (playingBar != editBar && playingBar < 16) {
            audioProcessor.apvts.getParameter("editBar")->setValueNotifyingHost((float)playingBar / 15.0f);
        }
        matrixGrid.repaint();
    }
}

void ChordMatrixAudioProcessorEditor::resized() {
    auto area = getLocalBounds();
    header.setBounds(area.removeFromTop(65));
    inspector.setBounds(area.removeFromLeft(380)); // 左側のInspector幅
    matrixGrid.setBounds(area); // 残りの右側がMatrixGrid
}

void ChordMatrixAudioProcessorEditor::paint(juce::Graphics& g) {
    // Editorは、部品の隙間やグローバルなボタン（DRAG MIDIなど）だけを描画する
    g.fillAll(juce::Colour(0xff1a1a1a));

    auto drawBtn = [&](int x, int y, int w, int h, const char* txt, bool active, juce::Colour c) {
        g.setColour(active ? c : juce::Colour(0xff3a3a3a));
        g.fillRoundedRectangle((float)x, (float)y, (float)w, (float)h, 4.0f);
        g.setColour(active ? juce::Colour(0xff1a1a1a) : juce::Colours::white); g.setFont(12.0f);
        g.drawText(txt, x, y, w, h, juce::Justification::centred);
        };

    drawBtn(progBtnBounds.getX(), progBtnBounds.getY(), progBtnBounds.getWidth(), progBtnBounds.getHeight(), "PROGRESSION", isProgressionMode, juce::Colours::hotpink.withAlpha(0.8f));
    drawBtn(allClearBtnBounds.getX(), allClearBtnBounds.getY(), allClearBtnBounds.getWidth(), allClearBtnBounds.getHeight(), "ALL CLEAR", false, juce::Colours::indianred.withAlpha(0.8f));
    drawBtn(dragMidiBtnBounds.getX(), dragMidiBtnBounds.getY(), dragMidiBtnBounds.getWidth(), dragMidiBtnBounds.getHeight(), "DRAG MIDI", false, juce::Colours::cyan.withAlpha(0.6f));
}

void ChordMatrixAudioProcessorEditor::mouseDown(const juce::MouseEvent& e) {
    isDraggingMidi = false;

    if (progBtnBounds.contains(e.getPosition())) {
        isProgressionMode = !isProgressionMode;
        matrixGrid.setProgressionMode(isProgressionMode);
        repaint();
        return;
    }

    if (allClearBtnBounds.contains(e.getPosition())) {
        juce::Component::SafePointer<ChordMatrixAudioProcessorEditor> safeThis(this);
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
                    }
                    safeThis->matrixGrid.repaint();
                }
            }
        );
    }
}

void ChordMatrixAudioProcessorEditor::mouseDrag(const juce::MouseEvent& e) {
    if (!isDraggingMidi && dragMidiBtnBounds.contains(e.getMouseDownPosition()) && e.mouseWasDraggedSinceMouseDown()) {
        isDraggingMidi = true;

        int tsNum = (int)*audioProcessor.apvts.getRawParameterValue("timeSigNum");
        int tsDenIdx = (int)*audioProcessor.apvts.getRawParameterValue("timeSigDen");
        int tsDen = (tsDenIdx == 0) ? 4 : (tsDenIdx == 1) ? 8 : 16;
        int stepSizeIdx = (int)*audioProcessor.apvts.getRawParameterValue("stepSize");
        float ppqPerStep = (stepSizeIdx == 0) ? 1.0f : (stepSizeIdx == 1) ? 0.5f : 0.25f;
        int stepsPerBar = juce::roundToInt((tsNum * (4.0f / tsDen)) / ppqPerStep);
        if (stepsPerBar < 1) stepsPerBar = 1;

        int loopIdx = (int)*audioProcessor.apvts.getRawParameterValue("loopBars");
        constexpr std::array<int, 5> barsMap = { 1, 4, 8, 12, 16 };

        // 独立したMIDI書き出しエンジンに処理を委譲
        ChordMatrix::MidiExport::exportAndDrag(audioProcessor.sequenceData, barsMap[loopIdx], stepsPerBar, ppqPerStep, this);
    }
}
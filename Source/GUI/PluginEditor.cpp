#include "../PluginProcessor.h"
#include "PluginEditor.h"

ChordMatrixAudioProcessorEditor::ChordMatrixAudioProcessorEditor(ChordMatrixAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), header(p), inspector(p), matrixGrid(p)
{
    setSize(1250, 800);

    addAndMakeVisible(header);
    addAndMakeVisible(inspector);
    addAndMakeVisible(matrixGrid);

    header.onRepaintRequest = [this] { repaint(); };
    inspector.onSettingsChanged = [this] { matrixGrid.repaint(); };

    matrixGrid.onStepSelected = [this](int step) {
        inspector.setSelectedStep(step);
        repaint();
        };
    matrixGrid.onRepaintRequest = [this] { repaint(); };

    startTimerHz(30);
}

ChordMatrixAudioProcessorEditor::~ChordMatrixAudioProcessorEditor() { stopTimer(); }

void ChordMatrixAudioProcessorEditor::timerCallback() {
    if (audioProcessor.isPlaying && audioProcessor.currentGlobalStep >= 0) {
        int tsNum = (int)*audioProcessor.apvts.getRawParameterValue("timeSigNum");
        int tsDenIdx = (int)*audioProcessor.apvts.getRawParameterValue("timeSigDen");
        int tsDen = (tsDenIdx == 0) ? 4 : (tsDenIdx == 1) ? 8 : 16;
        int stepSizeIdx = (int)*audioProcessor.apvts.getRawParameterValue("stepSize");
        float ppqPerStep = (stepSizeIdx == 0) ? 1.0f : (stepSizeIdx == 1) ? 0.5f : 0.25f;
        int stepsPerBar = juce::roundToInt((static_cast<float>(tsNum) * (4.0f / static_cast<float>(tsDen))) / ppqPerStep);
        if (stepsPerBar < 1) stepsPerBar = 1;

        int loopIdx = (int)*audioProcessor.apvts.getRawParameterValue("loopBars");
        constexpr std::array<int, 5> barsMap = { 1, 4, 8, 12, 16 };
        int totalStepsInLoop = barsMap[loopIdx] * stepsPerBar;

        int currentStepInLoop = audioProcessor.currentGlobalStep % totalStepsInLoop;
        int playingBar = currentStepInLoop / stepsPerBar;
        int editBar = (int)*audioProcessor.apvts.getRawParameterValue("editBar");

        if (playingBar != editBar && playingBar < 16) {
            audioProcessor.apvts.getParameter("editBar")->setValueNotifyingHost(static_cast<float>(playingBar) / 15.0f);
        }
        matrixGrid.repaint();
    }
}

void ChordMatrixAudioProcessorEditor::resized() {
    auto area = getLocalBounds();
    header.setBounds(area.removeFromTop(65));
    inspector.setBounds(area.removeFromLeft(380));
    matrixGrid.setBounds(area);
}

void ChordMatrixAudioProcessorEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xff1a1a1a));
}
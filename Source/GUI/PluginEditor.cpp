#include "PluginProcessor.h"
#include "PluginEditor.h"

// ------------------------------------------------------------
// 変更後 (PluginEditor.cpp 冒頭のコンストラクタ内)
// ------------------------------------------------------------
ChordMatrixAudioProcessorEditor::ChordMatrixAudioProcessorEditor(ChordMatrixAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), header(p), inspector(p), matrixGrid(p)
{
    setSize(1250, 880); // ★修正: サジェストUIの表示領域確保のため高さを880pxに拡張

    addAndMakeVisible(header);
    // ...
    addAndMakeVisible(inspector);
    addAndMakeVisible(matrixGrid);

    header.onRepaintRequest = [this] {
        matrixGrid.repaint();
        inspector.repaint();
        repaint();
        };

    header.onFollowModeChanged = [this](bool follow) { isFollowMode = follow; };

    inspector.onSettingsChanged = [this] { matrixGrid.repaint(); };

    matrixGrid.onStepSelected = [this](int step) {
        inspector.setSelectedStep(step);
        repaint();
        };
    matrixGrid.onRepaintRequest = [this] { repaint(); };

    startTimerHz(30);
}

ChordMatrixAudioProcessorEditor::~ChordMatrixAudioProcessorEditor() { stopTimer(); }

// ------------------------------------------------------------
// 変更後 (PluginEditor.cpp / timerCallback 内)
// ------------------------------------------------------------
void ChordMatrixAudioProcessorEditor::timerCallback() {

    // ★追加: ヘッダーのコンボボックス等の値をタイマー駆動で安全に同期（重さ・バグ解消）
    header.updateUI();

    if (audioProcessor.isPlaying && audioProcessor.currentGlobalStep >= 0) {
        // ------------------------------------------------------------
        // 変更後 (PluginEditor.cpp)
        // ------------------------------------------------------------
        int tsNum = (int)*audioProcessor.apvts.getRawParameterValue("timeSigNum");
        int tsDenIdx = (int)*audioProcessor.apvts.getRawParameterValue("timeSigDen");
        int tsDen = (tsDenIdx == 0) ? 4 : (tsDenIdx == 1) ? 8 : 16;

        // ★修正: Editorの同期も固定解像度(0.25 PPQ)ベースに合わせる
        float beatsPerBar = static_cast<float>(tsNum) * (4.0f / static_cast<float>(tsDen));
        int internalStepsPerBar = juce::roundToInt(beatsPerBar / 0.25f);
        if (internalStepsPerBar < 1) internalStepsPerBar = 1;

        int loopIdx = (int)*audioProcessor.apvts.getRawParameterValue("loopBars");
        constexpr std::array<int, 5> barsMap = { 1, 4, 8, 12, 16 };
        int totalStepsInLoop = barsMap[loopIdx] * internalStepsPerBar;

        int currentStepInLoop = audioProcessor.currentGlobalStep % totalStepsInLoop;
        int playingBar = currentStepInLoop / internalStepsPerBar;
        int editBar = (int)*audioProcessor.apvts.getRawParameterValue("editBar");

        if (isFollowMode) {
            if (playingBar != editBar && playingBar < 16) {
                audioProcessor.apvts.getParameter("editBar")->setValueNotifyingHost(static_cast<float>(playingBar) / 15.0f);
            }

            // ★修正: 和音が切り替わった時(EffectiveStepの変化時)だけInspectorを更新する
            int effStep = inspector.getEffectiveStep(currentStepInLoop);
            if (inspector.getSelectedStep() != effStep) {
                inspector.setSelectedStep(effStep);
            }
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
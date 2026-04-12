#include "InspectorComponent.h"
#include "../Engine/VoicingEngine.h"
#include "../Engine/MusicTheory.h"

namespace ChordMatrix {

    InspectorComponent::InspectorComponent(ChordMatrixAudioProcessor& p) : audioProcessor(p)
    {
        auto setupCombo = [this](juce::ComboBox& box, juce::Label& lbl) {
            addAndMakeVisible(box);
            addAndMakeVisible(lbl);
            box.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff252525));
            box.setColour(juce::ComboBox::textColourId, juce::Colours::white);
            box.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff3a3a3a));
            box.setColour(juce::ComboBox::arrowColourId, juce::Colour(0xffffa500));
            lbl.setColour(juce::Label::textColourId, juce::Colours::grey);
            lbl.attachToComponent(&box, true);
            lbl.setJustificationType(juce::Justification::centred);
            };

        setupCombo(stepKeyMenu, stepKeyLabel);
        for (int i = 0; i < 12; ++i) {
            stepKeyMenu.addItem(MusicTheory::getNoteName(i), i + 1);
        }
        stepKeyMenu.onChange = [this] {
            applyScope(scopeKey, [this](int s) { audioProcessor.sequenceData[s].keyRoot = stepKeyMenu.getSelectedId() - 1; });
            };

        setupCombo(stepScaleMenu, stepScaleLabel);
        auto scales = MusicTheory::getScaleNames();
        for (int i = 0; i < (int)scales.size(); ++i) {
            stepScaleMenu.addItem(scales[i], i + 1);
        }
        stepScaleMenu.onChange = [this] {
            applyScope(scopeScale, [this](int s) { audioProcessor.sequenceData[s].scaleType = stepScaleMenu.getSelectedId() - 1; });
            };

        setupCombo(stepDegreeMenu, stepDegreeLabel);
        auto degs = MusicTheory::getDegreeNames();
        for (int i = 0; i < (int)degs.size(); ++i) {
            stepDegreeMenu.addItem(degs[i], i + 1);
        }
        stepDegreeMenu.onChange = [this] {
            applyScope(scopeDegree, [this](int s) { audioProcessor.sequenceData[s].chordDegree = stepDegreeMenu.getSelectedId() - 1; });
            };

        setupCombo(voicingMenu, voicingLabel);

        voicingMenu.addItem("Close", 1);
        voicingMenu.addItem("Drop 2", 2);
        voicingMenu.addItem("Drop 3", 3);
        voicingMenu.addItem("Spread", 4);
        voicingMenu.addItem("Rootless A", 5);
        voicingMenu.addItem("Rootless B", 6);
        voicingMenu.addItem("UST (bII)", 7);
        voicingMenu.addItem("UST (bVI)", 8);
        voicingMenu.addItem("Quartal (4ths)", 9);
        voicingMenu.addItem("Shell (1-3-7)", 10);
        voicingMenu.addItem("Drop 2 & 4", 11);
        voicingMenu.addItem("Drop 2 & 3", 12);
        voicingMenu.addItem("So What (m11)", 13);
        voicingMenu.addItem("Cluster (2nds)", 14);
        voicingMenu.addItem("Kenny Barron", 15);
        voicingMenu.addItem("Block Chords", 16);
        voicingMenu.addItem("UST (bIII)", 17);
        voicingMenu.addItem("UST (bV)", 18);
        voicingMenu.addItem("UST (VI)", 19);
        voicingMenu.addItem("UST (II)", 20);

        voicingMenu.onChange = [this] {
            applyScope(scopeVoicing, [this](int s) { audioProcessor.sequenceData[s].voicingMode = voicingMenu.getSelectedId() - 1; });
            };

        addAndMakeVisible(btnOptimize);
        btnOptimize.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2a2a2a));
        btnOptimize.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffffa500));
        btnOptimize.onClick = [this] {
            optimizeAltIndex++;
            applyScope(scopeOptimize, [this](int s) {
                // ★修正: 最適化エンジンには常に内部解像度(0.25 PPQ)を渡して分析させる
                VoicingEngine::optimizeStep(audioProcessor.sequenceData, s, 0.25f, optimizeAltIndex);
                });
            };

        addAndMakeVisible(stepShiftSlider);
        addAndMakeVisible(stepShiftLabel);
        stepShiftSlider.setSliderStyle(juce::Slider::IncDecButtons);
        stepShiftSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 50, 30);
        stepShiftSlider.setRange(-24.0, 24.0, 1.0);
        stepShiftSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
        stepShiftSlider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff252525));
        stepShiftSlider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff3a3a3a));
        stepShiftLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
        stepShiftLabel.attachToComponent(&stepShiftSlider, true);
        stepShiftLabel.setJustificationType(juce::Justification::centred);
        stepShiftSlider.onValueChange = [this] {
            applyScope(scopeShift, [this](int s) { audioProcessor.sequenceData[s].shift = (int)stepShiftSlider.getValue(); });
            };

        updateInspector();
    }

    InspectorComponent::~InspectorComponent() {}

    void InspectorComponent::applyScope(int scopeType, std::function<void(int)> setterFunction) {
        if (selectedStep < 0) return;

        // ★修正: 内部解像度に基づく正確な1小節のステップ数（4/4なら常に16）を取得
        int internalSpb = getStepsPerBar();

        auto safeSetter = [this, setterFunction](int s) {
            if (!audioProcessor.sequenceData[s].isLocked) {
                setterFunction(s);
            }
            };

        if (scopeType == 0) {
            safeSetter(selectedStep);
        }
        else if (scopeType == 1) {
            // ★修正: 選択ステップが属する小節の先頭インデックスを計算し、16回ループで全コードを書き換える
            int barStart = static_cast<int>(selectedStep / internalSpb) * internalSpb;
            for (int i = 0; i < internalSpb; ++i) {
                safeSetter(barStart + i);
            }
        }
        else {
            for (int i = 0; i < ChordMatrix::TotalSteps; ++i) {
                safeSetter(i);
            }
        }

        for (int i = 0; i < ChordMatrix::TotalSteps; ++i) {
            audioProcessor.previewSequenceData[i] = audioProcessor.sequenceData[i];
        }

        if (onSettingsChanged) onSettingsChanged();
        repaint();
    }

    void InspectorComponent::setSelectedStep(int stepIndex) {
        selectedStep = stepIndex;
        updateInspector();
        repaint();
    }

    void InspectorComponent::updateInspector() {
        if (selectedStep < 0 || selectedStep >= ChordMatrix::TotalSteps) return;

        auto& sData = audioProcessor.sequenceData[selectedStep];
        stepKeyMenu.setSelectedId(sData.keyRoot + 1, juce::dontSendNotification);
        stepScaleMenu.setSelectedId(sData.scaleType + 1, juce::dontSendNotification);
        stepDegreeMenu.setSelectedId(sData.chordDegree + 1, juce::dontSendNotification);
        voicingMenu.setSelectedId(sData.voicingMode + 1, juce::dontSendNotification);
        stepShiftSlider.setValue(sData.shift, juce::dontSendNotification);
    }

    // =========================================================================
    // ★バグの元凶修正: 内部解像度(0.25 PPQ固定)における1小節のステップ数を返すように完全固定
    // =========================================================================
    int InspectorComponent::getStepsPerBar() const {
        int tsNum = (int)*audioProcessor.apvts.getRawParameterValue("timeSigNum");
        int tsDenIdx = (int)*audioProcessor.apvts.getRawParameterValue("timeSigDen");
        int tsDen = (tsDenIdx == 0) ? 4 : (tsDenIdx == 1) ? 8 : 16;
        float beatsPerBar = static_cast<float>(tsNum) * (4.0f / static_cast<float>(tsDen));
        return std::max(1, juce::roundToInt(beatsPerBar / 0.25f));
    }

    float InspectorComponent::getPpqPerStep() const {
        int stepSizeIdx = (int)*audioProcessor.apvts.getRawParameterValue("stepSize");
        return (stepSizeIdx == 0) ? 1.0f : (stepSizeIdx == 1) ? 0.5f : 0.25f;
    }

    int InspectorComponent::getEffectiveStep(int targetS) const {
        int eff = targetS;
        for (int prevS = targetS; prevS >= 0; --prevS) {
            // ★修正: 内部解像度(0.25f)で距離を正確に計算する
            float dist = static_cast<float>(targetS - prevS) * 0.25f;
            const auto& sData = audioProcessor.isPlayingModulationPreview.load() ? audioProcessor.previewSequenceData[prevS] : audioProcessor.sequenceData[prevS];

            bool hasNotes = false;
            for (int v = 0; v < 7; ++v) {
                if (sData.voices[v].isActive) {
                    hasNotes = true;
                    break;
                }
            }

            if (hasNotes && sData.gateLength > dist + 0.001f) {
                eff = prevS;
                break;
            }
        }
        return eff;
    }

    void InspectorComponent::resized() {
        int px = 80, py = 125, pw = 120, ph = 30, pySpace = 45;
        stepKeyMenu.setBounds(px, py, pw, ph);
        stepScaleMenu.setBounds(px, py + pySpace, pw, ph);
        stepDegreeMenu.setBounds(px, py + pySpace * 2, pw, ph);
        voicingMenu.setBounds(px, py + pySpace * 3, pw, ph);
        btnOptimize.setBounds(px, py + pySpace * 4, pw, ph);
        stepShiftSlider.setBounds(px, py + pySpace * 5, pw, ph);
    }

    void InspectorComponent::paint(juce::Graphics& g) {
        g.fillAll(juce::Colour(0xff1c1c1c));

        int internalStepsPerBar = getStepsPerBar();
        float uiPpqPerStep = getPpqPerStep();

        g.setColour(juce::Colours::grey);
        g.setFont(14.0f);

        int dispBar = (selectedStep / internalStepsPerBar) + 1;
        // ★修正: 内部の16ステップをUIの4ステップ表示に正確に逆変換する
        int internalStepMultiplier = juce::roundToInt(uiPpqPerStep / 0.25f);
        int dispStep = ((selectedStep % internalStepsPerBar) / internalStepMultiplier) + 1;

        g.drawText("BAR " + juce::String(dispBar) + " / STEP " + juce::String(dispStep), 20, 15, 300, 20, juce::Justification::centredLeft);

        const auto& activeSeqData = audioProcessor.isPlayingModulationPreview.load() ? audioProcessor.previewSequenceData : audioProcessor.sequenceData;
        int effStep = getEffectiveStep(selectedStep);

        if (selectedStep >= 0) {
            std::array<int, 7> vps = { 0 };
            int count = VoicingEngine::getVoicedPitches(activeSeqData[effStep], vps);

            int badgeRadius = 15;
            int startX = 140;
            int startY = 15;
            int spacingX = 35;
            int spacingY = 35;

            for (int i = 0; i < count; ++i) {
                int p = vps[i];
                juce::String nName = MusicTheory::getNoteName(p);
                juce::String oct = juce::String((p / 12) - 1);

                int cx = startX + (i % 5) * spacingX;
                int cy = startY + (i / 5) * spacingY;

                g.setColour(juce::Colour(0xff2a2a2a));
                g.fillEllipse((float)cx, (float)cy, badgeRadius * 2.0f, badgeRadius * 2.0f);
                g.setColour(juce::Colour(0xffffa500).withAlpha(0.8f));
                g.drawEllipse((float)cx, (float)cy, badgeRadius * 2.0f, badgeRadius * 2.0f, 1.5f);

                g.setColour(juce::Colours::white);
                g.setFont(juce::Font(12.0f, juce::Font::bold));
                g.drawText(nName + oct, cx, cy, badgeRadius * 2, badgeRadius * 2, juce::Justification::centred);
            }
        }

        g.setColour(juce::Colours::grey);
        g.setFont(12.0f);
        g.drawText("BASE SCALE SETTINGS", 20, 95, 180, 20, juce::Justification::centredLeft);

        auto drawScopeToggle = [&](int sValue, int x, int y, int w, int h) {
            juce::String txt = (sValue == 0) ? "STEP" : (sValue == 1) ? "BAR" : "ALL";
            juce::Colour c = (sValue == 0) ? juce::Colours::cyan : (sValue == 1) ? juce::Colours::orange : juce::Colours::hotpink;

            g.setColour(c.withAlpha(0.3f));
            g.fillRoundedRectangle((float)x, (float)y, (float)w, (float)h, 4.0f);

            g.setColour(c);
            g.drawRoundedRectangle((float)x, (float)y, (float)w, (float)h, 4.0f, 1.0f);

            g.setColour(juce::Colours::white);
            g.setFont(11.0f);
            g.drawText(txt, x, y, w, h, juce::Justification::centred);
            };

        int toggleX = 210, toggleW = 45;
        drawScopeToggle(scopeKey, toggleX, 125, toggleW, 30);
        drawScopeToggle(scopeScale, toggleX, 170, toggleW, 30);
        drawScopeToggle(scopeDegree, toggleX, 215, toggleW, 30);
        drawScopeToggle(scopeVoicing, toggleX, 260, toggleW, 30);
        drawScopeToggle(scopeOptimize, toggleX, 305, toggleW, 30);
        drawScopeToggle(scopeShift, toggleX, 350, toggleW, 30);

        // ★修正: コード解析にも内部解像度(0.25 PPQ)を渡す
        juce::String inspectorChordName = VoicingEngine::getRecognizedChordName(activeSeqData, selectedStep, 0.25f);

        juce::Rectangle<int> chordArea(20, 415, 340, 120);
        g.setColour(juce::Colour(0xff2a2a2a));
        g.fillRoundedRectangle(chordArea.toFloat(), 8.0f);
        g.setColour(juce::Colours::black.withAlpha(0.6f));
        g.drawRoundedRectangle(chordArea.toFloat(), 8.0f, 2.0f);

        juce::Rectangle<int> textArea = chordArea.reduced(10);
        if (inspectorChordName.contains("\n")) {
            juce::String part1 = inspectorChordName.upToFirstOccurrenceOf("\n", false, false).trim();
            juce::String part2 = inspectorChordName.fromFirstOccurrenceOf("\n", false, false).replaceCharacter('(', ' ').replaceCharacter(')', ' ').trim();

            if (part1.isNotEmpty()) {
                g.setColour(juce::Colour(0xffffa500));
                g.setFont(juce::Font(42.0f, juce::Font::bold));
                g.drawFittedText(part1, textArea.removeFromTop(static_cast<int>(textArea.getHeight() * 0.6f)), juce::Justification::centredBottom, 1, 0.2f);
            }
            if (part2.isNotEmpty()) {
                g.setColour(juce::Colours::white.withAlpha(0.8f));
                g.setFont(juce::Font(22.0f, juce::Font::bold));
                g.drawFittedText(part2, textArea, juce::Justification::centredTop, 1, 0.2f);
            }
        }
        else if (inspectorChordName.isNotEmpty()) {
            g.setColour(juce::Colour(0xffffa500));
            g.setFont(juce::Font(38.0f, juce::Font::bold));
            g.drawFittedText(inspectorChordName, textArea, juce::Justification::centred, 2, 0.2f);
        }

        int scaleAreaY = 550;
        int scaleAreaHeight = getHeight() - scaleAreaY - 20;
        juce::Rectangle<int> scaleArea(20, scaleAreaY, 340, scaleAreaHeight);

        g.setColour(juce::Colour(0xff222222));
        g.fillRoundedRectangle(scaleArea.toFloat(), 8.0f);
        g.setColour(juce::Colours::black.withAlpha(0.8f));
        g.drawRoundedRectangle(scaleArea.toFloat(), 8.0f, 2.0f);

        g.setColour(juce::Colours::grey);
        g.setFont(juce::Font(12.0f, juce::Font::bold));
        g.drawText("SCALE STRUCTURE (" + MusicTheory::getScaleNames()[activeSeqData[effStep].scaleType] + ")",
            scaleArea.getX(), scaleArea.getY() + 5, scaleArea.getWidth(), 20, juce::Justification::centred);

        int numNotes = MusicTheory::getScaleNoteCount(activeSeqData[effStep].scaleType);
        auto noteNames = MusicTheory::getScaleNoteNames(activeSeqData[effStep].keyRoot, activeSeqData[effStep].scaleType);
        auto intNames = MusicTheory::getScaleIntervalNames(activeSeqData[effStep].scaleType);

        int cols = (numNotes >= 10) ? 6 : (numNotes >= 7 ? 4 : numNotes);
        int rows = (numNotes + cols - 1) / cols;

        int margin = 15;
        int padding = 8;
        juce::Rectangle<int> gridArea = scaleArea.reduced(margin).withTrimmedTop(25);

        int cellW = (gridArea.getWidth() - padding * (cols - 1)) / cols;
        int cellH = (gridArea.getHeight() - padding * (rows - 1)) / rows;

        for (int i = 0; i < numNotes; ++i) {
            int c = i % cols;
            int r = i / cols;
            juce::Rectangle<int> cell(gridArea.getX() + c * (cellW + padding), gridArea.getY() + r * (cellH + padding), cellW, cellH);

            g.setColour(juce::Colour(0xff2a2a2a));
            g.fillRoundedRectangle(cell.toFloat(), 4.0f);

            g.setColour(juce::Colours::cyan.withAlpha(0.7f));
            g.setFont(juce::Font(11.0f, juce::Font::plain));
            g.drawText(intNames[i], cell.removeFromTop(14), juce::Justification::centredBottom);

            g.setColour(juce::Colours::white);
            g.setFont(juce::Font(18.0f, juce::Font::bold));
            g.drawFittedText(noteNames[i], cell, juce::Justification::centred, 1, 0.1f);
        }
    }

    void InspectorComponent::mouseDown(const juce::MouseEvent& e) {
        int toggleX = 210, toggleW = 45, toggleH = 30;
        if (juce::Rectangle<int>(toggleX, 125, toggleW, toggleH).contains(e.getPosition())) { scopeKey = (scopeKey + 1) % 3; repaint(); }
        if (juce::Rectangle<int>(toggleX, 170, toggleW, toggleH).contains(e.getPosition())) { scopeScale = (scopeScale + 1) % 3; repaint(); }
        if (juce::Rectangle<int>(toggleX, 215, toggleW, toggleH).contains(e.getPosition())) { scopeDegree = (scopeDegree + 1) % 3; repaint(); }
        if (juce::Rectangle<int>(toggleX, 260, toggleW, toggleH).contains(e.getPosition())) { scopeVoicing = (scopeVoicing + 1) % 3; repaint(); }
        if (juce::Rectangle<int>(toggleX, 305, toggleW, toggleH).contains(e.getPosition())) { scopeOptimize = (scopeOptimize + 1) % 3; repaint(); }
        if (juce::Rectangle<int>(toggleX, 350, toggleW, toggleH).contains(e.getPosition())) { scopeShift = (scopeShift + 1) % 3; repaint(); }
    }

} // namespace ChordMatrix
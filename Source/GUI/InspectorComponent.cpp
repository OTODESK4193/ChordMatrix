#include "InspectorComponent.h"
#include "../Engine/VoicingEngine.h"

namespace ChordMatrix {

    InspectorComponent::InspectorComponent(ChordMatrixAudioProcessor& p) : audioProcessor(p)
    {
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

        setupCombo(stepKeyMenu, stepKeyLabel);
        for (int i = 0; i < 12; ++i) stepKeyMenu.addItem(MusicTheory::getNoteName(i), i + 1);
        stepKeyMenu.onChange = [this] { applyScope(scopeKey, [this](int s) { audioProcessor.sequenceData[s].keyRoot = stepKeyMenu.getSelectedId() - 1; }); };

        setupCombo(stepScaleMenu, stepScaleLabel);
        auto scales = MusicTheory::getScaleNames();
        for (int i = 0; i < scales.size(); ++i) stepScaleMenu.addItem(scales[i], i + 1);
        stepScaleMenu.onChange = [this] { applyScope(scopeScale, [this](int s) { audioProcessor.sequenceData[s].scaleType = stepScaleMenu.getSelectedId() - 1; }); };

        setupCombo(stepDegreeMenu, stepDegreeLabel);
        auto degs = MusicTheory::getDegreeNames();
        for (int i = 0; i < degs.size(); ++i) stepDegreeMenu.addItem(degs[i], i + 1);
        stepDegreeMenu.onChange = [this] { applyScope(scopeDegree, [this](int s) { audioProcessor.sequenceData[s].chordDegree = stepDegreeMenu.getSelectedId() - 1; }); };

        setupCombo(voicingMenu, voicingLabel);
        const char* vNames[] = {
            "Close", "Drop 2", "Drop 3", "Spread", "Rootless A", "Rootless B", "UST (bII)", "UST (bVI)",
            "Quartal (4ths)", "Shell (1-3-7)", "Drop 2 & 4", "Drop 2 & 3", "So What (m11)", "Cluster (2nds)", "Kenny Barron", "Block Chords"
        };
        for (int i = 0; i < 16; ++i) voicingMenu.addItem(vNames[i], i + 1);
        voicingMenu.onChange = [this] { applyScope(scopeVoicing, [this](int s) { audioProcessor.sequenceData[s].voicingMode = voicingMenu.getSelectedId() - 1; }); };

        addAndMakeVisible(btnOptimize);
        btnOptimize.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2a2a2a));
        btnOptimize.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffffa500));
        btnOptimize.onClick = [this] { applyScope(scopeOptimize, [this](int s) { VoicingEngine::optimizeStep(audioProcessor.sequenceData, s, getPpqPerStep()); }); };

        addAndMakeVisible(stepShiftSlider); addAndMakeVisible(stepShiftLabel);
        stepShiftSlider.setSliderStyle(juce::Slider::IncDecButtons);
        stepShiftSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 50, 30);
        stepShiftSlider.setRange(-24.0, 24.0, 1.0);
        stepShiftSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
        stepShiftSlider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff252525));
        stepShiftSlider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff3a3a3a));
        stepShiftLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
        stepShiftLabel.attachToComponent(&stepShiftSlider, true);
        stepShiftLabel.setJustificationType(juce::Justification::centred);
        stepShiftSlider.onValueChange = [this] { applyScope(scopeShift, [this](int s) { audioProcessor.sequenceData[s].shift = (int)stepShiftSlider.getValue(); }); };

        updateInspector();
    }

    InspectorComponent::~InspectorComponent() {}

    void InspectorComponent::applyScope(int scopeType, std::function<void(int)> setterFunction) {
        if (selectedStep < 0) return;
        int spb = getStepsPerBar();

        if (scopeType == 0) { setterFunction(selectedStep); }
        else if (scopeType == 1) {
            int barStart = (selectedStep / spb) * spb;
            for (int i = 0; i < spb; ++i) setterFunction(barStart + i);
        }
        else { for (int i = 0; i < ChordMatrix::TotalSteps; ++i) setterFunction(i); }

        for (int i = 0; i < ChordMatrix::TotalSteps; ++i) { audioProcessor.previewSequenceData[i] = audioProcessor.sequenceData[i]; }

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

    int InspectorComponent::getStepsPerBar() const {
        int tsNum = (int)*audioProcessor.apvts.getRawParameterValue("timeSigNum");
        int tsDenIdx = (int)*audioProcessor.apvts.getRawParameterValue("timeSigDen");
        int tsDen = (tsDenIdx == 0) ? 4 : (tsDenIdx == 1) ? 8 : 16;
        float ppqPerStep = getPpqPerStep();
        return std::max(1, juce::roundToInt((static_cast<float>(tsNum) * (4.0f / static_cast<float>(tsDen))) / ppqPerStep));
    }

    float InspectorComponent::getPpqPerStep() const {
        int stepSizeIdx = (int)*audioProcessor.apvts.getRawParameterValue("stepSize");
        return (stepSizeIdx == 0) ? 1.0f : (stepSizeIdx == 1) ? 0.5f : 0.25f;
    }

    int InspectorComponent::getEffectiveStep(int targetS) const {
        int eff = targetS;
        float ppq = getPpqPerStep();
        for (int prevS = targetS; prevS >= 0; --prevS) {
            float dist = static_cast<float>(targetS - prevS) * ppq;
            const auto& sData = audioProcessor.isPlayingModulationPreview.load() ? audioProcessor.previewSequenceData[prevS] : audioProcessor.sequenceData[prevS];

            // 唯一絶対の「音符存在チェック (isActive)」によるカバー判定
            bool hasNotes = false;
            for (int v = 0; v < 7; ++v) {
                if (sData.voices[v].isActive) { hasNotes = true; break; }
            }

            if (hasNotes && sData.gateLength > dist + 0.001f) {
                eff = prevS; break;
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
        int stepsPerBar = getStepsPerBar();
        float ppqPerStep = getPpqPerStep();

        g.setColour(juce::Colours::grey); g.setFont(14.0f);
        int dispBar = (selectedStep / stepsPerBar) + 1;
        int dispStep = (selectedStep % stepsPerBar) + 1;
        g.drawText("BAR " + juce::String(dispBar) + " / STEP " + juce::String(dispStep), 20, 15, 300, 20, juce::Justification::centredLeft);

        const auto& activeSeqData = audioProcessor.isPlayingModulationPreview.load() ? audioProcessor.previewSequenceData : audioProcessor.sequenceData;

        if (selectedStep >= 0) {
            std::array<int, 7> vps = { 0 };
            int effStep = getEffectiveStep(selectedStep);
            int count = VoicingEngine::getVoicedPitches(activeSeqData[effStep], vps);
            juce::String noteStr = "";
            for (int i = 0; i < count; ++i) {
                if (i > 0) noteStr << ", ";
                noteStr << MusicTheory::getNoteName(vps[i]) << ((vps[i] / 12) - 1);
            }
            if (noteStr.isNotEmpty()) {
                g.setColour(juce::Colour(0xffffa500)); g.setFont(juce::Font(14.0f, juce::Font::bold));
                g.drawFittedText("NOTES:\n" + noteStr, 150, 35, 200, 40, juce::Justification::centredLeft, 2);
            }
        }

        g.setColour(juce::Colours::grey); g.setFont(12.0f);
        g.drawText("BASE SCALE SETTINGS", 20, 95, 180, 20, juce::Justification::centredLeft);

        auto drawScopeToggle = [&](int sValue, int x, int y, int w, int h) {
            juce::String txt = (sValue == 0) ? "STEP" : (sValue == 1) ? "BAR" : "ALL";
            juce::Colour c = (sValue == 0) ? juce::Colours::cyan : (sValue == 1) ? juce::Colours::orange : juce::Colours::hotpink;
            g.setColour(c.withAlpha(0.3f)); g.fillRoundedRectangle((float)x, (float)y, (float)w, (float)h, 4.0f);
            g.setColour(c); g.drawRoundedRectangle((float)x, (float)y, (float)w, (float)h, 4.0f, 1.0f);
            g.setColour(juce::Colours::white); g.setFont(11.0f);
            g.drawText(txt, x, y, w, h, juce::Justification::centred);
            };

        int toggleX = 210, toggleW = 45;
        drawScopeToggle(scopeKey, toggleX, 125, toggleW, 30);
        drawScopeToggle(scopeScale, toggleX, 170, toggleW, 30);
        drawScopeToggle(scopeDegree, toggleX, 215, toggleW, 30);
        drawScopeToggle(scopeVoicing, toggleX, 260, toggleW, 30);
        drawScopeToggle(scopeOptimize, toggleX, 305, toggleW, 30);
        drawScopeToggle(scopeShift, toggleX, 350, toggleW, 30);

        juce::String inspectorChordName = VoicingEngine::getRecognizedChordName(activeSeqData, selectedStep, ppqPerStep);
        juce::Rectangle<int> chordArea(20, 415, 340, 140);
        g.setColour(juce::Colour(0xff2a2a2a)); g.fillRoundedRectangle(chordArea.toFloat(), 8.0f);
        g.setColour(juce::Colours::black.withAlpha(0.6f)); g.drawRoundedRectangle(chordArea.toFloat(), 8.0f, 2.0f);
        juce::Rectangle<int> textArea = chordArea.reduced(10);

        if (inspectorChordName.contains("\n")) {
            juce::String part1 = inspectorChordName.upToFirstOccurrenceOf("\n", false, false).trim();
            juce::String part2 = inspectorChordName.fromFirstOccurrenceOf("\n", false, false).replaceCharacter('(', ' ').replaceCharacter(')', ' ').trim();
            if (part1.isNotEmpty()) {
                g.setColour(juce::Colour(0xffffa500)); g.setFont(juce::Font(46.0f, juce::Font::bold));
                g.drawFittedText(part1, textArea.removeFromTop(static_cast<int>(textArea.getHeight() * 0.6f)), juce::Justification::centredBottom, 1, 0.2f);
            }
            if (part2.isNotEmpty()) {
                g.setColour(juce::Colours::white.withAlpha(0.8f)); g.setFont(juce::Font(24.0f, juce::Font::bold));
                g.drawFittedText(part2, textArea, juce::Justification::centredTop, 1, 0.2f);
            }
        }
        else if (inspectorChordName.isNotEmpty()) {
            g.setColour(juce::Colour(0xffffa500)); g.setFont(juce::Font(42.0f, juce::Font::bold));
            g.drawFittedText(inspectorChordName, textArea, juce::Justification::centred, 2, 0.2f);
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
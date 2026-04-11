#include "HeaderComponent.h"

namespace ChordMatrix {

    HeaderComponent::HeaderComponent(ChordMatrixAudioProcessor& p) : audioProcessor(p)
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

        setupCombo(timeSigNumMenu, timeSigLabel);
        timeSigNumMenu.onChange = [this] {
            audioProcessor.apvts.getParameter("timeSigNum")->setValueNotifyingHost((timeSigNumMenu.getSelectedId() - 1) / 14.0f);
            if (onRepaintRequest) onRepaintRequest(); // ★修正: 変更時に即時再描画
            };

        setupCombo(timeSigDenMenu, timeSigSlashLabel);
        timeSigDenMenu.addItem("4", 1); timeSigDenMenu.addItem("8", 2); timeSigDenMenu.addItem("16", 3);
        timeSigDenMenu.onChange = [this] {
            audioProcessor.apvts.getParameter("timeSigDen")->setValueNotifyingHost((timeSigDenMenu.getSelectedId() - 1) / 2.0f);
            updateTimeSigLimits();
            updateStepSizeMenu();
            if (onRepaintRequest) onRepaintRequest(); // ★修正: 変更時に即時再描画
            };

        setupCombo(stepSizeMenu, stepSizeLabel);
        stepSizeMenu.onChange = [this] {
            audioProcessor.apvts.getParameter("stepSize")->setValueNotifyingHost((stepSizeMenu.getSelectedId() - 1) / 2.0f);
            if (onRepaintRequest) onRepaintRequest(); // ★修正: 変更時に即時再描画
            };

        setupCombo(loopBarsMenu, barsLabel);
        loopBarsMenu.addItem("1", 1); loopBarsMenu.addItem("4", 2); loopBarsMenu.addItem("8", 3); loopBarsMenu.addItem("12", 4); loopBarsMenu.addItem("16", 5);
        loopBarsMenu.onChange = [this] {
            audioProcessor.apvts.getParameter("loopBars")->setValueNotifyingHost((loopBarsMenu.getSelectedId() - 1) / 4.0f);
            if (onRepaintRequest) onRepaintRequest(); // ★修正: 変更時に即時再描画
            };

        addAndMakeVisible(tempoSlider); addAndMakeVisible(tempoLabel);
        tempoSlider.setSliderStyle(juce::Slider::LinearBar);
        tempoSlider.setRange(20.0, 300.0, 1.0);
        tempoSlider.setColour(juce::Slider::trackColourId, juce::Colours::transparentBlack);
        tempoSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
        tempoSlider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff252525));
        tempoSlider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff3a3a3a));
        tempoLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
        tempoLabel.attachToComponent(&tempoSlider, true);
        tempoSlider.onValueChange = [this] {
            audioProcessor.apvts.getParameter("tempo")->setValueNotifyingHost((tempoSlider.getValue() - 20.0f) / 280.0f);
            };

        timeSigDenMenu.setSelectedId((int)*audioProcessor.apvts.getRawParameterValue("timeSigDen") + 1, juce::dontSendNotification);
        updateTimeSigLimits();
        updateStepSizeMenu();
    }

    HeaderComponent::~HeaderComponent() {}

    void HeaderComponent::updateTimeSigLimits() {
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

    void HeaderComponent::updateStepSizeMenu() {
        int denIdx = timeSigDenMenu.getSelectedId();
        int currentId = (int)*audioProcessor.apvts.getRawParameterValue("stepSize") + 1;

        stepSizeMenu.clear(juce::dontSendNotification);
        if (denIdx == 1) {
            stepSizeMenu.addItem("1/4", 1); stepSizeMenu.addItem("1/8", 2); stepSizeMenu.addItem("1/16", 3);
        }
        else if (denIdx == 2) {
            stepSizeMenu.addItem("1/8", 2); stepSizeMenu.addItem("1/16", 3);
            if (currentId == 1) currentId = 2;
        }
        else if (denIdx == 3) {
            stepSizeMenu.addItem("1/16", 3);
            currentId = 3;
        }

        stepSizeMenu.setSelectedId(currentId, juce::dontSendNotification);
        audioProcessor.apvts.getParameter("stepSize")->setValueNotifyingHost((currentId - 1) / 2.0f);
    }

    void HeaderComponent::resized() {
        tempoSlider.setBounds(700, 20, 60, 25);
        timeSigNumMenu.setBounds(830, 20, 60, 25);
        timeSigDenMenu.setBounds(905, 20, 60, 25);
        stepSizeMenu.setBounds(1030, 20, 70, 25);
        loopBarsMenu.setBounds(1160, 20, 60, 25);
    }

    void HeaderComponent::paint(juce::Graphics& g) {
        const auto bg = juce::Colour(0xff252525);
        const auto activeColor = juce::Colour(0xffffa500);
        const auto textLight = juce::Colours::white;

        g.fillAll(bg);
        g.setColour(juce::Colour(0xff3a3a3a));
        g.drawLine(0, (float)getHeight(), (float)getWidth(), (float)getHeight(), 2.0f);

        g.setColour(textLight); g.setFont(juce::Font(22.0f, juce::Font::bold));
        g.drawText("CHORD MATRIX", 25, 0, 180, getHeight(), juce::Justification::centredLeft);

        timeSigNumMenu.setSelectedId((int)*audioProcessor.apvts.getRawParameterValue("timeSigNum"), juce::dontSendNotification);
        timeSigDenMenu.setSelectedId((int)*audioProcessor.apvts.getRawParameterValue("timeSigDen") + 1, juce::dontSendNotification);
        stepSizeMenu.setSelectedId((int)*audioProcessor.apvts.getRawParameterValue("stepSize") + 1, juce::dontSendNotification);
        loopBarsMenu.setSelectedId((int)*audioProcessor.apvts.getRawParameterValue("loopBars") + 1, juce::dontSendNotification);
        tempoSlider.setValue(*audioProcessor.apvts.getRawParameterValue("tempo"), juce::dontSendNotification);

        g.setColour(juce::Colours::grey); g.setFont(16.0f);
        g.drawText("/", 890, 20, 15, 25, juce::Justification::centred);

        auto drawBtn = [&](int x, int y, int w, int h, const char* txt, bool active, juce::Colour c) {
            g.setColour(active ? c : juce::Colour(0xff3a3a3a));
            g.fillRoundedRectangle(static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h), 4.0f);
            g.setColour(active ? juce::Colour(0xff1a1a1a) : textLight); g.setFont(12.0f);
            g.drawText(txt, x, y, w, h, juce::Justification::centred);
            };

        drawBtn(220, 15, 60, 35, "SYNC", audioProcessor.isSyncEnabled, activeColor);
        drawBtn(290, 15, 60, 35, "PLAY", audioProcessor.isInternalPlaying, activeColor);
        drawBtn(360, 15, 60, 35, "STOP", !audioProcessor.isInternalPlaying && !audioProcessor.isSyncEnabled, juce::Colours::red.withAlpha(0.8f));
        drawBtn(430, 15, 60, 35, "FOLLOW", isFollowMode, activeColor);
        drawBtn(500, 15, 80, 35, "OPTIMIZE", false, textLight);
    }

    void HeaderComponent::mouseDown(const juce::MouseEvent& e) {
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
        if (juce::Rectangle<int>(430, 15, 60, 35).contains(e.getPosition())) {
            isFollowMode = !isFollowMode;
            if (onFollowModeChanged) onFollowModeChanged(isFollowMode);
        }
        if (juce::Rectangle<int>(500, 15, 80, 35).contains(e.getPosition())) { audioProcessor.optimizeVoicing(); }

        if (onRepaintRequest) onRepaintRequest();
        repaint();
    }

} // namespace ChordMatrix
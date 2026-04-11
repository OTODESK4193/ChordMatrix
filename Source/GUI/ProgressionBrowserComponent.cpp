#include "ProgressionBrowserComponent.h"
#include "../Engine/VoicingEngine.h"
#include "../Engine/ProgressionEngine.h"

namespace ChordMatrix {

    ProgressionBrowserComponent::ProgressionBrowserComponent(ChordMatrixAudioProcessor& p)
        : audioProcessor(p), categoryModel(*this)
    {
        loadPresets();

        addAndMakeVisible(categoryList);
        categoryList.setModel(&categoryModel);
        categoryList.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff1c1c1c));
        categoryList.setRowHeight(35);

        addAndMakeVisible(presetList);
        presetList.setModel(this);
        presetList.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff222222));
        presetList.setRowHeight(35);

        addAndMakeVisible(btnApply);
        btnApply.setColour(juce::TextButton::buttonColourId, juce::Colours::hotpink);
        btnApply.onClick = [this] {
            applyPresetToProcessor();
            if (onApplyPreset) onApplyPreset();
            };

        addAndMakeVisible(btnCancel);
        btnCancel.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff3a3a3a));
        btnCancel.onClick = [this] {
            if (onCancel) onCancel();
            };

        categoryList.selectRow(0);
    }

    ProgressionBrowserComponent::~ProgressionBrowserComponent() {}

    void ProgressionBrowserComponent::loadPresets() {
        categories.clear();
        const auto& dict = ProgressionEngine::getProgressionDictionary();

        for (const auto& p : dict) {
            auto it = std::find_if(categories.begin(), categories.end(), [&](const ProgressionCategory& c) { return c.name == p.category; });
            if (it != categories.end()) {
                it->presets.push_back(p);
            }
            else {
                categories.push_back({ p.category, {p} });
            }
        }
    }

    void ProgressionBrowserComponent::CategoryListModel::paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool isSelected) {
        if (row < 0 || row >= (int)owner.categories.size()) return;

        g.fillAll(isSelected ? juce::Colour(0xffffa500).withAlpha(0.3f) : juce::Colour(0xff252525));
        g.setColour(isSelected ? juce::Colours::white : juce::Colours::grey);
        g.setFont(14.0f);

        juce::String name = owner.categories[row].name;
        if (name.isNotEmpty()) {
            g.drawText(name, 15, 0, w - 30, h, juce::Justification::centredLeft, true);
        }
    }

    void ProgressionBrowserComponent::CategoryListModel::listBoxItemClicked(int row, const juce::MouseEvent&) {
        owner.selectedCategory = row;
        owner.selectedPreset = -1;
        owner.presetList.updateContent();
        owner.repaint();
    }

    int ProgressionBrowserComponent::getNumRows() {
        if (selectedCategory >= 0 && selectedCategory < (int)categories.size()) {
            return (int)categories[selectedCategory].presets.size();
        }
        return 0;
    }

    void ProgressionBrowserComponent::paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool isSelected) {
        if (selectedCategory < 0 || selectedCategory >= (int)categories.size()) return;
        if (row < 0 || row >= (int)categories[selectedCategory].presets.size()) return;

        const auto& preset = categories[selectedCategory].presets[row];

        g.fillAll(isSelected ? juce::Colours::hotpink.withAlpha(0.4f) : juce::Colours::transparentBlack);
        g.setColour(juce::Colours::white);
        g.setFont(14.0f);

        juce::String itemText = preset.name + "  (" + juce::String(preset.numBars) + " Bars)";
        if (itemText.isNotEmpty()) {
            g.drawText(itemText, 15, 0, w - 30, h, juce::Justification::centredLeft, true);
        }
    }

    void ProgressionBrowserComponent::listBoxItemClicked(int row, const juce::MouseEvent& e) {
        selectedPreset = row;
        repaint();
        if (e.getNumberOfClicks() == 2) {
            applyPresetToProcessor();
            if (onApplyPreset) onApplyPreset();
        }
    }

    void ProgressionBrowserComponent::applyPresetToProcessor() {
        if (selectedCategory < 0 || selectedCategory >= (int)categories.size()) return;
        if (selectedPreset < 0 || selectedPreset >= (int)categories[selectedCategory].presets.size()) return;

        const auto& preset = categories[selectedCategory].presets[selectedPreset];

        int editBar = (int)*audioProcessor.apvts.getRawParameterValue("editBar");
        int tsNum = (int)*audioProcessor.apvts.getRawParameterValue("timeSigNum");
        int tsDenIdx = (int)*audioProcessor.apvts.getRawParameterValue("timeSigDen");
        int tsDen = (tsDenIdx == 0) ? 4 : (tsDenIdx == 1) ? 8 : 16;
        int stepSizeIdx = (int)*audioProcessor.apvts.getRawParameterValue("stepSize");
        float ppqPerStep = (stepSizeIdx == 0) ? 1.0f : (stepSizeIdx == 1) ? 0.5f : 0.25f;

        float beatsPerBar = (float)tsNum * (4.0f / (float)tsDen);
        int stepsPerBar = std::max(1, juce::roundToInt(beatsPerBar / ppqPerStep));
        float ppqPerBeat = 4.0f / (float)tsDen;
        int stepsPerBeat = std::max(1, juce::roundToInt(ppqPerBeat / ppqPerStep));

        for (int b = 0; b < preset.numBars; ++b) {
            int targetBar = editBar + b;
            if (targetBar >= 16) break;

            int barStartStep = targetBar * stepsPerBar;
            for (int s = 0; s < stepsPerBar; ++s) {
                int step = barStartStep + s;
                auto& sData = audioProcessor.sequenceData[step];
                sData.inversion = 0;
                sData.shift = 0;
                sData.voicingMode = 0;

                for (int v = 0; v < 7; ++v) {
                    sData.voices[v].isActive = false;
                    sData.voices[v].octaveShift = 0;
                    sData.voices[v].accidental = 0;
                }
                audioProcessor.previewSequenceData[step] = sData;
            }
        }

        int baseKey = audioProcessor.sequenceData[editBar * stepsPerBar].keyRoot;
        int baseScale = audioProcessor.sequenceData[editBar * stepsPerBar].scaleType;

        for (const auto& chord : preset.chords) {
            int targetBar = editBar + (chord.startBeat / (int)beatsPerBar);
            if (targetBar >= 16) continue;

            int targetStep = targetBar * stepsPerBar + (chord.startBeat % (int)beatsPerBar) * stepsPerBeat;

            if (targetStep < ChordMatrix::TotalSteps) {
                auto& sData = audioProcessor.sequenceData[targetStep];
                sData.keyRoot = baseKey;
                sData.scaleType = baseScale;
                sData.chordDegree = chord.chordDegree;
                sData.voicingMode = chord.voicingMode;
                sData.gateLength = chord.lengthBeats * ppqPerBeat;

                bool isAuto = VoicingEngine::isAutoPattern(chord.voicingMode);
                sData.voices[0].isActive = (chord.accRoot != -128);
                sData.voices[0].accidental = (chord.accRoot == -128) ? 0 : chord.accRoot;
                sData.voices[0].octaveShift = (chord.accRoot == -128 && isAuto) ? -128 : 0;

                sData.voices[1].isActive = (chord.acc3rd != -128);
                sData.voices[1].accidental = (chord.acc3rd == -128) ? 0 : chord.acc3rd;
                sData.voices[1].octaveShift = (chord.acc3rd == -128 && isAuto) ? -128 : 0;

                sData.voices[2].isActive = (chord.acc5th != -128);
                sData.voices[2].accidental = (chord.acc5th == -128) ? 0 : chord.acc5th;
                sData.voices[2].octaveShift = (chord.acc5th == -128 && isAuto) ? -128 : 0;

                sData.voices[3].isActive = (chord.acc7th != -128);
                sData.voices[3].accidental = (chord.acc7th == -128) ? 0 : chord.acc7th;
                sData.voices[3].octaveShift = (chord.acc7th == -128 && isAuto) ? -128 : 0;

                audioProcessor.previewSequenceData[targetStep] = sData;
            }
        }
    }

    void ProgressionBrowserComponent::resized() {
        auto area = getLocalBounds();
        auto bottomArea = area.removeFromBottom(80);
        btnApply.setBounds(bottomArea.removeFromRight(150).reduced(15));
        btnCancel.setBounds(bottomArea.removeFromRight(100).reduced(15, 20));
        categoryList.setBounds(area.removeFromLeft(getWidth() / 3).reduced(10));
        presetList.setBounds(area.reduced(10));
    }

    void ProgressionBrowserComponent::paint(juce::Graphics& g) {
        g.fillAll(juce::Colour(0xff1c1c1c));
        g.setColour(juce::Colour(0xff333333));
        g.drawRect(getLocalBounds(), 2);

        if (selectedCategory >= 0 && selectedCategory < (int)categories.size() &&
            selectedPreset >= 0 && selectedPreset < (int)categories[selectedCategory].presets.size()) {

            const auto& preset = categories[selectedCategory].presets[selectedPreset];
            if (preset.previewText.isNotEmpty()) {
                g.setColour(juce::Colours::white);
                g.setFont(juce::Font(18.0f, juce::Font::bold));
                g.drawText(preset.previewText, 20, getHeight() - 60, getWidth() - 300, 40, juce::Justification::centredLeft);
            }
        }
        else {
            g.setColour(juce::Colours::grey);
            g.setFont(16.0f);
            g.drawText("Select a progression to preview.", 20, getHeight() - 60, getWidth() - 300, 40, juce::Justification::centredLeft);
        }
    }
} // namespace ChordMatrix
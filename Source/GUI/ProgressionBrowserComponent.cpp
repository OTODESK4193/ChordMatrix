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
            audioProcessor.isProgressionPreviewPlaying.store(false);

            applyPresetToProcessor();
            if (onApplyPreset) onApplyPreset();
            };

        addAndMakeVisible(btnCancel);
        btnCancel.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff3a3a3a));
        btnCancel.onClick = [this] {
            audioProcessor.isProgressionPreviewPlaying.store(false);

            if (onCancel) onCancel();
            };

        addAndMakeVisible(btnPreview);
        btnPreview.setColour(juce::TextButton::buttonColourId, juce::Colours::cyan.withAlpha(0.6f));
        btnPreview.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        btnPreview.onClick = [this] {
            previewPreset();
            };

        categoryList.selectRow(0);

        // ★タイマー開始(30Hz)でPreviewボタンの点灯状態を監視
        startTimerHz(30);
    }

    ProgressionBrowserComponent::~ProgressionBrowserComponent() {}

    // ★タイマーでPreviewボタンの色を自動更新（再生が終わったら自動消灯）
    void ProgressionBrowserComponent::timerCallback() {
        bool isPlaying = audioProcessor.isProgressionPreviewPlaying.load();
        btnPreview.setColour(juce::TextButton::buttonColourId,
            isPlaying ? juce::Colours::hotpink : juce::Colours::cyan.withAlpha(0.6f));
    }

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
            audioProcessor.isProgressionPreviewPlaying.store(false);
            applyPresetToProcessor();
            if (onApplyPreset) onApplyPreset();
        }
    }

    void ProgressionBrowserComponent::applyPresetToProcessor() {
        applyPresetToArray(audioProcessor.sequenceData);
        audioProcessor.previewSequenceData = audioProcessor.sequenceData;
    }

    void ProgressionBrowserComponent::previewPreset() {
        if (selectedCategory < 0 || selectedCategory >= (int)categories.size()) return;
        if (selectedPreset < 0 || selectedPreset >= (int)categories[selectedCategory].presets.size()) return;

        // ★すでに再生中なら強制停止（トグル動作）
        if (audioProcessor.isProgressionPreviewPlaying.load()) {
            audioProcessor.isProgressionPreviewPlaying.store(false);
            audioProcessor.previewEndTimePPQ.store(-1.0);
            return;
        }

        const auto& preset = categories[selectedCategory].presets[selectedPreset];

        // プレビュー用配列に書き込み
        audioProcessor.previewSequenceData = audioProcessor.sequenceData;
        applyPresetToArray(audioProcessor.previewSequenceData);

        int editBar = (int)*audioProcessor.apvts.getRawParameterValue("editBar");
        int tsNum = (int)*audioProcessor.apvts.getRawParameterValue("timeSigNum");
        int tsDenIdx = (int)*audioProcessor.apvts.getRawParameterValue("timeSigDen");
        int tsDen = (tsDenIdx == 0) ? 4 : (tsDenIdx == 1) ? 8 : 16;
        float beatsPerBar = static_cast<float>(tsNum) * (4.0f / static_cast<float>(tsDen));
        int internalStepsPerBar = juce::roundToInt(beatsPerBar / 0.25f);

        // ★開始位置と終了位置をPPQで計算してエンジンに渡す
        double startPPQ = static_cast<double>(editBar) * static_cast<double>(internalStepsPerBar) * 0.25;
        double lengthPPQ = static_cast<double>(preset.numBars) * static_cast<double>(internalStepsPerBar) * 0.25;

        audioProcessor.internalPPQ = startPPQ;
        audioProcessor.previewEndTimePPQ.store(startPPQ + lengthPPQ);
        audioProcessor.isProgressionPreviewPlaying.store(true);
    }

    void ProgressionBrowserComponent::applyPresetToArray(std::array<StepData, TotalSteps>& destArray) {
        if (selectedCategory < 0 || selectedCategory >= (int)categories.size()) return;
        if (selectedPreset < 0 || selectedPreset >= (int)categories[selectedCategory].presets.size()) return;

        const auto& preset = categories[selectedCategory].presets[selectedPreset];

        int editBar = (int)*audioProcessor.apvts.getRawParameterValue("editBar");
        int tsNum = (int)*audioProcessor.apvts.getRawParameterValue("timeSigNum");
        int tsDenIdx = (int)*audioProcessor.apvts.getRawParameterValue("timeSigDen");
        int tsDen = (tsDenIdx == 0) ? 4 : (tsDenIdx == 1) ? 8 : 16;

        float beatsPerBar = static_cast<float>(tsNum) * (4.0f / static_cast<float>(tsDen));
        int internalStepsPerBar = juce::roundToInt(beatsPerBar / 0.25f);

        float ppqPerBeat = 4.0f / static_cast<float>(tsDen);
        int internalStepsPerBeat = juce::roundToInt(ppqPerBeat / 0.25f);

        for (int b = 0; b < preset.numBars; ++b) {
            int targetBar = editBar + b;
            if (targetBar >= 16) break;

            int barStartStep = targetBar * internalStepsPerBar;
            for (int s = 0; s < internalStepsPerBar; ++s) {
                int step = barStartStep + s;
                auto& sData = destArray[step];

                bool wasLocked = sData.isLocked;
                sData = {};
                sData.isLocked = wasLocked;
            }
        }

        int baseKey = audioProcessor.sequenceData[editBar * internalStepsPerBar].keyRoot;
        int baseScale = audioProcessor.sequenceData[editBar * internalStepsPerBar].scaleType;

        for (const auto& chord : preset.chords) {
            int totalBeatsOffset = chord.startBeat;
            int targetBar = editBar + (totalBeatsOffset / tsNum);
            if (targetBar >= 16) continue;

            int targetStep = (targetBar * internalStepsPerBar) + ((totalBeatsOffset % tsNum) * internalStepsPerBeat);

            if (targetStep < ChordMatrix::TotalSteps) {
                auto& sData = destArray[targetStep];

                if (sData.isLocked) continue;

                sData.keyRoot = baseKey;
                sData.scaleType = baseScale;
                sData.chordDegree = chord.chordDegree;
                sData.voicingMode = chord.voicingMode;
                sData.gateLength = static_cast<float>(chord.lengthBeats) * ppqPerBeat;

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
            }
        }
    }

    void ProgressionBrowserComponent::resized() {
        auto area = getLocalBounds();
        auto bottomArea = area.removeFromBottom(80);

        btnApply.setBounds(bottomArea.removeFromRight(150).reduced(15));
        btnCancel.setBounds(bottomArea.removeFromRight(100).reduced(15, 20));
        btnPreview.setBounds(bottomArea.removeFromRight(100).reduced(15, 20));

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
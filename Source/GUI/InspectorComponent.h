#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../PluginProcessor.h"

namespace ChordMatrix {

    class InspectorComponent : public juce::Component
    {
    public:
        InspectorComponent(ChordMatrixAudioProcessor& p);
        ~InspectorComponent() override;

        void paint(juce::Graphics& g) override;
        void resized() override;
        void mouseDown(const juce::MouseEvent& e) override;

        void setSelectedStep(int stepIndex);
        int getSelectedStep() const { return selectedStep; }

        int getEffectiveStep(int targetS) const;

        std::function<void()> onSettingsChanged;

    private:
        ChordMatrixAudioProcessor& audioProcessor;

        juce::ComboBox stepKeyMenu, stepScaleMenu, stepDegreeMenu, voicingMenu;
        juce::Slider stepShiftSlider;
        juce::Label stepKeyLabel{ "", "KEY" }, stepScaleLabel{ "", "SCALE" }, stepDegreeLabel{ "", "DEGREE" }, voicingLabel{ "", "VOICING" }, stepShiftLabel{ "", "SHIFT" };

        juce::TextButton btnOptimize{ "OPTIMIZE" };

        // ------------------------------------------------------------
        // 変更箇所 (InspectorComponent.h の private: 内)
        // ------------------------------------------------------------
        int selectedStep = 0;
        int scopeKey = 0, scopeScale = 0, scopeDegree = 0, scopeVoicing = 0, scopeOptimize = 0, scopeShift = 0;

        // ★新規追加: OPTIMIZEボタンを押すたびにインクリメントされる代替案インデックス
        int optimizeAltIndex = 0;

        void updateInspector();
        int getStepsPerBar() const;
        float getPpqPerStep() const;

        void applyScope(int scopeType, std::function<void(int)> setterFunction);

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InspectorComponent)
    };

} // namespace ChordMatrix
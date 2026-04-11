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

        std::function<void()> onSettingsChanged;

    private:
        ChordMatrixAudioProcessor& audioProcessor;

        juce::ComboBox stepKeyMenu, stepScaleMenu, stepDegreeMenu, voicingMenu;
        juce::Slider stepShiftSlider;
        juce::Label stepKeyLabel{ "", "KEY" }, stepScaleLabel{ "", "SCALE" }, stepDegreeLabel{ "", "DEGREE" }, voicingLabel{ "", "VOICING" }, stepShiftLabel{ "", "SHIFT" };

        int selectedStep = 0;
        int scopeKey = 0, scopeScale = 0, scopeDegree = 0, scopeVoicing = 0, scopeShift = 0;

        void updateInspector();
        int getStepsPerBar() const;
        float getPpqPerStep() const;
        int getEffectiveStep(int targetS) const;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InspectorComponent)
    };

} // namespace ChordMatrix
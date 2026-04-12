#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../PluginProcessor.h"

namespace ChordMatrix {

    class HeaderComponent : public juce::Component
    {
    public:
        HeaderComponent(ChordMatrixAudioProcessor& p);
        ~HeaderComponent() override;

        // ------------------------------------------------------------
        // 変更後 (HeaderComponent.h)
        // ------------------------------------------------------------
        void paint(juce::Graphics& g) override;
        void resized() override;
        void mouseDown(const juce::MouseEvent& e) override;

        void updateUI(); // ★新規追加: 安全なUI更新用メソッド

        std::function<void()> onRepaintRequest;        std::function<void(bool)> onFollowModeChanged;

    private:
        ChordMatrixAudioProcessor& audioProcessor;

        juce::ComboBox timeSigNumMenu, timeSigDenMenu, stepSizeMenu, loopBarsMenu;
        juce::Slider tempoSlider;
        juce::Label timeSigLabel{ "", "SIG:" }, stepSizeLabel{ "", "STEP:" }, barsLabel{ "", "BARS:" }, tempoLabel{ "", "BPM:" };
        juce::Label timeSigSlashLabel{ "", "/" };

        bool isFollowMode = false;

        void updateTimeSigLimits();
        void updateStepSizeMenu();

        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HeaderComponent)
    };

} // namespace ChordMatrix
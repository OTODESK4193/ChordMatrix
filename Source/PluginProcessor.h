// Source/PluginProcessor.h
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Data/StepData.h"
#include "Engine/MusicTheory.h"

class ChordMatrixAudioProcessor : public juce::AudioProcessor
{
public:
    ChordMatrixAudioProcessor();
    ~ChordMatrixAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "ChordMatrix"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int index) override {}
    const juce::String getProgramName(int index) override { return {}; }
    void changeProgramName(int index, const juce::String& newName) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    void optimizeVoicing();

    juce::AudioProcessorValueTreeState apvts;
    std::array<ChordMatrix::StepData, ChordMatrix::TotalSteps> sequenceData;

    bool isInternalPlaying = false;
    bool isSyncEnabled = true;
    bool isPlaying = false;
    int currentGlobalStep = -1;
    float currentBPM = 120.0f;
    double internalPPQ = 0.0;

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    double lastPPQ = 0.0;
    double currentNoteOffTimePPQ[ChordMatrix::NumVoices];
    int currentNoteOnPitch[ChordMatrix::NumVoices];

    float currentSampleRate = 44100.0f;
    std::array<juce::ADSR, ChordMatrix::NumVoices> adsrs;
    std::array<float, ChordMatrix::NumVoices> phases = { 0 };
    std::array<float, ChordMatrix::NumVoices> phaseDeltas = { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChordMatrixAudioProcessor)
};
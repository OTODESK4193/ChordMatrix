// Source/PluginProcessor.cpp
#include "PluginProcessor.h"
#include "PluginEditor.h"

ChordMatrixAudioProcessor::ChordMatrixAudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    lastPPQ = 0.0; internalPPQ = 0.0; isInternalPlaying = false; isSyncEnabled = true; isPlaying = false;
    currentGlobalStep = -1; currentBPM = 120.0f;

    for (int i = 0; i < ChordMatrix::NumVoices; ++i) {
        currentNoteOffTimePPQ[i] = -1.0;
        currentNoteOnPitch[i] = 0;
        phases[i] = 0.0f;
        phaseDeltas[i] = 0.0f;
    }

    for (int s = 0; s < ChordMatrix::TotalSteps; ++s) {
        sequenceData[s].gateLength = 0.25f;
        for (int v = 0; v < ChordMatrix::NumVoices; ++v) {
            sequenceData[s].voices[v].isActive = false;
            sequenceData[s].voices[v].octaveShift = 0;
            sequenceData[s].voices[v].accidental = 0;
        }
    }
    for (int b = 0; b < ChordMatrix::TotalBeats; ++b) {
        beatSettings[b].keyRoot = 0; beatSettings[b].chordDegree = 0; beatSettings[b].chordType = 0; beatSettings[b].tensionType = 0;
    }
}

ChordMatrixAudioProcessor::~ChordMatrixAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout ChordMatrixAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{ "loopBars", 1 }, "Bars", juce::StringArray{ "4", "8", "12", "16" }, 0));
    params.push_back(std::make_unique<juce::AudioParameterInt>(juce::ParameterID{ "editBar", 1 }, "Edit", 0, 15, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ "tempo", 1 }, "BPM", 20.0f, 300.0f, 120.0f));
    params.push_back(std::make_unique<juce::AudioParameterInt>(juce::ParameterID{ "rootKey", 1 }, "Key", 0, 11, 0));
    // 要件④: 拍子と要件⑥: STEPサイズ
    params.push_back(std::make_unique<juce::AudioParameterInt>(juce::ParameterID{ "timeSigNum", 1 }, "TimeSigNum", 4, 15, 4));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{ "timeSigDen", 1 }, "TimeSigDen", juce::StringArray{ "4", "8", "16" }, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{ "stepSize", 1 }, "StepSize", juce::StringArray{ "1/4", "1/8", "1/16" }, 2));

    return { params.begin(), params.end() };
}

void ChordMatrixAudioProcessor::optimizeVoicing()
{
    int loopIdx = (int)*apvts.getRawParameterValue("loopBars");
    int numBars = (loopIdx + 1) * 4;
    ChordMatrix::MusicTheory::optimizeVoiceLeading(sequenceData, beatSettings, numBars * 16);
}

void ChordMatrixAudioProcessor::prepareToPlay(double sampleRate, int) {
    lastPPQ = 0.0; internalPPQ = 0.0; currentGlobalStep = -1;
    currentSampleRate = static_cast<float>(sampleRate);

    // 優しい音色（エレピ風）のADSR設定
    juce::ADSR::Parameters adsrParams{ 0.05f, 0.3f, 0.4f, 0.8f };
    for (auto& env : adsrs) {
        env.setSampleRate(sampleRate);
        env.setParameters(adsrParams);
        env.reset();
    }
}

void ChordMatrixAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    buffer.clear(); // オーディオ出力用クリア

    auto* playHead = getPlayHead();
    float hostBPM = 120.0f;
    double ppq = internalPPQ;

    if (playHead != nullptr && playHead->getPosition().hasValue()) {
        const auto& pos = *playHead->getPosition();
        if (auto b = pos.getBpm()) hostBPM = (float)*b;
        if (isSyncEnabled) {
            if (auto p = pos.getPpqPosition()) ppq = *p;
            isPlaying = pos.getIsPlaying();
            currentBPM = hostBPM;
        }
    }

    float manualBPM = *apvts.getRawParameterValue("tempo");
    if (!isSyncEnabled) {
        if (isInternalPlaying) {
            double samplesPerBeat = (60.0 / (double)manualBPM) * getSampleRate();
            internalPPQ += (double)buffer.getNumSamples() / samplesPerBeat;
            ppq = internalPPQ;
        }
        isPlaying = isInternalPlaying;
        currentBPM = manualBPM;
    }

    // PPQ解像度の計算 (1/4=1.0, 1/8=0.5, 1/16=0.25)
    int stepSizeIdx = (int)*apvts.getRawParameterValue("stepSize");
    double ppqPerStep = (stepSizeIdx == 0) ? 1.0 : (stepSizeIdx == 1) ? 0.5 : 0.25;

    int loopIdx = (int)*apvts.getRawParameterValue("loopBars");
    int numBars = (loopIdx + 1) * 4;
    int totalStepsInLoop = numBars * 16; // 常に16列のUIベースとして扱う

    if (ppq < lastPPQ || (!isPlaying && lastPPQ > 0.0)) {
        for (int i = 0; i < ChordMatrix::NumVoices; ++i) {
            if (currentNoteOffTimePPQ[i] > 0.0) {
                midiMessages.addEvent(juce::MidiMessage::noteOff(1, currentNoteOnPitch[i]), 0);
                currentNoteOffTimePPQ[i] = -1.0;
                adsrs[i].noteOff();
            }
        }
        currentGlobalStep = -1;
        if (!isSyncEnabled && !isInternalPlaying) internalPPQ = 0.0;
    }
    lastPPQ = ppq;

    for (int i = 0; i < ChordMatrix::NumVoices; ++i) {
        if (currentNoteOffTimePPQ[i] > 0.0 && ppq >= currentNoteOffTimePPQ[i]) {
            midiMessages.addEvent(juce::MidiMessage::noteOff(1, currentNoteOnPitch[i]), 0);
            currentNoteOffTimePPQ[i] = -1.0;
            adsrs[i].noteOff();
        }
    }

    if (isPlaying) {
        int stepIdx = static_cast<int>(ppq / ppqPerStep);
        int stepInLoop = stepIdx % totalStepsInLoop;

        if (stepIdx != currentGlobalStep) {
            const auto& sData = sequenceData[stepInLoop];
            const auto& bData = beatSettings[stepInLoop / 4];
            int base = 60 + bData.keyRoot + ChordMatrix::MusicTheory::getDegreeInterval(bData.chordDegree);
            auto intervals = ChordMatrix::MusicTheory::getChordIntervals(bData.chordType, bData.tensionType);

            for (int i = 0; i < ChordMatrix::NumVoices; ++i) {
                if (sData.voices[i].isActive) {
                    if (currentNoteOffTimePPQ[i] > 0.0) {
                        midiMessages.addEvent(juce::MidiMessage::noteOff(1, currentNoteOnPitch[i]), 0);
                        adsrs[i].noteOff();
                    }

                    int p = juce::jlimit(0, 127, base + intervals[i] + sData.voices[i].accidental + (sData.voices[i].octaveShift * 12));
                    midiMessages.addEvent(juce::MidiMessage::noteOn(1, p, sData.velocity / 127.0f), 0);
                    currentNoteOnPitch[i] = p;
                    currentNoteOffTimePPQ[i] = ppq + sData.gateLength;

                    // 内蔵シンセの発音
                    float freq = 440.0f * std::pow(2.0f, (p - 69) / 12.0f);
                    phaseDeltas[i] = (freq * juce::MathConstants<float>::twoPi) / currentSampleRate;
                    adsrs[i].noteOn();
                }
            }
            currentGlobalStep = stepIdx;
        }
    }

    // オーディオ（正弦波）レンダリング
    float* left = buffer.getWritePointer(0);
    float* right = buffer.getWritePointer(1);
    for (int s = 0; s < buffer.getNumSamples(); ++s) {
        float mix = 0.0f;
        for (int i = 0; i < ChordMatrix::NumVoices; ++i) {
            if (adsrs[i].isActive()) {
                mix += std::sin(phases[i]) * adsrs[i].getNextSample();
                phases[i] += phaseDeltas[i];
                if (phases[i] >= juce::MathConstants<float>::twoPi) phases[i] -= juce::MathConstants<float>::twoPi;
            }
        }
        mix *= 0.1f; // マスターボリューム減衰
        left[s] += mix;
        if (buffer.getNumChannels() > 1) right[s] += mix;
    }
}

void ChordMatrixAudioProcessor::getStateInformation(juce::MemoryBlock& d) {
    auto state = apvts.copyState();
    state.setProperty("seq", juce::MemoryBlock(sequenceData.data(), sizeof(sequenceData)), nullptr);
    state.setProperty("beat", juce::MemoryBlock(beatSettings.data(), sizeof(beatSettings)), nullptr);
    std::unique_ptr<juce::XmlElement> xml(state.createXml()); copyXmlToBinary(*xml, d);
}
void ChordMatrixAudioProcessor::setStateInformation(const void* d, int s) {
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(d, s));
    if (xml) {
        auto tree = juce::ValueTree::fromXml(*xml); apvts.replaceState(tree);
        if (tree.hasProperty("seq")) { auto mb = tree.getProperty("seq").getBinaryData(); memcpy(sequenceData.data(), mb->getData(), sizeof(sequenceData)); }
        if (tree.hasProperty("beat")) { auto mb = tree.getProperty("beat").getBinaryData(); memcpy(beatSettings.data(), mb->getData(), sizeof(beatSettings)); }
    }
}
void ChordMatrixAudioProcessor::releaseResources() {}
bool ChordMatrixAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const { return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo(); }
juce::AudioProcessorEditor* ChordMatrixAudioProcessor::createEditor() { return new ChordMatrixAudioProcessorEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ChordMatrixAudioProcessor(); }
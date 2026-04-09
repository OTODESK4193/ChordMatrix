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
        phases[i] = 0.0f; phaseDeltas[i] = 0.0f;
    }

    // 【重要】未初期化メモリによる-51バグを防ぐため、完全な初期化を保証
    for (int s = 0; s < ChordMatrix::TotalSteps; ++s) {
        sequenceData[s].gateLength = 0.25f;
        sequenceData[s].velocity = 100; // 必須！
        sequenceData[s].keyRoot = 0;
        sequenceData[s].chordDegree = 0;
        sequenceData[s].chordType = 0;
        sequenceData[s].tensionType = 0;
        for (int v = 0; v < ChordMatrix::NumVoices; ++v) {
            sequenceData[s].voices[v].isActive = false;
            sequenceData[s].voices[v].octaveShift = 0;
            sequenceData[s].voices[v].accidental = 0;
        }
    }
}

ChordMatrixAudioProcessor::~ChordMatrixAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout ChordMatrixAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    params.push_back(std::make_unique<juce::AudioParameterInt>(juce::ParameterID{ "loopBars", 1 }, "Bars", 1, 16, 4));
    params.push_back(std::make_unique<juce::AudioParameterInt>(juce::ParameterID{ "editBar", 1 }, "Edit", 0, 15, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ "tempo", 1 }, "BPM", 20.0f, 300.0f, 120.0f));
    params.push_back(std::make_unique<juce::AudioParameterInt>(juce::ParameterID{ "timeSigNum", 1 }, "TimeSigNum", 1, 15, 4));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{ "timeSigDen", 1 }, "TimeSigDen", juce::StringArray{ "4", "8", "16" }, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{ "stepSize", 1 }, "StepSize", juce::StringArray{ "1/4", "1/8", "1/16" }, 2));
    return { params.begin(), params.end() };
}

void ChordMatrixAudioProcessor::optimizeVoicing()
{
    int numBars = (int)*apvts.getRawParameterValue("loopBars");
    int tsNum = (int)*apvts.getRawParameterValue("timeSigNum");
    int tsDenIdx = (int)*apvts.getRawParameterValue("timeSigDen");
    int tsDen = (tsDenIdx == 0) ? 4 : (tsDenIdx == 1) ? 8 : 16;
    int stepSizeIdx = (int)*apvts.getRawParameterValue("stepSize");
    float ppqPerStep = (stepSizeIdx == 0) ? 1.0f : (stepSizeIdx == 1) ? 0.5f : 0.25f;
    float beatsPerBar = tsNum * (4.0f / tsDen);
    int stepsPerBar = juce::roundToInt(beatsPerBar / ppqPerStep);
    if (stepsPerBar < 1) stepsPerBar = 1;

    ChordMatrix::MusicTheory::optimizeVoiceLeading(sequenceData, numBars * stepsPerBar);
}

void ChordMatrixAudioProcessor::prepareToPlay(double sampleRate, int) {
    lastPPQ = 0.0; internalPPQ = 0.0; currentGlobalStep = -1;
    currentSampleRate = static_cast<float>(sampleRate);

    juce::ADSR::Parameters adsrParams{ 0.05f, 0.3f, 0.4f, 0.8f };
    for (auto& env : adsrs) {
        env.setSampleRate(sampleRate);
        env.setParameters(adsrParams);
        env.reset();
    }
}

void ChordMatrixAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    buffer.clear();

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

    int tsNum = (int)*apvts.getRawParameterValue("timeSigNum");
    int tsDenIdx = (int)*apvts.getRawParameterValue("timeSigDen");
    int tsDen = (tsDenIdx == 0) ? 4 : (tsDenIdx == 1) ? 8 : 16;
    int stepSizeIdx = (int)*apvts.getRawParameterValue("stepSize");
    float ppqPerStep = (stepSizeIdx == 0) ? 1.0f : (stepSizeIdx == 1) ? 0.5f : 0.25f;

    float beatsPerBar = tsNum * (4.0f / tsDen);
    int stepsPerBar = juce::roundToInt(beatsPerBar / ppqPerStep);
    if (stepsPerBar < 1) stepsPerBar = 1;

    int numBars = (int)*apvts.getRawParameterValue("loopBars");
    int totalStepsInLoop = numBars * stepsPerBar;

    if (ppq < lastPPQ || (!isPlaying && lastPPQ > 0.0)) {
        for (int i = 0; i < ChordMatrix::NumVoices; ++i) {
            if (currentNoteOffTimePPQ[i] > 0.0) {
                midiMessages.addEvent(juce::MidiMessage::noteOff(1, currentNoteOnPitch[i]), 0);
                currentNoteOffTimePPQ[i] = -1.0;
                adsrs[i].noteOff();
            }
        }
        currentGlobalStep = -1;
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
            int base = 60 + sData.keyRoot + ChordMatrix::MusicTheory::getDegreeInterval(sData.chordDegree);
            auto intervals = ChordMatrix::MusicTheory::getChordIntervals(sData.chordType, sData.tensionType);

            for (int i = 0; i < ChordMatrix::NumVoices; ++i) {
                if (sData.voices[i].isActive) {
                    if (currentNoteOffTimePPQ[i] > 0.0) {
                        midiMessages.addEvent(juce::MidiMessage::noteOff(1, currentNoteOnPitch[i]), 0);
                        adsrs[i].noteOff();
                    }
                    int p = juce::jlimit(0, 127, base + intervals[i] + sData.voices[i].accidental + (sData.voices[i].octaveShift * 12));

                    // クラッシュ原因の完全排除（安全なVelocity範囲を保証）
                    float safeVel = juce::jlimit(0.0f, 1.0f, sData.velocity / 127.0f);

                    midiMessages.addEvent(juce::MidiMessage::noteOn(1, p, safeVel), 0);
                    currentNoteOnPitch[i] = p;
                    currentNoteOffTimePPQ[i] = ppq + sData.gateLength;

                    float freq = 440.0f * std::pow(2.0f, (p - 69) / 12.0f);
                    phaseDeltas[i] = (freq * juce::MathConstants<float>::twoPi) / currentSampleRate;
                    adsrs[i].noteOn();
                }
            }
            currentGlobalStep = stepIdx;
        }
    }

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
        mix *= 0.1f;
        left[s] += mix;
        if (buffer.getNumChannels() > 1) right[s] += mix;
    }
}

void ChordMatrixAudioProcessor::getStateInformation(juce::MemoryBlock& d) {
    auto state = apvts.copyState();
    state.setProperty("seq", juce::MemoryBlock(sequenceData.data(), sizeof(sequenceData)), nullptr);
    std::unique_ptr<juce::XmlElement> xml(state.createXml()); copyXmlToBinary(*xml, d);
}

void ChordMatrixAudioProcessor::setStateInformation(const void* d, int s) {
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(d, s));
    if (xml) {
        auto tree = juce::ValueTree::fromXml(*xml); apvts.replaceState(tree);
        if (tree.hasProperty("seq")) {
            auto mb = tree.getProperty("seq").getBinaryData();
            size_t bytesToCopy = juce::jmin((size_t)mb->getSize(), sizeof(sequenceData));
            memcpy(sequenceData.data(), mb->getData(), bytesToCopy);

            // 【重要】DAWからロードした古いゴミデータ（0xCD）によるクラッシュを完全に除染する
            for (auto& step : sequenceData) {
                step.velocity = juce::jlimit((uint8_t)0, (uint8_t)127, step.velocity);
                step.gateLength = juce::jlimit(0.01f, 32.0f, step.gateLength);
                if (std::isnan(step.gateLength)) step.gateLength = 0.25f;
                step.keyRoot = juce::jlimit(0, 11, step.keyRoot);
                step.chordDegree = juce::jlimit(0, 6, step.chordDegree);
                step.chordType = juce::jlimit(0, 5, step.chordType);
                step.tensionType = juce::jlimit(0, 4, step.tensionType);
                for (auto& v : step.voices) {
                    v.octaveShift = juce::jlimit((int8_t)-4, (int8_t)4, v.octaveShift);
                    v.accidental = juce::jlimit((int8_t)-2, (int8_t)2, v.accidental);
                }
            }
        }
    }
}
void ChordMatrixAudioProcessor::releaseResources() {}
bool ChordMatrixAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const { return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo(); }
juce::AudioProcessorEditor* ChordMatrixAudioProcessor::createEditor() { return new ChordMatrixAudioProcessorEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ChordMatrixAudioProcessor(); }
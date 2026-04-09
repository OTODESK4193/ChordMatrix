#include "PluginProcessor.h"
#include "PluginEditor.h"

ChordMatrixAudioProcessor::ChordMatrixAudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    lastPPQ = 0.0; internalPPQ = 0.0; isInternalPlaying = false; isSyncEnabled = true; isPlaying = false;
    currentGlobalStep = -1; currentBPM = 120.0f;
    for (int i = 0; i < ChordMatrix::NumVoices; ++i) { currentNoteOffTimePPQ[i] = -1.0; currentNoteOnPitch[i] = 0; }
    for (int s = 0; s < ChordMatrix::TotalSteps; ++s) {
        for (int v = 0; v < ChordMatrix::NumVoices; ++v) sequenceData[s].voices[v].isActive = false;
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
    return { params.begin(), params.end() };
}

void ChordMatrixAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    auto* playHead = getPlayHead();
    if (playHead == nullptr) return;
    auto optPos = playHead->getPosition();
    if (!optPos.hasValue()) return;

    const auto& pos = *optPos;
    float hostBPM = 120.0f; if (auto b = pos.getBpm()) hostBPM = (float)*b;
    float manualBPM = *apvts.getRawParameterValue("tempo");

    double ppq = 0.0;
    if (isSyncEnabled) {
        if (auto p = pos.getPpqPosition()) ppq = *p;
        isPlaying = pos.getIsPlaying();
        currentBPM = hostBPM;
    }
    else {
        if (isInternalPlaying) {
            double samplesPerBeat = (60.0 / (double)manualBPM) * getSampleRate();
            internalPPQ += (double)buffer.getNumSamples() / samplesPerBeat;
            ppq = internalPPQ;
        }
        isPlaying = isInternalPlaying;
        currentBPM = manualBPM;
    }

    int loopIdx = (int)*apvts.getRawParameterValue("loopBars");
    int numBars = (loopIdx + 1) * 4;
    int totalStepsInLoop = numBars * 16;

    if (ppq < lastPPQ || (!isPlaying && lastPPQ > 0.0)) {
        for (int i = 0; i < ChordMatrix::NumVoices; ++i) {
            if (currentNoteOffTimePPQ[i] > 0.0) {
                midiMessages.addEvent(juce::MidiMessage::noteOff(1, currentNoteOnPitch[i]), 0);
                currentNoteOffTimePPQ[i] = -1.0;
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
        }
    }

    if (!isPlaying) return;

    int stepIdx = static_cast<int>(ppq * 4.0);
    int stepInLoop = stepIdx % totalStepsInLoop;

    if (stepIdx != currentGlobalStep) {
        for (int i = 0; i < ChordMatrix::NumVoices; ++i) {
            if (currentNoteOffTimePPQ[i] > 0.0) {
                midiMessages.addEvent(juce::MidiMessage::noteOff(1, currentNoteOnPitch[i]), 0);
                currentNoteOffTimePPQ[i] = -1.0;
            }
        }
        const auto& sData = sequenceData[stepInLoop];
        const auto& bData = beatSettings[stepInLoop / 4];
        int base = 60 + bData.keyRoot + ChordMatrix::MusicTheory::getDegreeInterval(bData.chordDegree);
        auto intervals = ChordMatrix::MusicTheory::getChordIntervals(bData.chordType, bData.tensionType);

        for (int i = 0; i < ChordMatrix::NumVoices; ++i) {
            if (sData.voices[i].isActive) {
                int p = juce::jlimit(0, 127, base + intervals[i] + (sData.voices[i].octaveShift * 12));
                midiMessages.addEvent(juce::MidiMessage::noteOn(1, p, sData.velocity / 127.0f), 0);
                currentNoteOnPitch[i] = p;
                currentNoteOffTimePPQ[i] = ppq + (sData.gateLength * 0.25);
            }
        }
        currentGlobalStep = stepIdx;
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
void ChordMatrixAudioProcessor::prepareToPlay(double, int) { lastPPQ = 0.0; internalPPQ = 0.0; currentGlobalStep = -1; }
void ChordMatrixAudioProcessor::releaseResources() {}
bool ChordMatrixAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const { return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo(); }
juce::AudioProcessorEditor* ChordMatrixAudioProcessor::createEditor() { return new ChordMatrixAudioProcessorEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ChordMatrixAudioProcessor(); }
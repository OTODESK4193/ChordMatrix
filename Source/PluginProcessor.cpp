#include "PluginProcessor.h"
#include "GUI/PluginEditor.h"

ChordMatrixAudioProcessor::ChordMatrixAudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    lastPPQ = 0.0;
    internalPPQ = 0.0;
    isInternalPlaying = false;
    isSyncEnabled = true;
    isPlaying = false;
    wasPlaying = false;
    currentGlobalStep = -1;
    currentBPM = 120.0f;

    for (int i = 0; i < 7; ++i) previewNotes[i].store(-1);

    for (int i = 0; i < ChordMatrix::NumVoices; ++i)
    {
        currentNoteOffTimePPQ[i] = -1.0;
        currentNoteOnPitch[i] = 0;
        phases[i] = 0.0f;
        phaseDeltas[i] = 0.0f;
    }

    for (int s = 0; s < ChordMatrix::TotalSteps; ++s)
    {
        sequenceData[s].gateLength = 0.25f;
        sequenceData[s].velocity = 100;
        sequenceData[s].keyRoot = 0;
        sequenceData[s].chordDegree = 0;
        sequenceData[s].scaleType = 0;
        sequenceData[s].inversion = 0;
        sequenceData[s].voicingMode = 0;
        sequenceData[s].shift = 0;

        for (int v = 0; v < ChordMatrix::NumVoices; ++v)
        {
            sequenceData[s].voices[v].isActive = false;
            sequenceData[s].voices[v].octaveShift = 0;
            sequenceData[s].voices[v].accidental = 0;
        }

        // ★追加: プレビュー用バッファの初期化
        previewSequenceData[s] = sequenceData[s];
    }
}

ChordMatrixAudioProcessor::~ChordMatrixAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout ChordMatrixAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{ "loopBars", 1 }, "Bars", juce::StringArray{ "1", "4", "8", "12", "16" }, 1));
    params.push_back(std::make_unique<juce::AudioParameterInt>(juce::ParameterID{ "editBar", 1 }, "Edit", 0, 15, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ "tempo", 1 }, "BPM", 20.0f, 300.0f, 120.0f));
    params.push_back(std::make_unique<juce::AudioParameterInt>(juce::ParameterID{ "timeSigNum", 1 }, "TimeSigNum", 1, 15, 4));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{ "timeSigDen", 1 }, "TimeSigDen", juce::StringArray{ "4", "8", "16" }, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{ "stepSize", 1 }, "StepSize", juce::StringArray{ "1/4", "1/8", "1/16" }, 2));

    return { params.begin(), params.end() };
}

void ChordMatrixAudioProcessor::optimizeVoicing()
{
    int loopIdx = (int)*apvts.getRawParameterValue("loopBars");
    constexpr std::array<int, 5> barsMap = { 1, 4, 8, 12, 16 };
    int numBars = barsMap[loopIdx];

    int tsNum = (int)*apvts.getRawParameterValue("timeSigNum");
    int tsDenIdx = (int)*apvts.getRawParameterValue("timeSigDen");
    int tsDen = (tsDenIdx == 0) ? 4 : (tsDenIdx == 1) ? 8 : 16;
    int stepSizeIdx = (int)*apvts.getRawParameterValue("stepSize");
    float ppqPerStep = (stepSizeIdx == 0) ? 1.0f : (stepSizeIdx == 1) ? 0.5f : 0.25f;
    float beatsPerBar = (float)tsNum * (4.0f / (float)tsDen);
    int stepsPerBar = juce::roundToInt(beatsPerBar / ppqPerStep);
    if (stepsPerBar < 1) stepsPerBar = 1;

    ChordMatrix::VoicingEngine::optimizeVoiceLeading(sequenceData, numBars * stepsPerBar);
}

void ChordMatrixAudioProcessor::prepareToPlay(double sampleRate, int)
{
    lastPPQ = 0.0;
    internalPPQ = 0.0;
    currentGlobalStep = -1;
    currentSampleRate = static_cast<float>(sampleRate);

    juce::ADSR::Parameters adsrParams{ 0.01f, 0.2f, 0.6f, 0.1f };
    for (auto& env : adsrs) {
        env.setSampleRate(sampleRate);
        env.setParameters(adsrParams);
        env.reset();
    }

    juce::ADSR::Parameters prevParams{ 0.01f, 1.5f, 0.0f, 0.1f };
    for (auto& env : previewAdsrs) {
        env.setSampleRate(sampleRate);
        env.setParameters(prevParams);
        env.reset();
    }
}

void ChordMatrixAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    buffer.clear();

    auto* playHead = getPlayHead();
    float hostBPM = 120.0f;
    double ppq = internalPPQ;

    if (playHead != nullptr && playHead->getPosition().hasValue())
    {
        const auto& pos = *playHead->getPosition();
        if (auto b = pos.getBpm()) hostBPM = (float)*b;
        if (isSyncEnabled)
        {
            if (auto p = pos.getPpqPosition()) ppq = *p;
            isPlaying = pos.getIsPlaying();
            currentBPM = hostBPM;
        }
    }

    float manualBPM = *apvts.getRawParameterValue("tempo");
    if (!isSyncEnabled)
    {
        if (isInternalPlaying)
        {
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

    if (triggerPreview.exchange(false))
    {
        juce::ADSR::Parameters prevParams{ 0.01f, 1.5f, 0.0f, 0.1f };
        for (int i = 0; i < 7; ++i)
        {
            int p = previewNotes[i].load();
            if (p >= 0)
            {
                float freq = 440.0f * std::pow(2.0f, (static_cast<float>(p) - 69.0f) / 12.0f);
                previewPhaseDeltas[i] = (freq * juce::MathConstants<float>::twoPi) / currentSampleRate;
                previewAdsrs[i].setParameters(prevParams);
                previewAdsrs[i].noteOn();
            }
            else
            {
                previewAdsrs[i].noteOff();
            }
        }
    }

    float beatsPerBar = (float)tsNum * (4.0f / (float)tsDen);
    int stepsPerBar = juce::roundToInt(beatsPerBar / ppqPerStep);
    if (stepsPerBar < 1) stepsPerBar = 1;

    int loopIdx = (int)*apvts.getRawParameterValue("loopBars");
    constexpr std::array<int, 5> barsMap = { 1, 4, 8, 12, 16 };
    int numBars = barsMap[loopIdx];
    int totalStepsInLoop = numBars * stepsPerBar;

    if ((isPlaying && ppq < lastPPQ) || (!isPlaying && wasPlaying))
    {
        for (int i = 0; i < ChordMatrix::NumVoices; ++i)
        {
            if (currentNoteOffTimePPQ[i] > 0.0)
            {
                midiMessages.addEvent(juce::MidiMessage::noteOff(1, currentNoteOnPitch[i]), 0);
                currentNoteOffTimePPQ[i] = -1.0;
            }
            adsrs[i].noteOff();
        }
        for (int i = 0; i < 7; ++i) previewAdsrs[i].noteOff();
        currentGlobalStep = -1;
    }

    wasPlaying = isPlaying;
    lastPPQ = ppq;

    for (int i = 0; i < ChordMatrix::NumVoices; ++i)
    {
        if (currentNoteOffTimePPQ[i] > 0.0 && ppq >= currentNoteOffTimePPQ[i])
        {
            midiMessages.addEvent(juce::MidiMessage::noteOff(1, currentNoteOnPitch[i]), 0);
            currentNoteOffTimePPQ[i] = -1.0;
            adsrs[i].noteOff();
        }
    }

    if (isPlaying)
    {
        int stepIdx = static_cast<int>(ppq / (double)ppqPerStep);
        int stepInLoop = stepIdx % totalStepsInLoop;

        if (stepIdx != currentGlobalStep)
        {
            // ★修正: 再生プレビュー中なら previewSequenceData を参照する（動的メモリ確保ゼロのルーティング）
            const auto& sData = isPlayingModulationPreview.load() ? previewSequenceData[stepInLoop] : sequenceData[stepInLoop];

            std::array<int, 7> vps;
            int count = ChordMatrix::VoicingEngine::getVoicedPitches(sData, vps);

            if (count > 0)
            {
                for (int i = 0; i < ChordMatrix::NumVoices; ++i)
                {
                    if (currentNoteOffTimePPQ[i] > 0.0)
                    {
                        midiMessages.addEvent(juce::MidiMessage::noteOff(1, currentNoteOnPitch[i]), 0);
                        adsrs[i].noteOff();
                        currentNoteOffTimePPQ[i] = -1.0;
                    }
                }

                float chordDurationSec = static_cast<float>(sData.gateLength) * (60.0f / currentBPM);
                float activeDecay = juce::jmax(0.05f, chordDurationSec * 0.85f);

                juce::ADSR::Parameters mainParams{ 0.01f, activeDecay, 0.0f, 0.01f };

                for (int i = 0; i < count; ++i)
                {
                    int p = juce::jlimit(0, 127, vps[i]);
                    float safeVel = juce::jlimit(0.0f, 1.0f, (float)sData.velocity / 127.0f);

                    midiMessages.addEvent(juce::MidiMessage::noteOn(1, p, safeVel), 0);
                    currentNoteOnPitch[i] = p;
                    currentNoteOffTimePPQ[i] = ppq + (double)sData.gateLength;

                    float freq = 440.0f * std::pow(2.0f, ((float)p - 69.0f) / 12.0f);
                    phaseDeltas[i] = (freq * juce::MathConstants<float>::twoPi) / currentSampleRate;

                    adsrs[i].setParameters(mainParams);
                    adsrs[i].noteOn();
                }
            }
            currentGlobalStep = stepIdx;
        }
    }

    float* left = buffer.getWritePointer(0);
    float* right = buffer.getWritePointer(1);

    for (int s = 0; s < buffer.getNumSamples(); ++s)
    {
        float mix = 0.0f;

        for (int i = 0; i < ChordMatrix::NumVoices; ++i)
        {
            if (adsrs[i].isActive())
            {
                mix += std::sin(phases[i]) * adsrs[i].getNextSample();
                phases[i] += phaseDeltas[i];
                if (phases[i] >= juce::MathConstants<float>::twoPi) phases[i] -= juce::MathConstants<float>::twoPi;
            }

            if (previewAdsrs[i].isActive())
            {
                mix += std::sin(previewPhases[i]) * previewAdsrs[i].getNextSample();
                previewPhases[i] += previewPhaseDeltas[i];
                if (previewPhases[i] >= juce::MathConstants<float>::twoPi) previewPhases[i] -= juce::MathConstants<float>::twoPi;
            }
        }

        mix *= 0.1f;
        left[s] += mix;
        if (buffer.getNumChannels() > 1) right[s] += mix;
    }
}

void ChordMatrixAudioProcessor::getStateInformation(juce::MemoryBlock& d)
{
    auto state = apvts.copyState();
    state.setProperty("seq", juce::MemoryBlock(sequenceData.data(), sizeof(sequenceData)), nullptr);
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, d);
}

void ChordMatrixAudioProcessor::setStateInformation(const void* d, int s)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(d, s));
    if (xml)
    {
        auto tree = juce::ValueTree::fromXml(*xml);
        apvts.replaceState(tree);

        if (tree.hasProperty("seq"))
        {
            auto mb = tree.getProperty("seq").getBinaryData();
            size_t bytesToCopy = juce::jmin((size_t)mb->getSize(), sizeof(sequenceData));
            memcpy(sequenceData.data(), mb->getData(), bytesToCopy);
        }
    }
}

void ChordMatrixAudioProcessor::releaseResources() {}
bool ChordMatrixAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const { return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo(); }
juce::AudioProcessorEditor* ChordMatrixAudioProcessor::createEditor() { return new ChordMatrixAudioProcessorEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ChordMatrixAudioProcessor(); }
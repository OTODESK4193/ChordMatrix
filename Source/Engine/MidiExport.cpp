#include "MidiExport.h"
#include "VoicingEngine.h"

namespace ChordMatrix
{
    void MidiExport::exportAndDrag(const std::array<StepData, TotalSteps>& sequenceData, int numBars, int stepsPerBar, float ppqPerStep, juce::DragAndDropContainer* container)
    {
        juce::MidiMessageSequence seq;
        int totalSteps = numBars * stepsPerBar;

        for (int s = 0; s < totalSteps; ++s) {
            auto& sData = sequenceData[s];
            std::array<int, 7> vps;
            int count = VoicingEngine::getVoicedPitches(sData, vps);

            if (count > 0) {
                // 警告解消：先にすべて double にキャストしてから掛け算を行う
                double startTicks = static_cast<double>(s) * static_cast<double>(ppqPerStep) * 960.0;
                double endTicks = startTicks + (static_cast<double>(sData.gateLength) * 960.0);

                for (int i = 0; i < count; ++i) {
                    int pitch = juce::jlimit(0, 127, vps[i]);
                    int vel = juce::jlimit(1, 127, (int)sData.velocity);
                    seq.addEvent(juce::MidiMessage::noteOn(1, pitch, (juce::uint8)vel), startTicks);
                    seq.addEvent(juce::MidiMessage::noteOff(1, pitch), endTicks);
                }
            }
        }
        seq.updateMatchedPairs();

        juce::MidiFile midiFile;
        midiFile.setTicksPerQuarterNote(960);
        midiFile.addTrack(seq);

        juce::File tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory);
        juce::File outputFile = tempDir.getChildFile("ChordMatrix_Export.mid");
        outputFile.deleteFile();

        juce::FileOutputStream outStream(outputFile);
        if (outStream.openedOk()) {
            midiFile.writeTo(outStream);
            outStream.flush();
        }

        if (container != nullptr) {
            juce::StringArray files;
            files.add(outputFile.getFullPathName());
            container->performExternalDragDropOfFiles(files, false);
        }
    }
}
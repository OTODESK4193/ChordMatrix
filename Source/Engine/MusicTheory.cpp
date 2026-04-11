#include "MusicTheory.h"

namespace ChordMatrix
{
    juce::StringArray MusicTheory::getDegreeNames() {
        juce::StringArray arr;
        const char* names[] = { "I", "II", "III", "IV", "V", "VI", "VII" };
        for (int i = 0; i < 7; ++i) arr.add(juce::String(names[i]));
        return arr;
    }

    juce::StringArray MusicTheory::getScaleNames() {
        juce::StringArray arr;
        const char* names[] = {
            "Major (Ionian)", "Natural Minor", "Harmonic Minor", "Melodic Minor",
            "Dorian", "Phrygian", "Lydian", "Mixolydian", "Locrian"
        };
        for (int i = 0; i < 9; ++i) arr.add(juce::String(names[i]));
        return arr;
    }

    std::array<int, 7> MusicTheory::getScaleIntervals(int scaleType) {
        int safeType = juce::jlimit(0, 8, scaleType);
        switch (safeType) {
        case 0: return { 0, 2, 4, 5, 7, 9, 11 };
        case 1: return { 0, 2, 3, 5, 7, 8, 10 };
        case 2: return { 0, 2, 3, 5, 7, 8, 11 };
        case 3: return { 0, 2, 3, 5, 7, 9, 11 };
        case 4: return { 0, 2, 3, 5, 7, 9, 10 };
        case 5: return { 0, 1, 3, 5, 7, 8, 10 };
        case 6: return { 0, 2, 4, 6, 7, 9, 11 };
        case 7: return { 0, 2, 4, 5, 7, 9, 10 };
        case 8: return { 0, 1, 3, 5, 6, 8, 10 };
        default: return { 0, 2, 4, 5, 7, 9, 11 };
        }
    }

    juce::String MusicTheory::getNoteName(int n) {
        static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        int index = n % 12;
        if (index < 0) index += 12;
        return juce::String(names[juce::jlimit(0, 11, index)]);
    }

    int MusicTheory::getBasePitch(const StepData& step, int voiceIndex) {
        auto scale = getScaleIntervals(step.scaleType);
        int baseRoot = 60 + step.keyRoot;
        int degreeIndex = juce::jlimit(0, 6, step.chordDegree);
        int scaleIndex = (degreeIndex + voiceIndex * 2) % 7;
        int octaveOffset = (degreeIndex + voiceIndex * 2) / 7;
        return baseRoot + scale[scaleIndex] + (octaveOffset * 12) + step.shift;
    }

    // ★追加: 転調ロジックの土台。Targetへ向けて直前の小節にV7(ドミナント)を自動挿入する。
    void MusicTheory::applyModulation(const std::array<StepData, TotalSteps>& source,
        std::array<StepData, TotalSteps>& dest,
        int targetBar, int targetKey, int targetScale, int method, int stepsPerBar)
    {
        // 1. まず元のシーケンスをすべてコピー（非破壊）
        dest = source;

        // TargetBarが不正な場合は処理しない（Bar1への転調は前小節がないためスキップ）
        if (targetBar <= 0 || targetBar >= MaxBars) return;

        // 2. ターゲット小節(Bar)の頭のコードを、指定されたKeyとScaleに書き換える
        int targetStepStart = targetBar * stepsPerBar;
        for (int i = 0; i < stepsPerBar; ++i) {
            dest[targetStepStart + i].keyRoot = targetKey;
            dest[targetStepStart + i].scaleType = targetScale;
        }

        // 3. 直前の小節(Bar - 1)の後半に、ターゲットに対する「V7(ドミナント)」のコードを挿入する
        int prevBar = targetBar - 1;
        int prevStepStart = prevBar * stepsPerBar;
        int halfBar = stepsPerBar / 2;

        for (int i = halfBar; i < stepsPerBar; ++i) {
            int s = prevStepStart + i;
            dest[s].keyRoot = targetKey;
            dest[s].scaleType = 0;   // V7をシンプルに表現するためメジャースケール基準に設定
            dest[s].chordDegree = 4; // V (ドミナント)

            for (int v = 0; v < 7; ++v) {
                // 7thコードを鳴らすため、Root(6), 3rd(4), 5th(2), 7th(0) をアクティブにする
                dest[s].voices[v].isActive = (v == 6 || v == 4 || v == 2 || v == 0);
                dest[s].voices[v].octaveShift = 0;
                dest[s].voices[v].accidental = 0;
            }
            dest[s].voicingMode = 0; // Close Voicing
            dest[s].gateLength = 0.25f; // 安全なゲート長をセット
        }
    }
}
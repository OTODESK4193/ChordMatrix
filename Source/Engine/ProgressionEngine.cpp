#include "ProgressionEngine.h"

namespace ChordMatrix
{
    void ProgressionEngine::applyModulation(const std::array<StepData, TotalSteps>& source,
        std::array<StepData, TotalSteps>& dest,
        int targetBar, int targetKey, int targetScale, int method,
        int stepsPerBar, int stepsPerBeat, float ppqPerStep)
    {
        dest = source;
        if (targetBar <= 0 || targetBar >= MaxBars) return;

        float chordGate = static_cast<float>(stepsPerBeat) * ppqPerStep; // ぴったり1拍分

        // ターゲット小節の頭に解決先のトニック(I)を配置
        int targetStepStart = targetBar * stepsPerBar;
        dest[targetStepStart].keyRoot = targetKey;
        dest[targetStepStart].scaleType = targetScale;
        dest[targetStepStart].chordDegree = 0;
        for (int v = 0; v < 7; ++v) {
            dest[targetStepStart].voices[v].isActive = (v == 0 || v == 1 || v == 2 || v == 3);
        }
        dest[targetStepStart].voicingMode = 0;
        dest[targetStepStart].gateLength = chordGate * 2.0f; // 解決先は2拍分伸ばす

        int prevBar = targetBar - 1;
        int prevStepStart = prevBar * stepsPerBar;
        int beat2FromEnd = stepsPerBar - 2 * stepsPerBeat;
        int beat1FromEnd = stepsPerBar - stepsPerBeat;

        // =========================================================================
        // ★修正の核心: 前半の既存コードが転調エリアに侵食しないように長さをカットする
        // =========================================================================
        float modulationStartPPQ = static_cast<float>(beat2FromEnd) * ppqPerStep;
        for (int i = 0; i < beat2FromEnd; ++i) {
            float currentStepPPQ = static_cast<float>(i) * ppqPerStep;
            float maxAllowedGate = modulationStartPPQ - currentStepPPQ;
            if (dest[prevStepStart + i].gateLength > maxAllowedGate) {
                dest[prevStepStart + i].gateLength = maxAllowedGate; // 強制的にカット
            }
        }

        // 書き込むエリア（最後の2拍分）のノートを一旦クリア
        for (int i = beat2FromEnd; i < stepsPerBar; ++i) {
            for (int v = 0; v < 7; ++v) dest[prevStepStart + i].voices[v].isActive = false;
        }

        int s_ii = prevStepStart + beat2FromEnd;
        int s_V = prevStepStart + beat1FromEnd;

        // アプローチコードの挿入
        if (method == TwoFiveOne && beat2FromEnd >= 0) {
            // ii コード (3拍目)
            dest[s_ii].keyRoot = targetKey;
            dest[s_ii].scaleType = targetScale;
            dest[s_ii].chordDegree = 1;         // II
            for (int v = 0; v < 7; ++v) dest[s_ii].voices[v].isActive = (v == 0 || v == 1 || v == 2 || v == 3);
            dest[s_ii].voicingMode = 0;
            dest[s_ii].gateLength = chordGate;

            // V コード (4拍目)
            dest[s_V].keyRoot = targetKey;
            dest[s_V].scaleType = 0;
            dest[s_V].chordDegree = 4;          // V
            for (int v = 0; v < 7; ++v) dest[s_V].voices[v].isActive = (v == 0 || v == 1 || v == 2 || v == 3);
            dest[s_V].voicingMode = 0;
            dest[s_V].gateLength = chordGate;
        }
        else if (method == TritoneSub && beat2FromEnd >= 0) {
            // 裏コード (3拍目〜4拍目)
            int subVKey = (targetKey + 1) % 12; // ターゲットの半音上
            dest[s_ii].keyRoot = subVKey;
            dest[s_ii].scaleType = 7;   // Mixolydian -> Dom7
            dest[s_ii].chordDegree = 0; // I
            for (int v = 0; v < 7; ++v) dest[s_ii].voices[v].isActive = (v == 0 || v == 1 || v == 2 || v == 3);
            dest[s_ii].voicingMode = 0;
            dest[s_ii].gateLength = chordGate * 2.0f;
        }
        else {
            // ダイレクトドミナント (3拍目〜4拍目)
            dest[s_ii].keyRoot = targetKey;
            dest[s_ii].scaleType = 0;
            dest[s_ii].chordDegree = 4;
            for (int v = 0; v < 7; ++v) dest[s_ii].voices[v].isActive = (v == 0 || v == 1 || v == 2 || v == 3);
            dest[s_ii].voicingMode = 0;
            dest[s_ii].gateLength = chordGate * 2.0f;
        }
    }
}
#include "ProgressionEngine.h"

namespace ChordMatrix
{
    void ProgressionEngine::applyModulation(const std::array<StepData, TotalSteps>& source,
        std::array<StepData, TotalSteps>& dest,
        int targetBar, int targetKey, int targetScale, int method, int stepsPerBar)
    {
        // 1. まず元のシーケンスをすべてコピー（非破壊）
        dest = source;

        // TargetBarが不正な場合は処理しない（Bar 1への転調は前小節がないためスキップ）
        if (targetBar <= 0 || targetBar >= MaxBars) return;

        // 2. ターゲット小節(Bar)の頭のコードを、指定されたKeyとScaleに書き換える
        int targetStepStart = targetBar * stepsPerBar;
        for (int i = 0; i < stepsPerBar; ++i) {
            dest[targetStepStart + i].keyRoot = targetKey;
            dest[targetStepStart + i].scaleType = targetScale;
        }

        // 3. 直前の小節(Bar - 1)の後半に、アプローチコードを挿入する
        int prevBar = targetBar - 1;
        int prevStepStart = prevBar * stepsPerBar;
        int halfBar = stepsPerBar / 2;
        int quarterBar = stepsPerBar / 4;

        if (method == TwoFiveOne && quarterBar > 0) {
            // ---------------------------------------------------------
            // パターンB: ツー・ファイブ・ワン (ii - V - I)
            // ---------------------------------------------------------
            for (int i = halfBar; i < halfBar + quarterBar; ++i) {
                int s = prevStepStart + i;
                dest[s].keyRoot = targetKey;
                dest[s].scaleType = targetScale;
                dest[s].chordDegree = 1;         // 度数: II

                for (int v = 0; v < 7; ++v) {
                    dest[s].voices[v].isActive = (v == 6 || v == 4 || v == 2 || v == 0);
                    dest[s].voices[v].octaveShift = 0;
                    dest[s].voices[v].accidental = 0;
                }
                dest[s].voicingMode = 0;
                dest[s].gateLength = 0.25f;
            }

            for (int i = halfBar + quarterBar; i < stepsPerBar; ++i) {
                int s = prevStepStart + i;
                dest[s].keyRoot = targetKey;
                dest[s].scaleType = 0;
                dest[s].chordDegree = 4; // 度数: V

                for (int v = 0; v < 7; ++v) {
                    dest[s].voices[v].isActive = (v == 6 || v == 4 || v == 2 || v == 0);
                    dest[s].voices[v].octaveShift = 0;
                    dest[s].voices[v].accidental = 0;
                }
                dest[s].voicingMode = 0;
                dest[s].gateLength = 0.25f;
            }
        }
        else if (method == TritoneSub) {
            // ---------------------------------------------------------
            // パターンC: 裏コード (SubV7 - I)
            // ターゲットキーの半音上（トライトーン先のV7）を生成する
            // (targetKey + 6) % 12 をキーとするV7コードは、Targetの半音上のドミナント7thになる。
            // ---------------------------------------------------------
            int subVKey = (targetKey + 6) % 12;

            for (int i = halfBar; i < stepsPerBar; ++i) {
                int s = prevStepStart + i;
                dest[s].keyRoot = subVKey;
                dest[s].scaleType = 0;   // Major
                dest[s].chordDegree = 4; // V

                for (int v = 0; v < 7; ++v) {
                    dest[s].voices[v].isActive = (v == 6 || v == 4 || v == 2 || v == 0);
                    dest[s].voices[v].octaveShift = 0;
                    dest[s].voices[v].accidental = 0;
                }
                dest[s].voicingMode = 0;
                dest[s].gateLength = 0.25f;
            }
        }
        else {
            // ---------------------------------------------------------
            // パターンA: ダイレクト・ドミナント (V - I)
            // ---------------------------------------------------------
            for (int i = halfBar; i < stepsPerBar; ++i) {
                int s = prevStepStart + i;
                dest[s].keyRoot = targetKey;
                dest[s].scaleType = 0;
                dest[s].chordDegree = 4;

                for (int v = 0; v < 7; ++v) {
                    dest[s].voices[v].isActive = (v == 6 || v == 4 || v == 2 || v == 0);
                    dest[s].voices[v].octaveShift = 0;
                    dest[s].voices[v].accidental = 0;
                }
                dest[s].voicingMode = 0;
                dest[s].gateLength = 0.25f;
            }
        }
    }
}
#include "ProgressionEngine.h"

namespace ChordMatrix
{
    void ProgressionEngine::applyModulation(const std::array<StepData, TotalSteps>& source,
        std::array<StepData, TotalSteps>& dest,
        int targetBar, int targetKey, int targetScale, int method,
        int stepsPerBar, int stepsPerBeat, float ppqPerStep)
    {
        dest = source;
        if (targetBar <= 0 || targetBar >= 16) return;

        float chordGate = static_cast<float>(stepsPerBeat) * ppqPerStep;

        // ターゲット小節の頭に解決先の和音を配置
        int targetStepStart = targetBar * stepsPerBar;
        dest[targetStepStart].keyRoot = targetKey;
        dest[targetStepStart].scaleType = targetScale;
        dest[targetStepStart].chordDegree = 0;
        for (int v = 0; v < 7; ++v) {
            dest[targetStepStart].voices[v].isActive = (v == 0 || v == 1 || v == 2 || v == 3);
        }
        dest[targetStepStart].voicingMode = 0;
        dest[targetStepStart].gateLength = chordGate * 2.0f;

        int prevBar = targetBar - 1;
        int prevStepStart = prevBar * stepsPerBar;
        int beat2FromEnd = stepsPerBar - 2 * stepsPerBeat;
        int beat1FromEnd = stepsPerBar - stepsPerBeat;

        // 手前のコードがアプローチエリアに侵食しないようゲート長をクリップ
        float modulationStartPPQ = static_cast<float>(beat2FromEnd) * ppqPerStep;
        for (int i = 0; i < beat2FromEnd; ++i) {
            float currentStepPPQ = static_cast<float>(i) * ppqPerStep;
            float maxAllowedGate = modulationStartPPQ - currentStepPPQ;
            if (dest[prevStepStart + i].gateLength > maxAllowedGate) {
                dest[prevStepStart + i].gateLength = maxAllowedGate;
            }
        }

        // アプローチエリアの初期化
        for (int i = beat2FromEnd; i < stepsPerBar; ++i) {
            for (int v = 0; v < 7; ++v) dest[prevStepStart + i].voices[v].isActive = false;
        }

        int s_ii = prevStepStart + beat2FromEnd;
        int s_V = prevStepStart + beat1FromEnd;

        // 音楽理論に基づく転調アプローチの動的生成
        if (method == TwoFiveOne && beat2FromEnd >= 0) {
            // Major ii-V-I
            dest[s_ii].keyRoot = targetKey; dest[s_ii].scaleType = targetScale; dest[s_ii].chordDegree = 1;
            for (int v = 0; v < 7; ++v) dest[s_ii].voices[v].isActive = (v == 0 || v == 1 || v == 2 || v == 3);
            dest[s_ii].voicingMode = 4; dest[s_ii].gateLength = chordGate; // Rootless推奨

            dest[s_V].keyRoot = targetKey; dest[s_V].scaleType = 0; dest[s_V].chordDegree = 4;
            for (int v = 0; v < 7; ++v) dest[s_V].voices[v].isActive = (v == 0 || v == 1 || v == 2 || v == 3);
            dest[s_V].voicingMode = 4; dest[s_V].gateLength = chordGate;
        }
        else if (method == TritoneSub && beat2FromEnd >= 0) {
            // SubV7 (裏コード)
            int subVKey = (targetKey + 1) % 12;
            dest[s_ii].keyRoot = subVKey; dest[s_ii].scaleType = 7; dest[s_ii].chordDegree = 0;
            for (int v = 0; v < 7; ++v) dest[s_ii].voices[v].isActive = (v == 0 || v == 1 || v == 2 || v == 3);
            dest[s_ii].voicingMode = 0; dest[s_ii].gateLength = chordGate * 2.0f;
        }
        else if (method == MinorTwoFive && beat2FromEnd >= 0) {
            // Minor ii-V-i (m7b5 -> 7alt)
            dest[s_ii].keyRoot = targetKey; dest[s_ii].scaleType = 1; dest[s_ii].chordDegree = 1; // Natural Minorのiiはm7b5
            for (int v = 0; v < 7; ++v) dest[s_ii].voices[v].isActive = (v == 0 || v == 1 || v == 2 || v == 3);
            dest[s_ii].voicingMode = 0; dest[s_ii].gateLength = chordGate;

            dest[s_V].keyRoot = targetKey; dest[s_V].scaleType = 1; dest[s_V].chordDegree = 4; // v(m7)
            dest[s_V].voices[0].isActive = true;
            dest[s_V].voices[1].isActive = true; dest[s_V].voices[1].accidental = 1; // 3rdを上げてドミナント化 (V7)
            dest[s_V].voices[2].isActive = true; // 5th
            dest[s_V].voices[3].isActive = true; // 7th
            dest[s_V].voicingMode = 0; dest[s_V].gateLength = chordGate;
        }
        else if (method == Backdoor && beat2FromEnd >= 0) {
            // Backdoor Progression (IVm7 -> bVII7)
            dest[s_ii].keyRoot = targetKey; dest[s_ii].scaleType = 1; dest[s_ii].chordDegree = 3; // Minorのiv
            for (int v = 0; v < 7; ++v) dest[s_ii].voices[v].isActive = (v == 0 || v == 1 || v == 2 || v == 3);
            dest[s_ii].voicingMode = 4; dest[s_ii].gateLength = chordGate;

            dest[s_V].keyRoot = targetKey; dest[s_V].scaleType = 1; dest[s_V].chordDegree = 6; // MinorのbVII
            for (int v = 0; v < 7; ++v) dest[s_V].voices[v].isActive = (v == 0 || v == 1 || v == 2 || v == 3);
            dest[s_V].voicingMode = 4; dest[s_V].gateLength = chordGate;
        }
        else {
            // Direct Dominant
            dest[s_ii].keyRoot = targetKey; dest[s_ii].scaleType = 0; dest[s_ii].chordDegree = 4;
            for (int v = 0; v < 7; ++v) dest[s_ii].voices[v].isActive = (v == 0 || v == 1 || v == 2 || v == 3);
            dest[s_ii].voicingMode = 0; dest[s_ii].gateLength = chordGate * 2.0f;
        }
    }

    const std::vector<ProgressionPreset>& ProgressionEngine::getProgressionDictionary() {
        static std::vector<ProgressionPreset> dict;
        if (dict.empty()) {

            // ==============================================================================
            // 1. J-Pop / Anime Standards (機能和声の王道)
            // ==============================================================================
            dict.push_back({ "J-Pop / Anime Standards", "Royal Road (IV-V-iii-vi)", 4, {
                {0, 4, 3, 0, 0,0,0,0}, {4, 4, 4, 0, 0,0,0,0}, {8, 4, 2, 0, 0,0,0,0}, {12, 4, 5, 0, 0,0,0,0}
            }, "IVmaj7 - V7 - iiim7 - vim7" });

            dict.push_back({ "J-Pop / Anime Standards", "TK Progression (vi-IV-V-I)", 4, {
                {0, 4, 5, 0, 0,0,0,0}, {4, 4, 3, 0, 0,0,0,0}, {8, 4, 4, 0, 0,0,0,0}, {12, 4, 0, 0, 0,0,0,0}
            }, "vim7 - IVmaj7 - V7 - Imaj7" });

            dict.push_back({ "J-Pop / Anime Standards", "Anime Standard (vi-V-IV-V)", 4, {
                {0, 4, 5, 0, 0,0,0,0}, {4, 4, 4, 0, 0,0,0,0}, {8, 4, 3, 0, 0,0,0,0}, {12, 4, 4, 0, 0,0,0,0}
            }, "vim7 - V7 - IVmaj7 - V7" });

            // ==============================================================================
            // 2. R&B / Neo-Soul / Jazz (テンションとツーファイブ)
            // ==============================================================================
            dict.push_back({ "R&B / Neo-Soul / Jazz", "Just The Two Of Us (IV-III-vi-I)", 4, {
                // III7 は diatonic iii (m7) の3rdを半音上げる(acc3rd=1)
                // I7 は diatonic I (M7) の7thを半音下げる(acc7th=-1)
                {0, 4, 3, 4, 0,0,0,0}, {4, 4, 2, 4, 0,1,0,0}, {8, 4, 5, 4, 0,0,0,0}, {12, 4, 0, 4, 0,0,0,-1}
            }, "IVmaj9 - III7 - vim9 - I13 (Rootless)" });

            dict.push_back({ "R&B / Neo-Soul / Jazz", "Minor 2-5-1 (Half-Diminished)", 4, {
                // Major Keyにおける ii m7b5 は 5thを半音下げる
                {0, 4, 1, 0, 0,0,-1,0}, {4, 4, 4, 0, 0,0,0,0}, {8, 4, 5, 0, 0,0,0,0}, {12, 4, 5, 0, 0,0,0,0}
            }, "iim7b5 - V7 - vim7 - vim7" });

            dict.push_back({ "R&B / Neo-Soul / Jazz", "Autumn Leaves (Circle of 5ths)", 8, {
                {0, 4, 1, 4, 0,0,0,0}, {4, 4, 4, 4, 0,0,0,0}, {8, 4, 0, 4, 0,0,0,0}, {12, 4, 3, 4, 0,0,0,0},
                {16, 4, 6, 0, 0,0,-1,0}, {20, 4, 2, 4, 0,1,0,0}, {24, 4, 5, 4, 0,0,0,0}, {28, 4, 5, 4, 0,0,0,0}
            }, "iim7 - V7 - Imaj7 - IVmaj7 - viim7b5 - III7 - vim7" });

            dict.push_back({ "R&B / Neo-Soul / Jazz", "Rhythm Changes (I-VI-II-V)", 4, {
                // VI7, II7 はセカンダリードミナント(3rdを+1)
                {0, 2, 0, 4, 0,0,0,0}, {2, 2, 5, 4, 0,1,0,0}, {4, 2, 1, 4, 0,1,0,0}, {6, 2, 4, 4, 0,0,0,0},
                {8, 4, 0, 4, 0,0,0,0}, {12, 4, 0, 4, 0,0,0,0}
            }, "Imaj7 - VI7 - II7 - V7 - Imaj7" });

            // ==============================================================================
            // 3. Emotional / Modern (モーダルインターチェンジと代理和音)
            // ==============================================================================
            dict.push_back({ "Emotional / Modern", "Subdominant Minor (IV-iv-I)", 4, {
                // ivm7 は IVmaj7 の 3rd と 7th を半音下げる
                {0, 4, 3, 4, 0,0,0,0}, {4, 4, 3, 4, 0,-1,0,-1}, {8, 4, 0, 4, 0,0,0,0}, {12, 4, 0, 4, 0,0,0,0}
            }, "IVmaj7 - ivm7 - Imaj7 - Imaj7" });

            dict.push_back({ "Emotional / Modern", "Descending 4-3-2-1", 4, {
                {0, 4, 3, 5, 0,0,0,0}, {4, 4, 2, 4, 0,0,0,0}, {8, 4, 1, 4, 0,0,0,0}, {12, 4, 0, 4, 0,0,0,0}
            }, "IVmaj7 - iiim7 - iim7 - Imaj7" });

            dict.push_back({ "Emotional / Modern", "Mario Cadence (bVI-bVII-I)", 4, {
                // bVImaj7: 6度ルートを半音下げ、M3, P5, M7を構築
                {0, 4, 5, 0, -1,0,-1,-1}, {4, 4, 6, 0, -1,0,-1,-1}, {8, 8, 0, 0, 0,0,0,0}, {12, 8, 0, 0, 0,0,0,0}
            }, "bVImaj7 - bVIImaj7 - Imaj7" });

            // ==============================================================================
            // 4. Advanced / Theory (高度な理論的アプローチ)
            // ==============================================================================
            dict.push_back({ "Advanced / Theory", "Line Cliche (Minor Descending)", 4, {
                // vim - vim(maj7) - vim7 - vim6
                {0, 4, 5, 0, 0,0,0,-128}, {4, 4, 5, 0, 0,0,0,1}, {8, 4, 5, 0, 0,0,0,0}, {12, 4, 5, 0, 0,0,0,-1}
            }, "vim - vim(maj7) - vim7 - vim6" });

            dict.push_back({ "Advanced / Theory", "Coltrane Changes (Giant Steps)", 8, {
                // 長3度分割による3トニックシステム (I -> bIII7 -> bVI -> VII7 -> III)
                // 複雑なため、臨時記号で強引にピッチクラスを合わせる
                {0, 4, 0, 0, 0,0,0,0},           // Imaj7
                {4, 4, 2, 0, -1,0,-1,-1},        // bIII7
                {8, 4, 5, 0, -1,0,-1,-1},        // bVImaj7
                {12, 4, 6, 0, -1,0,-1,-1},       // VII7
                {16, 4, 2, 0, 0,1,0,0},          // IIImaj7
                {20, 4, 4, 0, 0,0,0,0},          // V7
                {24, 8, 0, 0, 0,0,0,0}           // Imaj7
            }, "I - bIII7 - bVI - VII7 - III - V7 - I" });

            // ==============================================================================
            // 5. Classic / Standard (トライアド主体の進行)
            // ==============================================================================
            dict.push_back({ "Classic / Standard", "Canon Progression", 4, {
                {0, 2, 0, 3, 0,0,0,-128}, {2, 2, 4, 3, 0,0,0,-128}, {4, 2, 5, 3, 0,0,0,-128}, {6, 2, 2, 3, 0,0,0,-128},
                {8, 2, 3, 3, 0,0,0,-128}, {10, 2, 0, 3, 0,0,0,-128}, {12, 2, 3, 3, 0,0,0,-128}, {14, 2, 4, 3, 0,0,0,-128}
            }, "I - V - vim - iiim - IV - I - IV - V" });

            dict.push_back({ "Classic / Standard", "Doo-wop (1-6-4-5)", 4, {
                {0, 4, 0, 3, 0,0,0,-128}, {4, 4, 5, 3, 0,0,0,-128}, {8, 4, 3, 3, 0,0,0,-128}, {12, 4, 4, 3, 0,0,0,-128}
            }, "I - vim - IV - V" });

            dict.push_back({ "Classic / Standard", "Let It Be (1-5-6-4)", 4, {
                {0, 4, 0, 3, 0,0,0,-128}, {4, 4, 4, 3, 0,0,0,-128}, {8, 4, 5, 3, 0,0,0,-128}, {12, 4, 3, 3, 0,0,0,-128}
            }, "I - V - vim - IV" });
        }
        return dict;
    }
}
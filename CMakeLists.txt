cmake_minimum_required(VERSION 3.22)
project(ChordMatrix VERSION 1.0.0)

# =========================================================================
# 1. 干渉排除設定
# =========================================================================
set(JUCE_VST3_AUTO_MANIFEST OFF)
add_compile_definitions(JUCE_VST3_CAN_REPLACE_VST2=0)
add_compile_definitions(JUCE_USE_WINDOWS_MEDIA_FORMAT=1)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =========================================================================
# 2. JUCEディレクトリの指定
# =========================================================================
set(JUCE_DIR "C:/JUCE")
add_subdirectory(${JUCE_DIR} build_chordmatrix_juce)

# =========================================================================
# 3. プラグインの定義
# =========================================================================
juce_add_plugin(ChordMatrix
    COMPANY_NAME "OTODESK"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE 
    VST3_AUTO_MANIFEST FALSE
    PLUGIN_MANUFACTURER_CODE "Otds"
    PLUGIN_CODE "Chmx"
    FORMATS VST3 Standalone
    PRODUCT_NAME "ChordMatrix"
    CHANNEL_CONFIGS 0,2
)

target_compile_options(ChordMatrix PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:
        $<$<CONFIG:Release>:/O2 /fp:fast> /arch:AVX2 /wd4819 /utf-8
    >
)

# =========================================================================
# 4. ソースコードの指定 (★修正：PluginEditorのパスをGUIフォルダ内に変更)
# =========================================================================
target_sources(ChordMatrix PRIVATE
    # Processor (Backend)
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    
    # Data
    Source/Data/StepData.h
    
    # Engine
    Source/Engine/MusicTheory.cpp
    Source/Engine/MusicTheory.h
    Source/Engine/VoicingEngine.cpp
    Source/Engine/VoicingEngine.h
    Source/Engine/MidiExport.cpp
    Source/Engine/MidiExport.h
    
    # GUI
    Source/GUI/PluginEditor.cpp      # ← 修正箇所
    Source/GUI/PluginEditor.h        # ← 修正箇所
    Source/GUI/HeaderComponent.cpp
    Source/GUI/HeaderComponent.h
    Source/GUI/InspectorComponent.cpp
    Source/GUI/InspectorComponent.h
    Source/GUI/MatrixGridComponent.cpp
    Source/GUI/MatrixGridComponent.h
)

# =========================================================================
# 5. インクルードパスの設定
# =========================================================================
# これにより #include "Data/StepData.h" などが正しく見つかるようになります
target_include_directories(ChordMatrix PRIVATE Source)

# =========================================================================
# 6. 必要なJUCEモジュールのリンク
# =========================================================================
target_link_libraries(ChordMatrix PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_dsp
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_core         
    juce::juce_events       
)
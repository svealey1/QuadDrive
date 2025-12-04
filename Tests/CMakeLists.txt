# Null Test Executable for STEVE

# Add executable
add_executable(NullTest
    NullTest.cpp
    ../Source/PluginProcessor.cpp
    ../Source/PluginEditor.cpp
    ../Source/OversamplingManager.h
)

# Include directories
target_include_directories(NullTest PRIVATE
    ${CMAKE_SOURCE_DIR}/Source
    ${CMAKE_SOURCE_DIR}/JUCE/modules
)

# Link against JUCE modules
target_link_libraries(NullTest PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_dsp
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
)

# Compiler definitions (include JUCE plugin macros)
target_compile_definitions(NullTest PRIVATE
    JUCE_STANDALONE_APPLICATION=1
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0
    JucePlugin_Name="STEVE"
    JucePlugin_Desc="STEVE"
    JucePlugin_Manufacturer="Steve Vealey"
    JucePlugin_ManufacturerCode=0x53765679  # 'SvVy'
    JucePlugin_PluginCode=0x53747665        # 'Stve'
    JucePlugin_IsSynth=0
    JucePlugin_WantsMidiInput=0
    JucePlugin_ProducesMidiOutput=0
    JucePlugin_IsMidiEffect=0
    JucePlugin_EditorRequiresKeyboardFocus=0
    JucePlugin_Version=1.8.5
    JucePlugin_VersionCode=0x010805
    JucePlugin_VersionString="1.8.5"
)

# Set C++ standard
set_target_properties(NullTest PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
)

juce_add_plugin(STEVE
    COMPANY_NAME "Steve Vealey"
    BUNDLE_ID com.SteveVealey.STEVE
    PLUGIN_MANUFACTURER_CODE SvVy
    PLUGIN_CODE Stve
    FORMATS VST3 AU
    PRODUCT_NAME "STEVE"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
)

# Add source files
target_sources(STEVE
    PRIVATE
        PluginProcessor.cpp
        PluginEditor.cpp
        TransferCurveMeter.cpp
        StereoMeter.cpp
        ThresholdMeter.cpp
        WaveformGRMeter.cpp
        STEVEScope.cpp
)

# Link JUCE modules
target_link_libraries(STEVE
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Compile definitions
target_compile_definitions(STEVE
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

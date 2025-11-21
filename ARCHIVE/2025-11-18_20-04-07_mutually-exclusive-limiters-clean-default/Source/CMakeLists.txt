juce_add_plugin(QuadBlendDrive
    COMPANY_NAME "YourStudioName"
    PLUGIN_MANUFACTURER_CODE Ystn
    PLUGIN_CODE Qbdr
    FORMATS VST3 AU
    PRODUCT_NAME "Quad-Blend Drive"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
)

# Add source files
target_sources(QuadBlendDrive
    PRIVATE
        PluginProcessor.cpp
        PluginEditor.cpp
)

# Link JUCE modules
target_link_libraries(QuadBlendDrive
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Compile definitions
target_compile_definitions(QuadBlendDrive
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

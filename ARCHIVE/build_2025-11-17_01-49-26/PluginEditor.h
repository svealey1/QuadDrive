#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include "PluginProcessor.h"

// Gain Reduction Meter Component
class GainReductionMeter : public juce::Component, private juce::Timer
{
public:
    GainReductionMeter(QuadBlendDriveAudioProcessor& p);

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;

    QuadBlendDriveAudioProcessor& processor;
    static constexpr int historySize = 90;  // 3 seconds at 30fps (3s * 30fps = 90 samples)
    std::array<float, historySize> grHistory;
    int historyIndex = 0;
};

// Oscilloscope Component
class Oscilloscope : public juce::Component, private juce::Timer
{
public:
    Oscilloscope(QuadBlendDriveAudioProcessor& p);

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;

    QuadBlendDriveAudioProcessor& processor;
};

// Custom XY Pad Component
class XYPad : public juce::Component
{
public:
    XYPad(juce::AudioProcessorValueTreeState& apvts,
          const juce::String& xParamID,
          const juce::String& yParamID);

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void resized() override;

private:
    void updateParameters(juce::Point<float> position);

    juce::AudioProcessorValueTreeState& apvts;
    juce::String xParamID, yParamID;

    juce::Slider xSlider, ySlider;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> xAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> yAttachment;
};

class QuadBlendDriveAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    QuadBlendDriveAudioProcessorEditor(QuadBlendDriveAudioProcessor&);
    ~QuadBlendDriveAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    QuadBlendDriveAudioProcessor& audioProcessor;

    // XY Pad
    XYPad xyPad;

    // Visualizations
    GainReductionMeter grMeter;
    Oscilloscope oscilloscope;

    // Global Sliders
    juce::Slider inputGainSlider, outputGainSlider, mixSlider;

    // Normalization/Threshold Control
    juce::Slider calibLevelSlider;
    juce::TextButton analyzeButton, normalizeButton, resetNormButton;
    juce::Label inputPeakLabel;
    juce::Label gainAppliedLabel;  // "GAIN APPLIED:" label
    juce::TextEditor gainAppliedValue;  // Copyable value only
    juce::Label calibLevelLabel;

    // Gain Linking Control
    juce::ToggleButton gainLinkButton;

    // Bypass Control
    juce::ToggleButton bypassButton;

    // Master Gain Compensation Control
    juce::ToggleButton masterCompButton;

    // Per-Type Trim Sliders
    juce::Slider hcTrimSlider, scTrimSlider, slTrimSlider, flTrimSlider;
    juce::TextButton resetTrimsButton;

    // Delta Mode Controls
    juce::ToggleButton deltaModeButton;

    // Protection Limiter Controls
    juce::ToggleButton protectionEnableButton;
    juce::Slider protectionCeilingSlider;
    juce::Label protectionCeilingLabel;

    // Toggle Buttons for Muting processors
    juce::ToggleButton hcMuteButton, scMuteButton, slMuteButton, flMuteButton;

    // Labels
    juce::Label inputGainLabel, outputGainLabel, mixLabel;
    juce::Label hcTrimLabel, scTrimLabel, slTrimLabel, flTrimLabel;

    // Preset buttons (A, B, C, D)
    juce::TextButton presetButtonA, presetButtonB, presetButtonC, presetButtonD;
    juce::TextButton saveButtonA, saveButtonB, saveButtonC, saveButtonD;

    // Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> inputGainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputGainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> calibLevelAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> hcTrimAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> scTrimAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> slTrimAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> flTrimAttachment;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> hcMuteAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> scMuteAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> slMuteAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> flMuteAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> deltaModeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> masterCompAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> protectionEnableAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> protectionCeilingAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(QuadBlendDriveAudioProcessorEditor)
};

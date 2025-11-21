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
    static constexpr int historySize = 200;
    std::array<float, historySize> grHistory;
    int historyIndex = 0;
};

// Transfer Curve Display Component
class TransferCurveDisplay : public juce::Component, private juce::Timer
{
public:
    TransferCurveDisplay(juce::AudioProcessorValueTreeState& apvts);

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;
    float calculateTransferFunction(float input);

    juce::AudioProcessorValueTreeState& apvts;
    float currentX = 0.5f;
    float currentY = 0.5f;
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

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> xAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> yAttachment;

    juce::Slider xSlider, ySlider;
};

class QuadBlendDriveAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    QuadBlendDriveAudioProcessorEditor(QuadBlendDriveAudioProcessor&);
    ~QuadBlendDriveAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    QuadBlendDriveAudioProcessor& audioProcessor;

    // XY Pad
    XYPad xyPad;

    // Visualizations
    GainReductionMeter grMeter;
    TransferCurveDisplay transferCurve;

    // Sliders
    juce::Slider inputGainSlider, outputGainSlider, mixSlider;
    juce::Slider clipThreshSlider, limitReleaseSlider;
    juce::Slider hcTrimSlider, scTrimSlider, slTrimSlider, flTrimSlider;

    // ComboBox
    juce::ComboBox oversampleCombo;

    // Toggle Buttons
    juce::ToggleButton hcCompButton, scCompButton, slCompButton, flCompButton;
    juce::ToggleButton deltaModeButton;

    // Labels
    juce::Label inputGainLabel, outputGainLabel, mixLabel;
    juce::Label clipThreshLabel, limitReleaseLabel, oversampleLabel;
    juce::Label hcTrimLabel, scTrimLabel, slTrimLabel, flTrimLabel;

    // Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> inputGainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputGainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> clipThreshAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> limitReleaseAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> hcTrimAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> scTrimAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> slTrimAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> flTrimAttachment;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> oversampleAttachment;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> hcCompAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> scCompAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> slCompAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> flCompAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> deltaModeAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(QuadBlendDriveAudioProcessorEditor)
};

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include "PluginProcessor.h"
#include "TransferCurveMeter.h"
#include "StereoMeter.h"
#include "STEVELookAndFeel.h"

// Master Meter Component
class MasterMeter : public juce::Component, private juce::Timer
{
public:
    MasterMeter(QuadBlendDriveAudioProcessor& p);

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;

    QuadBlendDriveAudioProcessor& processor;
    float peakL{0.0f};
    float peakR{0.0f};
    float peakHoldL{0.0f};
    float peakHoldR{0.0f};
    juce::int64 peakHoldTimeL{0};
    juce::int64 peakHoldTimeR{0};
    static constexpr juce::int64 peakHoldDurationMs = 3000;  // 3 seconds
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
    void paintGRTrace(juce::Graphics& g, juce::Rectangle<float> graphBounds, float scale);

    QuadBlendDriveAudioProcessor& processor;
    static constexpr int displaySize = 90;  // 3 seconds at 30fps (matches GR meter)
    int displayCursorPos = 0;
};

// Custom XY Pad Component
class XYPad : public juce::Component
{
public:
    XYPad(juce::AudioProcessorValueTreeState& apvts,
          const juce::String& xParamID,
          const juce::String& yParamID,
          QuadBlendDriveAudioProcessor& processor);

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void resized() override;

private:
    void updateParameters(juce::Point<float> position);

    juce::AudioProcessorValueTreeState& apvts;
    juce::String xParamID, yParamID;
    QuadBlendDriveAudioProcessor& processor;

    bool wasCommandPressed = false;
    bool deltaModeWasOn = false;
    bool gainCompWasOn = false;

    juce::Slider xSlider, ySlider;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> xAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> yAttachment;
};

// Calibration Panel (Collapsible)
class CalibrationPanel : public juce::Component
{
public:
    CalibrationPanel(juce::AudioProcessorValueTreeState& apvts, QuadBlendDriveAudioProcessor& processor);

    void paint(juce::Graphics& g) override;
    void resized() override;

    void setVisible(bool shouldBeVisible) override;
    bool isOpen() const { return getLocalBounds().getHeight() > 0; }

    // Public methods for updating display values
    void updateInputPeakDisplay(double peakDB);
    void updateGainAppliedDisplay(double gainDB);

    // Check if normalization is currently engaged
    bool isNormalizationEnabled() const { return normalizeButton.getToggleState(); }

private:
    juce::AudioProcessorValueTreeState& apvts;
    QuadBlendDriveAudioProcessor& processor;

    // Normalization/Threshold Control
    juce::Slider calibLevelSlider;
    juce::TextButton analyzeButton, normalizeButton, resetNormButton;
    juce::Label inputPeakLabel;
    juce::Label gainAppliedLabel;  // "GAIN APPLIED:" label
    juce::TextEditor gainAppliedValue;  // Copyable value only
    juce::Label calibLevelLabel;

    // Parameter attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> calibLevelAttachment;
};

// Advanced DSP Parameters Panel
class AdvancedPanel : public juce::Component
{
public:
    AdvancedPanel(juce::AudioProcessorValueTreeState& apvts);

    void paint(juce::Graphics& g) override;
    void resized() override;

    void setVisible(bool shouldBeVisible) override;
    bool isOpen() const { return getLocalBounds().getHeight() > 0; }

private:
    juce::AudioProcessorValueTreeState& apvts;

    // Section labels
    juce::Label softClipLabel, limitersLabel, compLabel;

    // Soft Clip controls
    juce::Slider scKneeSlider;
    juce::Label scKneeLabel;

    // Slow Limiter controls
    juce::Slider limitRelSlider;
    juce::Label limitRelLabel;
    juce::Slider slLimitAttackSlider;
    juce::Label slLimitAttackLabel;

    // Fast Limiter controls
    juce::Slider flLimitAttackSlider;
    juce::Label flLimitAttackLabel;

    // Gain Compensation controls
    juce::ToggleButton hcCompButton, scCompButton, slCompButton, flCompButton;

    // === ENVELOPE SHAPING CONTROLS ===
    juce::Label envelopeLabel;  // Section header

    // Hard Clip Envelope
    juce::Slider hcAttackSlider, hcSustainSlider;
    juce::Label hcAttackLabel, hcSustainLabel;

    // Soft Clip Envelope
    juce::Slider scAttackSlider, scSustainSlider;
    juce::Label scAttackLabel, scSustainLabel;

    // Slow Limit Envelope
    juce::Slider slAttackSlider, slSustainSlider;
    juce::Label slAttackLabel, slSustainLabel;

    // Fast Limit Envelope
    juce::Slider flAttackSlider, flSustainSlider;
    juce::Label flAttackLabel, flSustainLabel;

    // Parameter attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> scKneeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> limitRelAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> slLimitAttackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> flLimitAttackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> hcCompAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> scCompAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> slCompAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> flCompAttachment;

    // Envelope shaping attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> hcAttackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> hcSustainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> scAttackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> scSustainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> slAttackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> slSustainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> flAttackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> flSustainAttachment;
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

    // Calibration Panel
    CalibrationPanel calibrationPanel;
    juce::TextButton calibrationToggleButton;

    // Advanced Panel
    AdvancedPanel advancedPanel;
    juce::TextButton advancedToggleButton;

    // Visualizations
    Oscilloscope oscilloscope;
    TransferCurveMeter transferCurveMeter;  // Transfer curve with level meter
    juce::TextButton transferCurveToggleButton;  // Toggle button for transfer curve
    StereoMeter inputMeter;   // Stereo input meter
    StereoMeter outputMeter;  // Stereo output meter

    // Global Sliders
    juce::Slider inputGainSlider, outputGainSlider, mixSlider;

    // Threshold Control (remains on left side with Input)
    juce::Slider thresholdSlider;
    juce::Label thresholdLabel;
    juce::TextButton thresholdLinkButton;  // Link button next to threshold knob

    // Gain Linking Control
    juce::ToggleButton gainLinkButton;

    // Bypass Control
    juce::TextButton bypassButton;

    // Master Gain Compensation Control
    juce::TextButton masterCompButton;

    // Per-Type Trim Sliders
    juce::Slider hcTrimSlider, scTrimSlider, slTrimSlider, flTrimSlider;
    juce::TextButton resetTrimsButton;

    // Delta Mode Controls
    juce::TextButton deltaModeButton;
    juce::TextButton overshootDeltaModeButton;
    juce::TextButton truePeakDeltaModeButton;

    // === SIMPLIFIED OUTPUT LIMITER CONTROLS ===
    // Shared ceiling for both limiters
    juce::Slider outputCeilingSlider;
    juce::Label outputCeilingLabel;

    // Overshoot Suppression: Character shaping (allows some overshoot for punch)
    juce::TextButton overshootEnableButton;

    // True Peak Limiter: Strict ITU-R BS.1770-4 compliance (transparent)
    juce::TextButton truePeakEnableButton;

    // Processing Mode Selector (Zero Latency, Balanced, Linear Phase)
    juce::ComboBox processingModeCombo;
    juce::Label processingModeLabel;

    // Version Label (upper right corner)
    juce::Label versionLabel;

    // Toggle Buttons for Muting processors
    juce::ToggleButton hcMuteButton, scMuteButton, slMuteButton, flMuteButton;

    // Toggle Buttons for Soloing processors
    juce::ToggleButton hcSoloButton, scSoloButton, slSoloButton, flSoloButton;

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
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> thresholdAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> thresholdLinkAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> hcTrimAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> scTrimAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> slTrimAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> flTrimAttachment;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> hcMuteAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> scMuteAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> slMuteAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> flMuteAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> hcSoloAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> scSoloAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> slSoloAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> flSoloAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> deltaModeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> overshootDeltaModeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> truePeakDeltaModeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> masterCompAttachment;

    // === SIMPLIFIED OUTPUT LIMITER ATTACHMENTS ===
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputCeilingAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> overshootEnableAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> truePeakEnableAttachment;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> processingModeAttachment;

    // Custom look and feel
    STEVELookAndFeel lookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(QuadBlendDriveAudioProcessorEditor)
};

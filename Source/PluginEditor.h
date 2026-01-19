#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include "PluginProcessor.h"
#include "TransferCurveMeter.h"
#include "StereoMeter.h"
#include "STEVELookAndFeel.h"
#include "STEVEScope.h"

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
class XYPad : public juce::Component, private juce::Timer
{
public:
    XYPad(juce::AudioProcessorValueTreeState& apvts,
          const juce::String& xParamID,
          const juce::String& yParamID,
          QuadBlendDriveAudioProcessor& processor);
    ~XYPad() override;

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;
    void resized() override;

private:
    void timerCallback() override;
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
class CalibrationPanel : public juce::Component, private juce::Timer
{
public:
    CalibrationPanel(juce::AudioProcessorValueTreeState& apvts, QuadBlendDriveAudioProcessor& processor);
    ~CalibrationPanel() override;

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
    void timerCallback() override;

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
class AdvancedPanel : public juce::Component, private juce::Timer
{
public:
    AdvancedPanel(juce::AudioProcessorValueTreeState& apvts, QuadBlendDriveAudioProcessor& processor);
    ~AdvancedPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void setVisible(bool shouldBeVisible) override;
    bool isOpen() const { return getLocalBounds().getHeight() > 0; }

private:
    void timerCallback() override;

    juce::AudioProcessorValueTreeState& apvts;
    QuadBlendDriveAudioProcessor& processor;

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
    juce::Label flLimitAttackLabel;       // Section header "FAST LIMITER"
    juce::Label flLimitAttackTimeLabel;   // "Attack:" label for slider
    juce::Slider flLimitReleaseSlider;
    juce::Label flLimitReleaseLabel;

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
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> flLimitReleaseAttachment;
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

// Color Swatch Button for Preferences Panel
class ColorSwatchButton : public juce::Component
{
public:
    ColorSwatchButton(const juce::String& label, juce::Colour initialColor)
        : labelText(label), currentColor(initialColor)
    {
        changeListener = std::make_unique<ChangeListener>(*this);
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Draw color swatch
        auto swatchBounds = bounds.removeFromLeft(bounds.getHeight()).reduced(2.0f);
        g.setColour(currentColor);
        g.fillRoundedRectangle(swatchBounds, 4.0f);
        g.setColour(juce::Colours::white.withAlpha(0.3f));
        g.drawRoundedRectangle(swatchBounds, 4.0f, 1.0f);

        // Draw label
        g.setColour(juce::Colours::white.withAlpha(0.85f));
        g.setFont(juce::FontOptions(13.0f));
        g.drawText(labelText, bounds.reduced(8.0f, 0.0f), juce::Justification::centredLeft);
    }

    void mouseDown(const juce::MouseEvent&) override
    {
        // Show color picker
        auto colorSelector = std::make_unique<juce::ColourSelector>(
            juce::ColourSelector::showColourAtTop |
            juce::ColourSelector::showSliders |
            juce::ColourSelector::showColourspace);
        colorSelector->setCurrentColour(currentColor);
        colorSelector->setSize(300, 400);
        colorSelector->addChangeListener(changeListener.get());

        juce::CallOutBox::launchAsynchronously(std::move(colorSelector), getScreenBounds(), nullptr);
    }

    void setColor(juce::Colour color)
    {
        currentColor = color;
        repaint();
        if (onColorChanged)
            onColorChanged(currentColor);
    }

    juce::Colour getColor() const { return currentColor; }

    std::function<void(juce::Colour)> onColorChanged;

private:
    class ChangeListener : public juce::ChangeListener
    {
    public:
        ChangeListener(ColorSwatchButton& o) : ownerButton(o) {}
        void changeListenerCallback(juce::ChangeBroadcaster* source) override
        {
            if (auto* selector = dynamic_cast<juce::ColourSelector*>(source))
                ownerButton.setColor(selector->getCurrentColour());
        }
    private:
        ColorSwatchButton& ownerButton;
    };

    juce::String labelText;
    juce::Colour currentColor;
    std::unique_ptr<ChangeListener> changeListener;
};

// Preferences Panel (Processor Colors)
class PreferencesPanel : public juce::Component
{
public:
    PreferencesPanel(QuadBlendDriveAudioProcessor& processor);

    void paint(juce::Graphics& g) override;
    void resized() override;

    void setVisible(bool shouldBeVisible) override;
    bool isOpen() const { return isVisible(); }

    // Sync colors from processor (call when panel opens)
    void syncFromProcessor();

private:
    QuadBlendDriveAudioProcessor& processor;

    juce::Label titleLabel;
    ColorSwatchButton hardClipColor{"Hard Clip", juce::Colour(0xffff5050)};
    ColorSwatchButton softClipColor{"Soft Clip", juce::Colour(0xffff963c)};
    ColorSwatchButton slowLimitColor{"Slow Limit", juce::Colour(0xffffdc50)};
    ColorSwatchButton fastLimitColor{"Fast Limit", juce::Colour(0xff50a0ff)};
    ColorSwatchButton accentColor{"Accent", juce::Colour(0xff4a9eff)};
    juce::TextButton resetButton{"Reset to Defaults"};
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
    void updateABCDButtonStates();  // Update ABCD button visual states

    QuadBlendDriveAudioProcessor& audioProcessor;

    // XY Pad
    XYPad xyPad;

    // Calibration Panel
    CalibrationPanel calibrationPanel;
    juce::TextButton calibrationToggleButton;

    // Advanced Panel
    AdvancedPanel advancedPanel;
    juce::TextButton advancedToggleButton;

    // Preferences Panel
    PreferencesPanel preferencesPanel;
    juce::TextButton preferencesButton;

    // Visualizations
    STEVEScope steveScope;  // Comprehensive waveform scope with settings menu
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

    // === TOOLBAR COMPONENTS ===
    juce::TextButton presetMenuButton;      // "Default Setting" dropdown
    juce::TextButton undoButton, redoButton;
    juce::TextButton helpButton;
    juce::TextButton scopeToggleButton;     // Toggle scope visibility

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

    // Preset management
    std::unique_ptr<juce::FileChooser> fileChooser;
    juce::File currentPresetPath;
    int currentABCDSlot{-1};  // -1 = none, 0-3 = A/B/C/D

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(QuadBlendDriveAudioProcessorEditor)
};

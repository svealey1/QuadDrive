#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
// GainReductionMeter Implementation
//==============================================================================
GainReductionMeter::GainReductionMeter(QuadBlendDriveAudioProcessor& p)
    : processor(p)
{
    grHistory.fill(0.0f);
    startTimerHz(30);  // 30 fps updates
}

void GainReductionMeter::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const float width = bounds.getWidth();
    const float height = bounds.getHeight();

    // Minimal background
    g.setColour(juce::Colour(25, 25, 30));
    g.fillRoundedRectangle(bounds, 4.0f);

    // Compact title
    g.setColour(juce::Colours::white.withAlpha(0.7f));
    g.setFont(juce::Font(11.0f, juce::Font::plain));
    juce::Rectangle<float> titleArea(bounds.getX(), bounds.getY() + 4, width, 18.0f);
    g.drawText("GAIN REDUCTION (3s)", titleArea, juce::Justification::centred);

    juce::Rectangle<float> graphBounds(bounds.getX() + 24, bounds.getY() + 24, width - 32, height - 28);

    // Grid lines for -12 dBFS to 3 dBFS scale (15 dB range)
    g.setColour(juce::Colour(40, 40, 45).withAlpha(0.6f));
    g.setFont(juce::Font(9.0f, juce::Font::plain));

    // -12 dB (bottom)
    float yMinus12 = graphBounds.getBottom();
    g.drawLine(graphBounds.getX(), yMinus12, graphBounds.getRight(), yMinus12, 0.5f);
    g.drawText("-12", bounds.getX() + 2, yMinus12 - 8, 20, 16, juce::Justification::centredLeft);

    // 0 dB (middle-ish, 12 dB from bottom in 15 dB range = 80% up)
    float y0 = graphBounds.getBottom() - (12.0f / 15.0f) * graphBounds.getHeight();
    g.drawLine(graphBounds.getX(), y0, graphBounds.getRight(), y0, 0.5f);
    g.drawText("0", bounds.getX() + 2, y0 - 8, 20, 16, juce::Justification::centredLeft);

    // +3 dB (top)
    float yPlus3 = graphBounds.getY();
    g.drawLine(graphBounds.getX(), yPlus3, graphBounds.getRight(), yPlus3, 0.5f);
    g.drawText("+3", bounds.getX() + 2, yPlus3 - 8, 20, 16, juce::Justification::centredLeft);

    // Draw GR history path
    juce::Path grPath;
    bool pathStarted = false;

    for (int i = 0; i < historySize; ++i)
    {
        int idx = (historyIndex + i) % historySize;
        float grDB = grHistory[idx];

        float x = graphBounds.getX() + (i / float(historySize - 1)) * graphBounds.getWidth();
        // Map from -12 dBFS to +3 dBFS range (15 dB total)
        float y = graphBounds.getBottom() - ((grDB + 12.0f) / 15.0f) * graphBounds.getHeight();
        y = juce::jlimit(graphBounds.getY(), graphBounds.getBottom(), y);

        if (!pathStarted)
        {
            grPath.startNewSubPath(x, y);
            pathStarted = true;
        }
        else
        {
            grPath.lineTo(x, y);
        }
    }

    g.setColour(juce::Colour(255, 140, 60));  // Warm orange
    g.strokePath(grPath, juce::PathStrokeType(1.5f));

    // Subtle border
    g.setColour(juce::Colour(45, 45, 50).withAlpha(0.5f));
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
}

void GainReductionMeter::resized()
{
    repaint();
}

void GainReductionMeter::timerCallback()
{
    float currentGR = processor.currentGainReductionDB.load();
    grHistory[historyIndex] = currentGR;
    historyIndex = (historyIndex + 1) % historySize;
    repaint();
}

//==============================================================================
// Oscilloscope Implementation
//==============================================================================
Oscilloscope::Oscilloscope(QuadBlendDriveAudioProcessor& p)
    : processor(p)
{
    startTimerHz(30);  // 30 fps updates (matches gain reduction meter)
}

void Oscilloscope::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const float width = bounds.getWidth();
    const float height = bounds.getHeight();

    // Minimal background
    g.setColour(juce::Colour(25, 25, 30));
    g.fillRoundedRectangle(bounds, 4.0f);

    // Compact title
    g.setColour(juce::Colours::white.withAlpha(0.7f));
    g.setFont(juce::Font(11.0f, juce::Font::plain));
    juce::Rectangle<float> titleArea(bounds.getX(), bounds.getY() + 4, width, 18.0f);
    g.drawText("OSCILLOSCOPE (3s)", titleArea, juce::Justification::centred);

    juce::Rectangle<float> graphBounds(bounds.getX() + 8, bounds.getY() + 24, width - 16, height - 28);

    // Center line (0V)
    g.setColour(juce::Colour(40, 40, 45).withAlpha(0.6f));
    float centerY = graphBounds.getCentreY();
    g.drawLine(graphBounds.getX(), centerY, graphBounds.getRight(), centerY, 0.5f);

    // Draw waveform
    juce::Path wavePath;
    bool pathStarted = false;

    int writePos = processor.oscilloscopeWritePos.load();
    const int bufferSize = processor.oscilloscopeSize.load();

    if (bufferSize > 0 && static_cast<int>(processor.oscilloscopeBuffer.size()) >= bufferSize)
    {
        // Display points - use width in pixels for efficient rendering
        const int displayPoints = juce::jmin(static_cast<int>(graphBounds.getWidth()), bufferSize);

        for (int i = 0; i < displayPoints; ++i)
        {
            // Map display point to buffer position
            int bufferIndex = (i * bufferSize) / displayPoints;
            int readPos = (writePos + bufferIndex) % bufferSize;

            // Bounds check
            if (readPos >= 0 && readPos < bufferSize)
            {
                float sample = processor.oscilloscopeBuffer[readPos];

                float x = graphBounds.getX() + (i / float(displayPoints - 1)) * graphBounds.getWidth();
                // Scale: ±1.0 = full height, with headroom to ±2.0
                float y = centerY - (sample * graphBounds.getHeight() * 0.4f);
                y = juce::jlimit(graphBounds.getY(), graphBounds.getBottom(), y);

                if (!pathStarted)
                {
                    wavePath.startNewSubPath(x, y);
                    pathStarted = true;
                }
                else
                {
                    wavePath.lineTo(x, y);
                }
            }
        }
    }

    // Draw waveform in bright cyan/blue
    g.setColour(juce::Colour(60, 180, 255));
    g.strokePath(wavePath, juce::PathStrokeType(1.5f));

    // Subtle border
    g.setColour(juce::Colour(45, 45, 50).withAlpha(0.5f));
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
}

void Oscilloscope::resized()
{
    repaint();
}

void Oscilloscope::timerCallback()
{
    repaint();
}

//==============================================================================
// XYPad Implementation
//==============================================================================
XYPad::XYPad(juce::AudioProcessorValueTreeState& apvts,
             const juce::String& xParamID,
             const juce::String& yParamID)
    : apvts(apvts), xParamID(xParamID), yParamID(yParamID)
{
    xSlider.setRange(0.0, 1.0, 0.001);
    ySlider.setRange(0.0, 1.0, 0.001);

    xAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, xParamID, xSlider);
    yAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, yParamID, ySlider);

    // Add listeners to repaint when values change (e.g., from preset recall)
    xSlider.onValueChange = [this] { repaint(); };
    ySlider.onValueChange = [this] { repaint(); };

    addChildComponent(xSlider);
    addChildComponent(ySlider);
}

void XYPad::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const float width = bounds.getWidth();
    const float height = bounds.getHeight();

    // Minimal background
    g.setColour(juce::Colour(25, 25, 30));
    g.fillRoundedRectangle(bounds, 4.0f);

    // Clean grid lines
    g.setColour(juce::Colour(45, 45, 50));
    g.drawLine(width / 2.0f, 0, width / 2.0f, height, 1.0f);
    g.drawLine(0, height / 2.0f, width, height / 2.0f, 1.0f);

    // Minimal quadrant labels
    g.setColour(juce::Colours::white.withAlpha(0.35f));
    g.setFont(juce::Font(10.0f, juce::Font::plain));

    // Top-left: Slow Limit
    juce::Rectangle<float> slowLimitArea(8, 8, width / 2.0f - 16, 18);
    g.drawText("SLOW", slowLimitArea, juce::Justification::centredLeft);

    // Top-right: Hard Clip
    juce::Rectangle<float> hardClipArea(width / 2.0f + 8, 8, width / 2.0f - 16, 18);
    g.drawText("HARD", hardClipArea, juce::Justification::centredRight);

    // Bottom-left: Soft Clip
    juce::Rectangle<float> softClipArea(8, height - 26, width / 2.0f - 16, 18);
    g.drawText("SOFT", softClipArea, juce::Justification::centredLeft);

    // Bottom-right: Fast Limit
    juce::Rectangle<float> fastLimitArea(width / 2.0f + 8, height - 26, width / 2.0f - 16, 18);
    g.drawText("FAST", fastLimitArea, juce::Justification::centredRight);

    // Draw thumb position
    float x = xSlider.getValue();
    float y = 1.0f - ySlider.getValue(); // Invert Y for screen coordinates

    float thumbX = bounds.getX() + x * width;
    float thumbY = bounds.getY() + y * height;

    // Clean thumb design
    g.setColour(juce::Colour(255, 140, 60));  // Warm orange
    g.fillEllipse(thumbX - 7, thumbY - 7, 14, 14);

    g.setColour(juce::Colours::white.withAlpha(0.8f));
    g.drawEllipse(thumbX - 7, thumbY - 7, 14, 14, 1.5f);

    // Subtle border
    g.setColour(juce::Colour(45, 45, 50).withAlpha(0.5f));
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
}

void XYPad::mouseDown(const juce::MouseEvent& event)
{
    auto bounds = getLocalBounds().toFloat();
    float x = juce::jlimit(0.0f, 1.0f, event.position.x / bounds.getWidth());
    float y = juce::jlimit(0.0f, 1.0f, 1.0f - (event.position.y / bounds.getHeight()));

    xSlider.setValue(x, juce::sendNotificationAsync);
    ySlider.setValue(y, juce::sendNotificationAsync);
    repaint();
}

void XYPad::mouseDrag(const juce::MouseEvent& event)
{
    mouseDown(event);
}

void XYPad::resized()
{
    repaint();
}

void XYPad::updateParameters(juce::Point<float> position)
{
    auto bounds = getLocalBounds().toFloat();
    float x = juce::jlimit(0.0f, 1.0f, position.x / bounds.getWidth());
    float y = juce::jlimit(0.0f, 1.0f, 1.0f - (position.y / bounds.getHeight()));

    xSlider.setValue(x);
    ySlider.setValue(y);
}

//==============================================================================
// Editor Implementation
//==============================================================================
QuadBlendDriveAudioProcessorEditor::QuadBlendDriveAudioProcessorEditor(QuadBlendDriveAudioProcessor& p)
    : AudioProcessorEditor(&p),
      audioProcessor(p),
      xyPad(p.apvts, "XY_X_PARAM", "XY_Y_PARAM"),
      grMeter(p),
      oscilloscope(p)
{
    setSize(1120, 890);  // Default size
    setResizable(true, true);
    setResizeLimits(800, 600, 2240, 1780);  // Min 800x600, Max 2x default size

    // Setup XY Pad
    addAndMakeVisible(xyPad);

    // Setup Visualizations
    addAndMakeVisible(grMeter);
    addAndMakeVisible(oscilloscope);

    // Setup Sliders - Global Controls (proportional sizing)
    auto setupRotarySlider = [this](juce::Slider& slider, juce::Label& label, const juce::String& text)
    {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 68, 20);
        addAndMakeVisible(slider);

        label.setText(text, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centredLeft);
        label.setFont(juce::Font(12.0f));
        // Don't attach - we'll position manually
        addAndMakeVisible(label);
    };

    setupRotarySlider(inputGainSlider, inputGainLabel, "Input");
    setupRotarySlider(outputGainSlider, outputGainLabel, "Output");
    setupRotarySlider(mixSlider, mixLabel, "Mix");

    // Calibration/Threshold Control
    setupRotarySlider(calibLevelSlider, calibLevelLabel, "Thresh");

    // Normalization Buttons
    analyzeButton.setButtonText("ANALYZE");
    analyzeButton.onClick = [this]()
    {
        if (audioProcessor.isAnalyzing.load())
        {
            audioProcessor.stopAnalysis();
            analyzeButton.setButtonText("ANALYZE");
        }
        else
        {
            audioProcessor.startAnalysis();
            analyzeButton.setButtonText("STOP");
        }
    };
    addAndMakeVisible(analyzeButton);

    normalizeButton.setButtonText("NORMALIZE");
    normalizeButton.setClickingTogglesState(true);
    normalizeButton.onClick = [this]()
    {
        if (normalizeButton.getToggleState())
            audioProcessor.enableNormalization();
        else
            audioProcessor.disableNormalization();
    };
    addAndMakeVisible(normalizeButton);

    resetNormButton.setButtonText("RESET");
    resetNormButton.onClick = [this]()
    {
        audioProcessor.resetNormalizationData();
        normalizeButton.setToggleState(false, juce::dontSendNotification);
        analyzeButton.setButtonText("ANALYZE");
    };
    addAndMakeVisible(resetNormButton);

    // Gain Link Toggle
    gainLinkButton.setButtonText("Link I/O");
    gainLinkButton.setClickingTogglesState(true);
    gainLinkButton.setToggleState(false, juce::dontSendNotification);
    addAndMakeVisible(gainLinkButton);

    // Add listener to input gain for inverse linking
    inputGainSlider.onValueChange = [this]()
    {
        if (gainLinkButton.getToggleState())
        {
            // Inverse link: if input goes up by X, output goes down by X
            float inputValue = inputGainSlider.getValue();
            outputGainSlider.setValue(-inputValue, juce::sendNotificationSync);
        }
    };

    // Trim Reset Button
    resetTrimsButton.setButtonText("RESET TRIMS");
    resetTrimsButton.onClick = [this]()
    {
        hcTrimSlider.setValue(0.0, juce::sendNotificationSync);
        scTrimSlider.setValue(0.0, juce::sendNotificationSync);
        slTrimSlider.setValue(0.0, juce::sendNotificationSync);
        flTrimSlider.setValue(0.0, juce::sendNotificationSync);
    };
    addAndMakeVisible(resetTrimsButton);

    // Normalization Display Labels
    inputPeakLabel.setText("INPUT PEAK: -- dB", juce::dontSendNotification);
    inputPeakLabel.setJustificationType(juce::Justification::centred);
    inputPeakLabel.setColour(juce::Label::backgroundColourId, juce::Colour(32, 32, 40));
    inputPeakLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.9f));
    addAndMakeVisible(inputPeakLabel);

    // Gain Applied - label and copyable value
    gainAppliedLabel.setText("GAIN APPLIED:", juce::dontSendNotification);
    gainAppliedLabel.setJustificationType(juce::Justification::centredLeft);
    gainAppliedLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.7f));
    addAndMakeVisible(gainAppliedLabel);

    gainAppliedValue.setText("-- dB");
    gainAppliedValue.setReadOnly(true);
    gainAppliedValue.setCaretVisible(false);
    gainAppliedValue.setScrollbarsShown(false);
    gainAppliedValue.setJustification(juce::Justification::centredLeft);
    gainAppliedValue.setColour(juce::TextEditor::backgroundColourId, juce::Colour(32, 32, 40));
    gainAppliedValue.setColour(juce::TextEditor::textColourId, juce::Colours::white.withAlpha(0.9f));
    gainAppliedValue.setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    gainAppliedValue.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(255, 140, 60).withAlpha(0.5f));
    addAndMakeVisible(gainAppliedValue);

    // Start timer for updating normalization display
    startTimerHz(10);  // 10 Hz update rate

    // Setup Trim Sliders (proportional to main knobs)
    auto setupTrimSlider = [this](juce::Slider& slider, juce::Label& label, const juce::String& text)
    {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 62, 18);
        addAndMakeVisible(slider);

        label.setText(text, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centredLeft);
        label.setFont(juce::Font(11.0f));
        addAndMakeVisible(label);
    };

    setupTrimSlider(hcTrimSlider, hcTrimLabel, "HC");
    setupTrimSlider(scTrimSlider, scTrimLabel, "SC");
    setupTrimSlider(slTrimSlider, slTrimLabel, "SL");
    setupTrimSlider(flTrimSlider, flTrimLabel, "FL");

    // Setup Delta Mode Controls
    deltaModeButton.setButtonText("Delta Mode");
    addAndMakeVisible(deltaModeButton);

    // Setup Bypass Toggle
    bypassButton.setButtonText("BYPASS");
    bypassButton.setClickingTogglesState(true);
    bypassButton.setColour(juce::ToggleButton::textColourId, juce::Colours::white);
    bypassButton.setColour(juce::ToggleButton::tickColourId, juce::Colour(255, 60, 60));  // Red when engaged
    addAndMakeVisible(bypassButton);

    // Setup Master Gain Compensation Toggle
    masterCompButton.setButtonText("Gain Comp");
    masterCompButton.setClickingTogglesState(true);
    masterCompButton.setColour(juce::ToggleButton::textColourId, juce::Colours::white);
    masterCompButton.setColour(juce::ToggleButton::tickColourId, juce::Colour(60, 255, 60));  // Green when engaged
    addAndMakeVisible(masterCompButton);

    // Setup Toggle Buttons for Muting with clear visual feedback
    auto setupMuteButton = [](juce::ToggleButton& button)
    {
        button.setButtonText("M");  // "M" for Mute
        button.setClickingTogglesState(true);
        // OFF state (not muted): dark gray background, light text
        button.setColour(juce::TextButton::buttonColourId, juce::Colour(40, 40, 45));
        button.setColour(juce::TextButton::textColourOffId, juce::Colours::grey);
        // ON state (muted): bright red background, white text
        button.setColour(juce::TextButton::buttonOnColourId, juce::Colour(200, 40, 40));
        button.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    };

    setupMuteButton(hcMuteButton);
    setupMuteButton(scMuteButton);
    setupMuteButton(slMuteButton);
    setupMuteButton(flMuteButton);

    addAndMakeVisible(hcMuteButton);
    addAndMakeVisible(scMuteButton);
    addAndMakeVisible(slMuteButton);
    addAndMakeVisible(flMuteButton);

    // Setup Protection Limiter Controls
    protectionEnableButton.setButtonText("PROTECTION");
    protectionEnableButton.setClickingTogglesState(true);
    addAndMakeVisible(protectionEnableButton);

    setupRotarySlider(protectionCeilingSlider, protectionCeilingLabel, "Ceiling");

    // Setup Preset Buttons (A, B, C, D)
    auto setupPresetButton = [this](juce::TextButton& recallBtn, juce::TextButton& saveBtn, const juce::String& label, int slot)
    {
        recallBtn.setButtonText(label);
        recallBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(60, 60, 70));
        recallBtn.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        recallBtn.onClick = [this, slot] { audioProcessor.recallPreset(slot); };
        addAndMakeVisible(recallBtn);

        saveBtn.setButtonText("Save " + label);
        saveBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(40, 40, 50));
        saveBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(180, 180, 180));
        saveBtn.onClick = [this, slot] { audioProcessor.savePreset(slot); };
        addAndMakeVisible(saveBtn);
    };

    setupPresetButton(presetButtonA, saveButtonA, "A", 0);
    setupPresetButton(presetButtonB, saveButtonB, "B", 1);
    setupPresetButton(presetButtonC, saveButtonC, "C", 2);
    setupPresetButton(presetButtonD, saveButtonD, "D", 3);

    // Create Attachments
    inputGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        p.apvts, "INPUT_GAIN", inputGainSlider);
    outputGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        p.apvts, "OUTPUT_GAIN", outputGainSlider);
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        p.apvts, "MIX_WET", mixSlider);
    calibLevelAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        p.apvts, "CALIB_LEVEL", calibLevelSlider);

    hcTrimAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        p.apvts, "HC_TRIM", hcTrimSlider);
    scTrimAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        p.apvts, "SC_TRIM", scTrimSlider);
    slTrimAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        p.apvts, "SL_TRIM", slTrimSlider);
    flTrimAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        p.apvts, "FL_TRIM", flTrimSlider);

    hcMuteAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        p.apvts, "HC_MUTE", hcMuteButton);
    scMuteAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        p.apvts, "SC_MUTE", scMuteButton);
    slMuteAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        p.apvts, "SL_MUTE", slMuteButton);
    flMuteAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        p.apvts, "FL_MUTE", flMuteButton);
    deltaModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        p.apvts, "DELTA_MODE", deltaModeButton);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        p.apvts, "BYPASS", bypassButton);
    masterCompAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        p.apvts, "MASTER_COMP", masterCompButton);
    protectionEnableAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        p.apvts, "PROTECTION_ENABLE", protectionEnableButton);
    protectionCeilingAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        p.apvts, "PROTECTION_CEILING", protectionCeilingSlider);
}

QuadBlendDriveAudioProcessorEditor::~QuadBlendDriveAudioProcessorEditor()
{
    stopTimer();
}

void QuadBlendDriveAudioProcessorEditor::timerCallback()
{
    // Update normalization display labels
    double peakDB = audioProcessor.currentPeakDB.load();
    double gainDB = audioProcessor.normalizationGainDB.load();

    if (peakDB > -60.0)
        inputPeakLabel.setText("INPUT PEAK: " + juce::String(peakDB, 1) + " dB", juce::dontSendNotification);
    else
        inputPeakLabel.setText("INPUT PEAK: -- dB", juce::dontSendNotification);

    // Update only the value, not the label
    if (std::abs(gainDB) > 0.01)
        gainAppliedValue.setText(juce::String(gainDB, 1) + " dB");
    else
        gainAppliedValue.setText("-- dB");
}

void QuadBlendDriveAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Clean dark background
    g.fillAll(juce::Colour(18, 18, 22));

    // Minimal title
    g.setColour(juce::Colours::white.withAlpha(0.9f));
    g.setFont(juce::Font(24.0f, juce::Font::plain));
    g.drawText("QUAD BLEND DRIVE", 0, 10, getWidth(), 35, juce::Justification::centred);
}

void QuadBlendDriveAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop(45); // Title area

    const int padding = 20;
    const int knobSize = 70;  // Proportional knob size
    const int spacing = 10;

    // Reserve bottom for visualizations - FULL WIDTH
    auto oscilloscopeArea = bounds.removeFromBottom(100);
    bounds.removeFromBottom(10); // Spacing
    auto meterArea = bounds.removeFromBottom(90);
    bounds.removeFromBottom(10); // Spacing

    // Symmetrical three-column layout: LEFT (Input) | CENTER (XY) | RIGHT (Output)
    const int sideColumnWidth = 300;
    const int centerColumnWidth = 520;

    // ========== LEFT SECTION (INPUT CONTROLS) ==========
    auto leftSection = bounds.removeFromLeft(sideColumnWidth).reduced(padding, 0);

    // Input Gain
    auto inputRow = leftSection.removeFromTop(knobSize);
    inputGainLabel.setBounds(inputRow.removeFromLeft(60).removeFromTop(knobSize));
    inputGainSlider.setBounds(inputRow.removeFromLeft(knobSize));
    leftSection.removeFromTop(spacing);

    // Calib Level
    auto calibRow = leftSection.removeFromTop(knobSize);
    calibLevelLabel.setBounds(calibRow.removeFromLeft(60).removeFromTop(knobSize));
    calibLevelSlider.setBounds(calibRow.removeFromLeft(knobSize));
    leftSection.removeFromTop(spacing * 2);

    // Normalization displays
    auto normDisplays = leftSection.removeFromTop(56);
    inputPeakLabel.setBounds(normDisplays.removeFromTop(26));
    normDisplays.removeFromTop(4);

    // Gain applied - label and value side by side
    auto gainRow = normDisplays.removeFromTop(26);
    gainAppliedLabel.setBounds(gainRow.removeFromLeft(110));
    gainAppliedValue.setBounds(gainRow);

    leftSection.removeFromTop(spacing);

    // Normalization buttons (3 in a row)
    auto normButtons = leftSection.removeFromTop(28);
    analyzeButton.setBounds(normButtons.removeFromLeft(68));
    normButtons.removeFromLeft(6);
    normalizeButton.setBounds(normButtons.removeFromLeft(76));
    normButtons.removeFromLeft(6);
    resetNormButton.setBounds(normButtons.removeFromLeft(58));

    leftSection.removeFromTop(spacing * 2);

    // 4 Processor Trims - [Label][Knob][Comp]
    // Order (top to bottom): SL, SC, HC, FL (Column-based: Left column, then Right column)
    const int trimRowHeight = 70;
    const int trimKnobSize = 65;
    const int buttonSize = 28;  // Same size as delta mode button

    // SL Trim - [Label][Knob][Mute] (Top-Left quadrant)
    auto slRow = leftSection.removeFromTop(trimRowHeight);
    slTrimLabel.setBounds(slRow.removeFromLeft(42).withTrimmedTop(20));
    slTrimSlider.setBounds(slRow.removeFromLeft(trimKnobSize));
    slRow.removeFromLeft(spacing);
    slMuteButton.setBounds(slRow.removeFromLeft(buttonSize).withTrimmedTop(20));
    leftSection.removeFromTop(spacing);

    // SC Trim - [Label][Knob][Mute] (Bottom-Left quadrant)
    auto scRow = leftSection.removeFromTop(trimRowHeight);
    scTrimLabel.setBounds(scRow.removeFromLeft(42).withTrimmedTop(20));
    scTrimSlider.setBounds(scRow.removeFromLeft(trimKnobSize));
    scRow.removeFromLeft(spacing);
    scMuteButton.setBounds(scRow.removeFromLeft(buttonSize).withTrimmedTop(20));
    leftSection.removeFromTop(spacing);

    // HC Trim - [Label][Knob][Mute] (Top-Right quadrant)
    auto hcRow = leftSection.removeFromTop(trimRowHeight);
    hcTrimLabel.setBounds(hcRow.removeFromLeft(42).withTrimmedTop(20));
    hcTrimSlider.setBounds(hcRow.removeFromLeft(trimKnobSize));
    hcRow.removeFromLeft(spacing);
    hcMuteButton.setBounds(hcRow.removeFromLeft(buttonSize).withTrimmedTop(20));
    leftSection.removeFromTop(spacing);

    // FL Trim - [Label][Knob][Mute] (Bottom-Right quadrant)
    auto flRow = leftSection.removeFromTop(trimRowHeight);
    flTrimLabel.setBounds(flRow.removeFromLeft(42).withTrimmedTop(20));
    flTrimSlider.setBounds(flRow.removeFromLeft(trimKnobSize));
    flRow.removeFromLeft(spacing);
    flMuteButton.setBounds(flRow.removeFromLeft(buttonSize).withTrimmedTop(20));
    leftSection.removeFromTop(spacing);

    // Reset Trims button
    resetTrimsButton.setBounds(leftSection.removeFromTop(28).withSizeKeepingCentre(120, 28));

    // ========== CENTER SECTION (XY PAD) - CENTERED AND SQUARE ==========
    auto centerSection = bounds.removeFromLeft(centerColumnWidth).reduced(padding, 0);

    // Make XY Pad SQUARE - use the smaller dimension
    int availableHeight = centerSection.getHeight();
    int availableWidth = centerSection.getWidth();
    int xySize = juce::jmin(availableWidth, availableHeight);

    // Center the square XY pad both horizontally and vertically
    int xOffset = (availableWidth - xySize) / 2;
    int yOffset = (availableHeight - xySize) / 2;
    xyPad.setBounds(centerSection.getX() + xOffset, centerSection.getY() + yOffset, xySize, xySize);

    // Preset buttons below XY Pad in a single row (30% smaller than original)
    const int presetButtonWidth = 42;   // 60 * 0.7
    const int saveButtonWidth = 56;     // 80 * 0.7
    const int presetButtonHeight = 25;  // 35 * 0.7
    const int presetSpacing = 4;

    int presetYPos = centerSection.getY() + yOffset + xySize + 15; // 15px below XY pad
    int presetXStart = centerSection.getX() + xOffset;

    // Single row: A, Save A, B, Save B, C, Save C, D, Save D
    int xPos = presetXStart;

    presetButtonA.setBounds(xPos, presetYPos, presetButtonWidth, presetButtonHeight);
    xPos += presetButtonWidth + presetSpacing;
    saveButtonA.setBounds(xPos, presetYPos, saveButtonWidth, presetButtonHeight);
    xPos += saveButtonWidth + presetSpacing * 2;

    presetButtonB.setBounds(xPos, presetYPos, presetButtonWidth, presetButtonHeight);
    xPos += presetButtonWidth + presetSpacing;
    saveButtonB.setBounds(xPos, presetYPos, saveButtonWidth, presetButtonHeight);
    xPos += saveButtonWidth + presetSpacing * 2;

    presetButtonC.setBounds(xPos, presetYPos, presetButtonWidth, presetButtonHeight);
    xPos += presetButtonWidth + presetSpacing;
    saveButtonC.setBounds(xPos, presetYPos, saveButtonWidth, presetButtonHeight);
    xPos += saveButtonWidth + presetSpacing * 2;

    presetButtonD.setBounds(xPos, presetYPos, presetButtonWidth, presetButtonHeight);
    xPos += presetButtonWidth + presetSpacing;
    saveButtonD.setBounds(xPos, presetYPos, saveButtonWidth, presetButtonHeight);

    // ========== RIGHT SECTION (OUTPUT CONTROLS) ==========
    auto rightSection = bounds.reduced(padding, 0);

    // Output Gain
    auto outputRow = rightSection.removeFromTop(knobSize);
    outputGainLabel.setBounds(outputRow.removeFromLeft(60).removeFromTop(knobSize));
    outputGainSlider.setBounds(outputRow.removeFromLeft(knobSize));
    rightSection.removeFromTop(spacing);

    // Link I/O button
    gainLinkButton.setBounds(rightSection.removeFromTop(28).withSizeKeepingCentre(knobSize + 10, 28));
    rightSection.removeFromTop(spacing);

    // Bypass button (prominent global control)
    bypassButton.setBounds(rightSection.removeFromTop(32).withSizeKeepingCentre(knobSize + 10, 32));
    rightSection.removeFromTop(spacing);

    // Master Gain Compensation button
    masterCompButton.setBounds(rightSection.removeFromTop(28).withSizeKeepingCentre(knobSize + 10, 28));
    rightSection.removeFromTop(spacing);

    // Delta Mode button (moved below Gain Comp)
    deltaModeButton.setBounds(rightSection.removeFromTop(28).withSizeKeepingCentre(110, 28));
    rightSection.removeFromTop(spacing * 2);

    // Mix
    auto mixRow = rightSection.removeFromTop(knobSize);
    mixLabel.setBounds(mixRow.removeFromLeft(60).removeFromTop(knobSize));
    mixSlider.setBounds(mixRow.removeFromLeft(knobSize));
    rightSection.removeFromTop(spacing * 3);

    // Protection Limiter Controls
    auto protectionButtonRow = rightSection.removeFromTop(28);
    protectionEnableButton.setBounds(protectionButtonRow.removeFromLeft(110));
    rightSection.removeFromTop(spacing);

    auto protectionCeilingRow = rightSection.removeFromTop(knobSize);
    protectionCeilingLabel.setBounds(protectionCeilingRow.removeFromLeft(60).removeFromTop(knobSize));
    protectionCeilingSlider.setBounds(protectionCeilingRow.removeFromLeft(knobSize));
    rightSection.removeFromTop(spacing);

    // ========== BOTTOM (VISUALIZATIONS) - FULL WIDTH ==========
    grMeter.setBounds(meterArea.reduced(padding, 0));
    oscilloscope.setBounds(oscilloscopeArea.reduced(padding, 0));
}

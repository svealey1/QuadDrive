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

    // Background
    g.setColour(juce::Colour(30, 30, 35));
    g.fillRoundedRectangle(bounds, 8.0f);

    // Title
    g.setColour(juce::Colours::white);
    g.setFont(14.0f);
    juce::Rectangle<float> titleArea(bounds.getX(), bounds.getY(), width, 25.0f);
    g.drawText("Gain Reduction", titleArea.reduced(5), juce::Justification::centred);

    juce::Rectangle<float> graphBounds(bounds.getX() + 10, bounds.getY() + 30, width - 20, height - 35);

    // Grid lines
    g.setColour(juce::Colour(50, 50, 55));
    for (int db = 0; db >= -24; db -= 6)
    {
        float y = graphBounds.getY() + graphBounds.getHeight() * (1.0f - (db + 24.0f) / 24.0f);
        g.drawLine(graphBounds.getX(), y, graphBounds.getRight(), y, 1.0f);

        g.setFont(10.0f);
        g.drawText(juce::String(db) + "dB",
                   graphBounds.getX() + 5, y - 6, 40, 12,
                   juce::Justification::left);
    }

    // Draw GR history
    juce::Path grPath;
    bool pathStarted = false;

    for (int i = 0; i < historySize; ++i)
    {
        int idx = (historyIndex + i) % historySize;
        float grDB = grHistory[idx];

        float x = graphBounds.getX() + (i / float(historySize - 1)) * graphBounds.getWidth();
        float y = graphBounds.getBottom() - ((grDB + 24.0f) / 24.0f) * graphBounds.getHeight();
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

    g.setColour(juce::Colours::orange);
    g.strokePath(grPath, juce::PathStrokeType(2.0f));

    // Border
    g.setColour(juce::Colour(60, 60, 65));
    g.drawRoundedRectangle(getLocalBounds().toFloat(), 8.0f, 2.0f);
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
// TransferCurveDisplay Implementation
//==============================================================================
TransferCurveDisplay::TransferCurveDisplay(juce::AudioProcessorValueTreeState& apvts)
    : apvts(apvts)
{
    startTimerHz(30);  // 30 fps updates
}

void TransferCurveDisplay::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const float width = bounds.getWidth();
    const float height = bounds.getHeight();

    // Background
    g.setColour(juce::Colour(30, 30, 35));
    g.fillRoundedRectangle(bounds, 8.0f);

    // Title
    g.setColour(juce::Colours::white);
    g.setFont(14.0f);
    juce::Rectangle<float> titleArea(bounds.getX(), bounds.getY(), width, 25.0f);
    g.drawText("Transfer Function", titleArea.reduced(5), juce::Justification::centred);

    juce::Rectangle<float> graphBounds(bounds.getX() + 10, bounds.getY() + 30, width - 20, height - 35);

    // Grid
    g.setColour(juce::Colour(50, 50, 55));
    g.drawLine(graphBounds.getCentreX(), graphBounds.getY(),
               graphBounds.getCentreX(), graphBounds.getBottom(), 1.0f);
    g.drawLine(graphBounds.getX(), graphBounds.getCentreY(),
               graphBounds.getRight(), graphBounds.getCentreY(), 1.0f);

    // Diagonal reference line (unity gain)
    g.setColour(juce::Colour(60, 60, 70).withAlpha(0.5f));
    g.drawLine(graphBounds.getX(), graphBounds.getBottom(),
               graphBounds.getRight(), graphBounds.getY(), 1.0f);

    // Draw transfer curve
    juce::Path curvePath;
    bool pathStarted = false;

    const int numPoints = 200;
    for (int i = 0; i < numPoints; ++i)
    {
        float inputNorm = i / float(numPoints - 1);
        float input = (inputNorm - 0.5f) * 4.0f;  // Range: -2 to +2

        float output = calculateTransferFunction(input);

        float x = graphBounds.getX() + inputNorm * graphBounds.getWidth();
        float outputNorm = (output / 4.0f) + 0.5f;  // Normalize back to 0-1
        float y = graphBounds.getBottom() - outputNorm * graphBounds.getHeight();

        if (!pathStarted)
        {
            curvePath.startNewSubPath(x, y);
            pathStarted = true;
        }
        else
        {
            curvePath.lineTo(x, y);
        }
    }

    g.setColour(juce::Colours::lightblue);
    g.strokePath(curvePath, juce::PathStrokeType(2.0f));

    // Border
    g.setColour(juce::Colour(60, 60, 65));
    g.drawRoundedRectangle(getLocalBounds().toFloat(), 8.0f, 2.0f);
}

void TransferCurveDisplay::resized()
{
    repaint();
}

void TransferCurveDisplay::timerCallback()
{
    float newX = apvts.getRawParameterValue("XY_X_PARAM")->load();
    float newY = apvts.getRawParameterValue("XY_Y_PARAM")->load();

    if (std::abs(newX - currentX) > 0.01f || std::abs(newY - currentY) > 0.01f)
    {
        currentX = newX;
        currentY = newY;
        repaint();
    }
}

float TransferCurveDisplay::calculateTransferFunction(float input)
{
    // Get current parameters
    const float clipThreshDB = apvts.getRawParameterValue("CLIP_THRESH")->load();
    const float clipThreshold = juce::Decibels::decibelsToGain(clipThreshDB);

    const float hcTrimDB = apvts.getRawParameterValue("HC_TRIM")->load();
    const float scTrimDB = apvts.getRawParameterValue("SC_TRIM")->load();
    const float slTrimDB = apvts.getRawParameterValue("SL_TRIM")->load();
    const float flTrimDB = apvts.getRawParameterValue("FL_TRIM")->load();

    const float hcTrim = juce::Decibels::decibelsToGain(hcTrimDB);
    const float scTrim = juce::Decibels::decibelsToGain(scTrimDB);
    const float slTrim = juce::Decibels::decibelsToGain(slTrimDB);
    const float flTrim = juce::Decibels::decibelsToGain(flTrimDB);

    // Calculate blend weights
    const float wHC = (1.0f - currentX) * (1.0f - currentY);
    const float wSC = currentX * (1.0f - currentY);
    const float wSL = (1.0f - currentX) * currentY;
    const float wFL = currentX * currentY;

    // Apply each processing type
    float hcOut = juce::jlimit(-clipThreshold, clipThreshold, input * hcTrim) / hcTrim;
    float scOut = std::tanh(input * scTrim / clipThreshold) * clipThreshold / scTrim;
    float slOut = juce::jlimit(-clipThreshold, clipThreshold, input * slTrim) / slTrim;  // Simplified limiter
    float flOut = juce::jlimit(-clipThreshold, clipThreshold, input * flTrim) / flTrim;  // Simplified limiter

    // Blend results
    return wHC * hcOut + wSC * scOut + wSL * slOut + wFL * flOut;
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

    addChildComponent(xSlider);
    addChildComponent(ySlider);
}

void XYPad::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const float width = bounds.getWidth();
    const float height = bounds.getHeight();

    // Background
    g.setColour(juce::Colour(30, 30, 35));
    g.fillRoundedRectangle(bounds, 8.0f);

    // Grid lines
    g.setColour(juce::Colour(50, 50, 55));
    g.drawLine(width / 2.0f, 0, width / 2.0f, height, 2.0f);
    g.drawLine(0, height / 2.0f, width, height / 2.0f, 2.0f);

    // Labels for each quadrant (calculate positions without mutating bounds)
    g.setColour(juce::Colours::white.withAlpha(0.6f));
    g.setFont(12.0f);

    // Bottom-left: Hard Clip
    juce::Rectangle<float> hardClipArea(0, height / 2.0f, width / 2.0f, height / 2.0f);
    g.drawText("Hard Clip", hardClipArea.reduced(10), juce::Justification::centred);

    // Bottom-right: Soft Clip
    juce::Rectangle<float> softClipArea(width / 2.0f, height / 2.0f, width / 2.0f, height / 2.0f);
    g.drawText("Soft Clip", softClipArea.reduced(10), juce::Justification::centred);

    // Top-left: Slow Limit
    juce::Rectangle<float> slowLimitArea(0, 0, width / 2.0f, height / 2.0f);
    g.drawText("Slow Limit", slowLimitArea.reduced(10), juce::Justification::centred);

    // Top-right: Fast Limit
    juce::Rectangle<float> fastLimitArea(width / 2.0f, 0, width / 2.0f, height / 2.0f);
    g.drawText("Fast Limit", fastLimitArea.reduced(10), juce::Justification::centred);

    // Draw thumb position
    float x = xSlider.getValue();
    float y = 1.0f - ySlider.getValue(); // Invert Y for screen coordinates

    float thumbX = bounds.getX() + x * width;
    float thumbY = bounds.getY() + y * height;

    g.setColour(juce::Colours::orange);
    g.fillEllipse(thumbX - 8, thumbY - 8, 16, 16);

    g.setColour(juce::Colours::white);
    g.drawEllipse(thumbX - 8, thumbY - 8, 16, 16, 2.0f);

    // Border
    g.setColour(juce::Colour(60, 60, 65));
    g.drawRoundedRectangle(bounds, 8.0f, 2.0f);
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
      transferCurve(p.apvts)
{
    setSize(800, 750);  // Increased height for bottom visualizations

    // Setup XY Pad
    addAndMakeVisible(xyPad);

    // Setup Visualizations
    addAndMakeVisible(grMeter);
    addAndMakeVisible(transferCurve);

    // Setup Sliders - Global Controls
    auto setupRotarySlider = [this](juce::Slider& slider, juce::Label& label, const juce::String& text)
    {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
        addAndMakeVisible(slider);

        label.setText(text, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.attachToComponent(&slider, false);
        addAndMakeVisible(label);
    };

    setupRotarySlider(inputGainSlider, inputGainLabel, "Input");
    setupRotarySlider(outputGainSlider, outputGainLabel, "Output");
    setupRotarySlider(mixSlider, mixLabel, "Mix");
    setupRotarySlider(clipThreshSlider, clipThreshLabel, "Threshold");
    setupRotarySlider(limitReleaseSlider, limitReleaseLabel, "Release");

    // Setup Trim Sliders
    setupRotarySlider(hcTrimSlider, hcTrimLabel, "HC Trim");
    setupRotarySlider(scTrimSlider, scTrimLabel, "SC Trim");
    setupRotarySlider(slTrimSlider, slTrimLabel, "SL Trim");
    setupRotarySlider(flTrimSlider, flTrimLabel, "FL Trim");

    // Setup ComboBox
    oversampleCombo.addItem("1x", 1);
    oversampleCombo.addItem("2x", 2);
    oversampleCombo.addItem("4x", 3);
    oversampleCombo.addItem("8x", 4);
    addAndMakeVisible(oversampleCombo);

    oversampleLabel.setText("Oversample", juce::dontSendNotification);
    oversampleLabel.setJustificationType(juce::Justification::centred);
    oversampleLabel.attachToComponent(&oversampleCombo, false);
    addAndMakeVisible(oversampleLabel);

    // Setup Toggle Buttons
    hcCompButton.setButtonText("HC Comp");
    scCompButton.setButtonText("SC Comp");
    slCompButton.setButtonText("SL Comp");
    flCompButton.setButtonText("FL Comp");
    deltaModeButton.setButtonText("Delta Mode");

    addAndMakeVisible(hcCompButton);
    addAndMakeVisible(scCompButton);
    addAndMakeVisible(slCompButton);
    addAndMakeVisible(flCompButton);
    addAndMakeVisible(deltaModeButton);

    // Create Attachments
    inputGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        p.apvts, "INPUT_GAIN", inputGainSlider);
    outputGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        p.apvts, "OUTPUT_GAIN", outputGainSlider);
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        p.apvts, "MIX_WET", mixSlider);
    clipThreshAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        p.apvts, "CLIP_THRESH", clipThreshSlider);
    limitReleaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        p.apvts, "LIMIT_REL", limitReleaseSlider);

    hcTrimAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        p.apvts, "HC_TRIM", hcTrimSlider);
    scTrimAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        p.apvts, "SC_TRIM", scTrimSlider);
    slTrimAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        p.apvts, "SL_TRIM", slTrimSlider);
    flTrimAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        p.apvts, "FL_TRIM", flTrimSlider);

    oversampleAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        p.apvts, "OVERSAMPLE_VAL", oversampleCombo);

    hcCompAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        p.apvts, "HC_COMP", hcCompButton);
    scCompAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        p.apvts, "SC_COMP", scCompButton);
    slCompAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        p.apvts, "SL_COMP", slCompButton);
    flCompAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        p.apvts, "FL_COMP", flCompButton);
    deltaModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        p.apvts, "DELTA_MODE", deltaModeButton);
}

QuadBlendDriveAudioProcessorEditor::~QuadBlendDriveAudioProcessorEditor()
{
}

void QuadBlendDriveAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Background gradient
    g.fillAll(juce::Colour(20, 20, 25));

    // Title
    g.setColour(juce::Colours::white);
    g.setFont(28.0f);
    g.drawText("Quad-Blend Drive", 0, 10, getWidth(), 40, juce::Justification::centred);

    // Section labels
    g.setFont(16.0f);
    g.setColour(juce::Colours::lightblue);
    g.drawText("Blend", 20, 60, 200, 30, juce::Justification::left);
    g.drawText("Global Controls", 340, 60, 200, 30, juce::Justification::left);
    g.drawText("Per-Type Controls", 20, 360, 200, 30, juce::Justification::left);
}

void QuadBlendDriveAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop(50); // Title area

    // Reserve bottom section for visualizations
    auto bottomSection = bounds.removeFromBottom(180);

    // XY Pad section (left side)
    auto leftSection = bounds.removeFromLeft(320).reduced(20);
    leftSection.removeFromTop(40); // Label space
    xyPad.setBounds(leftSection.removeFromTop(300));

    // Right section
    auto rightSection = bounds.reduced(20);
    rightSection.removeFromTop(40); // Label space

    // Global Controls (top right)
    auto globalControls = rightSection.removeFromTop(120);
    auto knobWidth = 90;
    auto knobHeight = 110;

    inputGainSlider.setBounds(globalControls.removeFromLeft(knobWidth).removeFromTop(knobHeight));
    outputGainSlider.setBounds(globalControls.removeFromLeft(knobWidth).removeFromTop(knobHeight));
    mixSlider.setBounds(globalControls.removeFromLeft(knobWidth).removeFromTop(knobHeight));
    clipThreshSlider.setBounds(globalControls.removeFromLeft(knobWidth).removeFromTop(knobHeight));
    limitReleaseSlider.setBounds(globalControls.removeFromLeft(knobWidth).removeFromTop(knobHeight));

    rightSection.removeFromTop(20);

    // Oversample dropdown
    auto oversampleArea = rightSection.removeFromTop(60);
    oversampleCombo.setBounds(oversampleArea.removeFromLeft(120).removeFromTop(40));

    rightSection.removeFromTop(20);

    // Delta mode button
    deltaModeButton.setBounds(rightSection.removeFromTop(30).removeFromLeft(120));

    rightSection.removeFromTop(40);

    // Per-Type Controls
    auto trimSection = rightSection.removeFromTop(140);

    auto trim1 = trimSection.removeFromLeft(90);
    hcTrimSlider.setBounds(trim1.removeFromTop(110));
    hcCompButton.setBounds(trim1.removeFromTop(25));

    auto trim2 = trimSection.removeFromLeft(90);
    scTrimSlider.setBounds(trim2.removeFromTop(110));
    scCompButton.setBounds(trim2.removeFromTop(25));

    auto trim3 = trimSection.removeFromLeft(90);
    slTrimSlider.setBounds(trim3.removeFromTop(110));
    slCompButton.setBounds(trim3.removeFromTop(25));

    auto trim4 = trimSection.removeFromLeft(90);
    flTrimSlider.setBounds(trim4.removeFromTop(110));
    flCompButton.setBounds(trim4.removeFromTop(25));

    // Bottom Visualizations
    bottomSection.removeFromTop(10); // Spacing
    auto vizSection = bottomSection.reduced(20, 0);

    auto transferCurveArea = vizSection.removeFromLeft(vizSection.getWidth() / 2).reduced(5);
    transferCurve.setBounds(transferCurveArea);

    auto grMeterArea = vizSection.reduced(5);
    grMeter.setBounds(grMeterArea);
}

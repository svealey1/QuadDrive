#include "PluginProcessor.h"
#include "PluginEditor.h"

// Version string - update this single location for all version displays
static const juce::String kPluginVersion = "1.8.7";

//=============================================================================

//==============================================================================
// Master Meter Implementation
//==============================================================================
MasterMeter::MasterMeter(QuadBlendDriveAudioProcessor& p)
    : processor(p)
{
    startTimerHz(30);  // 30 fps updates
}

void MasterMeter::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const float width = bounds.getWidth();
    const float height = bounds.getHeight();

    // Scale factor based on current width (base width is 60)
    const float scale = width / 60.0f;

    // Modern gradient background
    juce::ColourGradient gradient(juce::Colour(22, 22, 26), bounds.getX(), bounds.getY(),
                                   juce::Colour(18, 18, 22), bounds.getX(), bounds.getBottom(), false);
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(bounds, 6.0f * scale);

    // Peak hold numerical display at top
    float peakHoldLdB = peakHoldL > 0.0f ? juce::Decibels::gainToDecibels(peakHoldL, -60.0f) : -60.0f;
    float peakHoldRdB = peakHoldR > 0.0f ? juce::Decibels::gainToDecibels(peakHoldR, -60.0f) : -60.0f;
    float maxPeakHold = juce::jmax(peakHoldLdB, peakHoldRdB);

    g.setFont(juce::Font(10.0f * scale, juce::Font::bold));
    juce::String peakText;
    if (maxPeakHold > -60.0f)
    {
        peakText = juce::String(maxPeakHold, 1) + " dB";
    }
    else
    {
        peakText = "-inf";
    }

    // Color the text based on level
    if (maxPeakHold < -6.0f)
        g.setColour(juce::Colour(80, 220, 120));  // Green
    else if (maxPeakHold < 0.0f)
        g.setColour(juce::Colour(255, 200, 50));  // Yellow
    else
        g.setColour(juce::Colour(255, 80, 80));   // Red

    g.drawText(peakText, bounds.getX(), bounds.getY() + 2 * scale, width, 14 * scale, juce::Justification::centred);

    // Calculate meter area with more padding for labels on the side
    juce::Rectangle<float> meterBounds(bounds.getX() + 3 * scale, bounds.getY() + 18 * scale,
                                        width - 18 * scale, height - 28 * scale);

    // Draw scale: -60 to +3 dB
    const float rangeDB = 63.0f;  // -60 to +3
    g.setFont(juce::Font(6.5f * scale, juce::Font::plain));

    // Draw ticks and labels at key positions
    for (int db = -60; db <= 3; db += 12)
    {
        float normalizedPos = (db + 60.0f) / rangeDB;
        float y = meterBounds.getBottom() - normalizedPos * meterBounds.getHeight();

        if (db == 0)
        {
            // 0 dBFS line (prominent red line)
            g.setColour(juce::Colour(255, 80, 80).withAlpha(0.8f));
            g.drawLine(meterBounds.getX() - 2 * scale, y, meterBounds.getRight(), y, 2.0f * scale);

            g.setColour(juce::Colours::white.withAlpha(0.95f));
            g.drawText("0", meterBounds.getRight() + 1 * scale, y - 5 * scale, 16 * scale, 10 * scale,
                      juce::Justification::centredLeft);
        }
        else if (db == -12)
        {
            g.setColour(juce::Colour(50, 50, 55).withAlpha(0.5f));
            g.drawLine(meterBounds.getX(), y, meterBounds.getRight(), y, 1.0f * scale);

            g.setColour(juce::Colours::white.withAlpha(0.6f));
            g.drawText("-12", meterBounds.getRight() + 1 * scale, y - 5 * scale, 16 * scale, 10 * scale,
                      juce::Justification::centredLeft);
        }
        else
        {
            g.setColour(juce::Colour(50, 50, 55).withAlpha(0.25f));
            g.drawLine(meterBounds.getX(), y, meterBounds.getRight(), y, 0.5f * scale);
        }
    }

    // Convert peak to dB and normalize
    float peakLdB = peakL > 0.0f ? juce::Decibels::gainToDecibels(peakL, -60.0f) : -60.0f;
    float peakRdB = peakR > 0.0f ? juce::Decibels::gainToDecibels(peakR, -60.0f) : -60.0f;

    float normPeakL = juce::jlimit(0.0f, 1.0f, (peakLdB + 60.0f) / rangeDB);
    float normPeakR = juce::jlimit(0.0f, 1.0f, (peakRdB + 60.0f) / rangeDB);

    // Draw meter background bars (dark)
    const float barWidth = (meterBounds.getWidth() - 1.5f * scale) / 2.0f;
    juce::Rectangle<float> leftBar(meterBounds.getX(), meterBounds.getY(), barWidth, meterBounds.getHeight());
    juce::Rectangle<float> rightBar(meterBounds.getX() + barWidth + 1.5f * scale, meterBounds.getY(),
                                     barWidth, meterBounds.getHeight());

    // Draw dark background for both bars
    g.setColour(juce::Colour(15, 15, 18));
    g.fillRect(leftBar);
    g.fillRect(rightBar);

    // Left channel meter fill
    float leftHeight = juce::jmax(2.0f * scale, normPeakL * leftBar.getHeight());
    juce::Rectangle<float> leftFill(leftBar.getX(), leftBar.getBottom() - leftHeight, leftBar.getWidth(), leftHeight);

    // Color gradient based on level - green->yellow at -6dB, yellow->red at 0 dBFS
    if (peakLdB < -6.0f)
        g.setColour(juce::Colour(80, 220, 120));  // Green
    else if (peakLdB < 0.0f)
        g.setColour(juce::Colour(255, 200, 50));  // Yellow
    else
        g.setColour(juce::Colour(255, 80, 80));   // Red (clipping!)

    g.fillRect(leftFill);

    // Right channel meter fill
    float rightHeight = juce::jmax(2.0f * scale, normPeakR * rightBar.getHeight());
    juce::Rectangle<float> rightFill(rightBar.getX(), rightBar.getBottom() - rightHeight, rightBar.getWidth(), rightHeight);

    if (peakRdB < -6.0f)
        g.setColour(juce::Colour(80, 220, 120));  // Green
    else if (peakRdB < 0.0f)
        g.setColour(juce::Colour(255, 200, 50));  // Yellow
    else
        g.setColour(juce::Colour(255, 80, 80));   // Red (clipping!)

    g.fillRect(rightFill);

    // Draw subtle outline around meter bars
    g.setColour(juce::Colour(60, 60, 65).withAlpha(0.3f));
    g.drawRect(leftBar, 0.5f * scale);
    g.drawRect(rightBar, 0.5f * scale);

    // "L" and "R" labels at bottom
    g.setColour(juce::Colours::white.withAlpha(0.7f));
    g.setFont(juce::Font(7.0f * scale, juce::Font::bold));
    g.drawText("L", leftBar.getX(), leftBar.getBottom() + 2 * scale, leftBar.getWidth(), 8 * scale,
              juce::Justification::centred);
    g.drawText("R", rightBar.getX(), rightBar.getBottom() + 2 * scale, rightBar.getWidth(), 8 * scale,
              juce::Justification::centred);

    // Clean outer border
    g.setColour(juce::Colour(60, 60, 65).withAlpha(0.4f));
    g.drawRoundedRectangle(bounds.reduced(0.5f * scale), 6.0f * scale, 1.0f * scale);
}

void MasterMeter::resized()
{
    repaint();
}

void MasterMeter::timerCallback()
{
    peakL = processor.currentOutputPeakL.load();
    peakR = processor.currentOutputPeakR.load();

    // Peak hold logic with 3-second decay
    juce::int64 currentTime = juce::Time::currentTimeMillis();

    // Left channel peak hold
    if (peakL > peakHoldL)
    {
        peakHoldL = peakL;
        peakHoldTimeL = currentTime;
    }
    else if (currentTime - peakHoldTimeL > peakHoldDurationMs)
    {
        peakHoldL = peakL;  // Reset to current if hold time expired
    }

    // Right channel peak hold
    if (peakR > peakHoldR)
    {
        peakHoldR = peakR;
        peakHoldTimeR = currentTime;
    }
    else if (currentTime - peakHoldTimeR > peakHoldDurationMs)
    {
        peakHoldR = peakR;  // Reset to current if hold time expired
    }

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

    // Scale factor based on current height (base height is 400)
    const float scale = height / 400.0f;

    // Modern gradient background
    juce::ColourGradient gradient(juce::Colour(22, 22, 26), bounds.getX(), bounds.getY(),
                                   juce::Colour(18, 18, 22), bounds.getX(), bounds.getBottom(), false);
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(bounds, 6.0f * scale);

    // Sleek title
    g.setColour(juce::Colours::white.withAlpha(0.85f));
    g.setFont(juce::Font(10.0f * scale, juce::Font::plain));
    juce::Rectangle<float> titleArea(bounds.getX(), bounds.getY() + 6 * scale, width, 16.0f * scale);
    g.drawText("WAVEFORM", titleArea, juce::Justification::centred);

    juce::Rectangle<float> graphBounds(bounds.getX() + 10 * scale, bounds.getY() + 26 * scale,
                                        width - 20 * scale, height - 30 * scale);

    // Center line (0V) - subtle
    g.setColour(juce::Colour(50, 50, 55).withAlpha(0.3f));
    float centerY = graphBounds.getCentreY();
    g.drawLine(graphBounds.getX(), centerY, graphBounds.getRight(), centerY, 0.5f * scale);

    // 0 dBFS reference lines (at ±1.0 amplitude = top/bottom 10% of display)
    // This shows headroom - signal should stay between these lines
    g.setColour(juce::Colour(255, 80, 80).withAlpha(0.5f));
    float zeroDbfsTopY = graphBounds.getY() + graphBounds.getHeight() * 0.1f;  // 10% from top
    float zeroDbfsBottomY = graphBounds.getBottom() - graphBounds.getHeight() * 0.1f;  // 10% from bottom
    g.drawLine(graphBounds.getX(), zeroDbfsTopY, graphBounds.getRight(), zeroDbfsTopY, 1.5f * scale);
    g.drawLine(graphBounds.getX(), zeroDbfsBottomY, graphBounds.getRight(), zeroDbfsBottomY, 1.5f * scale);

    // Labels for 0 dBFS lines
    g.setFont(juce::Font(8.0f * scale, juce::Font::plain));
    g.setColour(juce::Colour(255, 80, 80).withAlpha(0.7f));
    g.drawText("0dBFS", graphBounds.getRight() - 40 * scale, zeroDbfsTopY - 10 * scale,
              38 * scale, 12 * scale, juce::Justification::centredRight);
    g.drawText("0dBFS", graphBounds.getRight() - 40 * scale, zeroDbfsBottomY - 2 * scale,
              38 * scale, 12 * scale, juce::Justification::centredRight);

    // Draw waveform with three-band RGB coloring - smooth interpolated rendering
    int writePos = processor.oscilloscopeWritePos.load();
    const int bufferSize = processor.oscilloscopeSize.load();

    if (bufferSize > 0 &&
        static_cast<int>(processor.oscilloscopeBuffer.size()) >= bufferSize &&
        static_cast<int>(processor.oscilloscopeLowBand.size()) >= bufferSize &&
        static_cast<int>(processor.oscilloscopeMidBand.size()) >= bufferSize &&
        static_cast<int>(processor.oscilloscopeHighBand.size()) >= bufferSize)
    {
        // Use more points for smoother rendering (up to 2x the width for extra smoothness)
        const int displayPoints = juce::jmin(static_cast<int>(graphBounds.getWidth() * 2.0f), bufferSize);

        // Create smooth path for waveform
        juce::Path waveformPath;
        bool pathStarted = false;

        for (int i = 0; i < displayPoints; ++i)
        {
            int bufferIndex = (i * bufferSize) / displayPoints;
            int readPos = bufferIndex;  // Still window: read directly from buffer position

            if (readPos >= 0 && readPos < bufferSize)
            {
                float sample = processor.oscilloscopeBuffer[readPos];
                float x = graphBounds.getX() + (i / float(displayPoints - 1)) * graphBounds.getWidth();
                float y = centerY - (sample * graphBounds.getHeight() * 0.4f);
                y = juce::jlimit(graphBounds.getY(), graphBounds.getBottom(), y);

                if (!pathStarted)
                {
                    waveformPath.startNewSubPath(x, y);
                    pathStarted = true;
                }
                else
                {
                    // Use quadratic interpolation for smooth curves
                    waveformPath.lineTo(x, y);
                }
            }
        }

        // Draw filled waveform with RGB coloring - fill between waveform and zero line
        const int colorSegments = juce::jmin(200, displayPoints / 4);
        for (int seg = 0; seg < colorSegments; ++seg)
        {
            int startIdx = (seg * displayPoints) / colorSegments;
            int endIdx = ((seg + 1) * displayPoints) / colorSegments;

            if (startIdx >= displayPoints || endIdx > displayPoints)
                continue;

            int bufferIndex = (startIdx * bufferSize) / displayPoints;
            int readPos = bufferIndex;

            if (readPos >= 0 && readPos < bufferSize)
            {
                // Get three-band amplitudes for RGB coloring
                float lowAmp = processor.oscilloscopeLowBand[readPos];
                float midAmp = processor.oscilloscopeMidBand[readPos];
                float highAmp = processor.oscilloscopeHighBand[readPos];

                // Full brightness - scale values directly to 0-255 range
                float colorScale = 4.0f;
                uint8_t red = static_cast<uint8_t>(juce::jlimit(0.0f, 255.0f, lowAmp * colorScale * 255.0f));
                uint8_t green = static_cast<uint8_t>(juce::jlimit(0.0f, 255.0f, midAmp * colorScale * 255.0f));
                uint8_t blue = static_cast<uint8_t>(juce::jlimit(0.0f, 255.0f, highAmp * colorScale * 255.0f));

                juce::Colour lineColor(red, green, blue);

                // Create filled path from waveform to center line
                juce::Path fillPath;
                juce::Path strokePath;
                bool pathStarted = false;
                float firstX = 0, lastX = 0;

                for (int i = startIdx; i <= endIdx && i < displayPoints; ++i)
                {
                    int bufIdx = (i * bufferSize) / displayPoints;
                    int rPos = bufIdx;

                    if (rPos >= 0 && rPos < bufferSize)
                    {
                        float sample = processor.oscilloscopeBuffer[rPos];
                        float x = graphBounds.getX() + (i / float(displayPoints - 1)) * graphBounds.getWidth();
                        float y = centerY - (sample * graphBounds.getHeight() * 0.4f);
                        y = juce::jlimit(graphBounds.getY(), graphBounds.getBottom(), y);

                        if (!pathStarted)
                        {
                            fillPath.startNewSubPath(x, centerY);  // Start at center line
                            fillPath.lineTo(x, y);  // Go to waveform
                            strokePath.startNewSubPath(x, y);
                            firstX = x;
                            pathStarted = true;
                        }
                        else
                        {
                            fillPath.lineTo(x, y);
                            strokePath.lineTo(x, y);
                        }
                        lastX = x;
                    }
                }

                // Close the fill path back to center line
                if (pathStarted)
                {
                    fillPath.lineTo(lastX, centerY);  // Go back to center line
                    fillPath.closeSubPath();

                    // Draw semi-transparent fill
                    g.setColour(lineColor.withAlpha(0.3f));
                    g.fillPath(fillPath);

                    // Draw the stroke on top
                    g.setColour(lineColor);
                    g.strokePath(strokePath, juce::PathStrokeType(2.0f * scale, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                }
            }
        }

        // Draw moving cursor at synchronized display position (matches GR meter)
        float cursorX = graphBounds.getX() + (displayCursorPos / float(displaySize)) * graphBounds.getWidth();
        g.setColour(juce::Colours::white.withAlpha(0.6f));
        g.drawLine(cursorX, graphBounds.getY(), cursorX, graphBounds.getBottom(), 2.0f * scale);

        // Draw GR trace overlay
        paintGRTrace(g, graphBounds, scale);
    }

    // Clean outer border
    g.setColour(juce::Colour(60, 60, 65).withAlpha(0.4f));
    g.drawRoundedRectangle(bounds.reduced(0.5f * scale), 6.0f * scale, 1.0f * scale);
}

void Oscilloscope::resized()
{
    repaint();
}

void Oscilloscope::timerCallback()
{
    // Increment display cursor (synchronized with GR meter)
    displayCursorPos = (displayCursorPos + 1) % displaySize;
    repaint();
}

void Oscilloscope::paintGRTrace(juce::Graphics& g, juce::Rectangle<float> graphBounds, float scale)
{
    const int bufferSize = processor.oscilloscopeSize.load();
    const float width = graphBounds.getWidth();
    const float height = graphBounds.getHeight();

    // Vertical scale - match waveform's dB range
    const float maxDisplayDb = 48.0f;  // Full vertical range
    const float grDisplayRangeDb = 24.0f;  // Max GR depth to display
    const float pixelsPerDb = height / maxDisplayDb;

    auto* thresholdParam = processor.apvts.getRawParameterValue("THRESHOLD");
    const float thresholdDb = thresholdParam ? thresholdParam->load() : -6.0f;

    // 0 dBFS at TOP of waveform display (top 0 dBFS reference line is at 10% from top)
    const float zeroDbY = graphBounds.getY() + graphBounds.getHeight() * 0.1f;

    // Threshold line position (hangs DOWN from 0 dBFS by |thresholdDb| amount)
    // e.g., -6 dB threshold = 6dB below the top
    const float thresholdY = zeroDbY + std::abs(thresholdDb) * pixelsPerDb;

    // Downsampling for display
    const int displayPoints = juce::jmin(static_cast<int>(width * 2.0f), bufferSize);

    // Build GR path
    juce::Path grPath;
    grPath.startNewSubPath(graphBounds.getX(), thresholdY);

    for (int i = 0; i < displayPoints; ++i)
    {
        // Instead of segmenting bufferSize, just index buffer directly since we want pixel = time:
        int bufferIndex = (i * bufferSize) / displayPoints;
        int readPos = bufferIndex;
        float grDb = 0.0f;
        if (readPos >= 0 && readPos < bufferSize && readPos < processor.oscilloscopeGR.size())
            grDb = processor.oscilloscopeGR[readPos];
        // Clamp and map to screen
        grDb = std::max(grDb, -grDisplayRangeDb);
        float grY = thresholdY + std::abs(grDb) * pixelsPerDb;
        grY = std::min(grY, graphBounds.getBottom());
        float x = graphBounds.getX() + (i / float(displayPoints - 1)) * width;
        if (i == 0)
            grPath.startNewSubPath(x, grY);
        else
            grPath.lineTo(x, grY);
    }

    // Close path back to threshold line
    grPath.lineTo(graphBounds.getRight(), thresholdY);
    grPath.closeSubPath();

    // Render filled area - semi-transparent orange
    g.setColour(juce::Colour(0x66FF6600));
    g.fillPath(grPath);

    // Stroke outline - brighter orange
    g.setColour(juce::Colour(0xFFFF8800));
    g.strokePath(grPath, juce::PathStrokeType(1.5f * scale));

    // Threshold line - grey
    g.setColour(juce::Colours::grey.withAlpha(0.7f));
    g.drawLine(graphBounds.getX(), thresholdY, graphBounds.getRight(), thresholdY, 1.0f * scale);

    // 0 dBFS reference line - white (at top 0 dBFS position)
    g.setColour(juce::Colours::white.withAlpha(0.5f));
    g.drawLine(graphBounds.getX(), zeroDbY, graphBounds.getRight(), zeroDbY, 1.0f * scale);

    // Label threshold value
    g.setColour(juce::Colours::grey.withAlpha(0.9f));
    g.setFont(juce::Font(8.0f * scale, juce::Font::plain));
    juce::String thresholdLabel = juce::String(thresholdDb, 1) + " dB";
    g.drawText(thresholdLabel, graphBounds.getX() + 5 * scale, thresholdY - 12 * scale,
               60 * scale, 10 * scale, juce::Justification::centredLeft);
}

//==============================================================================
//==============================================================================
// XYPad Implementation
//==============================================================================
XYPad::XYPad(juce::AudioProcessorValueTreeState& apvts,
             const juce::String& xParamID,
             const juce::String& yParamID,
             QuadBlendDriveAudioProcessor& processor)
    : apvts(apvts), xParamID(xParamID), yParamID(yParamID), processor(processor)
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

    // Start timer for continuous ring updates (for mute toggle responsiveness)
    startTimerHz(30);
}

XYPad::~XYPad()
{
    stopTimer();
}

void XYPad::timerCallback()
{
    // Repaint to update ring colors when mute states or GR values change
    repaint();
}

void XYPad::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const float width = bounds.getWidth();
    const float height = bounds.getHeight();
    const float centerX = width / 2.0f;
    const float centerY = height / 2.0f;

    // Get processor colors
    const auto& colors = processor.processorColors;

    // Get mute states
    bool hcMuted = apvts.getRawParameterValue("HC_MUTE")->load() > 0.5f;
    bool scMuted = apvts.getRawParameterValue("SC_MUTE")->load() > 0.5f;
    bool slMuted = apvts.getRawParameterValue("SL_MUTE")->load() > 0.5f;
    bool flMuted = apvts.getRawParameterValue("FL_MUTE")->load() > 0.5f;

    // Get GR values (for ring glow intensity)
    float hcGR = processor.currentHardClipGR.load();
    float scGR = processor.currentSoftClipGR.load();
    float slGR = processor.currentSlowLimitGR.load();
    float flGR = processor.currentFastLimitGR.load();
    float totalGR = hcGR + scGR + slGR + flGR;

    // Modern gradient background - darker for contrast with ring
    juce::ColourGradient bgGradient(juce::Colour(12, 12, 16), centerX, centerY,
                                     juce::Colour(8, 8, 10), centerX, bounds.getBottom(), true);
    g.setGradientFill(bgGradient);
    g.fillRoundedRectangle(bounds, 8.0f);

    // === BLENDED RING VISUALIZATION ===
    // Single ring with color that blends based on XY position (matches thumb color)
    const float ringRadius = std::min(width, height) * 0.42f;
    const float ringThickness = 3.0f;
    const float pi = juce::MathConstants<float>::pi;

    // Calculate blend weights based on XY position
    float xPos = static_cast<float>(xSlider.getValue());
    float yPos = static_cast<float>(ySlider.getValue());

    // Get trim values in dB (-24 to +24) and convert to linear gain for weighting
    float hcTrimDB = apvts.getRawParameterValue("HC_TRIM")->load();
    float scTrimDB = apvts.getRawParameterValue("SC_TRIM")->load();
    float slTrimDB = apvts.getRawParameterValue("SL_TRIM")->load();
    float flTrimDB = apvts.getRawParameterValue("FL_TRIM")->load();

    // Convert dB to linear gain (higher trim = more contribution to blend)
    float hcTrimGain = juce::Decibels::decibelsToGain(hcTrimDB);
    float scTrimGain = juce::Decibels::decibelsToGain(scTrimDB);
    float slTrimGain = juce::Decibels::decibelsToGain(slTrimDB);
    float flTrimGain = juce::Decibels::decibelsToGain(flTrimDB);

    // Calculate base corner weights from XY position
    // Hard (TL): high when X=0, Y=1
    // Fast (TR): high when X=1, Y=1
    // Soft (BL): high when X=0, Y=0
    // Slow (BR): high when X=1, Y=0
    float hcWeight = (1.0f - xPos) * yPos;        // Top-left
    float flWeight = xPos * yPos;                  // Top-right
    float scWeight = (1.0f - xPos) * (1.0f - yPos); // Bottom-left
    float slWeight = xPos * (1.0f - yPos);         // Bottom-right

    // Apply trim gain as weight multiplier (higher gain = more color contribution)
    hcWeight *= hcTrimGain;
    scWeight *= scTrimGain;
    slWeight *= slTrimGain;
    flWeight *= flTrimGain;

    // Zero out muted processors completely
    if (hcMuted) hcWeight = 0.0f;
    if (scMuted) scWeight = 0.0f;
    if (slMuted) slWeight = 0.0f;
    if (flMuted) flWeight = 0.0f;

    // Normalize weights (so they sum to 1)
    float totalWeight = hcWeight + scWeight + slWeight + flWeight;

    juce::Colour ringColor;
    if (totalWeight > 0.001f)
    {
        hcWeight /= totalWeight;
        scWeight /= totalWeight;
        slWeight /= totalWeight;
        flWeight /= totalWeight;

        // Blend colors based on normalized weights
        float r = colors.hardClip.getFloatRed() * hcWeight +
                  colors.softClip.getFloatRed() * scWeight +
                  colors.slowLimit.getFloatRed() * slWeight +
                  colors.fastLimit.getFloatRed() * flWeight;
        float gr = colors.hardClip.getFloatGreen() * hcWeight +
                   colors.softClip.getFloatGreen() * scWeight +
                   colors.slowLimit.getFloatGreen() * slWeight +
                   colors.fastLimit.getFloatGreen() * flWeight;
        float b = colors.hardClip.getFloatBlue() * hcWeight +
                  colors.softClip.getFloatBlue() * scWeight +
                  colors.slowLimit.getFloatBlue() * slWeight +
                  colors.fastLimit.getFloatBlue() * flWeight;

        ringColor = juce::Colour::fromFloatRGBA(r, gr, b, 1.0f);
    }
    else
    {
        // All processors muted - show dim grey
        ringColor = juce::Colour(80, 80, 85);
    }

    // Draw the unified ring
    juce::Path ringPath;
    ringPath.addCentredArc(centerX, centerY, ringRadius, ringRadius, 0.0f, 0.0f, pi * 2.0f, true);
    g.setColour(ringColor.withAlpha(0.7f));
    g.strokePath(ringPath, juce::PathStrokeType(ringThickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // === GR-DRIVEN CENTER GLOW ===
    // Radial gradient from center that pulses with total GR
    float glowIntensity = juce::jmap(totalGR, 0.0f, 12.0f, 0.15f, 0.6f);
    glowIntensity = juce::jlimit(0.15f, 0.6f, glowIntensity);

    juce::ColourGradient centerGlow(ringColor.withAlpha(glowIntensity), centerX, centerY,
                                     juce::Colours::transparentBlack, centerX, centerY + ringRadius * 0.6f, true);
    g.setGradientFill(centerGlow);
    g.fillEllipse(centerX - ringRadius * 0.6f, centerY - ringRadius * 0.6f, ringRadius * 1.2f, ringRadius * 1.2f);

    // === CORNER INDICATORS ===
    // Small dots at each corner that show processor state
    const float cornerOffset = 18.0f;
    const float indicatorSize = 8.0f;

    // trimGain: 1.0 at 0dB, higher for positive trim, lower for negative
    // More aggressive mapping so small boosts (1-6dB) are clearly visible
    auto trimToAlphaMultiplier = [](float trimGain) {
        // At 0dB (gain=1.0): multiplier = 1.0
        // At +6dB (gain=2.0): multiplier ≈ 1.6
        // At +12dB (gain=4.0): multiplier ≈ 2.5
        // At -12dB (gain=0.25): multiplier ≈ 0.4
        // At -24dB (gain=0.06): multiplier ≈ 0.15
        float multiplier = std::pow(trimGain, 0.7f);  // Exponent < 1.0 = more sensitive to small changes
        return juce::jlimit(0.15f, 2.5f, multiplier);
    };

    auto drawCornerIndicator = [&](float cx, float cy, juce::Colour color, float gr, bool muted, float trimGain)
    {
        float trimAlpha = trimToAlphaMultiplier(trimGain);
        float alpha = muted ? 0.15f : (0.3f + 0.6f * trimAlpha);  // Base 0.3, scale up to 1.8 with max trim
        alpha = juce::jlimit(0.15f, 1.0f, alpha);

        // Glow
        if (!muted && gr > 0.1f)
        {
            g.setColour(color.withAlpha(alpha * 0.4f));
            g.fillEllipse(cx - indicatorSize, cy - indicatorSize, indicatorSize * 2, indicatorSize * 2);
        }

        // Core dot
        g.setColour(muted ? color.withSaturation(0.1f).withBrightness(0.3f) : color.withAlpha(alpha));
        g.fillEllipse(cx - indicatorSize * 0.5f, cy - indicatorSize * 0.5f, indicatorSize, indicatorSize);
    };

    // Corner positions: Hard (TL), Fast (TR), Soft (BL), Slow (BR)
    drawCornerIndicator(cornerOffset, cornerOffset, colors.hardClip, hcGR, hcMuted, hcTrimGain);
    drawCornerIndicator(width - cornerOffset, cornerOffset, colors.fastLimit, flGR, flMuted, flTrimGain);
    drawCornerIndicator(cornerOffset, height - cornerOffset, colors.softClip, scGR, scMuted, scTrimGain);
    drawCornerIndicator(width - cornerOffset, height - cornerOffset, colors.slowLimit, slGR, slMuted, slTrimGain);

    // === CORNER LABELS ===
    g.setFont(juce::FontOptions(9.5f));

    auto drawCornerLabel = [&](float lx, float ly, const juce::String& text, juce::Colour color, bool muted, float trimGain, juce::Justification just)
    {
        float trimAlpha = trimToAlphaMultiplier(trimGain);
        float alpha = muted ? 0.4f : 0.35f + 0.55f * trimAlpha;  // Base 0.35, scale up with trim
        alpha = juce::jlimit(0.25f, 1.0f, alpha);
        g.setColour(muted ? juce::Colours::grey.withAlpha(0.4f) : color.withAlpha(alpha));
        juce::Rectangle<float> labelArea(lx, ly, 40, 14);
        g.drawText(text, labelArea, just);
    };

    drawCornerLabel(cornerOffset + 12, cornerOffset - 3, "HARD", colors.hardClip, hcMuted, hcTrimGain, juce::Justification::centredLeft);
    drawCornerLabel(width - cornerOffset - 52, cornerOffset - 3, "FAST", colors.fastLimit, flMuted, flTrimGain, juce::Justification::centredRight);
    drawCornerLabel(cornerOffset + 12, height - cornerOffset - 11, "SOFT", colors.softClip, scMuted, scTrimGain, juce::Justification::centredLeft);
    drawCornerLabel(width - cornerOffset - 52, height - cornerOffset - 11, "SLOW", colors.slowLimit, slMuted, slTrimGain, juce::Justification::centredRight);

    // === THUMB/DOT ===
    float thumbX = bounds.getX() + xPos * width;
    float thumbY = bounds.getY() + (1.0f - yPos) * height;

    // Use the same blended color for thumb (accounts for mutes and trims)
    juce::Colour thumbColor = ringColor;

    // Outer glow (processor-colored)
    g.setColour(thumbColor.withAlpha(0.25f));
    g.fillEllipse(thumbX - 12, thumbY - 12, 24, 24);

    // Middle glow
    g.setColour(thumbColor.withAlpha(0.5f));
    g.fillEllipse(thumbX - 8, thumbY - 8, 16, 16);

    // Solid thumb
    g.setColour(thumbColor);
    g.fillEllipse(thumbX - 6, thumbY - 6, 12, 12);

    // White center dot
    g.setColour(juce::Colours::white.withAlpha(0.95f));
    g.fillEllipse(thumbX - 2, thumbY - 2, 4, 4);

    // === OUTER BORDER ===
    g.setColour(juce::Colour(50, 50, 55).withAlpha(0.6f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 8.0f, 1.5f);
}

void XYPad::mouseDown(const juce::MouseEvent& event)
{
    // Check if command key is pressed (macOS) or ctrl key (Windows/Linux)
    wasCommandPressed = event.mods.isCommandDown();

    if (wasCommandPressed)
    {
        // Store current states
        auto* deltaModeParam = apvts.getParameter("DELTA_MODE");
        auto* masterCompParam = apvts.getParameter("MASTER_COMP");

        deltaModeWasOn = deltaModeParam && deltaModeParam->getValue() > 0.5f;
        gainCompWasOn = masterCompParam && masterCompParam->getValue() > 0.5f;

        // Temporarily enable both
        if (deltaModeParam)
            deltaModeParam->setValueNotifyingHost(1.0f);
        if (masterCompParam)
            masterCompParam->setValueNotifyingHost(1.0f);
    }

    auto bounds = getLocalBounds().toFloat();
    float x = juce::jlimit(0.0f, 1.0f, event.position.x / bounds.getWidth());
    float y = juce::jlimit(0.0f, 1.0f, 1.0f - (event.position.y / bounds.getHeight()));

    xSlider.setValue(x, juce::sendNotificationAsync);
    ySlider.setValue(y, juce::sendNotificationAsync);
    repaint();
}

void XYPad::mouseDrag(const juce::MouseEvent& event)
{
    auto bounds = getLocalBounds().toFloat();
    float x = juce::jlimit(0.0f, 1.0f, event.position.x / bounds.getWidth());
    float y = juce::jlimit(0.0f, 1.0f, 1.0f - (event.position.y / bounds.getHeight()));

    xSlider.setValue(x, juce::sendNotificationAsync);
    ySlider.setValue(y, juce::sendNotificationAsync);
    repaint();
}

void XYPad::mouseUp(const juce::MouseEvent& event)
{
    // Restore previous states if command was pressed
    if (wasCommandPressed)
    {
        auto* deltaModeParam = apvts.getParameter("DELTA_MODE");
        auto* masterCompParam = apvts.getParameter("MASTER_COMP");

        if (deltaModeParam)
            deltaModeParam->setValueNotifyingHost(deltaModeWasOn ? 1.0f : 0.0f);
        if (masterCompParam)
            masterCompParam->setValueNotifyingHost(gainCompWasOn ? 1.0f : 0.0f);

        wasCommandPressed = false;
    }
}

void XYPad::mouseDoubleClick(const juce::MouseEvent& event)
{
    // Reset XY pad to center position (0.5, 0.5)
    xSlider.setValue(0.5, juce::sendNotificationAsync);
    ySlider.setValue(0.5, juce::sendNotificationAsync);
    repaint();
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
// Advanced Panel Implementation
//==============================================================================
// =============================================================================
// Calibration Panel Implementation
// =============================================================================

CalibrationPanel::CalibrationPanel(juce::AudioProcessorValueTreeState& apvts, QuadBlendDriveAudioProcessor& processor)
    : apvts(apvts), processor(processor)
{
    // Calibration Level Slider
    calibLevelSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    calibLevelSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 18);
    calibLevelSlider.setDoubleClickReturnValue(true, 0.0);
    addAndMakeVisible(calibLevelSlider);

    calibLevelLabel.setText("Calibration", juce::dontSendNotification);
    calibLevelLabel.setJustificationType(juce::Justification::centred);
    calibLevelLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.75f));
    addAndMakeVisible(calibLevelLabel);

    // Learn Button (was Analyze)
    analyzeButton.setButtonText("LEARN");
    analyzeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(35, 35, 40));
    analyzeButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.85f));
    analyzeButton.onClick = [this, &proc = this->processor]()
    {
        if (proc.isAnalyzing.load())
        {
            proc.stopAnalysis();
            analyzeButton.setButtonText("LEARN");
        }
        else
        {
            proc.startAnalysis();
            analyzeButton.setButtonText("STOP");
        }
    };
    addAndMakeVisible(analyzeButton);

    // Apply Button (was Normalize)
    normalizeButton.setButtonText("APPLY");
    normalizeButton.setClickingTogglesState(true);
    normalizeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(35, 35, 40));
    normalizeButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(255, 120, 50));
    normalizeButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.85f));
    normalizeButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    normalizeButton.onClick = [this, &proc = this->processor]()
    {
        if (normalizeButton.getToggleState())
            proc.enableNormalization();
        else
            proc.disableNormalization();
    };
    addAndMakeVisible(normalizeButton);

    // Reset Button
    resetNormButton.setButtonText("RESET");
    resetNormButton.setColour(juce::TextButton::buttonColourId, juce::Colour(35, 35, 40));
    resetNormButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.85f));
    resetNormButton.onClick = [this, &proc = this->processor]()
    {
        proc.resetNormalizationData();
        normalizeButton.setToggleState(false, juce::dontSendNotification);
        analyzeButton.setButtonText("LEARN");
    };
    addAndMakeVisible(resetNormButton);

    // Input Peak Label
    inputPeakLabel.setText("INPUT PEAK: -- dB", juce::dontSendNotification);
    inputPeakLabel.setJustificationType(juce::Justification::centred);
    inputPeakLabel.setColour(juce::Label::backgroundColourId, juce::Colour(28, 28, 32));
    inputPeakLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.9f));
    inputPeakLabel.setColour(juce::Label::outlineColourId, juce::Colour(60, 60, 65).withAlpha(0.3f));
    addAndMakeVisible(inputPeakLabel);

    // Gain Applied Label
    gainAppliedLabel.setText("GAIN APPLIED:", juce::dontSendNotification);
    gainAppliedLabel.setJustificationType(juce::Justification::centredLeft);
    gainAppliedLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.75f));
    addAndMakeVisible(gainAppliedLabel);

    // Gain Applied Value (copyable)
    gainAppliedValue.setText("-- dB");
    gainAppliedValue.setReadOnly(true);
    gainAppliedValue.setCaretVisible(false);
    gainAppliedValue.setScrollbarsShown(false);
    gainAppliedValue.setJustification(juce::Justification::centredLeft);
    gainAppliedValue.setColour(juce::TextEditor::backgroundColourId, juce::Colour(28, 28, 32));
    gainAppliedValue.setColour(juce::TextEditor::textColourId, juce::Colours::white.withAlpha(0.9f));
    gainAppliedValue.setColour(juce::TextEditor::outlineColourId, juce::Colour(60, 60, 65).withAlpha(0.3f));
    gainAppliedValue.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(255, 120, 50).withAlpha(0.5f));
    addAndMakeVisible(gainAppliedValue);

    // Create attachment
    calibLevelAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, "CALIB_LEVEL", calibLevelSlider);

    // Start timer to update colors
    startTimerHz(15);

    // Start collapsed
    setVisible(false);
}

CalibrationPanel::~CalibrationPanel()
{
    stopTimer();
}

void CalibrationPanel::timerCallback()
{
    // Update calibration slider color to match accent
    calibLevelSlider.setColour(juce::Slider::thumbColourId, processor.processorColors.accent);
}

void CalibrationPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(25, 25, 28));
    g.setColour(juce::Colour(60, 60, 65).withAlpha(0.3f));
    g.drawRect(getLocalBounds(), 1);
}

void CalibrationPanel::resized()
{
    auto bounds = getLocalBounds().reduced(10);

    // Row 1: Calib Knob
    auto row1 = bounds.removeFromTop(80);
    calibLevelLabel.setBounds(row1.removeFromTop(15));
    calibLevelSlider.setBounds(row1);
    bounds.removeFromTop(10);

    // Row 2: Buttons - sized to fit text
    auto row2 = bounds.removeFromTop(25);
    analyzeButton.setBounds(row2.removeFromLeft(55));  // Fits "LEARN"
    row2.removeFromLeft(5);
    normalizeButton.setBounds(row2.removeFromLeft(50));  // Fits "APPLY"
    row2.removeFromLeft(5);
    resetNormButton.setBounds(row2.removeFromLeft(50));  // Fits "RESET"
    bounds.removeFromTop(10);

    // Row 3: Input Peak
    inputPeakLabel.setBounds(bounds.removeFromTop(25));
    bounds.removeFromTop(5);

    // Row 4: Gain Applied
    auto row4 = bounds.removeFromTop(25);
    gainAppliedLabel.setBounds(row4.removeFromLeft(95));
    gainAppliedValue.setBounds(row4);
}

void CalibrationPanel::setVisible(bool shouldBeVisible)
{
    Component::setVisible(shouldBeVisible);
    if (shouldBeVisible)
    {
        setSize(getWidth(), 200);  // Expanded height
    }
    else
    {
        setSize(getWidth(), 0);  // Collapsed height
    }
}

void CalibrationPanel::updateInputPeakDisplay(double peakDB)
{
    if (peakDB > -60.0)
        inputPeakLabel.setText("INPUT PEAK: " + juce::String(peakDB, 1) + " dB", juce::dontSendNotification);
    else
        inputPeakLabel.setText("INPUT PEAK: -- dB", juce::dontSendNotification);
}

void CalibrationPanel::updateGainAppliedDisplay(double gainDB)
{
    if (std::abs(gainDB) > 0.01)
        gainAppliedValue.setText(juce::String(gainDB, 1) + " dB");
    else
        gainAppliedValue.setText("-- dB");
}

// =============================================================================
// Advanced Panel Implementation
// =============================================================================

AdvancedPanel::AdvancedPanel(juce::AudioProcessorValueTreeState& apvts, QuadBlendDriveAudioProcessor& p)
    : apvts(apvts), processor(p)
{
    startTimerHz(15);  // Update colors periodically
    // Configure section labels
    softClipLabel.setText("SOFT CLIP", juce::dontSendNotification);
    softClipLabel.setFont(juce::Font(10.0f, juce::Font::bold));
    softClipLabel.setColour(juce::Label::textColourId, juce::Colour(255, 150, 60));  // Orange for Soft Clip
    addAndMakeVisible(softClipLabel);

    limitersLabel.setText("SLOW LIMITER", juce::dontSendNotification);
    limitersLabel.setFont(juce::Font(10.0f, juce::Font::bold));
    limitersLabel.setColour(juce::Label::textColourId, juce::Colour(255, 220, 100));  // Yellow for Slow Limit
    addAndMakeVisible(limitersLabel);

    compLabel.setText("GAIN COMPENSATION", juce::dontSendNotification);
    compLabel.setFont(juce::Font(10.0f, juce::Font::bold));
    compLabel.setColour(juce::Label::textColourId, juce::Colour(180, 180, 190));
    addAndMakeVisible(compLabel);

    // Soft Clip controls
    scKneeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    scKneeSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 22);  // Increased width for better value visibility
    addAndMakeVisible(scKneeSlider);
    scKneeLabel.setText("Knee:", juce::dontSendNotification);
    scKneeLabel.setFont(juce::Font(10.0f));
    scKneeLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.8f));
    addAndMakeVisible(scKneeLabel);
    scKneeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "SC_KNEE", scKneeSlider);

    // Slow Limiter controls
    limitRelSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    limitRelSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 22);
    addAndMakeVisible(limitRelSlider);
    limitRelLabel.setText("Release Scale:", juce::dontSendNotification);
    limitRelLabel.setFont(juce::Font(10.0f));
    limitRelLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.8f));
    addAndMakeVisible(limitRelLabel);
    limitRelAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "LIMIT_REL", limitRelSlider);

    slLimitAttackSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    slLimitAttackSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 22);
    addAndMakeVisible(slLimitAttackSlider);
    slLimitAttackLabel.setText("Attack Time:", juce::dontSendNotification);
    slLimitAttackLabel.setFont(juce::Font(10.0f));
    slLimitAttackLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.8f));
    addAndMakeVisible(slLimitAttackLabel);
    slLimitAttackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "SL_LIMIT_ATTACK", slLimitAttackSlider);

    // Fast Limiter controls
    flLimitAttackSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    flLimitAttackSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 22);
    addAndMakeVisible(flLimitAttackSlider);
    flLimitAttackLabel.setText("FAST LIMITER", juce::dontSendNotification);
    flLimitAttackLabel.setFont(juce::Font(10.0f, juce::Font::bold));
    flLimitAttackLabel.setColour(juce::Label::textColourId, juce::Colour(80, 160, 255));  // Blue for Fast Limit
    addAndMakeVisible(flLimitAttackLabel);
    flLimitAttackTimeLabel.setText("Attack:", juce::dontSendNotification);
    flLimitAttackTimeLabel.setFont(juce::Font(10.0f));
    flLimitAttackTimeLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.8f));
    addAndMakeVisible(flLimitAttackTimeLabel);
    flLimitAttackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "FL_LIMIT_ATTACK", flLimitAttackSlider);

    flLimitReleaseSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    flLimitReleaseSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 22);
    addAndMakeVisible(flLimitReleaseSlider);
    flLimitReleaseLabel.setText("Release:", juce::dontSendNotification);
    flLimitReleaseLabel.setFont(juce::Font(10.0f));
    flLimitReleaseLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.8f));
    addAndMakeVisible(flLimitReleaseLabel);
    flLimitReleaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "FL_LIMIT_RELEASE", flLimitReleaseSlider);

    // Gain Compensation controls
    hcCompButton.setButtonText("Hard");
    hcCompButton.setColour(juce::ToggleButton::textColourId, juce::Colours::white);
    addAndMakeVisible(hcCompButton);
    hcCompAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, "HC_COMP", hcCompButton);

    scCompButton.setButtonText("Soft");
    scCompButton.setColour(juce::ToggleButton::textColourId, juce::Colours::white);
    addAndMakeVisible(scCompButton);
    scCompAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, "SC_COMP", scCompButton);

    slCompButton.setButtonText("Slow");
    slCompButton.setColour(juce::ToggleButton::textColourId, juce::Colours::white);
    addAndMakeVisible(slCompButton);
    slCompAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, "SL_COMP", slCompButton);

    flCompButton.setButtonText("Fast");
    flCompButton.setColour(juce::ToggleButton::textColourId, juce::Colours::white);
    addAndMakeVisible(flCompButton);
    flCompAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, "FL_COMP", flCompButton);

    // === ENVELOPE SHAPING CONTROLS ===
    envelopeLabel.setText("ENVELOPE SHAPING (PUNCH / SUSTAIN EMPHASIS)", juce::dontSendNotification);
    envelopeLabel.setFont(juce::Font(10.0f, juce::Font::bold));
    envelopeLabel.setColour(juce::Label::textColourId, juce::Colour(180, 180, 190));
    addAndMakeVisible(envelopeLabel);

    // Helper lambda to setup envelope sliders
    auto setupEnvelopeSlider = [this](juce::Slider& slider, juce::Label& label, const juce::String& text) {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 18);
        slider.setDoubleClickReturnValue(true, 0.0);  // Double-click resets to 0 dB
        addAndMakeVisible(slider);

        label.setText(text, juce::dontSendNotification);
        label.setFont(juce::Font(9.0f));
        label.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.7f));
        label.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(label);
    };

    // Hard Clip - Punch/Sustain emphasis
    setupEnvelopeSlider(hcAttackSlider, hcAttackLabel, "HARD CLIP");
    setupEnvelopeSlider(hcSustainSlider, hcSustainLabel, "Sustain");
    hcAttackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "HC_ATTACK", hcAttackSlider);
    hcSustainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "HC_SUSTAIN", hcSustainSlider);

    // Soft Clip - Punch/Sustain emphasis
    setupEnvelopeSlider(scAttackSlider, scAttackLabel, "SOFT CLIP");
    setupEnvelopeSlider(scSustainSlider, scSustainLabel, "Sustain");
    scAttackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "SC_ATTACK", scAttackSlider);
    scSustainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "SC_SUSTAIN", scSustainSlider);

    // Slow Limit - Punch/Sustain emphasis
    setupEnvelopeSlider(slAttackSlider, slAttackLabel, "SLOW LIMIT");
    setupEnvelopeSlider(slSustainSlider, slSustainLabel, "Sustain");
    slAttackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "SL_ATTACK", slAttackSlider);
    slSustainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "SL_SUSTAIN", slSustainSlider);

    // Fast Limit - Punch/Sustain emphasis
    setupEnvelopeSlider(flAttackSlider, flAttackLabel, "FAST LIMIT");
    setupEnvelopeSlider(flSustainSlider, flSustainLabel, "Sustain");
    flAttackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "FL_ATTACK", flAttackSlider);
    flSustainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "FL_SUSTAIN", flSustainSlider);

    setVisible(false);  // Start hidden
}

void AdvancedPanel::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Dark background with subtle gradient
    juce::ColourGradient gradient(juce::Colour(28, 28, 32), 0, 0,
                                   juce::Colour(22, 22, 26), 0, bounds.getHeight(), false);
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(bounds, 6.0f);

    // Subtle border
    g.setColour(juce::Colour(60, 60, 65));
    g.drawRoundedRectangle(bounds, 6.0f, 1.0f);
}

void AdvancedPanel::resized()
{
    auto bounds = getLocalBounds().reduced(15);
    const int panelWidth = bounds.getWidth();
    const int panelHeight = bounds.getHeight();

    const int headerHeight = 18;
    const int sliderRowHeight = 24;
    const int knobSize = 42;
    const int knobWithText = knobSize + 22;
    const int sectionGap = 15;

    int y = bounds.getY();

    // ========== ROW 1: SOFT CLIP | SLOW LIMITER | FAST LIMITER ==========
    // Three columns for processor-specific settings
    const int colGap = 20;
    const int colWidth = (panelWidth - colGap * 2) / 3;
    const int col1 = bounds.getX();
    const int col2 = col1 + colWidth + colGap;
    const int col3 = col2 + colWidth + colGap;
    const int labelW = 55;
    const int sliderW = colWidth - labelW - 5;

    // SOFT CLIP column
    softClipLabel.setBounds(col1, y, colWidth, headerHeight);
    scKneeLabel.setBounds(col1, y + headerHeight + 4, labelW, sliderRowHeight);
    scKneeSlider.setBounds(col1 + labelW, y + headerHeight + 4, sliderW, sliderRowHeight);

    // SLOW LIMITER column
    limitersLabel.setBounds(col2, y, colWidth, headerHeight);
    slLimitAttackLabel.setBounds(col2, y + headerHeight + 4, labelW, sliderRowHeight);
    slLimitAttackSlider.setBounds(col2 + labelW, y + headerHeight + 4, sliderW, sliderRowHeight);
    limitRelLabel.setBounds(col2, y + headerHeight + 4 + sliderRowHeight + 2, labelW, sliderRowHeight);
    limitRelSlider.setBounds(col2 + labelW, y + headerHeight + 4 + sliderRowHeight + 2, sliderW, sliderRowHeight);

    // FAST LIMITER column
    flLimitAttackLabel.setBounds(col3, y, colWidth, headerHeight);
    flLimitAttackTimeLabel.setBounds(col3, y + headerHeight + 4, labelW, sliderRowHeight);
    flLimitAttackSlider.setBounds(col3 + labelW, y + headerHeight + 4, sliderW, sliderRowHeight);
    flLimitReleaseLabel.setBounds(col3, y + headerHeight + 4 + sliderRowHeight + 2, labelW, sliderRowHeight);
    flLimitReleaseSlider.setBounds(col3 + labelW, y + headerHeight + 4 + sliderRowHeight + 2, sliderW, sliderRowHeight);

    y += headerHeight + 4 + sliderRowHeight * 2 + 2 + sectionGap;

    // ========== ROW 2: ENVELOPE SHAPING (4 across) ==========
    envelopeLabel.setBounds(bounds.getX(), y, panelWidth, headerHeight);
    y += headerHeight + 6;

    // Four columns for envelope knob pairs
    // Each column: Header (processor name), then two knobs (Punch | Sustain) with small labels
    const int envColGap = 8;
    const int envColWidth = (panelWidth - envColGap * 3) / 4;
    const int subLabelHeight = 12;

    auto layoutEnvKnobs = [&](int x, juce::Label& headerLabel, juce::Slider& punchKnob, juce::Slider& susKnob, juce::Label& susLabel) {
        // Header (processor name)
        headerLabel.setBounds(x, y, envColWidth, 14);

        // Position knobs side by side with small labels underneath header
        int knobX = x + (envColWidth - (knobSize * 2 + 8)) / 2;

        // "Punch" and "Sustain" sub-labels
        susLabel.setText("Punch      Sustain", juce::dontSendNotification);
        susLabel.setFont(juce::Font(8.0f));
        susLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.5f));
        susLabel.setJustificationType(juce::Justification::centred);
        susLabel.setBounds(x, y + 14, envColWidth, subLabelHeight);

        // Knobs below the sub-labels
        punchKnob.setBounds(knobX, y + 14 + subLabelHeight, knobSize, knobWithText);
        susKnob.setBounds(knobX + knobSize + 8, y + 14 + subLabelHeight, knobSize, knobWithText);
    };

    int ex = bounds.getX();
    layoutEnvKnobs(ex, hcAttackLabel, hcAttackSlider, hcSustainSlider, hcSustainLabel);
    ex += envColWidth + envColGap;
    layoutEnvKnobs(ex, flAttackLabel, flAttackSlider, flSustainSlider, flSustainLabel);
    ex += envColWidth + envColGap;
    layoutEnvKnobs(ex, scAttackLabel, scAttackSlider, scSustainSlider, scSustainLabel);
    ex += envColWidth + envColGap;
    layoutEnvKnobs(ex, slAttackLabel, slAttackSlider, slSustainSlider, slSustainLabel);

    y += 14 + subLabelHeight + knobWithText + sectionGap;

    // ========== ROW 3: GAIN COMPENSATION ==========
    const int compBtnW = 60;
    const int compBtnH = 24;
    const int compGap = 10;

    compLabel.setBounds(bounds.getX(), y, 140, headerHeight);
    int cx = bounds.getX() + 150;
    hcCompButton.setBounds(cx, y - 3, compBtnW, compBtnH);
    cx += compBtnW + compGap;
    scCompButton.setBounds(cx, y - 3, compBtnW, compBtnH);
    cx += compBtnW + compGap;
    slCompButton.setBounds(cx, y - 3, compBtnW, compBtnH);
    cx += compBtnW + compGap;
    flCompButton.setBounds(cx, y - 3, compBtnW, compBtnH);
}

void AdvancedPanel::setVisible(bool shouldBeVisible)
{
    Component::setVisible(shouldBeVisible);
}

AdvancedPanel::~AdvancedPanel()
{
    stopTimer();
}

void AdvancedPanel::timerCallback()
{
    // Update envelope slider colors to match processor colors
    hcAttackSlider.setColour(juce::Slider::thumbColourId, processor.processorColors.hardClip);
    hcSustainSlider.setColour(juce::Slider::thumbColourId, processor.processorColors.hardClip);
    scAttackSlider.setColour(juce::Slider::thumbColourId, processor.processorColors.softClip);
    scSustainSlider.setColour(juce::Slider::thumbColourId, processor.processorColors.softClip);
    slAttackSlider.setColour(juce::Slider::thumbColourId, processor.processorColors.slowLimit);
    slSustainSlider.setColour(juce::Slider::thumbColourId, processor.processorColors.slowLimit);
    flAttackSlider.setColour(juce::Slider::thumbColourId, processor.processorColors.fastLimit);
    flSustainSlider.setColour(juce::Slider::thumbColourId, processor.processorColors.fastLimit);

    // Update horizontal sliders with accent color
    scKneeSlider.setColour(juce::Slider::thumbColourId, processor.processorColors.accent);
    limitRelSlider.setColour(juce::Slider::thumbColourId, processor.processorColors.accent);
    slLimitAttackSlider.setColour(juce::Slider::thumbColourId, processor.processorColors.accent);
    flLimitAttackSlider.setColour(juce::Slider::thumbColourId, processor.processorColors.accent);

    // Update comp button text colors to match processor colors
    hcCompButton.setColour(juce::ToggleButton::textColourId, processor.processorColors.hardClip);
    scCompButton.setColour(juce::ToggleButton::textColourId, processor.processorColors.softClip);
    slCompButton.setColour(juce::ToggleButton::textColourId, processor.processorColors.slowLimit);
    flCompButton.setColour(juce::ToggleButton::textColourId, processor.processorColors.fastLimit);

    // Update section labels with processor colors
    softClipLabel.setColour(juce::Label::textColourId, processor.processorColors.softClip);
    limitersLabel.setColour(juce::Label::textColourId, processor.processorColors.slowLimit);
}

//==============================================================================
// Preferences Panel Implementation
//==============================================================================
PreferencesPanel::PreferencesPanel(QuadBlendDriveAudioProcessor& p)
    : processor(p)
{
    // Title
    titleLabel.setText("Processor Colors", juce::dontSendNotification);
    titleLabel.setFont(juce::FontOptions(14.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.9f));
    addAndMakeVisible(titleLabel);

    // Color swatches
    hardClipColor.onColorChanged = [this](juce::Colour c) {
        processor.processorColors.hardClip = c;
    };
    addAndMakeVisible(hardClipColor);

    softClipColor.onColorChanged = [this](juce::Colour c) {
        processor.processorColors.softClip = c;
    };
    addAndMakeVisible(softClipColor);

    slowLimitColor.onColorChanged = [this](juce::Colour c) {
        processor.processorColors.slowLimit = c;
    };
    addAndMakeVisible(slowLimitColor);

    fastLimitColor.onColorChanged = [this](juce::Colour c) {
        processor.processorColors.fastLimit = c;
    };
    addAndMakeVisible(fastLimitColor);

    accentColor.onColorChanged = [this](juce::Colour c) {
        processor.processorColors.accent = c;
    };
    addAndMakeVisible(accentColor);

    // Reset button
    resetButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2a2a2a));
    resetButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.7f));
    resetButton.onClick = [this]() {
        processor.processorColors.resetToDefaults();
        syncFromProcessor();
    };
    addAndMakeVisible(resetButton);

    // Sync colors from processor on construction
    syncFromProcessor();

    setSize(200, 180);
}

void PreferencesPanel::paint(juce::Graphics& g)
{
    // Panel background with gradient
    juce::ColourGradient gradient(juce::Colour(28, 28, 32), 0, 0,
                                   juce::Colour(22, 22, 26), 0, static_cast<float>(getHeight()), false);
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 8.0f);

    // Border
    g.setColour(juce::Colour(60, 60, 65));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 8.0f, 1.0f);
}

void PreferencesPanel::resized()
{
    auto bounds = getLocalBounds().reduced(12);
    const int rowHeight = 24;
    const int spacing = 6;

    titleLabel.setBounds(bounds.removeFromTop(rowHeight));
    bounds.removeFromTop(spacing);

    hardClipColor.setBounds(bounds.removeFromTop(rowHeight));
    bounds.removeFromTop(spacing);

    softClipColor.setBounds(bounds.removeFromTop(rowHeight));
    bounds.removeFromTop(spacing);

    slowLimitColor.setBounds(bounds.removeFromTop(rowHeight));
    bounds.removeFromTop(spacing);

    fastLimitColor.setBounds(bounds.removeFromTop(rowHeight));
    bounds.removeFromTop(spacing);

    accentColor.setBounds(bounds.removeFromTop(rowHeight));
    bounds.removeFromTop(spacing + 4);

    resetButton.setBounds(bounds.removeFromTop(22));
}

void PreferencesPanel::setVisible(bool shouldBeVisible)
{
    if (shouldBeVisible)
        syncFromProcessor();
    Component::setVisible(shouldBeVisible);
}

void PreferencesPanel::syncFromProcessor()
{
    hardClipColor.setColor(processor.processorColors.hardClip);
    softClipColor.setColor(processor.processorColors.softClip);
    slowLimitColor.setColor(processor.processorColors.slowLimit);
    fastLimitColor.setColor(processor.processorColors.fastLimit);
    accentColor.setColor(processor.processorColors.accent);
}

//==============================================================================
// Editor Implementation
//==============================================================================
QuadBlendDriveAudioProcessorEditor::QuadBlendDriveAudioProcessorEditor(QuadBlendDriveAudioProcessor& p)
    : AudioProcessorEditor(&p),
      audioProcessor(p),
      xyPad(p.apvts, "XY_X_PARAM", "XY_Y_PARAM", p),
      calibrationPanel(p.apvts, p),
      advancedPanel(p.apvts, p),
      preferencesPanel(p),
      steveScope(p),
      transferCurveMeter(p),  // Transfer curve with level meter
      inputMeter(p, true),    // Stereo input meter
      outputMeter(p, false)   // Stereo output meter
{
    // Apply custom look and feel
    setLookAndFeel(&lookAndFeel);

    setSize(1000, 610);  // Default size - compact, scope/advanced panels expand below
    setResizable(true, true);
    setResizeLimits(800, 610, 1900, 1200);  // Min 800x610, Max with panels expanded

    // Setup XY Pad
    addAndMakeVisible(xyPad);

    // Setup Visualizations (these go first so panels can overlay)
    addChildComponent(steveScope);          // Hidden by default - shown when SCOPE is clicked
    addChildComponent(transferCurveMeter);  // Transfer curve with level meter (hidden by default)
    addAndMakeVisible(inputMeter);          // Stereo input meter
    addAndMakeVisible(outputMeter);         // Stereo output meter

    // Setup Transfer Curve Toggle Button
    transferCurveToggleButton.setButtonText("TRANSFER CURVE");
    transferCurveToggleButton.setClickingTogglesState(false);
    transferCurveToggleButton.setColour(juce::TextButton::buttonColourId, juce::Colour(35, 35, 40));
    transferCurveToggleButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.85f));
    transferCurveToggleButton.onClick = [this]() {
        transferCurveMeter.setVisible(!transferCurveMeter.isVisible());
        resized();  // Re-layout when panel visibility changes
    };
    addAndMakeVisible(transferCurveToggleButton);

    // Setup Learn Panel (Calibration)
    calibrationToggleButton.setButtonText("LEARN");
    calibrationToggleButton.setClickingTogglesState(false);
    calibrationToggleButton.setColour(juce::TextButton::buttonColourId, juce::Colour(35, 35, 40));
    calibrationToggleButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.85f));
    addAndMakeVisible(calibrationToggleButton);

    calibrationToggleButton.onClick = [this]() {
        calibrationPanel.setVisible(!calibrationPanel.isVisible());
        resized();  // Re-layout when panel visibility changes
    };
    // Note: Button text and colors are dynamically updated in timerCallback() based on normalization state
    // CalibrationPanel added at end of constructor to appear on top

    // === TOOLBAR COMPONENTS ===
    // Preset Menu Button (dropdown for preset management)
    presetMenuButton.setButtonText("Default Setting");
    presetMenuButton.setColour(juce::TextButton::buttonColourId, juce::Colour(35, 35, 40));
    presetMenuButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.85f));
    presetMenuButton.onClick = [this]()
    {
        juce::PopupMenu menu;

        // Initialize to Default
        menu.addItem(1, "Initialize to Default");
        menu.addSeparator();

        // Save/Load
        menu.addItem(2, "Save Preset...");
        menu.addItem(3, "Save Preset As...");
        menu.addItem(4, "Load Preset...");
        menu.addSeparator();

        // Copy/Paste State
        menu.addItem(5, "Copy State");
        menu.addItem(6, "Paste State");
        menu.addSeparator();

        // A/B Compare submenu
        juce::PopupMenu abMenu;
        abMenu.addItem(10, "Copy to A");
        abMenu.addItem(11, "Copy to B");
        abMenu.addItem(12, "Copy to C");
        abMenu.addItem(13, "Copy to D");
        abMenu.addSeparator();
        abMenu.addItem(14, "Swap A <-> B");
        abMenu.addItem(15, "Swap C <-> D");
        menu.addSubMenu("A/B Compare", abMenu);

        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&presetMenuButton),
            [this](int result)
            {
                switch (result)
                {
                    case 1: // Initialize to Default
                    {
                        // Reset all parameters to default values
                        for (auto* param : audioProcessor.apvts.processor.getParameters())
                        {
                            if (auto* rangedParam = dynamic_cast<juce::RangedAudioParameter*>(param))
                                rangedParam->setValueNotifyingHost(rangedParam->getDefaultValue());
                        }
                        presetMenuButton.setButtonText("Default Setting");
                        currentPresetPath = juce::File();
                        break;
                    }
                    case 2: // Save Preset
                    case 3: // Save Preset As
                    {
                        bool saveAs = (result == 3) || !currentPresetPath.existsAsFile();
                        if (saveAs)
                        {
                            fileChooser = std::make_unique<juce::FileChooser>(
                                "Save Preset",
                                juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
                                "*.bjpreset");

                            fileChooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                                [this](const juce::FileChooser& fc)
                                {
                                    auto file = fc.getResult();
                                    if (file != juce::File())
                                    {
                                        auto state = audioProcessor.apvts.copyState();
                                        auto xml = state.createXml();
                                        if (xml != nullptr)
                                        {
                                            auto fileToWrite = file.withFileExtension(".bjpreset");
                                            xml->writeTo(fileToWrite);
                                            currentPresetPath = fileToWrite;
                                            presetMenuButton.setButtonText(fileToWrite.getFileNameWithoutExtension());
                                        }
                                    }
                                });
                        }
                        else
                        {
                            auto state = audioProcessor.apvts.copyState();
                            auto xml = state.createXml();
                            if (xml != nullptr)
                                xml->writeTo(currentPresetPath);
                        }
                        break;
                    }
                    case 4: // Load Preset
                    {
                        fileChooser = std::make_unique<juce::FileChooser>(
                            "Load Preset",
                            juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
                            "*.bjpreset");

                        fileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                            [this](const juce::FileChooser& fc)
                            {
                                auto file = fc.getResult();
                                if (file.existsAsFile())
                                {
                                    auto xml = juce::XmlDocument::parse(file);
                                    if (xml != nullptr)
                                    {
                                        auto state = juce::ValueTree::fromXml(*xml);
                                        if (state.isValid())
                                        {
                                            audioProcessor.apvts.replaceState(state);
                                            currentPresetPath = file;
                                            presetMenuButton.setButtonText(file.getFileNameWithoutExtension());
                                        }
                                    }
                                }
                            });
                        break;
                    }
                    case 5: // Copy State
                    {
                        auto state = audioProcessor.apvts.copyState();
                        auto xml = state.createXml();
                        if (xml != nullptr)
                            juce::SystemClipboard::copyTextToClipboard(xml->toString());
                        break;
                    }
                    case 6: // Paste State
                    {
                        auto clipText = juce::SystemClipboard::getTextFromClipboard();
                        auto xml = juce::XmlDocument::parse(clipText);
                        if (xml != nullptr)
                        {
                            auto state = juce::ValueTree::fromXml(*xml);
                            if (state.isValid())
                            {
                                audioProcessor.apvts.replaceState(state);
                                presetMenuButton.setButtonText("(Pasted)");
                                currentPresetPath = juce::File();
                            }
                        }
                        break;
                    }
                    case 10: audioProcessor.savePreset(0); break;  // Copy to A
                    case 11: audioProcessor.savePreset(1); break;  // Copy to B
                    case 12: audioProcessor.savePreset(2); break;  // Copy to C
                    case 13: audioProcessor.savePreset(3); break;  // Copy to D
                    case 14: // Swap A <-> B
                    {
                        auto tempA = audioProcessor.hasPreset(0) ? juce::ValueTree() : juce::ValueTree();
                        // Save current A, copy B to A, restore old A to B
                        juce::ValueTree slotA, slotB;
                        if (audioProcessor.hasPreset(0))
                        {
                            audioProcessor.recallPreset(0);
                            slotA = audioProcessor.apvts.copyState();
                        }
                        if (audioProcessor.hasPreset(1))
                        {
                            audioProcessor.recallPreset(1);
                            slotB = audioProcessor.apvts.copyState();
                        }
                        // Now swap
                        if (slotB.isValid()) { audioProcessor.apvts.replaceState(slotB); audioProcessor.savePreset(0); }
                        if (slotA.isValid()) { audioProcessor.apvts.replaceState(slotA); audioProcessor.savePreset(1); }
                        break;
                    }
                    case 15: // Swap C <-> D
                    {
                        juce::ValueTree slotC, slotD;
                        if (audioProcessor.hasPreset(2))
                        {
                            audioProcessor.recallPreset(2);
                            slotC = audioProcessor.apvts.copyState();
                        }
                        if (audioProcessor.hasPreset(3))
                        {
                            audioProcessor.recallPreset(3);
                            slotD = audioProcessor.apvts.copyState();
                        }
                        if (slotD.isValid()) { audioProcessor.apvts.replaceState(slotD); audioProcessor.savePreset(2); }
                        if (slotC.isValid()) { audioProcessor.apvts.replaceState(slotC); audioProcessor.savePreset(3); }
                        break;
                    }
                    default:
                        break;
                }
            });
    };
    addAndMakeVisible(presetMenuButton);

    // Undo Button
    undoButton.setButtonText("Undo");
    undoButton.setColour(juce::TextButton::buttonColourId, juce::Colour(35, 35, 40));
    undoButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.5f));
    undoButton.onClick = [this]()
    {
        if (audioProcessor.undoManager.canUndo())
        {
            audioProcessor.undoManager.undo();
        }
    };
    addAndMakeVisible(undoButton);

    // Redo Button
    redoButton.setButtonText("Redo");
    redoButton.setColour(juce::TextButton::buttonColourId, juce::Colour(35, 35, 40));
    redoButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.5f));
    redoButton.onClick = [this]()
    {
        if (audioProcessor.undoManager.canRedo())
        {
            audioProcessor.undoManager.redo();
        }
    };
    addAndMakeVisible(redoButton);

    // Help Button - shows quick reference
    helpButton.setButtonText("?");
    helpButton.setColour(juce::TextButton::buttonColourId, juce::Colour(35, 35, 40));
    helpButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.85f));
    helpButton.onClick = [this]()
    {
        juce::PopupMenu helpMenu;
        helpMenu.addSectionHeader("Emulsion - Quick Reference");
        helpMenu.addSeparator();

        // XY Pad section
        juce::PopupMenu xyMenu;
        xyMenu.addItem(100, "HC (Hard Clip) - Top Left", false, false);
        xyMenu.addItem(101, "FL (Fast Limit) - Top Right", false, false);
        xyMenu.addItem(102, "SC (Soft Clip) - Bottom Left", false, false);
        xyMenu.addItem(103, "SL (Slow Limit) - Bottom Right", false, false);
        xyMenu.addSeparator();
        xyMenu.addItem(104, "Double-click: Reset to center", false, false);
        xyMenu.addItem(105, "Cmd+drag: Temporary delta mode", false, false);
        helpMenu.addSubMenu("XY Pad", xyMenu);

        // Quick Presets section
        juce::PopupMenu presetMenu;
        presetMenu.addItem(200, "Click empty slot: Save current settings", false, false);
        presetMenu.addItem(201, "Click saved slot: Recall preset", false, false);
        presetMenu.addItem(202, "Cmd+Click: Overwrite slot", false, false);
        presetMenu.addItem(203, "Opt+Click: Clear slot", false, false);
        helpMenu.addSubMenu("A/B/C/D Presets", presetMenu);

        // Controls section
        juce::PopupMenu controlsMenu;
        controlsMenu.addItem(300, "Input: Pre-saturation gain", false, false);
        controlsMenu.addItem(301, "Threshold: Saturation onset level", false, false);
        controlsMenu.addItem(302, "Output: Post-saturation gain", false, false);
        controlsMenu.addItem(303, "Mix: Dry/Wet blend", false, false);
        controlsMenu.addItem(304, "Ceiling: Output limiter ceiling", false, false);
        controlsMenu.addSeparator();
        controlsMenu.addItem(305, "LINK I/O: Gangs input/output knobs", false, false);
        controlsMenu.addItem(306, "BYPASS: True bypass", false, false);
        controlsMenu.addItem(307, "DELTA: Hear only the saturation", false, false);
        helpMenu.addSubMenu("Controls", controlsMenu);

        // Processors section
        juce::PopupMenu procMenu;
        procMenu.addItem(400, "HC: Hard clipper (aggressive)", false, false);
        procMenu.addItem(401, "SC: Soft clipper (smooth)", false, false);
        procMenu.addItem(402, "SL: Slow limiter (pumping)", false, false);
        procMenu.addItem(403, "FL: Fast limiter (transparent)", false, false);
        procMenu.addSeparator();
        procMenu.addItem(404, "M button: Mute processor", false, false);
        procMenu.addItem(405, "S button: Solo processor", false, false);
        procMenu.addItem(406, "Trim knobs: Per-processor gain", false, false);
        helpMenu.addSubMenu("Processors", procMenu);

        // Engine modes section
        juce::PopupMenu engineMenu;
        engineMenu.addItem(500, "Zero Latency: No latency, basic quality", false, false);
        engineMenu.addItem(501, "Balanced: Low latency, good quality", false, false);
        engineMenu.addItem(502, "Linear Phase: Higher latency, best quality", false, false);
        helpMenu.addSubMenu("Engine Modes", engineMenu);

        // Learn section
        juce::PopupMenu learnMenu;
        learnMenu.addItem(600, "LEARN: Analyze input level", false, false);
        learnMenu.addItem(601, "APPLY: Apply gain compensation", false, false);
        learnMenu.addItem(602, "RESET: Clear learned data", false, false);
        helpMenu.addSubMenu("Learn Panel", learnMenu);

        helpMenu.addSeparator();
        helpMenu.addItem(900, "Version: " + kPluginVersion);

        helpMenu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&helpButton));
    };
    addAndMakeVisible(helpButton);

    // Preferences Button - shows processor color settings
    preferencesButton.setButtonText("COLORS");
    preferencesButton.setColour(juce::TextButton::buttonColourId, juce::Colour(35, 35, 40));
    preferencesButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.85f));
    preferencesButton.onClick = [this]()
    {
        // Toggle preferences panel visibility
        bool shouldShow = !preferencesPanel.isVisible();
        preferencesPanel.setVisible(shouldShow);

        // Position the panel near the button and bring to front
        if (shouldShow)
        {
            auto buttonBounds = preferencesButton.getBoundsInParent();
            preferencesPanel.setBounds(buttonBounds.getX() - 80, buttonBounds.getBottom() + 5, 200, 210);
            preferencesPanel.toFront(false);  // Bring to front so it shows on top
        }
    };
    addAndMakeVisible(preferencesButton);

    // Add preferences panel (initially hidden - must start closed)
    preferencesPanel.setVisible(false);
    addChildComponent(preferencesPanel);  // Use addChildComponent so it starts hidden

    // Scope Toggle Button - expands window to show oscilloscope below 600px
    scopeToggleButton.setButtonText("SCOPE");
    scopeToggleButton.setClickingTogglesState(true);
    scopeToggleButton.setToggleState(false, juce::dontSendNotification);  // Scope hidden by default
    scopeToggleButton.setColour(juce::TextButton::buttonColourId, juce::Colour(35, 35, 40));
    scopeToggleButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(50, 150, 255));
    scopeToggleButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.6f));
    scopeToggleButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    scopeToggleButton.onClick = [this]() {
        const int baseHeight = 610;
        const int scopeHeight = 300;  // Height added for scope

        if (scopeToggleButton.getToggleState())
        {
            // Expand window to show scope
            steveScope.setVisible(true);
            setSize(getWidth(), baseHeight + scopeHeight);
        }
        else
        {
            // Collapse window, hide scope
            steveScope.setVisible(false);
            // Only shrink if advanced panel is also hidden
            if (!advancedPanel.isVisible())
                setSize(getWidth(), baseHeight);
        }
        resized();
    };
    addAndMakeVisible(scopeToggleButton);

    // Setup Advanced Panel - expands window to show panel below 600px
    advancedToggleButton.setButtonText("ADVANCED");
    advancedToggleButton.setClickingTogglesState(true);
    advancedToggleButton.setToggleState(false, juce::dontSendNotification);
    advancedToggleButton.setColour(juce::TextButton::buttonColourId, juce::Colour(35, 35, 40));
    advancedToggleButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(50, 150, 255));
    advancedToggleButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.6f));
    advancedToggleButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    advancedToggleButton.onClick = [this]() {
        const int baseHeight = 610;
        const int advancedHeight = 290;  // Height to fit panel snugly (280 scaled + padding)

        if (advancedToggleButton.getToggleState())
        {
            // Expand window to show advanced panel
            advancedPanel.setVisible(true);
            setSize(getWidth(), baseHeight + advancedHeight);
        }
        else
        {
            // Collapse window, hide advanced panel
            advancedPanel.setVisible(false);
            // Only shrink if scope is also hidden
            if (!steveScope.isVisible())
                setSize(getWidth(), baseHeight);
        }
        resized();
    };
    addAndMakeVisible(advancedToggleButton);
    addChildComponent(advancedPanel);  // Use addChildComponent to keep panel hidden initially

    // Setup Sliders - Global Controls (proportional sizing)
    auto setupRotarySlider = [this](juce::Slider& slider, juce::Label& label, const juce::String& text)
    {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 68, 20);
        addAndMakeVisible(slider);

        label.setText(text, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centredLeft);
        label.setFont(juce::Font(14.0f));  // Larger font to match button text
        // Don't attach - we'll position manually
        addAndMakeVisible(label);
    };

    setupRotarySlider(inputGainSlider, inputGainLabel, "Input");
    setupRotarySlider(outputGainSlider, outputGainLabel, "Output");
    setupRotarySlider(mixSlider, mixLabel, "Mix");

    // Threshold Control (Calib moved to CalibrationPanel)
    setupRotarySlider(thresholdSlider, thresholdLabel, "Threshold");

    // CAL Button - Links threshold to calibration level (auto-tracking)
    thresholdLinkButton.setButtonText("CAL");
    thresholdLinkButton.setClickingTogglesState(true);
    thresholdLinkButton.setToggleState(true, juce::dontSendNotification);  // Default ON
    thresholdLinkButton.setColour(juce::TextButton::buttonColourId, juce::Colour(35, 35, 40));
    thresholdLinkButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(255, 140, 60));  // Orange when linked (matches calibration)
    thresholdLinkButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.6f));
    thresholdLinkButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    addAndMakeVisible(thresholdLinkButton);

    // Gain Link Toggle - Consistent button styling
    gainLinkButton.setButtonText("LINK I/O");
    gainLinkButton.setClickingTogglesState(true);
    gainLinkButton.setToggleState(false, juce::dontSendNotification);
    gainLinkButton.setColour(juce::TextButton::buttonColourId, juce::Colour(35, 35, 40));
    gainLinkButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(255, 120, 50));
    gainLinkButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.85f));
    gainLinkButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
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

    // Trim Reset Button - Modern styling
    resetTrimsButton.setButtonText("RESET TRIMS");
    resetTrimsButton.setColour(juce::TextButton::buttonColourId, juce::Colour(35, 35, 40));
    resetTrimsButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.85f));
    resetTrimsButton.onClick = [this]()
    {
        hcTrimSlider.setValue(0.0, juce::sendNotificationSync);
        scTrimSlider.setValue(0.0, juce::sendNotificationSync);
        slTrimSlider.setValue(0.0, juce::sendNotificationSync);
        flTrimSlider.setValue(0.0, juce::sendNotificationSync);
    };
    addAndMakeVisible(resetTrimsButton);

    // Start timer for updating normalization display (now in CalibrationPanel)
    startTimerHz(10);  // 10 Hz update rate

    // Setup Trim Sliders (proportional to main knobs) with processor colors
    auto setupTrimSlider = [this](juce::Slider& slider, juce::Label& label, const juce::String& text, int processorIndex)
    {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 62, 18);
        // Set processor-specific color (will be updated in timer callback if colors change)
        slider.setColour(juce::Slider::thumbColourId, audioProcessor.processorColors.getColor(processorIndex));
        addAndMakeVisible(slider);

        label.setText(text, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centredLeft);
        label.setFont(juce::Font(14.0f));  // Larger font to match button text
        addAndMakeVisible(label);
    };

    setupTrimSlider(hcTrimSlider, hcTrimLabel, "Hard Clip", 0);
    setupTrimSlider(scTrimSlider, scTrimLabel, "Soft Clip", 1);
    setupTrimSlider(slTrimSlider, slTrimLabel, "Slow Limit", 2);
    setupTrimSlider(flTrimSlider, flTrimLabel, "Fast Limit", 3);

    // Setup Delta Mode Controls - Consistent button styling
    deltaModeButton.setButtonText("DELTA MODE");
    deltaModeButton.setClickingTogglesState(true);
    deltaModeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(35, 35, 40));
    deltaModeButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(255, 120, 50));
    deltaModeButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.85f));
    deltaModeButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);

    // Auto-sync Master Comp with Delta Mode
    deltaModeButton.onClick = [this]()
    {
        // Read current delta mode state (after toggle)
        auto* deltaModeParam = audioProcessor.apvts.getRawParameterValue("DELTA_MODE");
        bool deltaModeOn = deltaModeParam ? deltaModeParam->load() > 0.5f : false;

        // Sync Master Comp to match Delta Mode
        if (auto* masterCompParam = audioProcessor.apvts.getParameter("MASTER_COMP"))
            masterCompParam->setValueNotifyingHost(deltaModeOn ? 1.0f : 0.0f);
    };

    addAndMakeVisible(deltaModeButton);

    // O/S Delta button removed - functionality integrated into main delta mode

    // TP Delta disabled for now - too complex to phase-align with lookahead buffer
    // truePeakDeltaModeButton.setButtonText("TP DELTA");
    // addAndMakeVisible(truePeakDeltaModeButton);

    // Setup Bypass Toggle - Consistent button styling
    bypassButton.setButtonText("BYPASS");
    bypassButton.setClickingTogglesState(true);
    bypassButton.setColour(juce::TextButton::buttonColourId, juce::Colour(35, 35, 40));
    bypassButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(255, 80, 80));
    bypassButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.85f));
    bypassButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    addAndMakeVisible(bypassButton);

    // Setup Master Gain Compensation Toggle - Enables per-processor compensation
    masterCompButton.setButtonText("GAIN COMP");
    masterCompButton.setClickingTogglesState(true);
    masterCompButton.setColour(juce::TextButton::buttonColourId, juce::Colour(35, 35, 40));
    masterCompButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(80, 220, 120));
    masterCompButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.6f));
    masterCompButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    addAndMakeVisible(masterCompButton);

    // Setup Toggle Buttons for Muting - Modern styling
    auto setupMuteButton = [](juce::ToggleButton& button)
    {
        button.setButtonText("M");
        button.setClickingTogglesState(true);
        // OFF state (not muted): subtle dark background
        button.setColour(juce::ToggleButton::textColourId, juce::Colours::white.withAlpha(0.5f));
        // ON state (muted): orange with modern styling
        button.setColour(juce::ToggleButton::tickColourId, juce::Colour(255, 140, 0));  // Orange
        button.setColour(juce::ToggleButton::tickDisabledColourId, juce::Colour(255, 140, 0));
    };

    setupMuteButton(hcMuteButton);
    setupMuteButton(scMuteButton);
    setupMuteButton(slMuteButton);
    setupMuteButton(flMuteButton);

    addAndMakeVisible(hcMuteButton);
    addAndMakeVisible(scMuteButton);
    addAndMakeVisible(slMuteButton);
    addAndMakeVisible(flMuteButton);

    // Solo Buttons - Modern styling with yellow/gold accent
    auto setupSoloButton = [](juce::ToggleButton& button)
    {
        button.setButtonText("S");
        // OFF state (not soloed): subtle dark background
        button.setColour(juce::ToggleButton::textColourId, juce::Colours::white.withAlpha(0.5f));
        // ON state (soloed): gold/yellow with modern styling
        button.setColour(juce::ToggleButton::tickColourId, juce::Colour(255, 200, 50));
        button.setColour(juce::ToggleButton::tickDisabledColourId, juce::Colour(255, 200, 50));
    };

    setupSoloButton(hcSoloButton);
    setupSoloButton(scSoloButton);
    setupSoloButton(slSoloButton);
    setupSoloButton(flSoloButton);

    // Make solo buttons mutually exclusive (radio button behavior)
    hcSoloButton.onClick = [this]() {
        if (hcSoloButton.getToggleState()) {
            scSoloButton.setToggleState(false, juce::sendNotification);
            slSoloButton.setToggleState(false, juce::sendNotification);
            flSoloButton.setToggleState(false, juce::sendNotification);
        }
    };
    scSoloButton.onClick = [this]() {
        if (scSoloButton.getToggleState()) {
            hcSoloButton.setToggleState(false, juce::sendNotification);
            slSoloButton.setToggleState(false, juce::sendNotification);
            flSoloButton.setToggleState(false, juce::sendNotification);
        }
    };
    slSoloButton.onClick = [this]() {
        if (slSoloButton.getToggleState()) {
            hcSoloButton.setToggleState(false, juce::sendNotification);
            scSoloButton.setToggleState(false, juce::sendNotification);
            flSoloButton.setToggleState(false, juce::sendNotification);
        }
    };
    flSoloButton.onClick = [this]() {
        if (flSoloButton.getToggleState()) {
            hcSoloButton.setToggleState(false, juce::sendNotification);
            scSoloButton.setToggleState(false, juce::sendNotification);
            slSoloButton.setToggleState(false, juce::sendNotification);
        }
    };

    addAndMakeVisible(hcSoloButton);
    addAndMakeVisible(scSoloButton);
    addAndMakeVisible(slSoloButton);
    addAndMakeVisible(flSoloButton);

    // === SIMPLIFIED OUTPUT LIMITER CONTROLS ===

    // Shared Output Ceiling Slider
    setupRotarySlider(outputCeilingSlider, outputCeilingLabel, "Ceiling");
    outputCeilingSlider.setTooltip("Output ceiling in dBTP\nUsed by both Overshoot and True Peak limiters");

    // Overshoot Enable Button
    overshootEnableButton.setButtonText("OVERSHOOT");
    overshootEnableButton.setClickingTogglesState(true);
    overshootEnableButton.setColour(juce::TextButton::buttonColourId, juce::Colour(35, 35, 40));
    overshootEnableButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(200, 130, 60));  // Orange when on
    overshootEnableButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.7f));
    overshootEnableButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    overshootEnableButton.setTooltip("Overshoot Suppression\n"
                                      "• Character shaping with controlled overshoot\n"
                                      "• Use for punch/warmth when strict compliance not needed\n"
                                      "• Runs BEFORE True Peak if both enabled");
    addAndMakeVisible(overshootEnableButton);

    // True Peak Enable Button
    truePeakEnableButton.setButtonText("TRUE PEAK");
    truePeakEnableButton.setClickingTogglesState(true);
    truePeakEnableButton.setColour(juce::TextButton::buttonColourId, juce::Colour(35, 35, 40));
    truePeakEnableButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(80, 180, 120));  // Green when on
    truePeakEnableButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.7f));
    truePeakEnableButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    truePeakEnableButton.setTooltip("True Peak Limiter (ITU-R BS.1770-4)\n"
                                     "• Strict compliance - guarantees ceiling\n"
                                     "• Maximum transparency, minimal distortion\n"
                                     "• Recommended for streaming/broadcast");
    addAndMakeVisible(truePeakEnableButton);

    // Setup Preset Buttons (A, B, C, D)
    // Click = Recall, Cmd+Click = Save, Opt+Click = Clear
    auto setupPresetButton = [this](juce::TextButton& recallBtn, juce::TextButton& saveBtn, const juce::String& label, int slot)
    {
        recallBtn.setButtonText(label);
        recallBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(45, 45, 52));
        recallBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(80, 140, 200));  // Blue when active
        recallBtn.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.6f));  // Dim when empty
        recallBtn.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
        recallBtn.setClickingTogglesState(false);  // We manage toggle state manually
        recallBtn.setTooltip("Click: Save (empty) / Recall (saved)\nCmd+Click: Overwrite " + label + "\nOpt+Click: Clear " + label);

        recallBtn.onClick = [this, slot, &recallBtn]
        {
            auto mods = juce::ModifierKeys::getCurrentModifiers();

            if (mods.isAltDown())
            {
                // Opt+Click = Clear this preset slot
                audioProcessor.clearPreset(slot);
                if (currentABCDSlot == slot)
                    currentABCDSlot = -1;
                // Visual feedback - red flash then dim
                recallBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(180, 60, 60));
                juce::Timer::callAfterDelay(200, [&recallBtn]() {
                    recallBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(45, 45, 52));
                });
                updateABCDButtonStates();
            }
            else if (mods.isCommandDown())
            {
                // Cmd+Click = Save current state to this slot
                audioProcessor.savePreset(slot);
                currentABCDSlot = slot;
                // Visual feedback - green flash
                recallBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(80, 180, 80));
                juce::Timer::callAfterDelay(200, [&recallBtn]() {
                    recallBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(45, 45, 52));
                });
                updateABCDButtonStates();
            }
            else
            {
                // Click behavior depends on whether preset exists
                if (audioProcessor.hasPreset(slot))
                {
                    // Slot has preset - recall it
                    audioProcessor.recallPreset(slot);
                    currentABCDSlot = slot;
                    updateABCDButtonStates();
                }
                else
                {
                    // Slot is empty - save current state (first click saves)
                    audioProcessor.savePreset(slot);
                    currentABCDSlot = slot;
                    // Visual feedback - green flash
                    recallBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(80, 180, 80));
                    juce::Timer::callAfterDelay(200, [&recallBtn]() {
                        recallBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(45, 45, 52));
                    });
                    updateABCDButtonStates();
                }
            }
        };
        addAndMakeVisible(recallBtn);

        // Save button (hidden - using Cmd+Click instead)
        saveBtn.setVisible(false);
    };

    setupPresetButton(presetButtonA, saveButtonA, "A", 0);
    setupPresetButton(presetButtonB, saveButtonB, "B", 1);
    setupPresetButton(presetButtonC, saveButtonC, "C", 2);
    setupPresetButton(presetButtonD, saveButtonD, "D", 3);

    // Setup Processing Mode Selector (Zero Latency, Balanced, Linear Phase)
    processingModeLabel.setText("ENGINE:", juce::dontSendNotification);
    processingModeLabel.setJustificationType(juce::Justification::centredRight);
    processingModeLabel.setFont(juce::Font(11.0f, juce::Font::bold));
    processingModeLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.85f));
    addAndMakeVisible(processingModeLabel);

    processingModeCombo.addItem("Zero Latency", 1);
    processingModeCombo.addItem("Balanced", 2);
    processingModeCombo.addItem("Linear Phase", 3);
    processingModeCombo.setColour(juce::ComboBox::backgroundColourId, juce::Colour(35, 35, 40));
    processingModeCombo.setColour(juce::ComboBox::textColourId, juce::Colours::white.withAlpha(0.9f));
    processingModeCombo.setColour(juce::ComboBox::outlineColourId, juce::Colour(60, 60, 65));
    processingModeCombo.setColour(juce::ComboBox::buttonColourId, juce::Colour(50, 50, 55));
    processingModeCombo.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(processingModeCombo);

    // Setup Version Label (upper right corner)
    versionLabel.setText("v" + kPluginVersion, juce::dontSendNotification);
    versionLabel.setJustificationType(juce::Justification::centredRight);
    versionLabel.setFont(juce::Font(10.0f, juce::Font::plain));
    versionLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.5f));
    addAndMakeVisible(versionLabel);

    // Create Attachments
    inputGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        p.apvts, "INPUT_GAIN", inputGainSlider);
    outputGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        p.apvts, "OUTPUT_GAIN", outputGainSlider);
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        p.apvts, "MIX_WET", mixSlider);
    thresholdAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        p.apvts, "THRESHOLD", thresholdSlider);
    thresholdLinkAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        p.apvts, "THRESHOLD_LINK", thresholdLinkButton);

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

    hcSoloAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        p.apvts, "HC_SOLO", hcSoloButton);
    scSoloAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        p.apvts, "SC_SOLO", scSoloButton);
    slSoloAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        p.apvts, "SL_SOLO", slSoloButton);
    flSoloAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        p.apvts, "FL_SOLO", flSoloButton);

    deltaModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        p.apvts, "DELTA_MODE", deltaModeButton);
    // O/S Delta attachment removed - functionality integrated into main delta mode
    // overshootDeltaModeAttachment removed
    truePeakDeltaModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        p.apvts, "TRUE_PEAK_DELTA_MODE", truePeakDeltaModeButton);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        p.apvts, "BYPASS", bypassButton);
    masterCompAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        p.apvts, "MASTER_COMP", masterCompButton);

    // === SIMPLIFIED OUTPUT LIMITER ATTACHMENTS ===
    outputCeilingAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        p.apvts, "OUTPUT_CEILING", outputCeilingSlider);
    overshootEnableAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        p.apvts, "OVERSHOOT_ENABLE", overshootEnableButton);
    truePeakEnableAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        p.apvts, "TRUE_PEAK_ENABLE", truePeakEnableButton);

    processingModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        p.apvts, "PROCESSING_MODE", processingModeCombo);

    // Initialize Master Comp to match Delta Mode state on plugin load
    auto* deltaModeParam = p.apvts.getRawParameterValue("DELTA_MODE");
    bool deltaModeOn = deltaModeParam ? deltaModeParam->load() > 0.5f : false;
    if (auto* masterCompParam = p.apvts.getParameter("MASTER_COMP"))
        masterCompParam->setValueNotifyingHost(deltaModeOn ? 1.0f : 0.0f);

    // Add CalibrationPanel LAST so it appears on top of all other controls
    addChildComponent(calibrationPanel);  // Use addChildComponent to keep panel hidden initially

    // Initialize ABCD button states
    updateABCDButtonStates();
}

QuadBlendDriveAudioProcessorEditor::~QuadBlendDriveAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void QuadBlendDriveAudioProcessorEditor::timerCallback()
{
    // Update normalization display labels in CalibrationPanel
    double peakDB = audioProcessor.currentPeakDB.load();
    double gainDB = audioProcessor.normalizationGainDB.load();

    calibrationPanel.updateInputPeakDisplay(peakDB);
    calibrationPanel.updateGainAppliedDisplay(gainDB);

    // Update trim slider colors if processor colors have changed
    hcTrimSlider.setColour(juce::Slider::thumbColourId, audioProcessor.processorColors.hardClip);
    scTrimSlider.setColour(juce::Slider::thumbColourId, audioProcessor.processorColors.softClip);
    slTrimSlider.setColour(juce::Slider::thumbColourId, audioProcessor.processorColors.slowLimit);
    flTrimSlider.setColour(juce::Slider::thumbColourId, audioProcessor.processorColors.fastLimit);

    // Update non-processor slider colors with accent color
    inputGainSlider.setColour(juce::Slider::thumbColourId, audioProcessor.processorColors.accent);
    outputGainSlider.setColour(juce::Slider::thumbColourId, audioProcessor.processorColors.accent);
    mixSlider.setColour(juce::Slider::thumbColourId, audioProcessor.processorColors.accent);
    thresholdSlider.setColour(juce::Slider::thumbColourId, audioProcessor.processorColors.accent);
    outputCeilingSlider.setColour(juce::Slider::thumbColourId, audioProcessor.processorColors.accent);

    // Update Undo/Redo button enabled states
    bool canUndo = audioProcessor.undoManager.canUndo();
    bool canRedo = audioProcessor.undoManager.canRedo();
    undoButton.setColour(juce::TextButton::textColourOffId,
        juce::Colours::white.withAlpha(canUndo ? 0.85f : 0.3f));
    redoButton.setColour(juce::TextButton::textColourOffId,
        juce::Colours::white.withAlpha(canRedo ? 0.85f : 0.3f));

    // Update Learn toggle button based on normalization state
    // Visual feedback shows normalization status even when panel is collapsed
    // Only lights up when Apply button is actually toggled ON (not just when learned)
    bool normalizationEnabled = calibrationPanel.isNormalizationEnabled();
    if (normalizationEnabled)
    {
        // APPLIED: Active/lit state with bright accent color and indicator
        calibrationToggleButton.setButtonText("● LEARN");  // Indicator dot
        calibrationToggleButton.setColour(juce::TextButton::buttonColourId, juce::Colour(255, 140, 60));  // Bright orange (lit)
        calibrationToggleButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    }
    else
    {
        // NOT APPLIED: Default/unlit state
        calibrationToggleButton.setButtonText("LEARN");  // No indicator
        calibrationToggleButton.setColour(juce::TextButton::buttonColourId, juce::Colour(35, 35, 40));  // Dark grey (unlit)
        calibrationToggleButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.85f));
    }

    // === TRUE PEAK LIMITER AVAILABILITY ===
    // TPL requires lookahead which is not available in Zero Latency mode
    auto* processingModeParam = audioProcessor.apvts.getRawParameterValue("PROCESSING_MODE");
    int processingMode = processingModeParam ? static_cast<int>(processingModeParam->load()) : 1;
    bool isZeroLatencyMode = (processingMode == 0);

    if (isZeroLatencyMode)
    {
        // Disable TPL in Zero Latency mode - it requires lookahead
        truePeakEnableButton.setEnabled(false);
        truePeakEnableButton.setAlpha(0.4f);
        truePeakEnableButton.setTooltip("True Peak Limiter unavailable in Zero Latency mode\n"
                                        "(requires 2ms lookahead for inter-sample peak detection)");
    }
    else
    {
        truePeakEnableButton.setEnabled(true);
        truePeakEnableButton.setAlpha(1.0f);
        truePeakEnableButton.setTooltip("True Peak Limiter (ITU-R BS.1770-4)\n"
                                        "• Strict compliance - guarantees ceiling\n"
                                        "• Maximum transparency, minimal distortion\n"
                                        "• Recommended for streaming/broadcast");
    }

    // === THRESHOLD LINK LOGIC ===
    // When linked, threshold automatically tracks calibration level
    bool thresholdLinked = thresholdLinkButton.getToggleState();
    if (thresholdLinked)
    {
        // Automatically sync threshold with calibration level
        auto* calibParam = audioProcessor.apvts.getRawParameterValue("CALIB_LEVEL");
        auto* threshParam = audioProcessor.apvts.getRawParameterValue("THRESHOLD");
        auto* threshParamObj = audioProcessor.apvts.getParameter("THRESHOLD");

        if (calibParam != nullptr && threshParam != nullptr && threshParamObj != nullptr)
        {
            float calibLevel = calibParam->load();
            float currentThreshold = threshParam->load();

            // Only update if there's a meaningful difference (avoid constant updates)
            if (std::abs(currentThreshold - calibLevel) > 0.05f)
            {
                thresholdSlider.setValue(calibLevel, juce::dontSendNotification);
                threshParamObj->setValueNotifyingHost(threshParamObj->convertTo0to1(calibLevel));
            }
        }

        // Visual feedback: Dim threshold slider when linked (shows it's automated)
        thresholdSlider.setAlpha(0.6f);
    }
    else
    {
        // Manual mode: Full opacity
        thresholdSlider.setAlpha(1.0f);
    }
}

void QuadBlendDriveAudioProcessorEditor::updateABCDButtonStates()
{
    // Update button appearance based on whether preset exists and is currently active
    auto updateButton = [this](juce::TextButton& btn, int slot)
    {
        bool hasPreset = audioProcessor.hasPreset(slot);
        bool isActive = (currentABCDSlot == slot);

        // Text brightness indicates if preset exists
        btn.setColour(juce::TextButton::textColourOffId,
            hasPreset ? juce::Colours::white.withAlpha(0.95f) : juce::Colours::white.withAlpha(0.4f));

        // Background color indicates active state
        if (isActive && hasPreset)
            btn.setColour(juce::TextButton::buttonColourId, juce::Colour(80, 140, 200));  // Blue = active
        else
            btn.setColour(juce::TextButton::buttonColourId, juce::Colour(45, 45, 52));  // Default grey
    };

    updateButton(presetButtonA, 0);
    updateButton(presetButtonB, 1);
    updateButton(presetButtonC, 2);
    updateButton(presetButtonD, 3);
}

void QuadBlendDriveAudioProcessorEditor::paint(juce::Graphics& g)
{
    const float scale = getWidth() / 1050.0f;
    const int toolbarHeight = static_cast<int>(28 * scale);
    const int titleAreaHeight = static_cast<int>(70 * scale);
    const int padding = static_cast<int>(20 * scale);

    // Calculate XY pad top position (same logic as resized())
    // XY pad starts at: toolbarHeight + titleAreaHeight + padding + yOffset
    // For centering title, we use the area between toolbar and XY pad top
    const int xyPadTopY = xyPad.getY();

    // Modern gradient background
    juce::ColourGradient bgGradient(juce::Colour(16, 16, 20), 0, 0,
                                     juce::Colour(12, 12, 16), 0, static_cast<float>(getHeight()), false);
    g.setGradientFill(bgGradient);
    g.fillAll();

    // ========== TOOLBAR BACKGROUND ==========
    juce::ColourGradient toolbarGradient(juce::Colour(28, 28, 32), 0, 0,
                                          juce::Colour(22, 22, 26), 0, static_cast<float>(toolbarHeight), false);
    g.setGradientFill(toolbarGradient);
    g.fillRect(0, 0, getWidth(), toolbarHeight);

    // Toolbar bottom border
    g.setColour(juce::Colour(50, 50, 55));
    g.drawLine(0, static_cast<float>(toolbarHeight - 1),
               static_cast<float>(getWidth()), static_cast<float>(toolbarHeight - 1), 1.0f);

    // ========== TITLE (centered between toolbar and XY pad) ==========
    const int titleFontHeight = static_cast<int>(42 * scale);
    const int availableSpace = xyPadTopY - toolbarHeight;
    const int titleY = toolbarHeight + (availableSpace - titleFontHeight) / 2;

    // Shadow/glow effect
    g.setColour(juce::Colour(255, 120, 50).withAlpha(0.15f));
    g.setFont(juce::Font(42.0f * scale, juce::Font::bold));
    g.drawText("Emulsion", 0, titleY + 1, getWidth(),
               titleFontHeight, juce::Justification::centred);

    // Main title
    g.setColour(juce::Colours::white.withAlpha(0.95f));
    g.setFont(juce::Font(42.0f * scale, juce::Font::plain));
    g.drawText("Emulsion", 0, titleY, getWidth(),
               titleFontHeight, juce::Justification::centred);
}

void QuadBlendDriveAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    const float scale = getWidth() / 1050.0f;  // Scale factor based on default width

    // ========== TOOLBAR AREA (TOP) ==========
    const int toolbarHeight = static_cast<int>(28 * scale);
    auto toolbarArea = bounds.removeFromTop(toolbarHeight);
    const int toolbarPadding = static_cast<int>(12 * scale);
    const int toolbarButtonHeight = static_cast<int>(20 * scale);
    const int toolbarY = (toolbarHeight - toolbarButtonHeight) / 2;

    // Toolbar button widths
    const int calibBtnWidth = static_cast<int>(95 * scale);
    const int presetMenuWidth = static_cast<int>(115 * scale);
    const int quickPresetWidth = static_cast<int>(26 * scale);
    const int engineLabelWidth = static_cast<int>(55 * scale);
    const int engineComboWidth = static_cast<int>(115 * scale);
    const int undoRedoWidth = static_cast<int>(42 * scale);
    const int helpBtnWidth = static_cast<int>(24 * scale);
    const int versionWidth = static_cast<int>(50 * scale);

    int toolbarX = toolbarPadding;

    // CALIBRATION button
    calibrationToggleButton.setBounds(toolbarX, toolbarY, calibBtnWidth, toolbarButtonHeight);
    toolbarX += calibBtnWidth + static_cast<int>(8 * scale);

    // Default Setting dropdown
    presetMenuButton.setBounds(toolbarX, toolbarY, presetMenuWidth, toolbarButtonHeight);
    toolbarX += presetMenuWidth + static_cast<int>(12 * scale);

    // Quick Presets A, B, C, D
    const int presetGap = static_cast<int>(4 * scale);
    presetButtonA.setBounds(toolbarX, toolbarY, quickPresetWidth, toolbarButtonHeight);
    toolbarX += quickPresetWidth + presetGap;
    presetButtonB.setBounds(toolbarX, toolbarY, quickPresetWidth, toolbarButtonHeight);
    toolbarX += quickPresetWidth + presetGap;
    presetButtonC.setBounds(toolbarX, toolbarY, quickPresetWidth, toolbarButtonHeight);
    toolbarX += quickPresetWidth + presetGap;
    presetButtonD.setBounds(toolbarX, toolbarY, quickPresetWidth, toolbarButtonHeight);
    toolbarX += quickPresetWidth + static_cast<int>(20 * scale);  // Spacer before ENGINE

    // ENGINE label and combo
    processingModeLabel.setBounds(toolbarX, toolbarY, engineLabelWidth, toolbarButtonHeight);
    toolbarX += engineLabelWidth + static_cast<int>(4 * scale);
    processingModeCombo.setBounds(toolbarX, toolbarY, engineComboWidth, toolbarButtonHeight);

    // Right side of toolbar: Colors, Undo, Redo, Help, Version
    const int colorsBtnWidth = static_cast<int>(55 * scale);
    int rightX = getWidth() - toolbarPadding - versionWidth;
    versionLabel.setBounds(rightX, toolbarY, versionWidth, toolbarButtonHeight);
    rightX -= helpBtnWidth + static_cast<int>(12 * scale);
    helpButton.setBounds(rightX, toolbarY, helpBtnWidth, toolbarButtonHeight);
    rightX -= colorsBtnWidth + static_cast<int>(8 * scale);
    preferencesButton.setBounds(rightX, toolbarY, colorsBtnWidth, toolbarButtonHeight);
    rightX -= undoRedoWidth + static_cast<int>(12 * scale);
    redoButton.setBounds(rightX, toolbarY, undoRedoWidth, toolbarButtonHeight);
    rightX -= undoRedoWidth + static_cast<int>(8 * scale);
    undoButton.setBounds(rightX, toolbarY, undoRedoWidth, toolbarButtonHeight);

    // Hide save buttons (not needed in toolbar)
    saveButtonA.setBounds(0, 0, 0, 0);
    saveButtonB.setBounds(0, 0, 0, 0);
    saveButtonC.setBounds(0, 0, 0, 0);
    saveButtonD.setBounds(0, 0, 0, 0);

    // ========== TITLE AREA ==========
    auto titleArea = bounds.removeFromTop(static_cast<int>(70 * scale));

    // Calibration Panel - Below toolbar when expanded
    if (calibrationPanel.isVisible())
    {
        const int panelWidth = static_cast<int>(250 * scale);
        const int panelHeight = 200;
        calibrationPanel.setBounds(toolbarPadding,
                                  toolbarHeight + static_cast<int>(5 * scale),
                                  panelWidth,
                                  panelHeight);
    }

    const int padding = static_cast<int>(20 * scale);
    const int knobSize = static_cast<int>(70 * scale);
    const int spacing = static_cast<int>(10 * scale);

    // === PANEL EXPANSION LOGIC ===
    // Base content height is 610px minus toolbar and title
    // Scope and Advanced panels expand BELOW this baseline
    const int baseHeight = static_cast<int>(610 * scale);
    const int titleHeight = static_cast<int>(70 * scale);
    const int mainContentHeight = baseHeight - toolbarHeight - titleHeight;
    const int advancedHeight = static_cast<int>(180 * scale);

    // Main content area is constrained to the top 600px (minus toolbar/title)
    juce::Rectangle<int> mainContentBounds = bounds;
    mainContentBounds.setHeight(mainContentHeight);

    // Scope area calculated after button positioning (see below)
    // We'll set the scope bounds after we know where the button row ends
    juce::Rectangle<int> scopeArea;

    // Use mainContentBounds instead of bounds for the 3-column layout
    bounds = mainContentBounds;

    // Now reserve right edge for input/output meters (from remaining height)
    const int meterWidth = static_cast<int>(49 * scale);  // 30% slimmer than before (70 * 0.7 = 49)
    const int meterGap = static_cast<int>(8 * scale);
    const int totalMeterWidth = (meterWidth * 2) + meterGap;  // Two meters plus gap
    auto meterArea = bounds.removeFromRight(totalMeterWidth);
    bounds.removeFromRight(spacing);  // Gap between meters and main content

    // Proportional three-column layout: LEFT (Input) | CENTER (XY) | RIGHT (Output)
    // Narrower side columns to give more space to XY pad
    const int sideColumnWidth = static_cast<int>(280 * scale);
    const int centerColumnWidth = static_cast<int>(490 * scale);

    // ========== LEFT SECTION (INPUT CONTROLS) ==========
    auto leftSection = bounds.removeFromLeft(sideColumnWidth).reduced(padding, 0);

    // === EVENLY SPACED LAYOUT ===
    // Larger knobs with tighter spacing
    const int mainKnobSize = static_cast<int>(72 * scale);     // Input/Thresh knobs (larger)
    const int trimKnobSize = static_cast<int>(68 * scale);     // Processor trim knobs (larger)
    const int msButtonSize = static_cast<int>(32 * scale);     // M/S buttons
    const int calButtonWidth = static_cast<int>(38 * scale);
    const int calButtonHeight = static_cast<int>(22 * scale);
    const int labelWidth = static_cast<int>(70 * scale);       // Wider for full names

    // Calculate even row spacing - 6 main rows + reset button row
    const int totalRows = 7;  // Input, Thresh, Slow, Soft, Hard, Fast, Reset
    const int availableHeight = leftSection.getHeight();
    const int rowHeight = availableHeight / totalRows;

    // Center the knobs in the column
    const int leftColumnCenter = leftSection.getX() + leftSection.getWidth() / 2;
    const int knobCenterX = leftColumnCenter;
    const int knobX = knobCenterX - mainKnobSize / 2;
    const int trimKnobX = knobCenterX - trimKnobSize / 2;

    // Labels to the left of knobs
    const int labelX = leftSection.getX();

    // M/S buttons to the right of knobs
    const int msButtonX = knobCenterX + trimKnobSize / 2 + static_cast<int>(15 * scale);
    const int msButtonGap = static_cast<int>(6 * scale);

    // CAL button position (to the right of threshold knob)
    const int calButtonX = knobCenterX + mainKnobSize / 2 + static_cast<int>(10 * scale);

    // Vertical centering helper
    auto centerInRow = [](int rowY, int rowH, int itemH) { return rowY + (rowH - itemH) / 2; };

    // Input Gain row
    auto inputRow = leftSection.removeFromTop(rowHeight);
    inputGainLabel.setBounds(labelX, centerInRow(inputRow.getY(), rowHeight, mainKnobSize), labelWidth, mainKnobSize);
    inputGainSlider.setBounds(knobX, centerInRow(inputRow.getY(), rowHeight, mainKnobSize), mainKnobSize, mainKnobSize);

    // Threshold row - CAL button to the RIGHT of knob
    auto threshRow = leftSection.removeFromTop(rowHeight);
    thresholdLabel.setBounds(labelX, centerInRow(threshRow.getY(), rowHeight, mainKnobSize), labelWidth, mainKnobSize);
    thresholdSlider.setBounds(knobX, centerInRow(threshRow.getY(), rowHeight, mainKnobSize), mainKnobSize, mainKnobSize);
    thresholdLinkButton.setBounds(calButtonX, centerInRow(threshRow.getY(), rowHeight, calButtonHeight), calButtonWidth, calButtonHeight);

    // 4 Processor Trims - [Label][Knob][M][S]
    // Slow Trim
    auto slRow = leftSection.removeFromTop(rowHeight);
    slTrimLabel.setBounds(labelX, centerInRow(slRow.getY(), rowHeight, trimKnobSize), labelWidth, trimKnobSize);
    slTrimSlider.setBounds(trimKnobX, centerInRow(slRow.getY(), rowHeight, trimKnobSize), trimKnobSize, trimKnobSize);
    slMuteButton.setBounds(msButtonX, centerInRow(slRow.getY(), rowHeight, msButtonSize), msButtonSize, msButtonSize);
    slSoloButton.setBounds(msButtonX + msButtonSize + msButtonGap, centerInRow(slRow.getY(), rowHeight, msButtonSize), msButtonSize, msButtonSize);

    // Soft Trim
    auto scRow = leftSection.removeFromTop(rowHeight);
    scTrimLabel.setBounds(labelX, centerInRow(scRow.getY(), rowHeight, trimKnobSize), labelWidth, trimKnobSize);
    scTrimSlider.setBounds(trimKnobX, centerInRow(scRow.getY(), rowHeight, trimKnobSize), trimKnobSize, trimKnobSize);
    scMuteButton.setBounds(msButtonX, centerInRow(scRow.getY(), rowHeight, msButtonSize), msButtonSize, msButtonSize);
    scSoloButton.setBounds(msButtonX + msButtonSize + msButtonGap, centerInRow(scRow.getY(), rowHeight, msButtonSize), msButtonSize, msButtonSize);

    // Hard Trim
    auto hcRow = leftSection.removeFromTop(rowHeight);
    hcTrimLabel.setBounds(labelX, centerInRow(hcRow.getY(), rowHeight, trimKnobSize), labelWidth, trimKnobSize);
    hcTrimSlider.setBounds(trimKnobX, centerInRow(hcRow.getY(), rowHeight, trimKnobSize), trimKnobSize, trimKnobSize);
    hcMuteButton.setBounds(msButtonX, centerInRow(hcRow.getY(), rowHeight, msButtonSize), msButtonSize, msButtonSize);
    hcSoloButton.setBounds(msButtonX + msButtonSize + msButtonGap, centerInRow(hcRow.getY(), rowHeight, msButtonSize), msButtonSize, msButtonSize);

    // Fast Trim
    auto flRow = leftSection.removeFromTop(rowHeight);
    flTrimLabel.setBounds(labelX, centerInRow(flRow.getY(), rowHeight, trimKnobSize), labelWidth, trimKnobSize);
    flTrimSlider.setBounds(trimKnobX, centerInRow(flRow.getY(), rowHeight, trimKnobSize), trimKnobSize, trimKnobSize);
    flMuteButton.setBounds(msButtonX, centerInRow(flRow.getY(), rowHeight, msButtonSize), msButtonSize, msButtonSize);
    flSoloButton.setBounds(msButtonX + msButtonSize + msButtonGap, centerInRow(flRow.getY(), rowHeight, msButtonSize), msButtonSize, msButtonSize);

    // Reset Trims button - aligned with ADVANCED and SCOPE buttons (positioned after advancedButtonY is calculated)
    // Just consume the row for now, button placement happens later
    auto resetRow = leftSection.removeFromTop(rowHeight);
    (void)resetRow;  // Unused - button positioned below with advancedButtonY

    // ========== CENTER SECTION (XY PAD) - CENTERED AND SQUARE ==========
    auto centerSection = bounds.removeFromLeft(centerColumnWidth).reduced(padding, 0);

    // (Engine mode and preset buttons are now in toolbar - no duplicate positioning here)

    // Make XY Pad SQUARE - use the smaller dimension
    int xyAvailableHeight = centerSection.getHeight();
    int xyAvailableWidth = centerSection.getWidth();
    int xySize = juce::jmin(xyAvailableWidth, xyAvailableHeight);

    // Center the square XY pad both horizontally and vertically
    int xOffset = (xyAvailableWidth - xySize) / 2;
    int yOffset = (xyAvailableHeight - xySize) / 2;
    xyPad.setBounds(centerSection.getX() + xOffset, centerSection.getY() + yOffset, xySize, xySize);

    // Advanced Toggle Button - Centered below XY Pad
    const int advancedButtonHeight = static_cast<int>(30 * scale);
    const int advancedButtonWidth = static_cast<int>(120 * scale);
    int advancedButtonY = centerSection.getY() + yOffset + xySize + static_cast<int>(10 * scale);
    int advancedButtonX = centerSection.getX() + (xyAvailableWidth - advancedButtonWidth) / 2;
    advancedToggleButton.setBounds(advancedButtonX, advancedButtonY, advancedButtonWidth, advancedButtonHeight);

    // Advanced Panel - Overlays waveform meter when visible
    // Positioned absolutely to overlay the bottom section
    if (advancedPanel.isVisible())
    {
        const int advancedPanelWidth = static_cast<int>(600 * scale);  // Wider for two-column layout
        const int advancedPanelHeight = static_cast<int>(280 * scale);  // Reduced height (horizontal gain comp layout)
        int advancedPanelY = advancedButtonY + advancedButtonHeight + static_cast<int>(8 * scale);
        int advancedPanelX = centerSection.getX() + (xyAvailableWidth - advancedPanelWidth) / 2;
        advancedPanel.setBounds(advancedPanelX, advancedPanelY, advancedPanelWidth, advancedPanelHeight);
    }
    else
    {
        advancedPanel.setBounds(0, 0, 0, 0);  // Hide when not visible
    }

    // ========== RIGHT SECTION (OUTPUT CONTROLS) ==========
    // Organized into 3 groups:
    // Group 1: Output, LINK I/O, BYPASS, DELTA MODE, Mix
    // Group 2: Ceiling, OVERSHOOT, TRUE PEAK
    // Group 3: SCOPE (aligned with ADVANCED button)
    auto rightSection = bounds.reduced(padding, 0);

    // Larger knobs with tighter spacing
    const int rightKnobSize = static_cast<int>(72 * scale);     // Match left column knob size
    const int rightButtonWidth = static_cast<int>(100 * scale); // Standard button width
    const int rightButtonHeight = static_cast<int>(28 * scale); // Standard button height
    const int rightLabelWidth = static_cast<int>(50 * scale);
    const int rightRowSpacing = static_cast<int>(4 * scale);    // Tighter spacing to fit larger knobs

    // Center the knobs/buttons in the column
    const int rightColumnCenter = rightSection.getX() + rightSection.getWidth() / 2;
    const int rightKnobX = rightColumnCenter - rightKnobSize / 2;
    const int rightButtonX = rightColumnCenter - rightButtonWidth / 2;
    const int rightLabelX = rightSection.getX();

    // === GROUP 1: Output, LINK I/O, BYPASS, DELTA MODE, Mix ===
    int currentY = rightSection.getY();

    // Output Gain
    outputGainLabel.setBounds(rightLabelX, currentY, rightLabelWidth, rightKnobSize);
    outputGainSlider.setBounds(rightKnobX, currentY, rightKnobSize, rightKnobSize);
    currentY += rightKnobSize + rightRowSpacing;

    // LINK I/O button
    gainLinkButton.setBounds(rightButtonX, currentY, rightButtonWidth, rightButtonHeight);
    currentY += rightButtonHeight + rightRowSpacing;

    // BYPASS button
    bypassButton.setBounds(rightButtonX, currentY, rightButtonWidth, rightButtonHeight);
    currentY += rightButtonHeight + rightRowSpacing;

    // DELTA MODE button
    deltaModeButton.setBounds(rightButtonX, currentY, rightButtonWidth, rightButtonHeight);
    currentY += rightButtonHeight + rightRowSpacing;

    // GAIN COMP button
    masterCompButton.setBounds(rightButtonX, currentY, rightButtonWidth, rightButtonHeight);
    currentY += rightButtonHeight + rightRowSpacing;

    // Mix knob
    mixLabel.setBounds(rightLabelX, currentY, rightLabelWidth, rightKnobSize);
    mixSlider.setBounds(rightKnobX, currentY, rightKnobSize, rightKnobSize);
    currentY += rightKnobSize + static_cast<int>(20 * scale);  // Extra gap before Group 2

    // === GROUP 2: Ceiling, OVERSHOOT, TRUE PEAK ===
    // Ceiling knob
    outputCeilingLabel.setBounds(rightLabelX, currentY, rightLabelWidth, rightKnobSize);
    outputCeilingSlider.setBounds(rightKnobX, currentY, rightKnobSize, rightKnobSize);
    currentY += rightKnobSize + rightRowSpacing;

    // OVERSHOOT button
    overshootEnableButton.setBounds(rightButtonX, currentY, rightButtonWidth, rightButtonHeight);
    currentY += rightButtonHeight + rightRowSpacing;

    // TRUE PEAK button
    truePeakEnableButton.setBounds(rightButtonX, currentY, rightButtonWidth, rightButtonHeight);

    // === GROUP 3: SCOPE button (aligned with ADVANCED button) ===
    // Position SCOPE at same Y as ADVANCED button
    scopeToggleButton.setBounds(rightButtonX, advancedButtonY, rightButtonWidth, advancedButtonHeight);

    // === RESET TRIMS button (aligned with ADVANCED and SCOPE) ===
    // Position in left column at same Y as ADVANCED/SCOPE
    const int resetTrimsWidth = static_cast<int>(100 * scale);
    const int resetTrimsX = padding + labelWidth + static_cast<int>(10 * scale);  // Aligned with trim knobs
    resetTrimsButton.setBounds(resetTrimsX, advancedButtonY, resetTrimsWidth, advancedButtonHeight);

    // ========== RIGHT EDGE METERS (INPUT/OUTPUT) ==========
    // Center meters between right controls and right edge of UI
    // Calculate the space available: from right edge of right section to right edge of window
    const int rightSectionRightEdge = rightSection.getRight();
    const int windowRightEdge = getWidth();
    const int availableMeterSpace = windowRightEdge - rightSectionRightEdge;
    const int meterBlockWidth = (meterWidth * 2) + meterGap;
    const int meterStartX = rightSectionRightEdge + (availableMeterSpace - meterBlockWidth) / 2;

    // Position input and output meters side-by-side, centered
    // Signal flow reads left to right: [IN] [OUT]
    inputMeter.setBounds(meterStartX, meterArea.getY(), meterWidth, meterArea.getHeight());
    outputMeter.setBounds(meterStartX + meterWidth + meterGap, meterArea.getY(), meterWidth, meterArea.getHeight());

    // ========== SCOPE AREA (when visible) ==========
    if (steveScope.isVisible())
    {
        // Scope starts below the button row (RESET TRIMS, ADVANCED, SCOPE buttons)
        const int scopeStartY = advancedButtonY + advancedButtonHeight + static_cast<int>(8 * scale);
        scopeArea = juce::Rectangle<int>(0, scopeStartY, getWidth(), getHeight() - scopeStartY);
        steveScope.setBounds(scopeArea.reduced(padding, padding));
    }
    else
    {
        // When hidden, make sure it's not visible
        steveScope.setBounds(0, 0, 0, 0);
    }

    // Transfer Curve toggle button and meter (hidden, not used in current layout)
    transferCurveToggleButton.setBounds(0, 0, 0, 0);
    transferCurveMeter.setBounds(0, 0, 0, 0);
}

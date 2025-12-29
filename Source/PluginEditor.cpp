#include "PluginProcessor.h"
#include "PluginEditor.h"

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

        // Draw colored segments based on RGB frequency content
        // Sample colors at regular intervals for smoother appearance
        const int colorSegments = juce::jmin(200, displayPoints / 4);
        for (int seg = 0; seg < colorSegments; ++seg)
        {
            int startIdx = (seg * displayPoints) / colorSegments;
            int endIdx = ((seg + 1) * displayPoints) / colorSegments;

            if (startIdx >= displayPoints || endIdx > displayPoints)
                continue;

            int bufferIndex = (startIdx * bufferSize) / displayPoints;
            int readPos = bufferIndex;  // Still window: read directly from buffer position

            if (readPos >= 0 && readPos < bufferSize)
            {
                // Get three-band amplitudes for RGB coloring
                float lowAmp = processor.oscilloscopeLowBand[readPos];     // Red channel
                float midAmp = processor.oscilloscopeMidBand[readPos];     // Green channel
                float highAmp = processor.oscilloscopeHighBand[readPos];   // Blue channel

                // Full brightness - scale values directly to 0-255 range
                // Use higher scale factor to ensure vibrant colors
                float colorScale = 4.0f;
                uint8_t red = static_cast<uint8_t>(juce::jlimit(0.0f, 255.0f, lowAmp * colorScale * 255.0f));
                uint8_t green = static_cast<uint8_t>(juce::jlimit(0.0f, 255.0f, midAmp * colorScale * 255.0f));
                uint8_t blue = static_cast<uint8_t>(juce::jlimit(0.0f, 255.0f, highAmp * colorScale * 255.0f));

                // Create full brightness color - no dimming or adjustments
                juce::Colour lineColor(red, green, blue);

                // Create segment path
                juce::Path segmentPath;
                bool segStarted = false;
                for (int i = startIdx; i <= endIdx && i < displayPoints; ++i)
                {
                    int bufIdx = (i * bufferSize) / displayPoints;
                    int rPos = bufIdx;  // Still window: read directly from buffer position

                    if (rPos >= 0 && rPos < bufferSize)
                    {
                        float sample = processor.oscilloscopeBuffer[rPos];
                        float x = graphBounds.getX() + (i / float(displayPoints - 1)) * graphBounds.getWidth();
                        float y = centerY - (sample * graphBounds.getHeight() * 0.4f);
                        y = juce::jlimit(graphBounds.getY(), graphBounds.getBottom(), y);

                        if (!segStarted)
                        {
                            segmentPath.startNewSubPath(x, y);
                            segStarted = true;
                        }
                        else
                        {
                            segmentPath.lineTo(x, y);
                        }
                    }
                }

                // Draw waveform with full brightness - no glow or shading
                g.setColour(lineColor);
                g.strokePath(segmentPath, juce::PathStrokeType(2.0f * scale, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
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

    const float thresholdDb = processor.apvts.getRawParameterValue("THRESHOLD")->load();

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
}

void XYPad::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const float width = bounds.getWidth();
    const float height = bounds.getHeight();

    // Modern gradient background
    juce::ColourGradient gradient(juce::Colour(22, 22, 26), bounds.getX(), bounds.getY(),
                                   juce::Colour(18, 18, 22), bounds.getX(), bounds.getBottom(), false);
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(bounds, 6.0f);

    // Ultra-minimal grid lines
    g.setColour(juce::Colour(55, 55, 60).withAlpha(0.4f));
    g.drawLine(width / 2.0f, 0, width / 2.0f, height, 1.0f);
    g.drawLine(0, height / 2.0f, width, height / 2.0f, 1.0f);

    // Sleek quadrant labels
    g.setColour(juce::Colours::white.withAlpha(0.5f));
    g.setFont(juce::Font(9.5f, juce::Font::plain));

    // Top-left: Hard Clip (Transparent/Safety Clipping)
    juce::Rectangle<float> hardClipArea(10, 10, width / 2.0f - 20, 16);
    g.drawText("HARD", hardClipArea, juce::Justification::centredLeft);

    // Top-right: Fast Limit (Quick Response Limiting)
    juce::Rectangle<float> fastLimitArea(width / 2.0f + 10, 10, width / 2.0f - 20, 16);
    g.drawText("FAST", fastLimitArea, juce::Justification::centredRight);

    // Bottom-left: Soft Clip (Musical Warmth/Saturation)
    juce::Rectangle<float> softClipArea(10, height - 24, width / 2.0f - 20, 16);
    g.drawText("SOFT", softClipArea, juce::Justification::centredLeft);

    // Bottom-right: Slow Limit (Smooth Mastering Character)
    juce::Rectangle<float> slowLimitArea(width / 2.0f + 10, height - 24, width / 2.0f - 20, 16);
    g.drawText("SLOW", slowLimitArea, juce::Justification::centredRight);

    // Draw thumb position with modern glow
    float x = xSlider.getValue();
    float y = 1.0f - ySlider.getValue();

    float thumbX = bounds.getX() + x * width;
    float thumbY = bounds.getY() + y * height;

    // Outer glow
    g.setColour(juce::Colour(255, 120, 50).withAlpha(0.2f));
    g.fillEllipse(thumbX - 10, thumbY - 10, 20, 20);

    // Middle glow
    g.setColour(juce::Colour(255, 120, 50).withAlpha(0.4f));
    g.fillEllipse(thumbX - 8, thumbY - 8, 16, 16);

    // Solid thumb
    g.setColour(juce::Colour(255, 120, 50));
    g.fillEllipse(thumbX - 6, thumbY - 6, 12, 12);

    // White center dot
    g.setColour(juce::Colours::white.withAlpha(0.9f));
    g.fillEllipse(thumbX - 2, thumbY - 2, 4, 4);

    // Clean outer border
    g.setColour(juce::Colour(60, 60, 65).withAlpha(0.4f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 6.0f, 1.0f);
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

    calibLevelLabel.setText("Calib", juce::dontSendNotification);
    calibLevelLabel.setJustificationType(juce::Justification::centred);
    calibLevelLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.75f));
    addAndMakeVisible(calibLevelLabel);

    // Analyze Button
    analyzeButton.setButtonText("ANALYZE");
    analyzeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(35, 35, 40));
    analyzeButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.85f));
    analyzeButton.onClick = [this, &proc = this->processor]()
    {
        if (proc.isAnalyzing.load())
        {
            proc.stopAnalysis();
            analyzeButton.setButtonText("ANALYZE");
        }
        else
        {
            proc.startAnalysis();
            analyzeButton.setButtonText("STOP");
        }
    };
    addAndMakeVisible(analyzeButton);

    // Normalize Button
    normalizeButton.setButtonText("NORMALIZE");
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
        analyzeButton.setButtonText("ANALYZE");
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

    // Start collapsed
    setVisible(false);
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

    // Row 2: Buttons (20% wider than 50% size)
    auto row2 = bounds.removeFromTop(25);
    analyzeButton.setBounds(row2.removeFromLeft(48));  // 40 * 1.2
    row2.removeFromLeft(5);
    normalizeButton.setBounds(row2.removeFromLeft(58));  // 48 * 1.2
    row2.removeFromLeft(5);
    resetNormButton.setBounds(row2.removeFromLeft(40));  // 33 * 1.2
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

AdvancedPanel::AdvancedPanel(juce::AudioProcessorValueTreeState& apvts)
    : apvts(apvts)
{
    // Configure section labels
    softClipLabel.setText("SOFT CLIP", juce::dontSendNotification);
    softClipLabel.setFont(juce::Font(10.0f, juce::Font::bold));
    softClipLabel.setColour(juce::Label::textColourId, juce::Colour(180, 180, 190));
    addAndMakeVisible(softClipLabel);

    limitersLabel.setText("SLOW LIMITER", juce::dontSendNotification);
    limitersLabel.setFont(juce::Font(10.0f, juce::Font::bold));
    limitersLabel.setColour(juce::Label::textColourId, juce::Colour(180, 180, 190));
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
    flLimitAttackLabel.setText("FL Attack Time:", juce::dontSendNotification);
    flLimitAttackLabel.setFont(juce::Font(10.0f));
    flLimitAttackLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.8f));
    addAndMakeVisible(flLimitAttackLabel);
    flLimitAttackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "FL_LIMIT_ATTACK", flLimitAttackSlider);

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
    envelopeLabel.setText("ENVELOPE SHAPING (TRANSIENT/SUSTAIN)", juce::dontSendNotification);
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

    // Hard Clip
    setupEnvelopeSlider(hcAttackSlider, hcAttackLabel, "HC ATK");
    setupEnvelopeSlider(hcSustainSlider, hcSustainLabel, "HC SUS");
    hcAttackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "HC_ATTACK", hcAttackSlider);
    hcSustainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "HC_SUSTAIN", hcSustainSlider);

    // Soft Clip
    setupEnvelopeSlider(scAttackSlider, scAttackLabel, "SC ATK");
    setupEnvelopeSlider(scSustainSlider, scSustainLabel, "SC SUS");
    scAttackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "SC_ATTACK", scAttackSlider);
    scSustainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "SC_SUSTAIN", scSustainSlider);

    // Slow Limit
    setupEnvelopeSlider(slAttackSlider, slAttackLabel, "SL ATK");
    setupEnvelopeSlider(slSustainSlider, slSustainLabel, "SL SUS");
    slAttackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "SL_ATTACK", slAttackSlider);
    slSustainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "SL_SUSTAIN", slSustainSlider);

    // Fast Limit
    setupEnvelopeSlider(flAttackSlider, flAttackLabel, "FL ATK");
    setupEnvelopeSlider(flSustainSlider, flSustainLabel, "FL SUS");
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
    auto bounds = getLocalBounds().reduced(12);

    // Use two-column layout with better spacing
    const int columnGap = 15;
    const int halfWidth = (bounds.getWidth() - columnGap) / 2;
    auto leftColumn = bounds.removeFromLeft(halfWidth);
    bounds.removeFromLeft(columnGap);
    auto rightColumn = bounds;

    const int rowHeight = 22;
    const int sectionGap = 10;
    const int headerHeight = 16;
    const int labelWidth = 95;
    const int sliderWidth = 150;

    // ========== LEFT COLUMN ==========
    int yPosLeft = 0;

    // === SOFT CLIP SECTION ===
    softClipLabel.setBounds(leftColumn.getX(), yPosLeft, leftColumn.getWidth(), headerHeight);
    yPosLeft += headerHeight + 8;

    scKneeLabel.setBounds(leftColumn.getX(), yPosLeft, labelWidth, rowHeight);
    scKneeSlider.setBounds(leftColumn.getX() + labelWidth, yPosLeft, sliderWidth, rowHeight);
    yPosLeft += rowHeight + sectionGap;

    // === LIMITERS SECTION ===
    limitersLabel.setBounds(leftColumn.getX(), yPosLeft, leftColumn.getWidth(), headerHeight);
    yPosLeft += headerHeight + 8;

    limitRelLabel.setBounds(leftColumn.getX(), yPosLeft, labelWidth, rowHeight);
    limitRelSlider.setBounds(leftColumn.getX() + labelWidth, yPosLeft, sliderWidth, rowHeight);
    yPosLeft += rowHeight + 4;

    slLimitAttackLabel.setBounds(leftColumn.getX(), yPosLeft, labelWidth, rowHeight);
    slLimitAttackSlider.setBounds(leftColumn.getX() + labelWidth, yPosLeft, sliderWidth, rowHeight);
    yPosLeft += rowHeight + 4;

    flLimitAttackLabel.setBounds(leftColumn.getX(), yPosLeft, labelWidth, rowHeight);
    flLimitAttackSlider.setBounds(leftColumn.getX() + labelWidth, yPosLeft, sliderWidth, rowHeight);
    yPosLeft += rowHeight + sectionGap;

    // === GAIN COMPENSATION SECTION ===
    compLabel.setBounds(leftColumn.getX(), yPosLeft, leftColumn.getWidth(), headerHeight);
    yPosLeft += headerHeight + 8;

    // Vertical 2x2 grid for better clarity
    const int compButtonWidth = 55;
    const int compButtonHeight = 22;
    const int compGap = 6;

    // Top row: HC and SC
    int xPosComp = leftColumn.getX();
    hcCompButton.setBounds(xPosComp, yPosLeft, compButtonWidth, compButtonHeight);
    xPosComp += compButtonWidth + compGap;
    scCompButton.setBounds(xPosComp, yPosLeft, compButtonWidth, compButtonHeight);

    yPosLeft += compButtonHeight + compGap;

    // Bottom row: SL and FL
    xPosComp = leftColumn.getX();
    slCompButton.setBounds(xPosComp, yPosLeft, compButtonWidth, compButtonHeight);
    xPosComp += compButtonWidth + compGap;
    flCompButton.setBounds(xPosComp, yPosLeft, compButtonWidth, compButtonHeight);

    // ========== RIGHT COLUMN - ENVELOPE SHAPING ==========
    int yPosRight = 0;

    envelopeLabel.setBounds(rightColumn.getX(), yPosRight, rightColumn.getWidth(), headerHeight);
    yPosRight += headerHeight + 8;

    // Envelope layout in 2x2 grid matching XY pad layout:
    // Row 1: HC (top-left) | FL (top-right)
    // Row 2: SC (bottom-left) | SL (bottom-right)
    const int knobSize = 36;
    const int knobSpacing = 8;
    const int envLabelWidth = 30;
    const int labelToKnobGap = 5;
    const int envColumnGap = 12;
    const int rowGap = 10;

    const int envHalfWidth = (rightColumn.getWidth() - envColumnGap) / 2;

    // Row 1: HC (left) and FL (right)
    int currentY = yPosRight;

    // HC (Top-Left)
    int xPosEnv = rightColumn.getX();
    hcAttackLabel.setBounds(xPosEnv, currentY, envLabelWidth, 14);
    hcAttackSlider.setBounds(xPosEnv + envLabelWidth + labelToKnobGap, currentY, knobSize, knobSize + 16);
    hcSustainSlider.setBounds(xPosEnv + envLabelWidth + labelToKnobGap + knobSize + knobSpacing, currentY, knobSize, knobSize + 16);

    // FL (Top-Right)
    xPosEnv = rightColumn.getX() + envHalfWidth + envColumnGap;
    flAttackLabel.setBounds(xPosEnv, currentY, envLabelWidth, 14);
    flAttackSlider.setBounds(xPosEnv + envLabelWidth + labelToKnobGap, currentY, knobSize, knobSize + 16);
    flSustainSlider.setBounds(xPosEnv + envLabelWidth + labelToKnobGap + knobSize + knobSpacing, currentY, knobSize, knobSize + 16);

    currentY += knobSize + 16 + rowGap;

    // Row 2: SC (left) and SL (right)

    // SC (Bottom-Left)
    xPosEnv = rightColumn.getX();
    scAttackLabel.setBounds(xPosEnv, currentY, envLabelWidth, 14);
    scAttackSlider.setBounds(xPosEnv + envLabelWidth + labelToKnobGap, currentY, knobSize, knobSize + 16);
    scSustainSlider.setBounds(xPosEnv + envLabelWidth + labelToKnobGap + knobSize + knobSpacing, currentY, knobSize, knobSize + 16);

    // SL (Bottom-Right)
    xPosEnv = rightColumn.getX() + envHalfWidth + envColumnGap;
    slAttackLabel.setBounds(xPosEnv, currentY, envLabelWidth, 14);
    slAttackSlider.setBounds(xPosEnv + envLabelWidth + labelToKnobGap, currentY, knobSize, knobSize + 16);
    slSustainSlider.setBounds(xPosEnv + envLabelWidth + labelToKnobGap + knobSize + knobSpacing, currentY, knobSize, knobSize + 16);

    // Hide sustain labels (using attack label for both columns)
    hcSustainLabel.setBounds(0, 0, 0, 0);
    scSustainLabel.setBounds(0, 0, 0, 0);
    slSustainLabel.setBounds(0, 0, 0, 0);
    flSustainLabel.setBounds(0, 0, 0, 0);
}

void AdvancedPanel::setVisible(bool shouldBeVisible)
{
    Component::setVisible(shouldBeVisible);
}

//==============================================================================
// Editor Implementation
//==============================================================================
QuadBlendDriveAudioProcessorEditor::QuadBlendDriveAudioProcessorEditor(QuadBlendDriveAudioProcessor& p)
    : AudioProcessorEditor(&p),
      audioProcessor(p),
      xyPad(p.apvts, "XY_X_PARAM", "XY_Y_PARAM", p),
      calibrationPanel(p.apvts, p),
      advancedPanel(p.apvts),
      oscilloscope(p),
      transferCurveMeter(p),  // Transfer curve with level meter
      inputMeter(p, true),    // Stereo input meter
      outputMeter(p, false)   // Stereo output meter
{
    // Apply custom look and feel
    setLookAndFeel(&lookAndFeel);

    setSize(950, 850);  // Default size
    setResizable(true, true);
    setResizeLimits(800, 600, 1900, 1700);  // Min 800x600, Max 2x default size

    // Setup XY Pad
    addAndMakeVisible(xyPad);

    // Setup Visualizations (these go first so panels can overlay)
    addAndMakeVisible(oscilloscope);
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

    // Setup Calibration Panel
    calibrationToggleButton.setButtonText("CALIBRATION");
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

    // Setup Advanced Panel (added AFTER waveform so it appears on top)
    advancedToggleButton.setButtonText("ADVANCED");
    advancedToggleButton.setClickingTogglesState(false);
    advancedToggleButton.onClick = [this]() {
        advancedPanel.setVisible(!advancedPanel.isVisible());
        resized();  // Re-layout when panel visibility changes
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
        label.setFont(juce::Font(12.0f));
        // Don't attach - we'll position manually
        addAndMakeVisible(label);
    };

    setupRotarySlider(inputGainSlider, inputGainLabel, "Input");
    setupRotarySlider(outputGainSlider, outputGainLabel, "Output");
    setupRotarySlider(mixSlider, mixLabel, "Mix");

    // Threshold Control (Calib moved to CalibrationPanel)
    setupRotarySlider(thresholdSlider, thresholdLabel, "Thresh");

    // Threshold Link Button - Toggle between automatic tracking and manual control
    thresholdLinkButton.setButtonText("LINK");
    thresholdLinkButton.setClickingTogglesState(true);
    thresholdLinkButton.setToggleState(true, juce::dontSendNotification);  // Default ON
    thresholdLinkButton.setColour(juce::TextButton::buttonColourId, juce::Colour(35, 35, 40));
    thresholdLinkButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(50, 150, 255));  // Blue when linked
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

    setupTrimSlider(hcTrimSlider, hcTrimLabel, "Hard");
    setupTrimSlider(scTrimSlider, scTrimLabel, "Soft");
    setupTrimSlider(slTrimSlider, slTrimLabel, "Slow");
    setupTrimSlider(flTrimSlider, flTrimLabel, "Fast");

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
        bool deltaModeOn = audioProcessor.apvts.getRawParameterValue("DELTA_MODE")->load() > 0.5f;

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

    // Setup Master Gain Compensation Toggle - Hidden but functional (always enabled)
    masterCompButton.setButtonText("GAIN COMP");
    masterCompButton.setClickingTogglesState(true);
    masterCompButton.setColour(juce::TextButton::buttonColourId, juce::Colour(35, 35, 40));
    masterCompButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(80, 220, 120));
    masterCompButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.85f));
    masterCompButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    addChildComponent(masterCompButton);
    masterCompButton.setVisible(false);  // Hidden - always enabled

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

    // Setup Preset Buttons (A, B, C, D) - Modern styling
    auto setupPresetButton = [this](juce::TextButton& recallBtn, juce::TextButton& saveBtn, const juce::String& label, int slot)
    {
        recallBtn.setButtonText(label);
        recallBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(45, 45, 52));
        recallBtn.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.9f));
        recallBtn.onClick = [this, slot] { audioProcessor.recallPreset(slot); };
        addAndMakeVisible(recallBtn);

        saveBtn.setButtonText("Save " + label);
        saveBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(35, 35, 40));
        saveBtn.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.65f));
        saveBtn.onClick = [this, slot] { audioProcessor.savePreset(slot); };
        addAndMakeVisible(saveBtn);
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
    versionLabel.setText("v1.8.5", juce::dontSendNotification);
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
    bool deltaModeOn = p.apvts.getRawParameterValue("DELTA_MODE")->load() > 0.5f;
    if (auto* masterCompParam = p.apvts.getParameter("MASTER_COMP"))
        masterCompParam->setValueNotifyingHost(deltaModeOn ? 1.0f : 0.0f);

    // Add CalibrationPanel LAST so it appears on top of all other controls
    addChildComponent(calibrationPanel);  // Use addChildComponent to keep panel hidden initially
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

    // Update calibration toggle button based on normalization state
    // Visual feedback shows normalization status even when panel is collapsed
    // Only lights up when Normalize button is actually toggled ON (not just when analyzed)
    bool normalizationEnabled = calibrationPanel.isNormalizationEnabled();
    if (normalizationEnabled)
    {
        // NORMALIZATION ON: Active/lit state with bright accent color and indicator
        calibrationToggleButton.setButtonText("● CALIBRATION");  // Indicator dot
        calibrationToggleButton.setColour(juce::TextButton::buttonColourId, juce::Colour(255, 140, 60));  // Bright orange (lit)
        calibrationToggleButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    }
    else
    {
        // NORMALIZATION OFF: Default/unlit state
        calibrationToggleButton.setButtonText("CALIBRATION");  // No indicator
        calibrationToggleButton.setColour(juce::TextButton::buttonColourId, juce::Colour(35, 35, 40));  // Dark grey (unlit)
        calibrationToggleButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.85f));
    }

    // === THRESHOLD LINK LOGIC ===
    // When linked, threshold automatically tracks calibration level
    bool thresholdLinked = thresholdLinkButton.getToggleState();
    if (thresholdLinked)
    {
        // Automatically sync threshold with calibration level
        float calibLevel = *audioProcessor.apvts.getRawParameterValue("CALIB_LEVEL");
        float currentThreshold = *audioProcessor.apvts.getRawParameterValue("THRESHOLD");

        // Only update if there's a meaningful difference (avoid constant updates)
        if (std::abs(currentThreshold - calibLevel) > 0.05f)
        {
            thresholdSlider.setValue(calibLevel, juce::dontSendNotification);
            audioProcessor.apvts.getParameter("THRESHOLD")->setValueNotifyingHost(
                audioProcessor.apvts.getParameter("THRESHOLD")->convertTo0to1(calibLevel));
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

void QuadBlendDriveAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Modern gradient background
    juce::ColourGradient bgGradient(juce::Colour(16, 16, 20), 0, 0,
                                     juce::Colour(12, 12, 16), 0, static_cast<float>(getHeight()), false);
    g.setGradientFill(bgGradient);
    g.fillAll();

    // Modern title with subtle shadow
    const float scale = getWidth() / 1120.0f;

    // Shadow/glow effect
    g.setColour(juce::Colour(255, 120, 50).withAlpha(0.15f));
    g.setFont(juce::Font(48.0f * scale, juce::Font::bold));  // 24 * 2 = 48
    g.drawText("S.T.E.V.E.", 0, static_cast<int>(11 * scale), getWidth(),
               static_cast<int>(65 * scale), juce::Justification::centred);

    // Main title
    g.setColour(juce::Colours::white.withAlpha(0.95f));
    g.setFont(juce::Font(48.0f * scale, juce::Font::plain));  // 24 * 2 = 48
    g.drawText("S.T.E.V.E.", 0, static_cast<int>(10 * scale), getWidth(),
               static_cast<int>(65 * scale), juce::Justification::centred);

    // Subtitle
    g.setColour(juce::Colours::white.withAlpha(0.6f));
    g.setFont(juce::Font(18.0f * scale, juce::Font::plain));  // 9 * 2 = 18
    g.drawText("Signal Transfer Enhanced Volume Engine", 0, static_cast<int>(55 * scale), getWidth(),
               static_cast<int>(32 * scale), juce::Justification::centred);

    // Window size display (upper right corner, above version)
    g.setColour(juce::Colour(255, 120, 50).withAlpha(0.8f));
    g.setFont(juce::Font(12.0f * scale, juce::Font::bold));
    juce::String sizeText = juce::String(getWidth()) + " × " + juce::String(getHeight());
    g.drawText(sizeText, getWidth() - static_cast<int>(120 * scale), static_cast<int>(5 * scale),
               static_cast<int>(110 * scale), static_cast<int>(16 * scale), juce::Justification::centredRight);
}

void QuadBlendDriveAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    const float scale = getWidth() / 1120.0f;  // Scale factor based on default width

    // Proportional title area (increased for 2x larger title/subtitle)
    auto titleArea = bounds.removeFromTop(static_cast<int>(95 * scale));

    // Version label in upper right corner (below window size)
    versionLabel.setBounds(titleArea.getWidth() - static_cast<int>(80 * scale),
                          static_cast<int>(22 * scale),
                          static_cast<int>(70 * scale),
                          static_cast<int>(16 * scale));

    // Calibration Toggle Button in upper left corner
    const int calibButtonWidth = static_cast<int>(100 * scale);
    const int calibButtonHeight = static_cast<int>(25 * scale);
    calibrationToggleButton.setBounds(static_cast<int>(10 * scale),
                                     static_cast<int>(10 * scale),
                                     calibButtonWidth,
                                     calibButtonHeight);

    // Calibration Panel - Below toggle button when expanded
    if (calibrationPanel.isVisible())
    {
        const int panelWidth = static_cast<int>(250 * scale);
        const int panelHeight = 200;
        calibrationPanel.setBounds(static_cast<int>(10 * scale),
                                  static_cast<int>(10 * scale) + calibButtonHeight + static_cast<int>(5 * scale),
                                  panelWidth,
                                  panelHeight);
    }

    const int padding = static_cast<int>(20 * scale);
    const int knobSize = static_cast<int>(70 * scale);
    const int spacing = static_cast<int>(10 * scale);

    // Reserve bottom for waveform display FIRST - FULL WIDTH
    // Combined height: previous meterArea (120) + spacing (10) + oscilloscope (250) = 380 * scale
    auto oscilloscopeArea = bounds.removeFromBottom(static_cast<int>(380 * scale));
    bounds.removeFromBottom(spacing);

    // Now reserve right edge for input/output meters (from remaining height)
    const int meterWidth = static_cast<int>(49 * scale);  // 30% slimmer than before (70 * 0.7 = 49)
    const int meterGap = static_cast<int>(8 * scale);
    const int totalMeterWidth = (meterWidth * 2) + meterGap;  // Two meters plus gap
    auto meterArea = bounds.removeFromRight(totalMeterWidth);
    bounds.removeFromRight(spacing);  // Gap between meters and main content

    // Proportional three-column layout: LEFT (Input) | CENTER (XY) | RIGHT (Output)
    const int sideColumnWidth = static_cast<int>(300 * scale);
    const int centerColumnWidth = static_cast<int>(520 * scale);

    // ========== LEFT SECTION (INPUT CONTROLS) ==========
    auto leftSection = bounds.removeFromLeft(sideColumnWidth).reduced(padding, 0);

    // Input Gain
    auto inputRow = leftSection.removeFromTop(knobSize);
    inputGainLabel.setBounds(inputRow.removeFromLeft(static_cast<int>(60 * scale)).removeFromTop(knobSize));
    inputGainSlider.setBounds(inputRow.removeFromLeft(knobSize));
    leftSection.removeFromTop(spacing);

    // Threshold (Calib controls moved to CalibrationPanel)
    auto threshRow = leftSection.removeFromTop(knobSize);
    thresholdLabel.setBounds(threshRow.removeFromLeft(static_cast<int>(60 * scale)).removeFromTop(knobSize));
    thresholdSlider.setBounds(threshRow.removeFromLeft(knobSize));
    threshRow.removeFromLeft(spacing / 2);  // Small gap between knob and button
    thresholdLinkButton.setBounds(threshRow.removeFromLeft(static_cast<int>(42 * scale)).withHeight(static_cast<int>(22 * scale)));
    leftSection.removeFromTop(spacing * 2);

    // 4 Processor Trims - [Label][Knob][Mute]
    // Order (top to bottom): SL, SC, HC, FL (Column-based: Left column, then Right column)
    const int trimRowHeight = static_cast<int>(70 * scale);
    const int trimKnobSize = static_cast<int>(65 * scale);
    const int buttonSize = static_cast<int>(28 * scale);
    const int trimLabelWidth = static_cast<int>(48 * scale);
    const int trimTopMargin = static_cast<int>(20 * scale);

    // SL Trim - [Label][Knob][Mute][Solo] (Top-Left quadrant)
    auto slRow = leftSection.removeFromTop(trimRowHeight);
    slTrimLabel.setBounds(slRow.removeFromLeft(trimLabelWidth).withTrimmedTop(trimTopMargin));
    slTrimSlider.setBounds(slRow.removeFromLeft(trimKnobSize));
    slRow.removeFromLeft(spacing);
    slMuteButton.setBounds(slRow.removeFromLeft(buttonSize).withTrimmedTop(trimTopMargin));
    slRow.removeFromLeft(spacing / 2);  // Smaller spacing between M and S buttons
    slSoloButton.setBounds(slRow.removeFromLeft(buttonSize).withTrimmedTop(trimTopMargin));
    leftSection.removeFromTop(spacing);

    // SC Trim - [Label][Knob][Mute][Solo] (Bottom-Left quadrant)
    auto scRow = leftSection.removeFromTop(trimRowHeight);
    scTrimLabel.setBounds(scRow.removeFromLeft(trimLabelWidth).withTrimmedTop(trimTopMargin));
    scTrimSlider.setBounds(scRow.removeFromLeft(trimKnobSize));
    scRow.removeFromLeft(spacing);
    scMuteButton.setBounds(scRow.removeFromLeft(buttonSize).withTrimmedTop(trimTopMargin));
    scRow.removeFromLeft(spacing / 2);  // Smaller spacing between M and S buttons
    scSoloButton.setBounds(scRow.removeFromLeft(buttonSize).withTrimmedTop(trimTopMargin));
    leftSection.removeFromTop(spacing);

    // HC Trim - [Label][Knob][Mute][Solo] (Top-Right quadrant)
    auto hcRow = leftSection.removeFromTop(trimRowHeight);
    hcTrimLabel.setBounds(hcRow.removeFromLeft(trimLabelWidth).withTrimmedTop(trimTopMargin));
    hcTrimSlider.setBounds(hcRow.removeFromLeft(trimKnobSize));
    hcRow.removeFromLeft(spacing);
    hcMuteButton.setBounds(hcRow.removeFromLeft(buttonSize).withTrimmedTop(trimTopMargin));
    hcRow.removeFromLeft(spacing / 2);  // Smaller spacing between M and S buttons
    hcSoloButton.setBounds(hcRow.removeFromLeft(buttonSize).withTrimmedTop(trimTopMargin));
    leftSection.removeFromTop(spacing);

    // FL Trim - [Label][Knob][Mute][Solo] (Bottom-Right quadrant)
    auto flRow = leftSection.removeFromTop(trimRowHeight);
    flTrimLabel.setBounds(flRow.removeFromLeft(trimLabelWidth).withTrimmedTop(trimTopMargin));
    flTrimSlider.setBounds(flRow.removeFromLeft(trimKnobSize));
    flRow.removeFromLeft(spacing);
    flMuteButton.setBounds(flRow.removeFromLeft(buttonSize).withTrimmedTop(trimTopMargin));
    flRow.removeFromLeft(spacing / 2);  // Smaller spacing between M and S buttons
    flSoloButton.setBounds(flRow.removeFromLeft(buttonSize).withTrimmedTop(trimTopMargin));
    leftSection.removeFromTop(spacing);

    // Reset Trims button (50% smaller)
    resetTrimsButton.setBounds(leftSection.removeFromTop(static_cast<int>(28 * scale))
        .withSizeKeepingCentre(static_cast<int>(60 * scale), static_cast<int>(22 * scale)));

    // ========== CENTER SECTION (XY PAD) - CENTERED AND SQUARE ==========
    auto centerSection = bounds.removeFromLeft(centerColumnWidth).reduced(padding, 0);

    // Processing Mode Selector above XY Pad (centered over XY grid)
    auto processingModeRow = centerSection.removeFromTop(static_cast<int>(32 * scale));
    int processingModeWidth = static_cast<int>(280 * scale);
    int processingModeLabelWidth = static_cast<int>(60 * scale);
    int processingModeComboWidth = static_cast<int>(160 * scale);
    int processingModeXOffset = (processingModeRow.getWidth() - processingModeWidth) / 2;

    processingModeLabel.setBounds(processingModeRow.getX() + processingModeXOffset,
                                  processingModeRow.getY(),
                                  processingModeLabelWidth,
                                  static_cast<int>(28 * scale));
    processingModeCombo.setBounds(processingModeRow.getX() + processingModeXOffset + processingModeLabelWidth + static_cast<int>(8 * scale),
                                  processingModeRow.getY(),
                                  processingModeComboWidth,
                                  static_cast<int>(28 * scale));

    centerSection.removeFromTop(static_cast<int>(12 * scale));  // Gap below Processing Mode selector

    // Preset buttons above XY Pad in a single row (proportionally scaled and centered)
    const int presetButtonWidth = static_cast<int>(42 * scale);
    const int saveButtonWidth = static_cast<int>(56 * scale);
    const int presetButtonHeight = static_cast<int>(25 * scale);
    const int presetSpacing = static_cast<int>(4 * scale);

    // Calculate total width of all preset buttons
    int totalPresetWidth = (presetButtonWidth + presetSpacing + saveButtonWidth + presetSpacing * 2) * 4 - presetSpacing * 2;

    auto presetRow = centerSection.removeFromTop(presetButtonHeight);
    int presetXStart = presetRow.getX() + (presetRow.getWidth() - totalPresetWidth) / 2;

    // Single row: A, Save A, B, Save B, C, Save C, D, Save D (centered)
    int xPos = presetXStart;

    presetButtonA.setBounds(xPos, presetRow.getY(), presetButtonWidth, presetButtonHeight);
    xPos += presetButtonWidth + presetSpacing;
    saveButtonA.setBounds(xPos, presetRow.getY(), saveButtonWidth, presetButtonHeight);
    xPos += saveButtonWidth + presetSpacing * 2;

    presetButtonB.setBounds(xPos, presetRow.getY(), presetButtonWidth, presetButtonHeight);
    xPos += presetButtonWidth + presetSpacing;
    saveButtonB.setBounds(xPos, presetRow.getY(), saveButtonWidth, presetButtonHeight);
    xPos += saveButtonWidth + presetSpacing * 2;

    presetButtonC.setBounds(xPos, presetRow.getY(), presetButtonWidth, presetButtonHeight);
    xPos += presetButtonWidth + presetSpacing;
    saveButtonC.setBounds(xPos, presetRow.getY(), saveButtonWidth, presetButtonHeight);
    xPos += saveButtonWidth + presetSpacing * 2;

    presetButtonD.setBounds(xPos, presetRow.getY(), presetButtonWidth, presetButtonHeight);
    xPos += presetButtonWidth + presetSpacing;
    saveButtonD.setBounds(xPos, presetRow.getY(), saveButtonWidth, presetButtonHeight);

    centerSection.removeFromTop(static_cast<int>(15 * scale));  // Gap below preset buttons

    // Make XY Pad SQUARE - use the smaller dimension
    int availableHeight = centerSection.getHeight();
    int availableWidth = centerSection.getWidth();
    int xySize = juce::jmin(availableWidth, availableHeight);

    // Center the square XY pad both horizontally and vertically
    int xOffset = (availableWidth - xySize) / 2;
    int yOffset = (availableHeight - xySize) / 2;
    xyPad.setBounds(centerSection.getX() + xOffset, centerSection.getY() + yOffset, xySize, xySize);

    // Advanced Toggle Button - Centered below XY Pad
    const int advancedButtonHeight = static_cast<int>(30 * scale);
    const int advancedButtonWidth = static_cast<int>(120 * scale);
    int advancedButtonY = centerSection.getY() + yOffset + xySize + static_cast<int>(10 * scale);
    int advancedButtonX = centerSection.getX() + (availableWidth - advancedButtonWidth) / 2;
    advancedToggleButton.setBounds(advancedButtonX, advancedButtonY, advancedButtonWidth, advancedButtonHeight);

    // Advanced Panel - Overlays waveform meter when visible
    // Positioned absolutely to overlay the bottom section
    if (advancedPanel.isVisible())
    {
        const int advancedPanelWidth = static_cast<int>(600 * scale);  // Wider for two-column layout
        const int advancedPanelHeight = static_cast<int>(312 * scale);  // 50% larger than previous (208 * 1.5)
        int advancedPanelY = advancedButtonY + advancedButtonHeight + static_cast<int>(8 * scale);
        int advancedPanelX = centerSection.getX() + (availableWidth - advancedPanelWidth) / 2;
        advancedPanel.setBounds(advancedPanelX, advancedPanelY, advancedPanelWidth, advancedPanelHeight);
    }
    else
    {
        advancedPanel.setBounds(0, 0, 0, 0);  // Hide when not visible
    }

    // ========== RIGHT SECTION (OUTPUT CONTROLS) ==========
    auto rightSection = bounds.reduced(padding, 0);

    // Output Gain
    auto outputRow = rightSection.removeFromTop(knobSize);
    outputGainLabel.setBounds(outputRow.removeFromLeft(static_cast<int>(60 * scale)).removeFromTop(knobSize));
    outputGainSlider.setBounds(outputRow.removeFromLeft(knobSize));
    rightSection.removeFromTop(spacing);

    // Consistent button sizing for Link I/O, Bypass, Gain Comp, Delta Mode
    const int buttonWidth = static_cast<int>(95 * scale);
    const int buttonHeight = static_cast<int>(28 * scale);

    // Link I/O button
    gainLinkButton.setBounds(rightSection.removeFromTop(buttonHeight)
        .withSizeKeepingCentre(buttonWidth, buttonHeight));
    rightSection.removeFromTop(spacing);

    // Bypass button
    bypassButton.setBounds(rightSection.removeFromTop(buttonHeight)
        .withSizeKeepingCentre(buttonWidth, buttonHeight));
    rightSection.removeFromTop(spacing);

    // Master Gain Compensation button (hidden - always enabled)
    // masterCompButton.setBounds(...) - removed, button is hidden

    // Delta Mode button
    deltaModeButton.setBounds(rightSection.removeFromTop(buttonHeight)
        .withSizeKeepingCentre(buttonWidth, buttonHeight));
    rightSection.removeFromTop(spacing * 2);

    // O/S Delta button removed - functionality integrated into main delta mode

    // TP Delta disabled
    // truePeakDeltaModeButton.setBounds(rightSection.removeFromTop(buttonHeight)
    //     .withSizeKeepingCentre(buttonWidth, buttonHeight));
    // rightSection.removeFromTop(spacing);

    // Mix
    auto mixRow = rightSection.removeFromTop(knobSize);
    mixLabel.setBounds(mixRow.removeFromLeft(static_cast<int>(60 * scale)).removeFromTop(knobSize));
    mixSlider.setBounds(mixRow.removeFromLeft(knobSize));
    rightSection.removeFromTop(spacing * 2);

    // === OUTPUT LIMITER CONTROLS (Simplified) ===
    rightSection.removeFromTop(spacing);

    // Output Ceiling (shared by both limiters)
    auto ceilingRow = rightSection.removeFromTop(knobSize);
    outputCeilingLabel.setBounds(ceilingRow.removeFromLeft(static_cast<int>(60 * scale)).removeFromTop(knobSize));
    outputCeilingSlider.setBounds(ceilingRow.removeFromLeft(knobSize));
    rightSection.removeFromTop(spacing);

    // Overshoot Enable Button
    auto overshootRow = rightSection.removeFromTop(static_cast<int>(28 * scale));
    overshootEnableButton.setBounds(overshootRow.removeFromLeft(static_cast<int>(95 * scale)));
    rightSection.removeFromTop(spacing);

    // True Peak Enable Button
    auto truePeakRow = rightSection.removeFromTop(static_cast<int>(28 * scale));
    truePeakEnableButton.setBounds(truePeakRow.removeFromLeft(static_cast<int>(95 * scale)));
    rightSection.removeFromTop(spacing);

    // ========== RIGHT EDGE METERS (INPUT/OUTPUT) ==========
    // Position input and output meters side-by-side from right edge: [IN] [OUT]
    // Signal flow reads left to right
    outputMeter.setBounds(meterArea.removeFromRight(meterWidth));
    meterArea.removeFromRight(meterGap);  // Gap between meters
    inputMeter.setBounds(meterArea.removeFromRight(meterWidth));

    // ========== BOTTOM (WAVEFORM) - FULL WIDTH ==========
    oscilloscope.setBounds(oscilloscopeArea.reduced(padding, 0));

    // Transfer Curve toggle button and meter (hidden, not used in current layout)
    transferCurveToggleButton.setBounds(0, 0, 0, 0);
    transferCurveMeter.setBounds(0, 0, 0, 0);
}

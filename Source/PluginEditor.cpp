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

    // Scale factor based on current height (base height is 180)
    const float scale = height / 180.0f;

    // Modern gradient background
    juce::ColourGradient gradient(juce::Colour(22, 22, 26), bounds.getX(), bounds.getY(),
                                   juce::Colour(18, 18, 22), bounds.getX(), bounds.getBottom(), false);
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(bounds, 6.0f * scale);

    // Sleek title
    g.setColour(juce::Colours::white.withAlpha(0.85f));
    g.setFont(juce::Font(10.0f * scale, juce::Font::plain));
    juce::Rectangle<float> titleArea(bounds.getX(), bounds.getY() + 6 * scale, width, 16.0f * scale);
    g.drawText("GAIN REDUCTION", titleArea, juce::Justification::centred);

    juce::Rectangle<float> graphBounds(bounds.getX() + 28 * scale, bounds.getY() + 26 * scale,
                                        width - 34 * scale, height - 30 * scale);

    // Detailed grid lines every 3dB
    g.setFont(juce::Font(8.0f * scale, juce::Font::plain));

    // Draw grid lines from -12 to +3 dB (every 3dB)
    for (int db = -12; db <= 3; db += 3)
    {
        float y = graphBounds.getBottom() - ((db + 12.0f) / 15.0f) * graphBounds.getHeight();

        // Major grid lines at 0, -6, -12
        if (db == 0 || db == -6 || db == -12)
        {
            g.setColour(juce::Colour(50, 50, 55).withAlpha(0.4f));
            g.drawLine(graphBounds.getX(), y, graphBounds.getRight(), y, 1.0f * scale);

            // Labels
            g.setColour(juce::Colours::white.withAlpha(0.7f));
            juce::String label = (db > 0 ? "+" : "") + juce::String(db);
            g.drawText(label, bounds.getX() + 3 * scale, y - 7 * scale, 24 * scale, 14 * scale,
                      juce::Justification::centredLeft);
        }
        // Minor grid lines
        else
        {
            g.setColour(juce::Colour(50, 50, 55).withAlpha(0.15f));
            g.drawLine(graphBounds.getX(), y, graphBounds.getRight(), y, 0.5f * scale);

            // Subtle labels for +3, -3, -9
            g.setColour(juce::Colours::white.withAlpha(0.4f));
            juce::String label = (db > 0 ? "+" : "") + juce::String(db);
            g.drawText(label, bounds.getX() + 3 * scale, y - 7 * scale, 24 * scale, 14 * scale,
                      juce::Justification::centredLeft);
        }
    }

    // Draw GR history path with gradient effect
    juce::Path grPath;
    bool pathStarted = false;

    for (int i = 0; i < historySize; ++i)
    {
        int idx = (historyIndex + i) % historySize;
        float grDB = grHistory[idx];

        float x = graphBounds.getX() + (i / float(historySize - 1)) * graphBounds.getWidth();
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

    // Modern gradient stroke
    g.setColour(juce::Colour(255, 120, 50));
    g.strokePath(grPath, juce::PathStrokeType(2.0f * scale, juce::PathStrokeType::curved));

    // Clean outer border
    g.setColour(juce::Colour(60, 60, 65).withAlpha(0.4f));
    g.drawRoundedRectangle(bounds.reduced(0.5f * scale), 6.0f * scale, 1.0f * scale);
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
            int readPos = (writePos + bufferIndex) % bufferSize;

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
            int readPos = (writePos + bufferIndex) % bufferSize;

            if (readPos >= 0 && readPos < bufferSize)
            {
                // Get three-band amplitudes for RGB coloring
                float lowAmp = processor.oscilloscopeLowBand[readPos];     // Red channel
                float midAmp = processor.oscilloscopeMidBand[readPos];     // Green channel
                float highAmp = processor.oscilloscopeHighBand[readPos];   // Blue channel

                // Scale and normalize the RGB values
                float colorScale = 3.0f;
                uint8_t red = static_cast<uint8_t>(juce::jlimit(0.0f, 255.0f, lowAmp * colorScale * 255.0f));
                uint8_t green = static_cast<uint8_t>(juce::jlimit(0.0f, 255.0f, midAmp * colorScale * 255.0f));
                uint8_t blue = static_cast<uint8_t>(juce::jlimit(0.0f, 255.0f, highAmp * colorScale * 255.0f));

                juce::Colour lineColor(red, green, blue);
                if (lineColor.getBrightness() < 0.3f)
                    lineColor = lineColor.brighter(0.3f);

                // Create segment path
                juce::Path segmentPath;
                bool segStarted = false;
                for (int i = startIdx; i <= endIdx && i < displayPoints; ++i)
                {
                    int bufIdx = (i * bufferSize) / displayPoints;
                    int rPos = (writePos + bufIdx) % bufferSize;

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

                // Draw glow layer
                g.setColour(lineColor.withAlpha(0.2f));
                g.strokePath(segmentPath, juce::PathStrokeType(4.0f * scale, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

                // Draw main line
                g.setColour(lineColor.withAlpha(0.85f));
                g.strokePath(segmentPath, juce::PathStrokeType(2.0f * scale, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }
        }
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
    repaint();
}

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

    // Top-left: Slow Limit
    juce::Rectangle<float> slowLimitArea(10, 10, width / 2.0f - 20, 16);
    g.drawText("SLOW", slowLimitArea, juce::Justification::centredLeft);

    // Top-right: Hard Clip
    juce::Rectangle<float> hardClipArea(width / 2.0f + 10, 10, width / 2.0f - 20, 16);
    g.drawText("HARD", hardClipArea, juce::Justification::centredRight);

    // Bottom-left: Soft Clip
    juce::Rectangle<float> softClipArea(10, height - 24, width / 2.0f - 20, 16);
    g.drawText("SOFT", softClipArea, juce::Justification::centredLeft);

    // Bottom-right: Fast Limit
    juce::Rectangle<float> fastLimitArea(width / 2.0f + 10, height - 24, width / 2.0f - 20, 16);
    g.drawText("FAST", fastLimitArea, juce::Justification::centredRight);

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
// Editor Implementation
//==============================================================================
QuadBlendDriveAudioProcessorEditor::QuadBlendDriveAudioProcessorEditor(QuadBlendDriveAudioProcessor& p)
    : AudioProcessorEditor(&p),
      audioProcessor(p),
      xyPad(p.apvts, "XY_X_PARAM", "XY_Y_PARAM", p),
      grMeter(p),
      oscilloscope(p),
      masterMeter(p)
{
    setSize(850, 900);  // Default size
    setResizable(true, true);
    setResizeLimits(800, 600, 1700, 1800);  // Min 800x600, Max 2x default size

    // Setup XY Pad
    addAndMakeVisible(xyPad);

    // Setup Visualizations
    addAndMakeVisible(grMeter);
    addAndMakeVisible(oscilloscope);
    addAndMakeVisible(masterMeter);

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
    setupRotarySlider(calibLevelSlider, calibLevelLabel, "Calib");
    setupRotarySlider(thresholdSlider, thresholdLabel, "Thresh");

    // Normalization Buttons - Modern styling
    analyzeButton.setButtonText("ANALYZE");
    analyzeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(35, 35, 40));
    analyzeButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.85f));
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
    normalizeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(35, 35, 40));
    normalizeButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(255, 120, 50));
    normalizeButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.85f));
    normalizeButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    normalizeButton.onClick = [this]()
    {
        if (normalizeButton.getToggleState())
            audioProcessor.enableNormalization();
        else
            audioProcessor.disableNormalization();
    };
    addAndMakeVisible(normalizeButton);

    resetNormButton.setButtonText("RESET");
    resetNormButton.setColour(juce::TextButton::buttonColourId, juce::Colour(35, 35, 40));
    resetNormButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.85f));
    resetNormButton.onClick = [this]()
    {
        audioProcessor.resetNormalizationData();
        normalizeButton.setToggleState(false, juce::dontSendNotification);
        analyzeButton.setButtonText("ANALYZE");
    };
    addAndMakeVisible(resetNormButton);

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

    // Normalization Display Labels - Modern styling
    inputPeakLabel.setText("INPUT PEAK: -- dB", juce::dontSendNotification);
    inputPeakLabel.setJustificationType(juce::Justification::centred);
    inputPeakLabel.setColour(juce::Label::backgroundColourId, juce::Colour(28, 28, 32));
    inputPeakLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.9f));
    inputPeakLabel.setColour(juce::Label::outlineColourId, juce::Colour(60, 60, 65).withAlpha(0.3f));
    addAndMakeVisible(inputPeakLabel);

    // Gain Applied - label and copyable value
    gainAppliedLabel.setText("GAIN APPLIED:", juce::dontSendNotification);
    gainAppliedLabel.setJustificationType(juce::Justification::centredLeft);
    gainAppliedLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.75f));
    addAndMakeVisible(gainAppliedLabel);

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
    addAndMakeVisible(deltaModeButton);

    overshootDeltaModeButton.setButtonText("O/S DELTA");
    overshootDeltaModeButton.setClickingTogglesState(true);
    overshootDeltaModeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(35, 35, 40));
    overshootDeltaModeButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(180, 100, 200));
    overshootDeltaModeButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.85f));
    overshootDeltaModeButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    addAndMakeVisible(overshootDeltaModeButton);

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

    // Setup Master Gain Compensation Toggle - Consistent button styling
    masterCompButton.setButtonText("GAIN COMP");
    masterCompButton.setClickingTogglesState(true);
    masterCompButton.setColour(juce::TextButton::buttonColourId, juce::Colour(35, 35, 40));
    masterCompButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(80, 220, 120));
    masterCompButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.85f));
    masterCompButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    addAndMakeVisible(masterCompButton);

    // Setup Toggle Buttons for Muting - Modern styling
    auto setupMuteButton = [](juce::ToggleButton& button)
    {
        button.setButtonText("M");
        button.setClickingTogglesState(true);
        // OFF state (not muted): subtle dark background
        button.setColour(juce::TextButton::buttonColourId, juce::Colour(35, 35, 40));
        button.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.5f));
        // ON state (muted): red with modern styling
        button.setColour(juce::TextButton::buttonOnColourId, juce::Colour(220, 50, 50));
        button.setColour(juce::TextButton::textColourOnId, juce::Colours::white.withAlpha(0.95f));
    };

    setupMuteButton(hcMuteButton);
    setupMuteButton(scMuteButton);
    setupMuteButton(slMuteButton);
    setupMuteButton(flMuteButton);

    addAndMakeVisible(hcMuteButton);
    addAndMakeVisible(scMuteButton);
    addAndMakeVisible(slMuteButton);
    addAndMakeVisible(flMuteButton);

    // Setup No OverShoot Limiter Controls - Modern styling (green when active)
    protectionEnableButton.setButtonText("NO OVERSHOOT");
    protectionEnableButton.setClickingTogglesState(true);
    protectionEnableButton.setColour(juce::TextButton::buttonColourId, juce::Colour(35, 35, 40));
    protectionEnableButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(80, 220, 120));
    protectionEnableButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.85f));
    protectionEnableButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    addAndMakeVisible(protectionEnableButton);

    setupRotarySlider(protectionCeilingSlider, protectionCeilingLabel, "Ceiling");

    // Setup Overshoot Character Blend Control
    setupRotarySlider(overshootBlendSlider, overshootBlendLabel, "Character");

    // OSM Mode Selector: OFF = Mode 0 (Safety Clipper), ON = Mode 1 (Advanced TPL)
    overshootModeButton.setButtonText("OSM: SAFE");  // OFF = "CLIP" mode, ON = "SAFE" mode
    overshootModeButton.setClickingTogglesState(true);
    overshootModeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(35, 35, 40));
    overshootModeButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(80, 180, 120));  // Green for "Safe" mode
    overshootModeButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.85f));
    overshootModeButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    addAndMakeVisible(overshootModeButton);

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
    versionLabel.setText("v1.8.0", juce::dontSendNotification);
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
    calibLevelAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        p.apvts, "CALIB_LEVEL", calibLevelSlider);
    thresholdAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        p.apvts, "THRESHOLD", thresholdSlider);

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
    overshootDeltaModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        p.apvts, "OVERSHOOT_DELTA_MODE", overshootDeltaModeButton);
    truePeakDeltaModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        p.apvts, "TRUE_PEAK_DELTA_MODE", truePeakDeltaModeButton);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        p.apvts, "BYPASS", bypassButton);
    masterCompAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        p.apvts, "MASTER_COMP", masterCompButton);
    protectionEnableAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        p.apvts, "PROTECTION_ENABLE", protectionEnableButton);
    protectionCeilingAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        p.apvts, "PROTECTION_CEILING", protectionCeilingSlider);
    overshootBlendAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        p.apvts, "OVERSHOOT_BLEND", overshootBlendSlider);
    overshootModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        p.apvts, "OSM_MODE", overshootModeButton);
    processingModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        p.apvts, "PROCESSING_MODE", processingModeCombo);

    // OSM_MODE note: false = Mode 0 (Safety Clipper), true = Mode 1 (Advanced TPL)
    // Both modes maintain constant latency via automatic compensation
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

    const int padding = static_cast<int>(20 * scale);
    const int knobSize = static_cast<int>(70 * scale);
    const int spacing = static_cast<int>(10 * scale);

    // Reserve right edge for master meter (wider for better visibility)
    const int masterMeterWidth = static_cast<int>(60 * scale);  // Increased from 35 to 60
    auto masterMeterArea = bounds.removeFromRight(masterMeterWidth);
    bounds.removeFromRight(spacing);  // Gap between meter and main content

    // Reserve bottom for visualizations - FULL WIDTH (reduced heights)
    auto oscilloscopeArea = bounds.removeFromBottom(static_cast<int>(250 * scale));  // Reduced from 400
    bounds.removeFromBottom(spacing);
    auto meterArea = bounds.removeFromBottom(static_cast<int>(120 * scale));  // Reduced from 180
    bounds.removeFromBottom(spacing);

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

    // Calib Level
    auto calibRow = leftSection.removeFromTop(knobSize);
    calibLevelLabel.setBounds(calibRow.removeFromLeft(static_cast<int>(60 * scale)).removeFromTop(knobSize));
    calibLevelSlider.setBounds(calibRow.removeFromLeft(knobSize));
    leftSection.removeFromTop(spacing);

    // Threshold
    auto threshRow = leftSection.removeFromTop(knobSize);
    thresholdLabel.setBounds(threshRow.removeFromLeft(static_cast<int>(60 * scale)).removeFromTop(knobSize));
    thresholdSlider.setBounds(threshRow.removeFromLeft(knobSize));
    leftSection.removeFromTop(spacing);

    // Calculate max width based on buttons below (Analyze + gap + Normalize + gap + Reset)
    const int buttonRowWidth = static_cast<int>(68 * scale) + static_cast<int>(6 * scale) +
                                 static_cast<int>(76 * scale) + static_cast<int>(6 * scale) +
                                 static_cast<int>(58 * scale);

    // Normalization displays - constrained to button width
    auto normDisplays = leftSection.removeFromTop(static_cast<int>(56 * scale));
    auto normDisplayArea = normDisplays.removeFromLeft(buttonRowWidth);

    inputPeakLabel.setBounds(normDisplayArea.removeFromTop(static_cast<int>(26 * scale)));
    normDisplayArea.removeFromTop(static_cast<int>(4 * scale));

    // Gain applied - label and value side by side
    auto gainRow = normDisplayArea.removeFromTop(static_cast<int>(26 * scale));
    gainAppliedLabel.setBounds(gainRow.removeFromLeft(static_cast<int>(110 * scale)));
    gainAppliedValue.setBounds(gainRow);

    leftSection.removeFromTop(spacing);

    // Normalization buttons (3 in a row)
    auto normButtons = leftSection.removeFromTop(static_cast<int>(28 * scale));
    analyzeButton.setBounds(normButtons.removeFromLeft(static_cast<int>(68 * scale)));
    normButtons.removeFromLeft(static_cast<int>(6 * scale));
    normalizeButton.setBounds(normButtons.removeFromLeft(static_cast<int>(76 * scale)));
    normButtons.removeFromLeft(static_cast<int>(6 * scale));
    resetNormButton.setBounds(normButtons.removeFromLeft(static_cast<int>(58 * scale)));

    leftSection.removeFromTop(spacing * 2);

    // 4 Processor Trims - [Label][Knob][Mute]
    // Order (top to bottom): SL, SC, HC, FL (Column-based: Left column, then Right column)
    const int trimRowHeight = static_cast<int>(70 * scale);
    const int trimKnobSize = static_cast<int>(65 * scale);
    const int buttonSize = static_cast<int>(28 * scale);
    const int trimLabelWidth = static_cast<int>(48 * scale);
    const int trimTopMargin = static_cast<int>(20 * scale);

    // SL Trim - [Label][Knob][Mute] (Top-Left quadrant)
    auto slRow = leftSection.removeFromTop(trimRowHeight);
    slTrimLabel.setBounds(slRow.removeFromLeft(trimLabelWidth).withTrimmedTop(trimTopMargin));
    slTrimSlider.setBounds(slRow.removeFromLeft(trimKnobSize));
    slRow.removeFromLeft(spacing);
    slMuteButton.setBounds(slRow.removeFromLeft(buttonSize).withTrimmedTop(trimTopMargin));
    leftSection.removeFromTop(spacing);

    // SC Trim - [Label][Knob][Mute] (Bottom-Left quadrant)
    auto scRow = leftSection.removeFromTop(trimRowHeight);
    scTrimLabel.setBounds(scRow.removeFromLeft(trimLabelWidth).withTrimmedTop(trimTopMargin));
    scTrimSlider.setBounds(scRow.removeFromLeft(trimKnobSize));
    scRow.removeFromLeft(spacing);
    scMuteButton.setBounds(scRow.removeFromLeft(buttonSize).withTrimmedTop(trimTopMargin));
    leftSection.removeFromTop(spacing);

    // HC Trim - [Label][Knob][Mute] (Top-Right quadrant)
    auto hcRow = leftSection.removeFromTop(trimRowHeight);
    hcTrimLabel.setBounds(hcRow.removeFromLeft(trimLabelWidth).withTrimmedTop(trimTopMargin));
    hcTrimSlider.setBounds(hcRow.removeFromLeft(trimKnobSize));
    hcRow.removeFromLeft(spacing);
    hcMuteButton.setBounds(hcRow.removeFromLeft(buttonSize).withTrimmedTop(trimTopMargin));
    leftSection.removeFromTop(spacing);

    // FL Trim - [Label][Knob][Mute] (Bottom-Right quadrant)
    auto flRow = leftSection.removeFromTop(trimRowHeight);
    flTrimLabel.setBounds(flRow.removeFromLeft(trimLabelWidth).withTrimmedTop(trimTopMargin));
    flTrimSlider.setBounds(flRow.removeFromLeft(trimKnobSize));
    flRow.removeFromLeft(spacing);
    flMuteButton.setBounds(flRow.removeFromLeft(buttonSize).withTrimmedTop(trimTopMargin));
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

    // Make XY Pad SQUARE - use the smaller dimension
    int availableHeight = centerSection.getHeight();
    int availableWidth = centerSection.getWidth();
    int xySize = juce::jmin(availableWidth, availableHeight);

    // Center the square XY pad both horizontally and vertically
    int xOffset = (availableWidth - xySize) / 2;
    int yOffset = (availableHeight - xySize) / 2;
    xyPad.setBounds(centerSection.getX() + xOffset, centerSection.getY() + yOffset, xySize, xySize);

    // Preset buttons below XY Pad in a single row (proportionally scaled and centered)
    const int presetButtonWidth = static_cast<int>(42 * scale);
    const int saveButtonWidth = static_cast<int>(56 * scale);
    const int presetButtonHeight = static_cast<int>(25 * scale);
    const int presetSpacing = static_cast<int>(4 * scale);

    // Calculate total width of all preset buttons
    int totalPresetWidth = (presetButtonWidth + presetSpacing + saveButtonWidth + presetSpacing * 2) * 4 - presetSpacing * 2;

    int presetYPos = centerSection.getY() + yOffset + xySize + static_cast<int>(15 * scale);
    int presetXStart = centerSection.getX() + (centerSection.getWidth() - totalPresetWidth) / 2;

    // Single row: A, Save A, B, Save B, C, Save C, D, Save D (centered)
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

    // Master Gain Compensation button
    masterCompButton.setBounds(rightSection.removeFromTop(buttonHeight)
        .withSizeKeepingCentre(buttonWidth, buttonHeight));
    rightSection.removeFromTop(spacing);

    // Delta Mode button
    deltaModeButton.setBounds(rightSection.removeFromTop(buttonHeight)
        .withSizeKeepingCentre(buttonWidth, buttonHeight));
    rightSection.removeFromTop(spacing);

    // Overshoot Delta Mode button
    overshootDeltaModeButton.setBounds(rightSection.removeFromTop(buttonHeight)
        .withSizeKeepingCentre(buttonWidth, buttonHeight));
    rightSection.removeFromTop(spacing * 2);

    // TP Delta disabled
    // truePeakDeltaModeButton.setBounds(rightSection.removeFromTop(buttonHeight)
    //     .withSizeKeepingCentre(buttonWidth, buttonHeight));
    // rightSection.removeFromTop(spacing);

    // Mix
    auto mixRow = rightSection.removeFromTop(knobSize);
    mixLabel.setBounds(mixRow.removeFromLeft(static_cast<int>(60 * scale)).removeFromTop(knobSize));
    mixSlider.setBounds(mixRow.removeFromLeft(knobSize));
    rightSection.removeFromTop(spacing * 3);

    // Protection Limiter Controls
    auto protectionButtonRow = rightSection.removeFromTop(static_cast<int>(28 * scale));
    protectionEnableButton.setBounds(protectionButtonRow.removeFromLeft(static_cast<int>(110 * scale)));
    rightSection.removeFromTop(spacing);

    auto protectionCeilingRow = rightSection.removeFromTop(knobSize);
    protectionCeilingLabel.setBounds(protectionCeilingRow.removeFromLeft(static_cast<int>(60 * scale)).removeFromTop(knobSize));
    protectionCeilingSlider.setBounds(protectionCeilingRow.removeFromLeft(knobSize));
    rightSection.removeFromTop(spacing);

    // Overshoot Character Blend Control
    auto overshootBlendRow = rightSection.removeFromTop(knobSize);
    overshootBlendLabel.setBounds(overshootBlendRow.removeFromLeft(static_cast<int>(60 * scale)).removeFromTop(knobSize));
    overshootBlendSlider.setBounds(overshootBlendRow.removeFromLeft(knobSize));
    rightSection.removeFromTop(spacing);

    // Overshoot Mode Toggle Button
    auto overshootModeButtonRow = rightSection.removeFromTop(static_cast<int>(28 * scale));
    overshootModeButton.setBounds(overshootModeButtonRow.removeFromLeft(static_cast<int>(110 * scale)));
    rightSection.removeFromTop(spacing);

    // ========== BOTTOM (VISUALIZATIONS) - FULL WIDTH ==========
    grMeter.setBounds(meterArea.reduced(padding, 0));
    oscilloscope.setBounds(oscilloscopeArea.reduced(padding, 0));

    // ========== RIGHT EDGE (MASTER METER) ==========
    // Master meter fills vertical space from gain reduction meter to oscilloscope
    // Calculate total height including both meter areas and the spacing between them
    int masterMeterTop = meterArea.getY();
    int masterMeterBottom = oscilloscopeArea.getBottom();
    int masterMeterHeight = masterMeterBottom - masterMeterTop;

    masterMeter.setBounds(masterMeterArea.getX() + static_cast<int>(4 * scale),
                          masterMeterTop,
                          masterMeterArea.getWidth() - static_cast<int>(8 * scale),
                          masterMeterHeight);
}

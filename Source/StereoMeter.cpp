#include "StereoMeter.h"
#include <juce_audio_basics/juce_audio_basics.h>

StereoMeter::StereoMeter(QuadBlendDriveAudioProcessor& p, bool isInput)
    : processor(p), isInputMeter(isInput)
{
    // Update at 60fps
    startTimerHz(60);
}

void StereoMeter::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Dark background
    g.setColour(juce::Colour(0xFF1A1A1A));
    g.fillRoundedRectangle(bounds, 2.0f);

    // Get peak levels (normalized 0-1)
    float peakL = isInputMeter ? processor.getInputPeakL() : processor.getOutputPeakL();
    float peakR = isInputMeter ? processor.getInputPeakR() : processor.getOutputPeakR();

    // Convert to dB
    float peakL_dB = juce::Decibels::gainToDecibels(peakL, -96.0f);
    float peakR_dB = juce::Decibels::gainToDecibels(peakR, -96.0f);

    // Use held peaks for numerical display (shows the highest recent peak)
    float peakHoldL_dB = juce::Decibels::gainToDecibels(peakHoldL, -96.0f);
    float peakHoldR_dB = juce::Decibels::gainToDecibels(peakHoldR, -96.0f);
    float maxPeak_dB = juce::jmax(peakHoldL_dB, peakHoldR_dB);

    // Reserve space for numerical readout at top
    const float textHeight = 16.0f;
    auto textArea = bounds.removeFromTop(textHeight);

    // Draw numerical readout (shows held peak)
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(10.0f, juce::Font::bold));
    juce::String peakText = juce::String(maxPeak_dB, 1) + " dB";
    g.drawText(peakText, textArea, juce::Justification::centred);

    // Calculate bar positions dynamically based on available width
    float availableWidth = bounds.getWidth() - 8.0f;  // Leave 4px padding on each side
    float channelWidth = (availableWidth - CHANNEL_GAP) * 0.5f;  // Split remaining space
    float startX = bounds.getX() + 4.0f;  // 4px left padding
    float barHeight = bounds.getHeight() - 4.0f;  // 2px padding top/bottom

    // Left channel bar
    juce::Rectangle<float> leftBar(
        startX,
        bounds.getY() + 2.0f,
        channelWidth,
        barHeight
    );

    // Right channel bar
    juce::Rectangle<float> rightBar(
        startX + channelWidth + CHANNEL_GAP,
        bounds.getY() + 2.0f,
        channelWidth,
        barHeight
    );

    // Draw background bars (dark gray)
    g.setColour(juce::Colour(0xFF2A2A2A));
    g.fillRect(leftBar);
    g.fillRect(rightBar);

    // Meter range: -60dB to +6dB (66dB total range)
    const float minDB = -60.0f;
    const float maxDB = 6.0f;
    const float dbRange = maxDB - minDB;

    // Draw 0dBFS reference line (white)
    float zeroDbY = leftBar.getY() + leftBar.getHeight() * (1.0f - ((0.0f - minDB) / dbRange));
    g.setColour(juce::Colours::white.withAlpha(0.8f));
    g.drawLine(leftBar.getX(), zeroDbY, rightBar.getRight(), zeroDbY, 1.0f);

    // Draw peak level bars with segmented colors
    // Green: bottom to 0dBFS, Orange: 0dBFS to +1dBFS, Red: +1dBFS to peak
    auto drawMeterBar = [&](float peakDB, const juce::Rectangle<float>& bar) {
        if (peakDB <= minDB)
            return;

        const float zeroDbNormalized = (0.0f - minDB) / dbRange;  // 0dBFS position
        const float onePlusDbNormalized = (1.0f - minDB) / dbRange;  // +1dBFS position
        const float peakNormalized = juce::jlimit(0.0f, 1.0f, (peakDB - minDB) / dbRange);

        // GREEN SEGMENT: From bottom to min(peak, 0dBFS)
        float greenTop = std::min(peakNormalized, zeroDbNormalized);
        if (greenTop > 0.0f)
        {
            float greenHeight = greenTop * barHeight;
            juce::Rectangle<float> greenFill(
                bar.getX(),
                bar.getBottom() - greenHeight,
                bar.getWidth(),
                greenHeight
            );
            g.setColour(juce::Colour(0xFF00D26A));  // Green
            g.fillRect(greenFill);
        }

        // ORANGE SEGMENT: From 0dBFS to min(peak, +1dBFS) - only if peak > 0dBFS
        if (peakDB > 0.0f)
        {
            float orangeBottom = zeroDbNormalized;
            float orangeTop = std::min(peakNormalized, onePlusDbNormalized);
            float orangeHeight = (orangeTop - orangeBottom) * barHeight;

            if (orangeHeight > 0.0f)
            {
                juce::Rectangle<float> orangeFill(
                    bar.getX(),
                    bar.getBottom() - (orangeTop * barHeight),
                    bar.getWidth(),
                    orangeHeight
                );
                g.setColour(juce::Colour(0xFFFF8C00));  // Orange
                g.fillRect(orangeFill);
            }
        }

        // RED SEGMENT: From +1dBFS to peak - only if peak > +1dBFS
        if (peakDB > 1.0f)
        {
            float redBottom = onePlusDbNormalized;
            float redTop = peakNormalized;
            float redHeight = (redTop - redBottom) * barHeight;

            if (redHeight > 0.0f)
            {
                juce::Rectangle<float> redFill(
                    bar.getX(),
                    bar.getBottom() - (redTop * barHeight),
                    bar.getWidth(),
                    redHeight
                );
                g.setColour(juce::Colour(0xFFFF3030));  // Red
                g.fillRect(redFill);
            }
        }
    };

    drawMeterBar(peakL_dB, leftBar);
    drawMeterBar(peakR_dB, rightBar);

    // Draw peak hold indicators (horizontal lines)
    auto drawPeakHold = [&](float peakHold, const juce::Rectangle<float>& bar) {
        if (peakHold > 0.0f)
        {
            float peakHoldDB = juce::Decibels::gainToDecibels(peakHold, -96.0f);
            if (peakHoldDB > minDB)
            {
                float normalizedLevel = juce::jlimit(0.0f, 1.0f, (peakHoldDB - minDB) / dbRange);
                float peakY = bar.getBottom() - (normalizedLevel * barHeight);
                g.setColour(juce::Colours::white.withAlpha(0.9f));
                g.drawLine(bar.getX(), peakY, bar.getRight(), peakY, 2.0f);
            }
        }
    };

    drawPeakHold(peakHoldL, leftBar);
    drawPeakHold(peakHoldR, rightBar);

    // Border
    g.setColour(juce::Colour(0xFF404040));
    g.drawRoundedRectangle(bounds.expanded(0.0f, textHeight), 2.0f, 1.0f);
}

void StereoMeter::resized()
{
    // No special layout needed - paint handles everything
}

void StereoMeter::timerCallback()
{
    // Get current peak levels
    float peakL = isInputMeter ? processor.getInputPeakL() : processor.getOutputPeakL();
    float peakR = isInputMeter ? processor.getInputPeakR() : processor.getOutputPeakR();

    // Update peak hold for left channel
    juce::int64 currentTime = juce::Time::currentTimeMillis();
    if (peakL > peakHoldL)
    {
        peakHoldL = peakL;
        peakHoldTimeL = currentTime;
    }
    else if (currentTime - peakHoldTimeL > peakHoldDurationMs)
    {
        peakHoldL = peakL;  // Reset to current level after hold time expires
    }

    // Update peak hold for right channel
    if (peakR > peakHoldR)
    {
        peakHoldR = peakR;
        peakHoldTimeR = currentTime;
    }
    else if (currentTime - peakHoldTimeR > peakHoldDurationMs)
    {
        peakHoldR = peakR;  // Reset to current level after hold time expires
    }

    // Trigger repaint for smooth animation
    repaint();
}

juce::Colour StereoMeter::getColorForLevel(float level)
{
    // Convert normalized level to dB
    float levelDB = juce::Decibels::gainToDecibels(level, -96.0f);

    // Hard color cuts at 0dB and +1dB
    if (levelDB >= RED_THRESHOLD_DB)
    {
        // Red zone (>+1dB) - Hot peaks
        return juce::Colour(0xFFFF3030);  // Bright red
    }
    else if (levelDB >= ORANGE_THRESHOLD_DB)
    {
        // Orange zone (0dB to +1dB) - At or above 0dBFS
        return juce::Colour(0xFFFF8C00);  // Dark orange
    }
    else
    {
        // Green zone (<0dB) - Below 0dBFS
        return juce::Colour(0xFF00D26A);  // Same green as transfer curve meter
    }
}

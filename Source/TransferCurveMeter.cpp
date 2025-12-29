#include "TransferCurveMeter.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>

TransferCurveMeter::TransferCurveMeter(QuadBlendDriveAudioProcessor& p)
    : processor(p)
{
    // Update at 60fps
    startTimerHz(60);
}

float TransferCurveMeter::dbToNormalized(float dB) const
{
    return (dB - displayMinDB) / (displayMaxDB - displayMinDB);
}

float TransferCurveMeter::normalizedToDb(float norm) const
{
    return displayMinDB + norm * (displayMaxDB - displayMinDB);
}

void TransferCurveMeter::paint(juce::Graphics& g)
{
    // Use square bounds calculated in resized()
    auto bounds = curveBounds;

    // Dark background
    g.setColour(juce::Colour(BACKGROUND_COLOR));
    g.fillRoundedRectangle(bounds, 4.0f);

    // Get threshold value from processor
    float thresholdDB = processor.apvts.getRawParameterValue("THRESHOLD")->load();
    float threshNorm = dbToNormalized(thresholdDB);
    threshNorm = juce::jlimit(0.0f, 1.0f, threshNorm);

    // Generate composite transfer curve
    TransferCurveData curve;
    generateCompositeCurve(curve,
        processor.getXY_X(),
        processor.getXY_Y(),
        processor.getDriveHC(),
        processor.getDriveSC(),
        processor.getDriveSL(),
        processor.getDriveFL(),
        processor.getSoloIndex());

    // Get current peak level (normalized 0-1)
    float peak = processor.getCurrentPeakNormalized();
    int peakIndex = juce::jlimit(0, 143, static_cast<int>(peak * 143.0f));

    // Draw threshold reference line (horizontal at threshold level on output axis)
    float threshY = bounds.getBottom() - threshNorm * bounds.getHeight();
    g.setColour(juce::Colour(WARNING_AMBER).withAlpha(isHoveringThreshold || isDraggingThreshold ? 0.9f : 0.5f));
    g.drawHorizontalLine(static_cast<int>(threshY), bounds.getX(), bounds.getRight());

    // Draw threshold grab handle when hovering
    if (isHoveringThreshold || isDraggingThreshold)
    {
        g.fillRoundedRectangle(bounds.getX() - 4.0f, threshY - 4.0f, 8.0f, 8.0f, 2.0f);
        g.fillRoundedRectangle(bounds.getRight() - 4.0f, threshY - 4.0f, 8.0f, 8.0f, 2.0f);
    }

    // Draw grid lines with dB labels
    g.setColour(juce::Colours::white.withAlpha(GRID_ALPHA));
    const float gridDBs[] = { -48.0f, -36.0f, -24.0f, -12.0f, -6.0f, 0.0f };
    for (float gridDB : gridDBs)
    {
        float gridNorm = dbToNormalized(gridDB);
        float lineY = bounds.getBottom() - gridNorm * bounds.getHeight();
        float lineX = bounds.getX() + gridNorm * bounds.getWidth();

        g.drawHorizontalLine(static_cast<int>(lineY), bounds.getX(), bounds.getRight());
        g.drawVerticalLine(static_cast<int>(lineX), bounds.getY(), bounds.getBottom());
    }

    // Draw dB labels on right side
    g.setColour(juce::Colours::white.withAlpha(0.4f));
    g.setFont(10.0f);
    for (float gridDB : gridDBs)
    {
        float gridNorm = dbToNormalized(gridDB);
        float labelY = bounds.getBottom() - gridNorm * bounds.getHeight();
        juce::String label = (gridDB == 0.0f) ? "0" : juce::String(static_cast<int>(gridDB));
        g.drawText(label, static_cast<int>(bounds.getRight() + 2), static_cast<int>(labelY - 6), 20, 12, juce::Justification::left);
    }

    // Build curve paths
    juce::Path whitePath, greenPath;

    for (int i = 0; i < 144; ++i)
    {
        float x = bounds.getX() + (static_cast<float>(i) / 143.0f) * bounds.getWidth();
        float y = bounds.getBottom() - curve.y[i] * bounds.getHeight();

        if (i == 0)
        {
            whitePath.startNewSubPath(x, y);
            if (i <= peakIndex)
                greenPath.startNewSubPath(x, y);
        }
        else
        {
            whitePath.lineTo(x, y);
            if (i <= peakIndex)
                greenPath.lineTo(x, y);
        }
    }

    // Draw white curve (unlit portion)
    g.setColour(juce::Colours::white.withAlpha(WHITE_ALPHA));
    g.strokePath(whitePath, juce::PathStrokeType(1.5f));

    // Draw green curve (lit portion up to current signal level)
    g.setColour(juce::Colour(GREEN_COLOR));
    g.strokePath(greenPath, juce::PathStrokeType(2.0f));

    // Draw input level indicator (vertical line showing current input)
    if (peak > 0.001f)
    {
        float inputX = bounds.getX() + peak * bounds.getWidth();
        g.setColour(juce::Colour(ACCENT_BLUE).withAlpha(0.7f));
        g.drawVerticalLine(static_cast<int>(inputX), bounds.getY(), bounds.getBottom());

        // Draw intersection point (where input meets curve)
        float outputY = bounds.getBottom() - curve.y[peakIndex] * bounds.getHeight();
        g.setColour(juce::Colour(ACCENT_BLUE));
        g.fillEllipse(inputX - 4.0f, outputY - 4.0f, 8.0f, 8.0f);
    }

    // Unity line (45 degree diagonal)
    g.setColour(juce::Colours::white.withAlpha(0.1f));
    g.drawLine(bounds.getX(), bounds.getBottom(), bounds.getRight(), bounds.getY(), 1.0f);

    // Border
    g.setColour(juce::Colour(0xff404040));
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

    // Threshold label
    g.setColour(juce::Colour(WARNING_AMBER).withAlpha(0.8f));
    g.setFont(11.0f);
    juce::String threshLabel = juce::String(thresholdDB, 1) + " dB";
    g.drawText(threshLabel, static_cast<int>(bounds.getX() + 4), static_cast<int>(threshY - 14), 50, 12, juce::Justification::left);
}

void TransferCurveMeter::resized()
{
    // Force square aspect ratio (1:1) and center horizontally, leave room for labels
    auto bounds = getLocalBounds().toFloat().reduced(2.0f);
    bounds.removeFromRight(25.0f);  // Room for dB labels
    int size = juce::jmin(static_cast<int>(bounds.getWidth()), static_cast<int>(bounds.getHeight()));
    curveBounds = bounds.withSizeKeepingCentre(static_cast<float>(size), static_cast<float>(size));
}

void TransferCurveMeter::mouseDown(const juce::MouseEvent& e)
{
    if (isHoveringThreshold)
    {
        isDraggingThreshold = true;
        repaint();
    }
}

void TransferCurveMeter::mouseDrag(const juce::MouseEvent& e)
{
    if (isDraggingThreshold)
    {
        // Convert y position to dB
        float y = static_cast<float>(e.position.y);
        float norm = 1.0f - (y - curveBounds.getY()) / curveBounds.getHeight();
        norm = juce::jlimit(0.0f, 1.0f, norm);
        float newThreshDB = normalizedToDb(norm);
        newThreshDB = juce::jlimit(-60.0f, 0.0f, newThreshDB);

        // Update the threshold parameter
        if (auto* param = processor.apvts.getParameter("THRESHOLD"))
            param->setValueNotifyingHost(param->convertTo0to1(newThreshDB));

        repaint();
    }
}

void TransferCurveMeter::mouseUp(const juce::MouseEvent&)
{
    isDraggingThreshold = false;
    repaint();
}

void TransferCurveMeter::mouseMove(const juce::MouseEvent& e)
{
    // Check if mouse is near the threshold line
    float thresholdDB = processor.apvts.getRawParameterValue("THRESHOLD")->load();
    float threshNorm = dbToNormalized(thresholdDB);
    float threshY = curveBounds.getBottom() - threshNorm * curveBounds.getHeight();

    bool wasHovering = isHoveringThreshold;
    isHoveringThreshold = std::abs(e.position.y - threshY) < thresholdHitZone &&
                          e.position.x >= curveBounds.getX() &&
                          e.position.x <= curveBounds.getRight();

    if (isHoveringThreshold != wasHovering)
    {
        setMouseCursor(isHoveringThreshold ? juce::MouseCursor::UpDownResizeCursor : juce::MouseCursor::NormalCursor);
        repaint();
    }
}

void TransferCurveMeter::timerCallback()
{
    // Trigger repaint for smooth animation
    repaint();
}

void TransferCurveMeter::generateCompositeCurve(TransferCurveData& data,
                                                float X, float Y,
                                                float driveHC_dB, float driveSC_dB,
                                                float driveSL_dB, float driveFL_dB,
                                                int soloIndex)
{
    // Convert dB trim values to linear gain
    float driveHC = juce::Decibels::decibelsToGain(driveHC_dB);
    float driveSC = juce::Decibels::decibelsToGain(driveSC_dB);
    float driveSL = juce::Decibels::decibelsToGain(driveSL_dB);
    float driveFL = juce::Decibels::decibelsToGain(driveFL_dB);

    // Calculate bi-linear blend weights from XY pad
    // Top-left (0,1) = Hard Clip, Top-right (1,1) = Fast Limit
    // Bottom-left (0,0) = Soft Clip, Bottom-right (1,0) = Slow Limit
    float wTL = (1.0f - X) * Y;          // Hard Clip (top-left)
    float wBL = (1.0f - X) * (1.0f - Y); // Soft Clip (bottom-left)
    float wBR = X * (1.0f - Y);          // Slow Limit (bottom-right)
    float wTR = X * Y;                   // Fast Limit (top-right)

    // Solo override: if a processor is soloed, show ONLY that curve
    if (soloIndex >= 0)
    {
        wTL = wBL = wBR = wTR = 0.0f;
        switch (soloIndex)
        {
            case 0: wTL = 1.0f; break;  // Hard Clip
            case 1: wBL = 1.0f; break;  // Soft Clip
            case 2: wBR = 1.0f; break;  // Slow Limit
            case 3: wTR = 1.0f; break;  // Fast Limit
        }
    }

    // Generate 144-point composite curve
    for (int i = 0; i < 144; ++i)
    {
        float x = static_cast<float>(i) / 143.0f;  // Normalized input (0-1)

        data.y[i] = wTL * hardClip(x, driveHC)
                  + wBL * softClip(x, driveSC)
                  + wBR * slowLimit(x, driveSL)
                  + wTR * fastLimit(x, driveFL);
    }
}

float TransferCurveMeter::hardClip(float x, float drive)
{
    // Hard clipping: multiply by drive, then clamp to [0, 1]
    // No normalization - shows actual transfer characteristic
    float driven = x * drive;
    return std::clamp(driven, 0.0f, 1.0f);
}

float TransferCurveMeter::softClip(float x, float drive)
{
    // Soft clipping using tanh saturation
    // For positive inputs, tanh gives smooth compression approaching 1.0
    // No normalization - shows actual transfer characteristic
    float driven = x * drive;
    return std::tanh(driven);
}

float TransferCurveMeter::slowLimit(float x, float drive)
{
    float thresh = 1.0f / std::max(drive, 1.0f);
    return softKneeCompress(x, thresh, 4.0f, 0.3f);  // Gentle ratio, soft knee
}

float TransferCurveMeter::fastLimit(float x, float drive)
{
    float thresh = 1.0f / std::max(drive, 1.0f);
    return softKneeCompress(x, thresh, 20.0f, 0.05f);  // Brick wall, hard knee
}

float TransferCurveMeter::softKneeCompress(float x, float thresh, float ratio, float knee)
{
    if (x < thresh - knee * 0.5f)
        return x;

    if (x < thresh + knee * 0.5f)
    {
        float delta = x - (thresh - knee * 0.5f);
        return x + (1.0f / ratio - 1.0f) * delta * delta / (2.0f * knee);
    }

    return thresh + (x - thresh) / ratio;
}

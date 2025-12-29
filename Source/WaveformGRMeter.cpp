#include "WaveformGRMeter.h"

WaveformGRMeter::WaveformGRMeter(QuadBlendDriveAudioProcessor& p)
    : processor(p)
{
    // Start timer for 30fps updates
    startTimer(33);  // ~30fps
}

void WaveformGRMeter::paint(juce::Graphics& g)
{
    // Dark background matching plugin theme
    g.fillAll(juce::Colour(26, 26, 26));

    auto bounds = getLocalBounds().toFloat();

    // === PHASE 1: BASIC SCROLLING WAVEFORM ===
    paintWaveform(g, bounds);

    // Border
    g.setColour(juce::Colour(0xff404040));
    g.drawRect(bounds, 1.0f);
}

void WaveformGRMeter::paintWaveform(juce::Graphics& g, const juce::Rectangle<float>& bounds)
{
    // Get decimated display data from processor
    const auto& decimatedDisplay = processor.decimatedDisplay;
    const int numSegments = QuadBlendDriveAudioProcessor::decimatedDisplaySize;

    if (!processor.decimatedDisplayReady.load() || numSegments == 0)
        return;

    const float width = bounds.getWidth();
    const float height = bounds.getHeight();
    const float segmentWidth = width / static_cast<float>(numSegments);
    const float centerY = bounds.getCentreY();

    // === RENDER INPUT WAVEFORM (GRAY, 40% OPACITY) ===
    juce::Path inputPath;
    bool inputPathStarted = false;

    for (int i = 0; i < numSegments; ++i)
    {
        const float x = bounds.getX() + i * segmentWidth;

        // Use actual input signal data
        const float inputMax = decimatedDisplay[i].inputMax;
        const float inputMin = decimatedDisplay[i].inputMin;

        // Map amplitude to screen Y (centered, with some scaling)
        const float scale = height * 0.4f;  // Use 40% of height for amplitude
        const float yMax = centerY - (inputMax * scale);
        const float yMin = centerY - (inputMin * scale);

        if (i == 0)
        {
            inputPath.startNewSubPath(x, yMax);
            inputPathStarted = true;
        }
        else
        {
            inputPath.lineTo(x, yMax);
        }
    }

    // Draw bottom envelope
    for (int i = numSegments - 1; i >= 0; --i)
    {
        const float x = bounds.getX() + i * segmentWidth;
        const float inputMin = decimatedDisplay[i].inputMin;
        const float scale = height * 0.4f;
        const float yMin = centerY - (inputMin * scale);
        inputPath.lineTo(x, yMin);
    }

    inputPath.closeSubPath();

    // Fill input waveform (gray, 40% opacity)
    g.setColour(juce::Colour(128, 128, 128).withAlpha(INPUT_OPACITY));
    g.fillPath(inputPath);

    // === PHASE 1: RENDER OUTPUT WAVEFORM (WHITE, 100% OPACITY) ===
    juce::Path outputPath;
    bool outputPathStarted = false;

    for (int i = 0; i < numSegments; ++i)
    {
        const float x = bounds.getX() + i * segmentWidth;

        // Use final output data
        const float outputMax = decimatedDisplay[i].finalOutputMax;
        const float outputMin = decimatedDisplay[i].finalOutputMin;

        const float scale = height * 0.4f;
        const float yMax = centerY - (outputMax * scale);
        const float yMin = centerY - (outputMin * scale);

        if (i == 0)
        {
            outputPath.startNewSubPath(x, yMax);
            outputPathStarted = true;
        }
        else
        {
            outputPath.lineTo(x, yMax);
        }
    }

    // Draw bottom envelope
    for (int i = numSegments - 1; i >= 0; --i)
    {
        const float x = bounds.getX() + i * segmentWidth;
        const float outputMin = decimatedDisplay[i].finalOutputMin;
        const float scale = height * 0.4f;
        const float yMin = centerY - (outputMin * scale);
        outputPath.lineTo(x, yMin);
    }

    outputPath.closeSubPath();

    // Fill output waveform (white, 100% opacity)
    g.setColour(juce::Colours::white.withAlpha(OUTPUT_OPACITY));
    g.fillPath(outputPath);

    // === PHASE 4: RENDER GAIN REDUCTION GAP WITH DYNAMIC COLOR BLENDING ===
    // Fill the space between input and output waveforms with colors that represent
    // which processors are actively causing gain reduction at each moment
    for (int i = 0; i < numSegments; ++i)
    {
        const float x = bounds.getX() + i * segmentWidth;
        const float scale = height * 0.4f;

        // Get input and output envelopes
        const float inputMax = decimatedDisplay[i].inputMax;
        const float inputMin = decimatedDisplay[i].inputMin;
        const float outputMax = decimatedDisplay[i].finalOutputMax;
        const float outputMin = decimatedDisplay[i].finalOutputMin;

        // Get per-processor outputs
        const float hcMax = decimatedDisplay[i].hardClipMax;
        const float scMax = decimatedDisplay[i].softClipMax;
        const float slMax = decimatedDisplay[i].slowLimitMax;
        const float flMax = decimatedDisplay[i].fastLimitMax;

        // Calculate per-processor gain reduction (how much each processor reduced the signal)
        // GR = input - processor_output (positive = reduction)
        float hcGR = std::max(0.0f, inputMax - hcMax);
        float scGR = std::max(0.0f, inputMax - scMax);
        float slGR = std::max(0.0f, inputMax - slMax);
        float flGR = std::max(0.0f, inputMax - flMax);

        // Calculate blended color based on proportional reduction
        juce::Colour gapColor = calculateGapColor(hcGR, scGR, slGR, flGR);

        // Only draw gap if there's visible reduction
        if (gapColor.getAlpha() > 0)
        {
            // Map to screen coordinates
            const float inputYMax = centerY - (inputMax * scale);
            const float inputYMin = centerY - (inputMin * scale);
            const float outputYMax = centerY - (outputMax * scale);
            const float outputYMin = centerY - (outputMin * scale);

            // Create path for this segment's GR gap
            juce::Path gapPath;
            gapPath.startNewSubPath(x, outputYMax);
            gapPath.lineTo(x + segmentWidth, outputYMax);
            gapPath.lineTo(x + segmentWidth, inputYMax);
            gapPath.lineTo(x, inputYMax);
            gapPath.closeSubPath();

            // Fill gap with dynamically blended color
            g.setColour(gapColor.withAlpha(0.7f));  // 70% opacity for gap
            g.fillPath(gapPath);
        }
    }
}

void WaveformGRMeter::resized()
{
    // Component resizing handled by parent (PluginEditor)
}

void WaveformGRMeter::timerCallback()
{
    // Update decimated display cache from high-resolution ring buffer
    processor.updateDecimatedDisplay();

    // Trigger repaint for smooth animation (30fps)
    repaint();
}

void WaveformGRMeter::soloProcessor(int processorIndex)
{
    if (processorIndex >= -1 && processorIndex < 4)
    {
        soloedProcessor = processorIndex;
        repaint();
    }
}

void WaveformGRMeter::unsoloAll()
{
    soloedProcessor = -1;
    repaint();
}

bool WaveformGRMeter::isProcessorSoloed(int processorIndex) const
{
    return soloedProcessor == processorIndex;
}

juce::Colour WaveformGRMeter::getProcessorColor(int processorIndex) const
{
    static const juce::Colour colors[4] = {
        juce::Colour(255, 80, 80),   // 0: Hard Clip - red
        juce::Colour(255, 150, 60),  // 1: Soft Clip - orange
        juce::Colour(255, 220, 80),  // 2: Slow Limit - yellow
        juce::Colour(80, 150, 255)   // 3: Fast Limit - blue
    };

    if (processorIndex >= 0 && processorIndex < 4)
        return colors[processorIndex];
    return juce::Colours::white;
}

juce::Colour WaveformGRMeter::calculateGapColor(float hcGR, float scGR, float slGR, float flGR) const
{
    // Calculate total gain reduction
    const float totalGR = hcGR + scGR + slGR + flGR;

    if (totalGR <= 0.0001f)
        return juce::Colours::transparentBlack;  // No gain reduction

    // Calculate proportional ratios
    const float hcRatio = hcGR / totalGR;
    const float scRatio = scGR / totalGR;
    const float slRatio = slGR / totalGR;
    const float flRatio = flGR / totalGR;

    // Get processor colors
    const juce::Colour hcColor = getProcessorColor(0);
    const juce::Colour scColor = getProcessorColor(1);
    const juce::Colour slColor = getProcessorColor(2);
    const juce::Colour flColor = getProcessorColor(3);

    // Blend colors proportionally
    const uint8_t r = static_cast<uint8_t>(
        hcColor.getRed() * hcRatio +
        scColor.getRed() * scRatio +
        slColor.getRed() * slRatio +
        flColor.getRed() * flRatio
    );
    const uint8_t gr = static_cast<uint8_t>(
        hcColor.getGreen() * hcRatio +
        scColor.getGreen() * scRatio +
        slColor.getGreen() * slRatio +
        flColor.getGreen() * flRatio
    );
    const uint8_t b = static_cast<uint8_t>(
        hcColor.getBlue() * hcRatio +
        scColor.getBlue() * scRatio +
        slColor.getBlue() * slRatio +
        flColor.getBlue() * flRatio
    );

    return juce::Colour(r, gr, b);
}

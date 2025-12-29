#include "ThresholdMeter.h"

ThresholdMeter::ThresholdMeter(QuadBlendDriveAudioProcessor& p)
    : processor(p)
{
    // Start timer for 30fps updates (non-blocking GUI rendering)
    startTimer(33);  // ~30fps
}

void ThresholdMeter::paint(juce::Graphics& g)
{
    // Dark background matching plugin theme
    g.fillAll(juce::Colour(26, 26, 26));

    auto bounds = getLocalBounds().toFloat();
    const float height = bounds.getHeight();

    // === PHASE 1: THRESHOLD LINE + OUTPUT ABOVE ===
    // Calculate threshold line position (threshold is in dBFS, need to convert to screen Y)
    float thresholdDB = processor.apvts.getRawParameterValue("THRESHOLD")->load();

    // Map threshold to screen coordinates
    // Threshold range: -12 dB to 0 dB
    // Screen mapping: -12 dB at bottom (80% of height), 0 dB at top (20% of height)
    float thresholdNorm = (thresholdDB + 12.0f) / 12.0f;  // 0.0 = -12dB, 1.0 = 0dB
    float thresholdY = bounds.getY() + height * (0.8f - thresholdNorm * 0.6f);  // Inverted (0dB at top)

    // Draw threshold line (subtle, light gray)
    g.setColour(juce::Colours::white.withAlpha(0.3f));
    g.drawHorizontalLine(static_cast<int>(thresholdY), bounds.getX(), bounds.getRight());

    // Optional: Threshold label
    g.setFont(juce::Font(10.0f));
    juce::String thresholdLabel = juce::String(thresholdDB, 1) + " dB";
    g.drawText(thresholdLabel, bounds.getX() + 5, thresholdY - 15, 60, 12, juce::Justification::left);

    // === OUTPUT SIGNAL ABOVE THRESHOLD ===
    paintOutputAboveThreshold(g, bounds, thresholdY);

    // === GAIN REDUCTION LAYERS BELOW THRESHOLD ===
    // Phase 2-3: Will render colored layers here
    paintGainReductionLayers(g, bounds, thresholdY);

    // Border
    g.setColour(juce::Colour(0xff404040));
    g.drawRect(bounds, 1.0f);
}

void ThresholdMeter::paintOutputAboveThreshold(juce::Graphics& g,
                                                const juce::Rectangle<float>& bounds,
                                                float thresholdY)
{
    // Get decimated display data from processor
    const auto& decimatedDisplay = processor.decimatedDisplay;
    const int numSegments = QuadBlendDriveAudioProcessor::decimatedDisplaySize;

    const float width = bounds.getWidth();
    const float height = bounds.getHeight();
    const float segmentWidth = width / static_cast<float>(numSegments);

    // Create path for output signal above threshold
    juce::Path outputPath;
    bool pathStarted = false;

    // Draw top envelope (max values above threshold)
    for (int i = 0; i < numSegments; ++i)
    {
        const float x = bounds.getX() + i * segmentWidth;

        // Use finalOutputMax from decimated display
        const float outputMax = decimatedDisplay[i].finalOutputMax;

        // Convert amplitude to dB and compare to threshold
        float outputDB = -100.0f;  // Start with very low value
        if (outputMax > 0.00001f)
            outputDB = juce::Decibels::gainToDecibels(outputMax);

        // Only render if above threshold
        float thresholdDB = processor.apvts.getRawParameterValue("THRESHOLD")->load();
        if (outputDB > thresholdDB)
        {
            // Map to screen Y (above threshold line)
            // 0 dB at 20% from top, threshold at thresholdY
            float outputNorm = (outputDB + 12.0f) / 12.0f;  // Normalize to 0-1 range
            float y = bounds.getY() + height * (0.8f - outputNorm * 0.6f);

            // Clamp to above threshold
            y = juce::jmin(y, thresholdY);

            if (!pathStarted)
            {
                outputPath.startNewSubPath(x, thresholdY);
                outputPath.lineTo(x, y);
                pathStarted = true;
            }
            else
            {
                outputPath.lineTo(x, y);
            }
        }
        else
        {
            // Below threshold - close path if it was started
            if (pathStarted)
            {
                outputPath.lineTo(x, thresholdY);
                outputPath.closeSubPath();
                pathStarted = false;
            }
        }
    }

    // Close path if still open
    if (pathStarted)
    {
        float x = bounds.getX() + numSegments * segmentWidth;
        outputPath.lineTo(x, thresholdY);
        outputPath.closeSubPath();
    }

    // Fill output signal with white (100% opacity)
    g.setColour(juce::Colours::white);
    g.fillPath(outputPath);
}

void ThresholdMeter::paintGainReductionLayers(juce::Graphics& g,
                                              const juce::Rectangle<float>& bounds,
                                              float thresholdY)
{
    // === PHASE 3: RENDER FOUR COLORED LAYERS BELOW THRESHOLD ===
    // All layers start at threshold line and extend downward
    // Depth = magnitude of gain reduction from that processor

    const auto& decimatedDisplay = processor.decimatedDisplay;
    const int numSegments = QuadBlendDriveAudioProcessor::decimatedDisplaySize;

    const float width = bounds.getWidth();
    const float height = bounds.getHeight();
    const float segmentWidth = width / static_cast<float>(numSegments);

    // Render order (back to front): Red → Orange → Yellow → Blue
    // This ensures deepest layer is visually dominant
    const int renderOrder[4] = { 0, 1, 2, 3 };  // HC, SC, SL, FL

    for (int layerIdx : renderOrder)
    {
        if (!layerVisible[layerIdx])
            continue;  // Skip hidden layers

        juce::Path grPath;
        bool pathStarted = false;

        // Draw layer from threshold line downward
        for (int i = 0; i < numSegments; ++i)
        {
            const float x = bounds.getX() + i * segmentWidth;

            // Get gain reduction value for this layer (in dB)
            float grValueDB = 0.0f;
            switch (layerIdx)
            {
                case 0: grValueDB = decimatedDisplay[i].hardClipGRMax; break;  // Hard Clip
                case 1: grValueDB = decimatedDisplay[i].softClipGRMax; break;  // Soft Clip
                case 2: grValueDB = decimatedDisplay[i].slowLimitGRMax; break; // Slow Limit
                case 3: grValueDB = decimatedDisplay[i].fastLimitGRMax; break; // Fast Limit
            }

            // Convert gain reduction (dB) to screen Y (below threshold)
            // Larger reduction = deeper (further down from threshold)
            // Scale: 1 dB of reduction = 20 pixels down
            // This gives us visible depth for typical GR values (0-6 dB = 0-120 pixels)
            const float grPixels = grValueDB * 20.0f;  // Scale factor: 20 pixels per dB
            const float y = thresholdY + grPixels;

            // Clamp to bounds
            const float clampedY = juce::jmin(y, bounds.getBottom());

            if (i == 0)
            {
                grPath.startNewSubPath(x, thresholdY);
                grPath.lineTo(x, clampedY);
                pathStarted = true;
            }
            else
            {
                grPath.lineTo(x, clampedY);
            }
        }

        // Close path back to threshold
        if (pathStarted)
        {
            grPath.lineTo(bounds.getRight(), thresholdY);
            grPath.closeSubPath();
        }

        // Fill layer with colored, semi-transparent paint (60% opacity)
        g.setColour(getLayerColor(layerIdx));
        g.fillPath(grPath);
    }
}

void ThresholdMeter::resized()
{
    // Component resizing handled by parent (PluginEditor)
}

void ThresholdMeter::timerCallback()
{
    // Update decimated display cache from high-resolution ring buffer
    processor.updateDecimatedDisplay();

    // Trigger repaint for smooth animation (30fps)
    repaint();
}

void ThresholdMeter::setLayerVisible(int layerIndex, bool visible)
{
    if (layerIndex >= 0 && layerIndex < 4)
    {
        layerVisible[layerIndex] = visible;
        repaint();
    }
}

bool ThresholdMeter::isLayerVisible(int layerIndex) const
{
    if (layerIndex >= 0 && layerIndex < 4)
        return layerVisible[layerIndex];
    return false;
}

void ThresholdMeter::soloLayer(int layerIndex)
{
    if (layerIndex >= 0 && layerIndex < 4)
    {
        soloedLayer = layerIndex;

        // Hide all other layers
        for (int i = 0; i < 4; ++i)
            layerVisible[i] = (i == layerIndex);

        repaint();
    }
}

void ThresholdMeter::unsoloAll()
{
    soloedLayer = -1;

    // Show all layers
    for (int i = 0; i < 4; ++i)
        layerVisible[i] = true;

    repaint();
}

juce::Colour ThresholdMeter::getLayerColor(int layerIndex) const
{
    // Layer colors at fixed 60% opacity
    static const juce::Colour colors[4] = {
        juce::Colour(255, 80, 80).withAlpha(static_cast<uint8_t>(LAYER_OPACITY)),   // 0: Hard Clip - red
        juce::Colour(255, 150, 60).withAlpha(static_cast<uint8_t>(LAYER_OPACITY)),  // 1: Soft Clip - orange
        juce::Colour(255, 220, 80).withAlpha(static_cast<uint8_t>(LAYER_OPACITY)),  // 2: Slow Limit - yellow
        juce::Colour(80, 150, 255).withAlpha(static_cast<uint8_t>(LAYER_OPACITY))   // 3: Fast Limit - blue
    };

    if (layerIndex >= 0 && layerIndex < 4)
        return colors[layerIndex];
    return juce::Colours::white;
}

const char* ThresholdMeter::getLayerName(int layerIndex) const
{
    static const char* names[4] = {
        "Hard Clip",
        "Soft Clip",
        "Slow Limit",
        "Fast Limit"
    };

    if (layerIndex >= 0 && layerIndex < 4)
        return names[layerIndex];
    return "Unknown";
}

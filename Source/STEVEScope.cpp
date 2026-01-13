#include "STEVEScope.h"
#include <cmath>

STEVEScope::STEVEScope(QuadBlendDriveAudioProcessor& p)
    : processor(p)
{
    startTimerHz(30);  // 30fps for smooth animation
}

STEVEScope::~STEVEScope()
{
    stopTimer();
}

void STEVEScope::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Background
    g.setColour(juce::Colour(BACKGROUND));
    g.fillRoundedRectangle(bounds, 4.0f);

    // Settings button (gear icon in upper right) - no title bar to avoid overlap with ADVANCED button
    paintSettingsButton(g);

    // Graph area - use full height since no title bar
    graphBounds = bounds.reduced(8.0f, 8.0f);

    // Draw components
    paintGrid(g, graphBounds);
    paintWaveform(g, graphBounds);
    if (showGRTraces)
        paintGRTraces(g, graphBounds);
    paintPlayhead(g, graphBounds);

    // Border
    g.setColour(juce::Colour(0xff404040));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 4.0f, 1.0f);
}

void STEVEScope::paintGrid(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    const float centerY = bounds.getCentreY();

    // Center line (0V)
    g.setColour(juce::Colour(GRID_COLOR).withAlpha(0.5f));
    g.drawHorizontalLine(static_cast<int>(centerY), bounds.getX(), bounds.getRight());

    // 0 dBFS reference lines (±1.0 amplitude)
    const float zeroDbTop = bounds.getY() + bounds.getHeight() * 0.1f;
    const float zeroDbBottom = bounds.getBottom() - bounds.getHeight() * 0.1f;

    g.setColour(juce::Colour(ZERO_DB_COLOR).withAlpha(0.4f));
    g.drawHorizontalLine(static_cast<int>(zeroDbTop), bounds.getX(), bounds.getRight());
    g.drawHorizontalLine(static_cast<int>(zeroDbBottom), bounds.getX(), bounds.getRight());

    // 0 dBFS labels
    g.setColour(juce::Colour(ZERO_DB_COLOR).withAlpha(0.6f));
    g.setFont(9.0f);
    g.drawText("0dB", bounds.getRight() - 30.0f, zeroDbTop - 10.0f, 28.0f, 12.0f, juce::Justification::centredRight);

    // === BAR LINES ===
    // Draw vertical lines at bar/beat divisions based on time base
    float timeBaseBeats = getTimeBaseBeats();
    int numBars = static_cast<int>(timeBaseBeats / 4.0f);  // 4 beats per bar
    if (numBars < 1) numBars = 1;

    // Calculate beat divisions to show
    // For time bases < 1 bar, show beat lines
    // For time bases >= 1 bar, show bar lines
    int numDivisions;
    float beatsPerDivision;

    if (timeBaseBeats <= 1.0f)
    {
        // Sub-beat: show 4 divisions
        numDivisions = 4;
        beatsPerDivision = timeBaseBeats / 4.0f;
    }
    else if (timeBaseBeats <= 4.0f)
    {
        // Up to 1 bar: show beat lines
        numDivisions = static_cast<int>(timeBaseBeats);
        beatsPerDivision = 1.0f;
    }
    else
    {
        // Multiple bars: show bar lines
        numDivisions = numBars;
        beatsPerDivision = 4.0f;
    }

    // Draw division lines
    for (int i = 1; i < numDivisions; ++i)
    {
        float ratio = static_cast<float>(i) / numDivisions;
        float x = bounds.getX() + ratio * bounds.getWidth();

        // Bar lines are brighter than beat lines
        bool isBarLine = (beatsPerDivision >= 4.0f) || (i * beatsPerDivision >= 4.0f && static_cast<int>(i * beatsPerDivision) % 4 == 0);
        float alpha = isBarLine ? 0.4f : 0.2f;

        g.setColour(juce::Colour(GRID_COLOR).withAlpha(alpha));
        g.drawVerticalLine(static_cast<int>(x), bounds.getY(), bounds.getBottom());
    }

    // Threshold line
    float thresholdDb = processor.apvts.getRawParameterValue("THRESHOLD")->load();
    float threshNorm = (thresholdDb + 60.0f) / 66.0f;  // -60 to +6 range
    threshNorm = juce::jlimit(0.0f, 1.0f, threshNorm);
    float threshY = bounds.getBottom() - threshNorm * bounds.getHeight();

    g.setColour(juce::Colour(THRESHOLD_COLOR).withAlpha(0.5f));
    g.drawHorizontalLine(static_cast<int>(threshY), bounds.getX(), bounds.getRight());

    // Threshold label
    g.setColour(juce::Colour(THRESHOLD_COLOR).withAlpha(0.7f));
    g.setFont(9.0f);
    juce::String threshLabel = juce::String(thresholdDb, 1) + "dB";
    g.drawText(threshLabel, bounds.getX() + 4.0f, threshY - 12.0f, 50.0f, 12.0f, juce::Justification::left);
}

void STEVEScope::paintWaveform(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    const float centerY = bounds.getCentreY();
    const float halfHeight = bounds.getHeight() * 0.4f;  // 80% of height for waveform

    // Helper lambda to apply scale (linear or log)
    auto applyScale = [this](float value) -> float
    {
        if (!useLogScale)
            return value;  // Linear scale

        // Logarithmic scale: convert amplitude to dB, then normalize
        // Range: -60dB to 0dB mapped to 0-1
        float absVal = std::abs(value);
        if (absVal < 0.001f)
            return 0.0f;  // Below -60dB

        float dB = 20.0f * std::log10(absVal);
        float normalized = (dB + 60.0f) / 60.0f;  // -60dB -> 0, 0dB -> 1
        normalized = juce::jlimit(0.0f, 1.0f, normalized);

        // Preserve sign for waveform display
        return (value >= 0.0f) ? normalized : -normalized;
    };

    // ============ COMMON RENDERING ============
    if (!processor.decimatedDisplayReady.load())
        return;

    // Always read from decimatedDisplay - the live data source
    const auto& decimated = processor.decimatedDisplay;
    const int totalSegments = QuadBlendDriveAudioProcessor::decimatedDisplaySize;

    // Calculate zoom based on time base
    constexpr float bufferDurationMs = 4096.0f;
    float timeBaseDurationMs = getTimeBaseDurationSeconds() * 1000.0f;

    float zoomRatio = juce::jlimit(0.1f, 1.0f, timeBaseDurationMs / bufferDurationMs);
    int numSegments = static_cast<int>(totalSegments * zoomRatio);
    numSegments = juce::jlimit(32, totalSegments, numSegments);

    int startSegment = totalSegments - numSegments;
    const float segmentWidth = bounds.getWidth() / static_cast<float>(numSegments);

    // Helper lambda to get waveform value based on channel mode
    auto getWaveformValues = [this](const QuadBlendDriveAudioProcessor::DecimatedSegment& seg, float& outMin, float& outMax)
    {
        switch (channelMode)
        {
            case ChannelMode::Left:
                outMin = seg.waveformMinL;
                outMax = seg.waveformMaxL;
                break;
            case ChannelMode::Right:
                outMin = seg.waveformMinR;
                outMax = seg.waveformMaxR;
                break;
            case ChannelMode::LR_Sum:
            case ChannelMode::Mid:
                outMin = (seg.waveformMinL + seg.waveformMinR) * 0.5f;
                outMax = (seg.waveformMaxL + seg.waveformMaxR) * 0.5f;
                break;
            case ChannelMode::Side:
                outMin = (seg.waveformMinL - seg.waveformMaxR) * 0.5f;
                outMax = (seg.waveformMaxL - seg.waveformMinR) * 0.5f;
                break;
            case ChannelMode::MS_Sum:
            {
                float midMin = (seg.waveformMinL + seg.waveformMinR) * 0.5f;
                float midMax = (seg.waveformMaxL + seg.waveformMaxR) * 0.5f;
                float sideMin = (seg.waveformMinL - seg.waveformMaxR) * 0.5f;
                float sideMax = (seg.waveformMaxL - seg.waveformMinR) * 0.5f;
                outMin = (midMin + sideMin) * 0.5f;
                outMax = (midMax + sideMax) * 0.5f;
                break;
            }
            default:
                outMin = seg.waveformMin;
                outMax = seg.waveformMax;
                break;
        }
    };

    // Helper to get the segment data for a display position
    // In scroll mode: read from live decimatedDisplay buffer (linear, newest on right)
    // In playhead mode: read from frozenDisplay buffer (data frozen as playhead passed)
    auto getSegment = [&](int displayIdx) -> const QuadBlendDriveAudioProcessor::DecimatedSegment&
    {
        if (scrollEnabled)
        {
            // Scroll mode: direct linear mapping from live buffer
            return decimated[startSegment + displayIdx];
        }
        else
        {
            // Playhead mode: read from frozen buffer
            // The frozenDisplay contains data that was captured as the playhead swept past
            int idx = juce::jlimit(0, frozenBufferSize - 1, displayIdx);
            return frozenDisplay[idx];
        }
    };

    // Build waveform path (min/max envelope)
    juce::Path waveformPath;

    // First pass: top envelope (max values)
    for (int i = 0; i < numSegments; ++i)
    {
        const auto& seg = getSegment(i);
        float waveMin, waveMax;
        getWaveformValues(seg, waveMin, waveMax);
        waveMax = applyScale(waveMax);

        float x = bounds.getX() + i * segmentWidth;
        float y = centerY - waveMax * halfHeight;
        y = juce::jlimit(bounds.getY(), bounds.getBottom(), y);

        if (i == 0)
            waveformPath.startNewSubPath(x, y);
        else
            waveformPath.lineTo(x, y);
    }

    // Second pass: bottom envelope (min values, reversed)
    for (int i = numSegments - 1; i >= 0; --i)
    {
        const auto& seg = getSegment(i);
        float waveMin, waveMax;
        getWaveformValues(seg, waveMin, waveMax);
        waveMin = applyScale(waveMin);

        float x = bounds.getX() + i * segmentWidth;
        float y = centerY - waveMin * halfHeight;
        y = juce::jlimit(bounds.getY(), bounds.getBottom(), y);
        waveformPath.lineTo(x, y);
    }

    waveformPath.closeSubPath();

    // Draw filled waveform with frequency-based coloring if enabled
    if (showFrequencyBands)
    {
        // Per-segment frequency coloring - original approach with full brightness
        // Draw at pixel resolution for smoothness
        const int pixelWidth = static_cast<int>(bounds.getWidth());
        const int drawSteps = juce::jmax(pixelWidth, numSegments * 2);
        const float stepWidth = bounds.getWidth() / static_cast<float>(drawSteps);

        for (int step = 0; step < drawSteps; ++step)
        {
            // Map step to segment index
            int segIdx = (step * numSegments) / drawSteps;
            segIdx = juce::jlimit(0, numSegments - 1, segIdx);

            const auto& seg = getSegment(segIdx);

            float waveMin, waveMax;
            getWaveformValues(seg, waveMin, waveMax);
            waveMax = applyScale(waveMax);
            waveMin = applyScale(waveMin);

            float x = bounds.getX() + step * stepWidth;

            // RGB from frequency bands - use higher color scale for full brightness
            float colorScale = 6.0f;  // Boosted for visibility
            uint8_t red = static_cast<uint8_t>(juce::jlimit(0.0f, 255.0f, seg.avgLow * colorScale * 255.0f));
            uint8_t green = static_cast<uint8_t>(juce::jlimit(0.0f, 255.0f, seg.avgMid * colorScale * 255.0f));
            uint8_t blue = static_cast<uint8_t>(juce::jlimit(0.0f, 255.0f, seg.avgHigh * colorScale * 255.0f));

            // Ensure minimum brightness - boost all channels if too dark
            uint8_t maxChannel = juce::jmax(red, juce::jmax(green, blue));
            if (maxChannel < 80 && maxChannel > 0)
            {
                float boost = 80.0f / maxChannel;
                red = static_cast<uint8_t>(juce::jmin(255.0f, red * boost));
                green = static_cast<uint8_t>(juce::jmin(255.0f, green * boost));
                blue = static_cast<uint8_t>(juce::jmin(255.0f, blue * boost));
            }
            else if (maxChannel == 0)
            {
                red = green = blue = 60;  // Silent - dim grey
            }

            juce::Colour segmentColor(red, green, blue);
            g.setColour(segmentColor);

            // Draw positive half: from center up to max peak
            if (waveMax > 0.0f)
            {
                float peakY = centerY - waveMax * halfHeight;
                peakY = juce::jlimit(bounds.getY(), centerY, peakY);
                float height = centerY - peakY;
                if (height > 0.5f)
                    g.fillRect(x, peakY, stepWidth + 0.5f, height);
            }

            // Draw negative half: from center down to min trough
            if (waveMin < 0.0f)
            {
                float troughY = centerY - waveMin * halfHeight;
                troughY = juce::jlimit(centerY, bounds.getBottom(), troughY);
                float height = troughY - centerY;
                if (height > 0.5f)
                    g.fillRect(x, centerY, stepWidth + 0.5f, height);
            }
        }
    }
    else
    {
        // Solid green waveform
        g.setColour(juce::Colour(WAVEFORM_COLOR).withAlpha(0.6f));
        g.fillPath(waveformPath);

        g.setColour(juce::Colour(WAVEFORM_COLOR));
        g.strokePath(waveformPath, juce::PathStrokeType(1.5f));
    }
}

void STEVEScope::paintGRTraces(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    // GR display range
    const float maxGRdB = 24.0f;  // Max GR depth to display
    const float grScale = bounds.getHeight() * 0.3f / maxGRdB;  // 30% of height for GR

    // Get threshold position for GR baseline
    float thresholdDb = processor.apvts.getRawParameterValue("THRESHOLD")->load();
    float threshNorm = (thresholdDb + 60.0f) / 66.0f;
    threshNorm = juce::jlimit(0.0f, 1.0f, threshNorm);
    float baselineY = bounds.getBottom() - threshNorm * bounds.getHeight();

    // ============ COMMON RENDERING ============
    if (!processor.decimatedDisplayReady.load())
        return;

    // Read from live decimatedDisplay for scroll mode
    const auto& decimated = processor.decimatedDisplay;
    const int totalSegments = QuadBlendDriveAudioProcessor::decimatedDisplaySize;

    // Calculate zoom based on time base
    constexpr float bufferDurationMs = 4096.0f;
    float timeBaseDurationMs = getTimeBaseDurationSeconds() * 1000.0f;
    float zoomRatio = juce::jlimit(0.1f, 1.0f, timeBaseDurationMs / bufferDurationMs);
    int numSegments = static_cast<int>(totalSegments * zoomRatio);
    numSegments = juce::jlimit(32, totalSegments, numSegments);
    int startSegment = totalSegments - numSegments;

    const float segmentWidth = bounds.getWidth() / static_cast<float>(numSegments);

    // Helper to get the segment data for a display position
    // In scroll mode: read from live decimatedDisplay buffer (linear, newest on right)
    // In playhead mode: read from frozenDisplay buffer (data frozen as playhead passed)
    auto getSegment = [&](int displayIdx) -> const QuadBlendDriveAudioProcessor::DecimatedSegment&
    {
        if (scrollEnabled)
        {
            // Scroll mode: direct linear mapping from live buffer
            return decimated[startSegment + displayIdx];
        }
        else
        {
            // Playhead mode: read from frozen buffer
            int idx = juce::jlimit(0, frozenBufferSize - 1, displayIdx);
            return frozenDisplay[idx];
        }
    };

    if (useMultiColorGR)
    {
        // Draw per-processor GR traces
        struct ProcessorTrace {
            uint32_t color;
            float (QuadBlendDriveAudioProcessor::DecimatedSegment::*grMin);
            float (QuadBlendDriveAudioProcessor::DecimatedSegment::*grMax);
        };

        ProcessorTrace traces[] = {
            { HC_COLOR, &QuadBlendDriveAudioProcessor::DecimatedSegment::hardClipGRMin, &QuadBlendDriveAudioProcessor::DecimatedSegment::hardClipGRMax },
            { SC_COLOR, &QuadBlendDriveAudioProcessor::DecimatedSegment::softClipGRMin, &QuadBlendDriveAudioProcessor::DecimatedSegment::softClipGRMax },
            { SL_COLOR, &QuadBlendDriveAudioProcessor::DecimatedSegment::slowLimitGRMin, &QuadBlendDriveAudioProcessor::DecimatedSegment::slowLimitGRMax },
            { FL_COLOR, &QuadBlendDriveAudioProcessor::DecimatedSegment::fastLimitGRMin, &QuadBlendDriveAudioProcessor::DecimatedSegment::fastLimitGRMax }
        };

        for (const auto& trace : traces)
        {
            juce::Path grPath;
            bool pathStarted = false;

            for (int i = 0; i < numSegments; ++i)
            {
                const auto& seg = getSegment(i);
                float x = bounds.getX() + i * segmentWidth;
                float grDb = std::abs(seg.*trace.grMax);  // Use max (most reduction)
                grDb = juce::jmin(grDb, maxGRdB);

                float grY = baselineY + grDb * grScale;  // GR hangs down from threshold
                grY = juce::jmin(grY, bounds.getBottom());

                if (!pathStarted)
                {
                    grPath.startNewSubPath(x, grY);
                    pathStarted = true;
                }
                else
                {
                    grPath.lineTo(x, grY);
                }
            }

            // Stroke the GR trace
            g.setColour(juce::Colour(trace.color).withAlpha(0.6f));
            g.strokePath(grPath, juce::PathStrokeType(1.5f));
        }
    }
    else
    {
        // Unified GR trace (combined reduction)
        juce::Path grPath;
        grPath.startNewSubPath(bounds.getX(), baselineY);

        for (int i = 0; i < numSegments; ++i)
        {
            const auto& seg = getSegment(i);
            float x = bounds.getX() + i * segmentWidth;

            // Use total GR from the segment
            float grDb = std::abs(seg.grMax);
            grDb = juce::jmin(grDb, maxGRdB);

            float grY = baselineY + grDb * grScale;
            grY = juce::jmin(grY, bounds.getBottom());

            grPath.lineTo(x, grY);
        }

        // Close path back to baseline
        grPath.lineTo(bounds.getRight(), baselineY);
        grPath.closeSubPath();

        // Fill with semi-transparent
        g.setColour(juce::Colour(UNIFIED_GR_COLOR).withAlpha(0.3f));
        g.fillPath(grPath);

        // Stroke outline
        g.setColour(juce::Colour(UNIFIED_GR_COLOR).withAlpha(0.8f));
        g.strokePath(grPath, juce::PathStrokeType(1.5f));
    }
}

void STEVEScope::paintPlayhead(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    // Only draw playhead when scroll is OFF (playhead mode)
    if (scrollEnabled)
        return;

    // Draw playhead cursor synced with display refresh
    float cursorX = bounds.getX() + (displayCursorPos / static_cast<float>(displayFrames)) * bounds.getWidth();

    g.setColour(juce::Colour(PLAYHEAD_COLOR).withAlpha(0.5f));
    g.drawVerticalLine(static_cast<int>(cursorX), bounds.getY(), bounds.getBottom());
}

void STEVEScope::resized()
{
    graphBounds = getLocalBounds().toFloat().reduced(8.0f).withTrimmedTop(20.0f);

    // Position settings button in upper right corner
    auto bounds = getLocalBounds().toFloat();
    settingsButtonBounds = juce::Rectangle<float>(bounds.getRight() - 24.0f, bounds.getY() + 2.0f, 20.0f, 16.0f);
}

void STEVEScope::timerCallback()
{
    // Update decimated display cache from high-resolution ring buffer
    processor.updateDecimatedDisplay();

    // Update display frames based on current tempo and time base
    updateDisplayFrames();

    if (!scrollEnabled)
    {
        // PLAYHEAD MODE: Sync to DAW transport PPQ position
        // The playhead position is derived from DAW's beat position

        // Calculate zoom settings
        const int totalSegments = QuadBlendDriveAudioProcessor::decimatedDisplaySize;
        constexpr float bufferDurationMs = 4096.0f;
        float timeBaseDurationMs = getTimeBaseDurationSeconds() * 1000.0f;
        float zoomRatio = juce::jlimit(0.1f, 1.0f, timeBaseDurationMs / bufferDurationMs);
        int numSegments = static_cast<int>(totalSegments * zoomRatio);
        numSegments = juce::jlimit(32, totalSegments, numSegments);

        // Get playhead position from DAW PPQ (beat position)
        // PPQ = pulses per quarter note, essentially beat position
        double ppq = processor.currentPPQPosition.load();
        float timeBaseBeats = getTimeBaseBeats();

        // Calculate position within the current time base window
        // Use fmod to wrap PPQ position within the time base duration
        double ppqInWindow = std::fmod(ppq, static_cast<double>(timeBaseBeats));
        if (ppqInWindow < 0.0) ppqInWindow += timeBaseBeats;  // Handle negative PPQ

        float playheadRatio = static_cast<float>(ppqInWindow / timeBaseBeats);
        playheadRatio = juce::jlimit(0.0f, 0.9999f, playheadRatio);

        // Convert to display cursor position
        displayCursorPos = static_cast<int>(playheadRatio * displayFrames);
        displayCursorPos = juce::jlimit(0, displayFrames - 1, displayCursorPos);

        // Calculate which display segment the playhead is at
        int playheadSegment = static_cast<int>(playheadRatio * numSegments);
        playheadSegment = juce::jlimit(0, numSegments - 1, playheadSegment);

        // Copy newest data from decimatedDisplay to frozenDisplay at playhead position
        const auto& decimated = processor.decimatedDisplay;
        int startSegment = totalSegments - numSegments;

        // Detect if playhead wrapped around (new cycle)
        bool wrapped = (playheadSegment < lastPlayheadDisplaySegment - numSegments / 4);

        // Calculate how many segments to update
        int lastSegment = lastPlayheadDisplaySegment;
        if (lastSegment < 0 || lastSegment >= numSegments || wrapped)
            lastSegment = playheadSegment;  // First frame or wrap reset

        // Copy segments from last position to current position
        if (playheadSegment >= lastSegment)
        {
            // Normal forward sweep
            for (int i = lastSegment; i <= playheadSegment && i < frozenBufferSize; ++i)
            {
                int offsetFromPlayhead = playheadSegment - i;
                int sourceIdx = (startSegment + numSegments - 1) - offsetFromPlayhead;
                sourceIdx = juce::jlimit(0, totalSegments - 1, sourceIdx);
                frozenDisplay[i] = decimated[sourceIdx];
            }
        }
        else if (!wrapped)
        {
            // Edge case: playhead moved backwards slightly (seek)
            // Just update current position
            int sourceIdx = startSegment + numSegments - 1;
            sourceIdx = juce::jlimit(0, totalSegments - 1, sourceIdx);
            frozenDisplay[playheadSegment] = decimated[sourceIdx];
        }

        lastPlayheadDisplaySegment = playheadSegment;
        currentNumSegments = numSegments;
        currentStartSegment = startSegment;
    }
    else
    {
        // SCROLL MODE: Just advance frame counter (for animations if any)
        displayCursorPos = (displayCursorPos + 1) % displayFrames;
    }

    repaint();
}

void STEVEScope::updateFrozenDisplay()
{
    // In playhead mode, we don't need to copy data anymore.
    // Instead, we render the decimated buffer as a circular buffer,
    // with the playhead showing the current position.
    //
    // The key insight: the decimatedDisplay buffer is continuously updated
    // and represents a sliding window of recent audio. For playhead mode,
    // we simply need to know WHERE in this buffer corresponds to "now"
    // and render accordingly, treating it as a circular buffer.
    //
    // This function now just updates the segment counts for rendering.

    if (!processor.decimatedDisplayReady.load())
        return;

    const int totalSegments = QuadBlendDriveAudioProcessor::decimatedDisplaySize;

    // Calculate zoom based on time base
    constexpr float bufferDurationMs = 4096.0f;
    float timeBaseDurationMs = getTimeBaseDurationSeconds() * 1000.0f;
    float zoomRatio = juce::jlimit(0.1f, 1.0f, timeBaseDurationMs / bufferDurationMs);
    int numSegments = static_cast<int>(totalSegments * zoomRatio);
    numSegments = juce::jlimit(32, totalSegments, numSegments);

    // Store current zoom settings for rendering
    currentNumSegments = numSegments;
    currentStartSegment = totalSegments - numSegments;
}

void STEVEScope::paintSettingsButton(juce::Graphics& g)
{
    // Draw a simple gear/settings icon
    float alpha = isHoveringSettings ? 1.0f : 0.6f;
    g.setColour(juce::Colours::white.withAlpha(alpha));

    // Draw three horizontal lines (hamburger menu style)
    float x = settingsButtonBounds.getX();
    float y = settingsButtonBounds.getY() + 4.0f;
    float w = settingsButtonBounds.getWidth() - 4.0f;

    g.drawLine(x, y, x + w, y, 1.5f);
    g.drawLine(x, y + 4.0f, x + w, y + 4.0f, 1.5f);
    g.drawLine(x, y + 8.0f, x + w, y + 8.0f, 1.5f);
}

float STEVEScope::getTimeBaseBeats() const
{
    switch (timeBase)
    {
        case TimeBase::Div_1_32: return 0.125f;    // 1/32 note = 1/8 beat
        case TimeBase::Div_1_16: return 0.25f;     // 1/16 note = 1/4 beat
        case TimeBase::Div_1_8:  return 0.5f;      // 1/8 note = 1/2 beat
        case TimeBase::Div_1_4:  return 1.0f;      // 1/4 note = 1 beat
        case TimeBase::Div_2_4:  return 2.0f;      // 2/4 = 2 beats
        case TimeBase::Div_4_4:  return 4.0f;      // 4/4 = 4 beats (1 bar)
        case TimeBase::Bars_2:   return 8.0f;      // 2 bars = 8 beats
        case TimeBase::Bars_4:   return 16.0f;     // 4 bars = 16 beats
        case TimeBase::Bars_8:   return 32.0f;     // 8 bars = 32 beats
        case TimeBase::Bars_16:  return 64.0f;     // 16 bars = 64 beats
        default:                 return 4.0f;
    }
}

float STEVEScope::getTimeBaseDurationSeconds() const
{
    float beats = getTimeBaseBeats();
    double bpm = processor.currentBPM.load();
    if (bpm <= 0.0)
        bpm = 120.0;  // Default fallback

    // beats / (beats per minute) * 60 seconds = duration in seconds
    return beats / static_cast<float>(bpm) * 60.0f;
}

void STEVEScope::updateDisplayFrames()
{
    // Calculate how many frames (at 30fps) fit in the time base duration
    float durationSeconds = getTimeBaseDurationSeconds();
    constexpr float fps = 30.0f;

    // Clamp to reasonable range (minimum 10 frames, maximum 600 frames = 20 seconds)
    displayFrames = juce::jlimit(10, 600, static_cast<int>(durationSeconds * fps));
}

juce::String STEVEScope::getChannelModeName(ChannelMode mode)
{
    switch (mode)
    {
        case ChannelMode::Left:   return "Left";
        case ChannelMode::Right:  return "Right";
        case ChannelMode::LR_Sum: return "L+R";
        case ChannelMode::Mid:    return "Mid";
        case ChannelMode::Side:   return "Side";
        case ChannelMode::MS_Sum: return "M+S";
        default:                  return "Mid";
    }
}

juce::String STEVEScope::getTimeBaseName(TimeBase tb)
{
    switch (tb)
    {
        case TimeBase::Div_1_32: return "1/32";
        case TimeBase::Div_1_16: return "1/16";
        case TimeBase::Div_1_8:  return "1/8";
        case TimeBase::Div_1_4:  return "1/4";
        case TimeBase::Div_2_4:  return "2/4";
        case TimeBase::Div_4_4:  return "4/4";
        case TimeBase::Bars_2:   return "2 BARS";
        case TimeBase::Bars_4:   return "4 BARS";
        case TimeBase::Bars_8:   return "8 BARS";
        case TimeBase::Bars_16:  return "16 BARS";
        default:                 return "4/4";
    }
}

void STEVEScope::showSettingsMenu()
{
    juce::PopupMenu menu;

    // === SCALE (LIN/LOG) ===
    juce::PopupMenu scaleMenu;
    scaleMenu.addItem(100, "Linear", true, !useLogScale);
    scaleMenu.addItem(101, "Logarithmic (dB)", true, useLogScale);
    menu.addSubMenu("Scale", scaleMenu);

    // === SCROLL (Off/On) ===
    juce::PopupMenu scrollMenu;
    scrollMenu.addItem(110, "Off (Playhead)", true, !scrollEnabled);
    scrollMenu.addItem(111, "On (Continuous)", true, scrollEnabled);
    menu.addSubMenu("Scroll", scrollMenu);

    // === TIME BASE ===
    juce::PopupMenu timeBaseMenu;
    timeBaseMenu.addItem(120, "1/32", true, timeBase == TimeBase::Div_1_32);
    timeBaseMenu.addItem(121, "1/16", true, timeBase == TimeBase::Div_1_16);
    timeBaseMenu.addItem(122, "1/8", true, timeBase == TimeBase::Div_1_8);
    timeBaseMenu.addItem(123, "1/4", true, timeBase == TimeBase::Div_1_4);
    timeBaseMenu.addItem(124, "2/4", true, timeBase == TimeBase::Div_2_4);
    timeBaseMenu.addItem(125, "4/4", true, timeBase == TimeBase::Div_4_4);
    timeBaseMenu.addItem(126, "2 BARS", true, timeBase == TimeBase::Bars_2);
    timeBaseMenu.addItem(127, "4 BARS", true, timeBase == TimeBase::Bars_4);
    timeBaseMenu.addItem(128, "8 BARS", true, timeBase == TimeBase::Bars_8);
    timeBaseMenu.addItem(129, "16 BARS", true, timeBase == TimeBase::Bars_16);
    menu.addSubMenu("Time Base", timeBaseMenu);

    // === CHANNEL ===
    juce::PopupMenu channelMenu;
    channelMenu.addItem(130, "Left", true, channelMode == ChannelMode::Left);
    channelMenu.addItem(131, "Right", true, channelMode == ChannelMode::Right);
    channelMenu.addItem(132, "L+R", true, channelMode == ChannelMode::LR_Sum);
    channelMenu.addItem(133, "Mid", true, channelMode == ChannelMode::Mid);
    channelMenu.addItem(134, "Side", true, channelMode == ChannelMode::Side);
    channelMenu.addItem(135, "M+S", true, channelMode == ChannelMode::MS_Sum);
    menu.addSubMenu("Channel", channelMenu);

    menu.addSeparator();

    // GR Display Mode submenu
    juce::PopupMenu grModeMenu;
    grModeMenu.addItem(1, "Multi-Color (Per-Processor)", true, useMultiColorGR);
    grModeMenu.addItem(2, "Unified (Accent Blue)", true, !useMultiColorGR);
    menu.addSubMenu("GR Display Mode", grModeMenu);

    menu.addSeparator();

    // Waveform display options
    menu.addItem(3, "Show Frequency Colors", true, showFrequencyBands);
    menu.addItem(4, "Show GR Traces", true, showGRTraces);

    menu.addSeparator();

    // Legend
    juce::PopupMenu legendMenu;
    legendMenu.addItem(-1, "HC = Hard Clip (Red)", false, false);
    legendMenu.addItem(-1, "SC = Soft Clip (Orange)", false, false);
    legendMenu.addItem(-1, "SL = Slow Limit (Yellow)", false, false);
    legendMenu.addItem(-1, "FL = Fast Limit (Blue)", false, false);
    menu.addSubMenu("GR Color Legend", legendMenu);

    // Position menu below the settings button (not at top of component)
    auto settingsBoundsInt = settingsButtonBounds.toNearestInt();
    auto screenPos = localPointToGlobal(juce::Point<int>(settingsBoundsInt.getX(), settingsBoundsInt.getBottom()));
    menu.showMenuAsync(juce::PopupMenu::Options()
        .withTargetComponent(this)
        .withTargetScreenArea(juce::Rectangle<int>(screenPos.x, screenPos.y, 1, 1)),
        [this](int result)
        {
            switch (result)
            {
                // GR Display Mode
                case 1:
                    useMultiColorGR = true;
                    break;
                case 2:
                    useMultiColorGR = false;
                    break;
                // Waveform options
                case 3:
                    showFrequencyBands = !showFrequencyBands;
                    break;
                case 4:
                    showGRTraces = !showGRTraces;
                    break;

                // Scale
                case 100:
                    useLogScale = false;
                    break;
                case 101:
                    useLogScale = true;
                    break;

                // Scroll
                case 110:
                    scrollEnabled = false;
                    break;
                case 111:
                    scrollEnabled = true;
                    break;

                // Time Base
                case 120: timeBase = TimeBase::Div_1_32; break;
                case 121: timeBase = TimeBase::Div_1_16; break;
                case 122: timeBase = TimeBase::Div_1_8; break;
                case 123: timeBase = TimeBase::Div_1_4; break;
                case 124: timeBase = TimeBase::Div_2_4; break;
                case 125: timeBase = TimeBase::Div_4_4; break;
                case 126: timeBase = TimeBase::Bars_2; break;
                case 127: timeBase = TimeBase::Bars_4; break;
                case 128: timeBase = TimeBase::Bars_8; break;
                case 129: timeBase = TimeBase::Bars_16; break;

                // Channel
                case 130: channelMode = ChannelMode::Left; break;
                case 131: channelMode = ChannelMode::Right; break;
                case 132: channelMode = ChannelMode::LR_Sum; break;
                case 133: channelMode = ChannelMode::Mid; break;
                case 134: channelMode = ChannelMode::Side; break;
                case 135: channelMode = ChannelMode::MS_Sum; break;

                default:
                    break;
            }
            repaint();
        });
}

void STEVEScope::mouseDown(const juce::MouseEvent& e)
{
    // Check if click is on settings button
    if (settingsButtonBounds.contains(e.position))
    {
        showSettingsMenu();
        return;
    }

    // Right-click anywhere shows settings menu
    if (e.mods.isRightButtonDown())
    {
        showSettingsMenu();
        return;
    }

    // Shift-click to toggle frequency bands (legacy shortcut)
    if (e.mods.isShiftDown())
    {
        showFrequencyBands = !showFrequencyBands;
        repaint();
    }
}

juce::Colour STEVEScope::getProcessorColor(int index) const
{
    if (!useMultiColorGR)
        return juce::Colour(UNIFIED_GR_COLOR);

    switch (index)
    {
        case 0: return juce::Colour(HC_COLOR);
        case 1: return juce::Colour(SC_COLOR);
        case 2: return juce::Colour(SL_COLOR);
        case 3: return juce::Colour(FL_COLOR);
        default: return juce::Colour(UNIFIED_GR_COLOR);
    }
}

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

/**
 * Stereo Meter Component
 *
 * Displays stereo peak levels as vertical bars with hard color cuts:
 * - Green: Below 0dBFS (<0dB)
 * - Orange: At or above 0dBFS (0dB to +1dB)
 * - Red: Hot peaks (>+1dB)
 *
 * Peak follower: Instant attack, ~50ms release
 */
class StereoMeter : public juce::Component, public juce::Timer
{
public:
    StereoMeter(QuadBlendDriveAudioProcessor& p, bool isInputMeter);

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;

    QuadBlendDriveAudioProcessor& processor;
    bool isInputMeter;  // true = input meter, false = output meter

    // Peak hold tracking
    float peakHoldL{0.0f};
    float peakHoldR{0.0f};
    juce::int64 peakHoldTimeL{0};
    juce::int64 peakHoldTimeR{0};
    static constexpr juce::int64 peakHoldDurationMs = 3000;  // 3 seconds (unified across all meters)

    // Display constants
    static constexpr int CHANNEL_GAP = 4;              // Gap between L/R channels
    static constexpr float ORANGE_THRESHOLD_DB = 0.0f;  // Green → Orange (hard cut)
    static constexpr float RED_THRESHOLD_DB = 1.0f;     // Orange → Red (hard cut)

    // Get color for given level (0-1 normalized)
    static juce::Colour getColorForLevel(float level);
};

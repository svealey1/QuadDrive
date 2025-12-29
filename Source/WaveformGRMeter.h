#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

/**
 * Dynamic Waveform Gain Reduction Meter
 *
 * Shows a scrolling waveform display with:
 * - Input waveform (gray, 40% opacity)
 * - Output waveform (white, 100% opacity)
 * - Gain reduction gap (dynamically colored based on which processors are reducing)
 *
 * The gap between input and output is filled with a color that blends proportionally
 * based on which processors are actively causing gain reduction at each moment:
 * - Red: Hard Clip
 * - Orange: Soft Clip
 * - Yellow: Slow Limit
 * - Blue: Fast Limit
 */
class WaveformGRMeter : public juce::Component, private juce::Timer
{
public:
    WaveformGRMeter(QuadBlendDriveAudioProcessor& p);

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Visual solo functionality (does not affect audio)
    void soloProcessor(int processorIndex);  // 0=HC, 1=SC, 2=SL, 3=FL, -1=none
    void unsoloAll();
    bool isProcessorSoloed(int processorIndex) const;

private:
    void timerCallback() override;

    // Render the waveform display
    void paintWaveform(juce::Graphics& g, const juce::Rectangle<float>& bounds);

    // Get color for a processor based on its contribution
    juce::Colour getProcessorColor(int processorIndex) const;

    // Calculate blended color based on per-processor GR ratios
    juce::Colour calculateGapColor(float hcGR, float scGR, float slGR, float flGR) const;

    QuadBlendDriveAudioProcessor& processor;

    // Visual solo state (audio unaffected)
    int soloedProcessor = -1;  // -1 = none soloed, 0-3 = specific processor soloed

    // Display constants
    static constexpr float INPUT_OPACITY = 0.4f;
    static constexpr float OUTPUT_OPACITY = 1.0f;
};

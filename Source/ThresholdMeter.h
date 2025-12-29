#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

//==============================================================================
// Threshold-Referenced Drive Meter
// Shows signal relative to threshold:
// - Above threshold: Output signal (white) showing overshoot
// - Below threshold: Per-drive-type gain reduction (colored layers)
//==============================================================================
class ThresholdMeter : public juce::Component, private juce::Timer
{
public:
    ThresholdMeter(QuadBlendDriveAudioProcessor& p);

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Layer visibility control (for Phase 4: solo functionality)
    void setLayerVisible(int layerIndex, bool visible);
    bool isLayerVisible(int layerIndex) const;
    void soloLayer(int layerIndex);  // Visual solo (audio unchanged)
    void unsoloAll();

private:
    void timerCallback() override;

    // Paint output signal above threshold
    void paintOutputAboveThreshold(juce::Graphics& g, const juce::Rectangle<float>& bounds, float thresholdY);

    // Paint gain reduction layers below threshold
    void paintGainReductionLayers(juce::Graphics& g, const juce::Rectangle<float>& bounds, float thresholdY);

    QuadBlendDriveAudioProcessor& processor;

    // Layer visibility flags (0=HC, 1=SC, 2=SL, 3=FL)
    bool layerVisible[4] = { true, true, true, true };
    int soloedLayer = -1;  // -1 = none soloed

    // Layer colors (fixed 60% opacity)
    static constexpr uint8_t LAYER_OPACITY = 153;  // 60% of 255
    juce::Colour getLayerColor(int layerIndex) const;
    const char* getLayerName(int layerIndex) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ThresholdMeter)
};

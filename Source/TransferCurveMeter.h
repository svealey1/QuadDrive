#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include "PluginProcessor.h"

/**
 * Transfer Curve Meter
 *
 * Visualizes the composite saturation transfer function based on:
 * - XY pad position (blends between 4 processor curves)
 * - Per-processor drive settings
 * - Current signal level (green portion of curve)
 * - Threshold reference line (draggable)
 * - Input/output level indicators
 *
 * XY Corner Mapping:
 * - Top-left: Hard Clip
 * - Top-right: Fast Limit
 * - Bottom-left: Soft Clip
 * - Bottom-right: Slow Limit
 */
class TransferCurveMeter : public juce::Component, public juce::Timer
{
public:
    TransferCurveMeter(QuadBlendDriveAudioProcessor& p);

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;

private:
    void timerCallback() override;

    // Transfer curve data (144 points)
    struct TransferCurveData
    {
        std::array<float, 144> y;
    };

    // Generate composite curve based on XY position and drive settings
    void generateCompositeCurve(TransferCurveData& data,
                                float X, float Y,
                                float driveHC, float driveSC,
                                float driveSL, float driveFL,
                                int soloIndex);

    // Individual transfer functions (normalized 0-1 range)
    static float hardClip(float x, float drive);
    static float softClip(float x, float drive);
    static float slowLimit(float x, float drive);
    static float fastLimit(float x, float drive);
    static float softKneeCompress(float x, float thresh, float ratio, float knee);

    // Helper to convert dB to normalized position in display range
    float dbToNormalized(float dB) const;
    float normalizedToDb(float norm) const;

    QuadBlendDriveAudioProcessor& processor;

    // Square bounds for transfer curve (maintained in resized())
    juce::Rectangle<float> curveBounds;

    // Threshold dragging state
    bool isDraggingThreshold = false;
    bool isHoveringThreshold = false;
    static constexpr float thresholdHitZone = 8.0f;  // Pixels for threshold grab zone

    // Display colors
    static constexpr uint32_t BACKGROUND_COLOR = 0xFF0a0a0a;
    static constexpr uint32_t GREEN_COLOR = 0xFF00D26A;
    static constexpr uint32_t ACCENT_BLUE = 0xFF4a9eff;
    static constexpr uint32_t WARNING_AMBER = 0xFFffaa00;
    static constexpr float WHITE_ALPHA = 0.4f;
    static constexpr float GRID_ALPHA = 0.08f;

    // Display range in dB
    static constexpr float displayMinDB = -60.0f;
    static constexpr float displayMaxDB = 6.0f;
};

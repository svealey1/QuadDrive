#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

/**
 * EmulsionScope - Mega Scope-inspired waveform and GR visualization
 *
 * Features:
 * - Smooth waveform rendering using decimated display buffer
 * - Per-processor GR traces (color-coded or unified)
 * - Threshold reference line
 * - Transport-synced playhead cursor
 * - RGB frequency band coloring option
 * - Settings button with popup menu for visualization options
 */
class STEVEScope : public juce::Component, private juce::Timer
{
public:
    STEVEScope(QuadBlendDriveAudioProcessor& p);
    ~STEVEScope() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;

    // Visualization settings
    void setMultiColorGR(bool multiColor) { useMultiColorGR = multiColor; repaint(); }
    bool isMultiColorGR() const { return useMultiColorGR; }
    void setShowFrequencyBands(bool show) { showFrequencyBands = show; repaint(); }
    bool isShowingFrequencyBands() const { return showFrequencyBands; }
    void setShowGRTraces(bool show) { showGRTraces = show; repaint(); }
    bool isShowingGRTraces() const { return showGRTraces; }

private:
    void timerCallback() override;
    void paintWaveform(juce::Graphics& g, juce::Rectangle<float> bounds);
    void paintGRTraces(juce::Graphics& g, juce::Rectangle<float> bounds);
    void paintPlayhead(juce::Graphics& g, juce::Rectangle<float> bounds);
    void paintGrid(juce::Graphics& g, juce::Rectangle<float> bounds);
    void paintSettingsButton(juce::Graphics& g);
    void showSettingsMenu();

    // Helper to get processor color
    juce::Colour getProcessorColor(int index) const;

    QuadBlendDriveAudioProcessor& processor;

    // Display settings
    bool useMultiColorGR = true;      // Multi-color or unified blue
    bool showFrequencyBands = true;   // RGB frequency coloring
    bool showGRTraces = true;         // Show GR overlay

    // Scale setting (linear vs logarithmic)
    bool useLogScale = false;         // false = Linear, true = Logarithmic (dB)

    // Scroll setting
    bool scrollEnabled = true;        // true = continuous scroll (default), false = playhead mode

    // Time base setting (musical divisions based on tempo)
    enum class TimeBase
    {
        Div_1_32,   // 1/32 note
        Div_1_16,   // 1/16 note
        Div_1_8,    // 1/8 note
        Div_1_4,    // 1/4 note
        Div_2_4,    // 2/4 (half bar)
        Div_4_4,    // 4/4 (1 bar)
        Bars_2,     // 2 bars
        Bars_4,     // 4 bars
        Bars_8,     // 8 bars
        Bars_16     // 16 bars
    };
    TimeBase timeBase = TimeBase::Div_4_4;

    // Channel selection
    enum class ChannelMode
    {
        Left,
        Right,
        LR_Sum,     // L+R summed
        Mid,
        Side,
        MS_Sum      // M+S summed
    };
    ChannelMode channelMode = ChannelMode::Mid;

    // Get time base duration in beats
    float getTimeBaseBeats() const;

    // Get time base duration in seconds (based on current BPM)
    float getTimeBaseDurationSeconds() const;

    // Update display frames based on time base and tempo
    void updateDisplayFrames();

    // Get channel mode display name
    static juce::String getChannelModeName(ChannelMode mode);
    static juce::String getTimeBaseName(TimeBase tb);

    // Settings button bounds
    juce::Rectangle<float> settingsButtonBounds;
    bool isHoveringSettings = false;

    // Graph bounds (calculated in resized)
    juce::Rectangle<float> graphBounds;

    // Display cursor for time sync
    int displayCursorPos = 0;
    int displayFrames = 90;  // Dynamic based on time base (default 3 seconds at 30fps)
    int lastPlayheadDisplaySegment = -1;  // Track last display segment for playhead mode

    // Frozen display buffer for playhead mode
    // This stores ACTUAL DATA VALUES captured as the playhead sweeps across
    // Size matches the maximum possible display segments (full buffer, no zoom)
    static constexpr int frozenBufferSize = 8192;
    std::array<QuadBlendDriveAudioProcessor::DecimatedSegment, frozenBufferSize> frozenDisplay;
    int currentNumSegments = 0;  // Current segment count based on zoom
    int currentStartSegment = 0;  // Current start segment based on zoom

    // Update frozen display incrementally at playhead position
    void updateFrozenDisplay();

    // Colors
    static constexpr uint32_t BACKGROUND = 0xff0a0a0a;
    static constexpr uint32_t GRID_COLOR = 0xff303030;
    static constexpr uint32_t WAVEFORM_COLOR = 0xff00d26a;
    static constexpr uint32_t THRESHOLD_COLOR = 0xffffaa00;
    static constexpr uint32_t ZERO_DB_COLOR = 0xffff5050;
    static constexpr uint32_t PLAYHEAD_COLOR = 0xffffffff;

    // Processor colors for multi-color mode
    static constexpr uint32_t HC_COLOR = 0xffff5050;  // Red
    static constexpr uint32_t SC_COLOR = 0xffff963c;  // Orange
    static constexpr uint32_t SL_COLOR = 0xffffdc50;  // Yellow
    static constexpr uint32_t FL_COLOR = 0xff50a0ff;  // Blue
    static constexpr uint32_t UNIFIED_GR_COLOR = 0xff4a9eff;  // Accent blue
};

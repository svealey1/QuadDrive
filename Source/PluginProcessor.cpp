#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

//==============================================================================
QuadBlendDriveAudioProcessor::QuadBlendDriveAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, &undoManager, "Parameters", createParameterLayout())
{
}

QuadBlendDriveAudioProcessor::~QuadBlendDriveAudioProcessor()
{
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
QuadBlendDriveAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Global/I/O Parameters
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "CALIB_LEVEL", "Calibration Level",
        juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " dBFS"; }));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "THRESHOLD", "Threshold",
        juce::NormalisableRange<float>(-24.0f, 0.0f, 0.1f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " dBFS"; }));

    // Threshold Link - automatically tracks Calibration Level when enabled
    layout.add(std::make_unique<juce::AudioParameterBool>(
        "THRESHOLD_LINK", "Threshold Link", true));  // Default ON

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "INPUT_GAIN", "Input Gain",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " dB"; }));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "OUTPUT_GAIN", "Output Gain",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " dB"; }));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "MIX_WET", "Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 100.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " %"; }));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "SC_KNEE", "Soft Clip Knee",
        juce::NormalisableRange<float>(1.0f, 100.0f, 0.1f), 20.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " %"; }));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "LIMIT_REL", "Slow Limiter Release Scale",
        juce::NormalisableRange<float>(10.0f, 500.0f, 1.0f, 0.5f), 100.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 0) + " ms"; }));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "SL_LIMIT_ATTACK", "Slow Limiter Attack Time",
        juce::NormalisableRange<float>(0.1f, 20.0f, 0.01f, 0.5f), 3.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 2) + " ms"; }));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "FL_LIMIT_ATTACK", "Fast Limiter Attack Time",
        juce::NormalisableRange<float>(0.01f, 10.0f, 0.01f, 0.5f), 1.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 2) + " ms"; }));

    // Blending Parameters
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "XY_X_PARAM", "X Position",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));  // Default to center

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "XY_Y_PARAM", "Y Position",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));  // Default to center

    // Utility Parameters
    layout.add(std::make_unique<juce::AudioParameterBool>(
        "DELTA_MODE", "Delta Mode", false));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        "OVERSHOOT_DELTA_MODE", "Overshoot Delta Mode", false));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        "TRUE_PEAK_DELTA_MODE", "True Peak Delta Mode", false));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        "BYPASS", "Bypass", false));

    // Per-Type Trim Parameters (x4)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "HC_TRIM", "Hard Clip Trim",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " dB"; }));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "SC_TRIM", "Soft Clip Trim",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " dB"; }));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "SL_TRIM", "Slow Limit Trim",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " dB"; }));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "FL_TRIM", "Fast Limit Trim",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " dB"; }));

    // Per-Type Mute Parameters (x4) - Default FALSE (not muted)
    layout.add(std::make_unique<juce::AudioParameterBool>(
        "HC_MUTE", "Hard Clip Mute", false));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        "SC_MUTE", "Soft Clip Mute", false));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        "SL_MUTE", "Slow Limit Mute", false));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        "FL_MUTE", "Fast Limit Mute", false));

    // Per-Type Solo Parameters (x4) - Default FALSE (not soloed)
    layout.add(std::make_unique<juce::AudioParameterBool>(
        "HC_SOLO", "Hard Clip Solo", false));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        "SC_SOLO", "Soft Clip Solo", false));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        "SL_SOLO", "Slow Limit Solo", false));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        "FL_SOLO", "Fast Limit Solo", false));

    // Per-Type Gain Compensation Parameters (x4) - Default TRUE (compensate by default)
    layout.add(std::make_unique<juce::AudioParameterBool>(
        "HC_COMP", "Hard Clip Compensation", true));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        "SC_COMP", "Soft Clip Compensation", true));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        "SL_COMP", "Slow Limit Compensation", true));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        "FL_COMP", "Fast Limit Compensation", true));

    // Master Gain Compensation Toggle - Default FALSE (off)
    layout.add(std::make_unique<juce::AudioParameterBool>(
        "MASTER_COMP", "Master Compensation", false));

    // === ENVELOPE SHAPING PARAMETERS ===
    // Per-Type Attack/Sustain Emphasis (x4 types × 2 parameters = 8 total)
    // Allows independent transient vs sustain emphasis before each drive processor

    // Hard Clip Envelope Shaping
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "HC_ATTACK", "Hard Clip Attack Emphasis",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " dB"; }));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "HC_SUSTAIN", "Hard Clip Sustain Emphasis",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " dB"; }));

    // Soft Clip Envelope Shaping
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "SC_ATTACK", "Soft Clip Attack Emphasis",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " dB"; }));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "SC_SUSTAIN", "Soft Clip Sustain Emphasis",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " dB"; }));

    // Slow Limit Envelope Shaping
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "SL_ATTACK", "Slow Limit Attack Emphasis",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " dB"; }));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "SL_SUSTAIN", "Slow Limit Sustain Emphasis",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " dB"; }));

    // Fast Limit Envelope Shaping
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "FL_ATTACK", "Fast Limit Attack Emphasis",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " dB"; }));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "FL_SUSTAIN", "Fast Limit Sustain Emphasis",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " dB"; }));

    // === SIMPLIFIED OUTPUT LIMITING ===
    // Two mutually exclusive limiters sharing ONE ceiling:
    // - OVERSHOOT: Character shaping (accepts some overshoot for punch)
    // - TRUE PEAK: Strict ITU-R BS.1770-4 compliance (maximum transparency)

    // Shared Output Ceiling (used by whichever limiter is active)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "OUTPUT_CEILING", "Output Ceiling",
        juce::NormalisableRange<float>(-3.0f, 0.0f, 0.1f), -0.1f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " dBTP"; }));

    // Overshoot Suppression: Use when some overshoot is OK for punch/warmth
    layout.add(std::make_unique<juce::AudioParameterBool>(
        "OVERSHOOT_ENABLE", "Overshoot", false));

    // True Peak Limiter: Use for strict compliance with maximum transparency
    layout.add(std::make_unique<juce::AudioParameterBool>(
        "TRUE_PEAK_ENABLE", "True Peak", false));  // OFF by default

    // === CHANNEL MANAGEMENT ===
    // Channel Mode: 0 = Stereo (L/R), 1 = Mid-Side (M/S)
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "CHANNEL_MODE", "Channel Mode",
        juce::StringArray{"Stereo", "Mid-Side"},
        0));  // Default to Stereo

    // Channel Link: 0% = Dual Mono (independent), 100% = Fully Linked
    // Controls how much the dynamics detection is shared between channels
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "CHANNEL_LINK", "Channel Link",
        juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 100.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(static_cast<int>(value)) + " %"; }));

    // Auto-Gain Compensation: Match output loudness to input for honest A/B
    layout.add(std::make_unique<juce::AudioParameterBool>(
        "AGC_ENABLE", "Auto-Gain Compensation", false));

    // Processing Mode: 0 = Zero Latency, 1 = Balanced (Halfband), 2 = Linear Phase
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "PROCESSING_MODE", "Processing Engine",
        juce::StringArray{"Zero Latency", "Balanced", "Linear Phase"},
        0));  // Default to Zero Latency

    return layout;
}

//==============================================================================
void QuadBlendDriveAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    // Get current processing mode
    int processingMode = static_cast<int>(apvts.getRawParameterValue("PROCESSING_MODE")->load());

    // ===== ARCHITECTURE A: Global Oversampling Manager =====
    // Prepare oversampling for entire signal chain
    // Mode 0: No OS (multiplier = 1)
    // Mode 1: 8× OS (multiplier = 8)
    // Mode 2: 16× OS (multiplier = 16)
    osManager.prepare(sampleRate, samplesPerBlock, processingMode);

    // Get OS sample rate for all processor calculations
    const double osSampleRate = osManager.getOsSampleRate();
    const int osMultiplier = osManager.getOsMultiplier();

    // Calculate lookahead time based on processing mode (hardcoded per mode)
    // Mode 0 (Zero Latency): 0ms - truly instant, no lookahead
    // Mode 1 (Balanced): 1ms - fast with good quality
    // Mode 2 (Linear Phase): 3ms - maximum quality
    double xyProcessorLookaheadMs = 3.0;  // Default
    if (processingMode == 0)
        xyProcessorLookaheadMs = 0.0;  // Zero Latency = 0ms lookahead
    else if (processingMode == 1)
        xyProcessorLookaheadMs = 1.0;  // Balanced = 1ms lookahead
    else
        xyProcessorLookaheadMs = 3.0;  // Linear Phase = 3ms lookahead

    constexpr double tplLookaheadMs = 2.0;  // TPL lookahead (separate from XY processors)

    // Calculate MAXIMUM lookahead samples (for buffer allocation to support dynamic mode switching)
    // Use 3ms (Standard mode) at 16x OS rate (Linear Phase mode) at max sample rate (192kHz)
    // IMPORTANT: Use hardcoded 192kHz max rate to ensure buffers can handle any sample rate change
    const int maxOsMultiplier = 16;  // Linear Phase mode
    constexpr double maxSystemSampleRate = 192000.0;  // Maximum supported sample rate
    const double maxOsSampleRate = maxSystemSampleRate * maxOsMultiplier;
    const double maxLookaheadMs = 3.0;  // Standard mode
    const int maxLookaheadSamples = static_cast<int>(std::ceil(maxOsSampleRate * maxLookaheadMs / 1000.0));

    // Calculate CURRENT lookahead samples at current OS rate (used for actual processing)
    lookaheadSamples = static_cast<int>(std::ceil(osSampleRate * xyProcessorLookaheadMs / 1000.0));
    advancedTPLLookaheadSamples = static_cast<int>(std::ceil(osSampleRate * tplLookaheadMs / 1000.0));

    // Maximum TPL lookahead samples for buffer allocation (handles sample rate changes)
    const int maxTPLLookaheadSamples = static_cast<int>(std::ceil(maxOsSampleRate * tplLookaheadMs / 1000.0));

    // Calculate plugin latency reported to host
    // Latency = OS filter latency + XY processor lookahead (converted back to base rate)
    // NOTE: Mode 0 (Zero Latency) skips lookahead entirely, so only report lookahead in Modes 1 & 2
    const int osFilterLatency = osManager.getLatencySamples();  // At base rate
    const int xyLookaheadBaseSamples = (processingMode == 0) ? 0 :
        static_cast<int>(std::ceil(sampleRate * xyProcessorLookaheadMs / 1000.0));
    totalLatencySamples = osFilterLatency + xyLookaheadBaseSamples;

    // Report latency to host so DAW can compensate all tracks automatically
    setLatencySamples(totalLatencySamples);

    for (int ch = 0; ch < 2; ++ch)
    {
        auto& state = advancedTPLState[ch];

        // Initialize lookahead buffer (use max size to handle sample rate changes)
        state.lookaheadBuffer.resize(maxTPLLookaheadSamples, 0.0);
        state.lookaheadWritePos = 0;
        state.grEnvelope = 0.0;
        state.lowBandEnv = 0.0;
        state.midBandEnv = 0.0;
        state.highBandEnv = 0.0;
        state.currentReleaseCoeff = 0.001;  // Default release

        // Initialize OSM Mode 0 compensation delay buffer (use max size for safety)
        osmCompensationState[ch].delayBuffer.resize(maxTPLLookaheadSamples, 0.0);
        osmCompensationState[ch].writePos = 0;

        // Initialize dry delay buffer for parallel processing (OS rate)
        // v1.7.6: Delay applied in OS domain for 4-channel sync processing
        // Use maximum OS rate lookahead samples for buffer sizing
        dryDelayState[ch].delayBuffer.resize(maxLookaheadSamples, 0.0);
        dryDelayState[ch].writePos = 0;

        // Design multiband filters for IRC at OS rate
        const double pi = juce::MathConstants<double>::pi;
        const double fs = (osSampleRate > 0.0) ? osSampleRate : 44100.0;  // Safe default if somehow 0

        // Low-pass filter: <200 Hz (2nd order Butterworth)
        {
            const double fc = 200.0;
            const double omega = 2.0 * pi * fc / fs;
            const double cosw = std::cos(omega);
            const double sinw = std::sin(omega);
            const double alpha = sinw / (2.0 * 0.707);  // Q = 0.707 (Butterworth)

            state.lowB0 = (1.0 - cosw) / 2.0;
            state.lowB1 = 1.0 - cosw;
            state.lowB2 = (1.0 - cosw) / 2.0;
            const double a0 = 1.0 + alpha;
            state.lowA1 = -2.0 * cosw / a0;
            state.lowA2 = (1.0 - alpha) / a0;
            state.lowB0 /= a0;
            state.lowB1 /= a0;
            state.lowB2 /= a0;
        }

        // Band-pass filter: 200-4000 Hz (2nd order)
        {
            const double fc = 1100.0;  // Center frequency
            const double Q = 0.5;      // Width
            const double omega = 2.0 * pi * fc / fs;
            const double cosw = std::cos(omega);
            const double sinw = std::sin(omega);
            const double alpha = sinw / (2.0 * Q);

            state.midB0 = alpha;
            state.midB1 = 0.0;
            state.midB2 = -alpha;
            const double a0 = 1.0 + alpha;
            state.midA1 = -2.0 * cosw / a0;
            state.midA2 = (1.0 - alpha) / a0;
            state.midB0 /= a0;
            state.midB1 /= a0;
            state.midB2 /= a0;
        }

        // High-pass filter: >4000 Hz (2nd order Butterworth)
        {
            const double fc = 4000.0;
            const double omega = 2.0 * pi * fc / fs;
            const double cosw = std::cos(omega);
            const double sinw = std::sin(omega);
            const double alpha = sinw / (2.0 * 0.707);  // Q = 0.707

            state.highB0 = (1.0 + cosw) / 2.0;
            state.highB1 = -(1.0 + cosw);
            state.highB2 = (1.0 + cosw) / 2.0;
            const double a0 = 1.0 + alpha;
            state.highA1 = -2.0 * cosw / a0;
            state.highA2 = (1.0 - alpha) / a0;
            state.highB0 /= a0;
            state.highB1 /= a0;
            state.highB2 /= a0;
        }
    }

    // Calculate oscilloscope buffer size (3000ms = 3 seconds)
    int newOscilloscopeSize = static_cast<int>(std::ceil(sampleRate * 3.0));
    oscilloscopeSize.store(newOscilloscopeSize);

    // Resize oscilloscope buffers (main + three bands for RGB)
    oscilloscopeBuffer.clear();
    oscilloscopeBuffer.resize(newOscilloscopeSize, 0.0f);
    oscilloscopeLowBand.clear();
    oscilloscopeLowBand.resize(newOscilloscopeSize, 0.0f);
    oscilloscopeMidBand.clear();
    oscilloscopeMidBand.resize(newOscilloscopeSize, 0.0f);
    oscilloscopeHighBand.clear();
    oscilloscopeHighBand.resize(newOscilloscopeSize, 0.0f);
    oscilloscopeGR.clear();
    oscilloscopeGR.resize(newOscilloscopeSize, 0.0f);

    oscilloscopeWritePos.store(0);

    // Initialize three-band filters for oscilloscope RGB visualization
    // Low-pass: 2nd order Butterworth at 250 Hz
    // Mid band-pass: 250 Hz - 4 kHz
    // High-pass: 2nd order Butterworth at 4 kHz
    const double pi = juce::MathConstants<double>::pi;
    const double safeSampleRate = (sampleRate > 0.0) ? sampleRate : 44100.0;  // Safe default

    for (int ch = 0; ch < 2; ++ch)
    {
        // Low-pass filter at 250 Hz
        double lowFreq = 250.0;
        double lowOmega = 2.0 * pi * lowFreq / safeSampleRate;
        double lowAlpha = std::sin(lowOmega) / (2.0 * 0.707);  // Q = 0.707 (Butterworth)

        oscFilters[ch].lowB0 = (1.0 - std::cos(lowOmega)) / 2.0;
        oscFilters[ch].lowB1 = 1.0 - std::cos(lowOmega);
        oscFilters[ch].lowB2 = (1.0 - std::cos(lowOmega)) / 2.0;
        double a0 = 1.0 + lowAlpha;
        oscFilters[ch].lowA1 = -2.0 * std::cos(lowOmega) / a0;
        oscFilters[ch].lowA2 = (1.0 - lowAlpha) / a0;
        oscFilters[ch].lowB0 /= a0;
        oscFilters[ch].lowB1 /= a0;
        oscFilters[ch].lowB2 /= a0;

        // Band-pass filter: 250 Hz - 4 kHz (center at ~1 kHz)
        double midFreq = 1000.0;
        double midOmega = 2.0 * pi * midFreq / safeSampleRate;
        double bandwidth = 2.0;  // Wide bandwidth for 250-4000 Hz range
        double midAlpha = std::sin(midOmega) * std::sinh(std::log(2.0) / 2.0 * bandwidth * midOmega / std::sin(midOmega));

        oscFilters[ch].midB0 = midAlpha;
        oscFilters[ch].midB1 = 0.0;
        oscFilters[ch].midB2 = -midAlpha;
        a0 = 1.0 + midAlpha;
        oscFilters[ch].midA1 = -2.0 * std::cos(midOmega) / a0;
        oscFilters[ch].midA2 = (1.0 - midAlpha) / a0;
        oscFilters[ch].midB0 /= a0;
        oscFilters[ch].midB1 /= a0;
        oscFilters[ch].midB2 /= a0;

        // High-pass filter at 4 kHz
        double highFreq = 4000.0;
        double highOmega = 2.0 * pi * highFreq / safeSampleRate;
        double highAlpha = std::sin(highOmega) / (2.0 * 0.707);  // Q = 0.707 (Butterworth)

        oscFilters[ch].highB0 = (1.0 + std::cos(highOmega)) / 2.0;
        oscFilters[ch].highB1 = -(1.0 + std::cos(highOmega));
        oscFilters[ch].highB2 = (1.0 + std::cos(highOmega)) / 2.0;
        a0 = 1.0 + highAlpha;
        oscFilters[ch].highA1 = -2.0 * std::cos(highOmega) / a0;
        oscFilters[ch].highA2 = (1.0 - highAlpha) / a0;
        oscFilters[ch].highB0 /= a0;
        oscFilters[ch].highB1 /= a0;
        oscFilters[ch].highB2 /= a0;

        // Reset filter states
        oscFilters[ch].lowZ1 = oscFilters[ch].lowZ2 = 0.0;
        oscFilters[ch].midZ1 = oscFilters[ch].midZ2 = 0.0;
        oscFilters[ch].highZ1 = oscFilters[ch].highZ2 = 0.0;
    }

    // Allocate float buffers
    dryBufferFloat.setSize(2, samplesPerBlock);
    tempBuffer1Float.setSize(2, samplesPerBlock);
    tempBuffer2Float.setSize(2, samplesPerBlock);
    tempBuffer3Float.setSize(2, samplesPerBlock);
    tempBuffer4Float.setSize(2, samplesPerBlock);
    combined4ChFloat.setSize(4, samplesPerBlock);  // 4-channel for phase-coherent processing
    protectionDeltaCombined4ChFloat.setSize(4, samplesPerBlock);  // Pre-allocated for overshoot delta mode

    // Allocate layer buffers for per-sample per-processor visualization (float)
    layerInputFloat.setSize(2, samplesPerBlock);
    layerHardClipFloat.setSize(2, samplesPerBlock);
    layerSoftClipFloat.setSize(2, samplesPerBlock);
    layerSlowLimitFloat.setSize(2, samplesPerBlock);
    layerFastLimitFloat.setSize(2, samplesPerBlock);
    layerFinalOutputFloat.setSize(2, samplesPerBlock);

    // Allocate double buffers
    dryBufferDouble.setSize(2, samplesPerBlock);
    tempBuffer1Double.setSize(2, samplesPerBlock);
    tempBuffer2Double.setSize(2, samplesPerBlock);
    tempBuffer3Double.setSize(2, samplesPerBlock);
    tempBuffer4Double.setSize(2, samplesPerBlock);
    combined4ChDouble.setSize(4, samplesPerBlock);  // 4-channel for phase-coherent processing
    protectionDeltaCombined4ChDouble.setSize(4, samplesPerBlock);  // Pre-allocated for overshoot delta mode

    // Allocate layer buffers for per-sample per-processor visualization (double)
    layerInputDouble.setSize(2, samplesPerBlock);
    layerHardClipDouble.setSize(2, samplesPerBlock);
    layerSoftClipDouble.setSize(2, samplesPerBlock);
    layerSlowLimitDouble.setSize(2, samplesPerBlock);
    layerFastLimitDouble.setSize(2, samplesPerBlock);
    layerFinalOutputDouble.setSize(2, samplesPerBlock);

    // Initialize ALL 4-channel oversamplers for phase-coherent dry/wet processing
    // Pre-allocate for all modes to allow hot-swapping without audio thread allocation
    // CRITICAL: Must use IDENTICAL filter settings as osManager for phase coherence!

    // Balanced mode (8x OS) - MUST match osManager Mode 1 settings
    oversampling4ChBalancedFloat = std::make_unique<juce::dsp::Oversampling<float>>(
        4, 3, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, false, false);
    oversampling4ChBalancedDouble = std::make_unique<juce::dsp::Oversampling<double>>(
        4, 3, juce::dsp::Oversampling<double>::filterHalfBandFIREquiripple, false, false);
    oversampling4ChBalancedFloat->initProcessing(static_cast<size_t>(samplesPerBlock));
    oversampling4ChBalancedDouble->initProcessing(static_cast<size_t>(samplesPerBlock));

    // Linear Phase mode (16x OS) - MUST match osManager Mode 2 settings
    oversampling4ChLinearFloat = std::make_unique<juce::dsp::Oversampling<float>>(
        4, 4, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true);
    oversampling4ChLinearDouble = std::make_unique<juce::dsp::Oversampling<double>>(
        4, 4, juce::dsp::Oversampling<double>::filterHalfBandFIREquiripple, true, true);
    oversampling4ChLinearFloat->initProcessing(static_cast<size_t>(samplesPerBlock));
    oversampling4ChLinearDouble->initProcessing(static_cast<size_t>(samplesPerBlock));

    // Delta mode (8x OS) - For overshoot suppression delta mode in Balanced mode
    // Pre-allocate to avoid audio thread allocation when delta mode is enabled
    // Uses 4 channels: [mainL, mainR, refL, refR] for phase-coherent processing
    oversampling4ChDeltaFloat = std::make_unique<juce::dsp::Oversampling<float>>(
        4, 3, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, false);
    oversampling4ChDeltaDouble = std::make_unique<juce::dsp::Oversampling<double>>(
        4, 3, juce::dsp::Oversampling<double>::filterHalfBandFIREquiripple, true, false);
    oversampling4ChDeltaFloat->initProcessing(static_cast<size_t>(samplesPerBlock));
    oversampling4ChDeltaDouble->initProcessing(static_cast<size_t>(samplesPerBlock));

    // Delta mode (16x OS) - For overshoot suppression delta mode in Linear Phase mode
    oversampling4ChDelta16xFloat = std::make_unique<juce::dsp::Oversampling<float>>(
        4, 4, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true);
    oversampling4ChDelta16xDouble = std::make_unique<juce::dsp::Oversampling<double>>(
        4, 4, juce::dsp::Oversampling<double>::filterHalfBandFIREquiripple, true, true);
    oversampling4ChDelta16xFloat->initProcessing(static_cast<size_t>(samplesPerBlock));
    oversampling4ChDelta16xDouble->initProcessing(static_cast<size_t>(samplesPerBlock));

    // 2-channel oversamplers for protection limiters (overshoot suppression, advanced TPL)
    // Balanced mode (8x OS) - matches osManager Mode 1
    oversampling2ChBalancedFloat = std::make_unique<juce::dsp::Oversampling<float>>(
        2, 3, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, false, false);
    oversampling2ChBalancedDouble = std::make_unique<juce::dsp::Oversampling<double>>(
        2, 3, juce::dsp::Oversampling<double>::filterHalfBandFIREquiripple, false, false);
    oversampling2ChBalancedFloat->initProcessing(static_cast<size_t>(samplesPerBlock));
    oversampling2ChBalancedDouble->initProcessing(static_cast<size_t>(samplesPerBlock));

    // Linear Phase mode (16x OS) - matches osManager Mode 2
    oversampling2ChLinearFloat = std::make_unique<juce::dsp::Oversampling<float>>(
        2, 4, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true);
    oversampling2ChLinearDouble = std::make_unique<juce::dsp::Oversampling<double>>(
        2, 4, juce::dsp::Oversampling<double>::filterHalfBandFIREquiripple, true, true);
    oversampling2ChLinearFloat->initProcessing(static_cast<size_t>(samplesPerBlock));
    oversampling2ChLinearDouble->initProcessing(static_cast<size_t>(samplesPerBlock));

    // Initialize parameter smoothers (20ms ramp time to prevent zipper noise)
    const double rampTimeSeconds = 0.020;  // 20ms
    smoothedInputGain.reset(sampleRate, rampTimeSeconds);
    smoothedOutputGain.reset(sampleRate, rampTimeSeconds);
    smoothedMixWet.reset(sampleRate, rampTimeSeconds);
    smoothedAgcGain.reset(sampleRate, 0.1);  // 100ms for smoother AGC transitions
    smoothedAgcGain.setCurrentAndTargetValue(1.0f);
    agcInputRMS.store(0.0f);
    agcOutputRMS.store(0.0f);

    // Initialize per-processor envelope followers for smooth GR visualization
    for (int ch = 0; ch < 2; ++ch)
        processorEnvelope[ch].prepare(sampleRate);

    // Set initial values from current parameter state (not arbitrary defaults)
    float inputGainDB = apvts.getRawParameterValue("INPUT_GAIN")->load();
    float outputGainDB = apvts.getRawParameterValue("OUTPUT_GAIN")->load();
    float currentInputGain = juce::Decibels::decibelsToGain(inputGainDB);
    float currentOutputGain = juce::Decibels::decibelsToGain(outputGainDB);
    float currentMix = apvts.getRawParameterValue("MIX_WET")->load() / 100.0f;

    // Quantize gains to exactly 1.0 when near 0 dB (unity gain)
    constexpr float unityThresholdDB = 0.05f;  // ±0.05 dB
    if (std::abs(inputGainDB) < unityThresholdDB)
        currentInputGain = 1.0f;
    if (std::abs(outputGainDB) < unityThresholdDB)
        currentOutputGain = 1.0f;

    // Quantize mix at extremes (ensures smoothing can reach exact 0/1)
    if (currentMix < 0.005f)
        currentMix = 0.0f;
    else if (currentMix > 0.995f)
        currentMix = 1.0f;

    smoothedInputGain.setCurrentAndTargetValue(currentInputGain);
    smoothedOutputGain.setCurrentAndTargetValue(currentOutputGain);
    smoothedMixWet.setCurrentAndTargetValue(currentMix);

    // === ENVELOPE SHAPERS INITIALIZATION ===
    // Prepare envelope shapers at oversampled rate for accurate transient detection
    // Note: Envelope detection runs at OS rate, matching drive processor rate
    hardClipShaper.prepare(osSampleRate);
    softClipShaper.prepare(osSampleRate);
    slowLimitShaper.prepare(osSampleRate);
    fastLimitShaper.prepare(osSampleRate);

    // Reset all processor states and allocate lookahead buffers (3ms for all)
    for (int ch = 0; ch < 2; ++ch)
    {
        // Architecture A: Dry delay buffers NO LONGER NEEDED
        // Modes 0: Both dry/wet bypass OS (no phase mismatch)
        // Modes 1/2: Both dry/wet pass through 4-channel oversampler (perfect phase alignment)
        dryDelayState[ch].delayBuffer.clear();
        dryDelayState[ch].writePos = 0;

        // Hard Clip lookahead (allocate at MAX size for dynamic switching)
        hardClipState[ch].lookaheadBuffer.resize(maxLookaheadSamples, 0.0);
        hardClipState[ch].lookaheadWritePos = 0;

        // Soft Clip lookahead (allocate at MAX size for dynamic switching)
        softClipState[ch].lookaheadBuffer.resize(maxLookaheadSamples, 0.0);
        softClipState[ch].lookaheadWritePos = 0;

        // Slow Limiter
        slowLimiterState[ch].envelope = 0.0;
        slowLimiterState[ch].adaptiveRelease = 100.0;
        slowLimiterState[ch].lookaheadBuffer.resize(maxLookaheadSamples, 0.0);
        slowLimiterState[ch].lookaheadWritePos = 0;

        // Fast Limiter
        fastLimiterState[ch].envelope = 0.0;
        fastLimiterState[ch].lookaheadBuffer.resize(maxLookaheadSamples, 0.0);
        fastLimiterState[ch].lookaheadWritePos = 0;

        // Protection Limiter (4ms lookahead, true-peak safety)
        protectionLimiterState[ch].envelope = 0.0;
        protectionLimiterState[ch].fastReleaseEnv = 0.0;
        protectionLimiterState[ch].slowReleaseEnv = 0.0;
        protectionLimiterState[ch].lookaheadBuffer.resize(protectionLookaheadSamples, 0.0);
        protectionLimiterState[ch].lookaheadWritePos = 0;
        // Allocate oversampled buffer (8x oversampling)
        protectionLimiterState[ch].oversampledBuffer.resize(samplesPerBlock * 8, 0.0);
    }
}

void QuadBlendDriveAudioProcessor::releaseResources()
{
    // Reset main oversampling manager
    osManager.reset();

    // Reset 4-channel dry/wet oversamplers
    if (oversampling4ChBalancedFloat) oversampling4ChBalancedFloat->reset();
    if (oversampling4ChBalancedDouble) oversampling4ChBalancedDouble->reset();
    if (oversampling4ChLinearFloat) oversampling4ChLinearFloat->reset();
    if (oversampling4ChLinearDouble) oversampling4ChLinearDouble->reset();

    // Reset delta mode oversamplers
    if (oversampling4ChDeltaFloat) oversampling4ChDeltaFloat->reset();
    if (oversampling4ChDeltaDouble) oversampling4ChDeltaDouble->reset();
    if (oversampling4ChDelta16xFloat) oversampling4ChDelta16xFloat->reset();
    if (oversampling4ChDelta16xDouble) oversampling4ChDelta16xDouble->reset();

    // Reset protection stage oversamplers
    if (oversampling2ChBalancedFloat) oversampling2ChBalancedFloat->reset();
    if (oversampling2ChBalancedDouble) oversampling2ChBalancedDouble->reset();
    if (oversampling2ChLinearFloat) oversampling2ChLinearFloat->reset();
    if (oversampling2ChLinearDouble) oversampling2ChLinearDouble->reset();
}

//==============================================================================
// Normalization Control Functions
void QuadBlendDriveAudioProcessor::startAnalysis()
{
    analyzingEnabled = true;
    isAnalyzing.store(true);
    peakInputLevel = 0.0;
}

void QuadBlendDriveAudioProcessor::stopAnalysis()
{
    analyzingEnabled = false;
    isAnalyzing.store(false);
    calculateNormalizationGain();
}

void QuadBlendDriveAudioProcessor::enableNormalization()
{
    normalizationEnabled = true;
    isNormalizing.store(true);
}

void QuadBlendDriveAudioProcessor::disableNormalization()
{
    normalizationEnabled = false;
    isNormalizing.store(false);
}

void QuadBlendDriveAudioProcessor::resetNormalizationData()
{
    peakInputLevel = 0.0;
    computedNormalizationGain = 1.0;
    currentPeakDB.store(-60.0);
    normalizationGainDB.store(0.0);
    analyzingEnabled = false;
    normalizationEnabled = false;
    isAnalyzing.store(false);
    isNormalizing.store(false);
}

//==============================================================================
// TRUE PEAK MEASUREMENT (ITU-R BS.1770-4 compliant)
// Uses 4x oversampling with sinc interpolation to detect inter-sample peaks
template<typename SampleType>
float QuadBlendDriveAudioProcessor::measureTruePeak(const SampleType* channelData, int numSamples)
{
    // ITU-R BS.1770-4 true peak measurement using 4x oversampling
    // Coefficients from libebur128 (reference implementation)
    // 48-tap polyphase FIR filter for accurate inter-sample peak detection

    // Coefficient table for 4x oversampling (3 inter-sample positions)
    static const float coefficients[3][12] = {
        // Position 1/4 between samples
        {
            0.0017089843750f, -0.0109863281250f, -0.0196533203125f, 0.0332031250000f,
            0.1373291015625f, 0.4650878906250f, 0.7797851562500f, 0.2003173828125f,
            -0.0582275390625f, 0.0330810546875f, -0.0154418945313f, 0.0048828125000f
        },
        // Position 2/4 (midpoint) between samples
        {
            -0.0291748046875f, 0.0292968750000f, -0.0517578125000f, 0.0891113281250f,
            -0.1665039062500f, 0.6003417968750f, 0.6003417968750f, -0.1665039062500f,
            0.0891113281250f, -0.0517578125000f, 0.0292968750000f, -0.0291748046875f
        },
        // Position 3/4 between samples
        {
            0.0048828125000f, -0.0154418945313f, 0.0330810546875f, -0.0582275390625f,
            0.2003173828125f, 0.7797851562500f, 0.4650878906250f, 0.1373291015625f,
            0.0332031250000f, -0.0196533203125f, -0.0109863281250f, 0.0017089843750f
        }
    };

    float truePeak = 0.0f;

    // Need at least 12 samples for the filter
    if (numSamples < 12)
    {
        for (int i = 0; i < numSamples; ++i)
            truePeak = std::max(truePeak, std::abs(static_cast<float>(channelData[i])));
        return truePeak;
    }

    // Process each sample position
    for (int i = 0; i < numSamples - 1; ++i)
    {
        // Check the current sample peak
        truePeak = std::max(truePeak, std::abs(static_cast<float>(channelData[i])));

        // Calculate the 3 inter-sample points between sample i and i+1
        // We need 12 samples centered on this interpolation region
        // The filter is centered between the two samples

        // Determine the range we can safely interpolate
        int startSample = i - 5;
        int endSample = i + 6;

        if (startSample < 0 || endSample >= numSamples)
            continue;  // Skip edge regions where we don't have enough samples

        // Extract 12 samples for the filter
        float samples[12];
        for (int j = 0; j < 12; ++j)
            samples[j] = static_cast<float>(channelData[startSample + j]);

        // Apply the 3 interpolation filters
        for (int phase = 0; phase < 3; ++phase)
        {
            float interpolated = 0.0f;
            for (int j = 0; j < 12; ++j)
                interpolated += samples[j] * coefficients[phase][j];

            truePeak = std::max(truePeak, std::abs(interpolated));
        }
    }

    // Check final sample
    if (numSamples > 0)
        truePeak = std::max(truePeak, std::abs(static_cast<float>(channelData[numSamples - 1])));

    return truePeak;
}

// Explicit template instantiations
template float QuadBlendDriveAudioProcessor::measureTruePeak<float>(const float*, int);
template float QuadBlendDriveAudioProcessor::measureTruePeak<double>(const double*, int);

void QuadBlendDriveAudioProcessor::calculateNormalizationGain()
{
    calibrationTargetDB = static_cast<double>(apvts.getRawParameterValue("CALIB_LEVEL")->load());

    // Guard against log of zero/negative - use minimum threshold of -120dB (1e-6)
    constexpr double minLevel = 1e-6;  // -120dB
    if (peakInputLevel > minLevel)
    {
        double peakDB = 20.0 * std::log10(peakInputLevel);
        // Clamp to valid range to prevent NaN propagation
        peakDB = juce::jlimit(-120.0, 24.0, peakDB);
        currentPeakDB.store(peakDB);

        // Calculate gain needed to bring peak to calibration target
        double gainNeededDB = calibrationTargetDB - peakDB;
        computedNormalizationGain = std::pow(10.0, gainNeededDB / 20.0);
        normalizationGainDB.store(gainNeededDB);
    }
    else
    {
        currentPeakDB.store(-120.0);
        computedNormalizationGain = 1.0;
        normalizationGainDB.store(0.0);
    }
}

void QuadBlendDriveAudioProcessor::updateDecimatedDisplay()
{
    // Decimate high-resolution displayBuffer (8192 samples) to GUI-friendly cache (512 segments)
    // Uses min/max envelope for accurate peak representation at 60fps
    // Called from GUI thread - thread-safe read of ring buffer

    const int currentWritePos = displayWritePos.load();
    constexpr int samplesPerSegment = displayBufferSize / decimatedDisplaySize;  // 8192 / 512 = 16

    // Read from ring buffer starting from oldest sample (write position is next write location)
    int readPos = currentWritePos;

    for (int segment = 0; segment < decimatedDisplaySize; ++segment)
    {
        DecimatedSegment& seg = decimatedDisplay[segment];

        // Initialize min/max for this segment
        float waveformMin = 0.0f;
        float waveformMax = 0.0f;
        float waveformMinL = 0.0f, waveformMaxL = 0.0f;
        float waveformMinR = 0.0f, waveformMaxR = 0.0f;
        float grMin = 0.0f;
        float grMax = 0.0f;
        float sumLow = 0.0f;
        float sumMid = 0.0f;
        float sumHigh = 0.0f;

        // === DRIVE VISUALIZER LAYER MIN/MAX ===
        float inputMin = 0.0f, inputMax = 0.0f;
        float hardClipMin = 0.0f, hardClipMax = 0.0f;
        float softClipMin = 0.0f, softClipMax = 0.0f;
        float slowLimitMin = 0.0f, slowLimitMax = 0.0f;
        float fastLimitMin = 0.0f, fastLimitMax = 0.0f;
        float finalOutputMin = 0.0f, finalOutputMax = 0.0f;

        // === THRESHOLD METER: GAIN REDUCTION MIN/MAX ===
        float hardClipGRMin = 0.0f, hardClipGRMax = 0.0f;
        float softClipGRMin = 0.0f, softClipGRMax = 0.0f;
        float slowLimitGRMin = 0.0f, slowLimitGRMax = 0.0f;
        float fastLimitGRMin = 0.0f, fastLimitGRMax = 0.0f;

        // Process all samples in this segment
        for (int i = 0; i < samplesPerSegment; ++i)
        {
            const DisplaySample& sample = displayBuffer[readPos];

            // Use mono mix for waveform envelope (L+R)/2
            float waveform = (sample.waveformL + sample.waveformR) * 0.5f;

            // Update min/max for waveform envelope
            if (i == 0)
            {
                waveformMin = waveform;
                waveformMax = waveform;
                waveformMinL = waveformMaxL = sample.waveformL;
                waveformMinR = waveformMaxR = sample.waveformR;
                grMin = sample.gainReduction;
                grMax = sample.gainReduction;

                // Initialize per-processor output min/max (raw outputs, not blend-weighted)
                inputMin = inputMax = sample.inputSignal;
                hardClipMin = hardClipMax = sample.hardClipOutput;
                softClipMin = softClipMax = sample.softClipOutput;
                slowLimitMin = slowLimitMax = sample.slowLimitOutput;
                fastLimitMin = fastLimitMax = sample.fastLimitOutput;
                finalOutputMin = finalOutputMax = sample.finalOutput;

                // Initialize gain reduction min/max
                hardClipGRMin = hardClipGRMax = sample.hardClipGainReduction;
                softClipGRMin = softClipGRMax = sample.softClipGainReduction;
                slowLimitGRMin = slowLimitGRMax = sample.slowLimitGainReduction;
                fastLimitGRMin = fastLimitGRMax = sample.fastLimitGainReduction;
            }
            else
            {
                waveformMin = std::min(waveformMin, waveform);
                waveformMax = std::max(waveformMax, waveform);
                waveformMinL = std::min(waveformMinL, sample.waveformL);
                waveformMaxL = std::max(waveformMaxL, sample.waveformL);
                waveformMinR = std::min(waveformMinR, sample.waveformR);
                waveformMaxR = std::max(waveformMaxR, sample.waveformR);
                grMin = std::min(grMin, sample.gainReduction);
                grMax = std::max(grMax, sample.gainReduction);

                // Update per-processor output min/max (raw outputs, not blend-weighted)
                inputMin = std::min(inputMin, sample.inputSignal);
                inputMax = std::max(inputMax, sample.inputSignal);
                hardClipMin = std::min(hardClipMin, sample.hardClipOutput);
                hardClipMax = std::max(hardClipMax, sample.hardClipOutput);
                softClipMin = std::min(softClipMin, sample.softClipOutput);
                softClipMax = std::max(softClipMax, sample.softClipOutput);
                slowLimitMin = std::min(slowLimitMin, sample.slowLimitOutput);
                slowLimitMax = std::max(slowLimitMax, sample.slowLimitOutput);
                fastLimitMin = std::min(fastLimitMin, sample.fastLimitOutput);
                fastLimitMax = std::max(fastLimitMax, sample.fastLimitOutput);
                finalOutputMin = std::min(finalOutputMin, sample.finalOutput);
                finalOutputMax = std::max(finalOutputMax, sample.finalOutput);

                // Update gain reduction min/max
                hardClipGRMin = std::min(hardClipGRMin, sample.hardClipGainReduction);
                hardClipGRMax = std::max(hardClipGRMax, sample.hardClipGainReduction);
                softClipGRMin = std::min(softClipGRMin, sample.softClipGainReduction);
                softClipGRMax = std::max(softClipGRMax, sample.softClipGainReduction);
                slowLimitGRMin = std::min(slowLimitGRMin, sample.slowLimitGainReduction);
                slowLimitGRMax = std::max(slowLimitGRMax, sample.slowLimitGainReduction);
                fastLimitGRMin = std::min(fastLimitGRMin, sample.fastLimitGainReduction);
                fastLimitGRMax = std::max(fastLimitGRMax, sample.fastLimitGainReduction);
            }

            // Accumulate frequency bands for averaging
            sumLow += sample.lowBand;
            sumMid += sample.midBand;
            sumHigh += sample.highBand;

            // Advance read position in ring buffer
            readPos = (readPos + 1) % displayBufferSize;
        }

        // Store decimated segment with min/max envelope
        seg.waveformMin = waveformMin;
        seg.waveformMax = waveformMax;
        seg.waveformMinL = waveformMinL;
        seg.waveformMaxL = waveformMaxL;
        seg.waveformMinR = waveformMinR;
        seg.waveformMaxR = waveformMaxR;
        seg.grMin = grMin;
        seg.grMax = grMax;
        seg.avgLow = sumLow / static_cast<float>(samplesPerSegment);
        seg.avgMid = sumMid / static_cast<float>(samplesPerSegment);
        seg.avgHigh = sumHigh / static_cast<float>(samplesPerSegment);

        // Store layer min/max
        seg.inputMin = inputMin;
        seg.inputMax = inputMax;
        seg.hardClipMin = hardClipMin;
        seg.hardClipMax = hardClipMax;
        seg.softClipMin = softClipMin;
        seg.softClipMax = softClipMax;
        seg.slowLimitMin = slowLimitMin;
        seg.slowLimitMax = slowLimitMax;
        seg.fastLimitMin = fastLimitMin;
        seg.fastLimitMax = fastLimitMax;
        seg.finalOutputMin = finalOutputMin;
        seg.finalOutputMax = finalOutputMax;

        // Store gain reduction min/max
        seg.hardClipGRMin = hardClipGRMin;
        seg.hardClipGRMax = hardClipGRMax;
        seg.softClipGRMin = softClipGRMin;
        seg.softClipGRMax = softClipGRMax;
        seg.slowLimitGRMin = slowLimitGRMin;
        seg.slowLimitGRMax = slowLimitGRMax;
        seg.fastLimitGRMin = fastLimitGRMin;
        seg.fastLimitGRMax = fastLimitGRMax;
    }

    // Signal that decimated display is ready for rendering
    decimatedDisplayReady.store(true);
}

void QuadBlendDriveAudioProcessor::updateTransportState()
{
    // === TRANSPORT STATE DETECTION ===
    // Syncs waveform display with DAW playback state
    bool currentlyPlaying = false;
    int64_t currentSamplePos = 0;

    if (auto* playHead = getPlayHead())
    {
        if (auto posInfo = playHead->getPosition())
        {
            currentlyPlaying = posInfo->getIsPlaying();
            if (auto samples = posInfo->getTimeInSamples())
                currentSamplePos = *samples;
            // Get tempo from host (default to 120 BPM if not available)
            if (auto bpm = posInfo->getBpm())
                currentBPM.store(*bpm);
            // Get PPQ position for beat-synced display
            if (auto ppq = posInfo->getPpqPosition())
                currentPPQPosition.store(*ppq);
        }
    }

    // Detect play state transitions
    bool wasPlayingPrev = wasPlaying.load();

    if (currentlyPlaying && !wasPlayingPrev)
    {
        // === PLAYBACK JUST STARTED ===
        // Clear display buffer for fresh start
        displayWritePos.store(0);

        // Clear the buffer contents
        for (auto& sample : displayBuffer)
        {
            sample.waveformL = 0.0f;
            sample.waveformR = 0.0f;
            sample.gainReduction = 0.0f;
            sample.lowBand = 0.0f;
            sample.midBand = 0.0f;
            sample.highBand = 0.0f;
        }

        displayBufferFrozen.store(false);
    }
    else if (!currentlyPlaying && wasPlayingPrev)
    {
        // === PLAYBACK JUST STOPPED ===
        displayBufferFrozen.store(true);
    }

    isPlaying.store(currentlyPlaying);
    wasPlaying.store(currentlyPlaying);
    playheadSamplePos.store(currentSamplePos);
}

//==============================================================================
// ProcessBlock wrappers
void QuadBlendDriveAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    processBlockInternal(buffer, midiMessages);
}

void QuadBlendDriveAudioProcessor::processBlock(juce::AudioBuffer<double>& buffer, juce::MidiBuffer& midiMessages)
{
    processBlockInternal(buffer, midiMessages);
}

//==============================================================================
// Template processBlock implementation
template<typename SampleType>
void QuadBlendDriveAudioProcessor::processBlockInternal(juce::AudioBuffer<SampleType>& buffer,
                                                         juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int totalNumInputChannels = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();

    // DEBUG: Uncomment this block to test absolute zero-latency bypass
    // If latency persists with this enabled, it's in JUCE/host, not our code
    // #define DEBUG_ZERO_LATENCY_TEST
    #ifdef DEBUG_ZERO_LATENCY_TEST
    {
        const int processingMode = static_cast<int>(apvts.getRawParameterValue("PROCESSING_MODE")->load());
        if (processingMode == 0)
        {
            // Absolute minimum: just clear unused channels and return
            for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
                buffer.clear(i, 0, numSamples);
            return;  // Complete bypass - buffer passes through unchanged
        }
    }
    #endif

    // === TRANSPORT STATE DETECTION ===
    updateTransportState();

    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, numSamples);

    // Bypass check
    const bool bypass = apvts.getRawParameterValue("BYPASS")->load() > 0.5f;

    // If bypass is engaged, capture the dry input signal to oscilloscope and return
    if (bypass)
    {
        // === WAVEFORM DISPLAY DATA CAPTURE (Bypass Mode) ===
        // Apply frequency band filters once and write to both legacy and new display buffers
        // Only update display buffer when transport is playing (not frozen)
        if (!displayBufferFrozen.load())
        {
            int writePos = oscilloscopeWritePos.load();
            int oscSize = oscilloscopeSize.load();
            int displayWrite = displayWritePos.load();

            for (int i = 0; i < numSamples; ++i)
            {
                // Get stereo samples
                float sampleL = 0.0f;
                float sampleR = 0.0f;
                if (buffer.getNumChannels() > 0)
                    sampleL = static_cast<float>(buffer.getReadPointer(0)[i]);
                if (buffer.getNumChannels() > 1)
                    sampleR = static_cast<float>(buffer.getReadPointer(1)[i]);

                // Create mono mix for frequency analysis
                float monoSample = (sampleL + sampleR) * 0.5f;
                double sample = static_cast<double>(monoSample);

                // Apply frequency band filters ONCE (use left channel filter state)
                // Low-pass filter (Red channel <250 Hz)
                double lowOut = oscFilters[0].lowB0 * sample + oscFilters[0].lowZ1;
                oscFilters[0].lowZ1 = oscFilters[0].lowB1 * sample - oscFilters[0].lowA1 * lowOut + oscFilters[0].lowZ2;
                oscFilters[0].lowZ2 = oscFilters[0].lowB2 * sample - oscFilters[0].lowA2 * lowOut;
                float lowBand = static_cast<float>(std::abs(lowOut));

                // Band-pass filter (Green channel 250-4000 Hz)
                double midOut = oscFilters[0].midB0 * sample + oscFilters[0].midZ1;
                oscFilters[0].midZ1 = oscFilters[0].midB1 * sample - oscFilters[0].midA1 * midOut + oscFilters[0].midZ2;
                oscFilters[0].midZ2 = oscFilters[0].midB2 * sample - oscFilters[0].midA2 * midOut;
                float midBand = static_cast<float>(std::abs(midOut));

                // High-pass filter (Blue channel >4000 Hz)
                double highOut = oscFilters[0].highB0 * sample + oscFilters[0].highZ1;
                oscFilters[0].highZ1 = oscFilters[0].highB1 * sample - oscFilters[0].highA1 * highOut + oscFilters[0].highZ2;
                oscFilters[0].highZ2 = oscFilters[0].highB2 * sample - oscFilters[0].highA2 * highOut;
                float highBand = static_cast<float>(std::abs(highOut));

                // Write to legacy oscilloscope buffer (if active)
                if (oscSize > 0 && static_cast<int>(oscilloscopeBuffer.size()) >= oscSize && writePos < oscSize)
                {
                    oscilloscopeBuffer[writePos] = monoSample;
                    oscilloscopeLowBand[writePos] = lowBand;
                    oscilloscopeMidBand[writePos] = midBand;
                    oscilloscopeHighBand[writePos] = highBand;
                }

                // Write to unified display buffer
                DisplaySample displaySample;
                displaySample.waveformL = sampleL;
                displaySample.waveformR = sampleR;
                displaySample.gainReduction = 0.0f;  // No GR in bypass mode
                displaySample.lowBand = lowBand;
                displaySample.midBand = midBand;
                displaySample.highBand = highBand;

                // === WAVEFORM GR METER: No processing in bypass mode ===
                displaySample.inputSignal = monoSample;  // Input = output in bypass
                displaySample.hardClipOutput = monoSample;
                displaySample.softClipOutput = monoSample;
                displaySample.slowLimitOutput = monoSample;
                displaySample.fastLimitOutput = monoSample;
                displaySample.finalOutput = monoSample;

                // No GR in bypass mode
                displaySample.hardClipGainReduction = 0.0f;
                displaySample.softClipGainReduction = 0.0f;
                displaySample.slowLimitGainReduction = 0.0f;
                displaySample.fastLimitGainReduction = 0.0f;

                displayBuffer[displayWrite] = displaySample;

                // Update ring buffer positions
                if (oscSize > 0)
                    writePos = (writePos + 1) % oscSize;
                displayWrite = (displayWrite + 1) % displayBufferSize;
            }

            // Store updated positions
            if (oscSize > 0)
                oscilloscopeWritePos.store(writePos);
            displayWritePos.store(displayWrite);
        }

        // Update output peak meters (bypass mode)
        float peakL = 0.0f;
        float peakR = 0.0f;
        for (int i = 0; i < numSamples; ++i)
        {
            if (buffer.getNumChannels() > 0)
            {
                float absVal = static_cast<float>(std::abs(buffer.getReadPointer(0)[i]));
                if (absVal > peakL) peakL = absVal;
            }
            if (buffer.getNumChannels() > 1)
            {
                float absVal = static_cast<float>(std::abs(buffer.getReadPointer(1)[i]));
                if (absVal > peakR) peakR = absVal;
            }
        }
        currentOutputPeakL.store(peakL);
        currentOutputPeakR.store(peakR);

        // Zero out gain reduction when bypassed
        currentGainReductionDB.store(0.0f);

        // === WAVEFORM GR METER: Zero per-processor data when bypassed ===
        currentInputPeak.store(0.0f);
        currentHardClipPeak.store(0.0f);
        currentSoftClipPeak.store(0.0f);
        currentSlowLimitPeak.store(0.0f);
        currentFastLimitPeak.store(0.0f);
        currentHardClipGR.store(0.0f);
        currentSoftClipGR.store(0.0f);
        currentSlowLimitGR.store(0.0f);
        currentFastLimitGR.store(0.0f);

        return;
    }

    // Get correct buffers for this precision
    auto& dryBuffer = std::is_same<SampleType, float>::value ?
                      reinterpret_cast<juce::AudioBuffer<SampleType>&>(dryBufferFloat) :
                      reinterpret_cast<juce::AudioBuffer<SampleType>&>(dryBufferDouble);
    auto& tempBuffer1 = std::is_same<SampleType, float>::value ?
                        reinterpret_cast<juce::AudioBuffer<SampleType>&>(tempBuffer1Float) :
                        reinterpret_cast<juce::AudioBuffer<SampleType>&>(tempBuffer1Double);
    auto& tempBuffer2 = std::is_same<SampleType, float>::value ?
                        reinterpret_cast<juce::AudioBuffer<SampleType>&>(tempBuffer2Float) :
                        reinterpret_cast<juce::AudioBuffer<SampleType>&>(tempBuffer2Double);
    auto& tempBuffer3 = std::is_same<SampleType, float>::value ?
                        reinterpret_cast<juce::AudioBuffer<SampleType>&>(tempBuffer3Float) :
                        reinterpret_cast<juce::AudioBuffer<SampleType>&>(tempBuffer3Double);
    auto& tempBuffer4 = std::is_same<SampleType, float>::value ?
                        reinterpret_cast<juce::AudioBuffer<SampleType>&>(tempBuffer4Float) :
                        reinterpret_cast<juce::AudioBuffer<SampleType>&>(tempBuffer4Double);

    // Read parameters
    const SampleType xyX = static_cast<SampleType>(apvts.getRawParameterValue("XY_X_PARAM")->load());
    const SampleType xyY = static_cast<SampleType>(apvts.getRawParameterValue("XY_Y_PARAM")->load());

    const SampleType thresholdDB = static_cast<SampleType>(apvts.getRawParameterValue("THRESHOLD")->load());
    const SampleType inputGainDB = static_cast<SampleType>(apvts.getRawParameterValue("INPUT_GAIN")->load());
    const SampleType outputGainDB = static_cast<SampleType>(apvts.getRawParameterValue("OUTPUT_GAIN")->load());
    const SampleType mixWetPercent = static_cast<SampleType>(apvts.getRawParameterValue("MIX_WET")->load());
    const SampleType scKneePercent = static_cast<SampleType>(apvts.getRawParameterValue("SC_KNEE")->load());
    const SampleType limitRelMs = static_cast<SampleType>(apvts.getRawParameterValue("LIMIT_REL")->load());
    const SampleType slAttackMs = static_cast<SampleType>(apvts.getRawParameterValue("SL_LIMIT_ATTACK")->load());
    const SampleType flAttackMs = static_cast<SampleType>(apvts.getRawParameterValue("FL_LIMIT_ATTACK")->load());

    const SampleType hcTrimDB = static_cast<SampleType>(apvts.getRawParameterValue("HC_TRIM")->load());
    const SampleType scTrimDB = static_cast<SampleType>(apvts.getRawParameterValue("SC_TRIM")->load());
    const SampleType slTrimDB = static_cast<SampleType>(apvts.getRawParameterValue("SL_TRIM")->load());
    const SampleType flTrimDB = static_cast<SampleType>(apvts.getRawParameterValue("FL_TRIM")->load());

    // Read mute and solo states
    // Mute takes priority over solo (like an analog console - if it's cut, it's cut)
    const bool hcMuteParam = apvts.getRawParameterValue("HC_MUTE")->load() > 0.5f;
    const bool scMuteParam = apvts.getRawParameterValue("SC_MUTE")->load() > 0.5f;
    const bool slMuteParam = apvts.getRawParameterValue("SL_MUTE")->load() > 0.5f;
    const bool flMuteParam = apvts.getRawParameterValue("FL_MUTE")->load() > 0.5f;

    const bool hcSolo = apvts.getRawParameterValue("HC_SOLO")->load() > 0.5f;
    const bool scSolo = apvts.getRawParameterValue("SC_SOLO")->load() > 0.5f;
    const bool slSolo = apvts.getRawParameterValue("SL_SOLO")->load() > 0.5f;
    const bool flSolo = apvts.getRawParameterValue("FL_SOLO")->load() > 0.5f;

    // Solo logic: if any processor is soloed, mute all non-soloed processors
    // BUT explicit mute always takes priority (mute overrides solo)
    const bool anySolo = hcSolo || scSolo || slSolo || flSolo;
    bool hcMute = hcMuteParam;  // Explicit mute always applies
    bool scMute = scMuteParam;
    bool slMute = slMuteParam;
    bool flMute = flMuteParam;
    if (anySolo)
    {
        // Only mute non-soloed processors that aren't already explicitly muted
        if (!hcSolo && !hcMuteParam) hcMute = true;
        if (!scSolo && !scMuteParam) scMute = true;
        if (!slSolo && !slMuteParam) slMute = true;
        if (!flSolo && !flMuteParam) flMute = true;
    }

    const bool deltaMode = apvts.getRawParameterValue("DELTA_MODE")->load() > 0.5f;
    const bool agcEnabled = apvts.getRawParameterValue("AGC_ENABLE")->load() > 0.5f;

    const SampleType threshold = juce::Decibels::decibelsToGain(thresholdDB);
    SampleType inputGain = juce::Decibels::decibelsToGain(inputGainDB);
    SampleType outputGain = juce::Decibels::decibelsToGain(outputGainDB);
    SampleType mixWet = mixWetPercent / static_cast<SampleType>(100.0);
    const SampleType scKnee = scKneePercent / static_cast<SampleType>(100.0);

    // Quantize gains to exactly 1.0 when near 0 dB (unity gain)
    // Prevents parameter smoothing from asymptotically approaching but never reaching unity
    // This is critical for null tests and bit-perfect passthrough at unity gain
    constexpr SampleType unityThresholdDB = static_cast<SampleType>(0.05);  // ±0.05 dB
    if (std::abs(inputGainDB) < unityThresholdDB)
        inputGain = static_cast<SampleType>(1.0);
    if (std::abs(outputGainDB) < unityThresholdDB)
        outputGain = static_cast<SampleType>(1.0);

    // Quantize mix parameter at extremes (ensures smoothing reaches exact 0/1)
    // Prevents asymptotic approach causing wet signal bleed at "0%" mix
    if (mixWet < static_cast<SampleType>(0.005))  // < 0.5%
        mixWet = static_cast<SampleType>(0.0);
    else if (mixWet > static_cast<SampleType>(0.995))  // > 99.5%
        mixWet = static_cast<SampleType>(1.0);

    // Update parameter smoother targets (smooth over 20ms to prevent zipper noise)
    smoothedInputGain.setTargetValue(static_cast<float>(inputGain));
    smoothedOutputGain.setTargetValue(static_cast<float>(outputGain));
    smoothedMixWet.setTargetValue(static_cast<float>(mixWet));

    const SampleType hcTrimGain = juce::Decibels::decibelsToGain(hcTrimDB);
    const SampleType scTrimGain = juce::Decibels::decibelsToGain(scTrimDB);
    const SampleType slTrimGain = juce::Decibels::decibelsToGain(slTrimDB);
    const SampleType flTrimGain = juce::Decibels::decibelsToGain(flTrimDB);

    // === MODE SWITCHING (LOCK-FREE - safe on audio thread) ===
    // Get current processing mode from parameter (NOT from osManager to detect changes)
    const int processingMode = static_cast<int>(apvts.getRawParameterValue("PROCESSING_MODE")->load());

    // Check if mode changed - update osManager with lock-free atomic operation
    if (processingMode != osManager.getProcessingMode())
    {
        // LOCK-FREE mode switch - NO memory allocation
        osManager.setMode(processingMode);

        // Recalculate lookahead times based on new processing mode
        double xyProcessorLookaheadMs = 3.0;  // Default
        if (processingMode == 0)
            xyProcessorLookaheadMs = 0.0;  // Zero Latency = 0ms lookahead
        else if (processingMode == 1)
            xyProcessorLookaheadMs = 1.0;  // Balanced = 1ms lookahead
        else
            xyProcessorLookaheadMs = 3.0;  // Linear Phase = 3ms lookahead

        // Recalculate lookahead samples at new OS rate
        const double newOsSampleRate = osManager.getOsSampleRate();
        lookaheadSamples = static_cast<int>(std::ceil(newOsSampleRate * xyProcessorLookaheadMs / 1000.0));

        // Update latency reporting for new mode
        const int osFilterLatency = osManager.getLatencySamples();  // At base rate
        const int xyLookaheadBaseSamples = (processingMode == 0) ? 0 :
            static_cast<int>(std::ceil(currentSampleRate * xyProcessorLookaheadMs / 1000.0));
        const int newLatency = osFilterLatency + xyLookaheadBaseSamples;
        setLatencySamples(newLatency);

        // Notify host of latency change
        updateHostDisplay(juce::AudioProcessorListener::ChangeDetails().withLatencyChanged(true));

        // Reset processor states to clear stale data from previous mode
        for (int ch = 0; ch < 2; ++ch)
        {
            // Reset lookahead write positions
            hardClipState[ch].lookaheadWritePos = 0;
            softClipState[ch].lookaheadWritePos = 0;
            slowLimiterState[ch].lookaheadWritePos = 0;
            fastLimiterState[ch].lookaheadWritePos = 0;

            // Clear lookahead buffers (prevent glitches from stale data)
            // NOTE: These are already allocated to max size in prepareToPlay() - no resize needed
            std::fill(hardClipState[ch].lookaheadBuffer.begin(), hardClipState[ch].lookaheadBuffer.end(), 0.0);
            std::fill(softClipState[ch].lookaheadBuffer.begin(), softClipState[ch].lookaheadBuffer.end(), 0.0);
            std::fill(slowLimiterState[ch].lookaheadBuffer.begin(), slowLimiterState[ch].lookaheadBuffer.end(), 0.0);
            std::fill(fastLimiterState[ch].lookaheadBuffer.begin(), fastLimiterState[ch].lookaheadBuffer.end(), 0.0);

            // Reset dry delay buffer write position (already allocated to max size)
            // NO RESIZE - buffer is pre-allocated in prepareToPlay()
            dryDelayState[ch].writePos = 0;
            // Clear only the portion we're using for this mode
            const int clearSize = std::min(lookaheadSamples, static_cast<int>(dryDelayState[ch].delayBuffer.size()));
            std::fill(dryDelayState[ch].delayBuffer.begin(),
                     dryDelayState[ch].delayBuffer.begin() + clearSize, 0.0);
        }
    }

    // === PEAK ANALYSIS (using double precision) ===
    if (analyzingEnabled)
    {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            const auto* data = buffer.getReadPointer(ch);
            for (int i = 0; i < numSamples; ++i)
            {
                double sampleAbs = std::abs(static_cast<double>(data[i]));
                peakInputLevel = std::max(peakInputLevel, sampleAbs);
            }
        }
    }

    // === SIGNAL FLOW (v1.7.4) ===
    // 1. Normalization applied to buffer (both dry and wet get this)
    // 2. Dry tapped (has normalization, NO input gain)
    // 3. Input gain applied to buffer (wet only - for drive)
    // 4. Buffer processed through OS + XY blend (wet path)
    // 5. Inverse gain applied to wet (if master comp enabled - keeps output level constant)
    // 6. Output gain applied to wet
    // 7. Dry/wet mix
    // Result: Adjusting input gain changes drive amount without changing output level (when master comp on)

    // === CAPTURE PRISTINE INPUT (BEFORE normalization) ===
    // Used for dry signal when all processors muted
    auto& pristineInputBuffer = (std::is_same<SampleType, float>::value)
        ? reinterpret_cast<juce::AudioBuffer<SampleType>&>(originalInputBufferFloat)
        : reinterpret_cast<juce::AudioBuffer<SampleType>&>(originalInputBufferDouble);

    pristineInputBuffer.makeCopyOf(buffer);  // Capture BEFORE normalization

    // === INPUT NORMALIZATION ===
    // Apply normalization gain FIRST (before everything else)
    SampleType normGain = static_cast<SampleType>(1.0);
    if (normalizationEnabled)
    {
        normGain = static_cast<SampleType>(computedNormalizationGain);
        buffer.applyGain(normGain);
    }

    // === CHANNEL MODE: M/S ENCODING ===
    // 0 = Stereo (no change), 1 = Mid-Side
    const int channelMode = static_cast<int>(apvts.getRawParameterValue("CHANNEL_MODE")->load());

    if (channelMode == 1 && buffer.getNumChannels() >= 2)  // Mid-Side mode
    {
        // Encode L/R to M/S: Mid = (L+R)/2, Side = (L-R)/2
        auto* left = buffer.getWritePointer(0);
        auto* right = buffer.getWritePointer(1);
        for (int i = 0; i < numSamples; ++i)
        {
            const SampleType l = left[i];
            const SampleType r = right[i];
            left[i] = (l + r) * static_cast<SampleType>(0.5);   // Mid
            right[i] = (l - r) * static_cast<SampleType>(0.5);  // Side
        }
    }

    // === CAPTURE NORMALIZED INPUT (After normalization, BEFORE input gain) ===
    // Used for wet signal when all processors muted, and for normal dry signal
    auto& normalizedInputBuffer = (std::is_same<SampleType, float>::value)
        ? reinterpret_cast<juce::AudioBuffer<SampleType>&>(originalInputBufferDouble)
        : reinterpret_cast<juce::AudioBuffer<SampleType>&>(originalInputBufferFloat);

    normalizedInputBuffer.makeCopyOf(buffer);  // Capture AFTER normalization

    // === INPUT GAIN (Wet path only - drives saturation) ===
    // Apply manual input gain to WET signal only (smoothed per-sample to prevent zipper noise)
    SampleType totalInputGainAccumulated = static_cast<SampleType>(0.0);
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        for (int i = 0; i < numSamples; ++i)
        {
            const SampleType smoothedGain = static_cast<SampleType>(smoothedInputGain.getNextValue());
            data[i] *= smoothedGain;
            if (ch == 0)  // Track only once per sample
                totalInputGainAccumulated += smoothedGain;
        }
    }

    // Store average total input gain for GR calculation
    const SampleType avgSmoothedInputGain = totalInputGainAccumulated / static_cast<SampleType>(numSamples);
    const SampleType totalInputGain = normGain * avgSmoothedInputGain;

    // === STEREO I/O METERS: MEASURE INPUT SAMPLE PEAK ===
    // Use sample peak for consistent metering with DAWs (true peak can read +3-4dB higher)
    float inPkL = 0.0f;
    float inPkR = 0.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        float absL = std::abs(static_cast<float>(buffer.getReadPointer(0)[i]));
        if (absL > inPkL) inPkL = absL;
        if (buffer.getNumChannels() > 1)
        {
            float absR = std::abs(static_cast<float>(buffer.getReadPointer(1)[i]));
            if (absR > inPkR) inPkR = absR;
        }
    }
    if (buffer.getNumChannels() == 1) inPkR = inPkL;

    // Update peak followers with instant attack, ~50ms release
    auto updatePeak = [](std::atomic<float>& peak, float blockPeak) {
        float currentPeak = peak.load();
        if (blockPeak > currentPeak)
            peak.store(blockPeak);  // Instant attack
        else
            peak.store(currentPeak * 0.95f);  // ~50ms release at 48kHz
    };

    updatePeak(inputPeakL, inPkL);
    updatePeak(inputPeakR, inPkR);

    // === AGC INPUT RMS MEASUREMENT ===
    // Measure pristine input RMS for auto-gain compensation
    if (agcEnabled)
    {
        SampleType inputSumSquares = static_cast<SampleType>(0.0);
        for (int ch = 0; ch < pristineInputBuffer.getNumChannels(); ++ch)
        {
            const auto* data = pristineInputBuffer.getReadPointer(ch);
            for (int i = 0; i < numSamples; ++i)
                inputSumSquares += data[i] * data[i];
        }
        float inputRms = std::sqrt(static_cast<float>(inputSumSquares) /
                                   static_cast<float>(numSamples * pristineInputBuffer.getNumChannels()));
        // Smooth input RMS measurement
        float prevInputRms = agcInputRMS.load();
        agcInputRMS.store(prevInputRms * 0.9f + inputRms * 0.1f);
    }

    // Calculate bi-linear blend weights from XY pad
    // XY Grid Layout: Top-Left=HC, Top-Right=FL, Bottom-Left=SC, Bottom-Right=SL
    // X-axis: Left=CLIPPING (HC/SC), Right=LIMITING (FL/SL)
    // Y-axis: Top=HARD/TRANSPARENT (HC/FL), Bottom=SOFT/MUSICAL (SC/SL)
    // GUI sends: Y=1 for TOP, Y=0 for BOTTOM; X=0 for LEFT, X=1 for RIGHT
    SampleType wHC = (static_cast<SampleType>(1.0) - xyX) * xyY;  // Top-left (X=0, Y=1)
    SampleType wFL = xyX * xyY;  // Top-right (X=1, Y=1)
    SampleType wSC = (static_cast<SampleType>(1.0) - xyX) * (static_cast<SampleType>(1.0) - xyY);  // Bottom-left (X=0, Y=0)
    SampleType wSL = xyX * (static_cast<SampleType>(1.0) - xyY);  // Bottom-right (X=1, Y=0)

    // Apply mute logic: zero out weights for muted processors
    if (hcMute) wHC = static_cast<SampleType>(0.0);
    if (scMute) wSC = static_cast<SampleType>(0.0);
    if (slMute) wSL = static_cast<SampleType>(0.0);
    if (flMute) wFL = static_cast<SampleType>(0.0);

    // Renormalize weights to sum to 1.0 (avoid division by zero)
    const SampleType weightSum = wHC + wSC + wSL + wFL;
    const bool allProcessorsMuted = (weightSum <= static_cast<SampleType>(1e-10));

    if (!allProcessorsMuted)
    {
        const SampleType normFactor = static_cast<SampleType>(1.0) / weightSum;
        wHC *= normFactor;
        wSC *= normFactor;
        wSL *= normFactor;
        wFL *= normFactor;
    }
    else
    {
        // All processors muted - weights stay zero (will skip processing and use dry signal)
        wHC = wSC = wSL = wFL = static_cast<SampleType>(0.0);
    }

    // === EARLY EXIT: All Processors Muted ===
    // When all processors are muted, skip ALL processing including oversampling
    // This prevents oversampling filter coloration and ensures bit-perfect passthrough at 0% mix
    // Dry = pristine input (no normalization), Wet = normalized input (no drive)
    if (allProcessorsMuted)
    {
        // When all processors muted:
        // - Dry = pristine input (NO normalization, NO gains)
        // - Wet = normalized input (normalization only, NO drive processing)
        // - Mix blends between them (0% = pure pristine, 100% = pure normalized)
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* out = buffer.getWritePointer(ch);
            const auto* pristine = pristineInputBuffer.getReadPointer(ch);      // NO normalization
            const auto* normalized = normalizedInputBuffer.getReadPointer(ch);  // HAS normalization

            for (int i = 0; i < numSamples; ++i)
            {
                const SampleType smoothedOutGain = static_cast<SampleType>(smoothedOutputGain.getNextValue());
                const SampleType smoothedMix = static_cast<SampleType>(smoothedMixWet.getNextValue());

                // Check for unity output gain (within 0.1% of 1.0)
                const bool isUnityOutputGain = (smoothedOutGain > static_cast<SampleType>(0.999) &&
                                                smoothedOutGain < static_cast<SampleType>(1.001));

                // Dry/wet mix with threshold checks
                if (smoothedMix < static_cast<SampleType>(0.001))
                {
                    // Pure dry (0% mix) = pristine input, no normalization
                    out[i] = pristine[i];
                }
                else if (smoothedMix > static_cast<SampleType>(0.999))
                {
                    // Pure wet (100% mix) = normalized input * output gain
                    if (isUnityOutputGain)
                    {
                        out[i] = normalized[i];  // Full normalization, unity output gain
                    }
                    else
                    {
                        out[i] = normalized[i] * smoothedOutGain;  // Normalization + output gain
                    }
                }
                else
                {
                    // Partial mix - blend between pristine (dry) and normalized (wet)
                    // At 50% mix: half normalization gain (blend of pristine + normalized)
                    SampleType wetSignal = isUnityOutputGain ? normalized[i] : normalized[i] * smoothedOutGain;
                    out[i] = pristine[i] * (static_cast<SampleType>(1.0) - smoothedMix) + wetSignal * smoothedMix;
                }
            }
        }

        // Skip to protection limiters / output section
        // (Fall through to overshoot limiter section below)
    }
    else
    {
    // === ARCHITECTURE A: GLOBAL OVERSAMPLING + XY BLEND PROCESSING ===
    // v1.7.6: Both wet and dry MUST go through OS together for phase coherence
    // Problem: Calling oversampler twice (wet, then dry) causes phase misalignment
    // Solution: 4-channel processing [wetL, wetR, dryL, dryR] through same OS call

    // Store dry signal (pristine input - NO normalization, NO gains, NO processing)
    // Mix at 0% should always give pristine input regardless of normalization setting
    dryBuffer.makeCopyOf(pristineInputBuffer);

    if (processingMode == 0)
    {
        // MODE 0: No oversampling needed
        auto osBlock = osManager.upsampleBlock(buffer);
        const int osNumSamples = static_cast<int>(osBlock.getNumSamples());
        const int osNumChannels = static_cast<int>(osBlock.getNumChannels());

        if (!allProcessorsMuted)
        {
            SampleType* channelPointers[2];
            for (int ch = 0; ch < osNumChannels; ++ch)
                channelPointers[ch] = osBlock.getChannelPointer(static_cast<size_t>(ch));

            juce::AudioBuffer<SampleType> osBuffer(channelPointers, osNumChannels, osNumSamples);
            processXYBlend(osBuffer, osManager.getOsSampleRate());
        }

        osManager.downsampleBlock(buffer, osBlock);
        // Dry already pristine, no processing needed
    }
    else
    {
        // MODES 1/2: 4-channel synchronized OS processing for phase coherence
        // Both dry and wet through SAME oversampler call

        // Get 4-channel oversampler
        juce::dsp::Oversampling<SampleType>* oversampler4Ch = nullptr;
        if (std::is_same<SampleType, float>::value)
        {
            oversampler4Ch = processingMode == 1
                ? reinterpret_cast<juce::dsp::Oversampling<SampleType>*>(oversampling4ChBalancedFloat.get())
                : reinterpret_cast<juce::dsp::Oversampling<SampleType>*>(oversampling4ChLinearFloat.get());
        }
        else
        {
            oversampler4Ch = processingMode == 1
                ? reinterpret_cast<juce::dsp::Oversampling<SampleType>*>(oversampling4ChBalancedDouble.get())
                : reinterpret_cast<juce::dsp::Oversampling<SampleType>*>(oversampling4ChLinearDouble.get());
        }

        // Create 4-channel buffer: [wetL, wetR, dryL, dryR]
        auto& combined4Ch = (std::is_same<SampleType, float>::value)
            ? reinterpret_cast<juce::AudioBuffer<SampleType>&>(combined4ChFloat)
            : reinterpret_cast<juce::AudioBuffer<SampleType>&>(combined4ChDouble);

        combined4Ch.setSize(4, numSamples, false, false, true);
        for (int ch = 0; ch < 2; ++ch)
        {
            combined4Ch.copyFrom(ch, 0, buffer, ch, 0, numSamples);           // Wet (0-1)
            combined4Ch.copyFrom(ch + 2, 0, dryBuffer, ch, 0, numSamples);    // Dry (2-3)
        }

        // Upsample all 4 channels together (phase-coherent OS filtering)
        juce::dsp::AudioBlock<SampleType> combined4ChBlock(combined4Ch);
        auto osBlock4Ch = oversampler4Ch->processSamplesUp(combined4ChBlock);
        const int osNumSamples = static_cast<int>(osBlock4Ch.getNumSamples());

        // Process ONLY wet channels (0-1) with XY blend (has lookahead)
        if (!allProcessorsMuted)
        {
            SampleType* wetPointers[2] = {
                osBlock4Ch.getChannelPointer(0),
                osBlock4Ch.getChannelPointer(1)
            };
            juce::AudioBuffer<SampleType> wetBuffer(wetPointers, 2, osNumSamples);
            processXYBlend(wetBuffer, oversampler4Ch->getOversamplingFactor() * currentSampleRate);
        }

        // Delay dry channels (2-3) in OS domain to match wet's XY lookahead
        const int osLookahead = lookaheadSamples;  // Already at OS rate
        if (osLookahead > 0)
        {
            for (int ch = 0; ch < 2; ++ch)
            {
                auto& delayState = dryDelayState[ch];
                auto* dryData = osBlock4Ch.getChannelPointer(static_cast<size_t>(ch + 2));

                for (int i = 0; i < osNumSamples; ++i)
                {
                    const double delayed = delayState.delayBuffer[delayState.writePos];
                    delayState.delayBuffer[delayState.writePos] = static_cast<double>(dryData[i]);
                    delayState.writePos = (delayState.writePos + 1) % osLookahead;
                    dryData[i] = static_cast<SampleType>(delayed);
                }
            }
        }

        // === DELTA MODE: Deferred to after limiters ===
        // Delta will be computed after ALL processing (processors + limiters)
        // For now, keep wet and dry separate through downsampling
        // (Delta computation happens later at line ~1458)

        // Downsample all 4 channels together (phase-coherent)
        auto& tempBuffer2 = (std::is_same<SampleType, float>::value)
            ? reinterpret_cast<juce::AudioBuffer<SampleType>&>(tempBuffer2Float)
            : reinterpret_cast<juce::AudioBuffer<SampleType>&>(tempBuffer2Double);

        tempBuffer2.setSize(4, numSamples, false, false, true);
        juce::dsp::AudioBlock<SampleType> downBlock(tempBuffer2);
        oversampler4Ch->processSamplesDown(downBlock);

        // Extract wet and dry (now perfectly phase-coherent)
        for (int ch = 0; ch < 2; ++ch)
        {
            buffer.copyFrom(ch, 0, tempBuffer2, ch, 0, numSamples);         // Wet: 0-1
            dryBuffer.copyFrom(ch, 0, tempBuffer2, ch + 2, 0, numSamples);  // Dry: 2-3
        }
    }

    // === TRANSFER CURVE METER: PEAK FOLLOWER ===
    // Measure peak after XY blend but before output limiters and mix
    // This shows the composite saturation character (mono sum)
    float blockPeak = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        float magnitude = buffer.getMagnitude(ch, 0, numSamples);
        blockPeak = std::max(blockPeak, magnitude);
    }

    // Update peak follower with fast attack, medium release
    float currentPeak = meterPeak.load();
    if (blockPeak > currentPeak)
    {
        meterPeak.store(blockPeak);  // Instant attack
    }
    else
    {
        // ~50ms release at 48kHz (adjust factor based on block size if needed)
        meterPeak.store(currentPeak * 0.95f);
    }

    // === PHASE DIFFERENCE LIMITER (DISABLED - Too audible) ===
    // Phase limiting on wet/dry deviation fundamentally changes nonlinear character
    // Harmonic generation from saturation naturally creates phase shifts
    // Constraining this removes the musical quality of the processing
    // const bool phaseLimiterEnabled = apvts.getRawParameterValue("PHASE_LIMITER_ENABLE")->load() > 0.5f;
    // const SampleType phaseDeviationDegrees = static_cast<SampleType>(apvts.getRawParameterValue("PHASE_LIMITER_THRESHOLD")->load());
    // const SampleType phaseDeviationRadians = phaseDeviationDegrees * static_cast<SampleType>(juce::MathConstants<double>::pi / 180.0);
    // processPhaseDifferenceLimiter(buffer, dryBuffer, phaseDeviationRadians, currentSampleRate, phaseLimiterEnabled);

    // NOTE: GR capture has been moved to AFTER all processing (including True Peak)
    // This ensures GR trace and waveform are perfectly synchronized

    // === DELTA MODE: COMPENSATE BOTH WET AND DRY FOR GAINS ===
    // When delta mode is ON, apply the same gain adjustments to both wet and dry
    // This compensates for input gain, processor trims, inverse input gain, and output gain
    // Result: delta = wet - dry shows ONLY distortion artifacts, not gain changes
    if (deltaMode)
    {
        // Apply inverse input gain and output gain to wet (same as normal mode)
        // NOTE: Master Comp is now ALWAYS enabled (removed UI button)
        if (std::abs(inputGain - static_cast<SampleType>(1.0)) > static_cast<SampleType>(1e-6))
        {
            buffer.applyGain(static_cast<SampleType>(1.0) / inputGain);
        }

        // Apply output gain to wet (smoothed per-sample)
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* wet = buffer.getWritePointer(ch);
            for (int i = 0; i < numSamples; ++i)
            {
                const SampleType smoothedOutGain = static_cast<SampleType>(smoothedOutputGain.getNextValue());
                wet[i] *= smoothedOutGain;
            }
        }
        // Calculate weighted trim gain (same as applied in processXYBlend)
        // Trims are applied per-processor, then blended, so effective trim = weighted sum
        const SampleType hcTrimGain = juce::Decibels::decibelsToGain(static_cast<SampleType>(apvts.getRawParameterValue("HC_TRIM")->load()));
        const SampleType scTrimGain = juce::Decibels::decibelsToGain(static_cast<SampleType>(apvts.getRawParameterValue("SC_TRIM")->load()));
        const SampleType slTrimGain = juce::Decibels::decibelsToGain(static_cast<SampleType>(apvts.getRawParameterValue("SL_TRIM")->load()));
        const SampleType flTrimGain = juce::Decibels::decibelsToGain(static_cast<SampleType>(apvts.getRawParameterValue("FL_TRIM")->load()));

        // Get XY blend weights (same calculation as in processBlockInternal)
        const SampleType xyX = static_cast<SampleType>(apvts.getRawParameterValue("XY_X_PARAM")->load());
        const SampleType xyY = static_cast<SampleType>(apvts.getRawParameterValue("XY_Y_PARAM")->load());
        SampleType wHC = (static_cast<SampleType>(1.0) - xyX) * xyY;  // Top-left
        SampleType wFL = xyX * xyY;  // Top-right
        SampleType wSC = (static_cast<SampleType>(1.0) - xyX) * (static_cast<SampleType>(1.0) - xyY);  // Bottom-left
        SampleType wSL = xyX * (static_cast<SampleType>(1.0) - xyY);  // Bottom-right

        // Apply mute and solo logic
        // Mute takes priority over solo (like an analog console - if it's cut, it's cut)
        const bool hcMuteParam = apvts.getRawParameterValue("HC_MUTE")->load() > 0.5f;
        const bool scMuteParam = apvts.getRawParameterValue("SC_MUTE")->load() > 0.5f;
        const bool slMuteParam = apvts.getRawParameterValue("SL_MUTE")->load() > 0.5f;
        const bool flMuteParam = apvts.getRawParameterValue("FL_MUTE")->load() > 0.5f;

        const bool hcSolo = apvts.getRawParameterValue("HC_SOLO")->load() > 0.5f;
        const bool scSolo = apvts.getRawParameterValue("SC_SOLO")->load() > 0.5f;
        const bool slSolo = apvts.getRawParameterValue("SL_SOLO")->load() > 0.5f;
        const bool flSolo = apvts.getRawParameterValue("FL_SOLO")->load() > 0.5f;

        // Solo logic: if any processor is soloed, mute all non-soloed processors
        // BUT explicit mute always takes priority (mute overrides solo)
        const bool anySolo = hcSolo || scSolo || slSolo || flSolo;
        bool hcMute = hcMuteParam;  // Explicit mute always applies
        bool scMute = scMuteParam;
        bool slMute = slMuteParam;
        bool flMute = flMuteParam;
        if (anySolo)
        {
            // Only mute non-soloed processors that aren't already explicitly muted
            if (!hcSolo && !hcMuteParam) hcMute = true;
            if (!scSolo && !scMuteParam) scMute = true;
            if (!slSolo && !slMuteParam) slMute = true;
            if (!flSolo && !flMuteParam) flMute = true;
        }

        if (hcMute) wHC = static_cast<SampleType>(0.0);
        if (scMute) wSC = static_cast<SampleType>(0.0);
        if (slMute) wSL = static_cast<SampleType>(0.0);
        if (flMute) wFL = static_cast<SampleType>(0.0);

        // Renormalize weights
        const SampleType weightSum = wHC + wSC + wSL + wFL;
        const bool allProcessorsMuted = (weightSum <= static_cast<SampleType>(1e-10));
        SampleType weightedTrimGain = static_cast<SampleType>(1.0);
        if (!allProcessorsMuted)
        {
            const SampleType normFactor = static_cast<SampleType>(1.0) / weightSum;
            wHC *= normFactor;
            wSC *= normFactor;
            wSL *= normFactor;
            wFL *= normFactor;

            // Calculate effective trim gain (weighted sum of trims)
            weightedTrimGain = wHC * hcTrimGain + wSC * scTrimGain + wSL * slTrimGain + wFL * flTrimGain;

            // Apply compensation if master comp is enabled (always enabled per comment at line 1290)
            const bool masterCompEnabled = apvts.getRawParameterValue("MASTER_COMP")->load() > 0.5f;
            if (masterCompEnabled)
            {
                // Master comp applies inverse trim per-processor, then blends
                // Effective gain = weighted sum of (trim * 1/trim) = weighted sum of 1.0 = 1.0
                // So no trim compensation needed if master comp is enabled
                weightedTrimGain = static_cast<SampleType>(1.0);
            }
        }

        // Apply gains to dry signal to match wet signal path:
        // 0. Normalization gain (wet has this, dry captured from pristineInputBuffer doesn't)
        // 1. Input gain (wet has this from before oversampling, dry doesn't)
        // 2. Weighted trim gain (wet has this from processXYBlend, dry doesn't)
        // 3. Inverse input gain (wet gets this, dry should too)
        // 4. Output gain (wet gets this, dry should too)
        const SampleType deltaCompNormGain = normalizationEnabled
            ? static_cast<SampleType>(computedNormalizationGain)
            : static_cast<SampleType>(1.0);

        for (int ch = 0; ch < dryBuffer.getNumChannels(); ++ch)
        {
            auto* dry = dryBuffer.getWritePointer(ch);

            for (int i = 0; i < numSamples; ++i)
            {
                // Apply normalization gain (to match wet - critical for calibration compensation)
                SampleType compensatedDry = dry[i] * deltaCompNormGain;

                // Apply input gain (to match wet)
                compensatedDry *= inputGain;

                // Apply weighted trim gain (to match wet)
                compensatedDry *= weightedTrimGain;

                // Apply inverse input gain (to match wet - master comp is always enabled)
                if (std::abs(inputGain - static_cast<SampleType>(1.0)) > static_cast<SampleType>(1e-6))
                {
                    compensatedDry *= static_cast<SampleType>(1.0) / inputGain;
                }

                // Apply output gain (to match wet - smoothed per-sample)
                const SampleType smoothedOutGain = static_cast<SampleType>(smoothedOutputGain.getNextValue());
                compensatedDry *= smoothedOutGain;

                dry[i] = compensatedDry;
            }
        }
    }
    else
    {
        // === PARALLEL PROCESSING: DRY/WET MIX ===
        // Dry: pristine input (with normalization only), delayed to match wet latency
        // Wet: normalization + input gain + processing + inverse gain (if master comp) + output gain
        // Both time-aligned → zero phase shift → no comb filtering

        // Apply inverse input gain to wet if master compensation enabled
        // This keeps output level constant as input gain (drive) is adjusted
        // Master Comp auto-enables when Delta Mode is ON, disabled when Delta Mode is OFF
        const bool masterCompEnabled = apvts.getRawParameterValue("MASTER_COMP")->load() > 0.5f;
        if (masterCompEnabled && std::abs(inputGain - static_cast<SampleType>(1.0)) > static_cast<SampleType>(1e-6))
        {
            buffer.applyGain(static_cast<SampleType>(1.0) / inputGain);
        }

        // Apply output gain to wet BEFORE mixing (smoothed per-sample to prevent zipper noise)
        // Combined with dry/wet mix in single loop for efficiency
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* wet = buffer.getWritePointer(ch);
            const auto* dry = dryBuffer.getReadPointer(ch);

            for (int i = 0; i < numSamples; ++i)
            {
                const SampleType smoothedOutGain = static_cast<SampleType>(smoothedOutputGain.getNextValue());
                const SampleType smoothedMix = static_cast<SampleType>(smoothedMixWet.getNextValue());

                // Apply output gain to wet
                wet[i] *= smoothedOutGain;

                // Dry/wet mix with threshold checks for exact 0% and 100%
                // Parameter smoothing may asymptotically approach but never reach extremes
                // At < 0.1% mix, output pure dry signal (bit-perfect for null tests)
                // At > 99.9% mix, output pure wet signal (no dry contamination)
                if (smoothedMix < static_cast<SampleType>(0.001))
                {
                    wet[i] = dry[i];  // Pure dry - no wet contamination
                }
                else if (smoothedMix > static_cast<SampleType>(0.999))
                {
                    // Pure wet - already in wet[i], no change needed
                }
                else
                {
                    // Normal crossfade (dry is pristine, wet has all gains + processing)
                    wet[i] = dry[i] * (static_cast<SampleType>(1.0) - smoothedMix) + wet[i] * smoothedMix;
                }
            }
        }
    }
    }  // End of else block (normal processing when processors are active)

    // === DELTA MODE: COMPUTE DELTA BEFORE LIMITERS ===
    // When delta mode is ON, compute delta = wet_processed - dry_compensated BEFORE limiters
    // Then apply limiters to the delta output (so delta shows only processor artifacts, not limiter artifacts)
    // This eliminates timing issues from lookahead buffers
    if (deltaMode)
    {
        // Compute delta = processed_wet - compensated_dry (before limiters)
        // Both signals have same gains applied, so delta shows only processor distortion
        for (int ch = 0; ch < buffer.getNumChannels() && ch < dryBuffer.getNumChannels(); ++ch)
        {
            auto* processed = buffer.getWritePointer(ch);
            const auto* dry = dryBuffer.getReadPointer(ch);

            for (int i = 0; i < numSamples; ++i)
            {
                // Delta = processed_wet - compensated_dry (shows only processor artifacts)
                processed[i] = processed[i] - dry[i];
            }
        }
        // Delta is now in buffer - limiters will be applied to delta below
    }

    // === SIMPLIFIED OUTPUT LIMITING ===
    // Both limiters share the same ceiling
    const bool overshootEnabled = apvts.getRawParameterValue("OVERSHOOT_ENABLE")->load() > 0.5f;
    // True Peak Limiter is disabled in Zero Latency mode (requires lookahead)
    const bool truePeakEnabled = (processingMode != 0) &&
                                 (apvts.getRawParameterValue("TRUE_PEAK_ENABLE")->load() > 0.5f);
    const SampleType outputCeilingDB = static_cast<SampleType>(apvts.getRawParameterValue("OUTPUT_CEILING")->load());
    const bool overshootDeltaMode = apvts.getRawParameterValue("OVERSHOOT_DELTA_MODE")->load() > 0.5f;
    const bool truePeakDeltaMode = apvts.getRawParameterValue("TRUE_PEAK_DELTA_MODE")->load() > 0.5f;

    // Process limiters and capture their artifacts for main Delta mode
    // When main Delta mode is ON, we want to include limiter GR in the overall artifact signal
    auto& limiterRefBuffer = tempBuffer4;

    // Save pre-limiter state if in main delta mode (to capture limiter artifacts)
    if (deltaMode && (overshootEnabled || truePeakEnabled))
    {
        limiterRefBuffer.makeCopyOf(buffer);
    }

    // Process limiters (independent delta modes are separate from main delta)
    // In delta mode, limiters process the delta signal itself
    if (!deltaMode || (overshootEnabled || truePeakEnabled))
    {
        // === SPECIAL CASE: BOTH LIMITERS ENABLED ===
        // Use combined processing path to avoid double oversampling artifacts
        if (overshootEnabled && truePeakEnabled && !overshootDeltaMode && !truePeakDeltaMode)
        {
            // Process both limiters in single oversample cycle: upsample → overshoot → truepeak → downsample
            processCombinedLimiters(buffer, outputCeilingDB, currentSampleRate);
        }
        else
        {
            // === OVERSHOOT SUPPRESSION ONLY ===
            // Character shaping with controlled overshoot
            if (overshootEnabled)
            {
                if (overshootDeltaMode)
                {
                    limiterRefBuffer.makeCopyOf(buffer);
                    processOvershootSuppression(buffer, outputCeilingDB, currentSampleRate, true, &limiterRefBuffer);

                    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                    {
                        auto* processed = buffer.getWritePointer(ch);
                        const auto* reference = limiterRefBuffer.getReadPointer(ch);
                        for (int i = 0; i < numSamples; ++i)
                            processed[i] = processed[i] - reference[i];
                    }
                }
                else
                {
                    processOvershootSuppression(buffer, outputCeilingDB, currentSampleRate, true);
                }
            }

            // === TRUE PEAK LIMITER ONLY ===
            // ITU-R BS.1770-4 compliance - catches peaks that Overshoot missed
            if (truePeakEnabled && !overshootDeltaMode)
            {
                if (truePeakDeltaMode)
                {
                    limiterRefBuffer.makeCopyOf(buffer);
                    processAdvancedTPL(buffer, outputCeilingDB, currentSampleRate, &limiterRefBuffer);

                    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                    {
                        auto* processed = buffer.getWritePointer(ch);
                        const auto* reference = limiterRefBuffer.getReadPointer(ch);
                        for (int i = 0; i < numSamples; ++i)
                            processed[i] = processed[i] - reference[i];
                    }
                }
                else
                {
                    processAdvancedTPL(buffer, outputCeilingDB, currentSampleRate);
                }
            }
        }
    }

    // === CHANNEL MODE: M/S DECODING ===
    // Convert back from M/S to L/R for output (if M/S mode was used)
    // Must happen AFTER all processing but BEFORE output metering
    if (channelMode == 1 && buffer.getNumChannels() >= 2)  // Mid-Side mode - decode back to L/R
    {
        // Decode M/S to L/R: Left = Mid + Side, Right = Mid - Side
        auto* mid = buffer.getWritePointer(0);
        auto* side = buffer.getWritePointer(1);
        for (int i = 0; i < numSamples; ++i)
        {
            const SampleType m = mid[i];
            const SampleType s = side[i];
            mid[i] = m + s;   // Left = Mid + Side
            side[i] = m - s;  // Right = Mid - Side
        }
    }
    // Note: Mono mode (channelMode == 2) doesn't need decoding - both channels stay identical

    // === AGC (AUTO-GAIN COMPENSATION) ===
    // Match output loudness to input for honest A/B comparison
    if (agcEnabled && !deltaMode)
    {
        // Measure output RMS
        SampleType outputSumSquares = static_cast<SampleType>(0.0);
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            const auto* data = buffer.getReadPointer(ch);
            for (int i = 0; i < numSamples; ++i)
                outputSumSquares += data[i] * data[i];
        }
        float outputRms = std::sqrt(static_cast<float>(outputSumSquares) /
                                    static_cast<float>(numSamples * buffer.getNumChannels()));

        // Smooth output RMS measurement
        float prevOutputRms = agcOutputRMS.load();
        agcOutputRMS.store(prevOutputRms * 0.9f + outputRms * 0.1f);

        // Calculate and apply AGC gain
        float smoothedInputRms = agcInputRMS.load();
        float smoothedOutputRms = agcOutputRMS.load();

        if (smoothedOutputRms > 0.0001f && smoothedInputRms > 0.0001f)
        {
            float targetAgcGain = smoothedInputRms / smoothedOutputRms;
            // Clamp AGC gain to reasonable range (±12dB)
            targetAgcGain = juce::jlimit(0.25f, 4.0f, targetAgcGain);

            // Update smoothed AGC gain
            smoothedAgcGain.setTargetValue(targetAgcGain);

            // Apply smoothed AGC gain per-sample to prevent zipper noise
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            {
                auto* data = buffer.getWritePointer(ch);
                for (int i = 0; i < numSamples; ++i)
                {
                    data[i] *= static_cast<SampleType>(smoothedAgcGain.getNextValue());
                }
            }
        }
    }
    else if (!agcEnabled)
    {
        // Reset AGC state when disabled
        smoothedAgcGain.setCurrentAndTargetValue(1.0f);
        agcInputRMS.store(0.0f);
        agcOutputRMS.store(0.0f);
    }

    // === STEREO I/O METERS: MEASURE OUTPUT SAMPLE PEAK ===
    // Use sample peak for consistent metering with DAWs (true peak can read +3-4dB higher)
    float outPkL = 0.0f;
    float outPkR = 0.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        float absL = std::abs(static_cast<float>(buffer.getReadPointer(0)[i]));
        if (absL > outPkL) outPkL = absL;
        if (buffer.getNumChannels() > 1)
        {
            float absR = std::abs(static_cast<float>(buffer.getReadPointer(1)[i]));
            if (absR > outPkR) outPkR = absR;
        }
    }
    if (buffer.getNumChannels() == 1) outPkR = outPkL;

    updatePeak(outputPeakL, outPkL);
    updatePeak(outputPeakR, outPkR);

    // === MAIN DELTA MODE: DELTA ALREADY COMPUTED ===
    // Delta was computed BEFORE limiters (above), then limiters were applied to delta
    // This ensures no timing issues from lookahead buffers
    // Delta now shows only processor artifacts (gain reduction/distortion), with limiters applied for safety

    // === UPDATE OUTPUT PEAK METERS ===
    // Capture the final output level after ALL processing (delta, mix, output gain, protection)
    float peakL = 0.0f;
    float peakR = 0.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        if (buffer.getNumChannels() > 0)
        {
            float absVal = static_cast<float>(std::abs(buffer.getReadPointer(0)[i]));
            if (absVal > peakL) peakL = absVal;
        }
        if (buffer.getNumChannels() > 1)
        {
            float absVal = static_cast<float>(std::abs(buffer.getReadPointer(1)[i]));
            if (absVal > peakR) peakR = absVal;
        }
    }
    currentOutputPeakL.store(peakL);
    currentOutputPeakR.store(peakR);

    // === FINAL VISUALIZATION DATA CAPTURE ===
    // CRITICAL: GR and waveform MUST be captured from SAME processed signal
    // This happens AFTER all processing: XY blend, mix, output gain, AND protection limiters
    // This ensures GR trace perfectly aligns with waveform display

    // Calculate final gain reduction (compares final output to normalized input)
    SampleType maxInputLevel = static_cast<SampleType>(0.0);
    SampleType maxOutputLevel = static_cast<SampleType>(0.0);
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        const auto* output = buffer.getReadPointer(ch);
        const auto* input = dryBuffer.getReadPointer(ch);  // Dry = normalized input before processing

        for (int i = 0; i < numSamples; ++i)
        {
            // Input level = normalized input × input gain (what we fed to processors)
            maxInputLevel = juce::jmax(maxInputLevel, std::abs(input[i] * inputGain));
            // Output level = final output after ALL processing including True Peak
            maxOutputLevel = juce::jmax(maxOutputLevel, std::abs(output[i]));
        }
    }

    float grDB = 0.0f;
    if (maxInputLevel > static_cast<SampleType>(0.00001) && maxOutputLevel > static_cast<SampleType>(0.00001))
    {
        grDB = juce::Decibels::gainToDecibels(static_cast<float>(maxOutputLevel / maxInputLevel));
        if (grDB > 0.0f) grDB = 0.0f;  // Only show reduction, not gain
    }
    currentGainReductionDB.store(grDB);

    // === WAVEFORM + GR DISPLAY DATA CAPTURE ===
    // Write synchronized waveform and GR data to unified display buffer
    // Only update display buffer when transport is playing (not frozen)
    if (!displayBufferFrozen.load())
    {
        int writePos = oscilloscopeWritePos.load();
        int oscSize = oscilloscopeSize.load();
        int displayWrite = displayWritePos.load();

        for (int i = 0; i < numSamples; ++i)
        {
            // Get stereo samples (OUTPUT)
            float sampleL = 0.0f;
            float sampleR = 0.0f;
            if (buffer.getNumChannels() > 0)
                sampleL = static_cast<float>(buffer.getReadPointer(0)[i]);
            if (buffer.getNumChannels() > 1)
                sampleR = static_cast<float>(buffer.getReadPointer(1)[i]);

            // === SAMPLE-ACCURATE GAIN REDUCTION CALCULATION ===
            // Compare input vs output at this exact sample to get precise GR
            float maxInputLevel = 0.0f;
            float maxOutputLevel = 0.0f;

            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            {
                const float dryVal = static_cast<float>(dryBuffer.getReadPointer(ch)[i]);
                const float wetVal = static_cast<float>(buffer.getReadPointer(ch)[i]);

                // Input level = normalized input × input gain (what we fed to processors)
                maxInputLevel = juce::jmax(maxInputLevel, std::abs(dryVal * static_cast<float>(inputGain)));
                // Output level = final output after ALL processing including True Peak
                maxOutputLevel = juce::jmax(maxOutputLevel, std::abs(wetVal));
            }

            float sampleGR = 0.0f;
            if (maxInputLevel > 0.00001f && maxOutputLevel > 0.00001f)
            {
                sampleGR = juce::Decibels::gainToDecibels(maxOutputLevel / maxInputLevel);
                if (sampleGR > 0.0f) sampleGR = 0.0f;  // Only show reduction, not gain
            }

            // Create mono mix for frequency analysis
            float monoSample = (sampleL + sampleR) * 0.5f;
            double sample = static_cast<double>(monoSample);

            // Apply frequency band filters ONCE (use left channel filter state)
            // Low-pass filter (Red channel <250 Hz)
            double lowOut = oscFilters[0].lowB0 * sample + oscFilters[0].lowZ1;
            oscFilters[0].lowZ1 = oscFilters[0].lowB1 * sample - oscFilters[0].lowA1 * lowOut + oscFilters[0].lowZ2;
            oscFilters[0].lowZ2 = oscFilters[0].lowB2 * sample - oscFilters[0].lowA2 * lowOut;
            float lowBand = static_cast<float>(std::abs(lowOut));

            // Band-pass filter (Green channel 250-4000 Hz)
            double midOut = oscFilters[0].midB0 * sample + oscFilters[0].midZ1;
            oscFilters[0].midZ1 = oscFilters[0].midB1 * sample - oscFilters[0].midA1 * midOut + oscFilters[0].midZ2;
            oscFilters[0].midZ2 = oscFilters[0].midB2 * sample - oscFilters[0].midA2 * midOut;
            float midBand = static_cast<float>(std::abs(midOut));

            // High-pass filter (Blue channel >4000 Hz)
            double highOut = oscFilters[0].highB0 * sample + oscFilters[0].highZ1;
            oscFilters[0].highZ1 = oscFilters[0].highB1 * sample - oscFilters[0].highA1 * highOut + oscFilters[0].highZ2;
            oscFilters[0].highZ2 = oscFilters[0].highB2 * sample - oscFilters[0].highA2 * highOut;
            float highBand = static_cast<float>(std::abs(highOut));

            // Write to legacy oscilloscope buffer (if active)
            if (oscSize > 0 && static_cast<int>(oscilloscopeBuffer.size()) >= oscSize && writePos < oscSize)
            {
                oscilloscopeBuffer[writePos] = monoSample;
                oscilloscopeLowBand[writePos] = lowBand;
                oscilloscopeMidBand[writePos] = midBand;
                oscilloscopeHighBand[writePos] = highBand;
            }

            // Write to unified display buffer with sample-accurate GR
            DisplaySample displaySample;
            displaySample.waveformL = sampleL;
            displaySample.waveformR = sampleR;
            displaySample.gainReduction = sampleGR;  // Sample-accurate GR!
            displaySample.lowBand = lowBand;
            displaySample.midBand = midBand;
            displaySample.highBand = highBand;

            // === WAVEFORM GR METER: PER-PROCESSOR OUTPUTS (ENVELOPE-SMOOTHED) ===
            // Use envelope followers for smooth per-sample visualization
            // Feed with max of stereo channels for consistent display
            float inputLevelNow = std::abs(static_cast<float>(dryBuffer.getReadPointer(0)[i] * inputGain));
            if (buffer.getNumChannels() > 1)
                inputLevelNow = std::max(inputLevelNow, std::abs(static_cast<float>(dryBuffer.getReadPointer(1)[i] * inputGain)));

            // Update envelope followers for input (smooth per-sample)
            processorEnvelope[0].processEnvelope(processorEnvelope[0].inputEnv, inputLevelNow);

            // Calculate envelope-smoothed per-processor outputs from block peaks
            // The block peaks give us the "target" - envelope followers smooth the transition
            float targetHC = currentHardClipPeak.load();
            float targetSC = currentSoftClipPeak.load();
            float targetSL = currentSlowLimitPeak.load();
            float targetFL = currentFastLimitPeak.load();

            processorEnvelope[0].processEnvelope(processorEnvelope[0].hardClipEnv, targetHC);
            processorEnvelope[0].processEnvelope(processorEnvelope[0].softClipEnv, targetSC);
            processorEnvelope[0].processEnvelope(processorEnvelope[0].slowLimitEnv, targetSL);
            processorEnvelope[0].processEnvelope(processorEnvelope[0].fastLimitEnv, targetFL);

            // Use smoothed envelope values for display (sample-accurate)
            displaySample.inputSignal = processorEnvelope[0].inputEnv;
            displaySample.hardClipOutput = processorEnvelope[0].hardClipEnv;
            displaySample.softClipOutput = processorEnvelope[0].softClipEnv;
            displaySample.slowLimitOutput = processorEnvelope[0].slowLimitEnv;
            displaySample.fastLimitOutput = processorEnvelope[0].fastLimitEnv;
            displaySample.finalOutput = std::abs(monoSample);  // Final output envelope

            // Calculate per-processor GR from envelope-smoothed values (sample-accurate)
            const float minLevel = 0.00001f;
            float inputEnv = processorEnvelope[0].inputEnv;
            if (inputEnv > minLevel)
            {
                auto calcGR = [minLevel, inputEnv](float outputEnv) -> float {
                    if (outputEnv > minLevel && outputEnv < inputEnv)
                        return 20.0f * std::log10(outputEnv / inputEnv);  // Negative dB = reduction
                    return 0.0f;
                };

                displaySample.hardClipGainReduction = calcGR(processorEnvelope[0].hardClipEnv);
                displaySample.softClipGainReduction = calcGR(processorEnvelope[0].softClipEnv);
                displaySample.slowLimitGainReduction = calcGR(processorEnvelope[0].slowLimitEnv);
                displaySample.fastLimitGainReduction = calcGR(processorEnvelope[0].fastLimitEnv);
            }
            else
            {
                displaySample.hardClipGainReduction = 0.0f;
                displaySample.softClipGainReduction = 0.0f;
                displaySample.slowLimitGainReduction = 0.0f;
                displaySample.fastLimitGainReduction = 0.0f;
            }

            displayBuffer[displayWrite] = displaySample;

            // Update ring buffer positions
            if (oscSize > 0)
                writePos = (writePos + 1) % oscSize;
            displayWrite = (displayWrite + 1) % displayBufferSize;
        }

        // Store updated positions
        if (oscSize > 0)
            oscilloscopeWritePos.store(writePos);
        displayWritePos.store(displayWrite);
    }
}

//==============================================================================
// Template DSP Processing Functions

// Hard Clipping - Pure digital clipping at threshold (like Ableton Saturator Digital Clip mode)
// v1.5.0 Pristine Essentials: Simple jlimit clipping, oversampling is the ONLY anti-aliasing
// MODE 0: Direct processing (no oversampling, no lookahead)
// MODE 1: 8× oversampling + 3ms lookahead
// MODE 2: 16× oversampling + 3ms lookahead
template<typename SampleType>
void QuadBlendDriveAudioProcessor::processHardClip(juce::AudioBuffer<SampleType>& buffer, SampleType threshold, double sampleRate)
{
    // Architecture A: Buffer is ALREADY in OS domain (osManager handles all oversampling)
    // Mode 0: Buffer at base rate (1× multiplier)
    // Mode 1: Buffer at 8× OS rate
    // Mode 2: Buffer at 16× OS rate

    const int processingMode = osManager.getProcessingMode();
    const double thresholdD = static_cast<double>(threshold);

    // MODE 0: Zero Latency - Direct hard clip, NO lookahead
    if (processingMode == 0)
    {
        for (int ch = 0; ch < juce::jmin(buffer.getNumChannels(), 2); ++ch)
        {
            auto* data = buffer.getWritePointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                const double x = static_cast<double>(data[i]);
                const double output = juce::jlimit(-thresholdD, thresholdD, x);
                data[i] = static_cast<SampleType>(output);
            }
        }
        return;
    }

    // MODE 1 & 2: Balanced or Linear Phase - Process with lookahead (already at OS rate)
    const int numSamples = buffer.getNumSamples();
    for (int ch = 0; ch < juce::jmin(buffer.getNumChannels(), 2); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        auto& state = hardClipState[ch];
        auto& lookaheadBuffer = state.lookaheadBuffer;
        int& writePos = state.lookaheadWritePos;

        for (int i = 0; i < numSamples; ++i)
        {
            const double currentSample = static_cast<double>(data[i]);

            // Store current sample in lookahead buffer
            if (writePos >= 0 && writePos < static_cast<int>(lookaheadBuffer.size()))
                lookaheadBuffer[writePos] = currentSample;

            // Read delayed sample for phase alignment
            int readPos = (writePos + 1) % lookaheadSamples;
            double delayedSample = currentSample;
            if (readPos >= 0 && readPos < static_cast<int>(lookaheadBuffer.size()))
                delayedSample = lookaheadBuffer[readPos];

            writePos = (writePos + 1) % lookaheadSamples;

            // Hard clip processing (osManager's oversampling handles anti-aliasing)
            const double output = juce::jlimit(-thresholdD, thresholdD, delayedSample);
            data[i] = static_cast<SampleType>(output);
        }
    }
}

// Soft Clipping - Tanh saturation with dynamic drive
// v1.5.0 Pristine Essentials: Simple tanh processing, oversampling is the ONLY anti-aliasing
// MODE 0: Direct processing (no oversampling, no lookahead)
// MODE 1: 8× oversampling + 3ms lookahead
// MODE 2: 16× oversampling + 3ms lookahead
template<typename SampleType>
void QuadBlendDriveAudioProcessor::processSoftClip(juce::AudioBuffer<SampleType>& buffer,
                                                     SampleType ceiling,
                                                     SampleType knee,
                                                     double sampleRate)
{
    // Get current processing mode from osManager
    const int processingMode = osManager.getProcessingMode();

    const double ceilingD = static_cast<double>(ceiling);
    const double color = static_cast<double>(knee);  // knee is already normalized to 0-1
    const double drive = 1.0 + (color * 3.0);  // Drive: 1.0 to 4.0

    // Makeup gain: normalize so output reaches ceiling (matches Hard Clip/Limiters)
    // tanh(drive) gives the max output, so divide by it to reach 1.0 at ceiling
    const double makeup = 1.0 / std::tanh(drive);

    // In oversampled modes, reduce makeup gain slightly to account for inter-sample peaks
    // The interpolation filter creates peaks ~0.5 dB higher than input samples
    // Hard Clip/Limiters handle this via peak detection, but Soft Clip needs compensation
    const double oversampleCompensation = (processingMode == 0) ? 1.0 : 0.944;  // -0.5 dB
    const double compensatedMakeup = makeup * oversampleCompensation;

    // MODE 0: Zero Latency - Direct tanh, NO oversampling, NO lookahead
    // Accept minimal aliasing as tradeoff for true zero latency
    if (processingMode == 0)
    {
        for (int ch = 0; ch < juce::jmin(buffer.getNumChannels(), 2); ++ch)
        {
            auto* data = buffer.getWritePointer(ch);

            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                const double x = static_cast<double>(data[i]) / ceilingD;
                const double output = std::tanh(x * drive) * compensatedMakeup;
                data[i] = static_cast<SampleType>(output * ceilingD);
            }
        }
        return;
    }

    // MODE 1 & 2: Architecture A - Process with lookahead (buffer already at OS rate)
    const int numSamples = buffer.getNumSamples();
    for (int ch = 0; ch < juce::jmin(buffer.getNumChannels(), 2); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        auto& lookaheadBuffer = softClipState[ch].lookaheadBuffer;
        int& writePos = softClipState[ch].lookaheadWritePos;

        for (int i = 0; i < numSamples; ++i)
        {
            const double currentSample = static_cast<double>(data[i]);

            // Store current sample in lookahead buffer
            if (writePos >= 0 && writePos < static_cast<int>(lookaheadBuffer.size()))
                lookaheadBuffer[writePos] = currentSample;

            // Read delayed sample (3ms lookahead)
            int readPos = (writePos + 1) % lookaheadSamples;
            double delayedSample = currentSample;
            if (readPos >= 0 && readPos < static_cast<int>(lookaheadBuffer.size()))
                delayedSample = lookaheadBuffer[readPos];

            writePos = (writePos + 1) % lookaheadSamples;

            // Tanh processing (osManager's oversampling handles anti-aliasing)
            const double x = delayedSample / ceilingD;
            const double output = std::tanh(x * drive) * compensatedMakeup;
            data[i] = static_cast<SampleType>(output * ceilingD);
        }
    }
}

// Slow Limiting - Crest-Factor-Based Adaptive Release Limiter
// Fast release (20-40ms) for transients, slow release (200-600ms) for sustained material
// Dynamically blended based on signal crest factor with exponential smoothing
// MODE-AWARE: Zero Latency = no oversampling, Balanced/Linear Phase = 8x oversampling
// CHANNEL LINKING: 0% = dual mono (independent), 100% = fully linked (max of both channels)
template<typename SampleType>
void QuadBlendDriveAudioProcessor::processSlowLimit(juce::AudioBuffer<SampleType>& buffer,
                                                      SampleType threshold,
                                                      SampleType baseReleaseMs,
                                                      SampleType attackMs,
                                                      double sampleRate)
{
    // Get current processing mode from osManager
    const int processingMode = osManager.getProcessingMode();

    // Get channel linking amount (0% = dual mono, 100% = fully linked)
    const float channelLink = apvts.getRawParameterValue("CHANNEL_LINK")->load() / 100.0f;
    const bool isStereo = buffer.getNumChannels() >= 2;

    // Fixed parameters for Slow Limiter
    const double attackMsD = static_cast<double>(attackMs);  // User-adjustable attack
    const double kneeDB = 3.0;                                // 3dB soft knee

    // Base release time scales the adaptive release ranges
    // Default: 100ms → fast(20-40ms), slow(300-800ms)
    // User can scale overall release speed via LIMIT_REL parameter
    const double baseReleaseScale = static_cast<double>(baseReleaseMs) / 100.0;  // Normalize to default
    const double fastReleaseMinMs = 20.0 * baseReleaseScale;    // Fast release min (for transients)
    const double fastReleaseMaxMs = 40.0 * baseReleaseScale;    // Fast release max
    const double slowReleaseMinMs = 300.0 * baseReleaseScale;   // Slow release min (for sustained)
    const double slowReleaseMaxMs = 800.0 * baseReleaseScale;   // Slow release max

    // Time constants for envelope followers
    // MODE 0: direct sample rate, MODE 1-2: oversampled rate
    const double safeSampleRate = (sampleRate > 0.0) ? sampleRate : 44100.0;
    const double effectiveRate = (processingMode == 0) ? safeSampleRate : (safeSampleRate * 8.0);
    const double rmsTimeMs = 50.0;                 // RMS averaging time
    const double peakTimeMs = 10.0;                // Peak tracking time
    const double crestSmoothTimeMs = 100.0;        // Crest factor smoothing time

    const double attackCoeff = std::exp(-1.0 / (attackMsD * 0.001 * effectiveRate));
    const double rmsCoeff = std::exp(-1.0 / (rmsTimeMs * 0.001 * effectiveRate));
    const double peakCoeff = std::exp(-1.0 / (peakTimeMs * 0.001 * effectiveRate));
    const double crestSmoothCoeff = std::exp(-1.0 / (crestSmoothTimeMs * 0.001 * effectiveRate));

    const double thresholdD = static_cast<double>(threshold);
    const double kneeStart = thresholdD * std::pow(10.0, -kneeDB / 20.0);  // 3dB below threshold

    // Gain smoothing filter coefficient to prevent control signal aliasing
    // Cutoff ~20kHz at the effective sample rate (prevents GR signal from aliasing)
    const double gainSmoothCutoff = 20000.0;  // 20 kHz cutoff
    const double gainSmoothCoeff = std::exp(-2.0 * juce::MathConstants<double>::pi * gainSmoothCutoff / effectiveRate);

    // MODE 0: Zero Latency - NO oversampling, NO lookahead, direct processing
    // With channel linking support: 0% = dual mono, 100% = fully linked (max of both)
    if (processingMode == 0)
    {
        const int numChannels = juce::jmin(buffer.getNumChannels(), 2);
        const int numSamples = buffer.getNumSamples();

        // Process sample-by-sample for proper channel linking
        for (int i = 0; i < numSamples; ++i)
        {
            // === PHASE 1: Detect input levels on all channels ===
            double channelInputEnv[2] = {0.0, 0.0};
            double channelEnvelope[2] = {0.0, 0.0};

            for (int ch = 0; ch < numChannels; ++ch)
            {
                auto* data = buffer.getWritePointer(ch);
                double& envelope = slowLimiterState[ch].envelope;
                double& rmsEnvelope = slowLimiterState[ch].rmsEnvelope;
                double& smoothedCrestFactor = slowLimiterState[ch].smoothedCrestFactor;

                const double currentSample = static_cast<double>(data[i]);
                const double inputAbs = std::abs(currentSample);
                channelInputEnv[ch] = inputAbs;

                // Track RMS for crest factor calculation
                const double inputSquared = currentSample * currentSample;
                rmsEnvelope = rmsCoeff * rmsEnvelope + (1.0 - rmsCoeff) * inputSquared;
                const double rmsValue = std::sqrt(juce::jmax(1e-10, rmsEnvelope));

                // Calculate crest factor
                const double instantCrestFactor = inputAbs / rmsValue;
                smoothedCrestFactor = crestSmoothCoeff * smoothedCrestFactor +
                                      (1.0 - crestSmoothCoeff) * instantCrestFactor;

                // Map crest factor to release time
                const double crestNorm = juce::jlimit(0.0, 1.0, (smoothedCrestFactor - 1.0) / 9.0);
                const double minRelease = fastReleaseMinMs + crestNorm * (slowReleaseMinMs - fastReleaseMinMs);
                const double maxRelease = fastReleaseMaxMs + crestNorm * (slowReleaseMaxMs - fastReleaseMaxMs);

                const double overAmount = juce::jmax(0.0, inputAbs - thresholdD);
                const double adaptiveFactor = overAmount / (thresholdD + 1e-10);
                const double adaptiveReleaseMs = juce::jlimit(minRelease, maxRelease,
                                                             minRelease + (maxRelease - minRelease) * adaptiveFactor);

                const double releaseCoeff = std::exp(-1.0 / (adaptiveReleaseMs * 0.001 * effectiveRate));

                // Attack/release envelope follower
                if (inputAbs > envelope)
                    envelope = attackCoeff * envelope + (1.0 - attackCoeff) * inputAbs;
                else
                    envelope = releaseCoeff * envelope + (1.0 - releaseCoeff) * inputAbs;

                channelEnvelope[ch] = envelope;
            }

            // === PHASE 2: Compute linked envelope ===
            // 0% link = dual mono (each channel uses own envelope)
            // 100% link = fully linked (both channels use max envelope)
            double maxEnvelope = 0.0;
            if (isStereo && numChannels >= 2)
            {
                maxEnvelope = juce::jmax(channelEnvelope[0], channelEnvelope[1]);
            }

            // === PHASE 3: Calculate and apply gain reduction ===
            for (int ch = 0; ch < numChannels; ++ch)
            {
                auto* data = buffer.getWritePointer(ch);
                const double currentSample = static_cast<double>(data[i]);

                // Blend between channel's own envelope and max envelope based on link amount
                double detectionEnvelope = channelEnvelope[ch];
                if (isStereo && channelLink > 0.0f)
                {
                    detectionEnvelope = channelEnvelope[ch] + static_cast<double>(channelLink) * (maxEnvelope - channelEnvelope[ch]);
                }

                // Calculate gain reduction with soft knee
                double targetGain = 1.0;
                const double kneeRange = thresholdD - kneeStart;
                if (detectionEnvelope <= kneeStart)
                {
                    targetGain = 1.0;
                }
                else if (detectionEnvelope < thresholdD && kneeRange > 1e-10)
                {
                    // Guard against division by zero when knee is very small
                    const double kneePos = (detectionEnvelope - kneeStart) / kneeRange;
                    const double compressionAmount = kneePos * kneePos;
                    targetGain = 1.0 - compressionAmount * (1.0 - thresholdD / detectionEnvelope);
                }
                else if (detectionEnvelope > 1e-10)
                {
                    targetGain = thresholdD / detectionEnvelope;
                }

                // Apply gain directly (MODE 0: no smoothing needed)
                const double limitedSample = currentSample * targetGain;
                data[i] = static_cast<SampleType>(limitedSample);
            }
        }
        return;
    }

    // MODE 1 & 2: Architecture A - Process with lookahead (buffer already at OS rate)
    // With channel linking support: 0% = dual mono, 100% = fully linked (max of both)
    {
        const int numChannels = juce::jmin(buffer.getNumChannels(), 2);
        const int numSamples = buffer.getNumSamples();

        // Process sample-by-sample for proper channel linking
        for (int i = 0; i < numSamples; ++i)
        {
            // === PHASE 1: Handle lookahead buffers and detect levels ===
            double channelEnvelope[2] = {0.0, 0.0};
            double delayedSamples[2] = {0.0, 0.0};

            for (int ch = 0; ch < numChannels; ++ch)
            {
                auto* data = buffer.getWritePointer(ch);
                double& envelope = slowLimiterState[ch].envelope;
                double& rmsEnvelope = slowLimiterState[ch].rmsEnvelope;
                double& smoothedCrestFactor = slowLimiterState[ch].smoothedCrestFactor;
                auto& lookaheadBuffer = slowLimiterState[ch].lookaheadBuffer;
                int& writePos = slowLimiterState[ch].lookaheadWritePos;

                const double currentSample = static_cast<double>(data[i]);

                // Store current sample in lookahead buffer
                if (writePos >= 0 && writePos < static_cast<int>(lookaheadBuffer.size()))
                    lookaheadBuffer[writePos] = currentSample;

                // Read delayed sample (3ms ago)
                int readPos = (writePos + 1) % lookaheadSamples;
                delayedSamples[ch] = currentSample;
                if (readPos >= 0 && readPos < static_cast<int>(lookaheadBuffer.size()))
                    delayedSamples[ch] = lookaheadBuffer[readPos];

                writePos = (writePos + 1) % lookaheadSamples;

                const double inputAbs = std::abs(currentSample);

                // Track RMS for crest factor calculation
                const double inputSquared = currentSample * currentSample;
                rmsEnvelope = rmsCoeff * rmsEnvelope + (1.0 - rmsCoeff) * inputSquared;
                const double rmsValue = std::sqrt(juce::jmax(1e-10, rmsEnvelope));

                // Calculate and smooth crest factor
                const double instantCrestFactor = inputAbs / rmsValue;
                smoothedCrestFactor = crestSmoothCoeff * smoothedCrestFactor +
                                      (1.0 - crestSmoothCoeff) * instantCrestFactor;

                // Map crest factor to release time
                const double crestNorm = juce::jlimit(0.0, 1.0, (smoothedCrestFactor - 1.0) / 9.0);
                const double minRelease = fastReleaseMinMs + crestNorm * (slowReleaseMinMs - fastReleaseMinMs);
                const double maxRelease = fastReleaseMaxMs + crestNorm * (slowReleaseMaxMs - fastReleaseMaxMs);

                const double overAmount = juce::jmax(0.0, inputAbs - thresholdD);
                const double adaptiveFactor = overAmount / (thresholdD + 1e-10);
                const double adaptiveReleaseMs = juce::jlimit(minRelease, maxRelease,
                                                             minRelease + (maxRelease - minRelease) * adaptiveFactor);

                const double releaseCoeff = std::exp(-1.0 / (adaptiveReleaseMs * 0.001 * effectiveRate));

                // Attack/release envelope follower
                if (inputAbs > envelope)
                    envelope = attackCoeff * envelope + (1.0 - attackCoeff) * inputAbs;
                else
                    envelope = releaseCoeff * envelope + (1.0 - releaseCoeff) * inputAbs;

                channelEnvelope[ch] = envelope;
            }

            // === PHASE 2: Compute linked envelope ===
            double maxEnvelope = 0.0;
            if (isStereo && numChannels >= 2)
            {
                maxEnvelope = juce::jmax(channelEnvelope[0], channelEnvelope[1]);
            }

            // === PHASE 3: Calculate and apply gain reduction ===
            for (int ch = 0; ch < numChannels; ++ch)
            {
                auto* data = buffer.getWritePointer(ch);
                double& smoothedGain = slowLimiterState[ch].smoothedGain;

                // Blend between channel's own envelope and max envelope based on link amount
                double detectionEnvelope = channelEnvelope[ch];
                if (isStereo && channelLink > 0.0f)
                {
                    detectionEnvelope = channelEnvelope[ch] + static_cast<double>(channelLink) * (maxEnvelope - channelEnvelope[ch]);
                }

                // Calculate gain reduction with soft knee
                double targetGain = 1.0;
                const double kneeRange = thresholdD - kneeStart;
                if (detectionEnvelope <= kneeStart)
                {
                    targetGain = 1.0;
                }
                else if (detectionEnvelope < thresholdD && kneeRange > 1e-10)
                {
                    // Guard against division by zero when knee is very small
                    const double kneePos = (detectionEnvelope - kneeStart) / kneeRange;
                    const double compressionAmount = kneePos * kneePos;
                    targetGain = 1.0 - compressionAmount * (1.0 - thresholdD / detectionEnvelope);
                }
                else if (detectionEnvelope > 1e-10)
                {
                    targetGain = thresholdD / detectionEnvelope;
                }

                // Apply gain smoothing (prevents control signal aliasing with oversampling)
                smoothedGain = gainSmoothCoeff * smoothedGain + (1.0 - gainSmoothCoeff) * targetGain;

                const double limitedSample = delayedSamples[ch] * smoothedGain;
                data[i] = static_cast<SampleType>(limitedSample);
            }
        }
    }
}

// Fast Limiting - Hard knee limiting with fixed 10ms release
// Attack: 1ms, Release: 10ms (fixed), Hard knee, Lookahead: 3ms
// MODE-AWARE: Zero Latency = no oversampling, Balanced/Linear Phase = 8x oversampling
// CHANNEL LINKING: 0% = dual mono (independent), 100% = fully linked (max of both channels)
template<typename SampleType>
void QuadBlendDriveAudioProcessor::processFastLimit(juce::AudioBuffer<SampleType>& buffer,
                                                      SampleType threshold,
                                                      SampleType attackMs,
                                                      double sampleRate)
{
    // Get current processing mode from osManager
    const int processingMode = osManager.getProcessingMode();

    // Get channel linking amount (0% = dual mono, 100% = fully linked)
    const float channelLink = apvts.getRawParameterValue("CHANNEL_LINK")->load() / 100.0f;
    const bool isStereo = buffer.getNumChannels() >= 2;

    // Fixed parameters for Fast Limiter
    const double attackMsD = static_cast<double>(attackMs);  // User-adjustable attack
    const double releaseMs = 10.0;                            // 10ms release (fixed)

    // MODE 0: direct sample rate, MODE 1-2: oversampled rate
    const double safeSampleRate = (currentSampleRate > 0.0) ? currentSampleRate : 44100.0;
    const double effectiveRate = (processingMode == 0) ? safeSampleRate : (safeSampleRate * 8.0);

    const double attackCoeff = std::exp(-1.0 / (attackMsD * 0.001 * effectiveRate));
    const double releaseCoeff = std::exp(-1.0 / (releaseMs * 0.001 * effectiveRate));
    const double thresholdD = static_cast<double>(threshold);

    // Gain smoothing filter coefficient to prevent control signal aliasing
    const double gainSmoothCutoff = 20000.0;
    const double gainSmoothCoeff = std::exp(-2.0 * juce::MathConstants<double>::pi * gainSmoothCutoff / effectiveRate);

    // MODE 0: Zero Latency - NO oversampling, NO lookahead, direct processing
    // With channel linking support
    if (processingMode == 0)
    {
        const int numChannels = juce::jmin(buffer.getNumChannels(), 2);
        const int numSamples = buffer.getNumSamples();

        for (int i = 0; i < numSamples; ++i)
        {
            // === PHASE 1: Detect envelope on all channels ===
            double channelEnvelope[2] = {0.0, 0.0};

            for (int ch = 0; ch < numChannels; ++ch)
            {
                auto* data = buffer.getWritePointer(ch);
                double& envelope = fastLimiterState[ch].envelope;

                const double currentSample = static_cast<double>(data[i]);
                const double inputAbs = std::abs(currentSample);

                // Envelope follower with attack/release
                if (inputAbs > envelope)
                    envelope = attackCoeff * envelope + (1.0 - attackCoeff) * inputAbs;
                else
                    envelope = releaseCoeff * envelope + (1.0 - releaseCoeff) * inputAbs;

                envelope = juce::jlimit(0.0, 10.0, envelope);
                channelEnvelope[ch] = envelope;
            }

            // === PHASE 2: Compute linked envelope ===
            double maxEnvelope = 0.0;
            if (isStereo && numChannels >= 2)
            {
                maxEnvelope = juce::jmax(channelEnvelope[0], channelEnvelope[1]);
            }

            // === PHASE 3: Calculate and apply gain reduction ===
            for (int ch = 0; ch < numChannels; ++ch)
            {
                auto* data = buffer.getWritePointer(ch);
                const double currentSample = static_cast<double>(data[i]);

                // Blend between channel's own envelope and max envelope
                double detectionEnvelope = channelEnvelope[ch];
                if (isStereo && channelLink > 0.0f)
                {
                    detectionEnvelope = channelEnvelope[ch] + static_cast<double>(channelLink) * (maxEnvelope - channelEnvelope[ch]);
                }

                // Calculate gain reduction - hard knee
                double targetGain = 1.0;
                if (detectionEnvelope > thresholdD)
                {
                    targetGain = thresholdD / detectionEnvelope;
                }
                targetGain = juce::jlimit(0.01, 1.0, targetGain);

                // Apply gain directly (MODE 0: no smoothing needed)
                const double limitedSample = currentSample * targetGain;
                data[i] = static_cast<SampleType>(limitedSample);
            }
        }
        return;
    }

    // MODE 1 & 2: Architecture A - Process with lookahead (buffer already at OS rate)
    // With channel linking support
    {
        const int numChannels = juce::jmin(buffer.getNumChannels(), 2);
        const int numSamples = buffer.getNumSamples();

        for (int i = 0; i < numSamples; ++i)
        {
            // === PHASE 1: Handle lookahead and detect envelope ===
            double channelEnvelope[2] = {0.0, 0.0};
            double delayedSamples[2] = {0.0, 0.0};

            for (int ch = 0; ch < numChannels; ++ch)
            {
                auto* data = buffer.getWritePointer(ch);
                double& envelope = fastLimiterState[ch].envelope;
                auto& lookaheadBuffer = fastLimiterState[ch].lookaheadBuffer;
                int& writePos = fastLimiterState[ch].lookaheadWritePos;

                const double currentSample = static_cast<double>(data[i]);

                // Store current sample in lookahead buffer
                if (writePos >= 0 && writePos < static_cast<int>(lookaheadBuffer.size()))
                    lookaheadBuffer[writePos] = currentSample;

                // Read delayed sample (3ms ago)
                int readPos = (writePos + 1) % lookaheadSamples;
                delayedSamples[ch] = currentSample;
                if (readPos >= 0 && readPos < static_cast<int>(lookaheadBuffer.size()))
                    delayedSamples[ch] = lookaheadBuffer[readPos];

                writePos = (writePos + 1) % lookaheadSamples;

                const double inputAbs = std::abs(currentSample);

                // Envelope follower
                if (inputAbs > envelope)
                    envelope = attackCoeff * envelope + (1.0 - attackCoeff) * inputAbs;
                else
                    envelope = releaseCoeff * envelope + (1.0 - releaseCoeff) * inputAbs;

                envelope = juce::jlimit(0.0, 10.0, envelope);
                channelEnvelope[ch] = envelope;
            }

            // === PHASE 2: Compute linked envelope ===
            double maxEnvelope = 0.0;
            if (isStereo && numChannels >= 2)
            {
                maxEnvelope = juce::jmax(channelEnvelope[0], channelEnvelope[1]);
            }

            // === PHASE 3: Calculate and apply gain reduction ===
            for (int ch = 0; ch < numChannels; ++ch)
            {
                auto* data = buffer.getWritePointer(ch);
                double& smoothedGain = fastLimiterState[ch].smoothedGain;

                // Blend between channel's own envelope and max envelope
                double detectionEnvelope = channelEnvelope[ch];
                if (isStereo && channelLink > 0.0f)
                {
                    detectionEnvelope = channelEnvelope[ch] + static_cast<double>(channelLink) * (maxEnvelope - channelEnvelope[ch]);
                }

                // Calculate gain reduction - hard knee
                double targetGain = 1.0;
                if (detectionEnvelope > thresholdD)
                {
                    targetGain = thresholdD / detectionEnvelope;
                }
                targetGain = juce::jlimit(0.01, 1.0, targetGain);

                // Apply gain smoothing
                smoothedGain = gainSmoothCoeff * smoothedGain + (1.0 - gainSmoothCoeff) * targetGain;

                const double limitedSample = delayedSamples[ch] * smoothedGain;
                data[i] = static_cast<SampleType>(limitedSample);
            }
        }
    }
}

// Overshoot Suppression - True-Peak Safe Micro-Limiter/Clipper
// Zero-latency, 8× oversampling with linear-phase FIR, removes only 0.1-0.5 dB overshoot
// Signal below ceiling is completely untouched (transparent passthrough)
template<typename SampleType>
void QuadBlendDriveAudioProcessor::processOvershootSuppression(juce::AudioBuffer<SampleType>& buffer, SampleType ceilingDB, double sampleRate, bool enabled, juce::AudioBuffer<SampleType>* referenceBuffer)
{
    // Early exit if disabled - complete passthrough with zero processing
    if (!enabled)
        return;

    // Convert ceiling to linear
    const double ceilingLinear = std::pow(10.0, static_cast<double>(ceilingDB) / 20.0);

    // Overshoot threshold - only process signal exceeding this (0.1-0.5 dB below ceiling)
    const double overshootMarginDB = 0.3;  // Process only the last 0.3 dB
    const double overshootThreshold = ceilingLinear * std::pow(10.0, -overshootMarginDB / 20.0);

    // Get current processing mode from osManager (consistent with Architecture A)
    const int processingMode = osManager.getProcessingMode();

    // MODE 0 (Zero Latency): NO oversampling for protection (flat frequency response)
    // MODE 1/2: Use pre-allocated 2-channel oversamplers (NO allocation on audio thread!)
    const bool useOversampling = (processingMode != 0);
    juce::dsp::Oversampling<SampleType>* oversamplingPtr = nullptr;

    if (useOversampling)
    {
        if (std::is_same<SampleType, float>::value)
        {
            if (processingMode == 1)
                oversamplingPtr = reinterpret_cast<juce::dsp::Oversampling<SampleType>*>(oversampling2ChBalancedFloat.get());
            else  // processingMode == 2
                oversamplingPtr = reinterpret_cast<juce::dsp::Oversampling<SampleType>*>(oversampling2ChLinearFloat.get());
        }
        else
        {
            if (processingMode == 1)
                oversamplingPtr = reinterpret_cast<juce::dsp::Oversampling<SampleType>*>(oversampling2ChBalancedDouble.get());
            else  // processingMode == 2
                oversamplingPtr = reinterpret_cast<juce::dsp::Oversampling<SampleType>*>(oversampling2ChLinearDouble.get());
        }
    }

    // If reference buffer provided, process both through oversampling together for perfect alignment
    if (referenceBuffer != nullptr && useOversampling)
    {
        // Use pre-allocated buffer (NO allocation on audio thread)
        auto& combinedBuffer = std::is_same_v<SampleType, float>
            ? reinterpret_cast<juce::AudioBuffer<SampleType>&>(protectionDeltaCombined4ChFloat)
            : reinterpret_cast<juce::AudioBuffer<SampleType>&>(protectionDeltaCombined4ChDouble);
        combinedBuffer.setSize(4, buffer.getNumSamples(), false, false, true);

        // Copy main buffer to first 2 channels
        for (int ch = 0; ch < 2; ++ch)
            combinedBuffer.copyFrom(ch, 0, buffer, ch, 0, buffer.getNumSamples());

        // Copy reference buffer to last 2 channels
        for (int ch = 0; ch < 2; ++ch)
            combinedBuffer.copyFrom(ch + 2, 0, *referenceBuffer, ch, 0, buffer.getNumSamples());

        // Select delta oversampler matching current processing mode (NO allocation on audio thread!)
        // Mode 1 (Balanced): 8×, Mode 2 (Linear Phase): 16×
        // This ensures perfect phase alignment between main and reference at the correct rate
        auto* oversamplingDelta = [&]() -> juce::dsp::Oversampling<SampleType>* {
            if (std::is_same_v<SampleType, float>)
            {
                return (processingMode == 2)
                    ? reinterpret_cast<juce::dsp::Oversampling<SampleType>*>(oversampling4ChDelta16xFloat.get())
                    : reinterpret_cast<juce::dsp::Oversampling<SampleType>*>(oversampling4ChDeltaFloat.get());
            }
            else
            {
                return (processingMode == 2)
                    ? reinterpret_cast<juce::dsp::Oversampling<SampleType>*>(oversampling4ChDelta16xDouble.get())
                    : reinterpret_cast<juce::dsp::Oversampling<SampleType>*>(oversampling4ChDeltaDouble.get());
            }
        }();

        // Process combined buffer through oversampling
        juce::dsp::AudioBlock<SampleType> combinedBlock(combinedBuffer);
        auto oversampledCombined = oversamplingDelta->processSamplesUp(combinedBlock);

        // Apply parameter-interpolated processor ONLY to first 2 channels (main signal)
        // Fixed blend for simplified design (0 = clean/gentle, 1 = punchy/aggressive)
        // Using 0.3 = slightly transparent character
        const double blend = 0.3;  // 0 = Clean, 1 = Punchy

        // === PARAMETER PRESETS ===
        // Punchy (A): Controlled dirt, punch, slight grunge on transients
        constexpr double kneeWidth_A = 0.005;     // 0.5% of ceiling (0.3-0.6% range, tight)
        constexpr double reduction_A = 0.5;       // 50% overshoot reduction (0.45-0.55 range, dirt control)
        constexpr double attackCoeff_A = 0.03;    // ~0.1ms attack at 8× (0.05-0.15ms range)
        constexpr double releaseCoeff_A = 0.001;  // ~20ms release at 8× (15-30ms range)

        // Clean (B): Maximum transparency, minimal audible dirt, invisible overshoot correction
        constexpr double kneeWidth_B = 0.055;     // 5.5% of ceiling (4-7% range, wide, transparent)
        constexpr double reduction_B = 0.9;       // 90% overshoot reduction (0.85-0.95 range, gentle)
        constexpr double attackCoeff_B = 0.002;   // ~1.4ms attack at 8× (0.8-2.0ms range)
        constexpr double releaseCoeff_B = 0.0002; // ~140ms release at 8× (80-200ms range)

        // === INTERPOLATE PARAMETERS ===
        // Linear interpolation for all shaping parameters
        const double kneeWidthNormalized = kneeWidth_B + (kneeWidth_A - kneeWidth_B) * blend;
        const double reductionFactor = reduction_B + (reduction_A - reduction_B) * blend;
        const double attackCoeff = attackCoeff_B + (attackCoeff_A - attackCoeff_B) * blend;
        const double releaseCoeff = releaseCoeff_B + (releaseCoeff_A - releaseCoeff_B) * blend;

        // Scale knee width by ceiling
        const double kneeWidth = ceilingLinear * kneeWidthNormalized;

        for (size_t ch = 0; ch < 2; ++ch)
        {
            auto* data = oversampledCombined.getChannelPointer(ch);
            const size_t numSamples = oversampledCombined.getNumSamples();

            // Envelope follower state (attack/release smoothing)
            double envelopeState = 0.0;

            for (size_t i = 0; i < numSamples; ++i)
            {
                const double sample = static_cast<double>(data[i]);
                const double absSample = std::abs(sample);

                if (absSample > overshootThreshold && absSample > ceilingLinear)
                {
                    const double overshoot = absSample - ceilingLinear;

                    // === ATTACK/RELEASE ENVELOPE FOLLOWER ===
                    // Use attack coefficient when signal is rising, release when falling
                    const double targetCoeff = (overshoot > envelopeState) ? attackCoeff : releaseCoeff;
                    envelopeState = envelopeState * (1.0 - targetCoeff) + overshoot * targetCoeff;

                    // === SINGLE UNIFIED TRANSFER CURVE ===
                    // Smooth soft-knee compression using tanh with interpolated parameters
                    const double compressed = std::tanh(envelopeState / kneeWidth) * kneeWidth;

                    // Apply interpolated overshoot reduction
                    const double target = ceilingLinear + compressed * reductionFactor;

                    // Output is the limited signal
                    data[i] = static_cast<SampleType>(sample >= 0.0 ? target : -target);
                }
                else if (absSample > overshootThreshold)
                {
                    // In the threshold zone but not exceeding ceiling - still untouched
                    // Release envelope state smoothly
                    envelopeState *= (1.0 - releaseCoeff);
                }
                else
                {
                    envelopeState = 0.0;
                }
            }
        }
        // Channels 2-3 (reference) pass through unchanged

        // Downsample together
        oversamplingDelta->processSamplesDown(combinedBlock);

        // Copy results back
        for (int ch = 0; ch < 2; ++ch)
            buffer.copyFrom(ch, 0, combinedBuffer, ch, 0, buffer.getNumSamples());

        for (int ch = 0; ch < 2; ++ch)
            referenceBuffer->copyFrom(ch, 0, combinedBuffer, ch + 2, 0, buffer.getNumSamples());

        return;
    }

    // Simple OSM - single processor with interpolated parameters
    // Fixed blend for simplified design (0 = clean/gentle, 1 = punchy/aggressive)
    // Using 0.3 = slightly transparent character
    const double blend = 0.3;  // 0 = Clean, 1 = Punchy

    // === PARAMETER PRESETS ===
    // Punchy (A): Controlled dirt, punch, slight grunge on transients
    constexpr double kneeWidth_A = 0.005;     // 0.5% of ceiling (0.3-0.6% range, tight)
    constexpr double reduction_A = 0.5;       // 50% overshoot reduction (0.45-0.55 range, dirt control)
    constexpr double attackCoeff_A = 0.03;    // ~0.1ms attack at 8× (0.05-0.15ms range)
    constexpr double releaseCoeff_A = 0.001;  // ~20ms release at 8× (15-30ms range)

    // Clean (B): Maximum transparency, minimal audible dirt, invisible overshoot correction
    constexpr double kneeWidth_B = 0.055;     // 5.5% of ceiling (4-7% range, wide, transparent)
    constexpr double reduction_B = 0.9;       // 90% overshoot reduction (0.85-0.95 range, gentle)
    constexpr double attackCoeff_B = 0.002;   // ~1.4ms attack at 8× (0.8-2.0ms range)
    constexpr double releaseCoeff_B = 0.0002; // ~140ms release at 8× (80-200ms range)

    // === INTERPOLATE PARAMETERS ===
    // Linear interpolation for all shaping parameters
    const double kneeWidthNormalized = kneeWidth_B + (kneeWidth_A - kneeWidth_B) * blend;
    const double reductionFactor = reduction_B + (reduction_A - reduction_B) * blend;
    const double attackCoeff = attackCoeff_B + (attackCoeff_A - attackCoeff_B) * blend;
    const double releaseCoeff = releaseCoeff_B + (releaseCoeff_A - releaseCoeff_B) * blend;

    // Scale knee width by ceiling
    const double kneeWidth = ceilingLinear * kneeWidthNormalized;

    // MODE 0 (Zero Latency): Process directly without oversampling
    // MODE 1/2: Use oversampling for true-peak protection
    juce::dsp::AudioBlock<SampleType> block(buffer);
    juce::dsp::AudioBlock<SampleType> processingBlock = useOversampling
        ? oversamplingPtr->processSamplesUp(block)
        : block;

    // Process signal - single unified transfer curve with interpolated parameters
    for (size_t ch = 0; ch < processingBlock.getNumChannels(); ++ch)
    {
        auto* data = processingBlock.getChannelPointer(ch);
        const size_t numSamples = processingBlock.getNumSamples();

        // Envelope follower state (attack/release smoothing)
        double envelopeState = 0.0;

        for (size_t i = 0; i < numSamples; ++i)
        {
            const double sample = static_cast<double>(data[i]);
            const double absSample = std::abs(sample);

            // Only process samples exceeding overshoot threshold (transparent below)
            if (absSample > overshootThreshold && absSample > ceilingLinear)
            {
                const double overshoot = absSample - ceilingLinear;

                // === ATTACK/RELEASE ENVELOPE FOLLOWER ===
                // Use attack coefficient when signal is rising, release when falling
                const double targetCoeff = (overshoot > envelopeState) ? attackCoeff : releaseCoeff;
                envelopeState = envelopeState * (1.0 - targetCoeff) + overshoot * targetCoeff;

                // === SINGLE UNIFIED TRANSFER CURVE ===
                // Smooth soft-knee compression using tanh with interpolated parameters
                const double compressed = std::tanh(envelopeState / kneeWidth) * kneeWidth;

                // Apply interpolated overshoot reduction
                const double target = ceilingLinear + compressed * reductionFactor;

                // Output is the limited signal
                data[i] = static_cast<SampleType>(sample >= 0.0 ? target : -target);
            }
            else if (absSample > overshootThreshold)
            {
                // In the threshold zone but not exceeding ceiling - still untouched
                // Release envelope state smoothly
                envelopeState *= (1.0 - releaseCoeff);
            }
            else
            {
                // Samples below overshoot threshold are completely untouched (bit-perfect passthrough)
                envelopeState = 0.0;
            }
        }
    }

    // Downsample back to original rate (only if we upsampled)
    if (useOversampling)
        oversamplingPtr->processSamplesDown(block);

    // Final safety clamp to ensure ceiling compliance
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            data[i] = juce::jlimit(static_cast<SampleType>(-ceilingLinear),
                                   static_cast<SampleType>(ceilingLinear),
                                   data[i]);
        }
    }
}

//==============================================================================
// Advanced True Peak Limiter with IRC (Intelligent Release Control)
//==============================================================================
template<typename SampleType>
void QuadBlendDriveAudioProcessor::processAdvancedTPL(juce::AudioBuffer<SampleType>& buffer, SampleType ceilingDB, double sampleRate, juce::AudioBuffer<SampleType>* referenceBuffer)
{
    // ========================================================================
    // ITU-R BS.1770-4 COMPLIANT TRUE PEAK LIMITER
    // ========================================================================
    // This limiter prevents inter-sample peaks (true peaks) from exceeding
    // the specified ceiling, ensuring broadcast/streaming compliance.
    //
    // COMPLIANCE VERIFICATION:
    // ✓ Inter-Sample Peak Detection: Uses 8-16x oversampling to detect peaks
    //   that occur between samples in the continuous-time waveform
    // ✓ FIR Filtering: JUCE's Oversampling uses filterHalfBandFIREquiripple
    //   for accurate bandlimited interpolation
    // ✓ Predictive Lookahead: 2ms lookahead buffer enables transparent limiting
    //   without overshoots
    // ✓ Sample-Accurate Processing: All gain reduction applied at oversampled
    //   rate before downsampling
    // ✓ Final Safety Clamp: Guarantees no sample exceeds ceiling after downsampling
    //
    // PROCESSING MODES:
    // - Mode 0 (Zero Latency): Direct limiting, no oversampling (non-compliant)
    // - Mode 1 (Balanced): 8x oversampling, ~32 samples latency (ITU-R compliant)
    // - Mode 2 (Linear Phase): 16x oversampling, ~128 samples latency (ITU-R compliant)
    //
    // DEFAULT: Mode 1 (Balanced) - Recommended for broadcast/streaming
    // ========================================================================

    // Convert ceiling to linear
    const double ceilingLinear = std::pow(10.0, static_cast<double>(ceilingDB) / 20.0);

    // Get current processing mode from osManager (consistent with Architecture A)
    const int processingMode = osManager.getProcessingMode();

    // MODE 0 (Zero Latency): NO oversampling for protection (flat frequency response)
    // MODE 1/2: Use pre-allocated 2-channel oversamplers (NO allocation on audio thread!)
    const bool useOversampling = (processingMode != 0);
    juce::dsp::Oversampling<SampleType>* oversamplingPtr = nullptr;

    if (useOversampling)
    {
        if (std::is_same<SampleType, float>::value)
        {
            if (processingMode == 1)
                oversamplingPtr = reinterpret_cast<juce::dsp::Oversampling<SampleType>*>(oversampling2ChBalancedFloat.get());
            else  // processingMode == 2
                oversamplingPtr = reinterpret_cast<juce::dsp::Oversampling<SampleType>*>(oversampling2ChLinearFloat.get());
        }
        else
        {
            if (processingMode == 1)
                oversamplingPtr = reinterpret_cast<juce::dsp::Oversampling<SampleType>*>(oversampling2ChBalancedDouble.get());
            else  // processingMode == 2
                oversamplingPtr = reinterpret_cast<juce::dsp::Oversampling<SampleType>*>(oversampling2ChLinearDouble.get());
        }
    }

    // Upsample for true-peak detection (or process directly in Mode 0)
    juce::dsp::AudioBlock<SampleType> block(buffer);
    juce::dsp::AudioBlock<SampleType> processingBlock = useOversampling
        ? oversamplingPtr->processSamplesUp(block)
        : block;

    const size_t oversampledSamples = processingBlock.getNumSamples();
    const int lookahead = advancedTPLLookaheadSamples;

    // Guard against division by zero
    if (lookahead <= 0)
    {
        // Fallback to simple hard limiting
        for (size_t ch = 0; ch < processingBlock.getNumChannels(); ++ch)
        {
            auto* data = processingBlock.getChannelPointer(ch);
            for (size_t i = 0; i < oversampledSamples; ++i)
            {
                data[i] = juce::jlimit(static_cast<SampleType>(-ceilingLinear),
                                       static_cast<SampleType>(ceilingLinear), data[i]);
            }
        }
        if (useOversampling)
            oversamplingPtr->processSamplesDown(block);
        return;
    }

    // Soft-knee parameters (designed for up to 6 dB GR)
    constexpr double kneeWidthDB = 1.5;  // Soft knee width in dB
    const double kneeWidth = ceilingLinear * (std::pow(10.0, -kneeWidthDB / 20.0) - 1.0);

    // Attack/Release parameters (at 8× sample rate)
    constexpr double attackCoeff = 0.9;     // Very fast attack for peak catching
    constexpr double fastReleaseCoeff = 0.0001;  // ~20ms fast release
    constexpr double slowReleaseCoeff = 0.00005; // ~80ms slow release

    // Process each channel
    for (size_t ch = 0; ch < processingBlock.getNumChannels(); ++ch)
    {
        auto* data = processingBlock.getChannelPointer(ch);
        auto& state = advancedTPLState[ch];

        for (size_t i = 0; i < oversampledSamples; ++i)
        {
            const double inputSample = static_cast<double>(data[i]);

            // Store in lookahead buffer
            state.lookaheadBuffer[state.lookaheadWritePos] = inputSample;
            state.lookaheadWritePos = (state.lookaheadWritePos + 1) % lookahead;

            // Read delayed sample from lookahead buffer
            const double delayedSample = state.lookaheadBuffer[state.lookaheadWritePos];
            const double absDelayed = std::abs(delayedSample);

            // === MULTIBAND ANALYSIS FOR IRC ===
            // Apply multiband filters to detect energy in 3 frequency bands
            const double inSample = inputSample;

            // Low band (<200 Hz) - 2-pole lowpass
            const double lowOut = state.lowB0 * inSample + state.lowB1 * state.lowZ1 + state.lowB2 * state.lowZ2
                                - state.lowA1 * state.lowZ1 - state.lowA2 * state.lowZ2;
            state.lowZ2 = state.lowZ1;
            state.lowZ1 = inSample;

            // Mid band (200-4000 Hz) - 2-pole bandpass
            const double midOut = state.midB0 * inSample + state.midB1 * state.midZ1 + state.midB2 * state.midZ2
                                - state.midA1 * state.midZ1 - state.midA2 * state.midZ2;
            state.midZ2 = state.midZ1;
            state.midZ1 = inSample;

            // High band (>4000 Hz) - 2-pole highpass
            const double highOut = state.highB0 * inSample + state.highB1 * state.highZ1 + state.highB2 * state.highZ2
                                 - state.highA1 * state.highZ1 - state.highA2 * state.highZ2;
            state.highZ2 = state.highZ1;
            state.highZ1 = inSample;

            // Envelope followers for each band (RMS-style)
            constexpr double bandEnvCoeff = 0.001;
            state.lowBandEnv = state.lowBandEnv * (1.0 - bandEnvCoeff) + std::abs(lowOut) * bandEnvCoeff;
            state.midBandEnv = state.midBandEnv * (1.0 - bandEnvCoeff) + std::abs(midOut) * bandEnvCoeff;
            state.highBandEnv = state.highBandEnv * (1.0 - bandEnvCoeff) + std::abs(highOut) * bandEnvCoeff;

            // === INTELLIGENT RELEASE CONTROL ===
            // Calculate release time based on frequency content
            // High frequencies → fast release (preserve transients)
            // Low frequencies → slow release (prevent pumping)
            const double totalEnergy = state.lowBandEnv + state.midBandEnv + state.highBandEnv + 1e-10;
            const double lowRatio = state.lowBandEnv / totalEnergy;
            const double highRatio = state.highBandEnv / totalEnergy;

            // Blend between fast and slow release based on frequency content
            // More low freq → slower release, more high freq → faster release
            const double releaseCoeff = slowReleaseCoeff + (fastReleaseCoeff - slowReleaseCoeff) * highRatio;
            state.currentReleaseCoeff = releaseCoeff;

            // === PEAK DETECTION WITH SOFT KNEE ===
            double targetGain = 1.0;

            if (absDelayed > ceilingLinear - kneeWidth)
            {
                if (absDelayed > ceilingLinear)
                {
                    // Hard limiting region (above ceiling)
                    const double overshoot = absDelayed - ceilingLinear;
                    const double compressed = std::tanh(overshoot / (kneeWidth * 0.5)) * (kneeWidth * 0.5);
                    const double target = ceilingLinear + compressed * 0.3;  // Allow 30% of overshoot through
                    targetGain = target / absDelayed;
                }
                else
                {
                    // Soft-knee region (below ceiling but within knee)
                    const double kneeInput = (absDelayed - (ceilingLinear - kneeWidth)) / kneeWidth;
                    const double kneeFactor = 1.0 - (kneeInput * kneeInput * 0.5);  // Smooth quadratic knee
                    targetGain = (ceilingLinear - kneeWidth * (1.0 - kneeFactor)) / absDelayed;
                }
            }

            // === ENVELOPE FOLLOWER (Attack/Release) ===
            const double envCoeff = (targetGain < state.grEnvelope) ? attackCoeff : releaseCoeff;
            state.grEnvelope = state.grEnvelope * (1.0 - envCoeff) + targetGain * envCoeff;

            // Clamp GR envelope to prevent excessive reduction (6 dB max = 0.5 gain)
            state.grEnvelope = std::max(0.5, state.grEnvelope);

            // Apply gain reduction
            data[i] = static_cast<SampleType>(delayedSample * state.grEnvelope);
        }
    }

    // Downsample back to original rate (only if we upsampled)
    if (useOversampling)
        oversamplingPtr->processSamplesDown(block);

    // Final safety clamp to ensure 0.0 dBFS compliance
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            data[i] = juce::jlimit(static_cast<SampleType>(-ceilingLinear),
                                   static_cast<SampleType>(ceilingLinear),
                                   data[i]);
        }
    }
}

//==============================================================================
// COMBINED LIMITERS: Process both Overshoot and True Peak in single oversample cycle
// This avoids double oversampling artifacts when both limiters are enabled
template<typename SampleType>
void QuadBlendDriveAudioProcessor::processCombinedLimiters(juce::AudioBuffer<SampleType>& buffer, SampleType ceilingDB, double sampleRate)
{
    // ========================================================================
    // COMBINED OVERSHOOT + TRUE PEAK LIMITER
    // ========================================================================
    // When both limiters are enabled, process them in a single oversample cycle:
    // 1. Upsample ONCE
    // 2. Apply Overshoot Suppression (character shaping)
    // 3. Apply True Peak Limiting (compliance guarantee)
    // 4. Downsample ONCE
    //
    // This eliminates the double oversampling artifacts that occur when
    // processing each limiter separately with its own up/down cycle.
    // ========================================================================

    const double ceilingLinear = std::pow(10.0, static_cast<double>(ceilingDB) / 20.0);
    const int processingMode = osManager.getProcessingMode();
    const bool useOversampling = (processingMode != 0);

    // Get oversampling pointer based on mode
    juce::dsp::Oversampling<SampleType>* oversamplingPtr = nullptr;
    if (useOversampling)
    {
        if (std::is_same<SampleType, float>::value)
        {
            if (processingMode == 1)
                oversamplingPtr = reinterpret_cast<juce::dsp::Oversampling<SampleType>*>(oversampling2ChBalancedFloat.get());
            else  // processingMode == 2
                oversamplingPtr = reinterpret_cast<juce::dsp::Oversampling<SampleType>*>(oversampling2ChLinearFloat.get());
        }
        else
        {
            if (processingMode == 1)
                oversamplingPtr = reinterpret_cast<juce::dsp::Oversampling<SampleType>*>(oversampling2ChBalancedDouble.get());
            else  // processingMode == 2
                oversamplingPtr = reinterpret_cast<juce::dsp::Oversampling<SampleType>*>(oversampling2ChLinearDouble.get());
        }
    }

    // Upsample ONCE (or process directly in Mode 0)
    juce::dsp::AudioBlock<SampleType> block(buffer);
    juce::dsp::AudioBlock<SampleType> processingBlock = useOversampling
        ? oversamplingPtr->processSamplesUp(block)
        : block;

    const size_t oversampledSamples = processingBlock.getNumSamples();

    // === STAGE 1: OVERSHOOT SUPPRESSION (Character Shaping) ===
    {
        // Overshoot parameters (using fixed blend of 0.3 = slightly transparent)
        const double blend = 0.3;

        constexpr double kneeWidth_A = 0.005;
        constexpr double reduction_A = 0.5;
        constexpr double attackCoeff_A = 0.03;
        constexpr double releaseCoeff_A = 0.001;

        constexpr double kneeWidth_B = 0.055;
        constexpr double reduction_B = 0.9;
        constexpr double attackCoeff_B = 0.002;
        constexpr double releaseCoeff_B = 0.0002;

        const double kneeWidthNormalized = kneeWidth_B + (kneeWidth_A - kneeWidth_B) * blend;
        const double reductionFactor = reduction_B + (reduction_A - reduction_B) * blend;
        const double attackCoeff = attackCoeff_B + (attackCoeff_A - attackCoeff_B) * blend;
        const double releaseCoeff = releaseCoeff_B + (releaseCoeff_A - releaseCoeff_B) * blend;
        const double kneeWidth = ceilingLinear * kneeWidthNormalized;

        const double overshootMarginDB = 0.3;
        const double overshootThreshold = ceilingLinear * std::pow(10.0, -overshootMarginDB / 20.0);

        for (size_t ch = 0; ch < processingBlock.getNumChannels(); ++ch)
        {
            auto* data = processingBlock.getChannelPointer(ch);
            double envelopeState = 0.0;

            for (size_t i = 0; i < oversampledSamples; ++i)
            {
                const double sample = static_cast<double>(data[i]);
                const double absSample = std::abs(sample);

                if (absSample > overshootThreshold && absSample > ceilingLinear)
                {
                    const double overshoot = absSample - ceilingLinear;
                    const double targetCoeff = (overshoot > envelopeState) ? attackCoeff : releaseCoeff;
                    envelopeState = envelopeState * (1.0 - targetCoeff) + overshoot * targetCoeff;
                    const double compressed = std::tanh(envelopeState / kneeWidth) * kneeWidth;
                    const double target = ceilingLinear + compressed * reductionFactor;
                    data[i] = static_cast<SampleType>(sample >= 0.0 ? target : -target);
                }
                else if (absSample > overshootThreshold)
                {
                    envelopeState *= (1.0 - releaseCoeff);
                }
                else
                {
                    envelopeState = 0.0;
                }
            }
        }
    }

    // === STAGE 2: TRUE PEAK LIMITER (ITU-R BS.1770-4 Compliance) ===
    {
        const int lookahead = advancedTPLLookaheadSamples;

        if (lookahead > 0)
        {
            constexpr double kneeWidthDB = 1.5;
            const double kneeWidth = ceilingLinear * (std::pow(10.0, -kneeWidthDB / 20.0) - 1.0);
            constexpr double attackCoeff = 0.9;
            constexpr double fastReleaseCoeff = 0.0001;
            constexpr double slowReleaseCoeff = 0.00005;

            for (size_t ch = 0; ch < processingBlock.getNumChannels(); ++ch)
            {
                auto* data = processingBlock.getChannelPointer(ch);
                auto& state = advancedTPLState[ch];

                for (size_t i = 0; i < oversampledSamples; ++i)
                {
                    const double inputSample = static_cast<double>(data[i]);

                    // Store in lookahead buffer
                    state.lookaheadBuffer[state.lookaheadWritePos] = inputSample;
                    state.lookaheadWritePos = (state.lookaheadWritePos + 1) % lookahead;

                    // Read delayed sample
                    const double delayedSample = state.lookaheadBuffer[state.lookaheadWritePos];
                    const double absDelayed = std::abs(delayedSample);

                    // Multiband analysis for intelligent release
                    const double inSample = inputSample;

                    const double lowOut = state.lowB0 * inSample + state.lowB1 * state.lowZ1 + state.lowB2 * state.lowZ2
                                        - state.lowA1 * state.lowZ1 - state.lowA2 * state.lowZ2;
                    state.lowZ2 = state.lowZ1;
                    state.lowZ1 = inSample;

                    const double midOut = state.midB0 * inSample + state.midB1 * state.midZ1 + state.midB2 * state.midZ2
                                        - state.midA1 * state.midZ1 - state.midA2 * state.midZ2;
                    state.midZ2 = state.midZ1;
                    state.midZ1 = inSample;

                    const double highOut = state.highB0 * inSample + state.highB1 * state.highZ1 + state.highB2 * state.highZ2
                                         - state.highA1 * state.highZ1 - state.highA2 * state.highZ2;
                    state.highZ2 = state.highZ1;
                    state.highZ1 = inSample;

                    constexpr double bandEnvCoeff = 0.001;
                    state.lowBandEnv = state.lowBandEnv * (1.0 - bandEnvCoeff) + std::abs(lowOut) * bandEnvCoeff;
                    state.midBandEnv = state.midBandEnv * (1.0 - bandEnvCoeff) + std::abs(midOut) * bandEnvCoeff;
                    state.highBandEnv = state.highBandEnv * (1.0 - bandEnvCoeff) + std::abs(highOut) * bandEnvCoeff;

                    const double totalEnergy = state.lowBandEnv + state.midBandEnv + state.highBandEnv + 1e-10;
                    const double highRatio = state.highBandEnv / totalEnergy;
                    const double releaseCoeff = slowReleaseCoeff + (fastReleaseCoeff - slowReleaseCoeff) * highRatio;

                    // Peak detection with soft knee
                    double targetGain = 1.0;

                    if (absDelayed > ceilingLinear - kneeWidth)
                    {
                        if (absDelayed > ceilingLinear)
                        {
                            const double overshoot = absDelayed - ceilingLinear;
                            const double compressed = std::tanh(overshoot / (kneeWidth * 0.5)) * (kneeWidth * 0.5);
                            const double target = ceilingLinear + compressed * 0.3;
                            targetGain = target / absDelayed;
                        }
                        else
                        {
                            const double kneeInput = (absDelayed - (ceilingLinear - kneeWidth)) / kneeWidth;
                            const double kneeFactor = 1.0 - (kneeInput * kneeInput * 0.5);
                            targetGain = (ceilingLinear - kneeWidth * (1.0 - kneeFactor)) / absDelayed;
                        }
                    }

                    // Envelope follower
                    const double envCoeff = (targetGain < state.grEnvelope) ? attackCoeff : releaseCoeff;
                    state.grEnvelope = state.grEnvelope * (1.0 - envCoeff) + targetGain * envCoeff;
                    state.grEnvelope = std::max(0.5, state.grEnvelope);

                    // Apply gain reduction
                    data[i] = static_cast<SampleType>(delayedSample * state.grEnvelope);
                }
            }
        }
        else
        {
            // Fallback: simple hard limiting
            for (size_t ch = 0; ch < processingBlock.getNumChannels(); ++ch)
            {
                auto* data = processingBlock.getChannelPointer(ch);
                for (size_t i = 0; i < oversampledSamples; ++i)
                {
                    data[i] = juce::jlimit(static_cast<SampleType>(-ceilingLinear),
                                           static_cast<SampleType>(ceilingLinear), data[i]);
                }
            }
        }
    }

    // Downsample ONCE (only if we upsampled)
    if (useOversampling)
        oversamplingPtr->processSamplesDown(block);

    // Final safety clamp
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            data[i] = juce::jlimit(static_cast<SampleType>(-ceilingLinear),
                                   static_cast<SampleType>(ceilingLinear),
                                   data[i]);
        }
    }
}

// Template instantiations
template void QuadBlendDriveAudioProcessor::processCombinedLimiters<float>(juce::AudioBuffer<float>&, float, double);
template void QuadBlendDriveAudioProcessor::processCombinedLimiters<double>(juce::AudioBuffer<double>&, double, double);

//==============================================================================
// Architecture A: XY Blend Processing (runs entirely in OS domain)
template<typename SampleType>
void QuadBlendDriveAudioProcessor::processXYBlend(juce::AudioBuffer<SampleType>& buffer, double osSampleRate)
{
    const int numSamples = buffer.getNumSamples();  // Already in OS domain

    // Get correct temp buffers for this precision
    auto& tempBuffer1 = std::is_same<SampleType, float>::value ?
                        reinterpret_cast<juce::AudioBuffer<SampleType>&>(tempBuffer1Float) :
                        reinterpret_cast<juce::AudioBuffer<SampleType>&>(tempBuffer1Double);
    auto& tempBuffer2 = std::is_same<SampleType, float>::value ?
                        reinterpret_cast<juce::AudioBuffer<SampleType>&>(tempBuffer2Float) :
                        reinterpret_cast<juce::AudioBuffer<SampleType>&>(tempBuffer2Double);
    auto& tempBuffer3 = std::is_same<SampleType, float>::value ?
                        reinterpret_cast<juce::AudioBuffer<SampleType>&>(tempBuffer3Float) :
                        reinterpret_cast<juce::AudioBuffer<SampleType>&>(tempBuffer3Double);
    auto& tempBuffer4 = std::is_same<SampleType, float>::value ?
                        reinterpret_cast<juce::AudioBuffer<SampleType>&>(tempBuffer4Float) :
                        reinterpret_cast<juce::AudioBuffer<SampleType>&>(tempBuffer4Double);

    // Ensure temp buffers are sized for OS domain
    tempBuffer1.setSize(2, numSamples, false, false, true);
    tempBuffer2.setSize(2, numSamples, false, false, true);
    tempBuffer3.setSize(2, numSamples, false, false, true);
    tempBuffer4.setSize(2, numSamples, false, false, true);

    // Read parameters (same as before, but at OS rate)
    const SampleType xyX = static_cast<SampleType>(apvts.getRawParameterValue("XY_X_PARAM")->load());
    const SampleType xyY = static_cast<SampleType>(apvts.getRawParameterValue("XY_Y_PARAM")->load());
    const SampleType thresholdDB = static_cast<SampleType>(apvts.getRawParameterValue("THRESHOLD")->load());
    const SampleType scKneePercent = static_cast<SampleType>(apvts.getRawParameterValue("SC_KNEE")->load());
    const SampleType limitRelMs = static_cast<SampleType>(apvts.getRawParameterValue("LIMIT_REL")->load());
    const SampleType slAttackMs = static_cast<SampleType>(apvts.getRawParameterValue("SL_LIMIT_ATTACK")->load());
    const SampleType flAttackMs = static_cast<SampleType>(apvts.getRawParameterValue("FL_LIMIT_ATTACK")->load());

    const SampleType hcTrimDB = static_cast<SampleType>(apvts.getRawParameterValue("HC_TRIM")->load());
    const SampleType scTrimDB = static_cast<SampleType>(apvts.getRawParameterValue("SC_TRIM")->load());
    const SampleType slTrimDB = static_cast<SampleType>(apvts.getRawParameterValue("SL_TRIM")->load());
    const SampleType flTrimDB = static_cast<SampleType>(apvts.getRawParameterValue("FL_TRIM")->load());

    // === ENVELOPE SHAPING PARAMETERS ===
    // Read attack/sustain emphasis for each drive type
    const float hcAttackDB = apvts.getRawParameterValue("HC_ATTACK")->load();
    const float hcSustainDB = apvts.getRawParameterValue("HC_SUSTAIN")->load();
    const float scAttackDB = apvts.getRawParameterValue("SC_ATTACK")->load();
    const float scSustainDB = apvts.getRawParameterValue("SC_SUSTAIN")->load();
    const float slAttackDB = apvts.getRawParameterValue("SL_ATTACK")->load();
    const float slSustainDB = apvts.getRawParameterValue("SL_SUSTAIN")->load();
    const float flAttackDB = apvts.getRawParameterValue("FL_ATTACK")->load();
    const float flSustainDB = apvts.getRawParameterValue("FL_SUSTAIN")->load();

    // Update envelope shapers with current parameter values
    hardClipShaper.setAttackEmphasis(hcAttackDB);
    hardClipShaper.setSustainEmphasis(hcSustainDB);
    softClipShaper.setAttackEmphasis(scAttackDB);
    softClipShaper.setSustainEmphasis(scSustainDB);
    slowLimitShaper.setAttackEmphasis(slAttackDB);
    slowLimitShaper.setSustainEmphasis(slSustainDB);
    fastLimitShaper.setAttackEmphasis(flAttackDB);
    fastLimitShaper.setSustainEmphasis(flSustainDB);

    // Read mute and solo states
    // Mute takes priority over solo (like an analog console - if it's cut, it's cut)
    const bool hcMuteParam = apvts.getRawParameterValue("HC_MUTE")->load() > 0.5f;
    const bool scMuteParam = apvts.getRawParameterValue("SC_MUTE")->load() > 0.5f;
    const bool slMuteParam = apvts.getRawParameterValue("SL_MUTE")->load() > 0.5f;
    const bool flMuteParam = apvts.getRawParameterValue("FL_MUTE")->load() > 0.5f;

    const bool hcSolo = apvts.getRawParameterValue("HC_SOLO")->load() > 0.5f;
    const bool scSolo = apvts.getRawParameterValue("SC_SOLO")->load() > 0.5f;
    const bool slSolo = apvts.getRawParameterValue("SL_SOLO")->load() > 0.5f;
    const bool flSolo = apvts.getRawParameterValue("FL_SOLO")->load() > 0.5f;

    // Solo logic: if any processor is soloed, mute all non-soloed processors
    // BUT explicit mute always takes priority (mute overrides solo)
    const bool anySolo = hcSolo || scSolo || slSolo || flSolo;
    bool hcMute = hcMuteParam;  // Explicit mute always applies
    bool scMute = scMuteParam;
    bool slMute = slMuteParam;
    bool flMute = flMuteParam;
    if (anySolo)
    {
        // Only mute non-soloed processors that aren't already explicitly muted
        if (!hcSolo && !hcMuteParam) hcMute = true;
        if (!scSolo && !scMuteParam) scMute = true;
        if (!slSolo && !slMuteParam) slMute = true;
        if (!flSolo && !flMuteParam) flMute = true;
    }

    const SampleType threshold = juce::Decibels::decibelsToGain(thresholdDB);
    const SampleType scKnee = scKneePercent / static_cast<SampleType>(100.0);
    const SampleType hcTrimGain = juce::Decibels::decibelsToGain(hcTrimDB);
    const SampleType scTrimGain = juce::Decibels::decibelsToGain(scTrimDB);
    const SampleType slTrimGain = juce::Decibels::decibelsToGain(slTrimDB);
    const SampleType flTrimGain = juce::Decibels::decibelsToGain(flTrimDB);

    // Calculate bi-linear blend weights from XY pad
    // XY Grid Layout: Top-Left=HC, Top-Right=FL, Bottom-Left=SC, Bottom-Right=SL
    SampleType wHC = (static_cast<SampleType>(1.0) - xyX) * xyY;  // Top-left
    SampleType wFL = xyX * xyY;  // Top-right
    SampleType wSC = (static_cast<SampleType>(1.0) - xyX) * (static_cast<SampleType>(1.0) - xyY);  // Bottom-left
    SampleType wSL = xyX * (static_cast<SampleType>(1.0) - xyY);  // Bottom-right

    // Apply mute logic
    if (hcMute) wHC = static_cast<SampleType>(0.0);
    if (scMute) wSC = static_cast<SampleType>(0.0);
    if (slMute) wSL = static_cast<SampleType>(0.0);
    if (flMute) wFL = static_cast<SampleType>(0.0);

    // Renormalize weights
    const SampleType weightSum = wHC + wSC + wSL + wFL;
    const bool allProcessorsMuted = (weightSum <= static_cast<SampleType>(1e-10));

    if (allProcessorsMuted)
        return;  // All processors muted - buffer unchanged (pristine passthrough)

    const SampleType normFactor = static_cast<SampleType>(1.0) / weightSum;
    wHC *= normFactor;
    wSC *= normFactor;
    wSL *= normFactor;
    wFL *= normFactor;

    // Copy input to temp buffers for parallel processing
    tempBuffer1.makeCopyOf(buffer);
    tempBuffer2.makeCopyOf(buffer);
    tempBuffer3.makeCopyOf(buffer);
    tempBuffer4.makeCopyOf(buffer);

    // === ENVELOPE SHAPING: Apply dynamic gain based on transient detection ===
    // Process each buffer with its corresponding envelope shaper
    // NOTE: Envelope detection is MONO (uses left channel for stereo analysis)
    for (int i = 0; i < numSamples; ++i)
    {
        // Use left channel for envelope detection (mono detection, stereo application)
        const float inputSampleL = static_cast<float>(tempBuffer1.getSample(0, i));

        // Calculate envelope-based gains for each drive type
        const float hcEnvGain = hardClipShaper.processEnvelope(inputSampleL);
        const float scEnvGain = softClipShaper.processEnvelope(inputSampleL);
        const float slEnvGain = slowLimitShaper.processEnvelope(inputSampleL);
        const float flEnvGain = fastLimitShaper.processEnvelope(inputSampleL);

        // Apply envelope shaping gain to both channels
        for (int ch = 0; ch < 2; ++ch)
        {
            tempBuffer1.setSample(ch, i, tempBuffer1.getSample(ch, i) * static_cast<SampleType>(hcEnvGain));
            tempBuffer2.setSample(ch, i, tempBuffer2.getSample(ch, i) * static_cast<SampleType>(scEnvGain));
            tempBuffer3.setSample(ch, i, tempBuffer3.getSample(ch, i) * static_cast<SampleType>(slEnvGain));
            tempBuffer4.setSample(ch, i, tempBuffer4.getSample(ch, i) * static_cast<SampleType>(flEnvGain));
        }
    }

    // Process all 4 paths (now at OS rate - no internal oversampling needed)
    // Apply trim gain AFTER envelope shaping
    tempBuffer1.applyGain(hcTrimGain);
    processHardClip(tempBuffer1, threshold, osSampleRate);

    tempBuffer2.applyGain(scTrimGain);
    processSoftClip(tempBuffer2, threshold, scKnee, osSampleRate);

    tempBuffer3.applyGain(slTrimGain);
    processSlowLimit(tempBuffer3, threshold, limitRelMs, slAttackMs, osSampleRate);

    tempBuffer4.applyGain(flTrimGain);
    processFastLimit(tempBuffer4, threshold, flAttackMs, osSampleRate);

    // Apply compensation gains if enabled
    const bool masterCompEnabled = apvts.getRawParameterValue("MASTER_COMP")->load() > 0.5f;
    if (masterCompEnabled)
    {
        const bool hcCompEnabled = apvts.getRawParameterValue("HC_COMP")->load() > 0.5f;
        const bool scCompEnabled = apvts.getRawParameterValue("SC_COMP")->load() > 0.5f;
        const bool slCompEnabled = apvts.getRawParameterValue("SL_COMP")->load() > 0.5f;
        const bool flCompEnabled = apvts.getRawParameterValue("FL_COMP")->load() > 0.5f;

        if (hcCompEnabled && std::abs(hcTrimGain - static_cast<SampleType>(1.0)) > static_cast<SampleType>(1e-6))
            tempBuffer1.applyGain(static_cast<SampleType>(1.0) / hcTrimGain);
        if (scCompEnabled && std::abs(scTrimGain - static_cast<SampleType>(1.0)) > static_cast<SampleType>(1e-6))
            tempBuffer2.applyGain(static_cast<SampleType>(1.0) / scTrimGain);
        if (slCompEnabled && std::abs(slTrimGain - static_cast<SampleType>(1.0)) > static_cast<SampleType>(1e-6))
            tempBuffer3.applyGain(static_cast<SampleType>(1.0) / slTrimGain);
        if (flCompEnabled && std::abs(flTrimGain - static_cast<SampleType>(1.0)) > static_cast<SampleType>(1e-6))
            tempBuffer4.applyGain(static_cast<SampleType>(1.0) / flTrimGain);
    }

    // === WAVEFORM GR METER: CAPTURE PER-PROCESSOR OUTPUTS AND GR ===
    // Store input and per-processor output peaks for waveform visualization
    // Also calculate GR for each processor
    // Only calculate for a reasonable number of samples (avoid huge OS buffers)
    if (numSamples < 16384)
    {
        // Calculate peak levels for input and each processor output
        float inputPeak = 0.0f;
        float hcPeak = 0.0f, scPeak = 0.0f, slPeak = 0.0f, flPeak = 0.0f;

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            const auto* input = buffer.getReadPointer(ch);
            const auto* hcOut = tempBuffer1.getReadPointer(ch);
            const auto* scOut = tempBuffer2.getReadPointer(ch);
            const auto* slOut = tempBuffer3.getReadPointer(ch);
            const auto* flOut = tempBuffer4.getReadPointer(ch);

            for (int i = 0; i < numSamples; ++i)
            {
                inputPeak = juce::jmax(inputPeak, std::abs(static_cast<float>(input[i])));
                hcPeak = juce::jmax(hcPeak, std::abs(static_cast<float>(hcOut[i])));
                scPeak = juce::jmax(scPeak, std::abs(static_cast<float>(scOut[i])));
                slPeak = juce::jmax(slPeak, std::abs(static_cast<float>(slOut[i])));
                flPeak = juce::jmax(flPeak, std::abs(static_cast<float>(flOut[i])));
            }
        }

        // Store per-processor output peaks for waveform visualization
        currentInputPeak.store(inputPeak);
        currentHardClipPeak.store(hcPeak);
        currentSoftClipPeak.store(scPeak);
        currentSlowLimitPeak.store(slPeak);
        currentFastLimitPeak.store(flPeak);

        // Calculate raw GR for each processor (input level - output level in dB)
        // Shows actual per-processor reduction without blend weight scaling
        const float minLevel = 0.00001f;  // -100 dB
        if (inputPeak > minLevel)
        {
            // Calculate individual GR values in dB (positive = reduction)
            // GR_dB = 20 * log10(input / output) = raw per-processor gain reduction
            // NOTE: Blend weights are NOT applied - we show actual reduction per processor
            float hcGR = 0.0f, scGR = 0.0f, slGR = 0.0f, flGR = 0.0f;

            if (hcPeak > minLevel)
            {
                const float ratio = inputPeak / hcPeak;  // Input / Output
                if (ratio > 1.0f)  // Only calculate if there's actual reduction
                {
                    // Raw GR without blend weight scaling
                    hcGR = 20.0f * std::log10(ratio);
                }
            }
            if (scPeak > minLevel)
            {
                const float ratio = inputPeak / scPeak;
                if (ratio > 1.0f)
                {
                    scGR = 20.0f * std::log10(ratio);
                }
            }
            if (slPeak > minLevel)
            {
                const float ratio = inputPeak / slPeak;
                if (ratio > 1.0f)
                {
                    slGR = 20.0f * std::log10(ratio);
                }
            }
            if (flPeak > minLevel)
            {
                const float ratio = inputPeak / flPeak;
                if (ratio > 1.0f)
                {
                    flGR = 20.0f * std::log10(ratio);
                }
            }

            // Store in atomic variables (now in dB)
            currentHardClipGR.store(hcGR);
            currentSoftClipGR.store(scGR);
            currentSlowLimitGR.store(slGR);
            currentFastLimitGR.store(flGR);
        }
        else
        {
            // No signal - zero GR
            currentHardClipGR.store(0.0f);
            currentSoftClipGR.store(0.0f);
            currentSlowLimitGR.store(0.0f);
            currentFastLimitGR.store(0.0f);
        }
    }

    // Blend the four paths (happens in OS domain - major quality improvement)
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* dest = buffer.getWritePointer(ch);
        const auto* src1 = tempBuffer1.getReadPointer(ch);
        const auto* src2 = tempBuffer2.getReadPointer(ch);
        const auto* src3 = tempBuffer3.getReadPointer(ch);
        const auto* src4 = tempBuffer4.getReadPointer(ch);

        for (int i = 0; i < numSamples; ++i)
        {
            dest[i] = wHC * src1[i] + wSC * src2[i] + wSL * src3[i] + wFL * src4[i];
        }
    }
}

//==============================================================================
// Explicit template instantiations
template void QuadBlendDriveAudioProcessor::processXYBlend<float>(juce::AudioBuffer<float>&, double);
template void QuadBlendDriveAudioProcessor::processXYBlend<double>(juce::AudioBuffer<double>&, double);

template void QuadBlendDriveAudioProcessor::processHardClip<float>(juce::AudioBuffer<float>&, float, double);
template void QuadBlendDriveAudioProcessor::processHardClip<double>(juce::AudioBuffer<double>&, double, double);

template void QuadBlendDriveAudioProcessor::processSoftClip<float>(juce::AudioBuffer<float>&, float, float, double);
template void QuadBlendDriveAudioProcessor::processSoftClip<double>(juce::AudioBuffer<double>&, double, double, double);

template void QuadBlendDriveAudioProcessor::processSlowLimit<float>(juce::AudioBuffer<float>&, float, float, float, double);
template void QuadBlendDriveAudioProcessor::processSlowLimit<double>(juce::AudioBuffer<double>&, double, double, double, double);

template void QuadBlendDriveAudioProcessor::processFastLimit<float>(juce::AudioBuffer<float>&, float, float, double);
template void QuadBlendDriveAudioProcessor::processFastLimit<double>(juce::AudioBuffer<double>&, double, double, double);

template void QuadBlendDriveAudioProcessor::processOvershootSuppression<float>(juce::AudioBuffer<float>&, float, double, bool, juce::AudioBuffer<float>*);
template void QuadBlendDriveAudioProcessor::processOvershootSuppression<double>(juce::AudioBuffer<double>&, double, double, bool, juce::AudioBuffer<double>*);

template void QuadBlendDriveAudioProcessor::processAdvancedTPL<float>(juce::AudioBuffer<float>&, float, double, juce::AudioBuffer<float>*);
template void QuadBlendDriveAudioProcessor::processAdvancedTPL<double>(juce::AudioBuffer<double>&, double, double, juce::AudioBuffer<double>*);

template void QuadBlendDriveAudioProcessor::processBlockInternal<float>(juce::AudioBuffer<float>&, juce::MidiBuffer&);
template void QuadBlendDriveAudioProcessor::processBlockInternal<double>(juce::AudioBuffer<double>&, juce::MidiBuffer&);

//==============================================================================
juce::AudioProcessorEditor* QuadBlendDriveAudioProcessor::createEditor()
{
    return new QuadBlendDriveAudioProcessorEditor(*this);
}

void QuadBlendDriveAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());

    // Save normalization state (not stored in APVTS)
    xml->setAttribute("normalizationEnabled", normalizationEnabled);
    xml->setAttribute("computedNormalizationGain", computedNormalizationGain);
    xml->setAttribute("peakInputLevel", peakInputLevel);

    // Save preset slots as child elements
    for (int i = 0; i < 4; ++i)
    {
        if (presetSlots[i].isValid())
        {
            auto presetXml = presetSlots[i].createXml();
            if (presetXml != nullptr)
            {
                presetXml->setTagName("PresetSlot" + juce::String(i));
                xml->addChildElement(presetXml.release());
            }
        }
    }

    copyXmlToBinary(*xml, destData);
}

void QuadBlendDriveAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState.get() != nullptr)
    {
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));

        // Restore normalization state
        if (xmlState->hasAttribute("normalizationEnabled"))
            normalizationEnabled = xmlState->getBoolAttribute("normalizationEnabled", false);
        if (xmlState->hasAttribute("computedNormalizationGain"))
            computedNormalizationGain = xmlState->getDoubleAttribute("computedNormalizationGain", 1.0);
        if (xmlState->hasAttribute("peakInputLevel"))
            peakInputLevel = xmlState->getDoubleAttribute("peakInputLevel", 0.0);

        // Restore preset slots
        for (int i = 0; i < 4; ++i)
        {
            auto* presetXml = xmlState->getChildByName("PresetSlot" + juce::String(i));
            if (presetXml != nullptr)
                presetSlots[i] = juce::ValueTree::fromXml(*presetXml);
        }
    }
}

//==============================================================================
// Preset Management (A, B, C, D)
//==============================================================================

void QuadBlendDriveAudioProcessor::savePreset(int slot)
{
    if (slot < 0 || slot >= 4)
        return;

    // Copy current APVTS state, excluding certain parameters
    auto fullState = apvts.copyState();

    // Exclude these parameters from presets (they are global/session settings):
    // - BYPASS: Plugin bypass state
    // - MASTER_COMP: Gain compensation setting
    // - DELTA_MODE, OVERSHOOT_DELTA_MODE, TRUE_PEAK_DELTA_MODE: Delta modes (diagnostic)
    static const juce::StringArray excludedParams = {
        "BYPASS",
        "MASTER_COMP",
        "DELTA_MODE",
        "OVERSHOOT_DELTA_MODE",
        "TRUE_PEAK_DELTA_MODE"
    };

    // Remove excluded parameters from the state
    auto state = fullState.createCopy();
    for (const auto& paramID : excludedParams)
    {
        for (int i = 0; i < state.getNumChildren(); ++i)
        {
            auto child = state.getChild(i);
            if (child.hasProperty("id") && child.getProperty("id").toString() == paramID)
            {
                state.removeChild(i, nullptr);
                break;
            }
        }
    }

    presetSlots[slot] = state;
}

void QuadBlendDriveAudioProcessor::recallPreset(int slot)
{
    if (slot < 0 || slot >= 4)
        return;

    // Check if preset exists
    if (!presetSlots[slot].isValid())
        return;

    // Save current values of excluded parameters
    const float currentBypass = apvts.getRawParameterValue("BYPASS")->load();
    const float currentMasterComp = apvts.getRawParameterValue("MASTER_COMP")->load();
    const float currentDeltaMode = apvts.getRawParameterValue("DELTA_MODE")->load();
    const float currentOSDelta = apvts.getRawParameterValue("OVERSHOOT_DELTA_MODE")->load();
    const float currentTPDelta = apvts.getRawParameterValue("TRUE_PEAK_DELTA_MODE")->load();

    // Restore preset state to APVTS
    apvts.replaceState(presetSlots[slot].createCopy());

    // Restore excluded parameter values
    if (auto* param = apvts.getParameter("BYPASS"))
        param->setValueNotifyingHost(currentBypass);
    if (auto* param = apvts.getParameter("MASTER_COMP"))
        param->setValueNotifyingHost(currentMasterComp);
    if (auto* param = apvts.getParameter("DELTA_MODE"))
        param->setValueNotifyingHost(currentDeltaMode);
    if (auto* param = apvts.getParameter("OVERSHOOT_DELTA_MODE"))
        param->setValueNotifyingHost(currentOSDelta);
    if (auto* param = apvts.getParameter("TRUE_PEAK_DELTA_MODE"))
        param->setValueNotifyingHost(currentTPDelta);
}

void QuadBlendDriveAudioProcessor::clearPreset(int slot)
{
    if (slot < 0 || slot >= 4)
        return;

    // Clear the preset by creating an invalid/empty ValueTree
    presetSlots[slot] = juce::ValueTree();
}

bool QuadBlendDriveAudioProcessor::hasPreset(int slot) const
{
    if (slot < 0 || slot >= 4)
        return false;

    return presetSlots[slot].isValid();
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new QuadBlendDriveAudioProcessor();
}

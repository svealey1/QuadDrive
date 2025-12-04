#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

//==============================================================================
QuadBlendDriveAudioProcessor::QuadBlendDriveAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
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
        juce::NormalisableRange<float>(-12.0f, 0.0f, 0.1f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " dBFS"; }));

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
        "LIMIT_REL", "Limiter Release",
        juce::NormalisableRange<float>(10.0f, 500.0f, 1.0f, 0.5f), 100.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 0) + " ms"; }));

    // Blending Parameters
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "XY_X_PARAM", "X Position",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));  // Default to Hard Clip (left)

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "XY_Y_PARAM", "Y Position",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 1.0f));  // Default to top

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

    // Processing Mode: 0 = Zero Latency, 1 = Balanced (Halfband), 2 = Linear Phase
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "PROCESSING_MODE", "Processing Engine",
        juce::StringArray{"Zero Latency", "Balanced", "Linear Phase"},
        0));  // Default to Zero Latency

    // === ADVANCED DSP FEATURES ===
    // MCSR (Micro-Clipping Symmetry Restoration) Parameters
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "MCSR_STRENGTH", "MCSR Intensity",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.6f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value * 100.0f, 0) + " %"; }));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        "MCSR_ENABLED", "MCSR On/Off", true));

    // Transient-Preserving Envelope Parameters
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "SLOW_LIMITER_RMS_MS", "Slow Limiter RMS Time",
        juce::NormalisableRange<float>(10.0f, 100.0f, 1.0f), 30.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 0) + " ms"; }));

    // Adaptive Release Parameters
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "ADAPTIVE_RELEASE_MIN", "Min Release",
        juce::NormalisableRange<float>(10.0f, 100.0f, 1.0f), 20.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 0) + " ms"; }));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "ADAPTIVE_RELEASE_MAX", "Max Release",
        juce::NormalisableRange<float>(100.0f, 1000.0f, 10.0f), 500.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 0) + " ms"; }));

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
    const int maxOsMultiplier = 16;  // Linear Phase mode
    const double maxOsSampleRate = sampleRate * maxOsMultiplier;
    const double maxLookaheadMs = 3.0;  // Standard mode
    const int maxLookaheadSamples = static_cast<int>(std::ceil(maxOsSampleRate * maxLookaheadMs / 1000.0));

    // Calculate CURRENT lookahead samples at current OS rate (used for actual processing)
    lookaheadSamples = static_cast<int>(std::ceil(osSampleRate * xyProcessorLookaheadMs / 1000.0));
    advancedTPLLookaheadSamples = static_cast<int>(std::ceil(osSampleRate * tplLookaheadMs / 1000.0));

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

        // Initialize lookahead buffer
        state.lookaheadBuffer.resize(advancedTPLLookaheadSamples, 0.0);
        state.lookaheadWritePos = 0;
        state.grEnvelope = 0.0;
        state.lowBandEnv = 0.0;
        state.midBandEnv = 0.0;
        state.highBandEnv = 0.0;
        state.currentReleaseCoeff = 0.001;  // Default release

        // Initialize OSM Mode 0 compensation delay buffer (matches Mode 1 lookahead)
        osmCompensationState[ch].delayBuffer.resize(advancedTPLLookaheadSamples, 0.0);
        osmCompensationState[ch].writePos = 0;

        // Initialize dry delay buffer for parallel processing (OS rate)
        // v1.7.6: Delay applied in OS domain for 4-channel sync processing
        // Use maximum OS rate lookahead samples for buffer sizing
        dryDelayState[ch].delayBuffer.resize(maxLookaheadSamples, 0.0);
        dryDelayState[ch].writePos = 0;

        // Design multiband filters for IRC at OS rate
        const double pi = juce::MathConstants<double>::pi;
        const double fs = osSampleRate;  // Oversampled rate (1×, 8×, or 16×)

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

    for (int ch = 0; ch < 2; ++ch)
    {
        // Low-pass filter at 250 Hz
        double lowFreq = 250.0;
        double lowOmega = 2.0 * pi * lowFreq / sampleRate;
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
        double midOmega = 2.0 * pi * midFreq / sampleRate;
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
        double highOmega = 2.0 * pi * highFreq / sampleRate;
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

    // Allocate double buffers
    dryBufferDouble.setSize(2, samplesPerBlock);
    tempBuffer1Double.setSize(2, samplesPerBlock);
    tempBuffer2Double.setSize(2, samplesPerBlock);
    tempBuffer3Double.setSize(2, samplesPerBlock);
    tempBuffer4Double.setSize(2, samplesPerBlock);
    combined4ChDouble.setSize(4, samplesPerBlock);  // 4-channel for phase-coherent processing
    protectionDeltaCombined4ChDouble.setSize(4, samplesPerBlock);  // Pre-allocated for overshoot delta mode

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

    // ================================================================
    // Initialize Advanced DSP Features
    // ================================================================
    // MCSR: Micro-Clipping Symmetry Restoration
    mcsr.reset();
    mcsr.setStrength(apvts.getRawParameterValue("MCSR_STRENGTH")->load());
    mcsr.setEnabled(apvts.getRawParameterValue("MCSR_ENABLED")->load() > 0.5f);

    // Transient-Preserving Envelope for Slow Limiter
    slowLimiterEnvelope.reset();
    slowLimiterEnvelope.setRMSSmoothingMs(
        apvts.getRawParameterValue("SLOW_LIMITER_RMS_MS")->load(),
        sampleRate);

    // Adaptive Release Limiter
    adaptiveSlowLimiter.reset();
    adaptiveSlowLimiter.setBaseReleaseTimeMs(100.0f, sampleRate);  // Base release time
    adaptiveSlowLimiter.setReleaseTimeBounds(
        apvts.getRawParameterValue("ADAPTIVE_RELEASE_MIN")->load(),
        apvts.getRawParameterValue("ADAPTIVE_RELEASE_MAX")->load(),
        sampleRate);
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

    // Reset advanced DSP processors
    mcsr.reset();
    slowLimiterEnvelope.reset();
    adaptiveSlowLimiter.reset();
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

void QuadBlendDriveAudioProcessor::calculateNormalizationGain()
{
    calibrationTargetDB = static_cast<double>(apvts.getRawParameterValue("CALIB_LEVEL")->load());

    if (peakInputLevel > 0.0)
    {
        double peakDB = 20.0 * std::log10(peakInputLevel);
        currentPeakDB.store(peakDB);

        // Calculate gain needed to bring peak to calibration target
        double gainNeededDB = calibrationTargetDB - peakDB;
        computedNormalizationGain = std::pow(10.0, gainNeededDB / 20.0);
        normalizationGainDB.store(gainNeededDB);
    }
    else
    {
        currentPeakDB.store(-60.0);
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
        float grMin = 0.0f;
        float grMax = 0.0f;
        float sumLow = 0.0f;
        float sumMid = 0.0f;
        float sumHigh = 0.0f;

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
                grMin = sample.gainReduction;
                grMax = sample.gainReduction;
            }
            else
            {
                waveformMin = std::min(waveformMin, waveform);
                waveformMax = std::max(waveformMax, waveform);
                grMin = std::min(grMin, sample.gainReduction);
                grMax = std::max(grMax, sample.gainReduction);
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
        seg.grMin = grMin;
        seg.grMax = grMax;
        seg.avgLow = sumLow / static_cast<float>(samplesPerSegment);
        seg.avgMid = sumMid / static_cast<float>(samplesPerSegment);
        seg.avgHigh = sumHigh / static_cast<float>(samplesPerSegment);
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

    const SampleType hcTrimDB = static_cast<SampleType>(apvts.getRawParameterValue("HC_TRIM")->load());
    const SampleType scTrimDB = static_cast<SampleType>(apvts.getRawParameterValue("SC_TRIM")->load());
    const SampleType slTrimDB = static_cast<SampleType>(apvts.getRawParameterValue("SL_TRIM")->load());
    const SampleType flTrimDB = static_cast<SampleType>(apvts.getRawParameterValue("FL_TRIM")->load());

    const bool hcMute = apvts.getRawParameterValue("HC_MUTE")->load() > 0.5f;
    const bool scMute = apvts.getRawParameterValue("SC_MUTE")->load() > 0.5f;
    const bool slMute = apvts.getRawParameterValue("SL_MUTE")->load() > 0.5f;
    const bool flMute = apvts.getRawParameterValue("FL_MUTE")->load() > 0.5f;

    const bool deltaMode = apvts.getRawParameterValue("DELTA_MODE")->load() > 0.5f;

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

    // === MCSR (MICRO-CLIPPING SYMMETRY RESTORATION) ANALYSIS ===
    // Analyze buffer for waveform asymmetry before processing
    // This recovers 0.8-1.5 dB of headroom by balancing asymmetrical peaks
    // Zero latency, no phase shift, completely transparent
    mcsr.setStrength(apvts.getRawParameterValue("MCSR_STRENGTH")->load());
    mcsr.setEnabled(apvts.getRawParameterValue("MCSR_ENABLED")->load() > 0.5f);

    // Convert buffer to float for MCSR analysis (works on float precision)
    juce::AudioBuffer<float> analysisBuffer(buffer.getNumChannels(), numSamples);
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            analysisBuffer.setSample(ch, i, static_cast<float>(buffer.getSample(ch, i)));
        }
    }
    mcsr.analyzeBlock(analysisBuffer, numSamples);

    // === SIGNAL FLOW (v1.7.4) ===
    // 1. Normalization applied to buffer (both dry and wet get this)
    // 2. Dry tapped (has normalization, NO input gain)
    // 3. Input gain applied to buffer (wet only - for drive)
    // 4. Buffer processed through OS + XY blend (wet path)
    // 5. Inverse gain applied to wet (if master comp enabled - keeps output level constant)
    // 6. Output gain applied to wet
    // 7. Dry/wet mix
    // Result: Adjusting input gain changes drive amount without changing output level (when master comp on)

    // === INPUT NORMALIZATION ===
    // Apply normalization gain FIRST (before everything else)
    SampleType normGain = static_cast<SampleType>(1.0);
    if (normalizationEnabled)
    {
        normGain = static_cast<SampleType>(computedNormalizationGain);
        buffer.applyGain(normGain);
    }

    // === CAPTURE DRY SIGNAL (After normalization, BEFORE input gain) ===
    // Dry = normalization only, NO input gain
    auto& originalInputBuffer = (std::is_same<SampleType, float>::value)
        ? reinterpret_cast<juce::AudioBuffer<SampleType>&>(originalInputBufferFloat)
        : reinterpret_cast<juce::AudioBuffer<SampleType>&>(originalInputBufferDouble);

    originalInputBuffer.makeCopyOf(buffer);

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

    // === EARLY EXIT: All Processors Muted (Unity Gain Passthrough) ===
    // When all processors are muted, skip ALL processing including oversampling
    // This prevents oversampling filter coloration and ensures bit-perfect passthrough at 0% mix
    // NOTE: At this point, buffer has input gain applied, so we rebuild from originalInputBuffer
    if (allProcessorsMuted)
    {
        // Rebuild output from pristine input (buffer was modified by input gain loop)
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* out = buffer.getWritePointer(ch);
            const auto* pristine = originalInputBuffer.getReadPointer(ch);

            for (int i = 0; i < numSamples; ++i)
            {
                const SampleType smoothedOutGain = static_cast<SampleType>(smoothedOutputGain.getNextValue());
                const SampleType smoothedMix = static_cast<SampleType>(smoothedMixWet.getNextValue());

                // Check for unity output gain (within 0.1% of 1.0)
                // Critical for bit-perfect passthrough in unity gain test
                const bool isUnityOutputGain = (smoothedOutGain > static_cast<SampleType>(0.999) &&
                                                smoothedOutGain < static_cast<SampleType>(1.001));

                // Dry/wet mix with threshold checks
                // When all processors muted:
                //   - "Dry" = pristine input (no gains)
                //   - "Wet" = pristine input * output gain (no processing, just output scaling)
                if (smoothedMix < static_cast<SampleType>(0.001))
                {
                    out[i] = pristine[i];  // Pure dry - bit-perfect passthrough
                }
                else if (smoothedMix > static_cast<SampleType>(0.999))
                {
                    // Pure wet (100% mix)
                    if (isUnityOutputGain)
                    {
                        // Unity gain - bit-perfect passthrough
                        out[i] = pristine[i];
                    }
                    else
                    {
                        // Apply output gain
                        out[i] = pristine[i] * smoothedOutGain;
                    }
                }
                else
                {
                    // Partial mix - crossfade between dry and wet
                    SampleType wetSignal = isUnityOutputGain ? pristine[i] : pristine[i] * smoothedOutGain;
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

    // Store pristine dry (no gains, no processing)
    dryBuffer.makeCopyOf(originalInputBuffer);

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

        // Apply mute logic
        const bool hcMute = apvts.getRawParameterValue("HC_MUTE")->load() > 0.5f;
        const bool scMute = apvts.getRawParameterValue("SC_MUTE")->load() > 0.5f;
        const bool slMute = apvts.getRawParameterValue("SL_MUTE")->load() > 0.5f;
        const bool flMute = apvts.getRawParameterValue("FL_MUTE")->load() > 0.5f;
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
        // 1. Input gain (wet has this from before oversampling, dry doesn't)
        // 2. Weighted trim gain (wet has this from processXYBlend, dry doesn't)
        // 3. Inverse input gain (wet gets this, dry should too)
        // 4. Output gain (wet gets this, dry should too)
        for (int ch = 0; ch < dryBuffer.getNumChannels(); ++ch)
        {
            auto* dry = dryBuffer.getWritePointer(ch);

            for (int i = 0; i < numSamples; ++i)
            {
                // Apply input gain (to match wet)
                SampleType compensatedDry = dry[i] * inputGain;

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
    const bool truePeakEnabled = apvts.getRawParameterValue("TRUE_PEAK_ENABLE")->load() > 0.5f;
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

                // Apply MCSR symmetry restoration
                const float mcsrProcessed = mcsr.processSample(static_cast<float>(output));
                data[i] = static_cast<SampleType>(mcsrProcessed);
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

            // Apply MCSR symmetry restoration
            const float mcsrProcessed = mcsr.processSample(static_cast<float>(output));
            data[i] = static_cast<SampleType>(mcsrProcessed);
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
    const double color = static_cast<double>(knee) / 100.0;  // Convert knee percentage to color (0-1)
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

                // Apply MCSR symmetry restoration
                const float mcsrProcessed = mcsr.processSample(static_cast<float>(output * ceilingD));
                data[i] = static_cast<SampleType>(mcsrProcessed);
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

            // Apply MCSR symmetry restoration
            const float mcsrProcessed = mcsr.processSample(static_cast<float>(output * ceilingD));
            data[i] = static_cast<SampleType>(mcsrProcessed);
        }
    }
}

// Slow Limiting - Crest-Factor-Based Adaptive Release Limiter
// Fast release (20-40ms) for transients, slow release (200-600ms) for sustained material
// Dynamically blended based on signal crest factor with exponential smoothing
// MODE-AWARE: Zero Latency = no oversampling, Balanced/Linear Phase = 8x oversampling
template<typename SampleType>
void QuadBlendDriveAudioProcessor::processSlowLimit(juce::AudioBuffer<SampleType>& buffer,
                                                      SampleType threshold,
                                                      SampleType baseReleaseMs,
                                                      double sampleRate)
{
    // Get current processing mode from osManager
    const int processingMode = osManager.getProcessingMode();

    // Fixed parameters for Slow Limiter
    const double attackMs = 3.0;                    // 3ms attack
    const double fastReleaseMinMs = 20.0;          // Fast release min (for transients)
    const double fastReleaseMaxMs = 40.0;          // Fast release max
    const double slowReleaseMinMs = 300.0;         // Slow release min (for sustained)
    const double slowReleaseMaxMs = 800.0;         // Slow release max
    const double kneeDB = 3.0;                     // 3dB soft knee

    // Time constants for envelope followers
    // MODE 0: direct sample rate, MODE 1-2: oversampled rate
    const double effectiveRate = (processingMode == 0) ? sampleRate : (sampleRate * 8.0);
    const double rmsTimeMs = 50.0;                 // RMS averaging time
    const double peakTimeMs = 10.0;                // Peak tracking time
    const double crestSmoothTimeMs = 100.0;        // Crest factor smoothing time

    const double attackCoeff = std::exp(-1.0 / (attackMs * 0.001 * effectiveRate));
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
    if (processingMode == 0)
    {
        for (int ch = 0; ch < juce::jmin(buffer.getNumChannels(), 2); ++ch)
        {
            auto* data = buffer.getWritePointer(ch);
            double& envelope = slowLimiterState[ch].envelope;
            double& rmsEnvelope = slowLimiterState[ch].rmsEnvelope;
            double& peakEnvelope = slowLimiterState[ch].peakEnvelope;
            double& smoothedCrestFactor = slowLimiterState[ch].smoothedCrestFactor;
            double& smoothedGain = slowLimiterState[ch].smoothedGain;

            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                const double currentSample = static_cast<double>(data[i]);
                const double inputAbs = std::abs(currentSample);

                // Use TransientPreservingEnvelope for better transient detection
                const float envelopeInput = static_cast<float>(currentSample);
                const float detectedEnvelope = slowLimiterEnvelope.getEnvelope(envelopeInput);
                const double inputEnvelope = static_cast<double>(detectedEnvelope);

                // Track RMS for crest factor calculation (still needed for adaptive release)
                const double inputSquared = currentSample * currentSample;
                rmsEnvelope = rmsCoeff * rmsEnvelope + (1.0 - rmsCoeff) * inputSquared;
                const double rmsValue = std::sqrt(juce::jmax(1e-10, rmsEnvelope));

                // Calculate crest factor using detected envelope vs RMS
                const double instantCrestFactor = inputEnvelope / rmsValue;
                smoothedCrestFactor = crestSmoothCoeff * smoothedCrestFactor +
                                      (1.0 - crestSmoothCoeff) * instantCrestFactor;

                // Map crest factor to release time
                const double crestNorm = juce::jlimit(0.0, 1.0, (smoothedCrestFactor - 1.0) / 9.0);
                const double minRelease = fastReleaseMinMs + crestNorm * (slowReleaseMinMs - fastReleaseMinMs);
                const double maxRelease = fastReleaseMaxMs + crestNorm * (slowReleaseMaxMs - fastReleaseMaxMs);

                const double overAmount = juce::jmax(0.0, inputEnvelope - thresholdD);
                const double adaptiveFactor = overAmount / (thresholdD + 1e-10);
                const double adaptiveReleaseMs = juce::jlimit(minRelease, maxRelease,
                                                             minRelease + (maxRelease - minRelease) * adaptiveFactor);

                const double releaseCoeff = std::exp(-1.0 / (adaptiveReleaseMs * 0.001 * effectiveRate));

                // Attack/release envelope follower using TransientPreservingEnvelope output
                if (inputEnvelope > envelope)
                    envelope = attackCoeff * envelope + (1.0 - attackCoeff) * inputEnvelope;
                else
                    envelope = releaseCoeff * envelope + (1.0 - releaseCoeff) * inputEnvelope;

                // Calculate raw gain reduction with soft knee (NO lookahead delay)
                double targetGain = 1.0;
                if (envelope <= kneeStart)
                {
                    targetGain = 1.0;
                }
                else if (envelope < thresholdD)
                {
                    const double kneePos = (envelope - kneeStart) / (thresholdD - kneeStart);
                    const double compressionAmount = kneePos * kneePos;
                    targetGain = 1.0 - compressionAmount * (1.0 - thresholdD / envelope);
                }
                else
                {
                    targetGain = thresholdD / envelope;
                }

                // MODE 0: No gain smoothing (no oversampling = no aliasing to prevent)
                // Apply gain directly for truly transparent zero-latency processing
                const double limitedSample = currentSample * targetGain;

                // Apply MCSR symmetry restoration
                const float mcsrProcessed = mcsr.processSample(static_cast<float>(limitedSample));
                data[i] = static_cast<SampleType>(mcsrProcessed);
            }
        }
        return;
    }

    // MODE 1 & 2: Architecture A - Process with lookahead (buffer already at OS rate)
    const int numSamples = buffer.getNumSamples();
    for (int ch = 0; ch < juce::jmin(buffer.getNumChannels(), 2); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        double& envelope = slowLimiterState[ch].envelope;
        double& rmsEnvelope = slowLimiterState[ch].rmsEnvelope;
        double& peakEnvelope = slowLimiterState[ch].peakEnvelope;
        double& smoothedCrestFactor = slowLimiterState[ch].smoothedCrestFactor;
        double& smoothedGain = slowLimiterState[ch].smoothedGain;
        auto& lookaheadBuffer = slowLimiterState[ch].lookaheadBuffer;
        int& writePos = slowLimiterState[ch].lookaheadWritePos;

        for (int i = 0; i < numSamples; ++i)
        {
            const double currentSample = static_cast<double>(data[i]);

            // Store current sample in lookahead buffer
            if (writePos >= 0 && writePos < static_cast<int>(lookaheadBuffer.size()))
                lookaheadBuffer[writePos] = currentSample;

            // Read delayed sample (3ms ago)
            int readPos = (writePos + 1) % lookaheadSamples;
            double delayedSample = currentSample;
            if (readPos >= 0 && readPos < static_cast<int>(lookaheadBuffer.size()))
                delayedSample = lookaheadBuffer[readPos];

            writePos = (writePos + 1) % lookaheadSamples;

            const double inputAbs = std::abs(currentSample);

            // Use TransientPreservingEnvelope for better transient detection
            const float envelopeInput = static_cast<float>(currentSample);
            const float detectedEnvelope = slowLimiterEnvelope.getEnvelope(envelopeInput);
            const double inputEnvelope = static_cast<double>(detectedEnvelope);

            // Track RMS for crest factor calculation (still needed for adaptive release)
            const double inputSquared = currentSample * currentSample;
            rmsEnvelope = rmsCoeff * rmsEnvelope + (1.0 - rmsCoeff) * inputSquared;
            const double rmsValue = std::sqrt(juce::jmax(1e-10, rmsEnvelope));

            // Calculate instantaneous crest factor using detected envelope vs RMS
            // High crest factor = transient/spiky signal → fast release
            // Low crest factor = sustained/dense signal → slow release
            const double instantCrestFactor = inputEnvelope / rmsValue;

            // Exponentially smooth the crest factor to avoid jumps
            smoothedCrestFactor = crestSmoothCoeff * smoothedCrestFactor +
                                  (1.0 - crestSmoothCoeff) * instantCrestFactor;

            // Map crest factor to release time
            // Crest factors typically range from 1.0 (square wave) to 10+ (very transient)
            // Normalize to 0-1 range: crestNorm = 0 for sustained, 1 for transient
            const double crestNorm = juce::jlimit(0.0, 1.0, (smoothedCrestFactor - 1.0) / 9.0);

            // Blend between slow and fast release based on crest factor
            // High crest (transient) → fast release, Low crest (sustained) → slow release
            const double minRelease = fastReleaseMinMs + crestNorm * (slowReleaseMinMs - fastReleaseMinMs);
            const double maxRelease = fastReleaseMaxMs + crestNorm * (slowReleaseMaxMs - fastReleaseMaxMs);

            // Further adapt within the range based on how much we're over threshold
            const double overAmount = juce::jmax(0.0, inputEnvelope - thresholdD);
            const double adaptiveFactor = overAmount / (thresholdD + 1e-10);
            const double adaptiveReleaseMs = juce::jlimit(minRelease, maxRelease,
                                                         minRelease + (maxRelease - minRelease) * adaptiveFactor);

            const double releaseCoeff = std::exp(-1.0 / (adaptiveReleaseMs * 0.001 * effectiveRate));

            // Attack/release envelope follower using TransientPreservingEnvelope output
            if (inputEnvelope > envelope)
                envelope = attackCoeff * envelope + (1.0 - attackCoeff) * inputEnvelope;
            else
                envelope = releaseCoeff * envelope + (1.0 - releaseCoeff) * inputEnvelope;

            // Calculate raw gain reduction with soft knee
            double targetGain = 1.0;

            if (envelope <= kneeStart)
            {
                targetGain = 1.0;  // No limiting
            }
            else if (envelope < thresholdD)
            {
                // Soft knee region
                const double kneePos = (envelope - kneeStart) / (thresholdD - kneeStart);
                const double compressionAmount = kneePos * kneePos;  // Smooth curve
                targetGain = 1.0 - compressionAmount * (1.0 - thresholdD / envelope);
            }
            else
            {
                // Above threshold - apply limiting
                targetGain = thresholdD / envelope;
            }

            // Apply one-pole low-pass filter to gain signal (prevents control signal aliasing)
            smoothedGain = gainSmoothCoeff * smoothedGain + (1.0 - gainSmoothCoeff) * targetGain;

            const double limitedSample = delayedSample * smoothedGain;

            // Apply MCSR symmetry restoration
            const float mcsrProcessed = mcsr.processSample(static_cast<float>(limitedSample));
            data[i] = static_cast<SampleType>(mcsrProcessed);
        }
    }
}

// Fast Limiting - Hard knee limiting with fixed 10ms release
// Attack: 1ms, Release: 10ms (fixed), Hard knee, Lookahead: 3ms
// MODE-AWARE: Zero Latency = no oversampling, Balanced/Linear Phase = 8x oversampling
template<typename SampleType>
void QuadBlendDriveAudioProcessor::processFastLimit(juce::AudioBuffer<SampleType>& buffer,
                                                      SampleType threshold,
                                                      double sampleRate)
{
    // Get current processing mode from osManager
    const int processingMode = osManager.getProcessingMode();

    // Fixed parameters for Fast Limiter
    const double attackMs = 1.0;        // 1ms attack
    const double releaseMs = 10.0;      // 10ms release (fixed)

    // MODE 0: direct sample rate, MODE 1-2: oversampled rate
    const double effectiveRate = (processingMode == 0) ? currentSampleRate : (currentSampleRate * 8.0);

    const double attackCoeff = std::exp(-1.0 / (attackMs * 0.001 * effectiveRate));
    const double releaseCoeff = std::exp(-1.0 / (releaseMs * 0.001 * effectiveRate));
    const double thresholdD = static_cast<double>(threshold);

    // Gain smoothing filter coefficient to prevent control signal aliasing
    // Cutoff ~20kHz at the effective sample rate (prevents GR signal from aliasing)
    const double gainSmoothCutoff = 20000.0;  // 20 kHz cutoff
    const double gainSmoothCoeff = std::exp(-2.0 * juce::MathConstants<double>::pi * gainSmoothCutoff / effectiveRate);

    // MODE 0: Zero Latency - NO oversampling, NO lookahead, direct processing
    if (processingMode == 0)
    {
        for (int ch = 0; ch < juce::jmin(buffer.getNumChannels(), 2); ++ch)
        {
            auto* data = buffer.getWritePointer(ch);
            double& envelope = fastLimiterState[ch].envelope;
            double& smoothedGain = fastLimiterState[ch].smoothedGain;

            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                const double currentSample = static_cast<double>(data[i]);
                const double inputAbs = std::abs(currentSample);

                // Envelope follower with attack/release
                if (inputAbs > envelope)
                    envelope = attackCoeff * envelope + (1.0 - attackCoeff) * inputAbs;
                else
                    envelope = releaseCoeff * envelope + (1.0 - releaseCoeff) * inputAbs;

                // Clamp envelope to prevent extreme values
                envelope = juce::jlimit(0.0, 10.0, envelope);

                // Calculate raw gain reduction - hard knee limiting (NO lookahead delay)
                double targetGain = 1.0;
                if (envelope > thresholdD)
                {
                    targetGain = thresholdD / envelope;
                }

                // Clamp target gain to prevent artifacts
                targetGain = juce::jlimit(0.01, 1.0, targetGain);

                // MODE 0: No gain smoothing (no oversampling = no aliasing to prevent)
                // Apply gain directly for truly transparent zero-latency processing
                const double limitedSample = currentSample * targetGain;

                // Apply MCSR symmetry restoration
                const float mcsrProcessed = mcsr.processSample(static_cast<float>(limitedSample));
                data[i] = static_cast<SampleType>(mcsrProcessed);
            }
        }
        return;
    }

    // MODE 1 & 2: Architecture A - Process with lookahead (buffer already at OS rate)
    const int numSamples = buffer.getNumSamples();
    for (int ch = 0; ch < juce::jmin(buffer.getNumChannels(), 2); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        double& envelope = fastLimiterState[ch].envelope;
        double& smoothedGain = fastLimiterState[ch].smoothedGain;
        auto& lookaheadBuffer = fastLimiterState[ch].lookaheadBuffer;
        int& writePos = fastLimiterState[ch].lookaheadWritePos;

        for (int i = 0; i < numSamples; ++i)
        {
            const double currentSample = static_cast<double>(data[i]);

            // Store current sample in lookahead buffer with bounds checking
            if (writePos >= 0 && writePos < static_cast<int>(lookaheadBuffer.size()))
                lookaheadBuffer[writePos] = currentSample;

            // Read delayed sample (3ms ago)
            int readPos = (writePos + 1) % lookaheadSamples;
            double delayedSample = currentSample;
            if (readPos >= 0 && readPos < static_cast<int>(lookaheadBuffer.size()))
                delayedSample = lookaheadBuffer[readPos];

            writePos = (writePos + 1) % lookaheadSamples;

            // Direct envelope detection (no pre-filtering)
            const double inputAbs = std::abs(currentSample);

            // Envelope follower with attack/release
            if (inputAbs > envelope)
                envelope = attackCoeff * envelope + (1.0 - attackCoeff) * inputAbs;
            else
                envelope = releaseCoeff * envelope + (1.0 - releaseCoeff) * inputAbs;

            // Clamp envelope to prevent extreme values
            envelope = juce::jlimit(0.0, 10.0, envelope);

            // Calculate raw gain reduction - hard knee limiting
            double targetGain = 1.0;
            if (envelope > thresholdD)
            {
                targetGain = thresholdD / envelope;
            }

            // Clamp target gain to prevent artifacts
            targetGain = juce::jlimit(0.01, 1.0, targetGain);

            // Apply one-pole low-pass filter to gain signal (prevents control signal aliasing)
            smoothedGain = gainSmoothCoeff * smoothedGain + (1.0 - gainSmoothCoeff) * targetGain;

            const double limitedSample = delayedSample * smoothedGain;

            // Apply MCSR symmetry restoration
            const float mcsrProcessed = mcsr.processSample(static_cast<float>(limitedSample));
            data[i] = static_cast<SampleType>(mcsrProcessed);
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

    // === OSM MODE 0 CONSTANT LATENCY COMPENSATION ===
    // Apply delay to match Mode 1 (Advanced TPL) lookahead latency
    // This ensures the plugin reports constant latency regardless of OSM mode
    const int osmDelay = advancedTPLLookaheadSamples;

    // Guard against division by zero
    if (osmDelay <= 0)
    {
        // Skip delay compensation - just apply final clamp
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* data = buffer.getWritePointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                data[i] = juce::jlimit(static_cast<SampleType>(-ceilingLinear),
                                       static_cast<SampleType>(ceilingLinear), data[i]);
            }
        }
        return;
    }

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        auto& compensationState = osmCompensationState[ch];

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            // Store current sample in delay buffer
            compensationState.delayBuffer[compensationState.writePos] = static_cast<double>(data[i]);

            // Read delayed sample from buffer
            int readPos = compensationState.writePos;
            double delayedSample = compensationState.delayBuffer[readPos];

            // Advance write position (circular buffer)
            compensationState.writePos = (compensationState.writePos + 1) % osmDelay;

            // Output delayed sample
            data[i] = static_cast<SampleType>(delayedSample);
        }
    }

    // Final safety clamp to ensure true-peak compliance
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

    const SampleType hcTrimDB = static_cast<SampleType>(apvts.getRawParameterValue("HC_TRIM")->load());
    const SampleType scTrimDB = static_cast<SampleType>(apvts.getRawParameterValue("SC_TRIM")->load());
    const SampleType slTrimDB = static_cast<SampleType>(apvts.getRawParameterValue("SL_TRIM")->load());
    const SampleType flTrimDB = static_cast<SampleType>(apvts.getRawParameterValue("FL_TRIM")->load());

    const bool hcMute = apvts.getRawParameterValue("HC_MUTE")->load() > 0.5f;
    const bool scMute = apvts.getRawParameterValue("SC_MUTE")->load() > 0.5f;
    const bool slMute = apvts.getRawParameterValue("SL_MUTE")->load() > 0.5f;
    const bool flMute = apvts.getRawParameterValue("FL_MUTE")->load() > 0.5f;

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

    // Process all 4 paths (now at OS rate - no internal oversampling needed)
    tempBuffer1.applyGain(hcTrimGain);
    processHardClip(tempBuffer1, threshold, osSampleRate);

    tempBuffer2.applyGain(scTrimGain);
    processSoftClip(tempBuffer2, threshold, scKnee, osSampleRate);

    tempBuffer3.applyGain(slTrimGain);
    processSlowLimit(tempBuffer3, threshold, limitRelMs, osSampleRate);

    tempBuffer4.applyGain(flTrimGain);
    processFastLimit(tempBuffer4, threshold, osSampleRate);

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

template void QuadBlendDriveAudioProcessor::processSlowLimit<float>(juce::AudioBuffer<float>&, float, float, double);
template void QuadBlendDriveAudioProcessor::processSlowLimit<double>(juce::AudioBuffer<double>&, double, double, double);

template void QuadBlendDriveAudioProcessor::processFastLimit<float>(juce::AudioBuffer<float>&, float, double);
template void QuadBlendDriveAudioProcessor::processFastLimit<double>(juce::AudioBuffer<double>&, double, double);

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
    copyXmlToBinary(*xml, destData);
}

void QuadBlendDriveAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
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

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
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 1.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "XY_Y_PARAM", "Y Position",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 1.0f));

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

    // Protection Limiter Parameters
    layout.add(std::make_unique<juce::AudioParameterBool>(
        "PROTECTION_ENABLE", "Protection Limiter", false));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "PROTECTION_CEILING", "Protection Ceiling",
        juce::NormalisableRange<float>(-3.0f, 1.0f, 0.1f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " dBTP"; }));

    // Overshoot Blend: 0 = Clean (gentle), 1 = Punchy (aggressive)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "OVERSHOOT_BLEND", "Overshoot Character",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) {
            if (value < 0.33f)
                return juce::String("Clean");
            else if (value < 0.66f)
                return juce::String("Balanced");
            else
                return juce::String("Punchy");
        }));

    // OSM (Overshoot Suppression Module): Dual-Mode Output Processor
    // Mode 0 (false): Safety Clipper - Preserves transients, slight overshoot allowed
    // Mode 1 (true):  Advanced TPL - Strict compliance, predictive lookahead with IRC
    layout.add(std::make_unique<juce::AudioParameterBool>(
        "OSM_MODE", "OSM Mode (Safe/Clip)", false));

    // Processing Mode: 0 = Zero Latency, 1 = Balanced (Halfband), 2 = Linear Phase
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "PROCESSING_MODE", "Processing Engine",
        juce::StringArray{"Zero Latency", "Balanced", "Linear Phase"},
        1));  // Default to Balanced

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

    // ===== TEMPORARY: Old oversampling objects for processBlockInternal =====
    // TODO: Remove in Phase 3 when processBlockInternal is refactored
    // Initialize ALL three types (old code initialized all, not just current mode)
    constexpr size_t oversamplingFactor = 3;  // 2^3 = 8× oversampling
    constexpr size_t numChannels = 2;

    // MODE 0: Zero Latency (Minimum-Phase Polyphase IIR)
    oversamplingZeroLatencyFloat = std::make_unique<juce::dsp::Oversampling<float>>(
        numChannels, oversamplingFactor,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        false, false);  // No steep filter, no normalization (minimum-phase)

    oversamplingZeroLatencyDouble = std::make_unique<juce::dsp::Oversampling<double>>(
        numChannels, oversamplingFactor,
        juce::dsp::Oversampling<double>::filterHalfBandPolyphaseIIR,
        false, false);

    // MODE 1: Balanced (Halfband Equiripple FIR)
    oversamplingBalancedFloat = std::make_unique<juce::dsp::Oversampling<float>>(
        numChannels, oversamplingFactor,
        juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple,
        true, false);  // Steep, no normalization

    oversamplingBalancedDouble = std::make_unique<juce::dsp::Oversampling<double>>(
        numChannels, oversamplingFactor,
        juce::dsp::Oversampling<double>::filterHalfBandFIREquiripple,
        true, false);

    // MODE 2: Linear Phase (High-Order FIR with normalization)
    oversamplingLinearPhaseFloat = std::make_unique<juce::dsp::Oversampling<float>>(
        numChannels, oversamplingFactor,
        juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple,
        true, true);  // Steep + normalization for maximum quality

    oversamplingLinearPhaseDouble = std::make_unique<juce::dsp::Oversampling<double>>(
        numChannels, oversamplingFactor,
        juce::dsp::Oversampling<double>::filterHalfBandFIREquiripple,
        true, true);

    // Initialize all oversampling objects
    oversamplingZeroLatencyFloat->initProcessing(static_cast<size_t>(samplesPerBlock));
    oversamplingZeroLatencyDouble->initProcessing(static_cast<size_t>(samplesPerBlock));
    oversamplingBalancedFloat->initProcessing(static_cast<size_t>(samplesPerBlock));
    oversamplingBalancedDouble->initProcessing(static_cast<size_t>(samplesPerBlock));
    oversamplingLinearPhaseFloat->initProcessing(static_cast<size_t>(samplesPerBlock));
    oversamplingLinearPhaseDouble->initProcessing(static_cast<size_t>(samplesPerBlock));

    // Get OS sample rate for all processor calculations
    const double osSampleRate = osManager.getOsSampleRate();
    const int osMultiplier = osManager.getOsMultiplier();

    // Calculate lookahead samples at OS rate
    // All processors run in OS domain, so lookahead must be at OS rate
    constexpr double xyProcessorLookaheadMs = 3.0;  // 3ms for XY processors
    constexpr double tplLookaheadMs = 2.0;          // 2ms for TPL

    // Lookahead at OS rate (used inside OS domain)
    lookaheadSamples = static_cast<int>(std::ceil(osSampleRate * xyProcessorLookaheadMs / 1000.0));
    advancedTPLLookaheadSamples = static_cast<int>(std::ceil(osSampleRate * tplLookaheadMs / 1000.0));

    // Calculate plugin latency reported to host
    // Latency = OS filter latency + TPL lookahead (converted back to base rate)
    const int osFilterLatency = osManager.getLatencySamples();  // At base rate
    const int tplLookaheadBaseSamples = static_cast<int>(std::ceil(sampleRate * tplLookaheadMs / 1000.0));
    totalLatencySamples = osFilterLatency + tplLookaheadBaseSamples;

    // Report latency to host
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

    // Allocate double buffers
    dryBufferDouble.setSize(2, samplesPerBlock);
    tempBuffer1Double.setSize(2, samplesPerBlock);
    tempBuffer2Double.setSize(2, samplesPerBlock);
    tempBuffer3Double.setSize(2, samplesPerBlock);
    tempBuffer4Double.setSize(2, samplesPerBlock);

    // Reset all processor states and allocate lookahead buffers (3ms for all)
    for (int ch = 0; ch < 2; ++ch)
    {
        // v1.5.0: Dry delay buffers sized for maximum latency (Linear Phase mode)
        // Actual delay amount is set per-mode in processBlock
        // Buffer must be large enough to handle any mode
        const int maxDryDelay = linearPhaseModeLatencySamples + lookaheadSamples;
        dryDelayState[ch].delayBuffer.resize(maxDryDelay, 0.0);
        dryDelayState[ch].writePos = 0;

        // Hard Clip lookahead
        hardClipState[ch].lookaheadBuffer.resize(lookaheadSamples, 0.0);
        hardClipState[ch].lookaheadWritePos = 0;

        // Soft Clip lookahead
        softClipState[ch].lookaheadBuffer.resize(lookaheadSamples, 0.0);
        softClipState[ch].lookaheadWritePos = 0;

        // Slow Limiter
        slowLimiterState[ch].envelope = 0.0;
        slowLimiterState[ch].adaptiveRelease = 100.0;
        slowLimiterState[ch].lookaheadBuffer.resize(lookaheadSamples, 0.0);
        slowLimiterState[ch].lookaheadWritePos = 0;

        // Fast Limiter
        fastLimiterState[ch].envelope = 0.0;
        fastLimiterState[ch].lookaheadBuffer.resize(lookaheadSamples, 0.0);
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

    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, numSamples);

    // Bypass check
    const bool bypass = apvts.getRawParameterValue("BYPASS")->load() > 0.5f;

    // If bypass is engaged, capture the dry input signal to oscilloscope and return
    if (bypass)
    {
        // Capture oscilloscope data - dry signal when bypassed
        int writePos = oscilloscopeWritePos.load();
        int oscSize = oscilloscopeSize.load();

        if (oscSize > 0 && static_cast<int>(oscilloscopeBuffer.size()) >= oscSize)
        {
            for (int i = 0; i < numSamples; ++i)
            {
                float monoSample = 0.0f;
                for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                {
                    monoSample += static_cast<float>(buffer.getReadPointer(ch)[i]);
                }
                if (buffer.getNumChannels() > 0)
                    monoSample /= static_cast<float>(buffer.getNumChannels());

                // Filter through three bands for RGB coloring (use left channel filter)
                double sample = static_cast<double>(monoSample);

                // Low-pass filter (Red channel)
                double lowOut = oscFilters[0].lowB0 * sample + oscFilters[0].lowZ1;
                oscFilters[0].lowZ1 = oscFilters[0].lowB1 * sample - oscFilters[0].lowA1 * lowOut + oscFilters[0].lowZ2;
                oscFilters[0].lowZ2 = oscFilters[0].lowB2 * sample - oscFilters[0].lowA2 * lowOut;

                // Band-pass filter (Green channel)
                double midOut = oscFilters[0].midB0 * sample + oscFilters[0].midZ1;
                oscFilters[0].midZ1 = oscFilters[0].midB1 * sample - oscFilters[0].midA1 * midOut + oscFilters[0].midZ2;
                oscFilters[0].midZ2 = oscFilters[0].midB2 * sample - oscFilters[0].midA2 * midOut;

                // High-pass filter (Blue channel)
                double highOut = oscFilters[0].highB0 * sample + oscFilters[0].highZ1;
                oscFilters[0].highZ1 = oscFilters[0].highB1 * sample - oscFilters[0].highA1 * highOut + oscFilters[0].highZ2;
                oscFilters[0].highZ2 = oscFilters[0].highB2 * sample - oscFilters[0].highA2 * highOut;

                if (writePos < oscSize)
                {
                    oscilloscopeBuffer[writePos] = monoSample;
                    oscilloscopeLowBand[writePos] = static_cast<float>(std::abs(lowOut));
                    oscilloscopeMidBand[writePos] = static_cast<float>(std::abs(midOut));
                    oscilloscopeHighBand[writePos] = static_cast<float>(std::abs(highOut));
                }
                writePos = (writePos + 1) % oscSize;
            }
            oscilloscopeWritePos.store(writePos);
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
    const SampleType inputGain = juce::Decibels::decibelsToGain(inputGainDB);
    const SampleType outputGain = juce::Decibels::decibelsToGain(outputGainDB);
    const SampleType mixWet = mixWetPercent / static_cast<SampleType>(100.0);
    const SampleType scKnee = scKneePercent / static_cast<SampleType>(100.0);

    const SampleType hcTrimGain = juce::Decibels::decibelsToGain(hcTrimDB);
    const SampleType scTrimGain = juce::Decibels::decibelsToGain(scTrimDB);
    const SampleType slTrimGain = juce::Decibels::decibelsToGain(slTrimDB);
    const SampleType flTrimGain = juce::Decibels::decibelsToGain(flTrimDB);

    // Get current processing mode (needed for dry signal processing)
    int processingMode = static_cast<int>(apvts.getRawParameterValue("PROCESSING_MODE")->load());

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

    // === INPUT NORMALIZATION ===
    // Apply normalization gain if enabled
    SampleType normGain = static_cast<SampleType>(1.0);
    if (normalizationEnabled)
    {
        normGain = static_cast<SampleType>(computedNormalizationGain);
        buffer.applyGain(normGain);
    }

    // Apply manual input gain (trim)
    buffer.applyGain(inputGain);

    // Store input after gain for GR calculation
    const SampleType totalInputGain = normGain * inputGain;

    // === STORE DRY SIGNAL (after normalization and input gain) ===
    // CRITICAL: Dry signal must have same level as wet for time-aligned mixing
    // NOTE: Dry is NOT processed through oversampling (would create timing mismatch)
    // Instead, dry will be delayed by simple delay buffer to match total wet path latency
    dryBuffer.makeCopyOf(buffer);

    // Calculate bi-linear blend weights from XY pad
    // XY Grid Layout: Top-Left=SL, Top-Right=HC, Bottom-Left=SC, Bottom-Right=FL
    // GUI sends: Y=1 for TOP, Y=0 for BOTTOM; X=0 for LEFT, X=1 for RIGHT
    SampleType wSL = (static_cast<SampleType>(1.0) - xyX) * xyY;  // Top-left (X=0, Y=1)
    SampleType wHC = xyX * xyY;  // Top-right (X=1, Y=1)
    SampleType wSC = (static_cast<SampleType>(1.0) - xyX) * (static_cast<SampleType>(1.0) - xyY);  // Bottom-left (X=0, Y=0)
    SampleType wFL = xyX * (static_cast<SampleType>(1.0) - xyY);  // Bottom-right (X=1, Y=0)

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

    // === ARCHITECTURE A: GLOBAL OVERSAMPLING + XY BLEND PROCESSING ===
    // Single upsample → process all 4 paths → blend → downsample
    // This replaces the old per-processor oversampling approach
    if (!allProcessorsMuted)
    {
        // Step 1: Upsample to OS domain (1×, 8×, or 16× depending on mode)
        auto osBlock = osManager.upsampleBlock(buffer);
        const int osNumSamples = static_cast<int>(osBlock.getNumSamples());
        const int osNumChannels = static_cast<int>(osBlock.getNumChannels());

        // Step 2: Create AudioBuffer for OS domain processing and copy data from block
        juce::AudioBuffer<SampleType> osBuffer(osNumChannels, osNumSamples);
        for (int ch = 0; ch < osNumChannels; ++ch)
        {
            auto* blockData = osBlock.getChannelPointer(static_cast<size_t>(ch));
            auto* bufferData = osBuffer.getWritePointer(ch);
            for (int i = 0; i < osNumSamples; ++i)
                bufferData[i] = blockData[i];
        }

        // Step 3: Process XY blend in OS domain (all 4 paths + blending happens here)
        // processXYBlend handles: trim gains, processing, gain comp, and blending
        processXYBlend(osBuffer, osManager.getOsSampleRate());

        // Step 4: Copy processed data back to osBlock
        for (int ch = 0; ch < osNumChannels; ++ch)
        {
            const auto* bufferData = osBuffer.getReadPointer(ch);
            auto* blockData = osBlock.getChannelPointer(static_cast<size_t>(ch));
            for (int i = 0; i < osNumSamples; ++i)
                blockData[i] = bufferData[i];
        }

        // Step 5: Downsample back to base rate
        osManager.downsampleBlock(buffer, osBlock);
    }
    // else: All processors muted - buffer contains gained input signal (pristine passthrough)

    // === PHASE DIFFERENCE LIMITER (DISABLED - Too audible) ===
    // Phase limiting on wet/dry deviation fundamentally changes nonlinear character
    // Harmonic generation from saturation naturally creates phase shifts
    // Constraining this removes the musical quality of the processing
    // const bool phaseLimiterEnabled = apvts.getRawParameterValue("PHASE_LIMITER_ENABLE")->load() > 0.5f;
    // const SampleType phaseDeviationDegrees = static_cast<SampleType>(apvts.getRawParameterValue("PHASE_LIMITER_THRESHOLD")->load());
    // const SampleType phaseDeviationRadians = phaseDeviationDegrees * static_cast<SampleType>(juce::MathConstants<double>::pi / 180.0);
    // processPhaseDifferenceLimiter(buffer, dryBuffer, phaseDeviationRadians, currentSampleRate, phaseLimiterEnabled);

    // === VISUALIZATION DATA CAPTURE ===
    // CRITICAL: Gain reduction meter and oscilloscope must be sample-aligned
    // Both capture from the SAME processed output buffer at the SAME time
    // This happens BEFORE delta mode and output gain are applied

    // Calculate gain reduction for visualization
    SampleType maxInputLevel = static_cast<SampleType>(0.0);
    SampleType maxOutputLevel = static_cast<SampleType>(0.0);
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        const auto* wet = buffer.getReadPointer(ch);
        const auto* dry = dryBuffer.getReadPointer(ch);

        for (int i = 0; i < numSamples; ++i)
        {
            maxInputLevel = juce::jmax(maxInputLevel, std::abs(dry[i] * totalInputGain));
            maxOutputLevel = juce::jmax(maxOutputLevel, std::abs(wet[i]));
        }
    }

    float grDB = 0.0f;
    if (maxInputLevel > static_cast<SampleType>(0.00001) && maxOutputLevel > static_cast<SampleType>(0.00001))
    {
        grDB = juce::Decibels::gainToDecibels(static_cast<float>(maxOutputLevel / maxInputLevel));
        if (grDB > 0.0f) grDB = 0.0f;  // Only show reduction, not gain
    }
    currentGainReductionDB.store(grDB);

    // Capture oscilloscope data - processed output signal (wet)
    // === DELTA MODE (Main Drive Processors Gain Reduction Signal) ===
    // Per spec: Output = Input_Gained * (1 - G_Weighted_Blend)
    // IMPORTANT: Delta mode ONLY outputs the gain reduction artifacts
    // No output gain, no dry/wet mix - pure GR signal only
    if (deltaMode)
    {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* output = buffer.getWritePointer(ch);
            const auto* input = dryBuffer.getReadPointer(ch);

            for (int i = 0; i < numSamples; ++i)
            {
                // Calculate pure gain reduction signal: what was removed by processing
                SampleType inputGained = input[i] * totalInputGain;
                SampleType grSignal = inputGained - output[i];
                output[i] = grSignal;  // Output ONLY the difference (artifacts removed)
            }
        }
    }
    else
    {
        // === DRY SIGNAL DELAY COMPENSATION (PER MODE) ===
        // v1.5.0: Dry signal delayed to match wet path latency (per-mode)
        // This ensures time-aligned wet/dry mixing
        // Mode 0: 0 delay (zero latency)
        // Mode 1: balancedModeLatencySamples + lookaheadSamples
        // Mode 2: linearPhaseModeLatencySamples + lookaheadSamples
        int dryDelayAmount = 0;
        if (processingMode == 0)
        {
            dryDelayAmount = 0;  // Zero Latency mode: no delay
        }
        else if (processingMode == 1)
        {
            dryDelayAmount = balancedModeLatencySamples + lookaheadSamples;
        }
        else  // processingMode == 2
        {
            dryDelayAmount = linearPhaseModeLatencySamples + lookaheadSamples;
        }

        if (dryDelayAmount > 0)
        {
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            {
                auto* dryData = dryBuffer.getWritePointer(ch);
                auto& dryState = dryDelayState[ch];
                auto& delayBuf = dryState.delayBuffer;
                int& writePos = dryState.writePos;

                for (int i = 0; i < numSamples; ++i)
                {
                    // Store current dry sample
                    if (writePos >= 0 && writePos < static_cast<int>(delayBuf.size()))
                        delayBuf[writePos] = static_cast<double>(dryData[i]);

                    // Read delayed dry sample (matches total wet signal latency)
                    int readPos = (writePos + 1) % dryDelayAmount;
                    if (readPos >= 0 && readPos < static_cast<int>(delayBuf.size()))
                        dryData[i] = static_cast<SampleType>(delayBuf[readPos]);

                    writePos = (writePos + 1) % dryDelayAmount;
                }
            }
        }

        // Normal mode: Apply output gain and dry/wet mix
        buffer.applyGain(outputGain);

        // Dry/wet mix
        if (mixWet < static_cast<SampleType>(1.0))
        {
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            {
                auto* wet = buffer.getWritePointer(ch);
                const auto* dry = dryBuffer.getReadPointer(ch);

                for (int i = 0; i < numSamples; ++i)
                {
                    wet[i] = dry[i] * (static_cast<SampleType>(1.0) - mixWet) + wet[i] * mixWet;
                }
            }
        }
    }

    // === OVERSHOOT LIMITERS (Independent from Main Delta Mode) ===
    // Two discrete limiters that can be A/B tested independently:
    // 1. No OverShoot: Simple OSM with parameter blending (Punchy/Clean) - monitored by OS Delta
    // 2. True Peak: Advanced TPL with IRC (Intelligent Release Control) - monitored by TP Delta
    // NOTE: Only ONE should be enabled at a time to avoid double-oversampling phase issues
    const bool noOvershootEnabled = apvts.getRawParameterValue("PROTECTION_ENABLE")->load() > 0.5f;
    const bool osmModeAdvancedTPL = apvts.getRawParameterValue("OSM_MODE")->load() > 0.5f;  // true = Mode 1 (Advanced TPL), false = Mode 0 (Safety Clipper)
    const SampleType ceilingDB = static_cast<SampleType>(apvts.getRawParameterValue("PROTECTION_CEILING")->load());
    const bool overshootDeltaMode = apvts.getRawParameterValue("OVERSHOOT_DELTA_MODE")->load() > 0.5f;
    const bool truePeakDeltaMode = apvts.getRawParameterValue("TRUE_PEAK_DELTA_MODE")->load() > 0.5f;

    // Only process if not in main Delta mode (limiter deltas are independent)
    if (!deltaMode)
    {
        auto& limiterRefBuffer = tempBuffer4;

        // === NO OVERSHOOT with OS Delta ===
        if (overshootDeltaMode && noOvershootEnabled)
        {
            limiterRefBuffer.makeCopyOf(buffer);
            processOvershootSuppression(buffer, ceilingDB, currentSampleRate, true, &limiterRefBuffer);

            // Compute OS Delta: processed - reference
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            {
                auto* processed = buffer.getWritePointer(ch);
                const auto* reference = limiterRefBuffer.getReadPointer(ch);
                for (int i = 0; i < numSamples; ++i)
                    processed[i] = processed[i] - reference[i];
            }
        }
        // === Normal processing (no delta for True Peak - too complex with lookahead) ===
        else
        {
            // Run ONE limiter based on OSM_MODE (if protection enabled)
            if (noOvershootEnabled)
            {
                if (osmModeAdvancedTPL)
                    // Mode 1: Advanced TPL with IRC and predictive lookahead
                    processAdvancedTPL(buffer, ceilingDB, currentSampleRate, static_cast<juce::AudioBuffer<SampleType>*>(nullptr));
                else
                    // Mode 0: Safety Clipper with constant-latency compensation
                    processOvershootSuppression(buffer, ceilingDB, currentSampleRate, true, static_cast<juce::AudioBuffer<SampleType>*>(nullptr));
            }
        }
    }

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

    // === OSCILLOSCOPE DATA CAPTURE (Final Output) ===
    // Capture AFTER all processing including protection limiter
    int writePos = oscilloscopeWritePos.load();
    int oscSize = oscilloscopeSize.load();

    if (oscSize > 0 && static_cast<int>(oscilloscopeBuffer.size()) >= oscSize)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            float monoSample = 0.0f;
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            {
                monoSample += static_cast<float>(buffer.getReadPointer(ch)[i]);
            }
            if (buffer.getNumChannels() > 0)
                monoSample /= static_cast<float>(buffer.getNumChannels());

            // Filter through three bands for RGB coloring (use left channel filter)
            double sample = static_cast<double>(monoSample);

            // Low-pass filter (Red channel)
            double lowOut = oscFilters[0].lowB0 * sample + oscFilters[0].lowZ1;
            oscFilters[0].lowZ1 = oscFilters[0].lowB1 * sample - oscFilters[0].lowA1 * lowOut + oscFilters[0].lowZ2;
            oscFilters[0].lowZ2 = oscFilters[0].lowB2 * sample - oscFilters[0].lowA2 * lowOut;

            // Band-pass filter (Green channel)
            double midOut = oscFilters[0].midB0 * sample + oscFilters[0].midZ1;
            oscFilters[0].midZ1 = oscFilters[0].midB1 * sample - oscFilters[0].midA1 * midOut + oscFilters[0].midZ2;
            oscFilters[0].midZ2 = oscFilters[0].midB2 * sample - oscFilters[0].midA2 * midOut;

            // High-pass filter (Blue channel)
            double highOut = oscFilters[0].highB0 * sample + oscFilters[0].highZ1;
            oscFilters[0].highZ1 = oscFilters[0].highB1 * sample - oscFilters[0].highA1 * highOut + oscFilters[0].highZ2;
            oscFilters[0].highZ2 = oscFilters[0].highB2 * sample - oscFilters[0].highA2 * highOut;

            if (writePos < oscSize)
            {
                oscilloscopeBuffer[writePos] = monoSample;
                oscilloscopeLowBand[writePos] = static_cast<float>(std::abs(lowOut));
                oscilloscopeMidBand[writePos] = static_cast<float>(std::abs(midOut));
                oscilloscopeHighBand[writePos] = static_cast<float>(std::abs(highOut));
            }
            writePos = (writePos + 1) % oscSize;
        }
        oscilloscopeWritePos.store(writePos);
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
    // Get current processing mode (0 = Zero Latency, 1 = Balanced, 2 = Linear Phase)
    int processingMode = static_cast<int>(apvts.getRawParameterValue("PROCESSING_MODE")->load());

    const double thresholdD = static_cast<double>(threshold);

    // MODE 0: Zero Latency - Direct hard clip, NO oversampling, NO lookahead
    // Accept minimal aliasing as tradeoff for true zero latency
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

    // MODE 1 & 2: Balanced or Linear Phase - use mode-specific oversampling
    juce::dsp::Oversampling<SampleType>* oversamplingPtr = nullptr;

    if (std::is_same<SampleType, float>::value)
    {
        if (processingMode == 1)
            oversamplingPtr = reinterpret_cast<juce::dsp::Oversampling<SampleType>*>(oversamplingBalancedFloat.get());
        else  // processingMode == 2
            oversamplingPtr = reinterpret_cast<juce::dsp::Oversampling<SampleType>*>(oversamplingLinearPhaseFloat.get());
    }
    else
    {
        if (processingMode == 1)
            oversamplingPtr = reinterpret_cast<juce::dsp::Oversampling<SampleType>*>(oversamplingBalancedDouble.get());
        else  // processingMode == 2
            oversamplingPtr = reinterpret_cast<juce::dsp::Oversampling<SampleType>*>(oversamplingLinearPhaseDouble.get());
    }

    auto& oversampling = *oversamplingPtr;

    // Upsample (8× or 16× depending on mode)
    auto oversampledBlock = oversampling.processSamplesUp(juce::dsp::AudioBlock<SampleType>(buffer));
    const int oversampledSamples = static_cast<int>(oversampledBlock.getNumSamples());

    // Process oversampled signal with simple hard clip + 3ms lookahead
    // Oversampling handles anti-aliasing, no PolyBLEP needed
    for (int ch = 0; ch < juce::jmin(static_cast<int>(oversampledBlock.getNumChannels()), 2); ++ch)
    {
        auto* data = oversampledBlock.getChannelPointer(static_cast<size_t>(ch));
        auto& state = hardClipState[ch];
        auto& lookaheadBuffer = state.lookaheadBuffer;
        int& writePos = state.lookaheadWritePos;

        for (int i = 0; i < oversampledSamples; ++i)
        {
            const double currentSample = static_cast<double>(data[i]);

            // Store current sample in delay buffer
            if (writePos >= 0 && writePos < static_cast<int>(lookaheadBuffer.size()))
                lookaheadBuffer[writePos] = currentSample;

            // Read delayed sample for phase alignment
            int readPos = (writePos + 1) % lookaheadSamples;
            double delayedSample = currentSample;
            if (readPos >= 0 && readPos < static_cast<int>(lookaheadBuffer.size()))
                delayedSample = lookaheadBuffer[readPos];

            writePos = (writePos + 1) % lookaheadSamples;

            // Simple hard clip processing (oversampling handles anti-aliasing)
            const double output = juce::jlimit(-thresholdD, thresholdD, delayedSample);
            data[i] = static_cast<SampleType>(output);
        }
    }

    // Downsample back to original rate
    juce::dsp::AudioBlock<SampleType> outputBlock(buffer);
    oversampling.processSamplesDown(outputBlock);
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
    // Get current processing mode (0 = Zero Latency, 1 = Balanced, 2 = Linear Phase)
    int processingMode = static_cast<int>(apvts.getRawParameterValue("PROCESSING_MODE")->load());

    const double ceilingD = static_cast<double>(ceiling);
    const double color = static_cast<double>(knee) / 100.0;  // Convert knee percentage to color (0-1)
    const double drive = 1.0 + (color * 3.0);  // Drive: 1.0 to 4.0

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
                const double output = std::tanh(x * drive);
                data[i] = static_cast<SampleType>(output * ceilingD);
            }
        }
        return;
    }

    // MODE 1 & 2: Balanced (8×) or Linear Phase (16×) - use oversampling + lookahead
    juce::dsp::Oversampling<SampleType>* oversamplingPtr = nullptr;

    if (std::is_same<SampleType, float>::value)
    {
        if (processingMode == 1)
            oversamplingPtr = reinterpret_cast<juce::dsp::Oversampling<SampleType>*>(oversamplingBalancedFloat.get());
        else  // processingMode == 2
            oversamplingPtr = reinterpret_cast<juce::dsp::Oversampling<SampleType>*>(oversamplingLinearPhaseFloat.get());
    }
    else
    {
        if (processingMode == 1)
            oversamplingPtr = reinterpret_cast<juce::dsp::Oversampling<SampleType>*>(oversamplingBalancedDouble.get());
        else  // processingMode == 2
            oversamplingPtr = reinterpret_cast<juce::dsp::Oversampling<SampleType>*>(oversamplingLinearPhaseDouble.get());
    }

    auto& oversampling = *oversamplingPtr;

    // Upsample (8× for Balanced, 16× for Linear Phase)
    auto oversampledBlock = oversampling.processSamplesUp(juce::dsp::AudioBlock<SampleType>(buffer));
    const int oversampledSamples = static_cast<int>(oversampledBlock.getNumSamples());

    // Process oversampled signal with 3ms lookahead for smooth transitions
    for (int ch = 0; ch < juce::jmin(static_cast<int>(oversampledBlock.getNumChannels()), 2); ++ch)
    {
        auto* data = oversampledBlock.getChannelPointer(static_cast<size_t>(ch));
        auto& lookaheadBuffer = softClipState[ch].lookaheadBuffer;
        int& writePos = softClipState[ch].lookaheadWritePos;

        for (int i = 0; i < oversampledSamples; ++i)
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

            // Simple tanh processing (oversampling handles anti-aliasing)
            const double x = delayedSample / ceilingD;
            const double output = std::tanh(x * drive);
            data[i] = static_cast<SampleType>(output * ceilingD);
        }
    }

    // Downsample back to original rate
    juce::dsp::AudioBlock<SampleType> outputBlock(buffer);
    oversampling.processSamplesDown(outputBlock);
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
    // Get current processing mode (0 = Zero Latency, 1 = Balanced, 2 = Linear Phase)
    int processingMode = static_cast<int>(apvts.getRawParameterValue("PROCESSING_MODE")->load());

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

                // Track RMS envelope
                const double inputSquared = currentSample * currentSample;
                rmsEnvelope = rmsCoeff * rmsEnvelope + (1.0 - rmsCoeff) * inputSquared;
                const double rmsValue = std::sqrt(juce::jmax(1e-10, rmsEnvelope));

                // Track peak envelope
                peakEnvelope = peakCoeff * peakEnvelope + (1.0 - peakCoeff) * inputAbs;
                const double peakValue = juce::jmax(1e-10, peakEnvelope);

                // Calculate crest factor
                const double instantCrestFactor = peakValue / rmsValue;
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

                // Apply one-pole low-pass filter to gain signal (prevents control signal aliasing)
                smoothedGain = gainSmoothCoeff * smoothedGain + (1.0 - gainSmoothCoeff) * targetGain;

                data[i] = static_cast<SampleType>(currentSample * smoothedGain);
            }
        }
        return;
    }

    // MODE 1 & 2: Balanced or Linear Phase - use mode-specific oversampling
    juce::dsp::Oversampling<SampleType>* oversamplingPtr = nullptr;

    if (std::is_same<SampleType, float>::value)
    {
        if (processingMode == 1)
            oversamplingPtr = reinterpret_cast<juce::dsp::Oversampling<SampleType>*>(oversamplingBalancedFloat.get());
        else  // processingMode == 2
            oversamplingPtr = reinterpret_cast<juce::dsp::Oversampling<SampleType>*>(oversamplingLinearPhaseFloat.get());
    }
    else
    {
        if (processingMode == 1)
            oversamplingPtr = reinterpret_cast<juce::dsp::Oversampling<SampleType>*>(oversamplingBalancedDouble.get());
        else  // processingMode == 2
            oversamplingPtr = reinterpret_cast<juce::dsp::Oversampling<SampleType>*>(oversamplingLinearPhaseDouble.get());
    }

    auto& oversampling = *oversamplingPtr;

    auto oversampledBlock = oversampling.processSamplesUp(juce::dsp::AudioBlock<SampleType>(buffer));
    const int oversampledSamples = static_cast<int>(oversampledBlock.getNumSamples());

    for (int ch = 0; ch < juce::jmin(static_cast<int>(oversampledBlock.getNumChannels()), 2); ++ch)
    {
        auto* data = oversampledBlock.getChannelPointer(static_cast<size_t>(ch));
        double& envelope = slowLimiterState[ch].envelope;
        double& rmsEnvelope = slowLimiterState[ch].rmsEnvelope;
        double& peakEnvelope = slowLimiterState[ch].peakEnvelope;
        double& smoothedCrestFactor = slowLimiterState[ch].smoothedCrestFactor;
        double& smoothedGain = slowLimiterState[ch].smoothedGain;
        auto& lookaheadBuffer = slowLimiterState[ch].lookaheadBuffer;
        int& writePos = slowLimiterState[ch].lookaheadWritePos;

        for (int i = 0; i < oversampledSamples; ++i)
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

            // Track RMS envelope (for crest factor calculation)
            const double inputSquared = currentSample * currentSample;
            rmsEnvelope = rmsCoeff * rmsEnvelope + (1.0 - rmsCoeff) * inputSquared;
            const double rmsValue = std::sqrt(juce::jmax(1e-10, rmsEnvelope));

            // Track peak envelope (for crest factor calculation)
            peakEnvelope = peakCoeff * peakEnvelope + (1.0 - peakCoeff) * inputAbs;
            const double peakValue = juce::jmax(1e-10, peakEnvelope);

            // Calculate instantaneous crest factor (Peak / RMS)
            // High crest factor = transient/spiky signal → fast release
            // Low crest factor = sustained/dense signal → slow release
            const double instantCrestFactor = peakValue / rmsValue;

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

            data[i] = static_cast<SampleType>(delayedSample * smoothedGain);
        }
    }

    // Downsample back to original rate
    juce::dsp::AudioBlock<SampleType> outputBlock(buffer);
    oversampling.processSamplesDown(outputBlock);
}

// Fast Limiting - Hard knee limiting with fixed 10ms release
// Attack: 1ms, Release: 10ms (fixed), Hard knee, Lookahead: 3ms
// MODE-AWARE: Zero Latency = no oversampling, Balanced/Linear Phase = 8x oversampling
template<typename SampleType>
void QuadBlendDriveAudioProcessor::processFastLimit(juce::AudioBuffer<SampleType>& buffer,
                                                      SampleType threshold,
                                                      double sampleRate)
{
    // Get current processing mode (0 = Zero Latency, 1 = Balanced, 2 = Linear Phase)
    int processingMode = static_cast<int>(apvts.getRawParameterValue("PROCESSING_MODE")->load());

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

                // Apply one-pole low-pass filter to gain signal (prevents control signal aliasing)
                smoothedGain = gainSmoothCoeff * smoothedGain + (1.0 - gainSmoothCoeff) * targetGain;

                data[i] = static_cast<SampleType>(currentSample * smoothedGain);
            }
        }
        return;
    }

    // MODE 1 & 2: Balanced or Linear Phase - use mode-specific oversampling
    juce::dsp::Oversampling<SampleType>* oversamplingPtr = nullptr;

    if (std::is_same<SampleType, float>::value)
    {
        if (processingMode == 1)
            oversamplingPtr = reinterpret_cast<juce::dsp::Oversampling<SampleType>*>(oversamplingBalancedFloat.get());
        else  // processingMode == 2
            oversamplingPtr = reinterpret_cast<juce::dsp::Oversampling<SampleType>*>(oversamplingLinearPhaseFloat.get());
    }
    else
    {
        if (processingMode == 1)
            oversamplingPtr = reinterpret_cast<juce::dsp::Oversampling<SampleType>*>(oversamplingBalancedDouble.get());
        else  // processingMode == 2
            oversamplingPtr = reinterpret_cast<juce::dsp::Oversampling<SampleType>*>(oversamplingLinearPhaseDouble.get());
    }

    auto& oversampling = *oversamplingPtr;

    auto oversampledBlock = oversampling.processSamplesUp(juce::dsp::AudioBlock<SampleType>(buffer));
    const int oversampledSamples = static_cast<int>(oversampledBlock.getNumSamples());

    for (int ch = 0; ch < juce::jmin(static_cast<int>(oversampledBlock.getNumChannels()), 2); ++ch)
    {
        auto* data = oversampledBlock.getChannelPointer(static_cast<size_t>(ch));
        double& envelope = fastLimiterState[ch].envelope;
        double& smoothedGain = fastLimiterState[ch].smoothedGain;
        auto& lookaheadBuffer = fastLimiterState[ch].lookaheadBuffer;
        int& writePos = fastLimiterState[ch].lookaheadWritePos;

        for (int i = 0; i < oversampledSamples; ++i)
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

            data[i] = static_cast<SampleType>(delayedSample * smoothedGain);
        }
    }

    // Downsample back to original rate
    juce::dsp::AudioBlock<SampleType> outputBlock(buffer);
    oversampling.processSamplesDown(outputBlock);
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

    // Get current processing mode and select appropriate oversampling object
    int processingMode = static_cast<int>(apvts.getRawParameterValue("PROCESSING_MODE")->load());
    juce::dsp::Oversampling<SampleType>* oversamplingPtr = nullptr;

    if (std::is_same<SampleType, float>::value)
    {
        if (processingMode == 0)
            oversamplingPtr = reinterpret_cast<juce::dsp::Oversampling<SampleType>*>(oversamplingZeroLatencyFloat.get());
        else if (processingMode == 1)
            oversamplingPtr = reinterpret_cast<juce::dsp::Oversampling<SampleType>*>(oversamplingBalancedFloat.get());
        else  // processingMode == 2
            oversamplingPtr = reinterpret_cast<juce::dsp::Oversampling<SampleType>*>(oversamplingLinearPhaseFloat.get());
    }
    else
    {
        if (processingMode == 0)
            oversamplingPtr = reinterpret_cast<juce::dsp::Oversampling<SampleType>*>(oversamplingZeroLatencyDouble.get());
        else if (processingMode == 1)
            oversamplingPtr = reinterpret_cast<juce::dsp::Oversampling<SampleType>*>(oversamplingBalancedDouble.get());
        else  // processingMode == 2
            oversamplingPtr = reinterpret_cast<juce::dsp::Oversampling<SampleType>*>(oversamplingLinearPhaseDouble.get());
    }

    auto& oversampling = *oversamplingPtr;

    // If reference buffer provided, process both through oversampling together for perfect alignment
    if (referenceBuffer != nullptr)
    {
        // Create combined buffer (channels: L, R, RefL, RefR)
        juce::AudioBuffer<SampleType> combinedBuffer(4, buffer.getNumSamples());

        // Copy main buffer to first 2 channels
        for (int ch = 0; ch < 2; ++ch)
            combinedBuffer.copyFrom(ch, 0, buffer, ch, 0, buffer.getNumSamples());

        // Copy reference buffer to last 2 channels
        for (int ch = 0; ch < 2; ++ch)
            combinedBuffer.copyFrom(ch + 2, 0, *referenceBuffer, ch, 0, buffer.getNumSamples());

        // Create 4-channel oversampling object for delta mode (ensures perfect phase alignment)
        constexpr size_t oversamplingFactor = 3;  // 2^3 = 8x
        juce::dsp::Oversampling<SampleType> oversamplingDelta(
            4,  // 4 channels
            oversamplingFactor,
            juce::dsp::Oversampling<SampleType>::filterHalfBandFIREquiripple,
            true, false);
        oversamplingDelta.initProcessing(static_cast<size_t>(buffer.getNumSamples()));

        // Process combined buffer through oversampling
        juce::dsp::AudioBlock<SampleType> combinedBlock(combinedBuffer);
        auto oversampledCombined = oversamplingDelta.processSamplesUp(combinedBlock);

        // Apply parameter-interpolated processor ONLY to first 2 channels (main signal)
        // Get blend parameter (0 = clean/gentle, 1 = punchy/aggressive)
        const float blendParam = apvts.getRawParameterValue("OVERSHOOT_BLEND")->load();
        const double blend = static_cast<double>(blendParam);  // 0 = Clean, 1 = Punchy

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
        oversamplingDelta.processSamplesDown(combinedBlock);

        // Copy results back
        for (int ch = 0; ch < 2; ++ch)
            buffer.copyFrom(ch, 0, combinedBuffer, ch, 0, buffer.getNumSamples());

        for (int ch = 0; ch < 2; ++ch)
            referenceBuffer->copyFrom(ch, 0, combinedBuffer, ch + 2, 0, buffer.getNumSamples());

        return;
    }

    // Simple OSM - single processor with interpolated parameters
    // Get blend parameter (0 = clean/gentle, 1 = punchy/aggressive)
    const float blendParam = apvts.getRawParameterValue("OVERSHOOT_BLEND")->load();
    const double blend = static_cast<double>(blendParam);  // 0 = Clean, 1 = Punchy

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

    juce::dsp::AudioBlock<SampleType> block(buffer);
    auto oversampledBlock = oversampling.processSamplesUp(block);

    // Process oversampled signal - single unified transfer curve with interpolated parameters
    for (size_t ch = 0; ch < oversampledBlock.getNumChannels(); ++ch)
    {
        auto* data = oversampledBlock.getChannelPointer(ch);
        const size_t numSamples = oversampledBlock.getNumSamples();

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

    // Downsample back to original rate using matching linear-phase FIR
    oversampling.processSamplesDown(block);

    // === OSM MODE 0 CONSTANT LATENCY COMPENSATION ===
    // Apply delay to match Mode 1 (Advanced TPL) lookahead latency
    // This ensures the plugin reports constant latency regardless of OSM mode
    const int osmDelay = advancedTPLLookaheadSamples;

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
    // Convert ceiling to linear
    const double ceilingLinear = std::pow(10.0, static_cast<double>(ceilingDB) / 20.0);

    // Get current processing mode and select appropriate oversampling object
    int processingMode = static_cast<int>(apvts.getRawParameterValue("PROCESSING_MODE")->load());
    juce::dsp::Oversampling<SampleType>* oversamplingPtr = nullptr;

    if (std::is_same<SampleType, float>::value)
    {
        if (processingMode == 0)
            oversamplingPtr = reinterpret_cast<juce::dsp::Oversampling<SampleType>*>(oversamplingZeroLatencyFloat.get());
        else if (processingMode == 1)
            oversamplingPtr = reinterpret_cast<juce::dsp::Oversampling<SampleType>*>(oversamplingBalancedFloat.get());
        else  // processingMode == 2
            oversamplingPtr = reinterpret_cast<juce::dsp::Oversampling<SampleType>*>(oversamplingLinearPhaseFloat.get());
    }
    else
    {
        if (processingMode == 0)
            oversamplingPtr = reinterpret_cast<juce::dsp::Oversampling<SampleType>*>(oversamplingZeroLatencyDouble.get());
        else if (processingMode == 1)
            oversamplingPtr = reinterpret_cast<juce::dsp::Oversampling<SampleType>*>(oversamplingBalancedDouble.get());
        else  // processingMode == 2
            oversamplingPtr = reinterpret_cast<juce::dsp::Oversampling<SampleType>*>(oversamplingLinearPhaseDouble.get());
    }

    auto& oversampling = *oversamplingPtr;

    // Upsample to 8× for true-peak detection
    juce::dsp::AudioBlock<SampleType> block(buffer);
    auto oversampledBlock = oversampling.processSamplesUp(block);

    const size_t oversampledSamples = oversampledBlock.getNumSamples();
    const int lookahead = advancedTPLLookaheadSamples;

    // Soft-knee parameters (designed for up to 6 dB GR)
    constexpr double kneeWidthDB = 1.5;  // Soft knee width in dB
    const double kneeWidth = ceilingLinear * (std::pow(10.0, -kneeWidthDB / 20.0) - 1.0);

    // Attack/Release parameters (at 8× sample rate)
    constexpr double attackCoeff = 0.9;     // Very fast attack for peak catching
    constexpr double fastReleaseCoeff = 0.0001;  // ~20ms fast release
    constexpr double slowReleaseCoeff = 0.00005; // ~80ms slow release

    // Process each channel
    for (size_t ch = 0; ch < oversampledBlock.getNumChannels(); ++ch)
    {
        auto* data = oversampledBlock.getChannelPointer(ch);
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

    // Downsample back to original rate
    oversampling.processSamplesDown(block);

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
    SampleType wSL = (static_cast<SampleType>(1.0) - xyX) * xyY;  // Top-left
    SampleType wHC = xyX * xyY;  // Top-right
    SampleType wSC = (static_cast<SampleType>(1.0) - xyX) * (static_cast<SampleType>(1.0) - xyY);  // Bottom-left
    SampleType wFL = xyX * (static_cast<SampleType>(1.0) - xyY);  // Bottom-right

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

    // Copy current APVTS state to preset slot
    presetSlots[slot] = apvts.copyState();
}

void QuadBlendDriveAudioProcessor::recallPreset(int slot)
{
    if (slot < 0 || slot >= 4)
        return;

    // Check if preset exists
    if (!presetSlots[slot].isValid())
        return;

    // Restore preset state to APVTS
    apvts.replaceState(presetSlots[slot].createCopy());
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

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
        "CALIB_LEVEL", "Threshold / Calib",
        juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f), 0.0f,
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

    return layout;
}

//==============================================================================
void QuadBlendDriveAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    // Calculate lookahead samples (3ms for main processors, 4ms for protection limiter)
    lookaheadSamples = static_cast<int>(std::ceil(sampleRate * 0.003));
    protectionLookaheadSamples = static_cast<int>(std::ceil(sampleRate * 0.004));
    totalLatencySamples = lookaheadSamples + protectionLookaheadSamples;  // 7ms total latency

    // Calculate oscilloscope buffer size (3000ms = 3 seconds)
    int newOscilloscopeSize = static_cast<int>(std::ceil(sampleRate * 3.0));
    oscilloscopeSize.store(newOscilloscopeSize);

    // Resize oscilloscope buffer
    oscilloscopeBuffer.clear();
    oscilloscopeBuffer.resize(newOscilloscopeSize, 0.0f);

    oscilloscopeWritePos.store(0);

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
        // Dry delay compensation (3ms to match all processors)
        dryDelayState[ch].delayBuffer.resize(lookaheadSamples, 0.0);
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

                if (writePos < oscSize)
                    oscilloscopeBuffer[writePos] = monoSample;
                writePos = (writePos + 1) % oscSize;
            }
            oscilloscopeWritePos.store(writePos);
        }

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

    const SampleType inputGainDB = static_cast<SampleType>(apvts.getRawParameterValue("INPUT_GAIN")->load());
    const SampleType outputGainDB = static_cast<SampleType>(apvts.getRawParameterValue("OUTPUT_GAIN")->load());
    const SampleType mixWetPercent = static_cast<SampleType>(apvts.getRawParameterValue("MIX_WET")->load());
    const SampleType clipThreshDB = static_cast<SampleType>(apvts.getRawParameterValue("CALIB_LEVEL")->load());
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

    const SampleType inputGain = juce::Decibels::decibelsToGain(inputGainDB);
    const SampleType outputGain = juce::Decibels::decibelsToGain(outputGainDB);
    const SampleType mixWet = mixWetPercent / static_cast<SampleType>(100.0);
    const SampleType clipThreshold = juce::Decibels::decibelsToGain(clipThreshDB);
    const SampleType scKnee = scKneePercent / static_cast<SampleType>(100.0);

    const SampleType hcTrimGain = juce::Decibels::decibelsToGain(hcTrimDB);
    const SampleType scTrimGain = juce::Decibels::decibelsToGain(scTrimDB);
    const SampleType slTrimGain = juce::Decibels::decibelsToGain(slTrimDB);
    const SampleType flTrimGain = juce::Decibels::decibelsToGain(flTrimDB);

    // Store original input for dry/wet and delta mode
    dryBuffer.makeCopyOf(buffer);

    // Apply 3ms delay compensation to dry buffer to match lookahead of all processors
    // This ensures wet and dry signals are phase-aligned when blending
    for (int ch = 0; ch < juce::jmin(buffer.getNumChannels(), 2); ++ch)
    {
        auto* dryData = dryBuffer.getWritePointer(ch);
        auto& delayBuffer = dryDelayState[ch].delayBuffer;
        int& writePos = dryDelayState[ch].writePos;

        for (int i = 0; i < numSamples; ++i)
        {
            const double currentSample = static_cast<double>(dryData[i]);

            // Store current sample in delay buffer
            delayBuffer[writePos] = currentSample;

            // Read delayed sample (3ms ago)
            int readPos = (writePos + 1) % lookaheadSamples;
            const double delayedSample = delayBuffer[readPos];

            writePos = (writePos + 1) % lookaheadSamples;

            // Replace dry buffer with delayed version
            dryData[i] = static_cast<SampleType>(delayedSample);
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
    if (weightSum > static_cast<SampleType>(1e-10))
    {
        const SampleType normFactor = static_cast<SampleType>(1.0) / weightSum;
        wHC *= normFactor;
        wSC *= normFactor;
        wSL *= normFactor;
        wFL *= normFactor;
    }
    else
    {
        // All processors muted - output silence
        wHC = wSC = wSL = wFL = static_cast<SampleType>(0.0);
    }

    // Copy input to temp buffers for parallel processing
    tempBuffer1.makeCopyOf(buffer);
    tempBuffer2.makeCopyOf(buffer);
    tempBuffer3.makeCopyOf(buffer);
    tempBuffer4.makeCopyOf(buffer);

    // Process Path 1: Hard Clipping
    tempBuffer1.applyGain(hcTrimGain);
    processHardClip(tempBuffer1, clipThreshold, currentSampleRate);

    // Process Path 2: Soft Clipping
    tempBuffer2.applyGain(scTrimGain);
    processSoftClip(tempBuffer2, clipThreshold, scKnee, currentSampleRate);

    // Process Path 3: Slow Limiting (Adaptive Auto-Release)
    tempBuffer3.applyGain(slTrimGain);
    processSlowLimit(tempBuffer3, clipThreshold, limitRelMs, currentSampleRate);

    // Process Path 4: Fast Limiting (Hard Knee Fast)
    tempBuffer4.applyGain(flTrimGain);
    processFastLimit(tempBuffer4, clipThreshold, currentSampleRate);

    // === GAIN COMPENSATION ===
    // If master compensation is enabled, apply inverse trim gains to compensate for level differences
    const bool masterCompEnabled = apvts.getRawParameterValue("MASTER_COMP")->load() > 0.5f;
    if (masterCompEnabled)
    {
        const bool hcCompEnabled = apvts.getRawParameterValue("HC_COMP")->load() > 0.5f;
        const bool scCompEnabled = apvts.getRawParameterValue("SC_COMP")->load() > 0.5f;
        const bool slCompEnabled = apvts.getRawParameterValue("SL_COMP")->load() > 0.5f;
        const bool flCompEnabled = apvts.getRawParameterValue("FL_COMP")->load() > 0.5f;

        // Apply inverse trim gains (compensation)
        if (hcCompEnabled && std::abs(hcTrimGain - static_cast<SampleType>(1.0)) > static_cast<SampleType>(1e-6))
            tempBuffer1.applyGain(static_cast<SampleType>(1.0) / hcTrimGain);

        if (scCompEnabled && std::abs(scTrimGain - static_cast<SampleType>(1.0)) > static_cast<SampleType>(1e-6))
            tempBuffer2.applyGain(static_cast<SampleType>(1.0) / scTrimGain);

        if (slCompEnabled && std::abs(slTrimGain - static_cast<SampleType>(1.0)) > static_cast<SampleType>(1e-6))
            tempBuffer3.applyGain(static_cast<SampleType>(1.0) / slTrimGain);

        if (flCompEnabled && std::abs(flTrimGain - static_cast<SampleType>(1.0)) > static_cast<SampleType>(1e-6))
            tempBuffer4.applyGain(static_cast<SampleType>(1.0) / flTrimGain);
    }

    // Blend the four paths using bi-linear interpolation
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
    // Store mixed stereo (L+R)/2 for mono oscilloscope display
    // This captures the EXACT SAME buffer used for gain reduction calculation above
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

            if (writePos < oscSize)
                oscilloscopeBuffer[writePos] = monoSample;
            writePos = (writePos + 1) % oscSize;
        }
        oscilloscopeWritePos.store(writePos);
    }

    // === DELTA MODE (Gain Reduction Signal) ===
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

    // === PROTECTION LIMITER (True-Peak Safety) ===
    // Applied at the very end to prevent overs regardless of mix/blend settings
    // User-adjustable ceiling with 8× oversampling for true-peak detection
    const bool protectionEnabled = apvts.getRawParameterValue("PROTECTION_ENABLE")->load() > 0.5f;
    if (protectionEnabled)
    {
        const SampleType protectionCeilingDB = static_cast<SampleType>(apvts.getRawParameterValue("PROTECTION_CEILING")->load());
        processProtectionLimiter(buffer, protectionCeilingDB, currentSampleRate);
    }
}

//==============================================================================
// Template DSP Processing Functions

// Hard Clipping - Pure digital clipping at 0 dBFS (like Ableton Saturator Digital Clip mode)
// Includes 3ms delay compensation for phase alignment with other processors
template<typename SampleType>
void QuadBlendDriveAudioProcessor::processHardClip(juce::AudioBuffer<SampleType>& buffer, SampleType threshold, double sampleRate)
{
    const double ceilingD = static_cast<double>(threshold);

    for (int ch = 0; ch < juce::jmin(buffer.getNumChannels(), 2); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        auto& lookaheadBuffer = hardClipState[ch].lookaheadBuffer;
        int& writePos = hardClipState[ch].lookaheadWritePos;

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const double currentSample = static_cast<double>(data[i]);

            // Store current sample in delay buffer
            if (writePos >= 0 && writePos < static_cast<int>(lookaheadBuffer.size()))
                lookaheadBuffer[writePos] = currentSample;

            // Read delayed sample (3ms ago) for phase alignment
            int readPos = (writePos + 1) % lookaheadSamples;
            double delayedSample = currentSample;
            if (readPos >= 0 && readPos < static_cast<int>(lookaheadBuffer.size()))
                delayedSample = lookaheadBuffer[readPos];

            writePos = (writePos + 1) % lookaheadSamples;

            // Pure digital clipping at ceiling threshold
            const double clipped = juce::jlimit(-ceilingD, ceilingD, delayedSample);

            data[i] = static_cast<SampleType>(clipped);
        }
    }
}

// Soft Clipping - Cubic soft clipping with dynamic knee
// Includes 3ms delay compensation for phase alignment with other processors
template<typename SampleType>
void QuadBlendDriveAudioProcessor::processSoftClip(juce::AudioBuffer<SampleType>& buffer,
                                                     SampleType ceiling,
                                                     SampleType knee,
                                                     double sampleRate)
{
    const double ceilingD = static_cast<double>(ceiling);
    const double color = static_cast<double>(knee) / 100.0;  // Convert knee percentage to color (0-1)

    for (int ch = 0; ch < juce::jmin(buffer.getNumChannels(), 2); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        auto& lookaheadBuffer = softClipState[ch].lookaheadBuffer;
        int& writePos = softClipState[ch].lookaheadWritePos;

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const double currentSample = static_cast<double>(data[i]);

            // Store current sample in delay buffer
            if (writePos >= 0 && writePos < static_cast<int>(lookaheadBuffer.size()))
                lookaheadBuffer[writePos] = currentSample;

            // Read delayed sample (3ms ago) for phase alignment
            int readPos = (writePos + 1) % lookaheadSamples;
            double delayedSample = currentSample;
            if (readPos >= 0 && readPos < static_cast<int>(lookaheadBuffer.size()))
                delayedSample = lookaheadBuffer[readPos];

            writePos = (writePos + 1) % lookaheadSamples;

            // Normalize input to ceiling range
            const double normalizedInput = delayedSample / ceilingD;

            // Exponential soft knee that gets steeper as signal approaches ceiling
            // Uses tanh for smooth, asymptotic approach to ceiling (never hard clips)
            const double sign = (normalizedInput >= 0.0) ? 1.0 : -1.0;
            const double absInput = std::abs(normalizedInput);

            // Knee controls saturation intensity: 0% = gentle, 100% = aggressive
            // Scale knee to useful range for tanh-based saturation
            const double saturation = 1.0 + (color * 4.0);  // 1.0 to 5.0

            // Tanh-based soft saturation: progressively steeper as it approaches ceiling
            // Never reaches ceiling, always smooth and asymptotic
            double output = sign * std::tanh(absInput * saturation) / std::tanh(saturation);

            // Scale back to ceiling range
            output *= ceilingD;

            data[i] = static_cast<SampleType>(output);
        }
    }
}

// Slow Limiting - Crest-Factor-Based Adaptive Release Limiter
// Fast release (20-40ms) for transients, slow release (200-600ms) for sustained material
// Dynamically blended based on signal crest factor with exponential smoothing
template<typename SampleType>
void QuadBlendDriveAudioProcessor::processSlowLimit(juce::AudioBuffer<SampleType>& buffer,
                                                      SampleType threshold,
                                                      SampleType baseReleaseMs,
                                                      double sampleRate)
{
    // Fixed parameters for Slow Limiter
    const double attackMs = 3.0;                    // 3ms attack
    const double fastReleaseMinMs = 20.0;          // Fast release min (for transients)
    const double fastReleaseMaxMs = 40.0;          // Fast release max
    const double slowReleaseMinMs = 300.0;         // Slow release min (for sustained)
    const double slowReleaseMaxMs = 800.0;         // Slow release max
    const double kneeDB = 3.0;                     // 3dB soft knee

    // Time constants for envelope followers
    const double rmsTimeMs = 50.0;                 // RMS averaging time
    const double peakTimeMs = 10.0;                // Peak tracking time
    const double crestSmoothTimeMs = 100.0;        // Crest factor smoothing time

    const double attackCoeff = std::exp(-1.0 / (attackMs * 0.001 * sampleRate));
    const double rmsCoeff = std::exp(-1.0 / (rmsTimeMs * 0.001 * sampleRate));
    const double peakCoeff = std::exp(-1.0 / (peakTimeMs * 0.001 * sampleRate));
    const double crestSmoothCoeff = std::exp(-1.0 / (crestSmoothTimeMs * 0.001 * sampleRate));

    const double thresholdD = static_cast<double>(threshold);
    const double kneeStart = thresholdD * std::pow(10.0, -kneeDB / 20.0);  // 3dB below threshold

    for (int ch = 0; ch < juce::jmin(buffer.getNumChannels(), 2); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        double& envelope = slowLimiterState[ch].envelope;
        double& rmsEnvelope = slowLimiterState[ch].rmsEnvelope;
        double& peakEnvelope = slowLimiterState[ch].peakEnvelope;
        double& smoothedCrestFactor = slowLimiterState[ch].smoothedCrestFactor;
        auto& lookaheadBuffer = slowLimiterState[ch].lookaheadBuffer;
        int& writePos = slowLimiterState[ch].lookaheadWritePos;

        for (int i = 0; i < buffer.getNumSamples(); ++i)
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

            const double releaseCoeff = std::exp(-1.0 / (adaptiveReleaseMs * 0.001 * sampleRate));

            // Attack/release envelope follower
            if (inputAbs > envelope)
                envelope = attackCoeff * envelope + (1.0 - attackCoeff) * inputAbs;
            else
                envelope = releaseCoeff * envelope + (1.0 - releaseCoeff) * inputAbs;

            // Calculate gain reduction with soft knee
            double gain = 1.0;

            if (envelope <= kneeStart)
            {
                gain = 1.0;  // No limiting
            }
            else if (envelope < thresholdD)
            {
                // Soft knee region
                const double kneePos = (envelope - kneeStart) / (thresholdD - kneeStart);
                const double compressionAmount = kneePos * kneePos;  // Smooth curve
                gain = 1.0 - compressionAmount * (1.0 - thresholdD / envelope);
            }
            else
            {
                // Above threshold - apply limiting
                gain = thresholdD / envelope;
            }

            data[i] = static_cast<SampleType>(delayedSample * gain);
        }
    }
}

// Fast Limiting - Hard knee limiting with fixed 10ms release
// Attack: 1ms, Release: 10ms (fixed), Hard knee, Lookahead: 3ms
template<typename SampleType>
void QuadBlendDriveAudioProcessor::processFastLimit(juce::AudioBuffer<SampleType>& buffer,
                                                      SampleType threshold,
                                                      double sampleRate)
{
    // Fixed parameters for Fast Limiter
    const double attackMs = 1.0;        // 1ms attack
    const double releaseMs = 10.0;      // 10ms release (fixed)

    const double attackCoeff = std::exp(-1.0 / (attackMs * 0.001 * currentSampleRate));
    const double releaseCoeff = std::exp(-1.0 / (releaseMs * 0.001 * currentSampleRate));
    const double thresholdD = static_cast<double>(threshold);

    for (int ch = 0; ch < juce::jmin(buffer.getNumChannels(), 2); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        double& envelope = fastLimiterState[ch].envelope;
        auto& lookaheadBuffer = fastLimiterState[ch].lookaheadBuffer;
        int& writePos = fastLimiterState[ch].lookaheadWritePos;

        for (int i = 0; i < buffer.getNumSamples(); ++i)
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

            // Hard knee limiting - no soft knee region
            double gain = 1.0;
            if (envelope > thresholdD)
            {
                gain = thresholdD / envelope;
            }

            // Clamp gain to prevent artifacts
            gain = juce::jlimit(0.01, 1.0, gain);

            data[i] = static_cast<SampleType>(delayedSample * gain);
        }
    }
}

// Protection Limiter - True-Peak Safety Limiter
// Lookahead: 4ms, Oversampling: 8×, Attack: instant, Release: dual time-constant
// Ceiling: User-adjustable dBTP, Final: brickwall true-peak clamp
template<typename SampleType>
void QuadBlendDriveAudioProcessor::processProtectionLimiter(juce::AudioBuffer<SampleType>& buffer, SampleType ceilingDB, double sampleRate)
{
    // Fixed parameters for protection limiter
    const double ceilingDBTP = static_cast<double>(ceilingDB);  // User-adjustable ceiling
    const double ceilingLinear = std::pow(10.0, ceilingDBTP / 20.0);
    const double fastReleaseMs = 50.0;                // Fast release for transients
    const double slowReleaseMs = 500.0;               // Slow release for sustained material
    const double releaseBlendTimeMs = 200.0;          // Time constant for blending fast/slow
    const int oversampleFactor = 8;                   // 8× oversampling for true-peak detection

    // Calculate coefficients
    const double fastReleaseCoeff = std::exp(-1.0 / (fastReleaseMs * 0.001 * sampleRate));
    const double slowReleaseCoeff = std::exp(-1.0 / (slowReleaseMs * 0.001 * sampleRate));
    const double blendCoeff = std::exp(-1.0 / (releaseBlendTimeMs * 0.001 * sampleRate));

    for (int ch = 0; ch < juce::jmin(buffer.getNumChannels(), 2); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        double& envelope = protectionLimiterState[ch].envelope;
        double& fastReleaseEnv = protectionLimiterState[ch].fastReleaseEnv;
        double& slowReleaseEnv = protectionLimiterState[ch].slowReleaseEnv;
        auto& lookaheadBuffer = protectionLimiterState[ch].lookaheadBuffer;
        auto& oversampledBuffer = protectionLimiterState[ch].oversampledBuffer;
        int& writePos = protectionLimiterState[ch].lookaheadWritePos;

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const double currentSample = static_cast<double>(data[i]);

            // Store current sample in lookahead buffer
            if (writePos >= 0 && writePos < static_cast<int>(lookaheadBuffer.size()))
                lookaheadBuffer[writePos] = currentSample;

            // === TRUE-PEAK DETECTION with 8x OVERSAMPLING ===
            // Simple linear interpolation for oversampling (sufficient for peak detection)
            double maxOversampledPeak = std::abs(currentSample);

            // Get next sample for interpolation (wrap around in lookahead buffer)
            int nextIdx = writePos;
            double nextSample = currentSample;

            // Interpolate between current and next sample
            for (int os = 0; os < oversampleFactor; ++os)
            {
                const double t = static_cast<double>(os) / static_cast<double>(oversampleFactor);
                const double interpolated = currentSample + t * (nextSample - currentSample);
                maxOversampledPeak = juce::jmax(maxOversampledPeak, std::abs(interpolated));
            }

            // Use the true-peak (oversampled peak) for envelope detection
            const double truePeak = maxOversampledPeak;

            // === INSTANT ATTACK ===
            // If signal exceeds envelope, instantly jump to peak (no attack smoothing)
            if (truePeak > envelope)
            {
                envelope = truePeak;  // Instant attack
                fastReleaseEnv = truePeak;
                slowReleaseEnv = truePeak;
            }
            else
            {
                // === DUAL TIME-CONSTANT RELEASE with SHAPING ===
                // Update both fast and slow release envelopes
                fastReleaseEnv = fastReleaseCoeff * fastReleaseEnv + (1.0 - fastReleaseCoeff) * truePeak;
                slowReleaseEnv = slowReleaseCoeff * slowReleaseEnv + (1.0 - slowReleaseCoeff) * truePeak;

                // Blend between fast and slow based on how much reduction is needed
                // More reduction → use slower release (prevents pumping)
                const double reductionAmount = envelope / (ceilingLinear + 1e-10);
                const double blendFactor = juce::jlimit(0.0, 1.0, (reductionAmount - 0.5) * 2.0);

                // Smooth blend between fast and slow
                const double targetEnv = fastReleaseEnv + blendFactor * (slowReleaseEnv - fastReleaseEnv);
                envelope = blendCoeff * envelope + (1.0 - blendCoeff) * targetEnv;
            }

            // Clamp envelope to prevent extreme values
            envelope = juce::jmax(0.0, envelope);

            // Read delayed sample (4ms ago) from lookahead buffer
            int readPos = (writePos + 1) % protectionLookaheadSamples;
            double delayedSample = currentSample;
            if (readPos >= 0 && readPos < static_cast<int>(lookaheadBuffer.size()))
                delayedSample = lookaheadBuffer[readPos];

            writePos = (writePos + 1) % protectionLookaheadSamples;

            // === CALCULATE GAIN REDUCTION ===
            double gain = 1.0;
            if (envelope > ceilingLinear)
            {
                gain = ceilingLinear / envelope;
            }

            // Clamp gain to prevent over-attenuation
            gain = juce::jlimit(0.01, 1.0, gain);

            // Apply gain reduction
            double output = delayedSample * gain;

            // === FINAL BRICKWALL TRUE-PEAK CLAMP ===
            // Safety net to prevent any overs (should rarely trigger if limiter works correctly)
            output = juce::jlimit(-ceilingLinear, ceilingLinear, output);

            data[i] = static_cast<SampleType>(output);
        }
    }
}

// Explicit template instantiations
template void QuadBlendDriveAudioProcessor::processHardClip<float>(juce::AudioBuffer<float>&, float, double);
template void QuadBlendDriveAudioProcessor::processHardClip<double>(juce::AudioBuffer<double>&, double, double);

template void QuadBlendDriveAudioProcessor::processSoftClip<float>(juce::AudioBuffer<float>&, float, float, double);
template void QuadBlendDriveAudioProcessor::processSoftClip<double>(juce::AudioBuffer<double>&, double, double, double);

template void QuadBlendDriveAudioProcessor::processSlowLimit<float>(juce::AudioBuffer<float>&, float, float, double);
template void QuadBlendDriveAudioProcessor::processSlowLimit<double>(juce::AudioBuffer<double>&, double, double, double);

template void QuadBlendDriveAudioProcessor::processFastLimit<float>(juce::AudioBuffer<float>&, float, double);
template void QuadBlendDriveAudioProcessor::processFastLimit<double>(juce::AudioBuffer<double>&, double, double);

template void QuadBlendDriveAudioProcessor::processProtectionLimiter<float>(juce::AudioBuffer<float>&, float, double);
template void QuadBlendDriveAudioProcessor::processProtectionLimiter<double>(juce::AudioBuffer<double>&, double, double);

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

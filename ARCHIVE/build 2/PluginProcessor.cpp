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

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "XY_X_PARAM", "X Position",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "XY_Y_PARAM", "Y Position",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

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
        "CLIP_THRESH", "Clip Threshold",
        juce::NormalisableRange<float>(-12.0f, -0.1f, 0.1f), -0.3f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " dBFS"; }));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "LIMIT_REL", "Limiter Release",
        juce::NormalisableRange<float>(10.0f, 500.0f, 1.0f, 0.5f), 50.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 0) + " ms"; }));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "OVERSAMPLE_VAL", "Oversampling",
        juce::StringArray{"1x", "2x", "4x", "8x"}, 1));

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

    layout.add(std::make_unique<juce::AudioParameterBool>(
        "HC_COMP", "Hard Clip Compensation", false));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        "SC_COMP", "Soft Clip Compensation", false));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        "SL_COMP", "Slow Limit Compensation", false));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        "FL_COMP", "Fast Limit Compensation", false));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        "DELTA_MODE", "Delta Mode", false));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        "BYPASS", "Bypass", false));

    return layout;
}

//==============================================================================
void QuadBlendDriveAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 2;

    hardClipChain.prepare(sampleRate, samplesPerBlock);
    softClipChain.prepare(sampleRate, samplesPerBlock);
    slowLimitChain.prepare(sampleRate, samplesPerBlock);
    fastLimitChain.prepare(sampleRate, samplesPerBlock);

    dryBuffer.setSize(2, samplesPerBlock);
    tempBuffer1.setSize(2, samplesPerBlock);
    tempBuffer2.setSize(2, samplesPerBlock);
    tempBuffer3.setSize(2, samplesPerBlock);
    tempBuffer4.setSize(2, samplesPerBlock);

    if (currentOversampleFactor > 1)
    {
        int order = 0;
        if (currentOversampleFactor == 2) order = 1;
        else if (currentOversampleFactor == 4) order = 2;
        else if (currentOversampleFactor == 8) order = 3;

        oversampling = std::make_unique<juce::dsp::Oversampling<float>>(
            2, order, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true);
        oversampling->initProcessing(static_cast<size_t>(samplesPerBlock));
    }

    inputPeakL.reset(sampleRate, 0.1);
    inputPeakR.reset(sampleRate, 0.1);
    outputPeakL.reset(sampleRate, 0.1);
    outputPeakR.reset(sampleRate, 0.1);
}

void QuadBlendDriveAudioProcessor::releaseResources()
{
    oversampling.reset();
}

//==============================================================================
void QuadBlendDriveAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                                 juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int totalNumInputChannels = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();

    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, numSamples);

    // Read parameters
    const float xyX = apvts.getRawParameterValue("XY_X_PARAM")->load();
    const float xyY = apvts.getRawParameterValue("XY_Y_PARAM")->load();

    const float inputGainDB = apvts.getRawParameterValue("INPUT_GAIN")->load();
    const float outputGainDB = apvts.getRawParameterValue("OUTPUT_GAIN")->load();
    const float mixWetPercent = apvts.getRawParameterValue("MIX_WET")->load();
    const float clipThreshDB = apvts.getRawParameterValue("CLIP_THRESH")->load();
    const float limitRelMs = apvts.getRawParameterValue("LIMIT_REL")->load();
    const int oversampleChoice = static_cast<int>(apvts.getRawParameterValue("OVERSAMPLE_VAL")->load());

    const float hcTrimDB = apvts.getRawParameterValue("HC_TRIM")->load();
    const float scTrimDB = apvts.getRawParameterValue("SC_TRIM")->load();
    const float slTrimDB = apvts.getRawParameterValue("SL_TRIM")->load();
    const float flTrimDB = apvts.getRawParameterValue("FL_TRIM")->load();

    const bool hcComp = apvts.getRawParameterValue("HC_COMP")->load() > 0.5f;
    const bool scComp = apvts.getRawParameterValue("SC_COMP")->load() > 0.5f;
    const bool slComp = apvts.getRawParameterValue("SL_COMP")->load() > 0.5f;
    const bool flComp = apvts.getRawParameterValue("FL_COMP")->load() > 0.5f;

    const bool deltaMode = apvts.getRawParameterValue("DELTA_MODE")->load() > 0.5f;

    const float inputGain = juce::Decibels::decibelsToGain(inputGainDB);
    const float outputGain = juce::Decibels::decibelsToGain(outputGainDB);
    const float mixWet = mixWetPercent / 100.0f;
    const float clipThreshold = juce::Decibels::decibelsToGain(clipThreshDB);

    const int oversampleFactors[] = {1, 2, 4, 8};
    const int targetOversample = oversampleFactors[juce::jlimit(0, 3, oversampleChoice)];

    // Update oversampling
    if (targetOversample != currentOversampleFactor)
    {
        currentOversampleFactor = targetOversample;
        oversampling.reset();

        if (currentOversampleFactor > 1)
        {
            int order = 0;
            if (currentOversampleFactor == 2) order = 1;
            else if (currentOversampleFactor == 4) order = 2;
            else if (currentOversampleFactor == 8) order = 3;

            oversampling = std::make_unique<juce::dsp::Oversampling<float>>(
                2, order, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true);
            oversampling->initProcessing(static_cast<size_t>(buffer.getNumSamples()));
        }
    }

    // Store dry signal
    dryBuffer.makeCopyOf(buffer);

    // Apply input gain
    buffer.applyGain(inputGain);

    // Upsample
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::AudioBlock<float> oversampledBlock;

    if (oversampling)
    {
        oversampledBlock = oversampling->processSamplesUp(block);
    }
    else
    {
        oversampledBlock = block;
    }

    // Calculate blend weights
    const float wHC = (1.0f - xyX) * (1.0f - xyY);
    const float wSC = xyX * (1.0f - xyY);
    const float wSL = (1.0f - xyX) * xyY;
    const float wFL = xyX * xyY;

    // Prepare temp buffers
    const int procSamples = static_cast<int>(oversampledBlock.getNumSamples());
    tempBuffer1.setSize(2, procSamples, false, false, true);
    tempBuffer2.setSize(2, procSamples, false, false, true);
    tempBuffer3.setSize(2, procSamples, false, false, true);
    tempBuffer4.setSize(2, procSamples, false, false, true);

    const double currentSR = oversampling ? getSampleRate() * currentOversampleFactor : getSampleRate();

    // Copy oversampled block to temp buffers
    for (int ch = 0; ch < 2; ++ch)
    {
        tempBuffer1.copyFrom(ch, 0, oversampledBlock.getChannelPointer(static_cast<size_t>(ch)), procSamples);
        tempBuffer2.copyFrom(ch, 0, oversampledBlock.getChannelPointer(static_cast<size_t>(ch)), procSamples);
        tempBuffer3.copyFrom(ch, 0, oversampledBlock.getChannelPointer(static_cast<size_t>(ch)), procSamples);
        tempBuffer4.copyFrom(ch, 0, oversampledBlock.getChannelPointer(static_cast<size_t>(ch)), procSamples);
    }

    // Process four paths
    hardClipChain.trimGainLinear = juce::Decibels::decibelsToGain(hcTrimDB);
    hardClipChain.compensationEnabled = hcComp;
    tempBuffer1.applyGain(hardClipChain.trimGainLinear);
    hardClipChain.processHardClip(tempBuffer1, clipThreshold);
    if (hcComp) tempBuffer1.applyGain(1.0f / hardClipChain.trimGainLinear);

    softClipChain.trimGainLinear = juce::Decibels::decibelsToGain(scTrimDB);
    softClipChain.compensationEnabled = scComp;
    tempBuffer2.applyGain(softClipChain.trimGainLinear);
    softClipChain.processSoftClip(tempBuffer2, clipThreshold);
    if (scComp) tempBuffer2.applyGain(1.0f / softClipChain.trimGainLinear);

    slowLimitChain.trimGainLinear = juce::Decibels::decibelsToGain(slTrimDB);
    slowLimitChain.compensationEnabled = slComp;
    tempBuffer3.applyGain(slowLimitChain.trimGainLinear);
    slowLimitChain.processSlowLimit(tempBuffer3, clipThreshold, 100.0f, currentSR);
    if (slComp) tempBuffer3.applyGain(1.0f / slowLimitChain.trimGainLinear);

    fastLimitChain.trimGainLinear = juce::Decibels::decibelsToGain(flTrimDB);
    fastLimitChain.compensationEnabled = flComp;
    tempBuffer4.applyGain(fastLimitChain.trimGainLinear);
    fastLimitChain.processFastLimit(tempBuffer4, clipThreshold, 10.0f, currentSR);
    if (flComp) tempBuffer4.applyGain(1.0f / fastLimitChain.trimGainLinear);

    // Blend
    for (size_t ch = 0; ch < oversampledBlock.getNumChannels(); ++ch)
    {
        auto* dest = oversampledBlock.getChannelPointer(ch);
        const auto* src1 = tempBuffer1.getReadPointer(static_cast<int>(ch));
        const auto* src2 = tempBuffer2.getReadPointer(static_cast<int>(ch));
        const auto* src3 = tempBuffer3.getReadPointer(static_cast<int>(ch));
        const auto* src4 = tempBuffer4.getReadPointer(static_cast<int>(ch));

        for (int i = 0; i < procSamples; ++i)
        {
            dest[i] = wHC * src1[i] + wSC * src2[i] + wSL * src3[i] + wFL * src4[i];
        }
    }

    // Downsample
    if (oversampling)
    {
        oversampling->processSamplesDown(block);
    }

    // Calculate gain reduction (compare processed signal to dry signal after input gain)
    float maxInputLevel = 0.0f;
    float maxOutputLevel = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        const auto* wet = buffer.getReadPointer(ch);
        const auto* dry = dryBuffer.getReadPointer(ch);

        for (int i = 0; i < numSamples; ++i)
        {
            maxInputLevel = juce::jmax(maxInputLevel, std::abs(dry[i] * inputGain));
            maxOutputLevel = juce::jmax(maxOutputLevel, std::abs(wet[i]));
        }
    }

    float grDB = 0.0f;
    if (maxInputLevel > 0.00001f && maxOutputLevel > 0.00001f)
    {
        grDB = juce::Decibels::gainToDecibels(maxOutputLevel / maxInputLevel);
        if (grDB > 0.0f) grDB = 0.0f;  // Only show reduction, not gain
    }
    currentGainReductionDB.store(grDB);

    // Apply output gain
    buffer.applyGain(outputGain);

    // Dry/wet mix
    if (mixWet < 1.0f)
    {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* wet = buffer.getWritePointer(ch);
            const auto* dry = dryBuffer.getReadPointer(ch);

            for (int i = 0; i < numSamples; ++i)
            {
                wet[i] = dry[i] * (1.0f - mixWet) + wet[i] * mixWet;
            }
        }
    }

    // Delta mode
    if (deltaMode)
    {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* output = buffer.getWritePointer(ch);
            const auto* input = dryBuffer.getReadPointer(ch);

            for (int i = 0; i < numSamples; ++i)
            {
                output[i] -= input[i];
            }
        }
    }
}

//==============================================================================
void QuadBlendDriveAudioProcessor::ProcessingChain::prepare(double sampleRate, int maxBlockSize)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(maxBlockSize);
    spec.numChannels = 2;

    inputTrim.prepare(spec);
    compensationGain.prepare(spec);

    for (auto& state : limiterState)
    {
        state.envelope = 0.0f;
        state.releaseCoeff = 0.0f;
    }
}

void QuadBlendDriveAudioProcessor::ProcessingChain::reset()
{
    inputTrim.reset();
    compensationGain.reset();

    for (auto& state : limiterState)
    {
        state.envelope = 0.0f;
    }
}

void QuadBlendDriveAudioProcessor::ProcessingChain::processHardClip(juce::AudioBuffer<float>& buffer, float threshold)
{
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            data[i] = juce::jlimit(-threshold, threshold, data[i]);
        }
    }
}

void QuadBlendDriveAudioProcessor::ProcessingChain::processSoftClip(juce::AudioBuffer<float>& buffer, float threshold)
{
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            data[i] = std::tanh(data[i] / threshold) * threshold;
        }
    }
}

void QuadBlendDriveAudioProcessor::ProcessingChain::processSlowLimit(juce::AudioBuffer<float>& buffer,
                                                                      float threshold,
                                                                      float releaseMs,
                                                                      double sampleRate)
{
    const float releaseCoeff = std::exp(-1.0f / (releaseMs * 0.001f * static_cast<float>(sampleRate)));

    for (int ch = 0; ch < juce::jmin(buffer.getNumChannels(), 2); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        float& envelope = limiterState[ch].envelope;

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const float inputAbs = std::abs(data[i]);

            if (inputAbs > threshold)
                envelope = inputAbs;
            else
                envelope = inputAbs + releaseCoeff * (envelope - inputAbs);

            if (envelope > threshold)
            {
                const float gain = threshold / envelope;
                data[i] *= gain;
            }
        }
    }
}

void QuadBlendDriveAudioProcessor::ProcessingChain::processFastLimit(juce::AudioBuffer<float>& buffer,
                                                                      float threshold,
                                                                      float releaseMs,
                                                                      double sampleRate)
{
    const float releaseCoeff = std::exp(-1.0f / (releaseMs * 0.001f * static_cast<float>(sampleRate)));

    for (int ch = 0; ch < juce::jmin(buffer.getNumChannels(), 2); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        float& envelope = limiterState[ch].envelope;

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const float inputAbs = std::abs(data[i]);

            if (inputAbs > threshold)
                envelope = inputAbs;
            else
                envelope = inputAbs + releaseCoeff * (envelope - inputAbs);

            if (envelope > threshold)
            {
                const float gain = threshold / envelope;
                data[i] *= gain;
            }
        }
    }
}

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
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new QuadBlendDriveAudioProcessor();
}

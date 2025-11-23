#pragma once

#include <juce_dsp/juce_dsp.h>

/**
 * OversamplingManager - Global oversampling handler for Architecture A
 *
 * Manages ALL oversampling for the entire plugin signal chain.
 * Owns all oversampling objects and provides a single interface for
 * upsampling/downsampling operations.
 *
 * Processing modes:
 * - Mode 0 (Zero Latency): No oversampling (osFactor = 0, multiplier = 1)
 * - Mode 1 (Balanced): 8× oversampling (osFactor = 3, multiplier = 8)
 * - Mode 2 (Linear Phase): 16× oversampling (osFactor = 4, multiplier = 16)
 */
class OversamplingManager
{
public:
    OversamplingManager() = default;

    /**
     * Prepare oversampling for processing
     *
     * @param baseSampleRate Base sample rate (e.g., 44100 Hz)
     * @param maxBlockSize Maximum block size at base rate
     * @param processingMode 0 = Zero Latency, 1 = Balanced, 2 = Linear Phase
     */
    void prepare(double baseSampleRate, int maxBlockSize, int processingMode)
    {
        sampleRate = baseSampleRate;
        mode = processingMode;

        // Determine OS factor based on mode
        if (mode == 0)  // Zero Latency
        {
            osFactor = 0;  // No oversampling
            osMultiplier = 1;
        }
        else if (mode == 1)  // Balanced
        {
            osFactor = 3;  // 2^3 = 8×
            osMultiplier = 8;
        }
        else  // mode == 2, Linear Phase
        {
            osFactor = 4;  // 2^4 = 16×
            osMultiplier = 16;
        }

        // Create oversampler if needed (not needed for Zero Latency)
        if (osFactor > 0)
        {
            if (mode == 1)  // Balanced - Halfband Equiripple FIR
            {
                oversamplerFloat = std::make_unique<juce::dsp::Oversampling<float>>(
                    2, osFactor,
                    juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple,
                    false, false);  // Not steep, not normalized (for efficiency)

                oversamplerDouble = std::make_unique<juce::dsp::Oversampling<double>>(
                    2, osFactor,
                    juce::dsp::Oversampling<double>::filterHalfBandFIREquiripple,
                    false, false);
            }
            else  // mode == 2, Linear Phase - High-order FIR
            {
                oversamplerFloat = std::make_unique<juce::dsp::Oversampling<float>>(
                    2, osFactor,
                    juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple,
                    true, true);  // Steep + normalization for linear phase

                oversamplerDouble = std::make_unique<juce::dsp::Oversampling<double>>(
                    2, osFactor,
                    juce::dsp::Oversampling<double>::filterHalfBandFIREquiripple,
                    true, true);
            }

            // Initialize oversampling
            oversamplerFloat->initProcessing(static_cast<size_t>(maxBlockSize));
            oversamplerDouble->initProcessing(static_cast<size_t>(maxBlockSize));
        }
        else
        {
            // Zero Latency mode - no oversampler needed
            oversamplerFloat.reset();
            oversamplerDouble.reset();
        }
    }

    /**
     * Upsample buffer to OS domain
     *
     * @param buffer Input buffer at base sample rate
     * @return AudioBlock in oversampled domain (or original if no OS)
     */
    template<typename SampleType>
    juce::dsp::AudioBlock<SampleType> upsampleBlock(juce::AudioBuffer<SampleType>& buffer)
    {
        if (osFactor == 0)
        {
            // No oversampling - return original buffer as block
            return juce::dsp::AudioBlock<SampleType>(buffer);
        }

        auto* oversampler = getOversampler<SampleType>();
        return oversampler->processSamplesUp(juce::dsp::AudioBlock<SampleType>(buffer));
    }

    /**
     * Downsample block back to base rate
     *
     * @param buffer Output buffer at base sample rate (will be filled)
     * @param osBlock Oversampled block to downsample
     */
    template<typename SampleType>
    void downsampleBlock(juce::AudioBuffer<SampleType>& buffer, juce::dsp::AudioBlock<SampleType>& osBlock)
    {
        if (osFactor == 0)
            return;  // No downsampling needed

        auto* oversampler = getOversampler<SampleType>();
        juce::dsp::AudioBlock<SampleType> outputBlock(buffer);
        oversampler->processSamplesDown(outputBlock);
    }

    /**
     * Get current oversampled sample rate
     * @return Sample rate in OS domain (base rate × OS multiplier)
     */
    double getOsSampleRate() const { return sampleRate * osMultiplier; }

    /**
     * Get oversampling multiplier (1, 8, or 16)
     */
    int getOsMultiplier() const { return osMultiplier; }

    /**
     * Get latency introduced by oversampling filters
     * @return Latency in samples at BASE sample rate
     */
    int getLatencySamples() const
    {
        if (osFactor == 0)
            return 0;
        return oversamplerFloat ? static_cast<int>(oversamplerFloat->getLatencyInSamples()) : 0;
    }

    /**
     * Check if oversampling is active
     */
    bool isOversampling() const { return osFactor > 0; }

    /**
     * Get current processing mode (0 = Zero Latency, 1 = Balanced, 2 = Linear Phase)
     */
    int getProcessingMode() const { return mode; }

    /**
     * Reset oversampling state (call when audio stops)
     */
    void reset()
    {
        if (oversamplerFloat)
            oversamplerFloat->reset();
        if (oversamplerDouble)
            oversamplerDouble->reset();
    }

private:
    template<typename SampleType>
    juce::dsp::Oversampling<SampleType>* getOversampler()
    {
        if constexpr (std::is_same_v<SampleType, float>)
            return oversamplerFloat.get();
        else
            return oversamplerDouble.get();
    }

    std::unique_ptr<juce::dsp::Oversampling<float>> oversamplerFloat;
    std::unique_ptr<juce::dsp::Oversampling<double>> oversamplerDouble;

    double sampleRate = 44100.0;
    int osFactor = 0;
    int osMultiplier = 1;
    int mode = 0;
};

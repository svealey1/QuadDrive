#pragma once

#include <juce_dsp/juce_dsp.h>
#include <atomic>

/**
 * OversamplingManager - Global oversampling handler for Architecture A
 *
 * ====================================================================================
 * THREAD-SAFETY MODEL
 * ====================================================================================
 *
 * This class is designed for LOCK-FREE, ALLOCATION-FREE operation on the audio thread.
 *
 * INITIALIZATION PHASE (prepareToPlay - main thread):
 * - prepare() allocates ALL three processing modes upfront
 * - Mode 0 (Zero Latency): No oversampling (1×)
 * - Mode 1 (Balanced): 8× oversampling (Halfband Equiripple FIR)
 * - Mode 2 (Linear Phase): 16× oversampling (Steep + normalized FIR)
 * - All memory allocation happens here, ONCE
 *
 * AUDIO THREAD OPERATION (processBlock):
 * - setMode() performs lock-free mode switching via std::atomic
 * - NO memory allocation
 * - NO locks/mutexes
 * - Single atomic write updates active mode index
 * - upsampleBlock/downsampleBlock use the currently active mode
 *
 * GUARANTEE: Once prepare() completes, all subsequent operations are wait-free
 * and real-time safe.
 *
 * ====================================================================================
 */
class OversamplingManager
{
public:
    OversamplingManager() = default;

    /**
     * Prepare oversampling for processing (CALL ONCE in prepareToPlay)
     *
     * Pre-allocates ALL three processing modes to enable lock-free switching.
     * This method allocates memory and MUST NOT be called on the audio thread.
     *
     * @param baseSampleRate Base sample rate (e.g., 44100 Hz)
     * @param maxBlockSize Maximum block size at base rate
     * @param initialMode Initial mode (0=Zero Latency, 1=Balanced, 2=Linear Phase)
     */
    void prepare(double baseSampleRate, int maxBlockSize, int initialMode = 1)
    {
        sampleRate = baseSampleRate;
        activeMode.store(initialMode, std::memory_order_release);

        // Pre-allocate ALL three modes
        // Mode 0: Zero Latency (no oversampling) - nullptr
        oversamplerFloat[0].reset();
        oversamplerDouble[0].reset();

        // Mode 1: Balanced (8× oversampling)
        oversamplerFloat[1] = std::make_unique<juce::dsp::Oversampling<float>>(
            2, 3,  // 2 channels, 2^3 = 8× oversampling
            juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple,
            false, false);  // Not steep, not normalized

        oversamplerDouble[1] = std::make_unique<juce::dsp::Oversampling<double>>(
            2, 3,
            juce::dsp::Oversampling<double>::filterHalfBandFIREquiripple,
            false, false);

        // Mode 2: Linear Phase (16× oversampling)
        oversamplerFloat[2] = std::make_unique<juce::dsp::Oversampling<float>>(
            2, 4,  // 2 channels, 2^4 = 16× oversampling
            juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple,
            true, true);  // Steep + normalization

        oversamplerDouble[2] = std::make_unique<juce::dsp::Oversampling<double>>(
            2, 4,
            juce::dsp::Oversampling<double>::filterHalfBandFIREquiripple,
            true, true);

        // Initialize all non-null oversamplers
        for (int mode = 1; mode <= 2; ++mode)
        {
            oversamplerFloat[mode]->initProcessing(static_cast<size_t>(maxBlockSize));
            oversamplerDouble[mode]->initProcessing(static_cast<size_t>(maxBlockSize));
        }

        // Pre-calculate latency for each mode (at base sample rate)
        latencySamples[0] = 0;  // Zero Latency
        latencySamples[1] = static_cast<int>(oversamplerFloat[1]->getLatencyInSamples());  // Balanced
        latencySamples[2] = static_cast<int>(oversamplerFloat[2]->getLatencyInSamples());  // Linear Phase
    }

    /**
     * Switch processing mode (LOCK-FREE - safe on audio thread)
     *
     * Changes which pre-allocated oversampler is active. NO memory allocation.
     * Thread-safe via atomic operation.
     *
     * @param processingMode 0 = Zero Latency, 1 = Balanced, 2 = Linear Phase
     */
    void setMode(int processingMode)
    {
        if (processingMode < 0 || processingMode > 2)
            return;

        activeMode.store(processingMode, std::memory_order_release);
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
        const int mode = activeMode.load(std::memory_order_acquire);

        if (mode == 0)
        {
            // No oversampling - return original buffer as block
            return juce::dsp::AudioBlock<SampleType>(buffer);
        }

        auto* oversampler = getOversampler<SampleType>(mode);
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
        const int mode = activeMode.load(std::memory_order_acquire);

        if (mode == 0)
            return;  // No downsampling needed

        auto* oversampler = getOversampler<SampleType>(mode);
        juce::dsp::AudioBlock<SampleType> outputBlock(buffer);
        oversampler->processSamplesDown(outputBlock);
    }

    /**
     * Get current oversampled sample rate
     * @return Sample rate in OS domain (base rate × OS multiplier)
     */
    double getOsSampleRate() const
    {
        return sampleRate * getOsMultiplier();
    }

    /**
     * Get oversampling multiplier (1, 8, or 16)
     */
    int getOsMultiplier() const
    {
        const int mode = activeMode.load(std::memory_order_acquire);
        static constexpr int multipliers[3] = { 1, 8, 16 };
        return multipliers[mode];
    }

    /**
     * Get latency introduced by oversampling filters
     * @return Latency in samples at BASE sample rate
     */
    int getLatencySamples() const
    {
        const int mode = activeMode.load(std::memory_order_acquire);
        return latencySamples[mode];
    }

    /**
     * Check if oversampling is active
     */
    bool isOversampling() const
    {
        return activeMode.load(std::memory_order_acquire) > 0;
    }

    /**
     * Get current processing mode (0 = Zero Latency, 1 = Balanced, 2 = Linear Phase)
     */
    int getProcessingMode() const
    {
        return activeMode.load(std::memory_order_acquire);
    }

    /**
     * Reset oversampling state (call when audio stops)
     */
    void reset()
    {
        for (int mode = 1; mode <= 2; ++mode)
        {
            if (oversamplerFloat[mode])
                oversamplerFloat[mode]->reset();
            if (oversamplerDouble[mode])
                oversamplerDouble[mode]->reset();
        }
    }

private:
    template<typename SampleType>
    juce::dsp::Oversampling<SampleType>* getOversampler(int mode)
    {
        if constexpr (std::is_same_v<SampleType, float>)
            return oversamplerFloat[mode].get();
        else
            return oversamplerDouble[mode].get();
    }

    // Pre-allocated oversamplers for ALL modes
    // [0] = Zero Latency (nullptr), [1] = Balanced (8×), [2] = Linear Phase (16×)
    std::unique_ptr<juce::dsp::Oversampling<float>> oversamplerFloat[3];
    std::unique_ptr<juce::dsp::Oversampling<double>> oversamplerDouble[3];

    // Pre-calculated latency for each mode (at base sample rate)
    int latencySamples[3] = { 0, 0, 0 };

    double sampleRate = 44100.0;
    std::atomic<int> activeMode{1};  // Thread-safe mode switching (default: Balanced)
};

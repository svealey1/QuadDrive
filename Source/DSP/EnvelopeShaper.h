#pragma once

#include "EnvelopeFollower.h"
#include <juce_audio_processors/juce_audio_processors.h>

/**
 * @brief Envelope-based transient shaper for pre-drive gain adjustment
 *
 * Uses dual envelope followers (fast and slow) to detect transientness,
 * then applies dynamic gain based on user-specified attack and sustain emphasis.
 * This allows independent control over transient vs sustain drive intensity.
 */
class EnvelopeShaper
{
public:
    EnvelopeShaper()
    {
        // Configure fast envelope for transient detection
        fastEnvelope.setAttackTime(0.5f);   // 0.5ms - catches transients
        fastEnvelope.setReleaseTime(20.0f); // 20ms - quick release

        // Configure slow envelope for sustain tracking
        slowEnvelope.setAttackTime(10.0f);   // 10ms - averages transients
        slowEnvelope.setReleaseTime(100.0f); // 100ms - tracks sustain
    }

    /**
     * @brief Prepare the shaper for playback
     * @param sampleRate Sample rate in Hz
     */
    void prepare(double sampleRate)
    {
        currentSampleRate = sampleRate;
        fastEnvelope.setSampleRate(sampleRate);
        slowEnvelope.setSampleRate(sampleRate);

        // Setup parameter smoothing (10-20ms to prevent zipper noise)
        const float smoothingTimeMs = 15.0f;
        const int smoothingSamples = static_cast<int>(sampleRate * smoothingTimeMs * 0.001f);

        attackGainSmoothed.reset(smoothingSamples);
        sustainGainSmoothed.reset(smoothingSamples);

        // Initialize with unity gain (0 dB)
        attackGainSmoothed.setCurrentAndTargetValue(1.0f);
        sustainGainSmoothed.setCurrentAndTargetValue(1.0f);
    }

    /**
     * @brief Reset all envelope state
     */
    void reset()
    {
        fastEnvelope.reset();
        slowEnvelope.reset();
        attackGainSmoothed.setCurrentAndTargetValue(1.0f);
        sustainGainSmoothed.setCurrentAndTargetValue(1.0f);
    }

    /**
     * @brief Set attack emphasis in dB (-12 to +12)
     * @param dB Decibels of gain to apply to transients
     */
    void setAttackEmphasis(float dB)
    {
        attackEmphasisDB = juce::jlimit(-12.0f, 12.0f, dB);
        const float linearGain = juce::Decibels::decibelsToGain(attackEmphasisDB);
        attackGainSmoothed.setTargetValue(linearGain);
    }

    /**
     * @brief Set sustain emphasis in dB (-12 to +12)
     * @param dB Decibels of gain to apply to sustain
     */
    void setSustainEmphasis(float dB)
    {
        sustainEmphasisDB = juce::jlimit(-12.0f, 12.0f, dB);
        const float linearGain = juce::Decibels::decibelsToGain(sustainEmphasisDB);
        sustainGainSmoothed.setTargetValue(linearGain);
    }

    /**
     * @brief Process envelope detection and return gain multiplier
     * @param inputSample Input audio sample
     * @return Gain multiplier to apply before drive processing (0.25 to 4.0)
     */
    float processEnvelope(float inputSample)
    {
        // Track fast and slow envelopes
        const float fastEnv = fastEnvelope.process(inputSample);
        const float slowEnv = slowEnvelope.process(inputSample);

        // Calculate transientness: 0.0 = pure sustain, 1.0 = pure transient
        // Formula: (fast - slow) / (fast + slow + epsilon)
        // This normalizes the difference to [0, 1] range
        constexpr float epsilon = 1e-6f;
        float transientness = (fastEnv - slowEnv) / (fastEnv + slowEnv + epsilon);

        // Clamp to valid range [0, 1]
        transientness = juce::jlimit(0.0f, 1.0f, transientness);

        // Store for UI metering
        currentTransientness = transientness;

        // Get smoothed gain values
        const float attackGain = attackGainSmoothed.getNextValue();
        const float sustainGain = sustainGainSmoothed.getNextValue();

        // Interpolate between sustain and attack gain based on transientness
        // transientness = 0.0 → sustainGain
        // transientness = 1.0 → attackGain
        const float outputGain = juce::jmap(transientness, sustainGain, attackGain);

        return outputGain;
    }

    /**
     * @brief Get current transientness amount for UI metering
     * @return Transientness value from 0.0 (sustain) to 1.0 (transient)
     */
    float getTransientnessAmount() const
    {
        return currentTransientness;
    }

    /**
     * @brief Get current attack emphasis in dB
     */
    float getAttackEmphasisDB() const
    {
        return attackEmphasisDB;
    }

    /**
     * @brief Get current sustain emphasis in dB
     */
    float getSustainEmphasisDB() const
    {
        return sustainEmphasisDB;
    }

private:
    EnvelopeFollower fastEnvelope;  // Fast envelope (transient detection)
    EnvelopeFollower slowEnvelope;  // Slow envelope (sustain tracking)

    double currentSampleRate{44100.0};

    // User parameters
    float attackEmphasisDB{0.0f};  // -12 to +12 dB
    float sustainEmphasisDB{0.0f}; // -12 to +12 dB

    // Parameter smoothing to prevent zipper noise
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> attackGainSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> sustainGainSmoothed;

    // Current transientness for metering
    std::atomic<float> currentTransientness{0.0f};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnvelopeShaper)
};

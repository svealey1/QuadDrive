#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <cmath>

/**
 * @brief Simple 1-pole envelope follower for transient detection
 *
 * Uses different time constants for attack (rising) and release (falling)
 * to accurately track signal dynamics. Thread-safe for audio processing.
 */
class EnvelopeFollower
{
public:
    EnvelopeFollower() = default;

    /**
     * @brief Set the sample rate for coefficient calculation
     * @param sampleRate Sample rate in Hz
     */
    void setSampleRate(double sampleRate)
    {
        currentSampleRate = sampleRate;
        updateCoefficients();
    }

    /**
     * @brief Set the attack time (rising envelope)
     * @param timeMs Time in milliseconds
     */
    void setAttackTime(float timeMs)
    {
        attackTimeMs = timeMs;
        updateCoefficients();
    }

    /**
     * @brief Set the release time (falling envelope)
     * @param timeMs Time in milliseconds
     */
    void setReleaseTime(float timeMs)
    {
        releaseTimeMs = timeMs;
        updateCoefficients();
    }

    /**
     * @brief Reset the envelope state
     */
    void reset()
    {
        envelope = 0.0f;
    }

    /**
     * @brief Process a single sample and return current envelope level
     * @param inputSample The input audio sample
     * @return Current envelope level (always positive)
     */
    float process(float inputSample)
    {
        const float rectified = std::abs(inputSample);

        // Use attack coefficient if envelope rising, release if falling
        const float coeff = (rectified > envelope) ? attackCoeff : releaseCoeff;

        // 1-pole IIR filter: y[n] = coeff * y[n-1] + (1 - coeff) * x[n]
        envelope = coeff * envelope + (1.0f - coeff) * rectified;

        return envelope;
    }

    /**
     * @brief Get current envelope level without processing
     * @return Current envelope level
     */
    float getCurrentEnvelope() const
    {
        return envelope;
    }

private:
    void updateCoefficients()
    {
        if (currentSampleRate <= 0.0)
            return;

        // Convert ms to seconds
        const float attackTimeSec = attackTimeMs * 0.001f;
        const float releaseTimeSec = releaseTimeMs * 0.001f;

        // Calculate 1-pole filter coefficients
        // Formula: coeff = exp(-1.0 / (sampleRate * timeInSeconds))
        // This gives approximately 63% rise/fall after the specified time
        attackCoeff = std::exp(-1.0f / static_cast<float>(currentSampleRate * attackTimeSec));
        releaseCoeff = std::exp(-1.0f / static_cast<float>(currentSampleRate * releaseTimeSec));
    }

    double currentSampleRate{44100.0};
    float attackTimeMs{0.5f};   // Default: 0.5ms attack
    float releaseTimeMs{20.0f}; // Default: 20ms release

    float attackCoeff{0.0f};
    float releaseCoeff{0.0f};
    float envelope{0.0f};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnvelopeFollower)
};

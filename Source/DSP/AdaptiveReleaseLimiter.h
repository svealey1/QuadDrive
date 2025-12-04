#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <algorithm>
#include <cmath>

/**
 * @class AdaptiveReleaseLimiter
 * @brief Intelligent release time modulation based on signal characteristics
 *
 * Dynamically adjusts release time to prevent bass pumping and preserve
 * transient naturalness. Detects signal energy density and spectral character.
 *
 * **Result:** Limiting feels "musical" rather than clinical.
 */
class AdaptiveReleaseLimiter {
public:
    AdaptiveReleaseLimiter();

    /**
     * Process single sample through adaptive-release limiter
     */
    float processSample(float input, float threshold) noexcept;

    /**
     * Set base release time (milliseconds)
     */
    void setBaseReleaseTimeMs(float ms, float sampleRate) noexcept;

    /**
     * Set minimum/maximum release time bounds
     */
    void setReleaseTimeBounds(float minMs, float maxMs, float sampleRate) noexcept;

    /**
     * Get current adaptive release time for visualization
     */
    float getCurrentReleaseTimeMs() const noexcept;

    /**
     * Reset internal state
     */
    void reset() noexcept;

private:
    // State tracking
    float gainState = 1.0f;  // Current gain (for smooth envelope)
    float energyDensity = 0.0f;  // 0 (sparse) to 1 (dense)
    float spectralCharacter = 0.0f;  // 0 (low freq) to 1 (high freq)

    // Release time parameters
    float baseReleaseSamples = 100.0f;
    float minReleaseSamples = 20.0f;
    float maxReleaseSamples = 2000.0f;
    float currentSampleRate = 48000.0f;

    // Previous sample tracking
    float lastSample = 0.0f;

    // Helpers
    void analyzeSignalCharacter(float sample) noexcept;
    float calculateAdaptiveReleaseTime() noexcept;
    float detectHighFrequencyContent(float sample) noexcept;
};

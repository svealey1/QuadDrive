#pragma once

#include <cmath>
#include <algorithm>

/**
 * @class TransientPreservingEnvelope
 * @brief Hybrid RMS + peak detector for natural envelope following
 *
 * Preserves transient peaks while maintaining smooth RMS-based envelope.
 * Ideal for Slow Limiter to prevent transient squashing.
 *
 * **Key insight:** Peak detector runs in parallel with RMS and dominates
 * in the transient region, ensuring punchy response.
 */
class TransientPreservingEnvelope {
public:
    TransientPreservingEnvelope();

    /**
     * Get envelope value for current sample
     */
    float getEnvelope(float input) noexcept;

    /**
     * Set RMS smoothing time (milliseconds)
     * Higher = smoother envelope (default: 30ms)
     */
    void setRMSSmoothingMs(float ms, float sampleRate) noexcept;

    /**
     * Set peak decay rate (0–1)
     * Higher = faster decay (default: 0.95 per sample)
     */
    void setPeakDecayRate(float rate) noexcept;

    /**
     * Reset internal state
     */
    void reset() noexcept;

private:
    float rmsValue = 0.0f;
    float peakValue = 0.0f;

    float rmsCoefficient = 0.99f;  // Smoothing: higher = smoother
    float peakDecay = 0.95f;        // Decay: higher = slower falloff

    // Helper
    float squareFloat(float x) const noexcept { return x * x; }
};

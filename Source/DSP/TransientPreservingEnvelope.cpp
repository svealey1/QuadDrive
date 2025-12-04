#include "TransientPreservingEnvelope.h"

TransientPreservingEnvelope::TransientPreservingEnvelope() {
    reset();
}

float TransientPreservingEnvelope::getEnvelope(float input) noexcept {
    float absInput = std::abs(input);

    // Update RMS (smooth, slow response)
    rmsValue = rmsValue * rmsCoefficient +
              absInput * (1.0f - rmsCoefficient);

    // Update peak detector (fast capture, exponential decay)
    peakValue = std::max(absInput, peakValue * peakDecay);

    // Blend: Use whichever is higher, with slight dampening of peak
    // This ensures transients dominate but RMS provides stability
    return std::max(rmsValue, peakValue * 0.85f);
}

void TransientPreservingEnvelope::setRMSSmoothingMs(float ms, float sampleRate) noexcept {
    // Convert ms to coefficient
    // Higher ms = smoother = coefficient closer to 1.0
    float windowSamples = (ms / 1000.0f) * sampleRate;
    rmsCoefficient = 1.0f - (1.0f / (windowSamples + 1.0f));
    rmsCoefficient = std::max(0.9f, std::min(0.999f, rmsCoefficient));
}

void TransientPreservingEnvelope::setPeakDecayRate(float rate) noexcept {
    peakDecay = std::max(0.5f, std::min(0.99f, rate));
}

void TransientPreservingEnvelope::reset() noexcept {
    rmsValue = 0.0f;
    peakValue = 0.0f;
}

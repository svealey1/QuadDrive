#include "AdaptiveReleaseLimiter.h"

AdaptiveReleaseLimiter::AdaptiveReleaseLimiter() {
    reset();
}

float AdaptiveReleaseLimiter::processSample(float input, float threshold) noexcept {
    // Analyze signal to determine adaptive release
    analyzeSignalCharacter(input);

    float absInput = std::abs(input);

    if (absInput > threshold) {
        // Peak detected: immediate attack
        gainState = threshold / std::max(threshold, absInput);
    } else {
        // Release phase: use adaptive release envelope
        float releaseTimeSamples = calculateAdaptiveReleaseTime();
        float releaseCoeff = 1.0f - (1.0f / (releaseTimeSamples + 1.0f));

        // Exponential release envelope
        gainState = gainState * releaseCoeff + 1.0f * (1.0f - releaseCoeff);
    }

    return input * gainState;
}

void AdaptiveReleaseLimiter::analyzeSignalCharacter(float sample) noexcept {
    // Energy density: Count of peaks in window
    float absInput = std::abs(sample);
    float isPeak = (absInput > 0.7f) ? 1.0f : 0.0f;
    energyDensity = energyDensity * 0.95f + isPeak * 0.05f;

    // Spectral character: High-frequency content via sample-to-sample difference
    spectralCharacter = detectHighFrequencyContent(sample);
}

float AdaptiveReleaseLimiter::calculateAdaptiveReleaseTime() noexcept {
    float releaseTime = baseReleaseSamples;

    // Dense signal + low frequency = longer release (protect bass, prevent pumping)
    float densityFactor = energyDensity * 1.5f;  // Up to 1.5x longer for dense signals

    // High frequency content = shorter release (natural transient feel)
    float spectralFactor = (1.0f - spectralCharacter) * 0.5f;

    // Calculate final release time
    releaseTime = baseReleaseSamples * (1.0f + densityFactor) * (1.0f + spectralFactor);

    // Clamp to bounds
    return std::max(minReleaseSamples, std::min(maxReleaseSamples, releaseTime));
}

float AdaptiveReleaseLimiter::detectHighFrequencyContent(float sample) noexcept {
    // Quick spectral estimate: sample-to-sample difference magnitude
    float diff = std::abs(sample - lastSample);
    lastSample = sample;

    // Normalize and smooth
    return std::min(1.0f, diff * 2.0f) * 0.3f + spectralCharacter * 0.7f;
}

void AdaptiveReleaseLimiter::setBaseReleaseTimeMs(float ms, float sampleRate) noexcept {
    currentSampleRate = sampleRate;
    baseReleaseSamples = (ms / 1000.0f) * sampleRate;
}

void AdaptiveReleaseLimiter::setReleaseTimeBounds(float minMs, float maxMs,
                                                  float sampleRate) noexcept {
    currentSampleRate = sampleRate;
    minReleaseSamples = (minMs / 1000.0f) * sampleRate;
    maxReleaseSamples = (maxMs / 1000.0f) * sampleRate;
}

float AdaptiveReleaseLimiter::getCurrentReleaseTimeMs() const noexcept {
    return (baseReleaseSamples / currentSampleRate) * 1000.0f;
}

void AdaptiveReleaseLimiter::reset() noexcept {
    gainState = 1.0f;
    energyDensity = 0.0f;
    spectralCharacter = 0.0f;
    lastSample = 0.0f;
}

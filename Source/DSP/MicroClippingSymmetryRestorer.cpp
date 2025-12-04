#include "MicroClippingSymmetryRestorer.h"

MicroClippingSymmetryRestorer::MicroClippingSymmetryRestorer() {
    reset();
}

void MicroClippingSymmetryRestorer::analyzeBlock(
    const juce::AudioBuffer<float>& buffer, int numSamples) {

    if (!enabled) {
        asymmetryFactor = 0.0f;
        return;
    }

    // Reset peak detection for this block
    positivePeak = 0.0f;
    negativePeak = 0.0f;

    // Scan all channels for peaks
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        const auto* data = buffer.getReadPointer(ch);

        for (int i = 0; i < numSamples; ++i) {
            float sample = data[i];
            positivePeak = std::max(positivePeak, sample);
            negativePeak = std::min(negativePeak, sample);
        }
    }

    // Convert to magnitudes
    positivePeak = std::abs(positivePeak);
    negativePeak = std::abs(negativePeak);

    // Calculate asymmetry ratio
    float maxPeak = std::max(positivePeak, negativePeak);
    if (maxPeak < 1e-6f) {
        asymmetryFactor = 0.0f;
        return;
    }

    float minPeak = std::min(positivePeak, negativePeak);
    float asymmetryRatio = maxPeak / std::max(1e-6f, minPeak);

    // Determine correction intensity
    float targetAsymmetry = 0.0f;
    if (asymmetryRatio > ASYMMETRY_RATIO_THRESHOLD) {
        // Map ratio to correction: higher ratio = more correction
        // Range: 0 (symmetric) to 1 (highly asymmetric)
        targetAsymmetry = std::min(1.0f, (asymmetryRatio - 1.0f) * 0.8f);
    }

    // Smooth asymmetry factor to prevent gain jumps
    asymmetryFactor = asymmetryFactor * ASYMMETRY_SMOOTH +
                     targetAsymmetry * (1.0f - ASYMMETRY_SMOOTH);
}

float MicroClippingSymmetryRestorer::processSample(float sample) noexcept {
    if (!enabled || asymmetryFactor < 0.01f) {
        return sample;
    }

    // Calculate differential correction for this sample
    float correctionAmount = calculateDifferentialCorrection(sample);

    // Apply correction as multiplicative gain
    return sample * (1.0f - correctionAmount * strength);
}

float MicroClippingSymmetryRestorer::calculateDifferentialCorrection(
    float sample) noexcept {

    // Only correct the upper 10% of magnitude range (peaks only)
    float magnitude = std::abs(sample);

    if (magnitude < 0.9f) {
        return 0.0f;  // No correction in bulk signal
    }

    // In peak region (0.9–1.0), apply smoothed correction
    float peakFraction = (magnitude - 0.9f) / 0.1f;

    // Smoothstep curve for click prevention
    float correctionCurve = peakFraction * peakFraction * (3.0f - 2.0f * peakFraction);

    // Scale by asymmetry factor
    return correctionCurve * asymmetryFactor;
}

void MicroClippingSymmetryRestorer::updatePeaks(float sample) noexcept {
    // Not used in current implementation, but available for future optimization
}

void MicroClippingSymmetryRestorer::setStrength(float newStrength) noexcept {
    strength = juce::jlimit(0.0f, 1.0f, newStrength);
}

void MicroClippingSymmetryRestorer::setEnabled(bool newEnabled) noexcept {
    enabled = newEnabled;
}

float MicroClippingSymmetryRestorer::getAsymmetryFactor() const noexcept {
    return asymmetryFactor;
}

void MicroClippingSymmetryRestorer::reset() noexcept {
    asymmetryFactor = 0.0f;
    positivePeak = 0.0f;
    negativePeak = 0.0f;
}

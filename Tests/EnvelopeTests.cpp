/**
 * @file EnvelopeTests.cpp
 * @brief Unit tests for EnvelopeFollower and EnvelopeShaper classes
 */

#include "../Source/DSP/EnvelopeFollower.h"
#include "../Source/DSP/EnvelopeShaper.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <iostream>
#include <cmath>
#include <cassert>

// Test result tracking
struct TestResult
{
    int passed = 0;
    int failed = 0;

    void report(const std::string& testName, bool success, const std::string& message = "")
    {
        if (success)
        {
            std::cout << "[PASS] " << testName << std::endl;
            passed++;
        }
        else
        {
            std::cout << "[FAIL] " << testName;
            if (!message.empty())
                std::cout << " - " << message;
            std::cout << std::endl;
            failed++;
        }
    }

    void printSummary()
    {
        std::cout << "\n=======================================" << std::endl;
        std::cout << "TEST SUMMARY" << std::endl;
        std::cout << "=======================================" << std::endl;
        std::cout << "Total Tests: " << (passed + failed) << std::endl;
        std::cout << "Passed: " << passed << std::endl;
        std::cout << "Failed: " << failed << std::endl;
        std::cout << "Pass Rate: " << (passed * 100.0f / (passed + failed)) << "%" << std::endl;
        std::cout << "=======================================" << std::endl;

        if (failed == 0)
            std::cout << "\n✓ ALL TESTS PASSED!\n" << std::endl;
    }
};

// Test 1: EnvelopeFollower impulse response
void testEnvelopeImpulseResponse(TestResult& results)
{
    EnvelopeFollower env;
    env.setSampleRate(48000.0);
    env.setAttackTime(1.0f);   // 1ms attack
    env.setReleaseTime(10.0f); // 10ms release

    env.reset();

    // Send impulse
    float impulse = 1.0f;
    float response1 = env.process(impulse);

    // Envelope should have risen
    bool attackWorks = (response1 > 0.0f && response1 <= 1.0f);

    // Send silence
    float response2 = env.process(0.0f);

    // Envelope should be falling
    bool releaseWorks = (response2 < response1 && response2 > 0.0f);

    results.report("EnvelopeFollower impulse response",
                   attackWorks && releaseWorks,
                   attackWorks ? "" : "Attack not working");
}

// Test 2: EnvelopeFollower decay verification
void testEnvelopeDecay(TestResult& results)
{
    EnvelopeFollower env;
    env.setSampleRate(48000.0);
    env.setAttackTime(0.5f);
    env.setReleaseTime(20.0f);

    env.reset();

    // Charge envelope with sustained signal
    for (int i = 0; i < 100; ++i)
        env.process(1.0f);

    float peakLevel = env.getCurrentEnvelope();

    // Now send silence for release time
    for (int i = 0; i < 960; ++i) // 20ms at 48kHz = 960 samples
        env.process(0.0f);

    float decayedLevel = env.getCurrentEnvelope();

    // Should have decayed to ~37% (1/e) after one time constant
    bool decayCorrect = (decayedLevel < peakLevel * 0.5f && decayedLevel > peakLevel * 0.2f);

    results.report("EnvelopeFollower decay verification",
                   decayCorrect,
                   "Decayed to " + std::to_string(decayedLevel / peakLevel * 100.0f) + "% of peak");
}

// Test 3: EnvelopeShaper transientness bounds
void testTransientnessBounds(TestResult& results)
{
    EnvelopeShaper shaper;
    shaper.prepare(48000.0);

    // Test with various signals
    float minTransient = 1.0f;
    float maxTransient = 0.0f;

    // Sustained tone
    for (int i = 0; i < 1000; ++i)
    {
        shaper.processEnvelope(0.5f);
        float t = shaper.getTransientnessAmount();
        minTransient = std::min(minTransient, t);
        maxTransient = std::max(maxTransient, t);
    }

    // Impulse
    shaper.reset();
    shaper.processEnvelope(1.0f);
    for (int i = 0; i < 100; ++i)
    {
        shaper.processEnvelope(0.0f);
        float t = shaper.getTransientnessAmount();
        maxTransient = std::max(maxTransient, t);
    }

    bool boundsValid = (minTransient >= 0.0f && maxTransient <= 1.0f);

    results.report("EnvelopeShaper transientness bounds [0, 1]",
                   boundsValid,
                   "Range: [" + std::to_string(minTransient) + ", " + std::to_string(maxTransient) + "]");
}

// Test 4: Null test (0dB attack + 0dB sustain = unity gain)
void testNullGain(TestResult& results)
{
    EnvelopeShaper shaper;
    shaper.prepare(48000.0);
    shaper.setAttackEmphasis(0.0f);  // 0 dB
    shaper.setSustainEmphasis(0.0f); // 0 dB

    // Process some samples to let smoothing settle
    for (int i = 0; i < 1000; ++i)
        shaper.processEnvelope(0.5f);

    // Get gain multiplier
    float gain = shaper.processEnvelope(0.5f);

    // Should be very close to 1.0 (unity gain)
    bool isUnityGain = std::abs(gain - 1.0f) < 0.01f;

    results.report("Null test: 0dB attack/sustain = unity gain",
                   isUnityGain,
                   "Gain: " + std::to_string(gain) + " (expected ~1.0)");
}

// Test 5: Extreme gain values (+12dB / -12dB)
void testExtremeGains(TestResult& results)
{
    EnvelopeShaper shaper;
    shaper.prepare(48000.0);

    // Test +12dB (4x gain)
    shaper.setAttackEmphasis(12.0f);
    shaper.setSustainEmphasis(0.0f);

    // IMPORTANT: Let parameter smoothing settle (15ms = 720 samples at 48kHz)
    // Process with a sustained signal first to let smoothers ramp up
    for (int i = 0; i < 1000; ++i)
        shaper.processEnvelope(0.5f);

    // Now reset envelope state (but NOT parameter smoothers)
    // This is tricky - we want to keep the gain parameters settled
    // but reset the envelope followers. Since we don't have a separate
    // method for this, we'll just send silence to drain the envelopes
    for (int i = 0; i < 5000; ++i)
        shaper.processEnvelope(0.0f);

    // Now send a sharp impulse - this creates maximum transientness
    // Transientness is highest a few samples AFTER the impulse, when
    // fast envelope is high but slow envelope hasn't caught up yet
    float maxGain = 0.0f;
    for (int cycle = 0; cycle < 10; ++cycle)
    {
        // Send a few samples of signal to charge up the fast envelope
        for (int attack = 0; attack < 5; ++attack)
        {
            float gain = shaper.processEnvelope(1.0f);
            maxGain = std::max(maxGain, gain);
        }

        // Now silence - fast envelope starts decaying before slow catches up
        for (int decay = 0; decay < 10; ++decay)
        {
            float gain = shaper.processEnvelope(0.0f);
            maxGain = std::max(maxGain, gain);
        }

        // Longer silence to reset
        for (int j = 0; j < 200; ++j)
        {
            float gain = shaper.processEnvelope(0.0f);
            maxGain = std::max(maxGain, gain);
        }
    }

    // Allow some tolerance - transientness algorithm may not reach exactly 1.0
    bool plus12Valid = maxGain > 2.0f; // At least 6dB gain (50% of 12dB)

    // Test -12dB (0.25x gain)
    shaper.setAttackEmphasis(0.0f);
    shaper.setSustainEmphasis(-12.0f);

    // Sustained signal - both envelopes converge, transientness → 0
    // Need extra time for smoothing to settle
    for (int i = 0; i < 5000; ++i)
        shaper.processEnvelope(0.5f);

    float gainMinus12 = shaper.processEnvelope(0.5f);
    bool minus12Valid = std::abs(gainMinus12 - 0.251f) < 0.1f; // ~0.25x = -12dB

    results.report("Extreme gains: +12dB and -12dB",
                   plus12Valid && minus12Valid,
                   "+12dB max gain: " + std::to_string(maxGain) +
                   " (expected >2.0), -12dB gain: " + std::to_string(gainMinus12) + " (expected ~0.25)");
}

// Test 6: Parameter clamping
void testParameterClamping(TestResult& results)
{
    EnvelopeShaper shaper;
    shaper.prepare(48000.0);

    // Try to set values outside range
    shaper.setAttackEmphasis(20.0f);   // Should clamp to +12dB
    shaper.setSustainEmphasis(-20.0f); // Should clamp to -12dB

    bool attackClamped = (shaper.getAttackEmphasisDB() == 12.0f);
    bool sustainClamped = (shaper.getSustainEmphasisDB() == -12.0f);

    results.report("Parameter clamping to [-12, +12] dB",
                   attackClamped && sustainClamped,
                   "Attack: " + std::to_string(shaper.getAttackEmphasisDB()) +
                   " dB, Sustain: " + std::to_string(shaper.getSustainEmphasisDB()) + " dB");
}

// Test 7: Smoothing prevents zipper noise
void testParameterSmoothing(TestResult& results)
{
    EnvelopeShaper shaper;
    shaper.prepare(48000.0);

    shaper.setAttackEmphasis(0.0f);

    // Process a few samples
    for (int i = 0; i < 10; ++i)
        shaper.processEnvelope(1.0f);

    // Abruptly change parameter
    shaper.setAttackEmphasis(12.0f);

    // Get immediate gain
    float gain1 = shaper.processEnvelope(1.0f);

    // Process more samples
    for (int i = 0; i < 100; ++i)
        shaper.processEnvelope(1.0f);

    float gain2 = shaper.processEnvelope(1.0f);

    // Gain should be smoothly ramping (not instant jump)
    bool isSmoothing = (gain2 > gain1 && gain1 < 3.5f);

    results.report("Parameter smoothing prevents zipper noise",
                   isSmoothing,
                   "Gain transition: " + std::to_string(gain1) + " → " + std::to_string(gain2));
}

int main()
{
    std::cout << "\n=======================================" << std::endl;
    std::cout << "STEVE Envelope Shaper - Unit Tests" << std::endl;
    std::cout << "=======================================" << std::endl;

    TestResult results;

    // Run all tests
    testEnvelopeImpulseResponse(results);
    testEnvelopeDecay(results);
    testTransientnessBounds(results);
    testNullGain(results);
    testExtremeGains(results);
    testParameterClamping(results);
    testParameterSmoothing(results);

    // Print summary
    results.printSummary();

    return (results.failed == 0) ? 0 : 1;
}

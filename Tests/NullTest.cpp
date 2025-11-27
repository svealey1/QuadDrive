#include "../Source/PluginProcessor.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <iostream>
#include <cmath>
#include <vector>

// Null Test Validation for QuadBlend Drive v1.8.0
// Tests bypass transparency, mix correctness, and unity gain behavior

class NullTestRunner
{
public:
    NullTestRunner() : processor()
    {
        // Initialize processor
        processor.prepareToPlay(sampleRate, blockSize);
    }

    ~NullTestRunner()
    {
        processor.releaseResources();
    }

    // Generate test signals
    static void generateSineSweep(juce::AudioBuffer<float>& buffer, double sampleRate, double startFreq, double endFreq)
    {
        const int numSamples = buffer.getNumSamples();
        const double duration = numSamples / sampleRate;

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* data = buffer.getWritePointer(ch);
            for (int i = 0; i < numSamples; ++i)
            {
                const double t = i / sampleRate;
                const double phase = 2.0 * juce::MathConstants<double>::pi *
                                    (startFreq * t + (endFreq - startFreq) * t * t / (2.0 * duration));
                data[i] = static_cast<float>(std::sin(phase) * 0.5);  // -6 dBFS
            }
        }
    }

    static void generateImpulse(juce::AudioBuffer<float>& buffer)
    {
        buffer.clear();
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            buffer.setSample(ch, 100, 1.0f);  // Impulse at sample 100
        }
    }

    static void generatePinkNoise(juce::AudioBuffer<float>& buffer)
    {
        juce::Random random;

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* data = buffer.getWritePointer(ch);

            // Simple pink noise using white noise + filtering
            float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f, b3 = 0.0f, b4 = 0.0f, b5 = 0.0f, b6 = 0.0f;

            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                float white = random.nextFloat() * 2.0f - 1.0f;

                // Paul Kellet's pink noise filter
                b0 = 0.99886f * b0 + white * 0.0555179f;
                b1 = 0.99332f * b1 + white * 0.0750759f;
                b2 = 0.96900f * b2 + white * 0.1538520f;
                b3 = 0.86650f * b3 + white * 0.3104856f;
                b4 = 0.55000f * b4 + white * 0.5329522f;
                b5 = -0.7616f * b5 - white * 0.0168980f;

                float pink = b0 + b1 + b2 + b3 + b4 + b5 + b6 + white * 0.5362f;
                b6 = white * 0.115926f;

                data[i] = pink * 0.11f;  // Scale to -18 dBFS
            }
        }
    }

    // Compute max difference in dB between two buffers
    static double computeMaxDifferenceDB(const juce::AudioBuffer<float>& buffer1,
                                         const juce::AudioBuffer<float>& buffer2)
    {
        double maxDiff = 0.0;

        for (int ch = 0; ch < buffer1.getNumChannels(); ++ch)
        {
            const auto* data1 = buffer1.getReadPointer(ch);
            const auto* data2 = buffer2.getReadPointer(ch);

            for (int i = 0; i < buffer1.getNumSamples(); ++i)
            {
                double diff = std::abs(static_cast<double>(data1[i]) - static_cast<double>(data2[i]));
                maxDiff = std::max(maxDiff, diff);
            }
        }

        if (maxDiff < 1e-10)  // Effectively zero
            return -200.0;  // Below noise floor

        return 20.0 * std::log10(maxDiff);
    }

    // Check if two buffers are bit-identical
    static bool areBitIdentical(const juce::AudioBuffer<float>& buffer1,
                               const juce::AudioBuffer<float>& buffer2)
    {
        if (buffer1.getNumChannels() != buffer2.getNumChannels() ||
            buffer1.getNumSamples() != buffer2.getNumSamples())
            return false;

        for (int ch = 0; ch < buffer1.getNumChannels(); ++ch)
        {
            const auto* data1 = buffer1.getReadPointer(ch);
            const auto* data2 = buffer2.getReadPointer(ch);

            for (int i = 0; i < buffer1.getNumSamples(); ++i)
            {
                if (data1[i] != data2[i])  // Bit-exact comparison
                    return false;
            }
        }

        return true;
    }

    // Set parameter by ID (using denormalized value, not normalized 0-1)
    void setParameter(const juce::String& paramID, float denormalizedValue)
    {
        if (auto* param = processor.apvts.getParameter(paramID))
        {
            // Convert denormalized value to normalized (0-1) range
            auto range = processor.apvts.getParameterRange(paramID);
            float normalizedValue = range.convertTo0to1(denormalizedValue);
            param->setValueNotifyingHost(normalizedValue);
        }
    }

    // Reset all parameters to defaults
    void resetToDefaults()
    {
        // Bypass OFF
        setParameter("BYPASS", 0.0f);

        // Unity gains (0 dB)
        setParameter("INPUT_GAIN", 0.0f);
        setParameter("OUTPUT_GAIN", 0.0f);

        // 100% wet
        setParameter("MIX_WET", 100.0f);

        // Threshold at 0 dB
        setParameter("THRESHOLD", 0.0f);

        // XY at center (all processors blended)
        setParameter("XY_X_PARAM", 0.5f);
        setParameter("XY_Y_PARAM", 0.5f);

        // Disable protection limiters
        setParameter("PROTECTION_ENABLE", 0.0f);

        // Delta mode OFF
        setParameter("DELTA_MODE", 0.0f);

        // Processing Mode 0 (Zero Latency)
        setParameter("PROCESSING_MODE", 0.0f);
    }

    // Process a buffer through the plugin
    void processBuffer(juce::AudioBuffer<float>& buffer)
    {
        juce::MidiBuffer midiBuffer;

        // CRITICAL: Process warm-up blocks to let parameter smoothers settle
        // Smoothers ramp over 20ms, so at 48kHz with 512-sample blocks:
        // 20ms = 960 samples = ~2 blocks needed for smoothers to fully settle
        // Use 10 blocks (100ms) to ensure smoothers have completely converged
        juce::AudioBuffer<float> warmupBuffer(buffer.getNumChannels(), blockSize);
        warmupBuffer.clear();
        juce::MidiBuffer warmupMidi;

        for (int i = 0; i < 10; ++i)  // 10 blocks = 100ms = plenty of time for convergence
        {
            processor.processBlock(warmupBuffer, warmupMidi);
        }

        // Now process the actual buffer with settled smoothers
        processor.processBlock(buffer, midiBuffer);
    }

    // TEST 1: Bypass Null Test
    bool testBypassNull(const juce::String& signalName, juce::AudioBuffer<float>& inputBuffer)
    {
        std::cout << "\n=== TEST 1: Bypass Null Test (" << signalName << ") ===" << std::endl;

        // Make a copy of input
        juce::AudioBuffer<float> outputBuffer;
        outputBuffer.makeCopyOf(inputBuffer);

        // Enable bypass
        setParameter("BYPASS", 1.0f);

        // Process
        processBuffer(outputBuffer);

        // Check if bit-identical
        bool bitIdentical = areBitIdentical(inputBuffer, outputBuffer);
        double maxDiffDB = computeMaxDifferenceDB(inputBuffer, outputBuffer);

        std::cout << "Bit-identical: " << (bitIdentical ? "PASS" : "FAIL") << std::endl;
        std::cout << "Max difference: " << maxDiffDB << " dB" << std::endl;

        bool pass = bitIdentical || maxDiffDB < -120.0;
        std::cout << "Result: " << (pass ? "PASS" : "FAIL") << std::endl;

        return pass;
    }

    // TEST 2: Mix 0% Null Test
    bool testMix0Null(const juce::String& signalName, juce::AudioBuffer<float>& inputBuffer)
    {
        std::cout << "\n=== TEST 2: Mix 0% Null Test (" << signalName << ") ===" << std::endl;

        // Make a copy of input
        juce::AudioBuffer<float> outputBuffer;
        outputBuffer.makeCopyOf(inputBuffer);

        // Disable bypass
        setParameter("BYPASS", 0.0f);

        // Set mix to 0% (fully dry)
        setParameter("MIX_WET", 0.0f);

        // Set some processing to ensure wet path is active
        setParameter("INPUT_GAIN", 12.0f);  // +12 dB drive
        setParameter("THRESHOLD", -6.0f);   // -6 dB threshold

        // Process
        processBuffer(outputBuffer);

        // Check if bit-identical to input (dry signal should be pristine)
        bool bitIdentical = areBitIdentical(inputBuffer, outputBuffer);
        double maxDiffDB = computeMaxDifferenceDB(inputBuffer, outputBuffer);

        std::cout << "Bit-identical: " << (bitIdentical ? "PASS" : "FAIL") << std::endl;
        std::cout << "Max difference: " << maxDiffDB << " dB" << std::endl;

        bool pass = bitIdentical || maxDiffDB < -120.0;
        std::cout << "Result: " << (pass ? "PASS" : "FAIL") << std::endl;

        return pass;
    }

    // TEST 3: Unity Gain Test (No Processing)
    bool testUnityGain(const juce::String& signalName, juce::AudioBuffer<float>& inputBuffer)
    {
        std::cout << "\n=== TEST 3: Unity Gain Test (" << signalName << ") ===" << std::endl;

        // Make a copy of input
        juce::AudioBuffer<float> outputBuffer;
        outputBuffer.makeCopyOf(inputBuffer);

        // Disable bypass
        setParameter("BYPASS", 0.0f);

        // Unity gains
        setParameter("INPUT_GAIN", 0.0f);   // 0 dB
        setParameter("OUTPUT_GAIN", 0.0f);  // 0 dB
        setParameter("MIX_WET", 100.0f);    // 100% wet

        // Mute all processors (zero weight = no processing)
        setParameter("HC_MUTE", 1.0f);
        setParameter("SC_MUTE", 1.0f);
        setParameter("SL_MUTE", 1.0f);
        setParameter("FL_MUTE", 1.0f);

        // Disable protection
        setParameter("PROTECTION_ENABLE", 0.0f);

        // DEBUG: Print parameter values
        std::cout << "DEBUG: INPUT_GAIN = " << processor.apvts.getRawParameterValue("INPUT_GAIN")->load() << " dB" << std::endl;
        std::cout << "DEBUG: OUTPUT_GAIN = " << processor.apvts.getRawParameterValue("OUTPUT_GAIN")->load() << " dB" << std::endl;
        std::cout << "DEBUG: MIX_WET = " << processor.apvts.getRawParameterValue("MIX_WET")->load() << " %" << std::endl;

        // Process
        processBuffer(outputBuffer);

        // Check if bit-identical (all processors muted = passthrough)
        bool bitIdentical = areBitIdentical(inputBuffer, outputBuffer);
        double maxDiffDB = computeMaxDifferenceDB(inputBuffer, outputBuffer);

        std::cout << "Bit-identical: " << (bitIdentical ? "PASS" : "FAIL") << std::endl;
        std::cout << "Max difference: " << maxDiffDB << " dB" << std::endl;

        // Note: With oversampling, bit-identical may not be achievable
        // Accept < -100 dB as pass (below 16-bit noise floor)
        bool pass = maxDiffDB < -100.0;
        std::cout << "Result: " << (pass ? "PASS" : "FAIL") << std::endl;

        return pass;
    }

    // Run all tests
    void runAllTests()
    {
        std::cout << "\n=======================================" << std::endl;
        std::cout << "QuadBlend Drive v1.8.0 - Null Test Suite" << std::endl;
        std::cout << "=======================================" << std::endl;

        const int testBufferSize = 48000;  // 1 second at 48kHz

        // Test with different signal types
        std::vector<std::pair<juce::String, juce::AudioBuffer<float>>> testSignals;

        // Sine sweep (20 Hz - 20 kHz)
        juce::AudioBuffer<float> sineSweep(2, testBufferSize);
        generateSineSweep(sineSweep, sampleRate, 20.0, 20000.0);
        testSignals.push_back({"Sine Sweep", sineSweep});

        // Impulse
        juce::AudioBuffer<float> impulse(2, testBufferSize);
        generateImpulse(impulse);
        testSignals.push_back({"Impulse", impulse});

        // Pink noise
        juce::AudioBuffer<float> pinkNoise(2, testBufferSize);
        generatePinkNoise(pinkNoise);
        testSignals.push_back({"Pink Noise", pinkNoise});

        int totalTests = 0;
        int passedTests = 0;

        // Run all tests on all signals
        for (auto& [signalName, buffer] : testSignals)
        {
            // Reset processor state before each test
            processor.releaseResources();
            processor.prepareToPlay(sampleRate, blockSize);
            resetToDefaults();

            // TEST 1: Bypass
            totalTests++;
            if (testBypassNull(signalName, buffer))
                passedTests++;

            // Reset processor
            processor.releaseResources();
            processor.prepareToPlay(sampleRate, blockSize);
            resetToDefaults();

            // TEST 2: Mix 0%
            totalTests++;
            if (testMix0Null(signalName, buffer))
                passedTests++;

            // Reset processor
            processor.releaseResources();
            processor.prepareToPlay(sampleRate, blockSize);
            resetToDefaults();

            // TEST 3: Unity Gain
            totalTests++;
            if (testUnityGain(signalName, buffer))
                passedTests++;
        }

        // Print summary
        std::cout << "\n=======================================" << std::endl;
        std::cout << "TEST SUMMARY" << std::endl;
        std::cout << "=======================================" << std::endl;
        std::cout << "Total Tests: " << totalTests << std::endl;
        std::cout << "Passed: " << passedTests << std::endl;
        std::cout << "Failed: " << (totalTests - passedTests) << std::endl;
        std::cout << "Pass Rate: " << (100.0 * passedTests / totalTests) << "%" << std::endl;
        std::cout << "=======================================" << std::endl;

        if (passedTests == totalTests)
        {
            std::cout << "\n✓ ALL TESTS PASSED!" << std::endl;
        }
        else
        {
            std::cout << "\n✗ SOME TESTS FAILED - Review output above" << std::endl;
        }
    }

private:
    QuadBlendDriveAudioProcessor processor;
    const double sampleRate = 48000.0;
    const int blockSize = 512;
};

// Main entry point
int main(int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI scopedJuce;

    NullTestRunner testRunner;
    testRunner.runAllTests();

    return 0;
}

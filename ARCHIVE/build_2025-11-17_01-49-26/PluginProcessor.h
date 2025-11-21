#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

class QuadBlendDriveAudioProcessor : public juce::AudioProcessor
{
public:
    QuadBlendDriveAudioProcessor();
    ~QuadBlendDriveAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    // Template processBlock to support both float and double
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlock(juce::AudioBuffer<double>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getLatencySamples() const { return totalLatencySamples; }  // 3ms + 4ms = 7ms total

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // Normalization state - exposed for UI
    std::atomic<double> currentPeakDB{-60.0};
    std::atomic<double> normalizationGainDB{0.0};
    std::atomic<bool> isAnalyzing{false};
    std::atomic<bool> isNormalizing{false};

    // For UI visualization
    std::atomic<float> currentGainReductionDB{0.0f};

    // Oscilloscope data - ring buffer for waveform display (2000ms)
    std::vector<float> oscilloscopeBuffer;
    std::atomic<int> oscilloscopeSize{0};
    std::atomic<int> oscilloscopeWritePos{0};

    // Normalization control functions
    void startAnalysis();
    void stopAnalysis();
    void enableNormalization();
    void disableNormalization();
    void resetNormalizationData();

    // Preset management (A, B, C, D)
    void savePreset(int slot);  // 0=A, 1=B, 2=C, 3=D
    void recallPreset(int slot);
    bool hasPreset(int slot) const;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Template processing function
    template<typename SampleType>
    void processBlockInternal(juce::AudioBuffer<SampleType>& buffer, juce::MidiBuffer& midiMessages);

    // Processing functions for each type (templated for float/double)
    template<typename SampleType>
    void processHardClip(juce::AudioBuffer<SampleType>& buffer, SampleType threshold, double sampleRate);

    template<typename SampleType>
    void processSoftClip(juce::AudioBuffer<SampleType>& buffer, SampleType ceiling, SampleType knee, double sampleRate);

    template<typename SampleType>
    void processSlowLimit(juce::AudioBuffer<SampleType>& buffer, SampleType threshold, SampleType releaseMs, double sampleRate);

    template<typename SampleType>
    void processFastLimit(juce::AudioBuffer<SampleType>& buffer, SampleType threshold, double sampleRate);

    // Protection limiter (true-peak safety limiter)
    template<typename SampleType>
    void processProtectionLimiter(juce::AudioBuffer<SampleType>& buffer, SampleType ceilingDB, double sampleRate);

    // Normalization helper
    void calculateNormalizationGain();

    // Lookahead buffer state for Hard Clip
    struct HardClipState
    {
        std::vector<double> lookaheadBuffer;
        int lookaheadWritePos{0};
    };
    HardClipState hardClipState[2];  // Per channel

    // Lookahead buffer state for Soft Clip
    struct SoftClipState
    {
        std::vector<double> lookaheadBuffer;
        int lookaheadWritePos{0};
    };
    SoftClipState softClipState[2];  // Per channel

    // Limiter state for adaptive auto-release (Slow Limit)
    struct SlowLimiterState
    {
        double envelope{0.0};
        double adaptiveRelease{100.0};
        double rmsEnvelope{0.0};           // RMS tracking for crest factor
        double peakEnvelope{0.0};          // Peak tracking for crest factor
        double smoothedCrestFactor{1.0};   // Smoothed crest factor with exponential smoothing
        std::vector<double> lookaheadBuffer;
        int lookaheadWritePos{0};
    };
    SlowLimiterState slowLimiterState[2];  // Per channel

    // Limiter state for fast limiter (Hard Knee Fast Limiting)
    struct FastLimiterState
    {
        double envelope{0.0};
        std::vector<double> lookaheadBuffer;
        int lookaheadWritePos{0};
    };
    FastLimiterState fastLimiterState[2];  // Per channel

    // Protection limiter state (true-peak safety limiter at output)
    struct ProtectionLimiterState
    {
        double envelope{0.0};                    // Main gain reduction envelope
        double fastReleaseEnv{0.0};              // Fast release component
        double slowReleaseEnv{0.0};              // Slow release component
        std::vector<double> lookaheadBuffer;     // 4ms lookahead buffer
        int lookaheadWritePos{0};
        // Oversampling buffers for true-peak detection (8x oversampling)
        std::vector<double> oversampledBuffer;   // 8x oversampled signal
    };
    ProtectionLimiterState protectionLimiterState[2];  // Per channel

    int lookaheadSamples{0};
    int protectionLookaheadSamples{0};           // 4ms for protection limiter
    int totalLatencySamples{0};                  // Total plugin latency for DAW compensation

    // Dry signal delay compensation buffers (to match 3ms lookahead)
    struct DryDelayState
    {
        std::vector<double> delayBuffer;
        int writePos{0};
    };
    DryDelayState dryDelayState[2];  // Per channel

    // Buffers for parallel processing (float)
    juce::AudioBuffer<float> dryBufferFloat;
    juce::AudioBuffer<float> tempBuffer1Float;
    juce::AudioBuffer<float> tempBuffer2Float;
    juce::AudioBuffer<float> tempBuffer3Float;
    juce::AudioBuffer<float> tempBuffer4Float;

    // Buffers for parallel processing (double)
    juce::AudioBuffer<double> dryBufferDouble;
    juce::AudioBuffer<double> tempBuffer1Double;
    juce::AudioBuffer<double> tempBuffer2Double;
    juce::AudioBuffer<double> tempBuffer3Double;
    juce::AudioBuffer<double> tempBuffer4Double;

    // Normalization state variables (use double precision for peak detection)
    double peakInputLevel{0.0};
    double calibrationTargetDB{0.0};
    double computedNormalizationGain{1.0};
    bool analyzingEnabled{false};
    bool normalizationEnabled{false};

    double currentSampleRate{44100.0};

    // Preset storage (A, B, C, D)
    juce::ValueTree presetSlots[4];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(QuadBlendDriveAudioProcessor)
};

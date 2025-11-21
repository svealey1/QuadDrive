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
    // Stores three frequency bands (low, mid, high) for RGB coloring
    std::vector<float> oscilloscopeBuffer;      // Main signal
    std::vector<float> oscilloscopeLowBand;     // Low frequencies (R channel)
    std::vector<float> oscilloscopeMidBand;     // Mid frequencies (G channel)
    std::vector<float> oscilloscopeHighBand;    // High frequencies (B channel)
    std::atomic<int> oscilloscopeSize{0};
    std::atomic<int> oscilloscopeWritePos{0};

    // Master meter for output level monitoring
    std::atomic<float> currentOutputPeakL{0.0f};
    std::atomic<float> currentOutputPeakR{0.0f};

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

    // Overshoot suppression (true-peak safety with 8x oversampling)
    template<typename SampleType>
    void processOvershootSuppression(juce::AudioBuffer<SampleType>& buffer, SampleType ceilingDB, double sampleRate, bool enabled = true, juce::AudioBuffer<SampleType>* referenceBuffer = nullptr);

    // Advanced True Peak Limiter with IRC (Intelligent Release Control)
    template<typename SampleType>
    void processAdvancedTPL(juce::AudioBuffer<SampleType>& buffer, SampleType ceilingDB, double sampleRate, juce::AudioBuffer<SampleType>* referenceBuffer = nullptr);

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

    // Advanced True Peak Limiter state (with IRC - Intelligent Release Control)
    struct AdvancedTPLState
    {
        // Lookahead buffer (1-3ms for peak prediction)
        std::vector<double> lookaheadBuffer;
        int lookaheadWritePos{0};

        // Main GR envelope (smoothed)
        double grEnvelope{0.0};

        // Multiband envelope followers for IRC (3 bands: Low, Mid, High)
        double lowBandEnv{0.0};      // <200 Hz
        double midBandEnv{0.0};      // 200-4000 Hz
        double highBandEnv{0.0};     // >4000 Hz

        // Dynamic release time state
        double currentReleaseCoeff{0.0};

        // Multiband filter state (2-pole biquads)
        double lowZ1{0.0}, lowZ2{0.0};
        double midZ1{0.0}, midZ2{0.0};
        double highZ1{0.0}, highZ2{0.0};

        // Filter coefficients (set during prepareToPlay)
        double lowB0{1.0}, lowB1{0.0}, lowB2{0.0}, lowA1{0.0}, lowA2{0.0};
        double midB0{1.0}, midB1{0.0}, midB2{0.0}, midA1{0.0}, midA2{0.0};
        double highB0{1.0}, highB1{0.0}, highB2{0.0}, highA1{0.0}, highA2{0.0};
    };
    AdvancedTPLState advancedTPLState[2];  // Per channel

    // Overshoot suppression with 8x oversampling (linear-phase FIR)
    std::unique_ptr<juce::dsp::Oversampling<float>> oversamplingFloat;
    std::unique_ptr<juce::dsp::Oversampling<double>> oversamplingDouble;

    int lookaheadSamples{0};
    int advancedTPLLookaheadSamples{0};      // Lookahead for advanced TPL (1-3ms)
    int protectionLookaheadSamples{0};           // No longer used (overshoot suppression is zero-latency)
    int totalLatencySamples{0};                  // Total plugin latency for DAW compensation

    // Dry signal delay compensation buffers (to match 3ms lookahead)
    struct DryDelayState
    {
        std::vector<double> delayBuffer;
        int writePos{0};
    };
    DryDelayState dryDelayState[2];  // Per channel

    // Three-band filters for oscilloscope RGB visualization
    struct OscilloscopeBandFilters
    {
        // Low-pass for bass (< 250 Hz)
        double lowZ1{0.0}, lowZ2{0.0};
        double lowB0{1.0}, lowB1{0.0}, lowB2{0.0}, lowA1{0.0}, lowA2{0.0};

        // Band-pass for mids (250 Hz - 4 kHz)
        double midZ1{0.0}, midZ2{0.0};
        double midB0{1.0}, midB1{0.0}, midB2{0.0}, midA1{0.0}, midA2{0.0};

        // High-pass for highs (> 4 kHz)
        double highZ1{0.0}, highZ2{0.0};
        double highB0{1.0}, highB1{0.0}, highB2{0.0}, highA1{0.0}, highA2{0.0};
    };
    OscilloscopeBandFilters oscFilters[2];  // Per channel

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

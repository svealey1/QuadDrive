#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "OversamplingManager.h"
#include "DSP/EnvelopeShaper.h"

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
    double getTailLengthSeconds() const override { return 1.0; }  // 800ms slow limiter release + margin
    int getLatencySamples() const { return totalLatencySamples; }  // 3ms + 4ms = 7ms total

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // === TRANSFER CURVE METER: PARAMETER GETTERS ===
    float getXY_X() const { return apvts.getRawParameterValue("XY_X_PARAM")->load(); }
    float getXY_Y() const { return apvts.getRawParameterValue("XY_Y_PARAM")->load(); }
    float getDriveHC() const { return apvts.getRawParameterValue("HC_TRIM")->load(); }
    float getDriveSC() const { return apvts.getRawParameterValue("SC_TRIM")->load(); }
    float getDriveSL() const { return apvts.getRawParameterValue("SL_TRIM")->load(); }
    float getDriveFL() const { return apvts.getRawParameterValue("FL_TRIM")->load(); }
    int getSoloIndex() const {
        // Return 0-3 for soloed processor, -1 if no solo
        if (apvts.getRawParameterValue("HC_SOLO")->load() > 0.5f) return 0;
        if (apvts.getRawParameterValue("SC_SOLO")->load() > 0.5f) return 1;
        if (apvts.getRawParameterValue("SL_SOLO")->load() > 0.5f) return 2;
        if (apvts.getRawParameterValue("FL_SOLO")->load() > 0.5f) return 3;
        return -1;
    }
    float getCurrentPeakNormalized() const { return juce::jlimit(0.0f, 1.0f, meterPeak.load()); }

    // === STEREO I/O METERS: GETTERS ===
    // Allow peaks up to +6dBFS (linear gain ~2.0) for true peak metering
    float getInputPeakL() const { return juce::jlimit(0.0f, 2.0f, inputPeakL.load()); }
    float getInputPeakR() const { return juce::jlimit(0.0f, 2.0f, inputPeakR.load()); }
    float getOutputPeakL() const { return juce::jlimit(0.0f, 2.0f, outputPeakL.load()); }
    float getOutputPeakR() const { return juce::jlimit(0.0f, 2.0f, outputPeakR.load()); }

    juce::AudioProcessorValueTreeState apvts;

    // Normalization state - exposed for UI
    std::atomic<double> currentPeakDB{-60.0};
    std::atomic<double> normalizationGainDB{0.0};
    std::atomic<bool> isAnalyzing{false};
    std::atomic<bool> isNormalizing{false};

    // For UI visualization
    std::atomic<float> currentGainReductionDB{0.0f};

    // === TRANSFER CURVE METER: PEAK FOLLOWER ===
    std::atomic<float> meterPeak{0.0f};  // Normalized 0-1 peak for transfer curve meter

    // === STEREO I/O METERS: TRUE PEAK FOLLOWERS (ITU-R BS.1770-4 compliant) ===
    std::atomic<float> inputPeakL{0.0f};   // Input true peak left channel
    std::atomic<float> inputPeakR{0.0f};   // Input true peak right channel
    std::atomic<float> outputPeakL{0.0f};  // Output true peak left channel
    std::atomic<float> outputPeakR{0.0f};  // Output true peak right channel

    // === WAVEFORM GR METER: PER-PROCESSOR DATA ===
    // Track per-processor outputs and gain reduction (for waveform GR meter visualization)
    std::atomic<float> currentInputPeak{0.0f};        // Input signal peak
    std::atomic<float> currentHardClipPeak{0.0f};     // HC output peak
    std::atomic<float> currentSoftClipPeak{0.0f};     // SC output peak
    std::atomic<float> currentSlowLimitPeak{0.0f};    // SL output peak
    std::atomic<float> currentFastLimitPeak{0.0f};    // FL output peak
    std::atomic<float> currentHardClipGR{0.0f};       // HC GR (dB, positive = reduction)
    std::atomic<float> currentSoftClipGR{0.0f};       // SC GR (dB, positive = reduction)
    std::atomic<float> currentSlowLimitGR{0.0f};      // SL GR (dB, positive = reduction)
    std::atomic<float> currentFastLimitGR{0.0f};      // FL GR (dB, positive = reduction)

    // Oscilloscope data - ring buffer for waveform display (2000ms)
    // Stores three frequency bands (low, mid, high) for RGB coloring
    std::vector<float> oscilloscopeBuffer;      // Main signal
    std::vector<float> oscilloscopeLowBand;     // Low frequencies (R channel)
    std::vector<float> oscilloscopeMidBand;     // Mid frequencies (G channel)
    std::vector<float> oscilloscopeHighBand;    // High frequencies (B channel)
    std::vector<float> oscilloscopeGR;         // Gain reduction (dB, matches oscilloscopeWindow)
    std::atomic<int> oscilloscopeSize{0};
    std::atomic<int> oscilloscopeWritePos{0};

    // === UNIFIED WAVEFORM + GR DISPLAY BUFFER ===
    // Sample-accurate waveform and gain reduction data for professional oscilloscope display
    struct DisplaySample
    {
        float waveformL = 0.0f;       // Left channel waveform
        float waveformR = 0.0f;       // Right channel waveform
        float gainReduction = 0.0f;   // Gain reduction in dB (positive values)
        float lowBand = 0.0f;         // Low frequency band (<250 Hz) for red channel
        float midBand = 0.0f;         // Mid frequency band (250-4000 Hz) for green channel
        float highBand = 0.0f;        // High frequency band (>4000 Hz) for blue channel

        // === WAVEFORM GR METER: PER-PROCESSOR OUTPUTS ===
        // Raw per-processor outputs (before XY blend weighting)
        float inputSignal = 0.0f;           // After input gain, before drive processing
        float hardClipOutput = 0.0f;        // Raw HC output (not blend-weighted)
        float softClipOutput = 0.0f;        // Raw SC output (not blend-weighted)
        float slowLimitOutput = 0.0f;       // Raw SL output (not blend-weighted)
        float fastLimitOutput = 0.0f;       // Raw FL output (not blend-weighted)
        float finalOutput = 0.0f;           // After XY blend + output gain (what user hears)

        // Legacy fields (kept for compatibility with existing visualizers)
        float hardClipContribution = 0.0f;  // Blended HC contribution (scaled by XY weight)
        float softClipContribution = 0.0f;  // Blended SC contribution (scaled by XY weight)
        float slowLimitContribution = 0.0f; // Blended SL contribution (scaled by XY weight)
        float fastLimitContribution = 0.0f; // Blended FL contribution (scaled by XY weight)

        // === THRESHOLD METER: PER-PROCESSOR GAIN REDUCTION ===
        // Gain reduction = how much each processor pulled the signal down
        // Calculated as: (inputSignal - processorOutput) * blendWeight
        float hardClipGainReduction = 0.0f;   // HC reduction (positive = reduction)
        float softClipGainReduction = 0.0f;   // SC reduction (positive = reduction)
        float slowLimitGainReduction = 0.0f;  // SL reduction (positive = reduction)
        float fastLimitGainReduction = 0.0f;  // FL reduction (positive = reduction)
    };

    // Ring buffer for real-time display capture (8192 samples ~= 170ms at 48kHz)
    static constexpr int displayBufferSize = 8192;
    std::array<DisplaySample, displayBufferSize> displayBuffer;
    std::atomic<int> displayWritePos{0};

    // Decimated display cache for efficient 60fps GUI rendering
    // Each segment stores min/max envelope for accurate peak representation
    struct DecimatedSegment
    {
        float waveformMin = 0.0f;     // Minimum waveform value in segment
        float waveformMax = 0.0f;     // Maximum waveform value in segment
        float grMin = 0.0f;           // Minimum GR in segment (dB)
        float grMax = 0.0f;           // Maximum GR in segment (dB)
        float avgLow = 0.0f;          // Average low band energy
        float avgMid = 0.0f;          // Average mid band energy
        float avgHigh = 0.0f;         // Average high band energy

        // === WAVEFORM GR METER: PER-PROCESSOR OUTPUT MIN/MAX ===
        // Raw per-processor outputs (not blend-weighted)
        float inputMin = 0.0f, inputMax = 0.0f;               // Input signal
        float hardClipMin = 0.0f, hardClipMax = 0.0f;         // HC raw output
        float softClipMin = 0.0f, softClipMax = 0.0f;         // SC raw output
        float slowLimitMin = 0.0f, slowLimitMax = 0.0f;       // SL raw output
        float fastLimitMin = 0.0f, fastLimitMax = 0.0f;       // FL raw output
        float finalOutputMin = 0.0f, finalOutputMax = 0.0f;   // Final blended output

        // === THRESHOLD METER: GAIN REDUCTION MIN/MAX ===
        float hardClipGRMin = 0.0f, hardClipGRMax = 0.0f;
        float softClipGRMin = 0.0f, softClipGRMax = 0.0f;
        float slowLimitGRMin = 0.0f, slowLimitGRMax = 0.0f;
        float fastLimitGRMin = 0.0f, fastLimitGRMax = 0.0f;
    };

    static constexpr int decimatedDisplaySize = 512;  // 512 segments for GUI rendering
    std::array<DecimatedSegment, decimatedDisplaySize> decimatedDisplay;
    std::atomic<bool> decimatedDisplayReady{false};

    // === TRANSPORT SYNC FOR DISPLAY ===
    std::atomic<bool> isPlaying{false};
    std::atomic<bool> wasPlaying{false};
    std::atomic<int64_t> playheadSamplePos{0};  // Sample position from host

    // Display buffer write control
    std::atomic<bool> displayBufferFrozen{false};  // True when stopped

    // Update decimated display cache (called from GUI thread)
    void updateDecimatedDisplay();

    // Update transport state and sync display (called from audio thread)
    void updateTransportState();

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

    // Architecture A: XY Blend processing (runs entirely in OS domain)
    template<typename SampleType>
    void processXYBlend(juce::AudioBuffer<SampleType>& buffer, double osSampleRate);

    // Processing functions for each type (templated for float/double)
    template<typename SampleType>
    void processHardClip(juce::AudioBuffer<SampleType>& buffer, SampleType threshold, double sampleRate);

    template<typename SampleType>
    void processSoftClip(juce::AudioBuffer<SampleType>& buffer, SampleType ceiling, SampleType knee, double sampleRate);

    template<typename SampleType>
    void processSlowLimit(juce::AudioBuffer<SampleType>& buffer, SampleType threshold, SampleType releaseMs, SampleType attackMs, double sampleRate);

    template<typename SampleType>
    void processFastLimit(juce::AudioBuffer<SampleType>& buffer, SampleType threshold, SampleType attackMs, double sampleRate);

    // Overshoot suppression (true-peak safety with 8x oversampling)
    template<typename SampleType>
    void processOvershootSuppression(juce::AudioBuffer<SampleType>& buffer, SampleType ceilingDB, double sampleRate, bool enabled = true, juce::AudioBuffer<SampleType>* referenceBuffer = nullptr);

    // Advanced True Peak Limiter with IRC (Intelligent Release Control)
    template<typename SampleType>
    void processAdvancedTPL(juce::AudioBuffer<SampleType>& buffer, SampleType ceilingDB, double sampleRate, juce::AudioBuffer<SampleType>* referenceBuffer = nullptr);

    // Combined Limiters: Processes both Overshoot and True Peak in single oversample cycle
    // (avoids double oversampling artifacts when both are enabled)
    template<typename SampleType>
    void processCombinedLimiters(juce::AudioBuffer<SampleType>& buffer, SampleType ceilingDB, double sampleRate);

    // True Peak Measurement (ITU-R BS.1770-4 compliant using 4x oversampling)
    template<typename SampleType>
    float measureTruePeak(const SampleType* channelData, int numSamples);

    // Normalization helper
    void calculateNormalizationGain();

    // Lookahead buffer state for Hard Clip
    struct HardClipState
    {
        std::vector<double> lookaheadBuffer;
        int lookaheadWritePos{0};
        // PolyBLEP state (for anti-aliasing only at discontinuities)
        double lastSample{0.0};  // Previous input sample for crossing detection
        // Micro Clip Symmetry Restoration (DC blocker)
        double dcBlockerZ1{0.0};  // Previous input for DC blocker
        double dcBlockerZ2{0.0};  // Previous output for DC blocker
    };
    HardClipState hardClipState[2];  // Per channel

    // Lookahead buffer state for Soft Clip
    struct SoftClipState
    {
        std::vector<double> lookaheadBuffer;
        int lookaheadWritePos{0};
        // ADAA state (Anti-Derivative Anti-Aliasing)
        double x1{0.0};   // Previous input sample
        double x2{0.0};   // Two samples ago (for 3rd order)
        double F1{0.0};   // Antiderivative at x1
        double F2{0.0};   // Antiderivative at x2 (for 3rd order)
        // Micro Clip Symmetry Restoration (DC blocker)
        double dcBlockerZ1{0.0};  // Previous input for DC blocker
        double dcBlockerZ2{0.0};  // Previous output for DC blocker
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
        // Gain smoothing state (prevents control signal aliasing)
        double smoothedGain{1.0};          // Low-pass filtered gain reduction
        // Micro Clip Symmetry Restoration (DC blocker)
        double dcBlockerZ1{0.0};  // Previous input for DC blocker
        double dcBlockerZ2{0.0};  // Previous output for DC blocker
    };
    SlowLimiterState slowLimiterState[2];  // Per channel

    // Limiter state for fast limiter (Hard Knee Fast Limiting)
    struct FastLimiterState
    {
        double envelope{0.0};
        std::vector<double> lookaheadBuffer;
        int lookaheadWritePos{0};
        // Gain smoothing state (prevents control signal aliasing)
        double smoothedGain{1.0};          // Low-pass filtered gain reduction
        // Micro Clip Symmetry Restoration (DC blocker)
        double dcBlockerZ1{0.0};  // Previous input for DC blocker
        double dcBlockerZ2{0.0};  // Previous output for DC blocker
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

    // OSM Mode 0 delay compensation state (to match Mode 1 latency)
    // When Mode 0 is active, this delay ensures constant plugin latency
    struct OSMCompensationState
    {
        std::vector<double> delayBuffer;
        int writePos{0};
    };
    OSMCompensationState osmCompensationState[2];  // Per channel

    // Architecture A: Global oversampling manager (handles ALL 2-channel oversampling)
    OversamplingManager osManager;

    // 4-channel oversamplers for phase-coherent dry/wet processing
    // Process [wetL, wetR, dryL, dryR] together through identical filters
    // Balanced mode (8×)
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling4ChBalancedFloat;
    std::unique_ptr<juce::dsp::Oversampling<double>> oversampling4ChBalancedDouble;
    // Linear Phase mode (16×)
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling4ChLinearFloat;
    std::unique_ptr<juce::dsp::Oversampling<double>> oversampling4ChLinearDouble;

    // Delta mode 4-channel oversampler (for overshoot suppression delta mode)
    // Uses 8× oversampling for [mainL, mainR, refL, refR] phase-coherent processing
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling4ChDeltaFloat;
    std::unique_ptr<juce::dsp::Oversampling<double>> oversampling4ChDeltaDouble;
    // Delta mode 16× oversampler (for Linear Phase mode overshoot delta)
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling4ChDelta16xFloat;
    std::unique_ptr<juce::dsp::Oversampling<double>> oversampling4ChDelta16xDouble;

    // 2-channel oversamplers for protection limiters (overshoot suppression, advanced TPL)
    // Balanced mode (8×)
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling2ChBalancedFloat;
    std::unique_ptr<juce::dsp::Oversampling<double>> oversampling2ChBalancedDouble;
    // Linear Phase mode (16×)
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling2ChLinearFloat;
    std::unique_ptr<juce::dsp::Oversampling<double>> oversampling2ChLinearDouble;

    int lookaheadSamples{0};
    int advancedTPLLookaheadSamples{0};      // Lookahead for advanced TPL (1-3ms)
    int protectionLookaheadSamples{0};           // No longer used (overshoot suppression is zero-latency)
    int totalLatencySamples{0};                  // Total plugin latency REPORTED to DAW host
    int internalDryCompensationSamples{0};       // ACTUAL delay applied to dry signal for wet/dry phase alignment

    // Mode-specific latency measurements (in samples at base sample rate)
    int zeroLatencyModeSamples{0};               // Zero Latency mode latency (should be 0 or minimal)
    int balancedModeLatencySamples{0};           // Balanced mode latency (~32 samples)
    int linearPhaseModeLatencySamples{0};        // Linear Phase mode latency (~128 samples)
    int maxModeLatencySamples{0};                // Maximum of Balanced and Linear Phase modes

    // Dry signal delay compensation buffers (to match processing latency per mode)
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

    // Parameter smoothing (prevents zipper noise during automation)
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> smoothedInputGain;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> smoothedOutputGain;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedMixWet;

    // Buffers for parallel processing (float)
    juce::AudioBuffer<float> dryBufferFloat;
    juce::AudioBuffer<float> tempBuffer1Float;
    juce::AudioBuffer<float> tempBuffer2Float;
    juce::AudioBuffer<float> tempBuffer3Float;
    juce::AudioBuffer<float> tempBuffer4Float;
    juce::AudioBuffer<float> combined4ChFloat;  // For phase-coherent dry/wet processing
    juce::AudioBuffer<float> originalInputBufferFloat;  // Pristine input before any gains
    juce::AudioBuffer<float> protectionDeltaCombined4ChFloat;  // Pre-allocated for overshoot delta mode

    // === DRIVE VISUALIZER LAYER BUFFERS ===
    // Temporary storage for per-layer contributions at base sample rate
    // Populated during processBlock, read during display buffer write
    juce::AudioBuffer<float> layerInputFloat;        // Input signal (after input gain)
    juce::AudioBuffer<float> layerHardClipFloat;     // HC contribution (weighted)
    juce::AudioBuffer<float> layerSoftClipFloat;     // SC contribution (weighted)
    juce::AudioBuffer<float> layerSlowLimitFloat;    // SL contribution (weighted)
    juce::AudioBuffer<float> layerFastLimitFloat;    // FL contribution (weighted)
    juce::AudioBuffer<float> layerFinalOutputFloat;  // Final output (after output gain)

    // Buffers for parallel processing (double)
    juce::AudioBuffer<double> dryBufferDouble;
    juce::AudioBuffer<double> tempBuffer1Double;
    juce::AudioBuffer<double> tempBuffer2Double;
    juce::AudioBuffer<double> tempBuffer3Double;
    juce::AudioBuffer<double> tempBuffer4Double;
    juce::AudioBuffer<double> combined4ChDouble;  // For phase-coherent dry/wet processing
    juce::AudioBuffer<double> originalInputBufferDouble;  // Pristine input before any gains
    juce::AudioBuffer<double> protectionDeltaCombined4ChDouble;  // Pre-allocated for overshoot delta mode

    // === DRIVE VISUALIZER LAYER BUFFERS (DOUBLE PRECISION) ===
    juce::AudioBuffer<double> layerInputDouble;
    juce::AudioBuffer<double> layerHardClipDouble;
    juce::AudioBuffer<double> layerSoftClipDouble;
    juce::AudioBuffer<double> layerSlowLimitDouble;
    juce::AudioBuffer<double> layerFastLimitDouble;
    juce::AudioBuffer<double> layerFinalOutputDouble;

    // Normalization state variables (use double precision for peak detection)
    double peakInputLevel{0.0};
    double calibrationTargetDB{0.0};
    double computedNormalizationGain{1.0};
    bool analyzingEnabled{false};
    bool normalizationEnabled{false};

    double currentSampleRate{44100.0};

    // Preset storage (A, B, C, D)
    juce::ValueTree presetSlots[4];

    // Envelope shapers for pre-drive transient/sustain shaping (one per drive type)
    EnvelopeShaper hardClipShaper;
    EnvelopeShaper softClipShaper;
    EnvelopeShaper slowLimitShaper;
    EnvelopeShaper fastLimitShaper;

    // === SAMPLE-ACCURATE PER-PROCESSOR ENVELOPE FOLLOWERS ===
    // For smooth GR visualization at base rate
    // Fast attack (< 1ms), medium release (~50ms) for responsive yet smooth display
    struct ProcessorEnvelope
    {
        float inputEnv = 0.0f;
        float hardClipEnv = 0.0f;
        float softClipEnv = 0.0f;
        float slowLimitEnv = 0.0f;
        float fastLimitEnv = 0.0f;
        float attackCoeff = 0.0f;
        float releaseCoeff = 0.0f;

        void prepare(double sampleRate)
        {
            // Attack: ~0.5ms, Release: ~50ms
            attackCoeff = static_cast<float>(std::exp(-1.0 / (sampleRate * 0.0005)));
            releaseCoeff = static_cast<float>(std::exp(-1.0 / (sampleRate * 0.050)));
        }

        void processEnvelope(float& env, float input)
        {
            float absInput = std::abs(input);
            if (absInput > env)
                env = attackCoeff * env + (1.0f - attackCoeff) * absInput;
            else
                env = releaseCoeff * env + (1.0f - releaseCoeff) * absInput;
        }
    };
    ProcessorEnvelope processorEnvelope[2];  // Stereo

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(QuadBlendDriveAudioProcessor)
};

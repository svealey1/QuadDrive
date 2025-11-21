#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <memory>

class QuadBlendDriveAudioProcessor : public juce::AudioProcessor
{
public:
    QuadBlendDriveAudioProcessor();
    ~QuadBlendDriveAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // For UI visualization
    std::atomic<float> currentGainReductionDB{0.0f};

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    struct ProcessingChain
    {
        float threshold{0.0f};
        float releaseMs{50.0f};
        float trimGainLinear{1.0f};
        bool compensationEnabled{false};

        juce::dsp::Gain<float> inputTrim;
        juce::dsp::Gain<float> compensationGain;

        struct LimiterState
        {
            float envelope{0.0f};
            float releaseCoeff{0.0f};
        };
        LimiterState limiterState[2];

        void prepare(double sampleRate, int maxBlockSize);
        void reset();

        void processHardClip(juce::AudioBuffer<float>& buffer, float threshold);
        void processSoftClip(juce::AudioBuffer<float>& buffer, float threshold);
        void processSlowLimit(juce::AudioBuffer<float>& buffer, float threshold, float releaseMs, double sampleRate);
        void processFastLimit(juce::AudioBuffer<float>& buffer, float threshold, float releaseMs, double sampleRate);
    };

    ProcessingChain hardClipChain;
    ProcessingChain softClipChain;
    ProcessingChain slowLimitChain;
    ProcessingChain fastLimitChain;

    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;
    int currentOversampleFactor{2};

    juce::AudioBuffer<float> dryBuffer;
    juce::AudioBuffer<float> tempBuffer1;
    juce::AudioBuffer<float> tempBuffer2;
    juce::AudioBuffer<float> tempBuffer3;
    juce::AudioBuffer<float> tempBuffer4;

    juce::LinearSmoothedValue<float> inputPeakL, inputPeakR;
    juce::LinearSmoothedValue<float> outputPeakL, outputPeakR;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(QuadBlendDriveAudioProcessor)
};

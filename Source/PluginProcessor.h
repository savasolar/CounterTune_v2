// PluginProcessor.h

#pragma once

#include <JuceHeader.h>
#include "dywapitchtrack.h"

class CounterTune_v2AudioProcessor  : public juce::AudioProcessor
{
public:
    CounterTune_v2AudioProcessor();
    ~CounterTune_v2AudioProcessor() override;
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;
    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    float getMixFloat() const { return *parameters.getRawParameterValue("mix"); }
    void setMixFloat(float newMixFloat) { auto* param = parameters.getParameter("mix"); auto range = param->getNormalisableRange(); param->setValueNotifyingHost(range.convertTo0to1(newMixFloat)); }
    
    float getTempoFloat() const { return *parameters.getRawParameterValue("tempo"); }
    void setTempoFloat(float newTempoFloat) { auto* param = parameters.getParameter("tempo"); auto range = param->getNormalisableRange(); param->setValueNotifyingHost(range.convertTo0to1(newTempoFloat)); }

    int getPeriodInt() const { return *parameters.getRawParameterValue("period"); }
    void setPeriodInt(int newPeriodInt) { auto* param = parameters.getParameter("period"); auto range = param->getNormalisableRange(); param->setValueNotifyingHost(range.convertTo0to1(newPeriodInt)); }

    int getDensityInt() const { return *parameters.getRawParameterValue("density"); }
    void setDensityInt(int newDensityInt) { auto* param = parameters.getParameter("density"); auto range = param->getNormalisableRange(); param->setValueNotifyingHost(range.convertTo0to1(newDensityInt)); }

    int getKeyInt() const { return *parameters.getRawParameterValue("key"); }
    void setKeyInt(int newKeyInt) { auto* param = parameters.getParameter("key"); auto range = param->getNormalisableRange(); param->setValueNotifyingHost(range.convertTo0to1(newKeyInt)); }

    int getScaleInt() const { return *parameters.getRawParameterValue("scale"); }
    void setScaleInt(int newScaleInt) { auto* param = parameters.getParameter("scale"); auto range = param->getNormalisableRange(); param->setValueNotifyingHost(range.convertTo0to1(newScaleInt)); }

    int getOctaveInt() const { return *parameters.getRawParameterValue("octave"); }
    void setOctaveInt(int newOctaveInt) { auto* param = parameters.getParameter("octave"); auto range = param->getNormalisableRange(); param->setValueNotifyingHost(range.convertTo0to1(newOctaveInt)); }

    float getDetuneFloat() const { return *parameters.getRawParameterValue("detune"); }
    void setDetuneFloat(float newDetuneFloat) { auto* param = parameters.getParameter("detune"); auto range = param->getNormalisableRange(); param->setValueNotifyingHost(range.convertTo0to1(newDetuneFloat)); }

    float getDefaultBpmFromHost()
    {
        // Default value in case we can't get BPM from host
        float defaultBpm = 120.0f;
        if (auto* playHead = getPlayHead())
        {
            if (auto position = playHead->getPosition())
            {
                if (position->getBpm().hasValue())
                    return static_cast<float>(*position->getBpm());
            }
        }
        return defaultBpm;
    }
    void synchronizeBpm()
    {
        float hostBpm = getDefaultBpmFromHost();
        if (firstSync)
        {
            if (!stateLoaded)
            {
                // No saved state: initialize BPM to DAW's BPM
                if (hostBpm > 0)
                {
                    setTempoFloat(hostBpm);
                    oldHostBpm = hostBpm;
                }
                stateLoaded = true; // Prevent repeated initialization
            }
            else
            {
                // Saved state exists: align oldHostBpm to current hostBpm without changing BPM
                oldHostBpm = hostBpm;
            }
            firstSync = false;
        }
        else
        {
            // Subsequent calls: update only if DAW BPM changes
            if (hostBpm != oldHostBpm && hostBpm > 0)
            {
                setTempoFloat(hostBpm);
                oldHostBpm = hostBpm;
            }
        }
    }



    juce::AudioBuffer<float> uiWaveform;
    int uiInputNote = -1;
    int uiOutputNote = -1;

    juce::AudioProcessorValueTreeState parameters;

private:

    // Timing utilities

    bool stateLoaded = false;
    float oldHostBpm = 140.0f;
    bool firstSync = true;
    float bpm = 140.0f;  // high tempos been crashy
    float speed = 1.00;
    int cycleLength = 2 ; // was 32
    int sPs = 0;
    int sampleDrift = 0;
    bool isFirstCycle = true;
    bool triggerCycle = false;
    int phaseCounter = 0;
    uint32_t symbolExecuted = 0;
    uint32_t playbackSymbolExecuted = 0;
    uint32_t fractionalSymbolExecuted = 0;
    inline void setExecuted(uint32_t& mask, int step) { jassert(step >= 0 && step < 32); mask |= (1u << step); }
    inline void clearExecuted(uint32_t& mask, int step) { jassert(step >= 0 && step < 32); mask &= ~(1u << step); }
    inline void resetAllExecuted(uint32_t& mask) { mask = 0; }
    inline bool isExecuted(uint32_t& mask, int step) const { jassert(step >= 0 && step < 32); return (mask & (1u << step)) != 0; }
    inline bool allExecuted(uint32_t& mask) const { return mask == (1u << cycleLength) - 1u; }

    void isolateBestNote();
    void resetTiming();

    // Pitch detection utilities
    dywapitchtracker pitchTracker;
    juce::AudioBuffer<float> analysisBuffer{ 1, 1024 };
    int pitchDetectorFillPos = 0;
    std::vector<float> detectedFrequencies;
    std::vector<int> detectedNoteNumbers;
    inline int frequencyToMidiNote(float frequency)
    {
        if (frequency <= 0.0f)
        {
            return -1;
        }
        return static_cast<int>(std::round(12.0f * std::log2(frequency / 440.0f) + 69.0f));
    }

    // Audio recording utilities
    juce::AudioBuffer<float> inputAudioBuffer;
    std::atomic<int> inputAudioBuffer_samplesToRecord{ 0 };
    std::atomic<int> inputAudioBuffer_writePos{ 0 };

    // Melody capture utilities
    std::vector<int> capturedMelody = std::vector<int>(32, -1);

    // Melody generation utilities
    void generateMelody();
    std::vector<int> generatedMelody = std::vector<int>(32, -2);
//    std::vector<int> generatedMelody{60, 62, 64, 65, 67, 69, 71, 72, -2, -2, -2, -2, 72, -2, 71, -2, 69, 69, 67, -2, 67, -2, 60, -2, 59, -2, 59, -2, 59, -2, 59, -2 };
    std::vector<int> lastGeneratedMelody = std::vector<int>(32, -1);
    int detectedKey = 0;
    
    inline juce::AudioBuffer<float> pitchShift(const juce::AudioBuffer<float>& input, int baseNote, float interval)
    {
        if (input.getNumSamples() == 0 || baseNote < 0)
        {
            return juce::AudioBuffer<float>(input.getNumChannels(), 0);
        }

        float semitoneShift = interval;

        float pitchRatio = std::pow(2.0f, semitoneShift / 12.0f);

        int numChannels = input.getNumChannels();
        int inputSamples = input.getNumSamples();
        int outputSamples = static_cast<int>(inputSamples / pitchRatio + 0.5f);

        if (outputSamples <= 0)
        {
            return juce::AudioBuffer<float>(numChannels, 0);
        }

        juce::AudioBuffer<float> output(numChannels, outputSamples);

        // Linear interpolation resampling
        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float* inputData = input.getReadPointer(ch);
            float* outputData = output.getWritePointer(ch);

            for (int i = 0; i < outputSamples; ++i)
            {
                float readPos = i * pitchRatio;
                int readIndex = static_cast<int>(readPos);
                float frac = readPos - readIndex;

                if (readIndex < inputSamples - 1)
                {
                    // Linear interpolation between samples
                    outputData[i] = inputData[readIndex] * (1.0f - frac) + inputData[readIndex + 1] * frac;
                }
                else if (readIndex < inputSamples)
                {
                    outputData[i] = inputData[readIndex];
                }
                else
                {
                    outputData[i] = 0.0f;
                }
            }
        }

        return output;
    }

    inline void bellCurve(juce::AudioBuffer<float>& input)
    {
        int numSamples = input.getNumSamples();
        if (numSamples > 0)
        {
            int fadeSamples = static_cast<int>(numSamples * 0.31f);
            fadeSamples = juce::jmin(fadeSamples, numSamples / 2);
            for (int ch = 0; ch < input.getNumChannels(); ++ch)
            {
                input.applyGainRamp(ch, 0, fadeSamples, 0.0f, 1.0f);
                input.applyGainRamp(ch, numSamples - fadeSamples, fadeSamples, 1.0f, 0.0f);
            }
        }
    }

    // Audio playback utilities - main voice and synthesis buffers
    juce::AudioBuffer<float> voiceBuffer;
    std::atomic<int> newVoiceNoteNumber{ -1 };
    std::atomic<int> voiceNoteNumber{ -1 };
    int randomOffset = 0;
    float randomPitch = 0.0f;
    juce::AudioBuffer<float> synthesisBuffer;
    std::atomic<int> synthesisBuffer_readPos{ 0 };
    int playbackNote = -1;
    bool playbackNoteActive = false;
    juce::ADSR flicker;
    juce::ADSR::Parameters flickerParams;
    std::atomic<bool> useFlicker{ false };

    // Audio playback utilities - envelope voice and synthesis buffers
    juce::AudioBuffer<float> r_voiceBuffer;
    std::atomic<int> r_newVoiceNoteNumber{ -1 };
    std::atomic<int> r_voiceNoteNumber{ -1 };
    int r_randomOffset = 0;
    float r_randomPitch = 0.0f;
    juce::AudioBuffer<float> r_synthesisBuffer;
    std::atomic<int> r_synthesisBuffer_readPos{ 0 };
    int r_playbackNote = -1;
    bool r_playbackNoteActive = false;
    juce::ADSR tailEnvelope;
    juce::ADSR::Parameters tailEnvelopeParams;
    std::atomic<bool> useTailEnvelope{ false };

    // Output utilities
    juce::dsp::DryWetMixer<float> dryWetMixer;
    juce::dsp::Limiter<float> wetLimiter;

    // random number lookup tables
    juce::Random rnd;
    std::vector<float> offsetFractions;
    std::vector<float> detuneSemitones;
    int offsetIndex = 0;
    int detuneIndex = 0;

    std::vector<float> r_offsetFractions;
    std::vector<float> r_detuneSemitones;
    int r_offsetIndex = 0;
    int r_detuneIndex = 0;

    constexpr static int tableSize = 512;

    // adsr vars for future use
    float attack = 0.0f;  // * 2 seconds before note end
    float decay = 0.0f;  // * 2 seconds before note end
    float sustain = 1.0f;  // Gain coefficient
    float release = 0.0f;  // * 2 seconds after note end



    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CounterTune_v2AudioProcessor)
};

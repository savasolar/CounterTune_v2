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

    juce::AudioProcessorValueTreeState parameters;

    juce::AudioBuffer<float> waveform;

private:

    // Timing utilities

    float bpm = 120.0f;
    float speed = 1.00;
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
    inline bool allExecuted(uint32_t& mask) const { return mask == 0xFFFFFFFFu; }
    
    inline void resetTiming()
    {
        if (!detectedFrequencies.empty() && std::all_of(detectedFrequencies.begin(), detectedFrequencies.end(), [](float f) { return f <= 0.0f; }))
        {
            triggerCycle = false;
            isFirstCycle = true;
        }

        pitchDetectorFillPos = 0;
        detectedFrequencies.clear();
        detectedNoteNumbers.clear();
        inputAudioBuffer.clear();
        inputAudioBuffer_writePos.store(0);
        phaseCounter = 0;
        std::fill(capturedMelody.begin(), capturedMelody.end(), -1);

        float currentBpm = bpm;
        float currentSpeed = speed;
        sPs = static_cast<int>(std::round(60.0 / currentBpm * getSampleRate() / 4.0 * 1.0 / speed));

        int requiredSize = 32 * sPs + 4096;
        inputAudioBuffer.setSize(2, requiredSize, false, true);
        inputAudioBuffer_samplesToRecord.store(requiredSize);

        adsrParams.attack = 0.0f;
        adsrParams.decay = 0.0f;
        adsrParams.sustain = 1.0f;
        adsrParams.release = static_cast<float>(sPs) / static_cast<float>(getSampleRate());
        adsr.setParameters(adsrParams);
    }

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
//    std::vector<int> formatMelody(const std::vector<int>& melody, bool isGeneratedMelody) const;

    // Melody generation utilities
    std::vector<int> generatedMelody{60, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2 , -2, -2, -2, -2 , -2, -2, -2, -2 , -2, -2, -2, -2 , -2, -2, -2, -2 , -2, -2, -2, -2 };
//    std::vector<int> generatedMelody = std::vector<int>(32, -1);
    std::vector<int> lastGeneratedMelody = std::vector<int>(32, -1);
//    void detectKey(const std::vector<int>& melody);
    int detectedKey = 0;
//    void produceMelody(const std::vector<int>& melody, int key, int notes, int chaos);
//    void magnetize(std::vector<int>& melody, float probability) const;

    // Voice buffer creation utilities
//    juce::AudioBuffer<float> isolateBestNote();
    
//    inline juce::AudioBuffer<float> pitchShiftByResampling(const juce::AudioBuffer<float>& input, int baseNote, int targetNote)
    inline juce::AudioBuffer<float> pitchShiftByResampling(const juce::AudioBuffer<float>& input, int baseNote, float interval)
    {
        if (input.getNumSamples() == 0 || baseNote < 0)
        {
            return juce::AudioBuffer<float>(input.getNumChannels(), 0);
        }

        // Calculate pitch ratio (semitones to frequency ratio)

//        float semitoneShift = static_cast<float>(targetNote - baseNote)/* + getDetuneFloat()*/;

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
    inline void textureSynthesis(juce::AudioBuffer<float>& input, int lengthInSamples)
    {
        // tile the input buffer using custom timing and pitch randomization

        // apply fade-in and fade-out

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

        
        juce::AudioBuffer<float> textureBuffer;
        int numTiles = 5;
        textureBuffer.setSize(input.getNumChannels(), /*lengthInSamples*/numTiles * input.getNumSamples(), false, true, true);

        // pitch randomization: +/- 1-10 cents
        // offset randomization: 8-16%

        //// push input buffers side by side in textureBuffer
        //for (int tile = 0; tile < numTiles; ++tile)
        //{
        //    int offset = tile * numSamples;
        //    for (int ch = 0; ch < input.getNumChannels(); ++ch)
        //    {
        //        textureBuffer.copyFrom(ch, offset, input, ch, 0, numSamples);
        //    }
        //}


        // push input buffers side by side in textureBuffer with overlap and crossfade
        for (int tile = 0; tile < numTiles; ++tile)
        {
            int overlapSamples = input.getNumSamples() - static_cast<int>(input.getNumSamples() * 0.16);
            int offset = tile * (numSamples - overlapSamples);

            for (int ch = 0; ch < input.getNumChannels(); ++ch)
            {
                if (tile == 0)
                {
                    // First tile: copy entirely
                    textureBuffer.copyFrom(ch, offset, input, ch, 0, numSamples);
                }
                else
                {
                    // Subsequent tiles: copy with crossfade in overlap region
                    const float* inputData = input.getReadPointer(ch);
                    float* textureData = textureBuffer.getWritePointer(ch);

                    // Crossfade in the overlap region
                    for (int i = 0; i < overlapSamples; ++i)
                    {
                        float fadeOut = 1.0f - (float)i / (float)overlapSamples; // Previous tile fades out
                        float fadeIn = (float)i / (float)overlapSamples;          // New tile fades in

                        int texturePos = offset + i;
                        textureData[texturePos] = textureData[texturePos] * fadeOut + inputData[i] * fadeIn;
                    }

                    // Copy the rest of the tile (non-overlapping part)
                    textureBuffer.copyFrom(ch, offset + overlapSamples, input, ch, overlapSamples, numSamples - overlapSamples);
                }
            }
        }



        

        input = textureBuffer;

        waveform = input;


        return;
    }
    


    // Audio playback utilities
    juce::AudioBuffer<float> voiceBuffer;
    std::atomic<int> newVoiceNoteNumber{ -1 };
    std::atomic<int> voiceNoteNumber{ -1 };
    juce::AudioBuffer<float> synthesisBuffer;
    std::atomic<int> synthesisBuffer_readPos{ 0 };
    int playbackNote = -1;
    bool playbackNoteActive = false;
    juce::dsp::DryWetMixer<float> dryWetMixer;
    juce::ADSR adsr;
    juce::ADSR::Parameters adsrParams;
    std::atomic<bool> useADSR{ false };



    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CounterTune_v2AudioProcessor)
};

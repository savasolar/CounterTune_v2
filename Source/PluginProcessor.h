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
private:

    // Timing utilities

    float bpm = 120.0f;
    float speed = 1.00;
    int sPs = 0;
    int sampleDrift = 0;
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
    }

    // Pitch detection utilities
    dywapitchtracker pitchTracker;
    juce::AudioBuffer<float> analysisBuffer{ 1, 1024 };
    int pitchDetectorFillPos = 0;
    std::vector<float> detectedFrequencies;
    std::vector<int> detectedNoteNumbers;
    int frequencyToMidiNote(float frequency);

    // Audio recording utilities
    juce::AudioBuffer<float> inputAudioBuffer;
    std::atomic<int> inputAudioBuffer_samplesToRecord{ 0 };
    std::atomic<int> inputAudioBuffer_writePos{ 0 };

    // Melody capture utilities
    std::vector<int> capturedMelody = std::vector<int>(32, -1);
    std::vector<int> formatMelody(const std::vector<int>& melody, bool isGeneratedMelody) const;

    // Melody generation utilities
    std::vector<int> generatedMelody = std::vector<int>(32, -1);
    std::vector<int> lastGeneratedMelody = std::vector<int>(32, -1);
    void detectKey(const std::vector<int>& melody);
    int detectedKey = 0;
    void produceMelody(const std::vector<int>& melody, int key, int notes, int chaos);
    void magnetize(std::vector<int>& melody, float probability) const;

    // Voice buffer creation utilities
    juce::AudioBuffer<float> isolateBestNote();
    void timeStretch(juce::AudioBuffer<float> inputAudio, float lengthSeconds);
    juce::AudioBuffer<float> pitchShiftByResampling(const juce::AudioBuffer<float>& input, int baseNote, int targetNote);


    // Audio playback utilities
    juce::AudioBuffer<float> voiceBuffer;
    std::atomic<int> newVoiceNoteNumber{ -1 };
    std::atomic<int> voiceNoteNumber{ -1 };
    juce::AudioBuffer<float> finalVoiceBuffer;
    std::atomic<int> finalVoiceBuffer_readPos{ 0 };
    int playbackNote = -1;
    bool playbackNoteActive = false;
    juce::dsp::DryWetMixer<float> dryWetMixer;
    juce::ADSR adsr;
    juce::ADSR::Parameters adsrParams;
    std::atomic<bool> useADSR{ false };



    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CounterTune_v2AudioProcessor)
};

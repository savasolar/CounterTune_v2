/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "dywapitchtrack.h"

//==============================================================================
/**
*/
class CounterTune_v2AudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    CounterTune_v2AudioProcessor();
    ~CounterTune_v2AudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
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
//    int duotrigesimaleCounter = 0;
//    int duotrigesimale = 0;
    int phaseCounter = 0;
    uint32_t symbolExecuted = 0;
    uint32_t playbackSymbolExecuted = 0;
    uint32_t fractionalSymbolExecuted = 0;
    inline void setExecuted(int step)
    {
        jassert(step >= 0 && step < 32);
        symbolExecuted |= (1u << step);
        DBG("setExecuted(" << step << ") -> symbolExecuted = 0x" << juce::String::formatted("%08X", symbolExecuted));
    }
    inline void clearExecuted(int step)
    {
        jassert(step >= 0 && step < 32);
        symbolExecuted &= ~(1u << step);
        DBG("clearExecuted(" << step << ") -> symbolExecuted = 0x" << juce::String::formatted("%08X", symbolExecuted));
    }
    inline void resetAllExecuted()
    {
        symbolExecuted = 0;
    }
    inline bool isExecuted(int step) const
    {
        jassert(step >= 0 && step < 32);
        return (symbolExecuted & (1u << step)) != 0;
    }
    // Optional: check if all steps executed this cycle
    inline bool allExecuted() const
    {
//        return symbolExecuted == 0xFFFFFFFFu;
        bool result = symbolExecuted == 0xFFFFFFFFu;
        DBG("allExecuted = " << (result ? "true" : "false") << " (symbolExecuted = 0x" << juce::String::formatted("%08X", symbolExecuted) << ")");
        return result;
    }
    inline void resetTiming()
    {
        pitchDetectorFillPos = 0;

        float currentBpm = bpm;
        float currentSpeed = speed;
        sPs = static_cast<int>(std::round(60.0 / currentBpm * getSampleRate() / 4.0 * 1.0 / speed));

        resetAllExecuted();
    }


    // Pitch detection utilities
    dywapitchtracker pitchTracker;
    juce::AudioBuffer<float> analysisBuffer{ 1, 1024 };
    int pitchDetectorFillPos = 0;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CounterTune_v2AudioProcessor)
};

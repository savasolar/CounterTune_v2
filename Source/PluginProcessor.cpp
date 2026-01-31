#include "PluginProcessor.h"
#include "PluginEditor.h"

CounterTune_v2AudioProcessor::CounterTune_v2AudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{

}

CounterTune_v2AudioProcessor::~CounterTune_v2AudioProcessor()
{

}

const juce::String CounterTune_v2AudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool CounterTune_v2AudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool CounterTune_v2AudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool CounterTune_v2AudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double CounterTune_v2AudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int CounterTune_v2AudioProcessor::getNumPrograms()
{
    return 1;
}

int CounterTune_v2AudioProcessor::getCurrentProgram()
{
    return 0;
}

void CounterTune_v2AudioProcessor::setCurrentProgram (int index)
{
}

const juce::String CounterTune_v2AudioProcessor::getProgramName (int index)
{
    return {};
}

void CounterTune_v2AudioProcessor::changeProgramName (int index, const juce::String& newName)
{

}

void CounterTune_v2AudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{

}

void CounterTune_v2AudioProcessor::releaseResources()
{

}

#ifndef JucePlugin_PreferredChannelConfigurations
bool CounterTune_v2AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else

    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void CounterTune_v2AudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // free demo condition check

    // synchronize bpm from DAW

    // detect if there is "musical" input audio
        // based on pitch detector readout

    // record input audio buffer

    // record pitches to 32 midi note numbers
        // once full, call generateMelody and generate synthetic voice

    // generate synthetic voice
        // isolate best note
            // preprocess, trim
            // use custom overlap-add method to elongate
            // post-process, add fade-in and fade-out ramps (spread across 500 samples each, ~15ms)

    // synthesize generated sequences
        // stop/start and apply pitch stretching to synthetic voice buffer
        // add post-processing like compression


}

bool CounterTune_v2AudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* CounterTune_v2AudioProcessor::createEditor()
{
    return new CounterTune_v2AudioProcessorEditor (*this);
}

void CounterTune_v2AudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void CounterTune_v2AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CounterTune_v2AudioProcessor();
}

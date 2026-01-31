/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
CounterTune_v2AudioProcessor::CounterTune_v2AudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
        parameters(*this, nullptr, "Parameters",
        {
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"mix", 1}, "Mix", 0.0f, 1.0f, 0.5f)
        })
#endif
{
    dywapitch_inittracking(&pitchTracker);

    DBG("constructor loaded");
}

CounterTune_v2AudioProcessor::~CounterTune_v2AudioProcessor()
{
}

//==============================================================================
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
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
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

//==============================================================================
void CounterTune_v2AudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    analysisBuffer.setSize(1, 1024, true);
    pitchDetectorFillPos = 0;
}

void CounterTune_v2AudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool CounterTune_v2AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
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
    
    int numSamples = buffer.getNumSamples();

    // Mix current block to mono for pitch detection
    juce::AudioBuffer<float> monoBlock(1, numSamples);
    monoBlock.clear();
    int numChannels = juce::jmin(getTotalNumInputChannels(), buffer.getNumChannels());
    for (int ch = 0; ch < numChannels; ++ch)
    {
        monoBlock.addFrom(0, 0, buffer, ch, 0, numSamples);
    }
    if (numChannels > 0) monoBlock.applyGain(1.0f / numChannels);
    auto* monoData = monoBlock.getReadPointer(0);

    // Accumulate for pitch detection
    int analysisSpaceLeft = analysisBuffer.getNumSamples() - pitchDetectorFillPos;
    int analysisToCopy = juce::jmin(analysisSpaceLeft, numSamples);
    analysisBuffer.copyFrom(0, pitchDetectorFillPos, monoData, analysisToCopy);
    pitchDetectorFillPos += analysisToCopy;

    // If full, detect pitch and store MIDI note
    if (pitchDetectorFillPos >= analysisBuffer.getNumSamples())
    {
        // DYWAPitchTrack uses double*, but analysisBuffer is float*. Convert temporarily.
        std::vector<double> doubleSamples(1024);
        for (int i = 0; i < 1024; ++i)
            doubleSamples[i] = analysisBuffer.getSample(0, i);

        // Compute pitch (returns Hz, or 0.0 if no pitch detected).
        double pitch = dywapitch_computepitch(&pitchTracker, doubleSamples.data(), 0, 1024);
        pitch *= (getSampleRate() / 44100.0); // Scale for DYWAPitchTrack's 44100 assumption

//        int midiNote = frequencyToMidiNote(static_cast<float>(pitch));

  //      detectedNoteNumbers.push_back(midiNote);

        DBG(pitch);

        pitchDetectorFillPos = 0;
    }

    // Handle overflow
    if (analysisToCopy < numSamples)
    {
        analysisBuffer.copyFrom(0, 0, monoData + analysisToCopy, numSamples - analysisToCopy);
        pitchDetectorFillPos = numSamples - analysisToCopy;
    }

}

//==============================================================================
bool CounterTune_v2AudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* CounterTune_v2AudioProcessor::createEditor()
{
    return new CounterTune_v2AudioProcessorEditor (*this);
}

//==============================================================================
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

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CounterTune_v2AudioProcessor();
}

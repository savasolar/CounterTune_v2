// PluginProcessor.cpp

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
                       ),
        parameters(*this, nullptr, "Parameters",
        {
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"mix", 1}, "Mix", 0.0f, 1.0f, 0.5f)
        })
#endif
{
    dywapitch_inittracking(&pitchTracker);

    DBG("check 123");
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
    analysisBuffer.setSize(1, 1024, true);

    resetTiming();
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
    
    // shared vars

    int numSamples = buffer.getNumSamples();

        

    // Get good pitch readouts

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

//        DBG(pitch);


        


        if (pitch != 0)
            triggerCycle = true;

        if (triggerCycle)
            detectedFrequencies.push_back(static_cast<float>(pitch));




        


        pitchDetectorFillPos = 0;
    }

    // Handle overflow
    if (analysisToCopy < numSamples)
    {
        analysisBuffer.copyFrom(0, 0, monoData + analysisToCopy, numSamples - analysisToCopy);
        pitchDetectorFillPos = numSamples - analysisToCopy;
    }


    // count stuff

    if (triggerCycle)
    {
        // High-resolution counter for recording input audio buffer
        int spaceLeft = inputAudioBuffer_samplesToRecord.load() - inputAudioBuffer_writePos.load();
        int toCopy = juce::jmin(numSamples, spaceLeft);

        for (int ch = 0; ch < juce::jmin(getTotalNumInputChannels(), inputAudioBuffer.getNumChannels()); ++ch)
        {
            inputAudioBuffer.copyFrom(ch, inputAudioBuffer_writePos.load(), buffer, ch, 0, toCopy);
        }

        inputAudioBuffer_writePos.store(inputAudioBuffer_writePos.load() + toCopy);


        // Low-resolution counter for symbolically transcribing input audio
        int captureSpaceLeft = (sPs * 32 + std::max(sampleDrift, 0)) - phaseCounter;
        int captureToCopy = juce::jmin(captureSpaceLeft, numSamples);

        for (int n = 0; n < 32; ++n)
        {
            if (phaseCounter > (n + 0.5) * sPs)
            {
                if (!isExecuted(symbolExecuted, n))
                {
                    //DBG(n);

                    sampleDrift = static_cast<int>(std::round(32.0 * (60.0 / bpm * getSampleRate() / 4.0 * 1.0 / speed - sPs)));
                    setExecuted(symbolExecuted, n);
                }
            }
        }

        // Low-resolution counter for reading generated transcriptions
        for (int n = 0; n < 32; ++n)
        {
            if (phaseCounter >= n * sPs)
            {
                if (!isExecuted(playbackSymbolExecuted, n))
                {
                    //DBG(n + 100);

                    setExecuted(playbackSymbolExecuted, n);
                }
            }
        }

        phaseCounter += captureToCopy;

        // Low-resolution counter for stopping or resetting timing

        if (phaseCounter >= sPs * 32 + sampleDrift)
        {
            DBG("cycle end");

            resetAllExecuted(symbolExecuted);
            resetAllExecuted(playbackSymbolExecuted);
            resetAllExecuted(fractionalSymbolExecuted);


            // DBG print:
            // - samples in inputAudioBuffer ----can't too long
            // - all indices in detectedFrequencies
            
            juce::StringArray freqArray;
            freqArray.ensureStorageAllocated(static_cast<int>(detectedFrequencies.size()));

            for (float freq : detectedFrequencies)
            {
                if (freq <= 0.0f)
                    freqArray.add("0");                     // or "no-pitch" if you prefer text
                else
                    freqArray.add(juce::String(freq, 3));   // 3 decimal places — plenty for musical Hz values
            }

            DBG("Detected Frequencies (" + juce::String(detectedFrequencies.size()) + " values): "
                + freqArray.joinIntoString(", "));
            
            // - all indices in detectedNoteNumbers
            // - all indices in capturedMelody



            resetTiming();
        }

        // High-resolution counter for synthesizing generated transcriptions
        // ...

    }



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
    
}

void CounterTune_v2AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CounterTune_v2AudioProcessor();
}

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
    DBG("prepareToPlay called");

    analysisBuffer.setSize(1, 1024, true);

    dryWetMixer.prepare(juce::dsp::ProcessSpec{ sampleRate, static_cast<std::uint32_t> (samplesPerBlock), static_cast<std::uint32_t> (getTotalNumOutputChannels()) });
    dryWetMixer.setMixingRule(juce::dsp::DryWetMixingRule::balanced);

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

//        DBG(pitch);



        if (pitch != 0)
        {
            triggerCycle = true;
        }


        if (triggerCycle)
        {
            detectedFrequencies.push_back(static_cast<float>(pitch));
            int midiNote = frequencyToMidiNote(static_cast<float>(pitch));
            detectedNoteNumbers.push_back(midiNote);
        }






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
                    useADSR.store(false);

                    // prepare a note for playback if there's a note number
                    if (generatedMelody[n] >= 0)
                    {
                        int shiftedNote = generatedMelody[n]/* + getOctaveInt() * 12*/;
                        synthesisBuffer = pitchShiftByResampling(voiceBuffer, voiceNoteNumber.load(), shiftedNote);
                        synthesisBuffer_readPos.store(0);
                    }

                    // if next generatedMelody symbol indicates a fadeout in the current symbol is needed, activate useADSR here

                    if ((generatedMelody[(n + 1) % 32]) >= -1)
                    {
                        useADSR.store(true);
                    }

                    if (useADSR.load())
                    {
                        adsr.reset();
                        adsr.noteOn();
                        adsr.noteOff();
                    }

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

            // dump first-chunk duplicate in first cycle after trigger
            if (isFirstCycle)
            {
                if (!detectedFrequencies.empty()) { detectedFrequencies.erase(detectedFrequencies.begin()); }
                if (!detectedNoteNumbers.empty()) { detectedNoteNumbers.erase(detectedNoteNumbers.begin()); }
                isFirstCycle = false;
            }

            // isolate best chunks in detectedNoteNumbers
            
            // dbg print: detectedNoteNumbers
            juce::StringArray nnArray;
            nnArray.ensureStorageAllocated(static_cast<int>(detectedNoteNumbers.size()));
            for (float note : detectedNoteNumbers) { nnArray.add(juce::String(note)); }
            DBG("Detected Notes (" + juce::String(detectedNoteNumbers.size()) + " values): " + nnArray.joinIntoString(", "));

            // Find the first occurrence of over 5 consecutive note numbers in detectedNoteNumbers, put it in lowResIsolatedChunks, and print lowResIsolatedChunks.
            std::vector<int> lowResIsolatedChunks;
            int lowResChunkFirstIdx = -1;
            int lowResChunkLastIdx = -1;

            if (detectedNoteNumbers.size() > 5) {
                size_t currentStart = 0;
                for (size_t i = 1; i <= detectedNoteNumbers.size(); ++i) {
                    if (i == detectedNoteNumbers.size() || detectedNoteNumbers[i] != detectedNoteNumbers[currentStart]) {
                        size_t length = i - currentStart;
                        if (length > 5) {
                            lowResIsolatedChunks.assign(detectedNoteNumbers.begin() + currentStart, detectedNoteNumbers.begin() + i);
                            lowResChunkFirstIdx = static_cast<int>(currentStart);
                            lowResChunkLastIdx = static_cast<int>(i - 1);
                            break;
                        }
                        currentStart = i;
                    }
                }
            }

            DBG("Low-res best chunks:");
            juce::StringArray lrIsoChkArray;
            lrIsoChkArray.ensureStorageAllocated(static_cast<int>(lowResIsolatedChunks.size()));
            for (float note : lowResIsolatedChunks) { lrIsoChkArray.add(juce::String(note)); }
            DBG(lrIsoChkArray.joinIntoString(", "));

            DBG("First and last low-res indices:");
            DBG(juce::String(lowResChunkFirstIdx) + ", " + juce::String(lowResChunkLastIdx));

            // place the first 5 indices of lowResIsolatedChunks into midResIsolatedChunks
            std::vector<int> midResIsolatedChunks;
            int midResChunkFirstIdx = lowResChunkFirstIdx;
            int midResChunkLastIdx = lowResChunkFirstIdx + 4;

            if (lowResIsolatedChunks.size() >= 5)
            {
                midResIsolatedChunks.assign(lowResIsolatedChunks.begin(), lowResIsolatedChunks.begin() + 5);
            }

            DBG("Mid-res best chunks:");
            juce::StringArray mrIsoChkArray;
            mrIsoChkArray.ensureStorageAllocated(static_cast<int>(midResIsolatedChunks.size()));
            for (float note : midResIsolatedChunks) { mrIsoChkArray.add(juce::String(note)); }
            DBG(mrIsoChkArray.joinIntoString(", "));

            DBG("First and last mid-res indices:");
            DBG(juce::String(midResChunkFirstIdx) + ", " + juce::String(midResChunkLastIdx));

            // populate hiResIsolatedChunks with the middle 3 indices of midResIsolatedChunks
            std::vector<int> hiResIsolatedChunks;
            int hiResChunkFirstIdx = midResChunkFirstIdx + 1;
            int hiResChunkLastIdx = midResChunkLastIdx - 1;

            hiResIsolatedChunks.assign(midResIsolatedChunks.begin() + 1, midResIsolatedChunks.begin() + 4);

            DBG("Hi-res best chunks:");
            juce::StringArray hrIsoChkArray;
            hrIsoChkArray.ensureStorageAllocated(static_cast<int>(hiResIsolatedChunks.size()));
            for (float note : hiResIsolatedChunks) { hrIsoChkArray.add(juce::String(note)); }
            DBG(hrIsoChkArray.joinIntoString(", "));

            DBG("First and last hi-res indices:");
            DBG(juce::String(hiResChunkFirstIdx) + ", " + juce::String(hiResChunkLastIdx));

            // DBG number of samples in inputAudioBuffer
            DBG("Audio buffer size:" + juce::String(inputAudioBuffer.getNumSamples()));

            // DBG number of samples in low-resolution isolated audio
            int lowResSampleFirstIdx = lowResChunkFirstIdx * 1024;
            int lowResSampleLastIdx = lowResChunkLastIdx * 1024;
            int lowResNumSamples = lowResSampleLastIdx - lowResSampleFirstIdx + 1024;
            DBG("Low res best samples amount: " + juce::String(lowResNumSamples));

            // DBG low-res first and last sample indices
            DBG("Low-res first and last sample indices: " + juce::String(lowResSampleFirstIdx) + ", " + juce::String(lowResSampleLastIdx));

            // DBG number of samples in mid-resolution isolated audio
            int midResSampleFirstIdx = midResChunkFirstIdx * 1024;
            int midResSampleLastIdx = midResChunkLastIdx * 1024;
            int midResNumSamples = midResSampleLastIdx - midResSampleFirstIdx + 1024;
            DBG("Mid res best samples amount: " + juce::String(midResNumSamples));

            // DBG mid-res first and last sample indices
            DBG("Mid-res first and last sample indices: " + juce::String(midResSampleFirstIdx) + ", " + juce::String(midResSampleLastIdx));

            // DBG number of samples in hi-resolution isolated audio
            int hiResSampleFirstIdx = hiResChunkFirstIdx * 1024;
            int hiResSampleLastIdx = hiResChunkLastIdx * 1024;
            int hiResNumSamples = hiResSampleLastIdx - hiResSampleFirstIdx + 1024;
            DBG("Hi res best samples amount: " + juce::String(hiResNumSamples));

            // DBG hi-res first and last sample indices
            DBG("Hi-res first and last sample indices: " + juce::String(hiResSampleFirstIdx) + ", " + juce::String(hiResSampleLastIdx));

            // This might hide the best chunks so I'll give it more consideration

            

            // Generate voiceBuffer

            // To create voiceBuffer, go to hi-res first idx of inputAudioBuffer and accumulate hiResNumSamples, and copy it to voiceBuffer



            voiceBuffer.setSize(inputAudioBuffer.getNumChannels(), hiResNumSamples, false, true, true);
            for (int ch = 0; ch < inputAudioBuffer.getNumChannels(); ++ch)
            {
                voiceBuffer.copyFrom(ch, 0, inputAudioBuffer, ch, hiResSampleFirstIdx, hiResNumSamples);
            }
            DBG(juce::String(voiceBuffer.getNumSamples()));



            resetTiming();
        }


        // High-resolution counter for synthesizing generated transcriptions
        juce::dsp::AudioBlock<float> block(buffer);
        dryWetMixer.pushDrySamples(block);
        block.clear();

        // populate output buffer
        // ...

        if (synthesisBuffer.getNumSamples() > 0)
        {
            int numSamples = buffer.getNumSamples();
            int voiceBufferSize = synthesisBuffer.getNumSamples();
            int readPos = synthesisBuffer_readPos.load();

            for (int i = 0; i < numSamples; ++i)
            {
                int currentPos = readPos + i;

                if (currentPos >= voiceBufferSize) break;

                float gain = useADSR.load() ? adsr.getNextSample() : 1.0f;

                for (int ch = 0; ch < juce::jmin(buffer.getNumChannels(), synthesisBuffer.getNumChannels()); ++ch)
                {
                    buffer.addSample(ch, i, synthesisBuffer.getSample(ch, currentPos) * gain);
                }
            }

            synthesisBuffer_readPos.store(readPos + numSamples);
        }


        dryWetMixer.setWetMixProportion(getMixFloat());
        dryWetMixer.mixWetSamples(block);

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

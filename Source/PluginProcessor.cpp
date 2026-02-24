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
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"mix", 1}, "Mix", 0.0f, 1.0f, 0.15f),
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"tempo", 1}, "Tempo", 60, 240, 140),
            std::make_unique<juce::AudioParameterInt>(juce::ParameterID{"period", 1}, "Period", 1, 32, 2),
            std::make_unique<juce::AudioParameterInt>(juce::ParameterID{"density", 1}, "Density", 1, 6, 6),
            std::make_unique<juce::AudioParameterInt>(juce::ParameterID{"key", 1}, "Key", 0, 11, 7),
            std::make_unique<juce::AudioParameterInt>(juce::ParameterID{"scale", 1}, "Scale", 1, 4, 1),
            std::make_unique<juce::AudioParameterInt>(juce::ParameterID{"octave", 1}, "Octave", -4, 4, 0),
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"detune", 1}, "Detune", -1.0f, 1.0f, 0.0f)
        })
#endif
{
    dywapitch_inittracking(&pitchTracker);

    uiWaveform.setSize(2, 1); // dummy initial size

    synthesisBuffer.setSize(2, 1);

    r_synthesisBuffer.setSize(2, 1); // init release buffer


    offsetFractions.resize(tableSize);
    detuneSemitones.resize(tableSize);
    r_offsetFractions.resize(tableSize);
    r_detuneSemitones.resize(tableSize);
    for (int i = 0; i < tableSize; ++i)
    {
        offsetFractions[i] = (rnd.nextInt(9) + 8) * 0.01f;
        detuneSemitones[i] = (rnd.nextInt(21) * 0.01f) - 0.10f;
        r_offsetFractions[i] = offsetFractions[i];
        r_detuneSemitones[i] = detuneSemitones[i];
    }
    offsetIndex = 0;
    detuneIndex = 0;
    r_offsetIndex = 0;
    r_detuneIndex = 0;


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

    juce::dsp::ProcessSpec spec{ sampleRate, static_cast<std::uint32_t>(samplesPerBlock), static_cast<std::uint32_t>(getTotalNumOutputChannels()) };
    wetLimiter.prepare(spec);
    wetLimiter.setThreshold(-11.0f);
    wetLimiter.setRelease(5.0f);

    flicker.setSampleRate(sampleRate);

    tailEnvelope.setSampleRate(sampleRate); // set for tail

    resetTiming();
    generateMelody();
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

void CounterTune_v2AudioProcessor::isolateBestNote()
{
    // Find the first occurrence of over 5 consecutive note numbers in detectedNoteNumbers & put it in lowResIsolatedChunks.
    std::vector<int> lowResIsolatedChunks;
    int lowResChunkFirstIdx = -1;
    int lowResChunkLastIdx = -1;

    if (detectedNoteNumbers.size() > 5)
    {
        size_t currentStart = 0;
        for (size_t i = 1; i <= detectedNoteNumbers.size(); ++i)
        {
            if (i == detectedNoteNumbers.size() || detectedNoteNumbers[i] != detectedNoteNumbers[currentStart])
            {
                size_t length = i - currentStart;
                if (length > 5)
                {
                    lowResIsolatedChunks.assign(detectedNoteNumbers.begin() + currentStart, detectedNoteNumbers.begin() + i);
                    lowResChunkFirstIdx = static_cast<int>(currentStart);
                    lowResChunkLastIdx = static_cast<int>(i - 1);
                    break;
                }
                currentStart = i;
            }
        }
    }


    // BAIL OUT early if we didn't find a valid chunk - keep existing voiceBuffer
    if (lowResIsolatedChunks.size() < 5 || lowResChunkFirstIdx < 0)
    {
        DBG("isolateBestNote: not enough data, keeping last valid voiceBuffer");
        return;
    }


    // place the first 5 indices of lowResIsolatedChunks into midResIsolatedChunks
    std::vector<int> midResIsolatedChunks;
    int midResChunkFirstIdx = lowResChunkFirstIdx;
    int midResChunkLastIdx = lowResChunkFirstIdx + 4;

    if (lowResIsolatedChunks.size() >= 5)
    {
        midResIsolatedChunks.assign(lowResIsolatedChunks.begin(), lowResIsolatedChunks.begin() + 5);
    }

    // populate hiResIsolatedChunks with the middle 3 indices of midResIsolatedChunks
    std::vector<int> hiResIsolatedChunks;
    int hiResChunkFirstIdx = midResChunkFirstIdx + 1;
    int hiResChunkLastIdx = midResChunkLastIdx - 1;
    hiResIsolatedChunks.assign(midResIsolatedChunks.begin() + 1, midResIsolatedChunks.begin() + 4);

    int lowResSampleFirstIdx = lowResChunkFirstIdx * 1024;
    int lowResSampleLastIdx = lowResChunkLastIdx * 1024;
    int lowResNumSamples = lowResSampleLastIdx - lowResSampleFirstIdx + 1024;
    int midResSampleFirstIdx = midResChunkFirstIdx * 1024;
    int midResSampleLastIdx = midResChunkLastIdx * 1024;
    int midResNumSamples = midResSampleLastIdx - midResSampleFirstIdx + 1024;
    int hiResSampleFirstIdx = hiResChunkFirstIdx * 1024;
    int hiResSampleLastIdx = hiResChunkLastIdx * 1024;
    int hiResNumSamples = hiResSampleLastIdx - hiResSampleFirstIdx + 1024;

    newVoiceNoteNumber.store(hiResIsolatedChunks[1]);
    voiceNoteNumber.store(newVoiceNoteNumber);

    // Generate voiceBuffer (single tile)
    voiceBuffer.setSize(inputAudioBuffer.getNumChannels(), hiResNumSamples, false, true, true);
    for (int ch = 0; ch < inputAudioBuffer.getNumChannels(); ++ch)
    {
        voiceBuffer.copyFrom(ch, 0, inputAudioBuffer, ch, hiResSampleFirstIdx, hiResNumSamples);
        bellCurve(voiceBuffer);

        uiWaveform = voiceBuffer;
        // stylize waveform here
        // Normalize the waveform to peak at 1.0
        float peak = 0.0f;
        for (int ch = 0; ch < uiWaveform.getNumChannels(); ++ch)
        {
            peak = std::max(peak, uiWaveform.getMagnitude(ch, 0, uiWaveform.getNumSamples()));
        }

        if (peak > 0.0f)
        {
            float gain = 1.0f / peak;
            uiWaveform.applyGain(gain);
        }
    }
}

void CounterTune_v2AudioProcessor::resetTiming()
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

    bpm = getTempoFloat();
    cycleLength = getPeriodInt();

    float currentBpm = bpm;
    float currentSpeed = speed;
    sPs = static_cast<int>(std::round(60.0 / currentBpm * getSampleRate() / 4.0 * 1.0 / speed));

//    int requiredSize = 32 * sPs + 4096;
    int requiredSize = cycleLength * sPs + 4096;
    inputAudioBuffer.setSize(2, requiredSize, false, true);
    inputAudioBuffer_samplesToRecord.store(requiredSize);

    flickerParams.attack = 0.0f;
    flickerParams.decay = 0.0f;
    flickerParams.sustain = 1.0f;
    flickerParams.release = static_cast<float>(sPs) / static_cast<float>(getSampleRate());
    flicker.setParameters(flickerParams);

    // duplicate for release envelope
    tailEnvelopeParams.attack = 0.0f;
    tailEnvelopeParams.decay = 0.0f;
    tailEnvelopeParams.sustain = 1.0f;
    tailEnvelopeParams.release = release;
    tailEnvelope.setParameters(tailEnvelopeParams);
}

void CounterTune_v2AudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    synchronizeBpm();

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
        // phase counting variables
//        int phaseAdvance = juce::jmin(((sPs * 32 + std::max(sampleDrift, 0)) - phaseCounter), numSamples);
        int phaseAdvance = juce::jmin(((sPs * cycleLength + std::max(sampleDrift, 0)) - phaseCounter), numSamples);

        // High-resolution counter for recording input audio buffer
        int spaceLeft = inputAudioBuffer_samplesToRecord.load() - inputAudioBuffer_writePos.load();
        int toCopy = juce::jmin(numSamples, spaceLeft);
        for (int ch = 0; ch < juce::jmin(getTotalNumInputChannels(), inputAudioBuffer.getNumChannels()); ++ch)
        {
            inputAudioBuffer.copyFrom(ch, inputAudioBuffer_writePos.load(), buffer, ch, 0, toCopy);
        }
        inputAudioBuffer_writePos.store(inputAudioBuffer_writePos.load() + toCopy);

        // Low-resolution counter for symbolically transcribing input audio
        for (int n = 0; n < cycleLength; ++n)
        {
            if (phaseCounter > (n + 0.5) * sPs)
            {
                if (!isExecuted(symbolExecuted, n))
                {
                    if (!detectedNoteNumbers.empty())
                    {
                        capturedMelody[n] = detectedNoteNumbers.back();

                        // set ui input and output notes at same time
                        uiInputNote = voiceNoteNumber.load() % 12;
                        if (detectedNoteNumbers.back() >= 0) uiInputNote = detectedNoteNumbers.back() % 12;
                        if (generatedMelody[n] >= 0) uiOutputNote = generatedMelody[n] % 12;
                    }

//                    DBG(capturedMelody[n]);


                    sampleDrift = static_cast<int>(std::round(cycleLength * (60.0 / bpm * getSampleRate() / 4.0 * 1.0 / speed - sPs)));
                    setExecuted(symbolExecuted, n);
                }
            }
        }

        // Low-res playback counter
        for (int n = 0; n < cycleLength; ++n)
        {
            if (phaseCounter >= n * sPs)
            {
                if (!isExecuted(playbackSymbolExecuted, n))
                {
                    useFlicker.store(false);

                    // DBG a running stream of generatedMelody values                    
                    DBG(juce::String(generatedMelody[n]));


                    // prepare a note for playback if there's a note number
                    if (generatedMelody[n] >= 0)
                    {
                        playbackNote = generatedMelody[n];
                        playbackNoteActive = true;


                        // prepare synthesis buffer with latest info

                        // OCTAVE SHIFT AND DETUNE KNOB
                        float octaveShift = static_cast<float>(getOctaveInt()) * 12.0f;
                        float detuneShift = getDetuneFloat();
                        float interval = static_cast<float>((playbackNote % 12) - (voiceNoteNumber.load() % 12)) + octaveShift + detuneShift;

                        synthesisBuffer = pitchShift(voiceBuffer, (voiceNoteNumber.load() % 12), interval);

//                        synthesisBuffer = pitchShift(voiceBuffer, (voiceNoteNumber.load() % 12), static_cast<float>((playbackNote % 12) - (voiceNoteNumber.load() % 12)));

                        randomOffset = static_cast<int>(synthesisBuffer.getNumSamples() * offsetFractions[offsetIndex & (tableSize - 1)]);
                        synthesisBuffer_readPos.store(0);


                    }
                    else
                    {
                        playbackNoteActive = false;
                    }

                    // if next generatedMelody symbol is a new note or noteoff event
                    if ((generatedMelody[(n + 1) % cycleLength]) >= -1)
                    {
                        if (release == 0.0f)
                        {
                            useFlicker.store(true);
                        }
                    }

                    if (useFlicker.load())
                    {
                        flicker.reset();
                        flicker.noteOn();
                        flicker.noteOff();
                    }

                    setExecuted(playbackSymbolExecuted, n);
                }
            }
        }

        phaseCounter += phaseAdvance;

        // Low-res counter for stopping or resetting timing

        if (phaseCounter >= sPs * cycleLength + sampleDrift)
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


            generateMelody();


            isolateBestNote();

            resetTiming();
        }

        // High-res playback counter
        juce::dsp::AudioBlock<float> block(buffer);
        dryWetMixer.pushDrySamples(block);
        block.clear();

        // tile synthesis to output buffer
        if (synthesisBuffer.getNumSamples() > 0)
        {
            int numSamples = buffer.getNumSamples();
            int synthesisBufferSize = synthesisBuffer.getNumSamples();
            int readPos = synthesisBuffer_readPos.load();
            int processed = 0;

            for (int i = 0; i < numSamples; ++i)
            {
                int currentPos = readPos + i;

                if (currentPos >= synthesisBufferSize) break;

                float gain = 1.0f;
                if (useFlicker.load()) gain = flicker.getNextSample();

                for (int ch = 0; ch < juce::jmin(buffer.getNumChannels(), synthesisBuffer.getNumChannels()); ++ch)
                {
                     buffer.addSample(ch, i, synthesisBuffer.getSample(ch, currentPos) * gain);
                }

                processed = i + 1;
            }

            synthesisBuffer_readPos.store(readPos + processed);

            // spawn synthesis tiles
            if (synthesisBuffer_readPos.load() >= randomOffset)
            {
                juce::AudioBuffer<float> baseTile;
                juce::AudioBuffer<float> newTile;

                int remainingSamples = synthesisBuffer.getNumSamples() - synthesisBuffer_readPos.load();
                if (remainingSamples < 0) remainingSamples = 0;

                randomPitch = detuneSemitones[detuneIndex & (tableSize - 1)];
                ++detuneIndex;

                // OCTAVE SHIFT AND DETUNE KNOB
                float octaveShift = static_cast<float>(getOctaveInt()) * 12.0f;
                float detuneShift = getDetuneFloat();
                float interval = static_cast<float>((playbackNote % 12) - (voiceNoteNumber.load() % 12)) + octaveShift + detuneShift;

                newTile = pitchShift(voiceBuffer, (voiceNoteNumber.load() % 12), interval + randomPitch);

//                newTile = pitchShift(voiceBuffer, (voiceNoteNumber.load() % 12), static_cast<float>((playbackNote % 12) - (voiceNoteNumber.load() % 12)) + randomPitch);


                // Calculate overlap and non-overlap first
                int overlapSamples = juce::jmin(remainingSamples, newTile.getNumSamples());
                int nonOverlapSamples = newTile.getNumSamples() - overlapSamples;

                // Set size correctly: remaining (to be crossfaded) + non-overlap append
                baseTile.setSize(synthesisBuffer.getNumChannels(), remainingSamples + nonOverlapSamples, false, true, true);

                // Copy remaining to first tile start
                for (int ch = 0; ch < synthesisBuffer.getNumChannels(); ++ch)
                {
                    baseTile.copyFrom(ch, 0, synthesisBuffer, ch, synthesisBuffer_readPos.load(), remainingSamples);
                }

                // Crossfade the overlap region
                for (int ch = 0; ch < baseTile.getNumChannels(); ++ch)
                {
                    float* baseData = baseTile.getWritePointer(ch);
                    const float* newData = newTile.getReadPointer(ch);

                    for (int i = 0; i < overlapSamples; ++i)
                    {
                        float fadeOut = 1.0f - static_cast<float>(i) / static_cast<float>(overlapSamples);
                        float fadeIn = static_cast<float>(i) / static_cast<float>(overlapSamples);
                        baseData[i] = baseData[i] * fadeOut + newData[i] * fadeIn;
                    }
                }

                // Append any non-overlapping part of newTile
                if (nonOverlapSamples > 0)
                {
                    for (int ch = 0; ch < baseTile.getNumChannels(); ++ch)
                    {
                        baseTile.copyFrom(ch, overlapSamples, newTile, ch, overlapSamples, nonOverlapSamples);
                    }
                }

                synthesisBuffer = std::move(baseTile);
                synthesisBuffer_readPos.store(0);




                randomOffset = static_cast<int>(synthesisBuffer.getNumSamples() * offsetFractions[offsetIndex & (tableSize - 1)]);
                ++offsetIndex;
            }
                        
        }







        // Add limiter to wet signal ...
        //juce::dsp::ProcessContextReplacing<float> limiterContext(block);
        //wetLimiter.process(limiterContext);

        const float gainBoost = juce::Decibels::decibelsToGain(9.0f);
        block.multiplyBy(gainBoost);   // +9 dB on wet only

        juce::dsp::ProcessContextReplacing<float> limiterContext(block);
        wetLimiter.process(limiterContext);



        dryWetMixer.setWetMixProportion(getMixFloat());
        dryWetMixer.mixWetSamples(block);

    }
}

void CounterTune_v2AudioProcessor::generateMelody()
{
    //// populate each index of the generatedMelody vector with a random number from 60 to 72
    //for (auto& note : generatedMelody)
    //{
    //    note = 60 + rnd.nextInt(13);  // rnd.nextInt(13) gives 0-12 - 60-72 inclusive
    //}

    int rootNote = 60 + getKeyInt();

    // Scale note offsets
    const std::vector<std::vector<int>> scaleDefs = {
        {}, // dummy index 0
        {0, 2, 4, 5, 7, 9, 11}, // 1 = major
        {0, 2, 4, 5, 7, 8, 11}, // 2 = harmonic major
        {0, 2, 4, 7, 9}, // 3 = pentatonic
        {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11} // 4 = chromatic
    };

    const auto& intervals = scaleDefs[getScaleInt()];

    std::vector<int> availableNotes;
    for (int offset : intervals)
        availableNotes.push_back(rootNote + offset);

    // rhythmic density step sizes
    const int stepSizes[7] = { 0, 32, 16, 8, 4, 2, 1 };
    int step = stepSizes[getDensityInt()];

    // fill generatedMelody
    generatedMelody.assign(32, -2);

    for (int i = 0; i < 32; i += step)
    {
        if (!availableNotes.empty())
        {
            int idx = rnd.nextInt(static_cast<int>(availableNotes.size()));
            generatedMelody[i] = availableNotes[idx];
        }
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

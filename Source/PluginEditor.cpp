// PluginEditor.cpp

#include "PluginProcessor.h"
#include "PluginEditor.h"

CounterTune_v2AudioProcessorEditor::CounterTune_v2AudioProcessorEditor (CounterTune_v2AudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (500, 490);

    addAndMakeVisible(waveform);
    waveform.setBounds(60, 0, 440, 440);
}

CounterTune_v2AudioProcessorEditor::~CounterTune_v2AudioProcessorEditor()
{
}

void CounterTune_v2AudioProcessorEditor::timerCallback()
{
//        waveform.setAudioBuffer(&audioProcessor.waveform, audioProcessor.waveform.getNumSamples());
}

void CounterTune_v2AudioProcessorEditor::paint (juce::Graphics& g)
{

}

void CounterTune_v2AudioProcessorEditor::resized()
{

}

// PluginEditor.cpp

#include "PluginProcessor.h"
#include "PluginEditor.h"

CounterTune_v2AudioProcessorEditor::CounterTune_v2AudioProcessorEditor (CounterTune_v2AudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), waveform(audioProcessor)
{
    setSize (540, 540);

    addAndMakeVisible(waveform);
    waveform.setBounds(60, 0, 480, 480);
    backgroundImage = juce::ImageCache::getFromMemory(BinaryData::uiv7_png, BinaryData::uiv7_pngSize);
    startTimerHz(30);
}

CounterTune_v2AudioProcessorEditor::~CounterTune_v2AudioProcessorEditor()
{
}

void CounterTune_v2AudioProcessorEditor::timerCallback()
{
    waveform.setAudioBuffer(&audioProcessor.waveform, audioProcessor.waveform.getNumSamples());
    waveform.repaint();

    DBG("current input, output: " + juce::String(audioProcessor.currentInputNote) + ", " + juce::String(audioProcessor.currentOutputNote));
}

void CounterTune_v2AudioProcessorEditor::paint (juce::Graphics& g)
{
    g.drawImage(backgroundImage, getLocalBounds().toFloat());
}

void CounterTune_v2AudioProcessorEditor::resized()
{

}

#include "PluginProcessor.h"
#include "PluginEditor.h"

CounterTune_v2AudioProcessorEditor::CounterTune_v2AudioProcessorEditor (CounterTune_v2AudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (400, 300);
}

CounterTune_v2AudioProcessorEditor::~CounterTune_v2AudioProcessorEditor()
{

}

void CounterTune_v2AudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (15.0f));
    g.drawFittedText ("CounterTune", getLocalBounds(), juce::Justification::centred, 1);
}

void CounterTune_v2AudioProcessorEditor::resized()
{

}

/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class CounterTune_v2AudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    CounterTune_v2AudioProcessorEditor (CounterTune_v2AudioProcessor&);
    ~CounterTune_v2AudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    CounterTune_v2AudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CounterTune_v2AudioProcessorEditor)
};

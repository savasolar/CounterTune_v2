// PluginEditor.cpp

#include "PluginProcessor.h"
#include "PluginEditor.h"

CounterTune_v2AudioProcessorEditor::CounterTune_v2AudioProcessorEditor (CounterTune_v2AudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), waveform(audioProcessor), parameters(p.parameters)
{
    setSize (720, 540);

    setWantsKeyboardFocus(true);

    addAndMakeVisible(waveform);
    waveform.setBounds(240, 0, 480, 480);
    backgroundImage = juce::ImageCache::getFromMemory(BinaryData::uiv7_png, BinaryData::uiv7_pngSize);
    setupParams();
    startTimerHz(30);
}

CounterTune_v2AudioProcessorEditor::~CounterTune_v2AudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void CounterTune_v2AudioProcessorEditor::timerCallback()
{
    if (firstLoad)
    {
        updateMixValueLabel();
        updateOctaveValueLabel();

        firstLoad = false;
    }

    waveform.setAudioBuffer(&audioProcessor.uiWaveform, audioProcessor.uiWaveform.getNumSamples());
    bool isFlat = waveform.isFlat();
    waveform.setVisible(!isFlat);
    waveform.repaint();

}

void CounterTune_v2AudioProcessorEditor::paint (juce::Graphics& g)
{
    g.drawImage(backgroundImage, getLocalBounds().toFloat());
}

void CounterTune_v2AudioProcessorEditor::resized()
{

}

void CounterTune_v2AudioProcessorEditor::setupParams()
{
    // MIX
    addAndMakeVisible(mixTitleLabel);
#ifdef JUCE_MAC
    mixTitleLabel.setBounds(579, -1, 240, 20);
    mixTitleLabel.setFont(getCustomFont(14.0f));
#else
    mixTitleLabel.setBounds(0, 0, 240, 20);
    mixTitleLabel.setFont(getCustomFont(18.0f));
#endif
    mixTitleLabel.setJustification(juce::Justification::centred);
    mixTitleLabel.setColour(juce::TextEditor::textColourId, foregroundColor);
    mixTitleLabel.setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
    mixTitleLabel.setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    mixTitleLabel.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
    mixTitleLabel.setReadOnly(true);
    mixTitleLabel.setCaretVisible(false);
    mixTitleLabel.setMouseCursor(juce::MouseCursor::NormalCursor);
    mixTitleLabel.setText("MIX", dontSendNotification);

    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(parameters, "mix", mixKnob);
    mixKnob.setSliderStyle(juce::Slider::LinearBar);
    mixKnob.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    mixKnob.setColour(juce::Slider::backgroundColourId, juce::Colours::transparentBlack);
    mixKnob.setColour(juce::Slider::trackColourId, foregroundColor);
    mixKnob.setColour(juce::Slider::thumbColourId, foregroundColor);
    mixKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    mixKnob.setBounds(0, 20, 240, 20);
    mixKnob.setRange(0.0, 1.0, 0.01);
    mixKnob.onValueChange = [this]() { updateMixValueLabel(); };
    addAndMakeVisible(mixKnob);

    addAndMakeVisible(mixValueLabel);
#ifdef JUCE_MAC
    mixValueLabel.setBounds(0, 39, 240, 16);
    mixValueLabel.setFont(getCustomFont(14.0f));
#else
    mixValueLabel.setBounds(0, 40, 240, 16);
    mixValueLabel.setFont(getCustomFont(18.0f));
#endif
    mixValueLabel.setJustification(juce::Justification::centredTop);
    mixValueLabel.setMultiLine(false);
    mixValueLabel.setReturnKeyStartsNewLine(false);
    mixValueLabel.setInputRestrictions(10, "0123456789.-+");
    mixValueLabel.setSelectAllWhenFocused(true);
    mixValueLabel.setColour(juce::TextEditor::textColourId, foregroundColor);
    mixValueLabel.setColour(juce::TextEditor::backgroundColourId, backgroundColor);
    mixValueLabel.setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    mixValueLabel.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
    updateMixValueLabel();
    auto commitMix = [this]()
    {
        mixValueLabel.moveCaretToEnd(false);

        juce::String text = mixValueLabel.getText().trim();
        float value = text.getFloatValue();
        if (text.isEmpty() || !std::isfinite(value))
        {
            updateMixValueLabel();
            return;
        }
        value = juce::jlimit(0.0f, 1.0f, value);
        mixKnob.setValue(value);
        updateMixValueLabel();

        grabKeyboardFocus();
    };
    mixValueLabel.onReturnKey = commitMix;
    mixValueLabel.onFocusLost = commitMix;
}
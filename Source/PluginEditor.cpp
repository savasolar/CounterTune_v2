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
        updateTempoValueLabel();
        // ...
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
    mixTitleLabel.setBounds(0, -1, 240, 20);
    mixTitleLabel.setFont(getCustomFont(14.0f));
#else
    mixTitleLabel.setBounds(0, 0, 240, 20);
    mixTitleLabel.setFont(getCustomFont(18.0f));
#endif
    mixTitleLabel.setJustification(juce::Justification::centredLeft);
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
    mixValueLabel.setJustification(juce::Justification::topLeft);
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

    // TEMPO
    addAndMakeVisible(tempoTitleLabel);
#ifdef JUCE_MAC
    tempoTitleLabel.setBounds(0, 59, 240, 20);
    tempoTitleLabel.setFont(getCustomFont(14.0f));
#else
    tempoTitleLabel.setBounds(0, 60, 240, 20);
    tempoTitleLabel.setFont(getCustomFont(18.0f));
#endif
    tempoTitleLabel.setJustification(juce::Justification::centredLeft);
    tempoTitleLabel.setColour(juce::TextEditor::textColourId, foregroundColor);
    tempoTitleLabel.setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
    tempoTitleLabel.setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    tempoTitleLabel.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
    tempoTitleLabel.setReadOnly(true);
    tempoTitleLabel.setCaretVisible(false);
    tempoTitleLabel.setMouseCursor(juce::MouseCursor::NormalCursor);
    tempoTitleLabel.setText("TEMPO", dontSendNotification);

    tempoAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(parameters, "tempo", tempoKnob);
    tempoKnob.setSliderStyle(juce::Slider::LinearBar);
    tempoKnob.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    tempoKnob.setColour(juce::Slider::backgroundColourId, juce::Colours::transparentBlack);
    tempoKnob.setColour(juce::Slider::trackColourId, foregroundColor);
    tempoKnob.setColour(juce::Slider::thumbColourId, foregroundColor);
    tempoKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    tempoKnob.setBounds(0, 80, 240, 20);
    tempoKnob.setRange(60, 240, 1);
//    tempoKnob.setSkewFactor(0.3);
    tempoKnob.onValueChange = [this]() { updateTempoValueLabel(); };
    addAndMakeVisible(tempoKnob);

    addAndMakeVisible(tempoValueLabel);
#ifdef JUCE_MAC
    tempoValueLabel.setBounds(0, 99, 240, 16);
    tempoValueLabel.setFont(getCustomFont(14.0f));
#else
    tempoValueLabel.setBounds(0, 100, 240, 16);
    tempoValueLabel.setFont(getCustomFont(18.0f));
#endif
    tempoValueLabel.setJustification(juce::Justification::topLeft);
    tempoValueLabel.setMultiLine(false);
    tempoValueLabel.setReturnKeyStartsNewLine(false);
    tempoValueLabel.setInputRestrictions(10, "0123456789.-+");
    tempoValueLabel.setSelectAllWhenFocused(true);
    tempoValueLabel.setColour(juce::TextEditor::textColourId, foregroundColor);
    tempoValueLabel.setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
    tempoValueLabel.setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    tempoValueLabel.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
    updateTempoValueLabel();
    auto commitTempo = [this]()
    {
        tempoValueLabel.moveCaretToEnd(false);

        juce::String text = tempoValueLabel.getText().trim();
        float value = text.getFloatValue();
        if (text.isEmpty() || !std::isfinite(value))
        {
            updateTempoValueLabel();
            return;
        }
        value = juce::jlimit(60.0f, 240.0f, value);
        tempoKnob.setValue(value);
        updateTempoValueLabel();
        grabKeyboardFocus();
    };
    tempoValueLabel.onReturnKey = commitTempo;
    tempoValueLabel.onFocusLost = commitTempo;

    // PERIOD
    addAndMakeVisible(periodTitleLabel);
#ifdef JUCE_MAC
    periodTitleLabel.setBounds(0, 119, 240, 20);
    periodTitleLabel.setFont(getCustomFont(14.0f));
#else
    periodTitleLabel.setBounds(0, 120, 240, 20);
    periodTitleLabel.setFont(getCustomFont(18.0f));
#endif
    periodTitleLabel.setJustification(juce::Justification::centredLeft);
    periodTitleLabel.setColour(juce::TextEditor::textColourId, foregroundColor);
    periodTitleLabel.setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
    periodTitleLabel.setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    periodTitleLabel.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
    periodTitleLabel.setReadOnly(true);
    periodTitleLabel.setCaretVisible(false);
    periodTitleLabel.setMouseCursor(juce::MouseCursor::NormalCursor);
    periodTitleLabel.setText("PERIOD", dontSendNotification);

    periodAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(parameters, "period", periodKnob);
    periodKnob.setSliderStyle(juce::Slider::LinearBar);
    periodKnob.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    periodKnob.setColour(juce::Slider::backgroundColourId, juce::Colours::transparentBlack);
    periodKnob.setColour(juce::Slider::trackColourId, foregroundColor);
    periodKnob.setColour(juce::Slider::thumbColourId, foregroundColor);
    periodKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    periodKnob.setBounds(0, 140, 240, 20);
    periodKnob.setRange(1, 32, 1);
    periodKnob.onValueChange = [this]() { updatePeriodValueLabel(); };
    addAndMakeVisible(periodKnob);

    addAndMakeVisible(periodValueLabel);
#ifdef JUCE_MAC
    periodValueLabel.setBounds(0, 159, 240, 16);
    periodValueLabel.setFont(getCustomFont(14.0f));
#else
    periodValueLabel.setBounds(0, 160, 240, 16);
    periodValueLabel.setFont(getCustomFont(18.0f));
#endif
    periodValueLabel.setJustification(juce::Justification::topLeft);
    periodValueLabel.setMultiLine(false);
    periodValueLabel.setReturnKeyStartsNewLine(false);
    periodValueLabel.setInputRestrictions(10, "0123456789.-+");
    periodValueLabel.setSelectAllWhenFocused(true);
    periodValueLabel.setColour(juce::TextEditor::textColourId, foregroundColor);
    periodValueLabel.setColour(juce::TextEditor::backgroundColourId, backgroundColor);
    periodValueLabel.setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    periodValueLabel.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
    updatePeriodValueLabel();
    auto commitPeriod = [this]()
    {
        periodValueLabel.moveCaretToEnd(false);

        juce::String text = periodValueLabel.getText().trim();
        int value = text.getIntValue();
        if (text.isEmpty())
        {
            updatePeriodValueLabel();
            return;
        }
        value = juce::jlimit(1, 32, value);
        periodKnob.setValue(value);
        updatePeriodValueLabel();
        grabKeyboardFocus();
    };
    periodValueLabel.onReturnKey = commitPeriod;
    periodValueLabel.onFocusLost = commitPeriod;

    // DENSITY
    addAndMakeVisible(densityTitleLabel);
#ifdef JUCE_MAC
    densityTitleLabel.setBounds(0, 179, 240, 20);
    densityTitleLabel.setFont(getCustomFont(14.0f));
#else
    densityTitleLabel.setBounds(0, 180, 240, 20);
    densityTitleLabel.setFont(getCustomFont(18.0f));
#endif
    densityTitleLabel.setJustification(juce::Justification::centredLeft);
    densityTitleLabel.setColour(juce::TextEditor::textColourId, foregroundColor);
    densityTitleLabel.setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
    densityTitleLabel.setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    densityTitleLabel.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
    densityTitleLabel.setReadOnly(true);
    densityTitleLabel.setCaretVisible(false);
    densityTitleLabel.setMouseCursor(juce::MouseCursor::NormalCursor);
    densityTitleLabel.setText("DENSITY", dontSendNotification);

    densityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(parameters, "density", densityKnob);
    densityKnob.setSliderStyle(juce::Slider::LinearBar);
    densityKnob.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    densityKnob.setColour(juce::Slider::backgroundColourId, juce::Colours::transparentBlack);
    densityKnob.setColour(juce::Slider::trackColourId, foregroundColor);
    densityKnob.setColour(juce::Slider::thumbColourId, foregroundColor);
    densityKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    densityKnob.setBounds(0, 200, 240, 20);
    densityKnob.setRange(1, 6, 1);
    densityKnob.onValueChange = [this]() { updateDensityValueLabel(); };
    addAndMakeVisible(densityKnob);

    addAndMakeVisible(densityValueLabel);
#ifdef JUCE_MAC
    densityValueLabel.setBounds(0, 219, 240, 16);
    densityValueLabel.setFont(getCustomFont(14.0f));
#else
    densityValueLabel.setBounds(0, 220, 240, 16);
    densityValueLabel.setFont(getCustomFont(18.0f));
#endif
    densityValueLabel.setJustification(juce::Justification::topLeft);
    densityValueLabel.setMultiLine(false);
    densityValueLabel.setReturnKeyStartsNewLine(false);
    densityValueLabel.setInputRestrictions(10, "0123456789.-+");
    densityValueLabel.setSelectAllWhenFocused(true);
    densityValueLabel.setColour(juce::TextEditor::textColourId, foregroundColor);
    densityValueLabel.setColour(juce::TextEditor::backgroundColourId, backgroundColor);
    densityValueLabel.setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    densityValueLabel.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
    updateDensityValueLabel();
    auto commitDensity = [this]()
        {
            densityValueLabel.moveCaretToEnd(false);

            juce::String text = densityValueLabel.getText().trim();
            int value = text.getIntValue();
            if (text.isEmpty())
            {
                updateDensityValueLabel();
                return;
            }
            value = juce::jlimit(1, 6, value);
            densityKnob.setValue(value);
            updateDensityValueLabel();
            grabKeyboardFocus();
        };
    densityValueLabel.onReturnKey = commitDensity;
    densityValueLabel.onFocusLost = commitDensity;

    // KEY
    addAndMakeVisible(keyTitleLabel);
#ifdef JUCE_MAC
    keyTitleLabel.setBounds(0, 239, 240, 20);
    keyTitleLabel.setFont(getCustomFont(14.0f));
#else
    keyTitleLabel.setBounds(0, 240, 240, 20);
    keyTitleLabel.setFont(getCustomFont(18.0f));
#endif
    keyTitleLabel.setJustification(juce::Justification::centredLeft);
    keyTitleLabel.setColour(juce::TextEditor::textColourId, foregroundColor);
    keyTitleLabel.setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
    keyTitleLabel.setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    keyTitleLabel.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
    keyTitleLabel.setReadOnly(true);
    keyTitleLabel.setCaretVisible(false);
    keyTitleLabel.setMouseCursor(juce::MouseCursor::NormalCursor);
    keyTitleLabel.setText("KEY", dontSendNotification);

    keyAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(parameters, "key", keyKnob);
    keyKnob.setSliderStyle(juce::Slider::LinearBar);
    keyKnob.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    keyKnob.setColour(juce::Slider::backgroundColourId, juce::Colours::transparentBlack);
    keyKnob.setColour(juce::Slider::trackColourId, foregroundColor);
    keyKnob.setColour(juce::Slider::thumbColourId, foregroundColor);
    keyKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    keyKnob.setBounds(0, 260, 240, 20);
    keyKnob.setRange(0, 11, 1);
    keyKnob.onValueChange = [this]() { updateKeyValueLabel(); };
    addAndMakeVisible(keyKnob);

    addAndMakeVisible(keyValueLabel);
#ifdef JUCE_MAC
    keyValueLabel.setBounds(0, 279, 240, 16);
    keyValueLabel.setFont(getCustomFont(14.0f));
#else
    keyValueLabel.setBounds(0, 280, 240, 16);
    keyValueLabel.setFont(getCustomFont(18.0f));
#endif
    keyValueLabel.setJustification(juce::Justification::centredLeft);
    keyValueLabel.setMultiLine(false);
    keyValueLabel.setReturnKeyStartsNewLine(false);
    keyValueLabel.setSelectAllWhenFocused(true);
    keyValueLabel.setColour(juce::TextEditor::textColourId, foregroundColor);
    keyValueLabel.setColour(juce::TextEditor::backgroundColourId, backgroundColor);
    keyValueLabel.setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    keyValueLabel.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
    updateKeyValueLabel();

    auto commitKey = [this]()
    {
        keyValueLabel.moveCaretToEnd(false);

        juce::String text = keyValueLabel.getText().trim().toUpperCase();

        if (text.isEmpty())
        {
            updateKeyValueLabel();
            return;
        }

        // Try parsing as integer first (backward compat)
        bool isNum = true;
        for (auto c : text) { if (!juce::CharacterFunctions::isDigit(c)) { isNum = false; break; } }
        if (isNum)
        {
            int value = text.getIntValue();
            if (value >= 0 && value <= 12)
            {
                keyKnob.setValue(value);
                updateKeyValueLabel();
                grabKeyboardFocus();
                return;
            }
        }

        // Then try sharp/flat names
        int value = keyNames.indexOf(text);
        if (value == -1)
            value = altKeyNames.indexOf(text);

        if (value >= 0 && value <= 12)
        {
            keyKnob.setValue(value);
            updateKeyValueLabel();
        }
        else
        {
            updateKeyValueLabel(); // Revert if invalid
        }

        grabKeyboardFocus();
    };
    keyValueLabel.onReturnKey = commitKey;
    keyValueLabel.onFocusLost = commitKey;

    // SCALE
    addAndMakeVisible(scaleTitleLabel);
#ifdef JUCE_MAC
    scaleTitleLabel.setBounds(0, 299, 240, 20);
    scaleTitleLabel.setFont(getCustomFont(14.0f));
#else
    scaleTitleLabel.setBounds(0, 300, 240, 20);
    scaleTitleLabel.setFont(getCustomFont(18.0f));
#endif
    scaleTitleLabel.setJustification(juce::Justification::centredLeft);
    scaleTitleLabel.setColour(juce::TextEditor::textColourId, foregroundColor);
    scaleTitleLabel.setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
    scaleTitleLabel.setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    scaleTitleLabel.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
    scaleTitleLabel.setReadOnly(true);
    scaleTitleLabel.setCaretVisible(false);
    scaleTitleLabel.setMouseCursor(juce::MouseCursor::NormalCursor);
    scaleTitleLabel.setText("SCALE", dontSendNotification);

    scaleAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(parameters, "scale", scaleKnob);
    scaleKnob.setSliderStyle(juce::Slider::LinearBar);
    scaleKnob.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    scaleKnob.setColour(juce::Slider::backgroundColourId, juce::Colours::transparentBlack);
    scaleKnob.setColour(juce::Slider::trackColourId, foregroundColor);
    scaleKnob.setColour(juce::Slider::thumbColourId, foregroundColor);
    scaleKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    scaleKnob.setBounds(0, 320, 240, 20);
    scaleKnob.setRange(1, 4, 1);
    scaleKnob.onValueChange = [this]() { updateScaleValueLabel(); };
    addAndMakeVisible(scaleKnob);

    addAndMakeVisible(scaleValueLabel);
#ifdef JUCE_MAC
    scaleValueLabel.setBounds(0, 339, 240, 16);
    scaleValueLabel.setFont(getCustomFont(14.0f));
#else
    scaleValueLabel.setBounds(0, 340, 240, 16);
    scaleValueLabel.setFont(getCustomFont(18.0f));
#endif
    scaleValueLabel.setJustification(juce::Justification::topLeft);
    scaleValueLabel.setMultiLine(false);
    scaleValueLabel.setReturnKeyStartsNewLine(false);
    scaleValueLabel.setInputRestrictions(10, "0123456789.-+");
    scaleValueLabel.setSelectAllWhenFocused(true);
    scaleValueLabel.setColour(juce::TextEditor::textColourId, foregroundColor);
    scaleValueLabel.setColour(juce::TextEditor::backgroundColourId, backgroundColor);
    scaleValueLabel.setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    scaleValueLabel.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
    updateScaleValueLabel();
    auto commitScale = [this]()
    {
        scaleValueLabel.moveCaretToEnd(false);

        juce::String text = scaleValueLabel.getText().trim();
        int value = text.getIntValue();
        if (text.isEmpty())
        {
            updateScaleValueLabel();
            return;
        }
        value = juce::jlimit(1, 4, value);
        scaleKnob.setValue(value);
        updateScaleValueLabel();

        grabKeyboardFocus();
    };
    scaleValueLabel.onReturnKey = commitScale;
    scaleValueLabel.onFocusLost = commitScale;


}

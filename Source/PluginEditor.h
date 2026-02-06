// PluginEditor.h

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class CounterTune_v2AudioProcessorEditor  : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    CounterTune_v2AudioProcessorEditor (CounterTune_v2AudioProcessor&);
    ~CounterTune_v2AudioProcessorEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    CounterTune_v2AudioProcessor& audioProcessor;

    juce::Image backgroundImage;

    // Refactored WaveformViewer - now buffer-agnostic
    struct WaveformViewer : public juce::Component
    {
        WaveformViewer() = default;

        void setAudioBuffer(const juce::AudioBuffer<float>* buffer, int numSamplesToDisplay)
        {
            audioBuffer = buffer;
            numSamples = numSamplesToDisplay;
            repaint();
        }

        void paint(juce::Graphics& g) override
        {
            if (audioBuffer == nullptr || numSamples == 0) return;
            if (audioBuffer->getNumChannels() == 0) return;

            g.setColour(juce::Colours::white);



            // <new>
            auto bounds = getLocalBounds().toFloat();
            auto centre = bounds.getCentre();

            g.saveState();
            g.addTransform(juce::AffineTransform::rotation(juce::MathConstants<float>::pi / 4.0f, centre.getX(), centre.getY()));

            // </new>



            float w = static_cast<float> (getWidth());
            float h = static_cast<float> (getHeight()) / 2.0f;
            float centreY = static_cast<float> (getHeight()) / 2.0f;

            auto* data = audioBuffer->getReadPointer(0);

            juce::Path p;
            float step = static_cast<float> (numSamples) / w;
            if (step < 1.0f) step = 1.0f;

            bool first = true;
            for (float x = 0; x < w; x += 1.0f)
            {
                int idx = static_cast<int> (x * step);
                if (idx >= numSamples) break;

                float sample = juce::jlimit(-1.0f, 1.0f, data[idx]);
                float y = centreY - sample * centreY;

                if (first)
                {
                    p.startNewSubPath(x, y);
                    first = false;
                }
                else
                {
                    p.lineTo(x, y);
                }
            }

            g.strokePath(p, juce::PathStrokeType(1.0f));

            g.restoreState();
        }

    private:
        const juce::AudioBuffer<float>* audioBuffer = nullptr;
        int numSamples = 0;
    };

    WaveformViewer waveform;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CounterTune_v2AudioProcessorEditor)
};

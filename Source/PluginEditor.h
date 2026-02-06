// PluginEditor.h

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <cmath>

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
        //WaveformViewer() = default;

        WaveformViewer(CounterTune_v2AudioProcessor& proc) : audioProcessor(proc) {}



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

            float leftPoint = (11.0f - static_cast<float>(audioProcessor.currentInputNote)) * 43.64f;
            float rightPoint = (11.0f - static_cast<float>(audioProcessor.currentOutputNote)) * 43.64f;




            // Fixed background square (in local component coords)
            juce::Rectangle<float> fixedRect = getLocalBounds().toFloat();
            juce::Point<float> center = fixedRect.getCentre();





            // STEP 1: RESIZE
            // - figure out hypotenuse between (60.0f, leftPoint) and (539.0f, rightPoint)
            // - set drawable area as a square with its width and height
            // - also figure out the angle of rotation between the hypotenuse and horizon

            float dx = fixedRect.getWidth();
            float dy = rightPoint - leftPoint;
            float side = std::hypot(dx, dy);
            float angle = std::atan2(dy, dx);  // radians




            // STEP 2: REPOSITION
            // - the center point of the resized drawable area has to align with the center point of x:60, y:0, w:480, h:480

            juce::Rectangle<float> drawRect(0.f, 0.f, side, side);
            drawRect = drawRect.withCentre(center);





            // STEP 3: ROTATE
            // - given the angle of rotation, rotate the resized, repositioned thing

            juce::Graphics::ScopedSaveState save(g);
            g.addTransform(juce::AffineTransform::rotation(angle, center.getX(), center.getY()));





//            // <new>
//            auto bounds = getLocalBounds().toFloat();
//            auto centre = bounds.getCentre();
//
//            g.saveState();
////            g.addTransform(juce::AffineTransform::rotation(juce::MathConstants<float>::pi / 4.0f, centre.getX(), centre.getY()));
//
//            float angleDegrees = 0.0f;  // change this to whatever you want
//
//            g.addTransform(juce::AffineTransform::rotation(angleDegrees * (juce::MathConstants<float>::pi / 180.0f), centre.getX(), centre.getY()));
//
//            // </new>



//            float w = static_cast<float> (getWidth());
////            float h = static_cast<float> (getHeight()) / 2.0f;
//            float centreY = static_cast<float> (getHeight()) / 2.0f;


            // Now draw the waveform inside drawRect
            float w = drawRect.getWidth();
            float h = drawRect.getHeight();
            float centreY = drawRect.getY() + h / 2.0f;


//            auto* data = audioBuffer->getReadPointer(0);

            juce::Path p;
            float step = static_cast<float> (numSamples) / w;
            if (step < 1.0f) step = 1.0f;

            bool first = true;

            const float* data = audioBuffer->getReadPointer(0);


            //for (float x = 0; x < w; x += 1.0f)
            //{
            //    int idx = static_cast<int> (x * step);
            //    if (idx >= numSamples) break;

            //    float sample = juce::jlimit(-1.0f, 1.0f, data[idx]);
            //    float y = centreY - sample * centreY;

            //    if (first)
            //    {
            //        p.startNewSubPath(x, y);
            //        first = false;
            //    }
            //    else
            //    {
            //        p.lineTo(x, y);
            //    }
            //}

            for (float x_pos = 0; x_pos < w; x_pos += 1.0f)
            {
                int idx = static_cast<int> (x_pos * step);
                if (idx >= numSamples) break;

                float sample = juce::jlimit(-1.0f, 1.0f, data[idx]);
                float y = centreY - sample * (h / 2.0f);
                float x = drawRect.getX() + x_pos;

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

//            g.restoreState();
        }

    private:
        CounterTune_v2AudioProcessor& audioProcessor;

        const juce::AudioBuffer<float>* audioBuffer = nullptr;
        int numSamples = 0;
    };

    WaveformViewer waveform;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CounterTune_v2AudioProcessorEditor)
};

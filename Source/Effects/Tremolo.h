/*
  ==============================================================================

    Plugin: AutoTremolando
    GitHub: MontyWh
    Author: Montague Whishaw
    Date/Time: 24th April 2026
    General Language: English (UK)

    Tremolo equations-only helpers.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <algorithm>
#include <cmath>

class Tremolo
{
public:
    static float getChannelScale(int channelIndex, int totalNumInputChannels)
    {
        return (totalNumInputChannels <= 1) ? 0.0f
            : static_cast<float>(channelIndex) / static_cast<float>(totalNumInputChannels - 1);
    }

    static float getPhaseOffsetForChannel(int channelIndex, int totalNumInputChannels, float phaseOffsetDegrees, float surroundWidth)
    {
        const float fMaxOffset = juce::degreesToRadians(phaseOffsetDegrees) * surroundWidth;
        return fMaxOffset * getChannelScale(channelIndex, totalNumInputChannels);
    }

    static float getRateWithOffset(float rate, float rateOffset, float channelScale)
    {
        return std::clamp(rate + (rateOffset * channelScale), 0.5f, 16.0f);
    }

    static float getDepthWithOffset(float depth, float depthOffset, float channelScale)
    {
        return std::clamp(depth + (depthOffset * channelScale), 0.0f, 1.0f);
    }

    static float getPhaseIncrement(float rate, float sampleRate)
    {
        return (juce::MathConstants<float>::twoPi * rate) / sampleRate;
    }

    static float wrapPhase(float phase)
    {
        const float fDoublePi = juce::MathConstants<float>::twoPi;
        while (phase > fDoublePi)
            phase -= fDoublePi;
        return phase;
    }

    static float getOscillatorValue(int choice, float phase, float pulseWidth)
    {
        if (choice == 0)
            return std::sin(phase);
        if (choice == 1)
            return (phase < juce::MathConstants<float>::pi) ? (-1.0f + ((2.0f / juce::MathConstants<float>::pi) * phase)) : (3.0f - ((2.0f / juce::MathConstants<float>::pi) * phase));
        if (choice == 2)
            return (phase / juce::MathConstants<float>::pi) - 1.0f;
        if (choice == 3)
            return (phase < pulseWidth * juce::MathConstants<float>::twoPi) ? 1.0f : -1.0f;
        if (choice == 4)
            return (phase < juce::MathConstants<float>::pi) ? 1.0f : -1.0f;

        return 0.0f;
    }

    static float getTremoloGain(float oscillatorValue, float depth, int depthMode)
    {
        if (depthMode == 0)
            return (1.0f - depth) + (depth * ((oscillatorValue + 1.0f) * 0.5f));

        return std::max(0.0f, 1.0f + (oscillatorValue * depth));
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Tremolo)
};

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
#include <array>
#include <cmath>
#include <vector>

//==============================================================================
/*
*/
class Tremolo  : public juce::Component
{
public:
    static constexpr int iBandCount = 4;
    static constexpr int iNoteDivisionCount = 15;

    Tremolo()
    {
        // In your constructor, you should add any child components, and
        // initialise any special settings that your component needs.
    }

    ~Tremolo() override
    {
    }

    //==============================================================================
    void initialisePhaseState(std::vector<float>& phaseOffsets,
        std::vector<std::array<float, iBandCount>>& phasePositions,
        int numInputChannels,
        int numOutputChannels,
        float startPhaseDegrees)
    {
        phaseOffsets.assign(numOutputChannels, 0.0f);
        phasePositions.assign(static_cast<size_t>(numInputChannels), { 0.0f, 0.0f, 0.0f, 0.0f });
        retriggerPhases(phasePositions, startPhaseDegrees);
    }

    void retriggerPhases(std::vector<std::array<float, iBandCount>>& phasePositions, float startPhaseDegrees)
    {
        const float fStartPhaseRadians = juce::degreesToRadians(startPhaseDegrees);

        for (auto& fChannelPhases : phasePositions)
            fChannelPhases.fill(fStartPhaseRadians);
    }

    void processRates(std::array<float, iBandCount>& rates, const std::array<int, iBandCount>& divisionIndices, float bpm, bool applyTempoSync, bool applyRateLock)
    {
        if (applyTempoSync)
        {
            static constexpr std::array<float, iNoteDivisionCount> fTempoMultipliers =
            {
                0.25f, 0.166667f, 0.375f,
                0.5f, 0.333333f, 0.75f,
                1.0f, 0.666667f, 1.5f,
                2.0f, 1.333333f, 3.0f,
                4.0f, 2.666667f, 6.0f
            };

            const float fBeatsPerSecond = bpm / 60.0f;

            for (int iBand = 0; iBand < iBandCount; ++iBand)
            {
                const int iClampedIndex = juce::jlimit(0, iNoteDivisionCount - 1, divisionIndices[static_cast<size_t>(iBand)]);
                rates[static_cast<size_t>(iBand)] = fBeatsPerSecond * fTempoMultipliers[static_cast<size_t>(iClampedIndex)];
            }
        }

        if (applyRateLock)
            std::fill(rates.begin() + 1, rates.end(), rates.front());
    }

    void prepareModulation(std::vector<float>& phaseOffsets,
        std::array<float, iBandCount>& channelRates,
        std::array<float, iBandCount>& channelDepths,
        const std::array<float, iBandCount>& rates,
        const std::array<float, iBandCount>& depths,
        float phaseOffsetDegrees,
        float surroundWidth,
        float rateOffset,
        float depthOffset,
        int channelIndex,
        int totalNumInputChannels,
        int totalNumOutputChannels)
    {
        if (surroundWidth <= 0.0f || totalNumInputChannels <= 1)
        {
            std::fill(phaseOffsets.begin(), phaseOffsets.end(), 0.0f);
        }
        else
        {
            const float fMaxOffset = juce::degreesToRadians(phaseOffsetDegrees) * surroundWidth;

            for (int iChannel = 0; iChannel < totalNumOutputChannels; ++iChannel)
            {
                const float fChannelScale = juce::jmap(static_cast<float>(iChannel),
                    0.0f,
                    static_cast<float>(totalNumInputChannels - 1),
                    0.0f,
                    1.0f);

                phaseOffsets[static_cast<size_t>(iChannel)] = fMaxOffset * fChannelScale;
            }
        }

        const float fCurrentChannelScale = (totalNumInputChannels <= 1)
            ? 0.0f
            : juce::jmap(static_cast<float>(channelIndex),
                0.0f,
                static_cast<float>(totalNumInputChannels - 1),
                0.0f,
                1.0f);

        for (int iBand = 0; iBand < iBandCount; ++iBand)
        {
            channelRates[static_cast<size_t>(iBand)] = juce::jlimit(0.5f, 16.0f,
                rates[static_cast<size_t>(iBand)] + (rateOffset * fCurrentChannelScale));

            channelDepths[static_cast<size_t>(iBand)] = juce::jlimit(0.0f, 1.0f,
                depths[static_cast<size_t>(iBand)] + (depthOffset * fCurrentChannelScale));
        }
    }

    void applyBandTremolo(std::array<float, iBandCount>& bandValues,
        std::vector<std::array<float, iBandCount>>& phasePositions,
        int channelIndex,
        const std::array<float, iBandCount>& channelRates,
        const std::vector<float>& phaseOffsets,
        const std::array<int, iBandCount>& choices,
        float pulseWidth,
        const std::array<float, iBandCount>& channelDepths,
        int depthMode,
        float sampleRate)
    {
        const float fPi = juce::MathConstants<float>::pi;
        const float fTwoPi = juce::MathConstants<float>::twoPi;

        for (int iBand = 0; iBand < iBandCount; ++iBand)
        {
            float fPhase = phasePositions[static_cast<size_t>(channelIndex)][static_cast<size_t>(iBand)]
                + ((fTwoPi * channelRates[static_cast<size_t>(iBand)]) / sampleRate);

            fPhase = std::fmod(fPhase, fTwoPi);
            if (fPhase < 0.0f)
                fPhase += fTwoPi;

            phasePositions[static_cast<size_t>(channelIndex)][static_cast<size_t>(iBand)] = fPhase;

            float fShiftedPhase = std::fmod(fPhase + phaseOffsets[static_cast<size_t>(channelIndex)], fTwoPi);
            if (fShiftedPhase < 0.0f)
                fShiftedPhase += fTwoPi;

            float fOscillator = 0.0f;

            switch (choices[static_cast<size_t>(iBand)])
            {
                case 0:
                    fOscillator = std::sin(fShiftedPhase);
                    break;

                case 1:
                    fOscillator = (fShiftedPhase < fPi)
                        ? (-1.0f + ((2.0f / fPi) * fShiftedPhase))
                        : (3.0f - ((2.0f / fPi) * fShiftedPhase));
                    break;

                case 2:
                    fOscillator = (fShiftedPhase / fPi) - 1.0f;
                    break;

                case 3:
                    fOscillator = (fShiftedPhase < pulseWidth * fTwoPi) ? 1.0f : -1.0f;
                    break;

                case 4:
                    fOscillator = (fShiftedPhase < fPi) ? 1.0f : -1.0f;
                    break;

                default:
                    fOscillator = 0.0f;
                    break;
            }

            const float fTremolo = (depthMode == 0)
                ? ((1.0f - channelDepths[static_cast<size_t>(iBand)])
                    + (channelDepths[static_cast<size_t>(iBand)] * ((fOscillator + 1.0f) * 0.5f)))
                : juce::jmax(0.0f, 1.0f + (fOscillator * channelDepths[static_cast<size_t>(iBand)]));

            bandValues[static_cast<size_t>(iBand)] *= fTremolo;
        }
    }

    //==============================================================================
    void paint (juce::Graphics& g) override {}

    void resized() override {}

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Tremolo)
};

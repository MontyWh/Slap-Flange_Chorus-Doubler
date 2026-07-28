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
		phaseOffsets.assign(numOutputChannels, 0.0f); // Initialise phase offsets for each output channel
		phasePositions.assign(numInputChannels, { 0.0f, 0.0f, 0.0f, 0.0f }); // Initialise phase positions for each input channel and band
		retriggerPhases(phasePositions, startPhaseDegrees); // Retrigger the phases for each input channel and band
    }

    void retriggerPhases(std::vector<std::array<float, iBandCount>>& phasePositions, float startPhaseDegrees)
    {
        const float fStartPhaseRadians = juce::degreesToRadians(startPhaseDegrees); // Convert start phase from degrees to radians

        for (auto& fChannelPhases : phasePositions)
            fChannelPhases.fill(fStartPhaseRadians); // Set all phases for the channel to the start phase
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
			}; // Tempo multipliers for different note divisions: 1/4, 1/6, 3/8, 1/2, 1/3, 3/4, 1, 2/3, 3/2, 2, 4/3, 3, 4, 8/3, 6

			const float fBeatsPerSecond = bpm / 60.0f; // Convert BPM to beats per second

			for (int iBand = 0; iBand < iBandCount; ++iBand)
				rates[iBand] = fBeatsPerSecond * fTempoMultipliers[divisionIndices[iBand]]; // Calculate the rate for the band
        }

        if (applyRateLock)
			std::fill(rates.begin() + 1, rates.end(), rates.front()); // Lock all rates to the first band's rate
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
		int totalNumOutputChannels) // Prepare the modulation parameters for the tremolo effect
    {
		if (surroundWidth <= 0.0f || totalNumInputChannels <= 1) // If surround width is zero or there is only one input channel, set all phase offsets to zero
        {
            std::fill(phaseOffsets.begin(), phaseOffsets.end(), 0.0f);
        }
        else
        {
			const float fMaxOffset = juce::degreesToRadians(phaseOffsetDegrees) * surroundWidth; // Calculate the maximum phase offset in radians based on the surround width

            for (int iChannel = 0; iChannel < totalNumOutputChannels; ++iChannel)
            {
				const float fChannelScale = juce::jmap(static_cast<float>(iChannel), // Map the channel index to a scale factor between 0 and 1 based on the total number of input channels
                    0.0f, // Minimum input value
					static_cast<float>(totalNumInputChannels - 1), // Maximum input value
					0.0f, // Minimum output value
					1.0f); // Maximum output value

				phaseOffsets[iChannel] = fMaxOffset * fChannelScale; // Set the phase offset for the channel based on the maximum offset and the scale factor
            }
        }

        const float fCurrentChannelScale = (totalNumInputChannels <= 1)
            ? 0.0f
            : juce::jmap(static_cast<float>(channelIndex),
                0.0f,
                static_cast<float>(totalNumInputChannels - 1),
                0.0f,
				1.0f); // Calculate the current channel scale factor based on the channel index and total number of input channels

        for (int iBand = 0; iBand < iBandCount; ++iBand)
        {
			channelRates[iBand] = juce::jlimit(0.5f, 16.0f, // Limit the channel rate to a range of 0.5 Hz to 16 Hz
				rates[iBand] + (rateOffset * fCurrentChannelScale));

			channelDepths[iBand] = juce::jlimit(0.0f, 1.0f, // Limit the channel depth to a range of 0.0 to 1.0
				depths[iBand] + (depthOffset * fCurrentChannelScale));
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
		float sampleRate) // Apply the band tremolo effect to the band values based on the modulation parameters
    {

        for (int iBand = 0; iBand < iBandCount; ++iBand)
        {
			float fPhase = phasePositions[channelIndex][iBand]
				+ ((fTwo_π * channelRates[iBand]) / sampleRate); // Update the phase for the band based on the channel rate and sample rate

			fPhase = std::fmod(fPhase, fTwo_π); // Wrap the phase to the range [0, 2fπ]
			if (fPhase < 0.0f) // Ensure the phase is non-negative
                fPhase += fTwo_π;

			phasePositions[channelIndex][iBand] = fPhase; // Store the updated phase for the band

			float fShiftedPhase = std::fmod(fPhase + phaseOffsets[channelIndex], fTwo_π); // Apply the phase offset for the channel and wrap to [0, 2fπ]
			if (fShiftedPhase < 0.0f) // Ensure the shifted phase is non-negative
				fShiftedPhase += fTwo_π; // Wrap the shifted phase to the range [0, 2fπ]

			float fOscillator = 0.0f; // Initialise the oscillator value for the band

            switch (choices[iBand])
            {
			    case 0: // Sine wave
                    fOscillator = std::sin(fShiftedPhase);
                    break;

			    case 1: // Triangle wave
                    fOscillator = (fShiftedPhase < fπ)
                        ? (-1.0f + ((2.0f / fπ) * fShiftedPhase))
                        : (3.0f - ((2.0f / fπ) * fShiftedPhase));
                    break;

			    case 2: // Sawtooth wave
                    fOscillator = (fShiftedPhase / fπ) - 1.0f;
                    break;

				case 3: // Pulse wave
                    fOscillator = (fShiftedPhase < pulseWidth * fTwo_π) ? 1.0f : -1.0f;
                    break;

				case 4: // Square wave
                    fOscillator = (fShiftedPhase < fπ) ? 1.0f : -1.0f;
                    break;

				default: // Default to silence if the choice is invalid
                    fOscillator = 0.0f;
                    break;
            }

            const float fTremolo = (depthMode == 0)
                ? ((1.0f - channelDepths[iBand])
                    + (channelDepths[iBand] * ((fOscillator + 1.0f) * 0.5f)))
                : juce::jmax(0.0f, 1.0f + (fOscillator * channelDepths[iBand]));

            bandValues[iBand] *= fTremolo;
        }
    }

    //==============================================================================
    void paint (juce::Graphics& g) override {}

    void resized() override {}

private:

	const float fπ = juce::MathConstants<float>::pi;
	const float fTwo_π = juce::MathConstants<float>::twoPi;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Tremolo)
};

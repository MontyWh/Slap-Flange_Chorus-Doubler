//  PluginExtra.h
//  Additional Plugin Code
//
//  TremoloProcess class encapsulates tremolo DSP.
//  Structure and tone preserved.
//

#pragma once
#include <cmath>
#include <corecrt_math_defines.h>
#include <iostream>
#include <algorithm>
#include <array>
#include <vector>

class TremoloProcess
{
public:
	TremoloProcess()
	{
		for (int b = 0; b < 4; ++b)
			fPhaseInc[b] = 0.0f;
	}

	// Reset phase positions (called in prepareToPlay)
	void reset(int numChannels)
	{
		fPhasePos.assign(numChannels, { 0.0f, 0.0f, 0.0f, 0.0f });
	}

	// Compute phase increments for this block
	void computePhaseIncrements(float fRate[4], float fSampleRate)
	{
		const float fTwoPI = static_cast<float>(2.0f * M_PI);

		for (int b = 0; b < 4; ++b)
			fPhaseInc[b] = (fTwoPI * fRate[b]) / fSampleRate;
	}

	// Compute phase increments for this channel
	void computeChannelPhaseIncrements(float fRate[4], float fSampleRate)
	{
		computePhaseIncrements(fRate, fSampleRate);
	}

	void sineProcess(const float fPhase, float& osc)
	{
		osc = std::sin(fPhase);
	}

	void triangleProcess(const float fPhase, float& osc)
	{
		if (0 <= fPhase && fPhase < M_PI)
			osc = -1.0f + (2.0f / M_PI) * fPhase;
		else if (M_PI <= fPhase && fPhase < 2 * M_PI)
			osc = 3.0f - (2.0f / M_PI) * fPhase;
	}

	void sawtoothProcess(const float fPhase, float& osc)
	{
		osc = (fPhase / M_PI) - 1.0f;
	}

	void pulseProcess(const float fPhase, const float fTwoPI, float fPulseWidth, float& osc)
	{
		const float p = fPhase;

		osc = (p < fPulseWidth * fTwoPI) ? 1.0f : -1.0f;
	}


	void squareProcess(const float fPhase, const float fTwoPI, float& osc)
	{
		juce::ignoreUnused(fTwoPI);
		osc = (fPhase < M_PI) ? 1.0f : -1.0f;
	}

	// Apply tremolo to each band
	void processBands(int channel, float band[4], float depth[4], int choice[4], float fOffset, float fPulseWidth)
	{
		const float fTwoPI = static_cast<float>(2.0f * M_PI);

		for (int b = 0; b < 4; ++b)
		{
			fPhasePos[channel][b] += fPhaseInc[b];
			if (fPhasePos[channel][b] > fTwoPI)
				fPhasePos[channel][b] -= fTwoPI;

			float fPhase = fPhasePos[channel][b] + fOffset;
			while (fPhase > fTwoPI)
				fPhase -= fTwoPI;

			float osc = 0.0f;
			if (choice[b] == 0)
				sineProcess(fPhase, osc);
			else if (choice[b] == 1)
				triangleProcess(fPhase, osc);
			else if (choice[b] == 2)
				sawtoothProcess(fPhase, osc);
			else if (choice[b] == 3)
				pulseProcess(fPhase, fTwoPI, fPulseWidth, osc);
			else if (choice[b] == 4)
				squareProcess(fPhase, fTwoPI, osc);


			float trem = (osc * depth[b] * 0.5f) + 0.5f;

			band[b] *= trem;
		}
	}

	// Apply tremolo to this channel using effective values
	void processChannelBands(int channel, float band[4], float depth[4], int choice[4], float fRate[4], float fSampleRate, float fOffset, float fPulseWidth)
	{
		computeChannelPhaseIncrements(fRate, fSampleRate);
		processBands(channel, band, depth, choice, fOffset, fPulseWidth);
	}

private:
	std::vector<std::array<float, 4>> fPhasePos;
	float fPhaseInc[4];
};

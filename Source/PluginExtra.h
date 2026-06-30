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

constexpr float DOUBLE_PI = static_cast<float>(2.0f * M_PI);

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
	void computePhaseIncrements(float rate[4], float sampleRate)
	{

		for (int b = 0; b < 4; ++b)
			fPhaseInc[b] = (DOUBLE_PI * rate[b]) / sampleRate;
	}

	void sineProcess(const float phase, float& osc)
	{
		osc = std::sin(phase);
	}

	void triangleProcess(const float phase, float& osc)
	{
		if (0 <= phase && phase < M_PI)
			osc = -1.0f + (2.0f / M_PI) * phase;
		else if (M_PI <= phase && phase < 2 * M_PI)
			osc = 3.0f - (2.0f / M_PI) * phase;
	}

	void sawtoothProcess(const float phase, float& osc)
	{
		osc = (phase / M_PI) - 1.0f;
	}

	void pulseProcess(const float phase, float pulseWidth, float& osc)
	{
		const float p = phase;

		osc = (p < pulseWidth * DOUBLE_PI) ? 1.0f : -1.0f;
	}


	void squareProcess(const float phase, float& osc)
	{
		juce::ignoreUnused(DOUBLE_PI);
		osc = (phase < M_PI) ? 1.0f : -1.0f;
	}

	// Apply tremolo to each band
	void processBands(int channel, float band[4], float depth[4], int choice[4], float offset, float pulseWidth)
	{
		for (int b = 0; b < 4; ++b)
		{
			fPhasePos[channel][b] += fPhaseInc[b];
			if (fPhasePos[channel][b] > DOUBLE_PI)
				fPhasePos[channel][b] -= DOUBLE_PI;

			float fPhase = fPhasePos[channel][b] + offset;
			while (fPhase > DOUBLE_PI)
				fPhase -= DOUBLE_PI;

			float fOsc = 0.0f;
			if (choice[b] == 0)
				sineProcess(fPhase, fOsc);
			else if (choice[b] == 1)
				triangleProcess(fPhase, fOsc);
			else if (choice[b] == 2)
				sawtoothProcess(fPhase, fOsc);
			else if (choice[b] == 3)
				pulseProcess(fPhase, pulseWidth, fOsc);
			else if (choice[b] == 4)
				squareProcess(fPhase, fOsc);


			float fTrem = (fOsc * depth[b] * 0.5f) + 0.5f;

			band[b] *= fTrem;
		}
	}

	// Apply tremolo to this channel using effective values
	void processChannelBands(int channel, float band[4], float depth[4], int choice[4], float rate[4], float sampleRate, float offset, float pulseWidth)
	{
		computePhaseIncrements(rate, sampleRate);
		processBands(channel, band, depth, choice, offset, pulseWidth);
	}

private:
	std::vector<std::array<float, 4>> fPhasePos;
	float fPhaseInc[4];
};

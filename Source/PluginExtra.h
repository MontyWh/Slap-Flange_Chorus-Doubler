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
		for (int iBand = 0; iBand < 4; ++iBand)
			fPhaseInc[iBand] = 0.0f;
	}

	// Reset phase positions (called in prepareToPlay)
	void reset(int numChannels)
	{
		fPhasePos.assign(numChannels, { 0.0f, 0.0f, 0.0f, 0.0f });
	}

	// Compute phase increments for this block
	void computePhaseIncrements(float rate[4], float sampleRate)
	{
		for (int iBand = 0; iBand < 4; ++iBand)
			fPhaseInc[iBand] = (DOUBLE_PI * rate[iBand]) / sampleRate;
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
		osc = (phase < pulseWidth * DOUBLE_PI) ? 1.0f : -1.0f;
	}

	void squareProcess(const float phase, float& osc)
	{
		juce::ignoreUnused(DOUBLE_PI);
		osc = (phase < M_PI) ? 1.0f : -1.0f;
	}

	void retrigger(float startPhase)
	{
		for (auto& fChannelPhases : fPhasePos)
			for (int iBand = 0; iBand < 4; ++iBand)
				fChannelPhases[iBand] = startPhase;
	}

	// Apply tremolo to each band
	void processBands(int channel, float band[4], float depth[4], int choice[4], float offset, float pulseWidth, int depthMode)
	{
		for (int iBand = 0; iBand < 4; ++iBand)
		{
			fPhasePos[channel][iBand] += fPhaseInc[iBand];
			if (fPhasePos[channel][iBand] > DOUBLE_PI)
				fPhasePos[channel][iBand] -= DOUBLE_PI;

			float fPhase = fPhasePos[channel][iBand] + offset;
			while (fPhase > DOUBLE_PI)
				fPhase -= DOUBLE_PI;

			float fOsc = 0.0f;
			if (choice[iBand] == 0)
				sineProcess(fPhase, fOsc);
			else if (choice[iBand] == 1)
				triangleProcess(fPhase, fOsc);
			else if (choice[iBand] == 2)
				sawtoothProcess(fPhase, fOsc);
			else if (choice[iBand] == 3)
				pulseProcess(fPhase, pulseWidth, fOsc);
			else if (choice[iBand] == 4)
				squareProcess(fPhase, fOsc);

			float fTrem = 1.0f;
			if (depthMode == 0)
			{
				const float fUnipolar = (fOsc + 1.0f) * 0.5f;
				fTrem = (1.0f - depth[iBand]) + (depth[iBand] * fUnipolar);
			}
			else
			{
				fTrem = std::max(0.0f, 1.0f + (fOsc * depth[iBand]));
			}

			band[iBand] *= fTrem;
		}
	}

	// Apply tremolo to this channel using effective values
	void processChannelBands(int channel, float band[4], float depth[4], int choice[4], float rate[4], float sampleRate, float offset, float pulseWidth, int depthMode)
	{
		computePhaseIncrements(rate, sampleRate);
		processBands(channel, band, depth, choice, offset, pulseWidth, depthMode);
	}

private:
	std::vector<std::array<float, 4>> fPhasePos;
	float fPhaseInc[4];
};

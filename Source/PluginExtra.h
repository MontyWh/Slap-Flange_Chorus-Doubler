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

class TremoloProcess
{
public:
    TremoloProcess()
    {
        for (int b = 0; b < 4; ++b)
        {
            fPhasePos[b] = 0.0f;
            fPhaseInc[b] = 0.0f;
        }
    }

    // Reset phase positions (called in prepareToPlay)
    void reset()
    {
        for (int b = 0; b < 4; ++b)
            fPhasePos[b] = 0.0f;
    }

    // Compute phase increments for this block
    void computePhaseIncrements(float fRate[4], float fSampleRate)
    {
        const float fTwoPI = static_cast<float>(2.0f * M_PI);

        for (int b = 0; b < 4; ++b)
            fPhaseInc[b] = (fTwoPI * fRate[b]) / fSampleRate;
    }

    void sineProcess(int b, const float twoPI, float& osc)
    {
        if (fPhasePos[b] > twoPI)
            fPhasePos[b] -= twoPI;

        osc = std::sin(fPhasePos[b]);
    }

    void triangleProcess(int b, const float twoPI, float& osc)
    {
        if (0 <= fPhasePos[b] && fPhasePos[b] < M_PI)
            osc = -1.0f + (2.0f / M_PI) * fPhasePos[b];
        else if (M_PI <= fPhasePos[b] && fPhasePos[b] < 2 * M_PI)
            osc = 3.0f - (2.0f / M_PI) * fPhasePos[b];
    }

    void sawtoothProcess(int b, const float twoPI, float& osc)
    {
        if (fPhasePos[b] > twoPI)
            fPhasePos[b] -= twoPI;

        osc = (fPhasePos[b] / M_PI) - 1.0f;
    }

    void pulseProcess(int b, const float fTwoPI, float& osc)
    {
        if (fPhasePos[b] > fTwoPI)
            fPhasePos[b] -= fTwoPI;

        const float p = fPhasePos[b];

		const float pulseWidth = 0.5f; // 50% duty cycle

        osc = (p < pulseWidth * fTwoPI) ? 1.0f : -1.0f;
    }


	void squareProcess(int b, const float fTwoPI, float& osc)
	{
        osc = (fPhasePos[b] < M_PI) ? 1.0f : -1.0f;
	}

    // Apply tremolo to each band
    void processBands(float band[4], float depth[4], int choice[4])
    {
        const float fTwoPI = static_cast<float>(2.0f * M_PI);

        for (int b = 0; b < 4; ++b)
        {
            fPhasePos[b] += fPhaseInc[b];
            float osc;
            if (choice[b] == 0)
                sineProcess(b, fTwoPI, osc);
            else if (choice[b] == 1)
                triangleProcess(b, fTwoPI, osc);
            else if (choice[b] == 2)
                sawtoothProcess(b, fTwoPI, osc);
            else if (choice[b] == 3)
                pulseProcess(b, fTwoPI, osc);
            else if (choice[b] == 4)
                squareProcess(b, fTwoPI, osc);


            float trem = (osc * depth[b] * 0.5f) + 0.5f;

            band[b] *= trem;
        }
    }

private:
    float fPhasePos[4];
    float fPhaseInc[4];
};
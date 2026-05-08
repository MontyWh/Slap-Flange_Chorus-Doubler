//
//  EffectExtra.h
//  Additional Plugin Code
//
//  This file is a workspace for developing new DSP objects or functions to use in your plugin.
//

#include <cmath>
#include <corecrt_math_defines.h>
#include <iostream>
#include <algorithm>


// Primary distortion types

// Function to rectify a float value
float rectify(float value) {
    return (value < 0) ? -value : value;
}

float SoftClip(float fInput, float control)
{
    float fOutput = (2 / M_PI) * atan(fInput * control);
    return fOutput;
}

float HardClip(float fInput, float control)
{
    fInput *= control;
    if (fInput > 1) fInput = 1;
    else if (fInput < -1) fInput = -1;

    return fInput;
}

float QuantisedDistortion(float fInput, float control)
{
    // Adjust gain for distortion based on control parameter
    float adjustedGain = std::pow(10, (control * 2.5) / 20);

    // Apply distortion by scaling the fInput
    int quantised = static_cast<int>(std::round(fInput * adjustedGain));

    // Convert quantized integer back to floating-point
    float fOutput = static_cast<float>(quantised);

    // Return the distorted signal
    return fOutput;
}

float RectifiedDistortion(float fInput, float control)
{

    float fOutput = rectify(fInput * control);

    return fOutput;
}

float FoldingDistortion(float fInput, float control)
{
    fInput = HardClip(fInput, control);
    if (fInput < -1) fInput = -1 + (1 + fInput);
    else if (fInput > 1) fInput = 1 - (1 - fInput);

    return fInput;
}

float AsymmetricDistortion(float fInput, float control)
{
    if (fInput < 0) fInput = SoftClip(fInput, control);
    else if (fInput > 0) fInput = HardClip(fInput, control);

    return fInput;
}

float ParabolicDistortion(float fInput, float control)
{
    if (fInput < 0) fInput = 0 - pow(HardClip(fInput, control), 2);
    else if (fInput > 0) fInput = pow(HardClip(fInput, control), 2);

    if (fInput < 0) fInput = fInput;
    else if (fInput > 0) fInput = fInput;
    else fInput = 0;

    return fInput;
}

float QuarterCircleDistortion(float fInput, float control)
{
    fInput *= control;
    float radius = 1.0;

    // Edit: Added std::max to ensure we never calculate sqrt of a negative number
    float fOutput = sqrt(std::max(0.0f, radius - pow(fInput, 2.0f)));

    return fOutput;
}


// Lo-Fi distortion types

float TangentDistortion(float fInput, float control)
{
    // Use a non-linear function like tanh for soft clipping
    float fOutput = std::tanh(fInput * control);


    return fOutput;
}

float AliasingDistortion(float fInput, float control, float numOfSamples, float fSR)
{
    // "Time-quantising" aka 'aliasing'

    fInput *= control;

    static int sampleCounter = 0; // Keeps track of the iSample count
    static float lastOutput = 0.0; // Holds the last fOutput iSample for aliasing effect

    // Calculate the downsampling factor based on the control parameter
    int downsampleFactor = static_cast<int>(fSR / numOfSamples) + 1;

    // Downsample by only updating fOutput on every Nth iSample, according to downsampleFactor
    if (sampleCounter % downsampleFactor == 0) {
        lastOutput = fInput;
    }

    // Increment iSample counter and wrap it around to prevent overflow
    sampleCounter++;
    if (sampleCounter >= downsampleFactor) sampleCounter = 0;

    return lastOutput;
}

float PhaseDistortion(float fInput, float control, float numOfSamples, double dSR)
{
    // Static phase accumulator to maintain state across calls
    static float phaseAccumulator = 0.0f;

    float phaseShift = control * 0.1f;
    float phaseIncrement = 2.0f * M_PI * phaseShift / dSR;

    // Apply distortion based on the current phase state
    float fOutput = fInput * std::cos(phaseAccumulator);

    // Increment phase for the NEXT sample
    phaseAccumulator += phaseIncrement;

    // Wrap phase to keep it within a reasonable range (0 to 2PI)
    if (phaseAccumulator >= 2.0f * M_PI) phaseAccumulator -= 2.0f * M_PI;

    return fOutput;
}

float AlterBitDepth(float fInput, float control)
{
    // Number of bits to reduce by
    float maxDepth = 24 * (1 - std::pow(control, 1.0 / 4.0));

    // Calculate the altered iSample value
    float alteredSample = round((fInput + 1.0) * maxDepth) / (maxDepth + 1.0);

    return alteredSample;
}

float VinylCrackle(float fInput, float numOfSamples, float control, int counter, float fSR)
{
    int randomNumber = rand() % 200 + 1;     // in the range 1 to x

    float fInputDip = fInput * 0.5;

    if (numOfSamples == randomNumber)
    {
        if (randomNumber == 0)
            fInput = 0 + (randomNumber / 100);
        else if (randomNumber == 1)
            fInput = HardClip(fInput * (1.0 + control), 1.0);
        else if (randomNumber == 2)
        {
            fInput = AlterBitDepth(fInputDip, control);
        }
        else if (randomNumber == 3)
        {
            fInput = AliasingDistortion(fInputDip, control, numOfSamples, fSR);
        }
        else if (randomNumber == 4)
        {
            fInput = PhaseDistortion(fInputDip, control, numOfSamples, fSR);
        }
    }
    fInput = AlterBitDepth(fInput, 0.0625 * control);

    return fInput;
}

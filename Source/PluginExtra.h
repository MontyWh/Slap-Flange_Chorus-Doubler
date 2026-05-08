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

float SoftClip(float dInput, float control)
{
    float dOutput = (2 / M_PI) * atan(dInput * control);
    return dOutput;
}

float HardClip(float dInput, float control)
{
    dInput *= control;
    if (dInput > 1) dInput = 1;
    else if (dInput < -1) dInput = -1;

    return dInput;
}

float QuantisedDistortion(float dInput, float control)
{
    // Adjust gain for distortion based on control parameter
    float adjustedGain = std::pow(10, (control * 2.5) / 20);

    // Apply distortion by scaling the dInput
    int quantised = static_cast<int>(std::round(dInput * adjustedGain));

    // Convert quantized integer back to floating-point
    float dOutput = static_cast<float>(quantised);

    // Return the distorted signal
    return dOutput;
}

float RectifiedDistortion(float dInput, float control)
{

    float dOutput = rectify(dInput * control);

    return dOutput;
}

float FoldingDistortion(float dInput, float control)
{
    dInput = HardClip(dInput, control);
    if (dInput < -1) dInput = -1 + (1 + dInput);
    else if (dInput > 1) dInput = 1 - (1 - dInput);

    return dInput;
}

float AsymmetricDistortion(float dInput, float control)
{
    if (dInput < 0) dInput = SoftClip(dInput, control);
    else if (dInput > 0) dInput = HardClip(dInput, control);

    return dInput;
}

float ParabolicDistortion(float dInput, float control)
{
    if (dInput < 0) dInput = 0 - pow(HardClip(dInput, control), 2);
    else if (dInput > 0) dInput = pow(HardClip(dInput, control), 2);

    if (dInput < 0) dInput = dInput;
    else if (dInput > 0) dInput = dInput;
    else dInput = 0;

    return dInput;
}

float QuarterCircleDistortion(float dInput, float control)
{
    dInput *= control;
    float radius = 1.0;

    float dOutput = sqrt(std::max(0.0, radius - pow(dInput, 2)));

    return dOutput;
}


// Lo-Fi distortion types

float TangentDistortion(float dInput, float control)
{
    // Use a non-linear function like tanh for soft clipping
    float dOutput = std::tanh(dInput * control);


    return dOutput;
}

float AliasingDistortion(float dInput, float control, float numOfSamples, float fSR)
{
    // "Time-quantising" aka 'aliasing'

    dInput *= control;

    static int sampleCounter = 0; // Keeps track of the iSample count
    static float lastOutput = 0.0; // Holds the last dOutput iSample for aliasing effect

    // Calculate the downsampling factor based on the control parameter
    int downsampleFactor = static_cast<int>(fSR / numOfSamples) + 1;

    // Downsample by only updating dOutput on every Nth iSample, according to downsampleFactor
    if (sampleCounter % downsampleFactor == 0) {
        lastOutput = dInput;
    }

    // Increment iSample counter and wrap it around to prevent overflow
    sampleCounter++;
    if (sampleCounter >= downsampleFactor) sampleCounter = 0;

    return lastOutput;
}

float PhaseDistortion(float dInput, float control, float numOfSamples, double dSR)
{
    // Static phase accumulator to maintain state across calls
    static float phaseAccumulator = 0.0f;

    float phaseShift = control * 0.1f;
    float phaseIncrement = 2.0f * M_PI * phaseShift / dSR;

    // Apply distortion based on the current phase state
    float dOutput = dInput * std::cos(phaseAccumulator);

    // Increment phase for the NEXT sample
    phaseAccumulator += phaseIncrement;

    // Wrap phase to keep it within a reasonable range (0 to 2PI)
    if (phaseAccumulator >= 2.0f * M_PI) phaseAccumulator -= 2.0f * M_PI;

    return dOutput;
}

float AlterBitDepth(float dInput, float control)
{
    // Number of bits to reduce by
    float maxDepth = 24 * (1 - std::pow(control, 1.0 / 4.0));

    // Calculate the altered iSample value
    float alteredSample = round((dInput + 1.0) * maxDepth) / (maxDepth + 1.0);

    return alteredSample;
}

float VinylCrackle(float dInput, float numOfSamples, float control, int counter, float fSR)
{
    int randomNumber = rand() % 200 + 1;     // in the range 1 to x

    float fInputDip = dInput * 0.5;

    if (numOfSamples == randomNumber)
    {
        if (randomNumber == 0)
            dInput = 0 + (randomNumber / 100);
        else if (randomNumber == 1)
            dInput = HardClip(dInput * (1.0 + control), 1.0);
        else if (randomNumber == 2)
        {
            dInput = AlterBitDepth(fInputDip, control);
        }
        else if (randomNumber == 3)
        {
            dInput = AliasingDistortion(fInputDip, control, numOfSamples, fSR);
        }
        else if (randomNumber == 4)
        {
            dInput = PhaseDistortion(fInputDip, control, numOfSamples, fSR);
        }
    }
    dInput = AlterBitDepth(dInput, 0.0625 * control);

    return dInput;
}

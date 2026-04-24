//
//  EffectExtra.h
//  Additional Plugin Code
//
//  This file is a workspace for developing new DSP objects or functions to use in your plugin.
//

#include <cmath>
#include <iostream>

// formula for pi
float pi = atan(1) * 4;


// Primary distortion types

// Function to rectify a float value
float rectify(float value) {
    return (value < 0) ? -value : value;
}

float SoftClip(float input, float control)
{
    float output = (2 / pi) * atan(input * control);
    return output;
}

float HardClip(float input, float control)
{
    input *= control;
    if (input > 1) input = 1;
    else if (input < -1) input = -1;

    return input;
}

float QuantisedDistortion(float input, float control)
{
    // Adjust gain for distortion based on control parameter
    float adjustedGain = std::pow(10, (control * 2.5) / 20);

    // Apply distortion by scaling the input
    int quantised = static_cast<int>(std::round(input * adjustedGain));

    // Convert quantized integer back to floating-point
    float output = static_cast<float>(quantised);

    // Return the distorted signal
    return output;
}

float RectifiedDistortion(float input, float control)
{

    float output = rectify(input * control);

    return output;
}

float FoldingDistortion(float input, float control)
{
    input = HardClip(input, control);
    if (input < -1) input = -1 + (1 + input);
    else if (input > 1) input = 1 - (1 - input);

    return input;
}

float AsymmetricDistortion(float input, float control)
{
    if (input < 0) input = SoftClip(input, control);
    else if (input > 0) input = HardClip(input, control);

    return input;
}

float ParabolicDistortion(float input, float control)
{
    if (input < 0) input = 0 - pow(HardClip(input, control), 2);
    else if (input > 0) input = pow(HardClip(input, control), 2);

    if (input < 0) input = input;
    else if (input > 0) input = input;
    else input = 0;

    return input;
}

float QuarterCircleDistortion(float input, float control)
{
    input *= control;
    float radius = 1.0;

    float output = sqrt(radius - pow(input, 2));

    return output;
}


// Lo-Fi distortion types

float TangentDistortion(float input, float control)
{
    // Use a non-linear function like tanh for soft clipping
    float output = std::tanh(input * control);


    return output;
}

float AliasingDistortion(float input, float control, float numOfSamples, float fSR)
{
    // "Time-quantising" aka 'aliasing'

    input *= control;

    static int sampleCounter = 0; // Keeps track of the sample count
    static float lastOutput = 0.0; // Holds the last output sample for aliasing effect

    // Calculate the downsampling factor based on the control parameter
    int downsampleFactor = static_cast<int>(fSR / numOfSamples) + 1;

    // Downsample by only updating output on every Nth sample, according to downsampleFactor
    if (sampleCounter % downsampleFactor == 0) {
        lastOutput = input;
    }

    // Increment sample counter and wrap it around to prevent overflow
    sampleCounter++;
    if (sampleCounter >= downsampleFactor) sampleCounter = 0;

    return lastOutput;
}

float PhaseDistortion(float input, float control, float numOfSamples, float fSR)
{
    // Apply phase distortion by modifying the phase of the input
    float phaseShift = control * 0.1; // Adjust the scaling factor as needed

    // Calculate the phase increment per sample
    float phaseIncrement = 2.0f * M_PI * phaseShift / fSR;

    // Initialise the output
    float output = 0.0;

    // Process each sample
    for (int sampleIndex = 0; sampleIndex < numOfSamples; ++sampleIndex)
    {
        // Compute the phase for this sample
        float phase = phaseIncrement * sampleIndex;

        // Update the output using the input and phase
        output += input * std::cos(phase);
    }

    return output;
}

float AlterBitDepth(float input, float control)
{
    // Number of bits to reduce by
    float maxDepth = 24 * (1 - std::pow(control, 1.0 / 4.0));

    // Calculate the altered sample value
    float alteredSample = round((input + 1.0) * maxDepth) / (maxDepth + 1.0);

    return alteredSample;
}

float VinylCrackle(float input, float numOfSamples, float control, int counter, float fSR)
{
    int randomNumber = rand() % 200 + 1;     // in the range 1 to x

    float fInputDip = input * 0.5;

    if (numOfSamples == randomNumber)
    {
        if (randomNumber == 0)
            input = 0 + (randomNumber / 100);
        else if (randomNumber == 1)
            input = HardClip(input * (1.0 + control), 1.0);
        else if (randomNumber == 2)
        {
            input = AlterBitDepth(fInputDip, control);
        }
        else if (randomNumber == 3)
        {
            input = AliasingDistortion(fInputDip, control, numOfSamples, fSR);
        }
        else if (randomNumber == 4)
        {
            input = PhaseDistortion(fInputDip, control, numOfSamples, fSR);
        }
    }
    input = AlterBitDepth(input, 0.0625 * control);

    return input;
}
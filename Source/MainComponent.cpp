#include "MainComponent.h"

#include <cmath>
#include <iostream>

// formula for pi
float pi = atan(1) * 4;

//==============================================================================
MainComponent::MainComponent()
{
    // Make sure you set the size of the component after
    // you add any child components.
    setSize (800, 600);

    // Some platforms require permissions to open input channels so request that here
    if (juce::RuntimePermissions::isRequired (juce::RuntimePermissions::recordAudio)
        && ! juce::RuntimePermissions::isGranted (juce::RuntimePermissions::recordAudio))
    {
        juce::RuntimePermissions::request (juce::RuntimePermissions::recordAudio,
                                           [&] (bool granted) { setAudioChannels (granted ? 2 : 0, 2); });
    }
    else
    {
        // Specify the number of input and output channels that we want to open
        setAudioChannels (2, 2);
    }
}

MainComponent::~MainComponent()
{
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();
}

//==============================================================================
float ProcessDistortion(float input, int type, float control)
{
    float inputGain = 1.0 + (pow(control, 3.0) * (25.0 - 1.0));

    // Distortion types

    if (type == 1)
    {
        input = SoftClip(input, inputGain);
    }

    else if (type == 2)
    {
        input = HardClip(input, inputGain);
    }

    else if (type == 3)
    {
        input = QuantisedDistortion(input, inputGain);
    }

    else if (type == 4)
    {
        input = RectifiedDistortion(input, inputGain);
    }

    else if (type == 5)
    {
        input = FoldingDistortion(input, inputGain);
    }

    else if (type == 6)
    {
        input = AsymmetricDistortion(input, inputGain);
    }

    else if (type == 7)
    {
        input = ParabolicDistortion(input, inputGain);
    }

    else if (type == 8)
    {
        input = QuarterCircleDistortion(input, inputGain);
    }

    return input;
}

float ProcessDistortion(float input, int type, float control)
{
    float inputGain = 1.0 + (pow(control, 3.0) * (25.0 - 1.0));

    // Distortion types

    if (type == 1)
    {
        input = SoftClip(input, inputGain);
    }

    else if (type == 2)
    {
        input = HardClip(input, inputGain);
    }

    else if (type == 3)
    {
        input = QuantisedDistortion(input, inputGain);
    }

    else if (type == 4)
    {
        input = RectifiedDistortion(input, inputGain);
    }

    else if (type == 5)
    {
        input = FoldingDistortion(input, inputGain);
    }

    else if (type == 6)
    {
        input = AsymmetricDistortion(input, inputGain);
    }

    else if (type == 7)
    {
        input = ParabolicDistortion(input, inputGain);
    }

    else if (type == 8)
    {
        input = QuarterCircleDistortion(input, inputGain);
    }

    return input;
}

//==============================================================================
float LoFiEffects(float input, int type, float control, float numOfSamples, float fSR, int counter)
{
    float output = 0;
    if (type == 1)
    {
        output = TangentDistortion(input, control);
    }
    if (type == 2)
    {
        output = AliasingDistortion(input, control, numOfSamples, fSR);
    }
    if (type == 3)
    {
        output = PhaseDistortion(input, control, numOfSamples, fSR);
    }
    if (type == 4)
    {
        output = AlterBitDepth(input, control);
    }
    if (type == 5)
    {
        output = VinylCrackle(input, numOfSamples, control, counter, fSR);
    }

    output = (output * control) + ((1.0f - control) * input);

    return output;
}

//==============================================================================
float MyEffect::NoiseGate(float monoMix, float control, float fReduction)
{
    float fThresh = control;
    bool meterCounterCondition = false;


    // Define the input range (1 to 40)
    float x = 1.0;
    float y = 40.0;

    if (meterCounterCondition == true)
    {
        // test and cycle through fThresh
        meterCounter += 0.0000025; // count up
        control = meterCounter; // display the value
        if (meterCounter > 1) meterCounter = 0; // reset the value
    }

    float fAbsolute = fabsf(monoMix);

    if (fPeak < fAbsolute) fPeak = fAbsolute; // did we see a louder peak?
    iMeasuredItems++;


    if (iMeasuredItems == iMeasuredLength)
    {
        //// Scale and offset the values of fMax0
        fPeak = x + (fPeak * (y - x));

        // Use the log10() function to calculate log values, to give you values between log10(1) to log10(40), to then scale to the range of 0 to 1 by dividing by log10(4)
        fPeak = log10(fPeak) / log10(40);

        iMeasuredItems = fPeak = 0; // reset for next time
    }

    fGateTarget = (fPeak > fThresh) ? 1 : fReduction; // should the gate open?
    if (fGateGain < fGateTarget) // the gate is opening - 'Attack'
    {
        fGateGain += 0.01;
        if (fGateGain > 1) fGateGain = 1;
    }
    if (fGateGain > fGateTarget) // the gate is closing - 'Release'
    {
        fGateGain -= 0.01;
        if (fGateGain < fReduction) fGateGain = fReduction;
    }

    return fGateGain;
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    // This function will be called when the audio device is started, or when
    // its settings (i.e. sample rate, block size, etc) are changed.

    // You can use this function to initialise any resources you might need,
    // but be careful - it will be called on the audio thread, not the GUI thread.

    // For more details, see the help for AudioProcessor::prepareToPlay()

    iMeasuredLength = sampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();

    subBass.prepare(spec);
    bassLower.prepare(spec);
    bassUpper.prepare(spec);
    midLower.prepare(spec);
    midUpper.prepare(spec);
    treble.prepare(spec);

    // Assign coefficients directly here (no helper functions)
    subBass.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 60.0f);
    bassLower.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 120.0f);
    bassUpper.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 120.0f);

    midLower.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 1000.0f);
    midUpper.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 1000.0f);

    treble.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 6000.0f);
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    // Your audio-processing code goes here!

    // For more details, see the help for AudioProcessor::getNextAudioBlock()

    // Right now we are not producing any data, in which case we need to clear the buffer
    // (to prevent the output of random noise)
    bufferToFill.clearActiveBufferRegion();
}

void MainComponent::releaseResources()
{
    // This will be called when the audio device stops, or when it is being
    // restarted due to a setting change.

    // For more details, see the help for AudioProcessor::releaseResources()
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    // You can add your drawing code here!
}

void MainComponent::resized()
{
    // This is called when the MainContentComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.
}

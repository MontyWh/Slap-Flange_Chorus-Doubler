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
float MainComponent::NoiseGate(float monoMix, float control, float fReduction)
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

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlockExpected;
    spec.numChannels = deviceManager.getCurrentAudioDevice()->getActiveOutputChannels().countNumberOfSetBits(); // Get the number of active output channels from the audio device manager


    fSampleRate = spec.sampleRate;

    float fOutGain = pow(parameters[1], 3.0);
    float fInGain = pow(parameters[0], 3.0);

    float fWetStereo[2];
    float fDryStereo[2];
    float fBand[4];
    float fTonalDistortion[4];

    float control1 = pow(parameters[7], 3);
    float control2 = pow(parameters[8], 3);
    float control3 = pow(parameters[9], 3);
    float control4 = pow(parameters[10], 3);

    subBass.setCutoff(60.0);
    bassUpper.setCutoff(250.0);
    bassLower.setCutoff(60.0);
    midUpper.setCutoff(2000.0);
    midLower.setCutoff(250.0);
    treble.setCutoff(2000.0);

    float presenceCenterFrequency = pow(parameters[12], 3.0) * (20000.0 - 1000.0) + 1000.0; // Example range: 1000 Hz to 20000 Hz
    float presenceGain = 1.41; // Approximately +3 dB
    float mixDrop = 1 - 0.41;
    resonanceFilter.setQ(presenceCenterFrequency, presenceGain);
    resonanceFilter.setGain(presenceGain);


    float fGate = pow(1 - parameters[13], 3);
    float fGainReduc = 1 - (parameters[16] * parameters[16] * parameters[16]);
    float gateFilterCutoff = 200.0 * fGate;
    fFilter.setCutoff(gateFilterCutoff); // configure with the cutoff frequency

    float fWetDryControl = pow(parameters[11], 3);

    float fLoFiblend = pow(parameters[15], 3);

    vinylCounter = 0;
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    // Your audio-processing code goes here!

    // For more details, see the help for AudioProcessor::getNextAudioBlock()

    // Right now we are not producing any data, in which case we need to clear the buffer
    // (to prevent the output of random noise)
    bufferToFill.clearActiveBufferRegion();

    /*const float* pfBuffer0 = inputBuffers[0], * pfBuffer1 = inputBuffers[1];
    float* pfOutBuffer0 = outputBuffers[0], * pfOutBuffer1 = outputBuffers[1];*/
    auto* pfBuffer = bufferToFill.buffer;
    std::vector<float*&> fIn(pfBuffer->getNumChannels());
    std::vector<float*> fOut(pfBuffer->getNumChannels());

    std::vector<float> fDry(pfBuffer->getNumChannels());
    std::vector<float> fWet(pfBuffer->getNumChannels());

    float fBand[4];
    float fToneDistort[4];

        // Get sample from input
        for (int channel = 0; channel < pfBuffer->getNumChannels(); ++channel)
        {
            fDry[channel] = fIn[channel] = pfBuffer->getWritePointer(channel);

            for (int s = 0; s < pfBuffer->getNumSamples(); ++s)
            {
                // Add your effect processing here
                // monoMix = fFilter.tick(monoMix);
                fDry[s] = fIn * fInGain * NoiseGate(fFilter.tick(fMix), fGate, fGainReduc);

                fDry[s] = resonanceFilter.tick(fDry) * mixDrop;

                for (int i = 0; i <= 1; i++) {
                    // Initialise bands for each channel
                	fBand[0] = subBass.tick(fWet[i] * control1);
                    fBand[1] = (bassUpper.tick(fWet[i] * control2) + bassLower.tick(fDry[i] * control2));
                    fBand[2] = (midUpper.tick(fWet[i] * control3) + midLower.tick(fDry[i] * control3));
                    fBand[3] = treble.tick(fWet[i] * control4);


                    // Process each band if required
                    for (int j = 0; j <= 3; j++)
                    {
                        fToneDistort[j] = ProcessDistortion(fBand[j], parameters[3 + j], parameters[2]);
                    }

                    // Sum the processed bands
                    fWet[s] = fToneDistort[0] + fToneDistort[1] + fToneDistort[2] + fToneDistort[3];
                    fWet[s] = LoFiEffects(fWet[i], parameters[14], fLoFiblend, pfBuffer->getNumSamples(), fSampleRate, vinylCounter);
                }

                // Output calculation
                fOut[s] = (fWet[s] * fWetDryControl) + (fDry[s] * (1.0 - fWetDryControl));
                // Copy result to output
                fOut[s] *= fOutGain;
        }
    }
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

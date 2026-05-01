#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
{
    // Make sure you set the size of the component after
    // you add any child components.

    parameters = juce::ValueTree("Parameters");

    // --- Rotaries / Floats ---
    parameters.setProperty("InGain", 0.0f, nullptr);
    parameters.setProperty("OutGain", 0.0f, nullptr);
    parameters.setProperty("Distortion", 0.0f, nullptr);
    parameters.setProperty("SubBass", 0.0f, nullptr);
    parameters.setProperty("Bass", 0.0f, nullptr);
    parameters.setProperty("Mid", 0.0f, nullptr);
    parameters.setProperty("Treble", 0.0f, nullptr);
    parameters.setProperty("Wet", 0.0f, nullptr);
    parameters.setProperty("Presence", 0.0f, nullptr);
    parameters.setProperty("NoiseGate", 0.0f, nullptr);
    parameters.setProperty("LofiBlend", 0.0f, nullptr);
    parameters.setProperty("GateReduction", 0.0f, nullptr);

    // --- Menus / Choices (stored as Integers) ---
    parameters.setProperty("SubBassDistortion", 0, nullptr);
    parameters.setProperty("BassDistortion", 0, nullptr);
    parameters.setProperty("MidDistortion", 0, nullptr);
    parameters.setProperty("TrebleDistortion", 0, nullptr);
    parameters.setProperty("LofiEffects", 0, nullptr);

    // The keys in the exact order of your original float array
    static const std::vector<juce::Identifier> ids = {
        "InGain", "OutGain", "Distortion",
        "SubBassDistortion", "BassDistortion", "MidDistortion", "TrebleDistortion",
        "SubBass", "Bass", "Mid", "Treble", "Wet", "Presence", "NoiseGate",
        "LofiEffects", "LofiBlend", "GateReduction"
    };

    const float presetData[][17] = {
        { 1.0f, 1.0f, 1.0f, 0, 0, 2, 1, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.75f, 1.0f, 0, 0, 1 }, // Monty Guitar
        { 1.000, 0.620, 1.000, 0, 4, 2, 1, 1.000, 1.000, 1.000, 1.000, 1.000, 1.000, 0.413, 4, 0.245, 0.861 }, // Dirty Drum Gate
        { 0.753, 1.000, 0.603, 0, 6, 1, 6, 1.000, 1.000, 1.000, 0.701, 0.607, 1.000, 1.000, 0, 0.000, 1.000 }, // EP Saturation
        { 1.000, 1.000, 1.000, 0, 0, 0, 0, 1.000, 1.000, 1.000, 1.000, 0.823, 1.000, 1.000, 4, 0.816, 0.000 }, // Bits Crushed
        { 1.000, 1.000, 1.000, 0, 0, 0, 0, 1.000, 1.000, 1.000, 1.000, 1.000, 1.000, 1.000, 2, 0.907, 0.000 }  // Oops, I broke it!
    };

    for (int i = 0; i < ids.size(); ++i)
        parameters.setProperty(ids[i], presetData[presetIndex][i], nullptr);


    setSize(800, 600);

    // Some platforms require permissions to open input channels so request that here
    if (juce::RuntimePermissions::isRequired(juce::RuntimePermissions::recordAudio)
        && !juce::RuntimePermissions::isGranted(juce::RuntimePermissions::recordAudio))
    {
        juce::RuntimePermissions::request(juce::RuntimePermissions::recordAudio,
            [&](bool granted) { setAudioChannels(granted ? 2 : 0, 2); });
    }
    else
    {
        // Specify the number of input and output channels that we want to open
        setAudioChannels(2, 2);
    }
}

MainComponent::~MainComponent()
{
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();
}

//==============================================================================
double ProcessDistortion(double input, int type, double dControl)
{
    double dInputGain = 1.0 + (pow(dControl, 3.0) * (25.0 - 1.0));

    // Distortion types

    if (type == 1)
    {
        input = SoftClip(input, dInputGain);
    }

    else if (type == 2)
    {
        input = HardClip(input, dInputGain);
    }

    else if (type == 3)
    {
        input = QuantisedDistortion(input, dInputGain);
    }

    else if (type == 4)
    {
        input = RectifiedDistortion(input, dInputGain);
    }

    else if (type == 5)
    {
        input = FoldingDistortion(input, dInputGain);
    }

    else if (type == 6)
    {
        input = AsymmetricDistortion(input, dInputGain);
    }

    else if (type == 7)
    {
        input = ParabolicDistortion(input, dInputGain);
    }

    else if (type == 8)
    {
        input = QuarterCircleDistortion(input, dInputGain);
    }

    return input;
}

//==============================================================================
double LoFiEffects(double input, int type, double dControl, double dNumOfSamples, double fSR, int counter)
{
    double dOutput = 0;
    if (type == 1)
    {
        dOutput = TangentDistortion(input, dControl);
    }
    if (type == 2)
    {
        dOutput = AliasingDistortion(input, dControl, dNumOfSamples, fSR);
    }
    if (type == 3)
    {
        dOutput = PhaseDistortion(input, dControl, dNumOfSamples, fSR);
    }
    if (type == 4)
    {
        dOutput = AlterBitDepth(input, dControl);
    }
    if (type == 5)
    {
        dOutput = VinylCrackle(input, dNumOfSamples, dControl, counter, fSR);
    }

    dOutput = (dOutput * dControl) + ((1.0 - dControl) * input);

    return dOutput;
}

//==============================================================================
double MainComponent::NoiseGate(double dMonoMix, double dControl, double dReduction)
{
    double dThresh = dControl;
    bool dMeterCounterCondition = false;


    // Define the input range (1 to 40)
    double x = 1.0;
    double y = 40.0;

    if (dMeterCounterCondition == true)
    {
        // test and cycle through dThresh
        dMeterCounter += 0.0000025; // count up
        dControl = dMeterCounter; // display the value
        if (dMeterCounter > 1) dMeterCounter = 0; // reset the value
    }

    double dAbsolute = fabs(dMonoMix);

    if (dPeak < dAbsolute) dPeak = dAbsolute; // did we see a louder peak?
    iMeasuredItems++;


    if (iMeasuredItems == iMeasuredLength)
    {
        //// Scale and offset the values of dMax0
        dPeak = x + (dPeak * (y - x));

        // Use the log10() function to calculate log values, to give you values between log10(1) to log10(40), to then scale to the range of 0 to 1 by dividing by log10(4)
        dPeak = log10(dPeak) / log10(40);

        iMeasuredItems = 0;
        dPeak = 0.0; // reset for next time
    }

    dGateTarget = (dPeak > dThresh) ? 1 : dReduction; // should the gate open?
    if (dGateGain < dGateTarget) // the gate is opening - 'Attack'
    {
        dGateGain += 0.01;
        if (dGateGain > 1) dGateGain = 1;
    }
    if (dGateGain > dGateTarget) // the gate is closing - 'Release'
    {
        dGateGain -= 0.01;
        if (dGateGain < dReduction) dGateGain = dReduction;
    }

    return dGateGain;
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


    dSampleRate = spec.sampleRate;

    double dOutGain = pow(parameters[1], 3.0);
    double dInGain = pow(parameters[0], 3.0);

    double dWetStereo[2];
    double dDryStereo[2];
    double dBand[4];
    double dTonalDistortion[4];

    double dControl1 = pow(parameters[7], 3);
    double dControl2 = pow(parameters[8], 3);
    double dControl3 = pow(parameters[9], 3);
    double dControl4 = pow(parameters[10], 3);

    subBass.prepare(spec);
	auto subBassCoeff = juce::dsp::IIR::Coefficients<double>::makeLowPass(sampleRate, 60.0);
	subBass.coefficients = subBassCoeff;

    bassUpper.prepare(spec);
    auto bassUpperCoeff = juce::dsp::IIR::Coefficients<double>::makeBandPass(sampleRate, 250.0);
    bassUpper.coefficients = bassUpperCoeff;

    bassLower.prepare(spec);
    auto bassLowerCoeff = juce::dsp::IIR::Coefficients<double>::makeBandPass(sampleRate, 60.0);
    bassLower.coefficients = bassLowerCoeff;

    midUpper.prepare(spec);
    auto midUpperCoeff = juce::dsp::IIR::Coefficients<double>::makeBandPass(sampleRate, 2000.0);
    midUpper.coefficients = midUpperCoeff;

    midLower.prepare(spec);
    auto midLowerCoeff = juce::dsp::IIR::Coefficients<double>::makeBandPass(sampleRate, 250.0);
    midLower.coefficients = midLowerCoeff;

    treble.prepare(spec);
	auto trebleCoeff = juce::dsp::IIR::Coefficients<double>::makeHighPass(sampleRate, 2000.0);
	treble.coefficients = trebleCoeff;

    double dPresenceCenterFrequency = pow(parameters[12], 3.0) * (20000.0 - 1000.0) + 1000.0; // Example range: 1000 Hz to 20000 Hz
    double dPresenceGain = 1.41; // Approximately +3 dB
    double dMixDrop = 1 - 0.41;
    resonanceFilter.prepare(spec);
	auto resonanceCoeff = juce::dsp::IIR::Coefficients<double>::makePeakFilter(sampleRate, dPresenceCenterFrequency, 1.0, dPresenceGain);
    resonanceFilter.coefficients = resonanceCoeff;

    double dGate = pow(1 - parameters[13], 3);
    double dGainReduc = 1 - (parameters[16] * parameters[16] * parameters[16]);
    double dGateFilterCutoff = 200.0 * dGate;
    dFilter.setCutoff(dGateFilterCutoff); // configure with the cutoff frequency

    double dWetDryControl = pow(parameters[11], 3);

    double dLoFiblend = pow(parameters[15], 3);

    dVinylCounter = 0;
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    // Your audio-processing code goes here!

    // For more details, see the help for AudioProcessor::getNextAudioBlock()

    // Right now we are not producing any data, in which case we need to clear the buffer
    // (to prevent the output of random noise)
    bufferToFill.clearActiveBufferRegion();

    auto* pdBuffer = bufferToFill.buffer;
    std::vector<double*&> dIn(pdBuffer->getNumChannels());
    std::vector<double*> dOut(pdBuffer->getNumChannels());

    std::vector<double> dDry(pdBuffer->getNumChannels());
    std::vector<double> dWet(pdBuffer->getNumChannels());

    double dBand[4];
    double dToneDistort[4];

        // Get sample from input
        for (int channel = 0; channel < pdBuffer->getNumChannels(); ++channel)
        {
            dDry[channel] = dIn[channel] = pdBuffer->getWritePointer(channel);

            for (int s = 0; s < pdBuffer->getNumSamples(); ++s)
            {
                // Add your effect processing here
                // monoMix = fFilter.tick(monoMix);
                dDry[s] = dIn * dInGain * NoiseGate(dFilter.tick(dMix), dGate, dGainReduc);

                dDry[s] = resonanceFilter.tick(dDry) * dMixDrop;

                for (int i = 0; i <= 1; i++) {
                    // Initialise bands for each channel
                	dBand[0] = subBass.tick(dWet[i] * dControl1);
                    dBand[1] = (bassUpper.tick(dWet[i] * dControl2) + bassLower.tick(dDry[i] * dControl2));
                    dBand[2] = (midUpper.tick(dWet[i] * dControl3) + midLower.tick(dDry[i] * dControl3));
                    dBand[3] = treble.tick(dWet[i] * dControl4);


                    // Process each band if required
                    for (int j = 0; j <= 3; j++)
                    {
                        dToneDistort[j] = ProcessDistortion(dBand[j], parameters[3 + j], parameters[2]);
                    }

                    // Sum the processed bands
                    dWet[s] = dToneDistort[0] + dToneDistort[1] + dToneDistort[2] + dToneDistort[3];
                    dWet[s] = LoFiEffects(dWet[i], parameters[14], dLoFiblend, pdBuffer->getNumSamples(), dSampleRate, dVinylCounter);
                }

                // Output calculation
                dOut[s] = (dWet[s] * dWetDryControl) + (dDry[s] * (1.0 - dWetDryControl));
                // Copy result to output
                dOut[s] *= dOutGain;
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

/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
#include "PluginExtra.h"

//==============================================================================
DistortionPlusAudioProcessor::DistortionPlusAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("dInput",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("dOutput", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ), apvts(*this /*processor*/, nullptr, "Parameters", createParameters())
#endif
{
    initPresets();
}

DistortionPlusAudioProcessor::~DistortionPlusAudioProcessor()
{
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout DistortionPlusAudioProcessor::createParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Setup Menus
    juce::StringArray distortOptions = { "Clean", "Soft Clip", "Hard Clip", "Quantised", "Rectified", "Folded", "Asymmetric", "Parabolic (expander)", "Quarter-Circle" };
    juce::StringArray loFiOptions = { "None", "Tangent-distortion", "Aliasing", "Phase distortion", "Alter bit depth", "Vinyl crackle" };

    params.push_back(std::make_unique<juce::AudioParameterChoice>("SUB_DISTORT", "Sub-Bass Distortion", distortOptions, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("BASS_DISTORT", "Bass Distortion", distortOptions, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("MID_DISTORT", "Mid Distortion", distortOptions, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("TREBLE_DISTORT", "Treble Distortion", distortOptions, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("LO-FI_TYPE", "Lo-Fi effects", loFiOptions, 0));

    // Setup Rotary Controls
    params.push_back(std::make_unique<juce::AudioParameterFloat>("INPUT_GAIN", "dInput Gain", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("OUTPUT_GAIN", "dOutput Gain", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("DISTORTION", "Distortion", 0.0f, 1.0f, 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("SUB-BASS", "Sub-Bass", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("BASS", "Bass", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("MID", "Mid", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("TREBLE", "Treble", 0.0f, 1.0f, 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("WET", "Wet", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("PRESENCE", "Presence", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("NOISE_GATE", "Noise Gate", 0.0f, 1.0f, 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("LO-FI_BLEND", "Lo-Fi Blend", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("GATE_REDUCTION", "Gate Reduction", 0.0f, 1.0f, 0.0f));

    return { params.begin(), params.end() };
}

void DistortionPlusAudioProcessor::initPresets()
{
    // Match your original APDI preset list
    presets = {
        { "Monty Guitar",      { 0, 0, 0, 0, 0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 0.75, 1.0 } },
        { "Dirty Drum Gate",   { 0, 4, 2, 1, 4, 1.0, 0.62, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 0.413 } },
        { "EP Saturation",     { 0, 6, 1, 6, 0, 0.75, 1.0, 0.6, 1.0, 1.0, 1.0, 0.7, 0.6, 1.0, 1.0, 1.0, 0.0 } }
    };
}

void DistortionPlusAudioProcessor::loadPreset(int index)
{
    if (index < 0 || index >= presets.size()) return;
    auto& preset = presets[index];

    // The order here must match the order in createParameters()
    // It's safer to loop through parameters by ID if you have many
    const char* paramIDs[] = {
        "SUB_DISTORT", "BASS_DISTORT", "MID_DISTORT", "TREBLE_DISTORT", "LO-FI_TYPE",
        "INPUT_GAIN", "OUTPUT_GAIN", "DISTORTION", "SUB-BASS", "BASS", "MID", "TREBLE",
        "WET", "PRESENCE", "NOISE_GATE", "LO-FI_BLEND", "GATE_REDUCTION"
    };

    for (int i = 0; i < preset.values.size(); ++i)
    {
        if (auto* param = apvts.getParameter(paramIDs[i]))
            param->setValueNotifyingHost(param->convertTo0to1(preset.values[i]));
    }
}

//==============================================================================
const juce::String DistortionPlusAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool DistortionPlusAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool DistortionPlusAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool DistortionPlusAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double DistortionPlusAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int DistortionPlusAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int DistortionPlusAudioProcessor::getCurrentProgram()
{
    return 0;
}

void DistortionPlusAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String DistortionPlusAudioProcessor::getProgramName (int index)
{
    return {};
}

void DistortionPlusAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

float ProcessDistortion(float dInput, int type, float control)
{
    float inputGain = 1.0 + (pow(control, 3.0) * (25.0 - 1.0));

    // Distortion types

    if (type == 0)
    {
		return dInput; // Clean signal, no distortion
	}

    if (type == 1)
    {
        dInput = SoftClip(dInput, inputGain);
    }

    else if (type == 2)
    {
        dInput = HardClip(dInput, inputGain);
    }

    else if (type == 3)
    {
        dInput = QuantisedDistortion(dInput, inputGain);
    }

    else if (type == 4)
    {
        dInput = RectifiedDistortion(dInput, inputGain);
    }

    else if (type == 5)
    {
        dInput = FoldingDistortion(dInput, inputGain);
    }

    else if (type == 6)
    {
        dInput = AsymmetricDistortion(dInput, inputGain);
    }

    else if (type == 7)
    {
        dInput = ParabolicDistortion(dInput, inputGain);
    }

    else if (type == 8)
    {
        dInput = QuarterCircleDistortion(dInput, inputGain);
    }

    return dInput;
}


float LoFiEffects(float dInput, int type, float control, float numOfSamples, float fSR, int counter)
{
	if (type == 0) return dInput; // No Lo-Fi effect, return clean signal
	
	float dOutput = 0;
    if (type == 1)
    {
        dOutput = TangentDistortion(dInput, control);
    }
    if (type == 2)
    {
        dOutput = AliasingDistortion(dInput, control, numOfSamples, fSR);
    }
    if (type == 3)
    {
        dOutput = PhaseDistortion(dInput, control, numOfSamples, fSR);
    }
    if (type == 4)
    {
        dOutput = AlterBitDepth(dInput, control);
    }
    if (type == 5)
    {
        dOutput = VinylCrackle(dInput, numOfSamples, control, counter, fSR);
    }

    dOutput = (dOutput * control) + ((1.0f - control) * dInput);

    return dOutput;
}


float DistortionPlusAudioProcessor::NoiseGate(float monoMix, float control, float fReduction)
{
    float fThresh = control;
    bool meterCounterCondition = false;


    // Define the dInput range (1 to 40)
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
        fGateGain += 0.01f;
        if (fGateGain > 1) fGateGain = 1;
    }
    if (fGateGain > fGateTarget) // the gate is closing - 'Release'
    {
        fGateGain -= 0.01f;
        if (fGateGain < fReduction) fGateGain = fReduction;
    }

    return fGateGain;
}

//==============================================================================
void DistortionPlusAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need...

    int numChannels = getTotalNumInputChannels();
    dSampleRate = sampleRate;

    // Initialise the measurement length (e.g., 10ms window)
    iMeasuredLength = static_cast<int>(sampleRate * 0.01);
    iMeasuredItems = 0;
    fPeak = 0;
    fGateGain = 1.0f; // Start with gate open to avoid a "pop" on start

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 1; // Each individual filter in the array is mono

    // Clear and re-fill the arrays based on channel count
    subBass.clear();
    bassLower.clear();
	bassUpper.clear();
	midLower.clear();
    midUpper.clear();
	treble.clear();
	resonanceFilter.clear();

    for (int i = 0; i < numChannels; ++i)
    {
        subBass.add(new Filter());
        subBass[i]->prepare(spec);

        bassLower.add(new Filter());
        bassLower[i]->prepare(spec);

        bassUpper.add(new Filter());
        bassUpper[i]->prepare(spec);

        midLower.add(new Filter());
        midLower[i]->prepare(spec);

        midUpper.add(new Filter());
        midUpper[i]->prepare(spec);

        treble.add(new Filter());
        treble[i]->prepare(spec);

        resonanceFilter.add(new Filter());
        resonanceFilter[i]->prepare(spec);

        gateFilter.add(new Filter());
        gateFilter[i]->prepare(spec);
    }
}

void DistortionPlusAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool DistortionPlusAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the dInput layout matches the dOutput layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void DistortionPlusAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Fetch Parameter Values (Outside the loop for efficiency)
    float fInGain = std::pow(*apvts.getRawParameterValue("INPUT_GAIN"), 3.0f);
    float fOutGain = std::pow(*apvts.getRawParameterValue("OUTPUT_GAIN"), 3.0f);
    float fDistAmt = *apvts.getRawParameterValue("DISTORTION");

    float fSubBassGain = std::pow(*apvts.getRawParameterValue("SUB-BASS"), 3.0f);
    float fBassGain = std::pow(*apvts.getRawParameterValue("BASS"), 3.0f);
    float fMidGain = std::pow(*apvts.getRawParameterValue("MID"), 3.0f);
    float fTrebleGain = std::pow(*apvts.getRawParameterValue("TREBLE"), 3.0f);

    float fWetDryControl = std::pow(*apvts.getRawParameterValue("WET"), 3.0f);
    float fPresence = *apvts.getRawParameterValue("PRESENCE");
    float fGate = std::pow(1.0f - *apvts.getRawParameterValue("NOISE_GATE"), 3.0f);
    float fLoFiBlend = std::pow(*apvts.getRawParameterValue("LO-FI_BLEND"), 3.0f);
    float fGainReduce = 1.0f - std::pow(*apvts.getRawParameterValue("GATE_REDUCTION"), 3.0f);

    int iSubDistortType = (int)*apvts.getRawParameterValue("SUB_DISTORT");
    int iBassDistType = (int)*apvts.getRawParameterValue("BASS_DISTORT");
    int iMidDistType = (int)*apvts.getRawParameterValue("MID_DISTORT");
    int iTrebleDistType = (int)*apvts.getRawParameterValue("TREBLE_DISTORT");
    int iLoFiType = (int)*apvts.getRawParameterValue("LO-FI_TYPE");

    int iDistortTypes[4] = { iSubDistortType, iBassDistType, iMidDistType, iTrebleDistType };

    // Update Filter Coefficients
    auto subBassCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(dSampleRate, 60.0f);
    auto bassLowerCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(dSampleRate, 60.0f);
    auto bassUpperCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(dSampleRate, 250.0f);
    auto midLowerCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(dSampleRate, 250.0f);
    auto midUpperCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(dSampleRate, 2000.0f);
    auto trebleCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(dSampleRate, 2000.0f);

    float presenceFreq = fPresence * (20000.0f - 1000.0f) + 1000.0f;
    auto resonanceCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(dSampleRate, presenceFreq, 1.41f, 1.41f);
    auto gateCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(dSampleRate, 200.0f * fGate);

    // Apply coefficients to every channel instance
    for (int i = 0; i < totalNumInputChannels; ++i)
    {
        subBass[i]->coefficients = subBassCoeffs;
        bassLower[i]->coefficients = bassLowerCoeffs;
        bassUpper[i]->coefficients = bassUpperCoeffs;
        midLower[i]->coefficients = midLowerCoeffs;
        midUpper[i]->coefficients = midUpperCoeffs;
        treble[i]->coefficients = trebleCoeffs;
        resonanceFilter[i]->coefficients = resonanceCoeffs;
        gateFilter[i]->coefficients = gateCoeffs;
    }

    float fMixDrop = 1.0f - 0.41f;

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);

        for (int iSample = 0; iSample < buffer.getNumSamples(); ++iSample)
        {
            // Get iSample from dInput
            float dInput = channelData[iSample];

            // Use dInput consistently
            float gatedInput = gateFilter[channel]->processSample(dInput);
            double dGatedGain = NoiseGate(gatedInput, fGate, fGainReduce);

            double dDry = dInput * fInGain * dGatedGain;
            double dWet = resonanceFilter[channel]->processSample(dDry) * fMixDrop;

            // Band Splitting
            double dBand[4];
            dBand[0] = subBass[channel]->processSample(dWet * fSubBassGain);

            // Use the channel index for the rest:
            dBand[1] = (bassUpper[channel]->processSample(dWet * fBassGain) +
                bassLower[channel]->processSample(dDry * fBassGain));
            dBand[2] = (midUpper[channel]->processSample(dWet * fMidGain) +
                midLower[channel]->processSample(dDry * fMidGain));
            dBand[3] = treble[channel]->processSample(dWet * fTrebleGain);

            // Tonal Distortion
            double dSummedBands = 0;
            for (int j = 0; j < 4; ++j)
            {
                dSummedBands += ProcessDistortion(dBand[j], iDistortTypes[j], fDistAmt);
            }

            // Lo-Fi Effects
            double dProcessedWet = LoFiEffects(dSummedBands, iLoFiType, fLoFiBlend, (float)iSample, (float)dSampleRate, iVinylCounter);

            // Increment and wrap the counter (so it doesn't overflow)
            iVinylCounter++;
            if (iVinylCounter >= dSampleRate) iVinylCounter = 0;

            // dOutput calculation (Wet/Dry mix)
            double dOutput = (dProcessedWet * fWetDryControl) + (dDry * (1.0f - fWetDryControl));

            channelData[iSample] = dOutput * fOutGain;
        }
    }
}

//==============================================================================
bool DistortionPlusAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* DistortionPlusAudioProcessor::createEditor()
{
    return new DistortionPlusAudioProcessorEditor (*this);
}

//==============================================================================
void DistortionPlusAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.

    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void DistortionPlusAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.

    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr)
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DistortionPlusAudioProcessor();
}
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
                       .withInput  ("fInput",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("fOutput", juce::AudioChannelSet::stereo(), true)
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

    juce::StringArray distortOptions = { "Clean", "Soft Clip", "Hard Clip", "Quantised", "Rectified", "Folded", "Asymmetric", "Parabolic (expander)", "Quarter-Circle" };
    juce::StringArray loFiOptions = { "None", "Tangent-distortion", "Aliasing", "Phase distortion", "Alter bit depth", "Vinyl crackle" };

    params.push_back(std::make_unique<juce::AudioParameterChoice>("SUB_DISTORT", "Sub-Bass Distortion", distortOptions, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("BASS_DISTORT", "Bass Distortion", distortOptions, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("MID_DISTORT", "Mid Distortion", distortOptions, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("TREBLE_DISTORT", "Treble Distortion", distortOptions, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("LO-FI_TYPE", "Lo-Fi effects", loFiOptions, 0));

    // Edits: Changed initial values from 0.0f to 1.0f or 0.5f to ensure the plugin is audible on load
    params.push_back(std::make_unique<juce::AudioParameterFloat>("INPUT_GAIN", "fInput Gain", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("OUTPUT_GAIN", "fOutput Gain", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("DISTORTION", "Distortion", 0.0f, 1.0f, 0.1f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("SUB-BASS", "Sub-Bass", 0.0f, 1.0f, 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("BASS", "Bass", 0.0f, 1.0f, 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("MID", "Mid", 0.0f, 1.0f, 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("TREBLE", "Treble", 0.0f, 1.0f, 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("WET", "Wet", 0.0f, 1.0f, 1.0f));
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

float ProcessDistortion(float input, int type, float control)
{
    float fInputGain = 1.0 + (pow(control, 3.0) * (25.0 - 1.0));

    // Distortion types

    if (type == 0)
    {
		return input; // Clean signal, no distortion
	}

    if (type == 1)
    {
        input = SoftClip(input, fInputGain);
    }

    else if (type == 2)
    {
        input = HardClip(input, fInputGain);
    }

    else if (type == 3)
    {
        input = QuantisedDistortion(input, fInputGain);
    }

    else if (type == 4)
    {
        input = RectifiedDistortion(input, fInputGain);
    }

    else if (type == 5)
    {
        input = FoldingDistortion(input, fInputGain);
    }

    else if (type == 6)
    {
        input = AsymmetricDistortion(input, fInputGain);
    }

    else if (type == 7)
    {
        input = ParabolicDistortion(input, fInputGain);
    }

    else if (type == 8)
    {
        input = QuarterCircleDistortion(input, fInputGain);
    }

    return input;
}


float LoFiEffects(float input, int type, float control, float numOfSamples, float fSR, int counter)
{
	if (type == 0) return input; // No Lo-Fi effect, return clean signal
	
	float fOutput = 0;
    if (type == 1)
    {
        fOutput = TangentDistortion(input, control);
    }
    if (type == 2)
    {
        fOutput = AliasingDistortion(input, control, numOfSamples, fSR);
    }
    if (type == 3)
    {
        fOutput = PhaseDistortion(input, control, numOfSamples, fSR);
    }
    if (type == 4)
    {
        fOutput = AlterBitDepth(input, control);
    }
    if (type == 5)
    {
        fOutput = VinylCrackle(input, numOfSamples, control, counter, fSR);
    }

    fOutput = (fOutput * control) + ((1.0f - control) * input);

    return fOutput;
}


float DistortionPlusAudioProcessor::NoiseGate(float input, float control, float reduction)
{
    float fThresh = control;
    bool meterCounterCondition = false;


    // Define the fInput range (1 to 40)
    float x = 1.0;
    float y = 40.0;

    if (meterCounterCondition == true)
    {
        // test and cycle through fThresh
        meterCounter += 0.0000025; // count up
        control = meterCounter; // display the value
        if (meterCounter > 1) meterCounter = 0; // reset the value
    }

    float fAbsolute = fabsf(input);

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

    fGateTarget = (fPeak > fThresh) ? 1 : reduction; // should the gate open?
    if (fGateGain < fGateTarget) // the gate is opening - 'Attack'
    {
        fGateGain += 0.01f;
        if (fGateGain > 1) fGateGain = 1;
    }
    if (fGateGain > fGateTarget) // the gate is closing - 'Release'
    {
        fGateGain -= 0.01f;
        if (fGateGain < reduction) fGateGain = reduction;
    }

    return fGateGain;
}

//==============================================================================
void DistortionPlusAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need...

    int numChannels = getTotalNumInputChannels();
    fSampleRate = static_cast<float>(sampleRate);

    // Initialise the measurement length (e.g., 10ms window)
    iMeasuredLength = static_cast<int>(sampleRate * 0.01f);
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

    // This checks if the fInput layout matches the fOutput layout
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

    // Fetch values using local variables to avoid multiple pointer dereferences in the loop
    float fInGain = std::pow(*apvts.getRawParameterValue("INPUT_GAIN"), 3.0f);
    float fOutGain = std::pow(*apvts.getRawParameterValue("OUTPUT_GAIN"), 3.0f);
    float fDistortAmt = *apvts.getRawParameterValue("DISTORTION");

    float fSubBassGain = std::pow(*apvts.getRawParameterValue("SUB-BASS"), 3.0f);
    float fBassGain = std::pow(*apvts.getRawParameterValue("BASS"), 3.0f);
    float fMidGain = std::pow(*apvts.getRawParameterValue("MID"), 3.0f);
    float fTrebleGain = std::pow(*apvts.getRawParameterValue("TREBLE"), 3.0f);

    float fWetDryControl = std::pow(*apvts.getRawParameterValue("WET"), 3.0f);
    float fPresence = *apvts.getRawParameterValue("PRESENCE");
    float fGate = std::pow(1.0f - *apvts.getRawParameterValue("NOISE_GATE"), 3.0f);
    float fLoFiBlend = std::pow(*apvts.getRawParameterValue("LO-FI_BLEND"), 3.0f);
    float fGainReduce = 1.0f - std::pow(*apvts.getRawParameterValue("GATE_REDUCTION"), 3.0f);

    int iDistortTypes[4] = {
        *apvts.getRawParameterValue("SUB_DISTORT"),
        *apvts.getRawParameterValue("BASS_DISTORT"),
        *apvts.getRawParameterValue("MID_DISTORT"),
        *apvts.getRawParameterValue("TREBLE_DISTORT")
    };
    int iLoFiType = (int)*apvts.getRawParameterValue("LO-FI_TYPE");

    // Update Filter Coefficients (using double for calculation accuracy)
    float presenceFreq = fPresence * (19000.0f) + 1000.0f;
    auto resonanceCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(fSampleRate, presenceFreq, 1.41f, 1.41f);
    auto gateCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(fSampleRate, std::max(20.0f, 20000.0f * fGate));

    // Define the static cutoff frequencies for your crossover
    auto subCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(fSampleRate, 60.0f);
    auto bassLCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(fSampleRate, 250.0f);
    auto bassUCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(fSampleRate, 60.0f);
    auto midLCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(fSampleRate, 2000.0f);
    auto midUCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(fSampleRate, 250.0f);
    auto trebCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(fSampleRate, 2000.0f);
	
	for (int i = 0; i < totalNumInputChannels; ++i)
    {
        resonanceFilter[i]->coefficients = resonanceCoeffs;
        gateFilter[i]->coefficients = gateCoeffs;

        subBass[i]->coefficients = subCoeffs;
        bassLower[i]->coefficients = bassLCoeffs;
        bassUpper[i]->coefficients = bassUCoeffs;
        midLower[i]->coefficients = midLCoeffs;
        midUpper[i]->coefficients = midUCoeffs;
        treble[i]->coefficients = trebCoeffs;
    }

    float fMixDrop = 1.0f - 0.41f;

    // This is the place where you'd normally do the guts of your plugin's audio processing...
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);

        for (int iSample = 0; iSample < buffer.getNumSamples(); ++iSample)
        {
            // Use float for the audio signal to match the buffer's datatype
            float fInput = channelData[iSample];

            // 1. Dynamics stage (filtering the sidechain for the gate)
            float fGatedSidechain = gateFilter[channel]->processSample(fInput);
            float fGatedGain = NoiseGate(fGatedSidechain, fGate, fGainReduce);

            // 2. Pre-gain and Dry path calculation
            float fDry = fInput * fInGain * fGatedGain;
            float fWet = resonanceFilter[channel]->processSample(fDry) * fMixDrop;

            // 3. Multiband splitting logic
            float fBand[4];
            fBand[0] = subBass[channel]->processSample(fWet * fSubBassGain);
            fBand[1] = (bassUpper[channel]->processSample(fWet * fBassGain) + bassLower[channel]->processSample(fDry * fBassGain));
            fBand[2] = (midUpper[channel]->processSample(fWet * fMidGain) + midLower[channel]->processSample(fDry * fMidGain));
            fBand[3] = treble[channel]->processSample(fWet * fTrebleGain);

            // 4. Summing the processed bands
            float fSummedBands = 0;
            for (int j = 0; j < 4; ++j)
                fSummedBands += ProcessDistortion(fBand[j], iDistortTypes[j], fDistortAmt);

            // 5. Final Lo-Fi character stage
            float fProcessWet = LoFiEffects(fSummedBands, iLoFiType, fLoFiBlend, iSample, fSampleRate, iVinylCounter);

            iVinylCounter = (iVinylCounter + 1) % (int)fSampleRate;

            // 6. Wet/Dry crossfade and Output Gain
            float fOutput = (fProcessWet * fWetDryControl) + (fDry * (1.0f - fWetDryControl));
            channelData[iSample] = fOutput * fOutGain;
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
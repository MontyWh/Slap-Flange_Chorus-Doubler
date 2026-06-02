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
AutophonicAudioProcessor::AutophonicAudioProcessor()
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

AutophonicAudioProcessor::~AutophonicAudioProcessor()
{
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout AutophonicAudioProcessor::createParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Removed all distortion option arrays

    // Edits: Changed initial values from 0.0f to 1.0f or 0.5f to ensure the plugin is audible on load
    params.push_back(std::make_unique<juce::AudioParameterFloat>("INPUT_GAIN", "fInput Gain", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("OUTPUT_GAIN", "fOutput Gain", 0.0f, 1.0f, 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("SUB-BASS", "Sub-Bass", 0.0f, 1.0f, 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("BASS", "Bass", 0.0f, 1.0f, 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("MID", "Mid", 0.0f, 1.0f, 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("TREBLE", "Treble", 0.0f, 1.0f, 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("WET", "Wet", 0.0f, 1.0f, 1.0f));

    return { params.begin(), params.end() };
}

void AutophonicAudioProcessor::initPresets()
{
    // Match your original APDI preset list (updated to match new parameter count)
    presets = {
        { "Preset 1",      { 0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f } },
        { "Preset 2",   { 0.5f, 0.62f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f } },
        { "Preset 3",     { 0.75f, 1.0f, 0.7f, 0.6f, 1.0f, 1.0f, 1.0f } }
    };
}

void AutophonicAudioProcessor::loadPreset(int index)
{
    if (index < 0 || index >= presets.size()) return;
    auto& preset = presets[index];

    // The order here must match the order in createParameters()
    // It's safer to loop through parameters by ID if you have many
    const char* paramIDs[] = {
        "INPUT_GAIN", "OUTPUT_GAIN", "SUB-BASS", "BASS", "MID", "TREBLE", "WET"
    };

    for (int i = 0; i < preset.values.size(); ++i)
    {
        if (auto* param = apvts.getParameter(paramIDs[i]))
            param->setValueNotifyingHost(param->convertTo0to1(preset.values[i]));
    }
}

//==============================================================================
const juce::String AutophonicAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AutophonicAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool AutophonicAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool AutophonicAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double AutophonicAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AutophonicAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int AutophonicAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AutophonicAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String AutophonicAudioProcessor::getProgramName (int index)
{
    return {};
}

void AutophonicAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void AutophonicAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
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

void AutophonicAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool AutophonicAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void AutophonicAudioProcessor::processFilters(float fSubBassGain, float fBassGain, float fMidGain, float fTrebleGain, int channel, float fDry, float fWet)
{
	// 2. Multiband splitting logic
	float fBand[4];
	fBand[0] = subBass[channel]->processSample(fWet * fSubBassGain);
	fBand[1] = (bassUpper[channel]->processSample(fWet * fBassGain) + bassLower[channel]->processSample(fDry * fBassGain));
	fBand[2] = (midUpper[channel]->processSample(fWet * fMidGain) + midLower[channel]->processSample(fDry * fMidGain));
	fBand[3] = treble[channel]->processSample(fWet * fTrebleGain);

	// 3. Summing the processed bands
	fWet = 0;
	for (int j = 0; j < 4; ++j)
		fWet += fBand[j];
}

void AutophonicAudioProcessor::additionalProcess(float fSubBassGain, float fBassGain, float fMidGain, float fTrebleGain, float fMixDrop, int channel, float fWet, float& fDry)
{
	resonanceFilter[channel]->processSample(fDry);
	fDry *= fMixDrop;
	processFilters(fSubBassGain, fBassGain, fMidGain, fTrebleGain, channel, fDry, fWet);
}

void AutophonicAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();

    // Fetch values using local variables to avoid multiple pointer dereferences in the loop
    float fInGain = std::pow(*apvts.getRawParameterValue("INPUT_GAIN"), 3.0f);
    float fOutGain = std::pow(*apvts.getRawParameterValue("OUTPUT_GAIN"), 3.0f);

    float fWetDryControl = std::pow(*apvts.getRawParameterValue("WET"), 3.0f);

    float fSubBassGain = std::pow(*apvts.getRawParameterValue("SUB-BASS"), 3.0f);
    float fBassGain = std::pow(*apvts.getRawParameterValue("BASS"), 3.0f);
    float fMidGain = std::pow(*apvts.getRawParameterValue("MID"), 3.0f);
    float fTrebleGain = std::pow(*apvts.getRawParameterValue("TREBLE"), 3.0f);

    // Update Filter Coefficients (using double for calculation accuracy)
    auto resonanceCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(fSampleRate, 10000.0f, 1.41f, 1.41f);
    auto gateCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(fSampleRate, 20000.0f);

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

            // 1. Pre-Output-Gain
            float fWet = fInput * fInGain, fDry = fInput * fInGain;



            additionalProcess(fSubBassGain, fBassGain, fMidGain, fTrebleGain, fMixDrop, channel, fWet, fDry);

            // 2. Wet/Dry crossfade and Output Gain
            float fOutput = (fWet * fWetDryControl) + (fDry * (1.0f - fWetDryControl));
            channelData[iSample] = fOutput * fOutGain;
        }
    }
}
//==============================================================================
bool AutophonicAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AutophonicAudioProcessor::createEditor()
{
    return new AutophonicAudioProcessorEditor (*this);
}

//==============================================================================
void AutophonicAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.

    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void AutophonicAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
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
    return new AutophonicAudioProcessor();
}
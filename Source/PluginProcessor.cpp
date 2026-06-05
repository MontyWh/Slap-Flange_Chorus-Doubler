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
AutoTremolandoAudioProcessor::AutoTremolandoAudioProcessor()
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

AutoTremolandoAudioProcessor::~AutoTremolandoAudioProcessor()
{
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout AutoTremolandoAudioProcessor::createParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // For now, only expose the Sine tremolo type. The ComboBox menus are
    // intentionally kept in the UI as placeholders for a custom waveform
    // system to be implemented later.
    juce::StringArray tremoloOptions = { "Sine" };

    params.push_back(std::make_unique<juce::AudioParameterChoice>("SUB-BASS_WAVE_TYPE", "Sub-Bass Wave Type", tremoloOptions, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("BASS_WAVE_TYPE", "Bass Wave Type", tremoloOptions, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("MID_WAVE_TYPE", "Mid Wave Type", tremoloOptions, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("TREBLE_WAVE_TYPE", "Treble Wave Type", tremoloOptions, 0));

    // Edits: Changed initial values from 0.0f to 1.0f or 0.5f to ensure the plugin is audible on load
    params.push_back(std::make_unique<juce::AudioParameterFloat>("INPUT_GAIN", "fInput Gain", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("OUTPUT_GAIN", "fOutput Gain", 0.0f, 1.0f, 0.5f));
	params.push_back(std::make_unique<juce::AudioParameterFloat>("WET", "Wet/Dry", 0.0f, 1.0f, 0.5f));

	params.push_back(std::make_unique<juce::AudioParameterFloat>("SUB-BASS_DEPTH", "Sub-Bass Depth", 0.0f, 1.0f, 1.0f));
	params.push_back(std::make_unique<juce::AudioParameterFloat>("SUB-BASS_RATE", "Sub-Bass Rate", 1.0f, 15.0f, 5.0f));
	params.push_back(std::make_unique<juce::AudioParameterFloat>("BASS_DEPTH", "Bass Depth", 0.0f, 1.0f, 1.0f));
	params.push_back(std::make_unique<juce::AudioParameterFloat>("BASS_RATE", "Bass Rate", 1.0f, 15.0f, 5.0f));
	params.push_back(std::make_unique<juce::AudioParameterFloat>("MID_DEPTH", "Mid Depth", 0.0f, 1.0f, 1.0f));
	params.push_back(std::make_unique<juce::AudioParameterFloat>("MID_RATE", "Mid Rate", 1.0f, 15.0f, 5.0f));
	params.push_back(std::make_unique<juce::AudioParameterFloat>("TREBLE_DEPTH", "Treble Depth", 0.0f, 1.0f, 1.0f));
	params.push_back(std::make_unique<juce::AudioParameterFloat>("TREBLE_RATE", "Treble Rate", 1.0f, 15.0f, 5.0f));

    return { params.begin(), params.end() };
}

void AutoTremolandoAudioProcessor::initPresets()
{
    // Match your original APDI preset list (updated to match new parameter count)
    presets = {
        { "Preset 1",      { 0, 0, 0, 0, 0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 5.0f, 0.5f, 1.0f, 0.0f } },
        { "Preset 2",   { 0, 0, 0, 0, 0.5f, 0.62f, 1.0f, 1.0f, 1.0f, 1.0f, 8.0f, 0.6f, 1.0f, 0.0f } },
        { "Preset 3",     { 0, 0, 0, 0, 0.75f, 1.0f, 0.7f, 0.6f, 1.0f, 1.0f, 3.0f, 0.7f, 1.0f, 0.0f } }
    };
}

void AutoTremolandoAudioProcessor::loadPreset(int index)
{
    if (index < 0 || index >= presets.size()) return;
    auto& preset = presets[index];

    // The order here must match the order in createParameters()
    // It's safer to loop through parameters by ID if you have many
    const char* paramIDs[] = {
        "SUB-BASS_WAVE_TYPE", "BASS_WAVE_TYPE", "MID_WAVE_TYPE", "TREBLE_WAVE_TYPE",
        "INPUT_GAIN", "OUTPUT_GAIN", "SUB-BASS_DEPTH", "SUB-BASS_RATE", "BASS_DEPTH", "BASS_RATE", "MID_DEPTH", "MID_RATE", "TREBLE_DEPTH", "TREBLE_RATE"
    };

    for (int i = 0; i < preset.values.size(); ++i)
    {
        if (auto* param = apvts.getParameter(paramIDs[i]))
            param->setValueNotifyingHost(param->convertTo0to1(preset.values[i]));
    }
}

//==============================================================================
const juce::String AutoTremolandoAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AutoTremolandoAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool AutoTremolandoAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool AutoTremolandoAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double AutoTremolandoAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AutoTremolandoAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int AutoTremolandoAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AutoTremolandoAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String AutoTremolandoAudioProcessor::getProgramName (int index)
{
    return {};
}

void AutoTremolandoAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void AutoTremolandoAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need...

    int numChannels = getTotalNumInputChannels();
    fSampleRate = static_cast<float>(sampleRate);

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
    }
}

void AutoTremolandoAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool AutoTremolandoAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

//==============================================================================
// Tremolo waveform generators
float AutoTremolandoAudioProcessor::setTremoloShape(float phase, int type, float depth)
{
    // Always use a sine oscillator for now. The `type` parameter is kept
    // for future waveform system but is ignored here so the output is
    // consistently a sine-based tremolo.
    float osc = std::sin(phase);
    return (osc * depth * 0.5f) + 0.5f; // Convert to 0-1 range
}

void AutoTremolandoAudioProcessor::processFilters(float* depths, int channel, float fDry, float& fWet, float* fPhases, int* iTremTypes)
{
    // Multiband splitting logic with per-band tremolo
    float fBand[4];
    float fTremolo[4];

    // Calculate the distinct LFO modulator value for each band using its unique phase position and depth
    for (int i = 0; i < 4; ++i)
    {
        fTremolo[i] = setTremoloShape(fPhases[i], iTremTypes[i], depths[i]);
    }

    fBand[0] = subBass[channel]->processSample(fWet * fTremolo[0]);
    fBand[1] = (bassUpper[channel]->processSample(fWet * fTremolo[1]) + bassLower[channel]->processSample(fDry * fTremolo[1]));
    fBand[2] = (midUpper[channel]->processSample(fWet * fTremolo[2]) + midLower[channel]->processSample(fDry * fTremolo[2]));
    fBand[3] = treble[channel]->processSample(fWet * fTremolo[3]);

    // Summing the processed bands back together
    fWet = 0.0f;
    for (int j = 0; j < 4; ++j)
        fWet += fBand[j];
}

void AutoTremolandoAudioProcessor::additionalProcess(float* depths, float fMixDrop, int channel, float& fWet, float fDry, float* fPhases, int* iTremTypes)
{
    resonanceFilter[channel]->processSample(fDry);
    fDry *= fMixDrop;
    processFilters(depths, channel, fDry, fWet, fPhases, iTremTypes);
}

void AutoTremolandoAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();

    // Fetch values using local variables to avoid multiple pointer dereferences in the loop
    float fInGain = std::pow(*apvts.getRawParameterValue("INPUT_GAIN"), 3.0f);
    float fOutGain = std::pow(*apvts.getRawParameterValue("OUTPUT_GAIN"), 3.0f);
    float fWetDryControl = std::pow(*apvts.getRawParameterValue("WET"), 3.0f);

    float fRate[4] = {
        std::pow(*apvts.getRawParameterValue("SUB-BASS_RATE"), 3.0f),
        std::pow(*apvts.getRawParameterValue("BASS_RATE"), 3.0f),
        std::pow(*apvts.getRawParameterValue("MID_RATE"), 3.0f),
        std::pow(*apvts.getRawParameterValue("TREBLE_RATE"), 3.0f)
    };
    float fDepth[4] = {
        std::pow(*apvts.getRawParameterValue("SUB-BASS_DEPTH"), 3.0f),
        std::pow(*apvts.getRawParameterValue("BASS_DEPTH"), 3.0f),
        std::pow(*apvts.getRawParameterValue("MID_DEPTH"), 3.0f),
        std::pow(*apvts.getRawParameterValue("TREBLE_DEPTH"), 3.0f)
    };

    // Update Filter Coefficients
    float presenceFreq = 5000.0f; // Temporary fallback to prevent crashing on the missing 'PRESENCE' parameter
    auto resonanceCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(fSampleRate, presenceFreq, 1.41f, 1.41f);

    auto subCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(fSampleRate, 60.0f);
    auto bassLCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(fSampleRate, 250.0f);
    auto bassUCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(fSampleRate, 60.0f);
    auto midLCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(fSampleRate, 2000.0f);
    auto midUCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(fSampleRate, 250.0f);
    auto trebCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(fSampleRate, 2000.0f);

    for (int i = 0; i < totalNumInputChannels; ++i)
    {
        resonanceFilter[i]->coefficients = resonanceCoeffs;
        subBass[i]->coefficients = subCoeffs;
        bassLower[i]->coefficients = bassLCoeffs;
        bassUpper[i]->coefficients = bassUCoeffs;
        midLower[i]->coefficients = midLCoeffs;
        midUpper[i]->coefficients = midUCoeffs;
        treble[i]->coefficients = trebCoeffs;
    }

    float fMixDrop = 1.0f - 0.41f;

    // Fetch tremolo types (Safely static casted to int for your array checks)
    int iTremTypes[4] = {
        static_cast<int>(*apvts.getRawParameterValue("SUB-BASS_WAVE_TYPE")),
        static_cast<int>(*apvts.getRawParameterValue("BASS_WAVE_TYPE")),
        static_cast<int>(*apvts.getRawParameterValue("MID_WAVE_TYPE")),
        static_cast<int>(*apvts.getRawParameterValue("TREBLE_WAVE_TYPE"))
    };

    constexpr float fTwoPI = static_cast<float> (2.0f * M_PI);
    float fPhaseInc[4];
    for (int i = 0; i < 4; i++) fPhaseInc[i] = (fTwoPI * fRate[i]) / fSampleRate; // small steps

    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);

        for (int iSample = 0; iSample < buffer.getNumSamples(); ++iSample)
        {
            float fInput = channelData[iSample];

            // 1. Pre-Output-Gain
            float fDry = fInput * fInGain;
            float fWet = fDry;

            // Increment LFO phases outside the band loop, only once per sample frame
            if (channel == 0)
            {
                for (int i = 0; i < 4; i++)
                {
                    fPhasePos[i] += fPhaseInc[i]; // Move our oscillator forward
                    if (fPhasePos[i] > fTwoPI) fPhasePos[i] -= fTwoPI; // Wrap around safely
                }
            }

            // 2 & 3. Process multiband filters passing down the complete rate/depth arrays
            additionalProcess(fDepth, fMixDrop, channel, fWet, fDry, fPhasePos, iTremTypes);

            // 4. Wet/Dry crossfade and Output Gain
            float fOutput = (fWet * fWetDryControl) + (fDry * (1.0f - fWetDryControl));
            channelData[iSample] = fOutput * fOutGain;
        }
    }
}

//==============================================================================
bool AutoTremolandoAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AutoTremolandoAudioProcessor::createEditor()
{
    return new AutoTremolandoAudioProcessorEditor (*this);
}

//==============================================================================
void AutoTremolandoAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.

    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void AutoTremolandoAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
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
    return new AutoTremolandoAudioProcessor();
}
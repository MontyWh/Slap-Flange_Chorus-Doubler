/*
  ==============================================================================

	Plugin: AutoTremolando
    GitHub: MontyWh
    Author: Montague Whishaw
	Date/Time: 24th April 2026
	General Language: English (UK)
	
	This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <algorithm>
#include <cmath>

//==============================================================================
AutoTremolandoAudioProcessor::AutoTremolandoAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ), apvts(*this, nullptr, "Parameters", createParameters()
					   )
#endif
{
    initPresets();
}

AutoTremolandoAudioProcessor::~AutoTremolandoAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout AutoTremolandoAudioProcessor::createParameters()
{
	std::vector<std::unique_ptr<juce::RangedAudioParameter>> params; // Vector to hold the parameters

	juce::StringArray sTremoloOptions = { "Sine", "Triangle", "Sawtooth", "Pulse", "Square" }; // Options for the tremolo waveform selection

    params.push_back(std::make_unique<juce::AudioParameterChoice>("SUB_TREMOLO",    "Sub Tremolo",    sTremoloOptions, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("BASS_TREMOLO",   "Bass Tremolo",   sTremoloOptions, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("MID_TREMOLO",    "Mid Tremolo",    sTremoloOptions, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("TREBLE_TREMOLO", "Treble Tremolo", sTremoloOptions, 0));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("INPUT_GAIN",  "Input Gain",  0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("OUTPUT_GAIN", "Output Gain", 0.0f, 1.0f, 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("SUB_TREM_RATE",    "Sub Rate",    0.5f, 16.0f, 5.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("BASS_TREM_RATE",   "Bass Rate",   0.5f, 16.0f, 5.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("MID_TREM_RATE",    "Mid Rate",    0.5f, 16.0f, 5.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("TREBLE_TREM_RATE", "Treble Rate", 0.5f, 16.0f, 5.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("MASTER_RATE", "Master Rate", 0.5f, 2.0f, 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("SUB_TREM_DEPTH",    "Sub Depth",    0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("BASS_TREM_DEPTH",   "Bass Depth",   0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("MID_TREM_DEPTH",    "Mid Depth",    0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("TREBLE_TREM_DEPTH", "Treble Depth", 0.0f, 1.0f, 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("WET",          "Wet",          0.0f,   1.0f, 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("PRESENCE",     "Presence",     0.0f,   1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("PHASE_OFFSET", "Phase Offset", 0.0f, 180.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("START_PHASE",  "Start Phase",  0.0f, 360.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("RATE_OFFSET",  "Rate Offset",  -7.0f,  7.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("DEPTH_OFFSET", "Depth Offset", -1.0f,  1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("PULSE_WIDTH",  "Pulse Width",  0.05f, 0.95f, 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterBool>("RATE_LOCK",         "Rate Lock",         false));
    params.push_back(std::make_unique<juce::AudioParameterBool>("RETRIGGER_ON_PLAY", "Retrigger On Play", true));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("SURROUND_WIDTH",   "Surround Width", 0.0f, 1.0f, 1.0f));

	juce::StringArray sDepthModes = { "Unipolar", "Bipolar" };	// Unipolar represents a depth range of 0 to 1,
																// while Bipolar represents a depth range of -1 to 1
    params.push_back(std::make_unique<juce::AudioParameterChoice>("DEPTH_MODE", "Depth Mode", sDepthModes, 0));

    params.push_back(std::make_unique<juce::AudioParameterBool>("BYPASS", "Bypass", false));

    params.push_back(std::make_unique<juce::AudioParameterBool>("TEMPO_SYNC", "Tempo Sync", false));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("SUB_NOTE_DIV",    "Sub Note Div",    0.0f, 14.0f, 9.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("BASS_NOTE_DIV",   "Bass Note Div",   0.0f, 14.0f, 9.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("MID_NOTE_DIV",    "Mid Note Div",    0.0f, 14.0f, 9.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("TREBLE_NOTE_DIV", "Treble Note Div", 0.0f, 14.0f, 9.0f));

    return { params.begin(), params.end() };
}

const std::array<const char*, AutoTremolandoAudioProcessor::iBandCount>& AutoTremolandoAudioProcessor::getRateParamIds()
{
    static const std::array<const char*, iBandCount> sIds = {
        "SUB_TREM_RATE", "BASS_TREM_RATE", "MID_TREM_RATE", "TREBLE_TREM_RATE"
    };
    return sIds;
}

const std::array<const char*, AutoTremolandoAudioProcessor::iBandCount>& AutoTremolandoAudioProcessor::getDepthParamIds()
{
    static const std::array<const char*, iBandCount> sIds = {
        "SUB_TREM_DEPTH", "BASS_TREM_DEPTH", "MID_TREM_DEPTH", "TREBLE_TREM_DEPTH"
    };
    return sIds;
}

const std::array<const char*, AutoTremolandoAudioProcessor::iBandCount>& AutoTremolandoAudioProcessor::getChoiceParamIds()
{
    static const std::array<const char*, iBandCount> sIds = {
        "SUB_TREMOLO", "BASS_TREMOLO", "MID_TREMOLO", "TREBLE_TREMOLO"
    };
    return sIds;
}

const std::array<const char*, AutoTremolandoAudioProcessor::iBandCount>& AutoTremolandoAudioProcessor::getNoteDivisionParamIds()
{
    static const std::array<const char*, iBandCount> sIds = {
        "SUB_NOTE_DIV", "BASS_NOTE_DIV", "MID_NOTE_DIV", "TREBLE_NOTE_DIV"
    };
    return sIds;
}

void AutoTremolandoAudioProcessor::initPresets()
{
    presets = {
        { "Preset 1", {
            0.5f, 0.0f,
            0, 0, 0, 0,
            1.0f,
            5.0f, 5.0f, 5.0f, 5.0f,
            0.5f, 0.5f, 0.5f, 0.5f,
            0.0f, 0.0f, 0.0f, 0.0f, 0.5f,
            0.0f, 1.0f, 1.0f, 0.0f,
            1.0f, 0.5f, 0.0f
        }},
        { "Preset 2", {
            0.5f, 0.0f,
            0, 0, 0, 0,
            1.0f,
            5.0f, 5.0f, 5.0f, 5.0f,
            0.5f, 0.5f, 0.5f, 0.5f,
            0.0f, 0.0f, 0.0f, 0.0f, 0.5f,
            0.0f, 1.0f, 1.0f, 0.0f,
            1.0f, 0.5f, 0.0f
        }},
        { "Preset 3", {
            0.5f, 0.0f,
            0, 0, 0, 0,
            1.0f,
            5.0f, 5.0f, 5.0f, 5.0f,
            0.5f, 0.5f, 0.5f, 0.5f,
            0.0f, 0.0f, 0.0f, 0.0f, 0.5f,
            0.0f, 1.0f, 1.0f, 0.0f,
            1.0f, 0.5f, 0.0f
        }}
    };
}

void AutoTremolandoAudioProcessor::loadPreset(int index)
{
	const auto& preset = presets[static_cast<size_t>(index)]; // Get the preset at the specified index

    const char* sParamIds[] = {
        "INPUT_GAIN", "PRESENCE",
        "SUB_TREMOLO", "BASS_TREMOLO", "MID_TREMOLO", "TREBLE_TREMOLO",
        "MASTER_RATE",
        "SUB_TREM_RATE", "BASS_TREM_RATE", "MID_TREM_RATE", "TREBLE_TREM_RATE",
        "SUB_TREM_DEPTH", "BASS_TREM_DEPTH", "MID_TREM_DEPTH", "TREBLE_TREM_DEPTH",
        "PHASE_OFFSET", "START_PHASE", "RATE_OFFSET", "DEPTH_OFFSET", "PULSE_WIDTH",
        "RATE_LOCK", "RETRIGGER_ON_PLAY", "SURROUND_WIDTH", "DEPTH_MODE",
        "WET", "OUTPUT_GAIN", "BYPASS"
    };

	for (int i = 0; i < static_cast<int>(std::size(sParamIds)); ++i) // Iterate through the parameter IDs
		if (auto* param = apvts.getParameter(sParamIds[i]))
			param->setValueNotifyingHost(param->convertTo0to1(preset.fValues[static_cast<size_t>(i)]));
}

void AutoTremolandoAudioProcessor::registerTapTempo()
{
	const double dNowMs = juce::Time::getMillisecondCounterHiRes(); // Get the current time in milliseconds
	const double dPreviousTapMs = dLastTapTimeMs.exchange(dNowMs); // Atomically exchange the last tap time with the current time
	if (dPreviousTapMs <= 0.0) // If this is the first tap, just store the time and
		return;

	const double dDeltaMs = dNowMs - dPreviousTapMs; // Calculate the time difference between the current tap and the previous tap
	if (dDeltaMs < 120.0 || dDeltaMs > 2000.0) // Ignore taps that are too close or too far apart
		return;

	const float fNewBpm = static_cast<float>(60000.0 / dDeltaMs); // Calculate the new BPM based on the time difference
	fTapTempoBpm.store(juce::jlimit(40.0f, 240.0f, fNewBpm)); // Store the new BPM, clamped between 40 and 240
}

void AutoTremolandoAudioProcessor::resetParametersToDefaults()
{
    for (auto* baseParam : getParameters()) // Iterate through all parameters
		if (auto* rangedParam = dynamic_cast<juce::RangedAudioParameter*>(baseParam)) // Check if the parameter is a ranged parameter
			rangedParam->setValueNotifyingHost(rangedParam->getDefaultValue()); // Reset the parameter to its default value and notify the host
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
    juce::ignoreUnused (index);
}

const juce::String AutoTremolandoAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void AutoTremolandoAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void AutoTremolandoAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
	// Use this method as the place to do any pre-playback
	// initialisation that you need..
	int iNumInputChannels = getTotalNumInputChannels();
	int iNumOutputChannels = getTotalNumOutputChannels();

	fSampleRate = static_cast<float>(sampleRate);

	juce::dsp::ProcessSpec spec; // Create a ProcessSpec object to hold the processing specifications
	spec.sampleRate = sampleRate;
	spec.maximumBlockSize = samplesPerBlock;
	spec.numChannels = 1;

	subBass.clear();
	bassLower.clear();
	bassUpper.clear();
	midLower.clear();
	midUpper.clear();
	treble.clear();
	resonanceFilter.clear();

	for (int iChannel = 0; iChannel < iNumInputChannels; ++iChannel)
	{
		subBass.add(new juce::dsp::IIR::Filter<float>);      subBass[iChannel]->prepare(spec);
		bassLower.add(new juce::dsp::IIR::Filter<float>);    bassLower[iChannel]->prepare(spec);
		bassUpper.add(new juce::dsp::IIR::Filter<float>);    bassUpper[iChannel]->prepare(spec);
		midLower.add(new juce::dsp::IIR::Filter<float>());     midLower[iChannel]->prepare(spec);
		midUpper.add(new juce::dsp::IIR::Filter<float>());     midUpper[iChannel]->prepare(spec);
		treble.add(new juce::dsp::IIR::Filter<float>());       treble[iChannel]->prepare(spec);
		resonanceFilter.add(new juce::dsp::IIR::Filter<float>()); resonanceFilter[iChannel]->prepare(spec);
	}

	mod.initialisePhaseState(mod.fPhaseOffset,
		mod.fPhasePos,
		iNumInputChannels,
		iNumOutputChannels,
		*apvts.getRawParameterValue("START_PHASE")); // Initialise the phase state for mod

	bWasPlaying = false;
	fInputMeterLevel.store(0.0f); // Reset the input meter level to 0.0f
	fOutputMeterLevel.store(0.0f); // Reset the output meter level to 0.0f
}

void AutoTremolandoAudioProcessor::releaseResources()
{
	// When playback stops, you can use this as an opportunity to free up any
	// spare memory, etc.

	subBass.clear();
	bassLower.clear();
	bassUpper.clear();
	midLower.clear();
	midUpper.clear();
	treble.clear();
	resonanceFilter.clear();

	mod.fPhaseOffset.clear();
	mod.fPhasePos.clear();

	fInputMeterLevel.store(0.0f);
	fOutputMeterLevel.store(0.0f);
	bWasPlaying = false;
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool AutoTremolandoAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
	juce::ignoreUnused (layouts);
	return true;
  #else
	if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
	 && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
		return false;

   #if ! JucePlugin_IsSynth
	if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
		return false;
   #endif

	return true;
  #endif
}
#endif

//==============================================================================
void AutoTremolandoAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
	juce::ignoreUnused (midiMessages);
	juce::ScopedNoDenormals noDenormals;
	auto iTotalNumInputChannels = getTotalNumInputChannels();
	auto iTotalNumOutputChannels = getTotalNumOutputChannels();

	// This is the place where you'd normally do the guts of your plugin's
	// audio processing...
	// Make sure to reset the state if your inner loop is processing
	// the samples and the outer loop is handling the channels.
	// Alternatively, you can process the samples with the channels
	// interleaved by keeping the same state.
	if (iTotalNumInputChannels <= 0)
		return;

	const bool bBypass = *apvts.getRawParameterValue("BYPASS") > 0.5f;
	const bool bTempoSync = *apvts.getRawParameterValue("TEMPO_SYNC") > 0.5f;
	const bool bRateLock = *apvts.getRawParameterValue("RATE_LOCK") > 0.5f;
	const bool bRetriggerOnPlay = *apvts.getRawParameterValue("RETRIGGER_ON_PLAY") > 0.5f;
	const float fSurroundWidth = juce::jlimit(0.0f, 1.0f, apvts.getRawParameterValue("SURROUND_WIDTH")->load());
	const int iDepthMode = static_cast<int>(*apvts.getRawParameterValue("DEPTH_MODE")); // Cast the depth mode parameter to an int

	const float fInGain = std::pow(*apvts.getRawParameterValue("INPUT_GAIN"), 3.0f);
	const float fOutGain = std::pow(*apvts.getRawParameterValue("OUTPUT_GAIN"), 3.0f);
	const float fWetDryControl = std::pow(*apvts.getRawParameterValue("WET"), 3.0f);
	const float fPresence = *apvts.getRawParameterValue("PRESENCE");
	const float fPhaseOffsetDegrees = *apvts.getRawParameterValue("PHASE_OFFSET");
	const float fStartPhaseDegrees = *apvts.getRawParameterValue("START_PHASE");
	const float fRateOffset = *apvts.getRawParameterValue("RATE_OFFSET");
	const float fDepthOffset = *apvts.getRawParameterValue("DEPTH_OFFSET");
	const float fPulseWidth = *apvts.getRawParameterValue("PULSE_WIDTH");
	const float fMasterRate = *apvts.getRawParameterValue("MASTER_RATE");
	const float fBypassMix = bBypass ? 1.0f : 0.0f;

	std::array<float, iBandCount> fRate{}; // Array to hold the rate values for each band
	std::array<float, iBandCount> fDepth{}; // Array to hold the depth values for each band
	for (int iBand = 0; iBand < iBandCount; ++iBand)
		fRate[static_cast<size_t>(iBand)] = *apvts.getRawParameterValue(getRateParamIds()[static_cast<size_t>(iBand)]); // Get the rate parameter for each band

	float fSyncBpm = fTapTempoBpm.load(); // Load the current tap tempo BPM
	bool bIsPlaying = false; // Flag to indicate if the plugin host's transport is currently playing
	if (auto* ph = getPlayHead()) // Get the plugin host's playhead
	{
		if (auto pos = ph->getPosition()) // Get the current position of the playhead
		{
			if (auto optBpm = pos->getBpm()) // Get the current BPM from the playhead position
				fSyncBpm = static_cast<float>(*optBpm); // Update the sync BPM if available. Cast to float for consistency with other BPM values
			bIsPlaying = pos->getIsPlaying(); // Update the playing state
		}
	}

	if (bTempoSync && bRetriggerOnPlay && bIsPlaying && !bWasPlaying) // If tempo sync is enabled, retrigger on play is enabled, the host is playing, and it wasn't playing before
		mod.retriggerPhases(mod.fPhasePos, fStartPhaseDegrees); // Retrigger the phases to the start phase
	bWasPlaying = bIsPlaying; // Update the previous playing state for the next block

	if (bTempoSync) // If tempo sync is enabled, apply the tempo sync to the rate values
	{
		std::array<int, iBandCount> iDivIdx; // Array to hold the note division indices for each band
		for (int iBand = 0; iBand < iBandCount; ++iBand)
			iDivIdx[static_cast<size_t>(iBand)] = static_cast<int>(*apvts.getRawParameterValue(getNoteDivisionParamIds()[static_cast<size_t>(iBand)])); // Get the note division parameter for each band

		mod.processRates(fRate, iDivIdx, fSyncBpm, true, bRateLock); // Apply tempo sync and optionally rate lock
	}
	else if (bRateLock)
	{
		std::array<int, iBandCount> iDivIdx{}; // Dummy, not used
		mod.processRates(fRate, iDivIdx, fSyncBpm, false, true); // Apply only rate lock
	}

	for (int iBand = 0; iBand < iBandCount; ++iBand)
	{
		float fScaledRate = fRate[static_cast<size_t>(iBand)] * fMasterRate; // Scale the rate value by the master rate
		fScaledRate = (16.0f * fScaledRate) / (16.0f + fScaledRate); // Apply a non-linear scaling to the rate value to keep it within a reasonable range
		if (fScaledRate < 0.5f)
			fScaledRate = 0.5f;

		fRate[static_cast<size_t>(iBand)] = fScaledRate; // Update the rate value for the band with the scaled value
		fDepth[static_cast<size_t>(iBand)] = *apvts.getRawParameterValue(getDepthParamIds()[static_cast<size_t>(iBand)]); // Get the depth parameter for each band
	}

	FilterCoefficients coeffs;
	const float fPresenceFreq = (fPresence * 19000.0f) + 1000.0f;
	coeffs.resonance = juce::dsp::IIR::Coefficients<float>::makePeakFilter(fSampleRate, fPresenceFreq, 1.41f, 1.41f); // Create a peak filter for the presence control with a Q factor of 1.41
	coeffs.sub = juce::dsp::IIR::Coefficients<float>::makeLowPass(fSampleRate, 60.0f);
	coeffs.bassLower = juce::dsp::IIR::Coefficients<float>::makeLowPass(fSampleRate, 250.0f);
	coeffs.bassUpper = juce::dsp::IIR::Coefficients<float>::makeHighPass(fSampleRate, 60.0f);
	coeffs.midLower = juce::dsp::IIR::Coefficients<float>::makeLowPass(fSampleRate, 2000.0f);
	coeffs.midUpper = juce::dsp::IIR::Coefficients<float>::makeHighPass(fSampleRate, 250.0f);
	coeffs.treble = juce::dsp::IIR::Coefficients<float>::makeHighPass(fSampleRate, 2000.0f);

	for (int iChannel = 0; iChannel < iTotalNumInputChannels; ++iChannel)
	{
		resonanceFilter[iChannel]->coefficients = coeffs.resonance;
		subBass[iChannel]->coefficients = coeffs.sub;
		bassLower[iChannel]->coefficients = coeffs.bassLower;
		bassUpper[iChannel]->coefficients = coeffs.bassUpper;
		midLower[iChannel]->coefficients = coeffs.midLower;
		midUpper[iChannel]->coefficients = coeffs.midUpper;
		treble[iChannel]->coefficients = coeffs.treble;
	}

	std::array<int, iBandCount> iChoice;
	for (int iBand = 0; iBand < iBandCount; ++iBand)
		iChoice[static_cast<size_t>(iBand)] = static_cast<int>(*apvts.getRawParameterValue(getChoiceParamIds()[static_cast<size_t>(iBand)])); // Get the tremolo waveform choice parameter for each band

	float fInputPeak = 0.0f;
	float fOutputPeak = 0.0f;

	for (int iChannel = 0; iChannel < iTotalNumInputChannels; ++iChannel)
	{
		auto* channelData = buffer.getWritePointer(iChannel);
		const int iNumSamples = buffer.getNumSamples();
		std::array<float, iBandCount> fChannelRate;
		std::array<float, iBandCount> fChannelDepth;

		mod.prepareModulation(mod.fPhaseOffset,
			fChannelRate,
			fChannelDepth,
			fRate,
			fDepth,
			fPhaseOffsetDegrees,
			fSurroundWidth,
			fRateOffset,
			fDepthOffset,
			iChannel,
			iTotalNumInputChannels,
			iTotalNumOutputChannels); // Prepare the mod parameters for the current channel

		for (int iSample = 0; iSample < iNumSamples; ++iSample)
		{
			const auto input = channelData[iSample];
			const auto dry = input * fInGain;
			const auto wet = resonanceFilter[iChannel]->processSample(dry) * 0.59f;

			std::array<float, iBandCount> fBand;
			fBand[0] = subBass[iChannel]->processSample(wet);
			fBand[1] = bassUpper[iChannel]->processSample(wet) + bassLower[iChannel]->processSample(dry);
			fBand[2] = midUpper[iChannel]->processSample(wet) + midLower[iChannel]->processSample(dry);
			fBand[3] = treble[iChannel]->processSample(wet);

			mod.applyBandTremolo(fBand,
				mod.fPhasePos,
				iChannel,
				fChannelRate,
				mod.fPhaseOffset,
				iChoice,
				fPulseWidth,
				fChannelDepth,
				iDepthMode,
				fSampleRate);

			const auto summedBands = fBand[0] + fBand[1] + fBand[2] + fBand[3]; // Sum the processed bands to create the final wet signal
			auto processed = (summedBands * fWetDryControl) + (dry * (1.0f - fWetDryControl)); // Mix the wet and dry signals based on the wet/dry control parameter
			processed *= fOutGain; // Apply the output gain to the processed signal
			const auto output = (processed * (1.0f - fBypassMix)) + (input * fBypassMix);
			channelData[iSample] = output;

			fInputPeak = juce::jmax(fInputPeak, std::abs(input)); // Update the input peak level with the absolute value of the input sample
			fOutputPeak = juce::jmax(fOutputPeak, std::abs(output)); // Update the output peak level with the absolute value of the output sample
		}
	}

	fInputMeterLevel.store(juce::jmax(fInputPeak, fInputMeterLevel.load() * 0.9f)); // Update the input meter level with a decay factor of 0.9
	fOutputMeterLevel.store(juce::jmax(fOutputPeak, fOutputMeterLevel.load() * 0.9f)); // Update the output meter level with a decay factor of 0.9
}

//==============================================================================
bool AutoTremolandoAudioProcessor::hasEditor() const
{
	return true;
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
	std::unique_ptr<juce::XmlElement> xml (state.createXml());
	copyXmlToBinary (*xml, destData);
}

void AutoTremolandoAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
	// You should use this method to restore your parameters from this memory block,
	// whose contents will have been created by the getStateInformation() call.
	std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
	if (xmlState != nullptr)
		if (xmlState->hasTagName (apvts.state.getType()))
			apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
	return new AutoTremolandoAudioProcessor();
}



/*
  ==============================================================================

    AutoTremolando processor implementation.
    Contains parameter definition, preset recall, host integration, and the
    realtime multiband tremolo signal path for float/double processing.

    Plugin: AutoTremolando
    GitHub: MontyWh
    Author: Montague Whishaw

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "PluginExtra.h"

//==============================================================================
// Processor construction and APVTS bootstrap
//==============================================================================
AutoTremolandoAudioProcessor::AutoTremolandoAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
    ), apvts(*this, nullptr, "Parameters", createParameters())
#endif
{
    initPresets();
}

AutoTremolandoAudioProcessor::~AutoTremolandoAudioProcessor() {}

//==============================================================================
// Parameter model (automation surface + defaults)
//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout AutoTremolandoAudioProcessor::createParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    juce::StringArray sTremoloOptions = { "Sine", "Triangle", "Sawtooth", "Pulse", "Square" };

    // Tremolo type menus
    params.push_back(std::make_unique<juce::AudioParameterChoice>("SUB_TREMOLO",    "Sub Tremolo",    sTremoloOptions, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("BASS_TREMOLO",   "Bass Tremolo",   sTremoloOptions, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("MID_TREMOLO",    "Mid Tremolo",    sTremoloOptions, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("TREBLE_TREMOLO", "Treble Tremolo", sTremoloOptions, 0));

    // Input/Output gain
    params.push_back(std::make_unique<juce::AudioParameterFloat>("INPUT_GAIN",  "Input Gain",  0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("OUTPUT_GAIN", "Output Gain", 0.0f, 1.0f, 0.5f));

    // Per-band rate
    params.push_back(std::make_unique<juce::AudioParameterFloat>("SUB_TREM_RATE",    "Sub Rate",    0.5f, 16.0f, 5.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("BASS_TREM_RATE",   "Bass Rate",   0.5f, 16.0f, 5.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("MID_TREM_RATE",    "Mid Rate",    0.5f, 16.0f, 5.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("TREBLE_TREM_RATE", "Treble Rate", 0.5f, 16.0f, 5.0f));

    // Master rate multiplier
    params.push_back(std::make_unique<juce::AudioParameterFloat>("MASTER_RATE", "Master Rate", 0.5f, 2.0f, 1.0f));

    // Per-band depth
    params.push_back(std::make_unique<juce::AudioParameterFloat>("SUB_TREM_DEPTH",    "Sub Depth",    0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("BASS_TREM_DEPTH",   "Bass Depth",   0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("MID_TREM_DEPTH",    "Mid Depth",    0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("TREBLE_TREM_DEPTH", "Treble Depth", 0.0f, 1.0f, 0.5f));

    // Wet, Presence, offsets & Pulse Width
    params.push_back(std::make_unique<juce::AudioParameterFloat>("WET",          "Wet",          0.0f,   1.0f, 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("PRESENCE",     "Presence",     0.0f,   1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("PHASE_OFFSET", "Phase Offset", 0.0f, 180.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("START_PHASE",  "Start Phase",  0.0f, 360.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("RATE_OFFSET",  "Rate Offset",  -7.0f,  7.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("DEPTH_OFFSET", "Depth Offset", -1.0f,  1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("PULSE_WIDTH",  "Pulse Width",  0.05f, 0.95f, 0.5f));

    // Mode switches
    params.push_back(std::make_unique<juce::AudioParameterBool>("RATE_LOCK",         "Rate Lock",         false));
    params.push_back(std::make_unique<juce::AudioParameterBool>("RETRIGGER_ON_PLAY", "Retrigger On Play", true));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("SURROUND_WIDTH",   "Surround Width", 0.0f, 1.0f, 1.0f));

    juce::StringArray sDepthModes = { "Unipolar", "Bipolar" };
    params.push_back(std::make_unique<juce::AudioParameterChoice>("DEPTH_MODE", "Depth Mode", sDepthModes, 0));

    // Bypass
    params.push_back(std::make_unique<juce::AudioParameterBool>("BYPASS", "Bypass", false));

    // Tempo sync
    params.push_back(std::make_unique<juce::AudioParameterBool>("TEMPO_SYNC", "Tempo Sync", false));

    // Note division / time sliders: 0-14 range supports 15 standard sync positions
    // Tempo mode: Straight, Triplet and Dotted for common note values
    // Time mode: 0.5 Hz to 16 Hz (2s to 62ms)
    params.push_back(std::make_unique<juce::AudioParameterFloat>("SUB_NOTE_DIV",    "Sub Note Div",    0.0f, 14.0f, 9.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("BASS_NOTE_DIV",   "Bass Note Div",   0.0f, 14.0f, 9.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("MID_NOTE_DIV",    "Mid Note Div",    0.0f, 14.0f, 9.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("TREBLE_NOTE_DIV", "Treble Note Div", 0.0f, 14.0f, 9.0f));

    return { params.begin(), params.end() };
}

//==============================================================================
// Per-band parameter-id lookup tables
//==============================================================================
const std::array<const char*, TremoloEffect::iBandCount>& AutoTremolandoAudioProcessor::getRateParamIds()
{
    static const std::array<const char*, TremoloEffect::iBandCount> sIds = {
        "SUB_TREM_RATE", "BASS_TREM_RATE", "MID_TREM_RATE", "TREBLE_TREM_RATE"
    };
    return sIds;
}

const std::array<const char*, TremoloEffect::iBandCount>& AutoTremolandoAudioProcessor::getDepthParamIds()
{
    static const std::array<const char*, TremoloEffect::iBandCount> sIds = {
        "SUB_TREM_DEPTH", "BASS_TREM_DEPTH", "MID_TREM_DEPTH", "TREBLE_TREM_DEPTH"
    };
    return sIds;
}

const std::array<const char*, TremoloEffect::iBandCount>& AutoTremolandoAudioProcessor::getChoiceParamIds()
{
    static const std::array<const char*, TremoloEffect::iBandCount> sIds = {
        "SUB_TREMOLO", "BASS_TREMOLO", "MID_TREMOLO", "TREBLE_TREMOLO"
    };
    return sIds;
}

const std::array<const char*, TremoloEffect::iBandCount>& AutoTremolandoAudioProcessor::getNoteDivisionParamIds()
{
    static const std::array<const char*, TremoloEffect::iBandCount> sIds = {
        "SUB_NOTE_DIV", "BASS_NOTE_DIV", "MID_NOTE_DIV", "TREBLE_NOTE_DIV"
    };
    return sIds;
}

//==============================================================================
// Preset storage and recall plumbing
//==============================================================================
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
    if (index < 0 || index >= static_cast<int>(presets.size()))
        return;

    const auto& preset = presets[static_cast<size_t>(index)];

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

    const int iValueCount = juce::jmin(static_cast<int>(preset.fValues.size()), static_cast<int>(std::size(sParamIds)));

    for (int i = 0; i < iValueCount; ++i)
        if (auto* param = apvts.getParameter(sParamIds[i]))
            param->setValueNotifyingHost(param->convertTo0to1(preset.fValues[static_cast<size_t>(i)]));
}

void AutoTremolandoAudioProcessor::registerTapTempo()
{
	const double dNowMs = juce::Time::getMillisecondCounterHiRes();
	const double dPreviousTapMs = dLastTapTimeMs.exchange(dNowMs);

	float fNewBpm = 0.0f;
	if (TremoloEffect::tryCalculateTapTempoBpm(dNowMs, dPreviousTapMs, fNewBpm))
		fTapTempoBpm.store(fNewBpm);
}

void AutoTremolandoAudioProcessor::resetParametersToDefaults()
{
    for (auto* baseParam : getParameters())
        if (auto* rangedParam = dynamic_cast<juce::RangedAudioParameter*>(baseParam))
            rangedParam->setValueNotifyingHost(rangedParam->getDefaultValue());
}

float AutoTremolandoAudioProcessor::getInputMeterLevel() const
{
    return fInputMeterLevel.load();
}

float AutoTremolandoAudioProcessor::getOutputMeterLevel() const
{
    return fOutputMeterLevel.load();
}

//==============================================================================
// JUCE host capability and program contract
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
    return 1;
}

int AutoTremolandoAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AutoTremolandoAudioProcessor::setCurrentProgram(int index)
{
    juce::ignoreUnused(index);
}

const juce::String AutoTremolandoAudioProcessor::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return {};
}

void AutoTremolandoAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

//==============================================================================
//======================================================================
// DSP lifecycle and realtime processing
//======================================================================
void AutoTremolandoAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) // Initialises DSP state before audio starts processing.
{
	int iNumChannels = getTotalNumInputChannels();
	int iNumOutputChannels = getTotalNumOutputChannels();

	fSampleRate = static_cast<float>(sampleRate);

	juce::dsp::ProcessSpec spec;
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

	for (int iChannel = 0; iChannel < iNumChannels; ++iChannel) // Iterates channels, bands, or samples for deterministic DSP.
	{
		subBass.add(new Filter());      subBass[iChannel]->prepare(spec);
		bassLower.add(new Filter());    bassLower[iChannel]->prepare(spec);
		bassUpper.add(new Filter());    bassUpper[iChannel]->prepare(spec);
		midLower.add(new Filter());     midLower[iChannel]->prepare(spec);
		midUpper.add(new Filter());     midUpper[iChannel]->prepare(spec);
		treble.add(new Filter());       treble[iChannel]->prepare(spec);
		resonanceFilter.add(new Filter()); resonanceFilter[iChannel]->prepare(spec);
	}

	fPhaseOffset.assign(iNumOutputChannels, 0.0f);
	tremolo.reset(iNumChannels);

	const float fStartPhaseDegrees = *apvts.getRawParameterValue("START_PHASE"); // Reads host automation values for the current audio block.
	tremolo.retrigger(juce::degreesToRadians(fStartPhaseDegrees));

	TremoloEffect::initialiseGlobalSmoothers(smoothedInputGain, smoothedOutputGain, smoothedWet, smoothedPulseWidth, smoothedBypass, sampleRate);
	TremoloEffect::initialiseBandSmoothers(smoothedRate, smoothedDepth, sampleRate);

	bWasPlaying = false;
	fInputMeterLevel.store(0.0f);
	fOutputMeterLevel.store(0.0f);
}

void AutoTremolandoAudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool AutoTremolandoAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
	juce::ignoreUnused(layouts);
	return true;
#else
	const auto mainOutput = layouts.getMainOutputChannelSet();

	if (mainOutput.isDisabled()) // Branches logic to keep modulation behaviour context-aware.
		return false;

#if ! JucePlugin_IsSynth
	if (mainOutput != layouts.getMainInputChannelSet()) // Branches logic to keep modulation behaviour context-aware.
		return false;
#endif

	return true;
#endif
}
#endif

//==============================================================================
void AutoTremolandoAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
	juce::ScopedNoDenormals noDenormals;
	const int iTotalNumInputChannels = getTotalNumInputChannels();

	const bool bBypass = *apvts.getRawParameterValue("BYPASS") > 0.5f;
	const bool bTempoSync = *apvts.getRawParameterValue("TEMPO_SYNC") > 0.5f;
	const bool bRateLock = *apvts.getRawParameterValue("RATE_LOCK") > 0.5f;
	const bool bRetriggerOnPlay = *apvts.getRawParameterValue("RETRIGGER_ON_PLAY") > 0.5f;
	const float fSurroundWidth = juce::jlimit(0.0f, 1.0f, apvts.getRawParameterValue("SURROUND_WIDTH")->load());
	const int iDepthMode = static_cast<int>(*apvts.getRawParameterValue("DEPTH_MODE"));

	const float fInGainTarget = std::pow(*apvts.getRawParameterValue("INPUT_GAIN"), 3.0f);
	const float fOutGainTarget = std::pow(*apvts.getRawParameterValue("OUTPUT_GAIN"), 3.0f);
	const float fWetDryTarget = std::pow(*apvts.getRawParameterValue("WET"), 3.0f);
	const float fPresence = *apvts.getRawParameterValue("PRESENCE");
	const float fPhaseOffsetDegrees = *apvts.getRawParameterValue("PHASE_OFFSET");
	const float fStartPhaseDegrees = *apvts.getRawParameterValue("START_PHASE");
	const float fRateOffset = *apvts.getRawParameterValue("RATE_OFFSET");
	const float fDepthOffset = *apvts.getRawParameterValue("DEPTH_OFFSET");
	const float fPulseWidthTarget = *apvts.getRawParameterValue("PULSE_WIDTH");
	const float fMasterRate = *apvts.getRawParameterValue("MASTER_RATE");

	TremoloEffect::setGlobalSmoothingTargets(
		smoothedInputGain, smoothedOutputGain, smoothedWet, smoothedPulseWidth, smoothedBypass,
		fInGainTarget, fOutGainTarget, fWetDryTarget, fPulseWidthTarget, bBypass);

	TremoloEffect::loadFloatParams(apvts, getRateParamIds(), fRate);

	float fSyncBpm = fTapTempoBpm.load();
	bool bIsPlaying = false;
	if (auto* ph = getPlayHead())
	{
		if (auto pos = ph->getPosition())
		{
			if (auto optBpm = pos->getBpm())
				fSyncBpm = static_cast<float>(*optBpm);
			bIsPlaying = pos->getIsPlaying();
		}
	}

	if (bTempoSync && bRetriggerOnPlay && bIsPlaying && !bWasPlaying)
		tremolo.retrigger(juce::degreesToRadians(fStartPhaseDegrees));
	bWasPlaying = bIsPlaying;

	if (bTempoSync)
	{
		TremoloEffect::BandIntArray iDivIdx;
		TremoloEffect::loadNoteDivisionIndices(apvts, getNoteDivisionParamIds(), iDivIdx);
		TremoloEffect::applyTempoSyncRates(fRate, iDivIdx, fSyncBpm);
	}

	TremoloEffect::applyMasterRateAndLock(fRate, fMasterRate, bRateLock);
	TremoloEffect::loadFloatParams(apvts, getDepthParamIds(), fDepth);
	TremoloEffect::setBandSmoothingTargets(smoothedRate, smoothedDepth, fRate, fDepth);

	const float fPhaseOffsetRadians = juce::degreesToRadians(fPhaseOffsetDegrees);
	TremoloEffect::computePhaseOffsets(fPhaseOffset, iTotalNumInputChannels, fPhaseOffsetRadians, fSurroundWidth);

	const auto coeffs = TremoloEffect::createFilterCoefficients(fSampleRate, fPresence);
	TremoloEffect::applyFilterCoefficients(resonanceFilter, subBass, bassLower, bassUpper, midLower, midUpper, treble, coeffs, iTotalNumInputChannels);

	TremoloEffect::BandIntArray iChoice;
	TremoloEffect::loadIntParams(apvts, getChoiceParamIds(), iChoice);

	float fInputPeak = 0.0f;
	float fOutputPeak = 0.0f;

	for (int iChannel = 0; iChannel < iTotalNumInputChannels; ++iChannel)
		TremoloEffect::processChannel(buffer, iChannel, iTotalNumInputChannels,
			smoothedInputGain, smoothedOutputGain, smoothedWet, smoothedPulseWidth, smoothedBypass,
			smoothedRate, smoothedDepth, tremolo,
			resonanceFilter, subBass, bassLower, bassUpper, midLower, midUpper, treble,
			iChoice, fSampleRate, fRateOffset, fDepthOffset, iDepthMode,
			fPhaseOffset[static_cast<size_t>(iChannel)], fInputPeak, fOutputPeak);

	TremoloEffect::updateMeterLevel(fInputMeterLevel, fInputPeak);
	TremoloEffect::updateMeterLevel(fOutputMeterLevel, fOutputPeak);
}

void AutoTremolandoAudioProcessor::processBlock(juce::AudioBuffer<double>& buffer, juce::MidiBuffer&)
{
	juce::ScopedNoDenormals noDenormals;
	const int iTotalNumInputChannels = getTotalNumInputChannels();

	const bool bBypass = *apvts.getRawParameterValue("BYPASS") > 0.5f;
	const bool bTempoSync = *apvts.getRawParameterValue("TEMPO_SYNC") > 0.5f;
	const bool bRateLock = *apvts.getRawParameterValue("RATE_LOCK") > 0.5f;
	const bool bRetriggerOnPlay = *apvts.getRawParameterValue("RETRIGGER_ON_PLAY") > 0.5f;
	const float fSurroundWidth = juce::jlimit(0.0f, 1.0f, apvts.getRawParameterValue("SURROUND_WIDTH")->load());
	const int iDepthMode = static_cast<int>(*apvts.getRawParameterValue("DEPTH_MODE"));

	const float fInGainTarget = std::pow(*apvts.getRawParameterValue("INPUT_GAIN"), 3.0f);
	const float fOutGainTarget = std::pow(*apvts.getRawParameterValue("OUTPUT_GAIN"), 3.0f);
	const float fWetDryTarget = std::pow(*apvts.getRawParameterValue("WET"), 3.0f);
	const float fPresence = *apvts.getRawParameterValue("PRESENCE");
	const float fPhaseOffsetDegrees = *apvts.getRawParameterValue("PHASE_OFFSET");
	const float fStartPhaseDegrees = *apvts.getRawParameterValue("START_PHASE");
	const float fRateOffset = *apvts.getRawParameterValue("RATE_OFFSET");
	const float fDepthOffset = *apvts.getRawParameterValue("DEPTH_OFFSET");
	const float fPulseWidthTarget = *apvts.getRawParameterValue("PULSE_WIDTH");
	const float fMasterRate = *apvts.getRawParameterValue("MASTER_RATE");

	TremoloEffect::setGlobalSmoothingTargets(
		smoothedInputGain, smoothedOutputGain, smoothedWet, smoothedPulseWidth, smoothedBypass,
		fInGainTarget, fOutGainTarget, fWetDryTarget, fPulseWidthTarget, bBypass);

	TremoloEffect::loadFloatParams(apvts, getRateParamIds(), fRate);

	float fSyncBpm = fTapTempoBpm.load();
	bool bIsPlaying = false;
	if (auto* ph = getPlayHead())
	{
		if (auto pos = ph->getPosition())
		{
			if (auto optBpm = pos->getBpm())
				fSyncBpm = static_cast<float>(*optBpm);
			bIsPlaying = pos->getIsPlaying();
		}
	}

	if (bTempoSync && bRetriggerOnPlay && bIsPlaying && !bWasPlaying)
		tremolo.retrigger(juce::degreesToRadians(fStartPhaseDegrees));
	bWasPlaying = bIsPlaying;

	if (bTempoSync)
	{
		TremoloEffect::BandIntArray iDivIdx;
		TremoloEffect::loadNoteDivisionIndices(apvts, getNoteDivisionParamIds(), iDivIdx);
		TremoloEffect::applyTempoSyncRates(fRate, iDivIdx, fSyncBpm);
	}

	TremoloEffect::applyMasterRateAndLock(fRate, fMasterRate, bRateLock);
	TremoloEffect::loadFloatParams(apvts, getDepthParamIds(), fDepth);
	TremoloEffect::setBandSmoothingTargets(smoothedRate, smoothedDepth, fRate, fDepth);

	const float fPhaseOffsetRadians = juce::degreesToRadians(fPhaseOffsetDegrees);
	TremoloEffect::computePhaseOffsets(fPhaseOffset, iTotalNumInputChannels, fPhaseOffsetRadians, fSurroundWidth);

	const auto coeffs = TremoloEffect::createFilterCoefficients(fSampleRate, fPresence);
	TremoloEffect::applyFilterCoefficients(resonanceFilter, subBass, bassLower, bassUpper, midLower, midUpper, treble, coeffs, iTotalNumInputChannels);

	TremoloEffect::BandIntArray iChoice;
	TremoloEffect::loadIntParams(apvts, getChoiceParamIds(), iChoice);

	float fInputPeak = 0.0f;
	float fOutputPeak = 0.0f;

	for (int iChannel = 0; iChannel < iTotalNumInputChannels; ++iChannel)
		TremoloEffect::processChannel(buffer, iChannel, iTotalNumInputChannels,
			smoothedInputGain, smoothedOutputGain, smoothedWet, smoothedPulseWidth, smoothedBypass,
			smoothedRate, smoothedDepth, tremolo,
			resonanceFilter, subBass, bassLower, bassUpper, midLower, midUpper, treble,
			iChoice, fSampleRate, fRateOffset, fDepthOffset, iDepthMode,
			fPhaseOffset[static_cast<size_t>(iChannel)], fInputPeak, fOutputPeak);

	TremoloEffect::updateMeterLevel(fInputMeterLevel, fInputPeak);
	TremoloEffect::updateMeterLevel(fOutputMeterLevel, fOutputPeak);
}

bool AutoTremolandoAudioProcessor::supportsDoublePrecisionProcessing() const
{
	return true;
}

//==============================================================================
//======================================================================
// Editor creation bridge
//======================================================================
bool AutoTremolandoAudioProcessor::hasEditor() const // Connects processor state to the plugin user interface.
{
	return true;
}

juce::AudioProcessorEditor* AutoTremolandoAudioProcessor::createEditor() // Connects processor state to the plugin user interface.
{
	return new AutoTremolandoAudioProcessorEditor(*this);
}

//==============================================================================
//======================================================================
// State serialisation/deserialisation
//======================================================================
void AutoTremolandoAudioProcessor::getStateInformation(juce::MemoryBlock& destData) // Persists and restores plugin settings across sessions.
{
	auto state = apvts.copyState(); // Persists and restores plugin settings across sessions.
	std::unique_ptr<juce::XmlElement> xml(state.createXml());
	copyXmlToBinary(*xml, destData);
}

void AutoTremolandoAudioProcessor::setStateInformation(const void* data, int sizeInBytes) // Persists and restores plugin settings across sessions.
{
	std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
	if (xmlState != nullptr) // Branches logic to keep modulation behaviour context-aware.
		if (xmlState->hasTagName(apvts.state.getType())) // Persists and restores plugin settings across sessions.
			apvts.replaceState(juce::ValueTree::fromXml(*xmlState)); // Persists and restores plugin settings across sessions.
}

//==============================================================================
//======================================================================
// Plugin factory entry point
//======================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
	return new AutoTremolandoAudioProcessor();
}



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
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    juce::StringArray sTremoloOptions = { "Sine", "Triangle", "Sawtooth", "Pulse", "Square" };

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
	if (dPreviousTapMs <= 0.0)
		return;

	const double dDeltaMs = dNowMs - dPreviousTapMs;
	if (dDeltaMs < 120.0 || dDeltaMs > 2000.0)
		return;

	const float fNewBpm = static_cast<float>(60000.0 / dDeltaMs);
	fTapTempoBpm.store(juce::jlimit(40.0f, 240.0f, fNewBpm));
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

	for (int iChannel = 0; iChannel < iNumChannels; ++iChannel)
	{
		subBass.add(new Filter());      subBass[iChannel]->prepare(spec);
		bassLower.add(new Filter());    bassLower[iChannel]->prepare(spec);
		bassUpper.add(new Filter());    bassUpper[iChannel]->prepare(spec);
		midLower.add(new Filter());     midLower[iChannel]->prepare(spec);
		midUpper.add(new Filter());     midUpper[iChannel]->prepare(spec);
		treble.add(new Filter());       treble[iChannel]->prepare(spec);
		resonanceFilter.add(new Filter()); resonanceFilter[iChannel]->prepare(spec);
	}

	Modulation::initialisePhaseState(fPhaseOffset,
		fPhasePos,
		iNumChannels,
		iNumOutputChannels,
		*apvts.getRawParameterValue("START_PHASE"));

	bWasPlaying = false;
	fInputMeterLevel.store(0.0f);
	fOutputMeterLevel.store(0.0f);
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
template <typename SampleType>
void AutoTremolandoAudioProcessor::processAudioBlock(juce::AudioBuffer<SampleType>& buffer)
{
	juce::ScopedNoDenormals noDenormals;
	const int iTotalNumInputChannels = getTotalNumInputChannels();
	if (iTotalNumInputChannels <= 0)
		return;

	const bool bBypass = *apvts.getRawParameterValue("BYPASS") > 0.5f;
	const bool bTempoSync = *apvts.getRawParameterValue("TEMPO_SYNC") > 0.5f;
	const bool bRateLock = *apvts.getRawParameterValue("RATE_LOCK") > 0.5f;
	const bool bRetriggerOnPlay = *apvts.getRawParameterValue("RETRIGGER_ON_PLAY") > 0.5f;
	const float fSurroundWidth = juce::jlimit(0.0f, 1.0f, apvts.getRawParameterValue("SURROUND_WIDTH")->load());
	const int iDepthMode = static_cast<int>(*apvts.getRawParameterValue("DEPTH_MODE"));

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

	BandFloatArray fRate {};
	BandFloatArray fDepth {};
	for (int iBand = 0; iBand < iBandCount; ++iBand)
		fRate[static_cast<size_t>(iBand)] = *apvts.getRawParameterValue(getRateParamIds()[static_cast<size_t>(iBand)]);

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
		Modulation::retriggerPhases(fPhasePos, fStartPhaseDegrees);
	bWasPlaying = bIsPlaying;

	if (bTempoSync)
	{
		BandIntArray iDivIdx;
		for (int iBand = 0; iBand < iBandCount; ++iBand)
			iDivIdx[static_cast<size_t>(iBand)] = static_cast<int>(*apvts.getRawParameterValue(getNoteDivisionParamIds()[static_cast<size_t>(iBand)]));

		Modulation::applyTempoSync(fRate, iDivIdx, fSyncBpm);
	}

	for (int iBand = 0; iBand < iBandCount; ++iBand)
	{
		fRate[static_cast<size_t>(iBand)] = std::clamp(fRate[static_cast<size_t>(iBand)] * fMasterRate, 0.5f, 16.0f);
		fDepth[static_cast<size_t>(iBand)] = *apvts.getRawParameterValue(getDepthParamIds()[static_cast<size_t>(iBand)]);
	}

	if (bRateLock)
		Modulation::applyRateLock(fRate);

	Modulation::updatePhaseOffsets(fPhaseOffset,
		iTotalNumInputChannels,
		fPhaseOffsetDegrees,
		fSurroundWidth);

	FilterCoefficients coeffs;
	const float fPresenceFreq = (fPresence * 19000.0f) + 1000.0f;
	coeffs.resonance = juce::dsp::IIR::Coefficients<float>::makePeakFilter(fSampleRate, fPresenceFreq, 1.41f, 1.41f);
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

	BandIntArray iChoice;
	for (int iBand = 0; iBand < iBandCount; ++iBand)
		iChoice[static_cast<size_t>(iBand)] = static_cast<int>(*apvts.getRawParameterValue(getChoiceParamIds()[static_cast<size_t>(iBand)]));

	const float fDoublePi = juce::MathConstants<float>::twoPi;
	float fInputPeak = 0.0f;
	float fOutputPeak = 0.0f;

	for (int iChannel = 0; iChannel < iTotalNumInputChannels; ++iChannel)
	{
		const float fChannelScale = iTotalNumInputChannels <= 1 ? 0.0f : static_cast<float>(iChannel) / static_cast<float>(iTotalNumInputChannels - 1);
		auto* channelData = buffer.getWritePointer(iChannel);
		const int iNumSamples = buffer.getNumSamples();

		for (int iSample = 0; iSample < iNumSamples; ++iSample)
		{
			BandFloatArray fChannelRate;
			BandFloatArray fChannelDepth;
			for (int iBand = 0; iBand < iBandCount; ++iBand)
			{
				fChannelRate[static_cast<size_t>(iBand)] = std::clamp(fRate[static_cast<size_t>(iBand)] + (fRateOffset * fChannelScale), 0.5f, 16.0f);
				fChannelDepth[static_cast<size_t>(iBand)] = std::clamp(fDepth[static_cast<size_t>(iBand)] + (fDepthOffset * fChannelScale), 0.0f, 1.0f);
			}

			const auto input = channelData[iSample];
			const auto dry = input * static_cast<SampleType>(fInGain);
			const auto wet = static_cast<SampleType>(resonanceFilter[iChannel]->processSample(static_cast<float>(dry)))
				* static_cast<SampleType>(0.59f);

			BandFloatArray fBand;
			fBand[0] = subBass[iChannel]->processSample(static_cast<float>(wet));
			fBand[1] = bassUpper[iChannel]->processSample(static_cast<float>(wet)) + bassLower[iChannel]->processSample(static_cast<float>(dry));
			fBand[2] = midUpper[iChannel]->processSample(static_cast<float>(wet)) + midLower[iChannel]->processSample(static_cast<float>(dry));
			fBand[3] = treble[iChannel]->processSample(static_cast<float>(wet));

			for (int iBand = 0; iBand < iBandCount; ++iBand)
			{
				const float fPhaseIncrement = (fDoublePi * fChannelRate[static_cast<size_t>(iBand)]) / fSampleRate;
				fPhasePos[static_cast<size_t>(iChannel)][static_cast<size_t>(iBand)] = Modulation::wrapPhase(fPhasePos[static_cast<size_t>(iChannel)][static_cast<size_t>(iBand)] + fPhaseIncrement);

				const float fPhase = Modulation::wrapPhase(fPhasePos[static_cast<size_t>(iChannel)][static_cast<size_t>(iBand)] + fPhaseOffset[static_cast<size_t>(iChannel)]);
				const float fOsc = Modulation::getOscillatorValue(iChoice[static_cast<size_t>(iBand)], fPhase, fPulseWidth);
				const float fTrem = Modulation::getTremoloGain(fOsc, fChannelDepth[static_cast<size_t>(iBand)], iDepthMode);
				fBand[static_cast<size_t>(iBand)] *= fTrem;
			}

			const auto summedBands = static_cast<SampleType>(fBand[0]) + static_cast<SampleType>(fBand[1])
				+ static_cast<SampleType>(fBand[2]) + static_cast<SampleType>(fBand[3]);
			auto processed = (summedBands * static_cast<SampleType>(fWetDryControl))
				+ (dry * (static_cast<SampleType>(1.0f) - static_cast<SampleType>(fWetDryControl)));
			processed *= static_cast<SampleType>(fOutGain);
			const auto output = (processed * static_cast<SampleType>(1.0f - fBypassMix)) + (input * static_cast<SampleType>(fBypassMix));
			channelData[iSample] = output;

			fInputPeak = juce::jmax(fInputPeak, std::abs(static_cast<float>(input)));
			fOutputPeak = juce::jmax(fOutputPeak, std::abs(static_cast<float>(output)));
		}
	}

	fInputMeterLevel.store(juce::jmax(fInputPeak, fInputMeterLevel.load() * 0.9f));
	fOutputMeterLevel.store(juce::jmax(fOutputPeak, fOutputMeterLevel.load() * 0.9f));
}

void AutoTremolandoAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
	juce::ignoreUnused (midiMessages);
	juce::ScopedNoDenormals noDenormals;
	auto totalNumInputChannels  = getTotalNumInputChannels();
	auto totalNumOutputChannels = getTotalNumOutputChannels();

	// In case we have more outputs than inputs, this code clears any output
	// channels that didn't contain input data, (because these aren't
	// guaranteed to be empty - they may contain garbage).
	// This is here to avoid people getting screaming feedback
	// when they first compile a plugin, but obviously you don't need to keep
	// this code if your algorithm always overwrites all the output channels.
	for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
		buffer.clear (i, 0, buffer.getNumSamples());

	processAudioBlock (buffer);
}

void AutoTremolandoAudioProcessor::processBlock (juce::AudioBuffer<double>& buffer, juce::MidiBuffer& midiMessages)
{
	juce::ignoreUnused (midiMessages);
	processAudioBlock (buffer);
}

bool AutoTremolandoAudioProcessor::supportsDoublePrecisionProcessing() const
{
	return true;
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



/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "PluginExtra.h"
#include <type_traits>

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
juce::AudioProcessorValueTreeState::ParameterLayout AutoTremolandoAudioProcessor::createParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    juce::StringArray sTremoloOptions = { "Sine", "Triangle", "Sawtooth", "Pulse", "Square" };

    // Tremolo type menus
    params.push_back(std::make_unique<juce::AudioParameterChoice>("SUB_TREMOLO", "Sub Tremolo", sTremoloOptions, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("BASS_TREMOLO", "Bass Tremolo", sTremoloOptions, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("MID_TREMOLO", "Mid Tremolo", sTremoloOptions, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("TREBLE_TREMOLO", "Treble Tremolo", sTremoloOptions, 0));

    // Input/Output gain
    params.push_back(std::make_unique<juce::AudioParameterFloat>("INPUT_GAIN", "Input Gain", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("OUTPUT_GAIN", "Output Gain", 0.0f, 1.0f, 0.5f));

    // Per-band RATE
    params.push_back(std::make_unique<juce::AudioParameterFloat>("SUB_TREM_RATE", "Sub Rate", 0.5f, 16.0f, 5.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("BASS_TREM_RATE", "Bass Rate", 0.5f, 16.0f, 5.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("MID_TREM_RATE", "Mid Rate", 0.5f, 16.0f, 5.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("TREBLE_TREM_RATE", "Treble Rate", 0.5f, 16.0f, 5.0f));

    // Master Rate (multiplier for all band rates)
    params.push_back(std::make_unique<juce::AudioParameterFloat>("MASTER_RATE", "Master Rate", 0.5f, 2.0f, 1.0f));

    // Per-band DEPTH
    params.push_back(std::make_unique<juce::AudioParameterFloat>("SUB_TREM_DEPTH", "Sub Depth", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("BASS_TREM_DEPTH", "Bass Depth", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("MID_TREM_DEPTH", "Mid Depth", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("TREBLE_TREM_DEPTH", "Treble Depth", 0.0f, 1.0f, 0.5f));

    // Wet, Presence, Offsets & Pulse Width
    params.push_back(std::make_unique<juce::AudioParameterFloat>("WET", "Wet", 0.0f, 1.0f, 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("PRESENCE", "Presence", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("PHASE_OFFSET", "Phase Offset", 0.0f, 180.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("START_PHASE", "Start Phase", 0.0f, 360.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("RATE_OFFSET", "Rate Offset", -7.0f, 7.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("DEPTH_OFFSET", "Depth Offset", -1.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("PULSE_WIDTH", "Pulse Width", 0.05f, 0.95f, 0.5f));

    // Mode switches
    params.push_back(std::make_unique<juce::AudioParameterBool>("RATE_LOCK", "Rate Lock", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>("RETRIGGER_ON_PLAY", "Retrigger On Play", true));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("SURROUND_WIDTH", "Surround Width", 0.0f, 1.0f, 1.0f));

    juce::StringArray sDepthModes = { "Unipolar", "Bipolar" };
    params.push_back(std::make_unique<juce::AudioParameterChoice>("DEPTH_MODE", "Depth Mode", sDepthModes, 0));

    // Bypass
    params.push_back(std::make_unique<juce::AudioParameterBool>("BYPASS", "Bypass", false));

    // Tempo sync
    params.push_back(std::make_unique<juce::AudioParameterBool>("TEMPO_SYNC", "Tempo Sync", false));

    // Note division / time sliders: 0–14 range supports 15 standard sync positions
    // Tempo mode: Straight, Triplet and Dotted for common note values
    // Time mode: 0.5 Hz to 16 Hz (2s to 62ms)
    params.push_back(std::make_unique<juce::AudioParameterFloat>("SUB_NOTE_DIV",    "Sub Note Div",    0.0f, 14.0f, 9.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("BASS_NOTE_DIV",   "Bass Note Div",   0.0f, 14.0f, 9.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("MID_NOTE_DIV",    "Mid Note Div",    0.0f, 14.0f, 9.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("TREBLE_NOTE_DIV", "Treble Note Div", 0.0f, 14.0f, 9.0f));

    return { params.begin(), params.end() };
}

//==============================================================================
void AutoTremolandoAudioProcessor::initPresets()
{
    presets = {
        { "Preset 1", {
        0.5f, 0.0f,          // Input, Presence
        0, 0, 0, 0,          // Sub/Bass/Mid/Treble Tremolo types
        1.0f,                // Master Rate
        5.0f, 5.0f, 5.0f, 5.0f,   // Sub/Bass/Mid/Treble Rates
        0.5f, 0.5f, 0.5f, 0.5f,   // Sub/Bass/Mid/Treble Depths
        0.0f, 0.0f, 0.0f, 0.0f, 0.5f,   // Phase Offset, Start Phase, Rate Offset, Depth Offset, Pulse Width
        0.0f, 1.0f, 1.0f, 0.0f,         // Rate Lock, Retrigger On Play, Surround Width, Depth Mode
        1.0f, 0.5f, 0.0f     // Wet, Output, Bypass
    }},
    { "Preset 2", {
        0.5f, 0.0f,          // Input, Presence
        0, 0, 0, 0,          // Sub/Bass/Mid/Treble Tremolo types
        1.0f,                // Master Rate
        5.0f, 5.0f, 5.0f, 5.0f,   // Sub/Bass/Mid/Treble Rates
        0.5f, 0.5f, 0.5f, 0.5f,   // Sub/Bass/Mid/Treble Depths
        0.0f, 0.0f, 0.0f, 0.0f, 0.5f,   // Phase Offset, Start Phase, Rate Offset, Depth Offset, Pulse Width
        0.0f, 1.0f, 1.0f, 0.0f,         // Rate Lock, Retrigger On Play, Surround Width, Depth Mode
        1.0f, 0.5f, 0.0f     // Wet, Output, Bypass
    }},
    { "Preset 3", {
        0.5f, 0.0f,          // Input, Presence
        0, 0, 0, 0,          // Sub/Bass/Mid/Treble Tremolo types
        1.0f,                // Master Rate
        5.0f, 5.0f, 5.0f, 5.0f,   // Sub/Bass/Mid/Treble Rates
        0.5f, 0.5f, 0.5f, 0.5f,   // Sub/Bass/Mid/Treble Depths
        0.0f, 0.0f, 0.0f, 0.0f, 0.5f,   // Phase Offset, Start Phase, Rate Offset, Depth Offset, Pulse Width
        0.0f, 1.0f, 1.0f, 0.0f,         // Rate Lock, Retrigger On Play, Surround Width, Depth Mode
        1.0f, 0.5f, 0.0f     // Wet, Output, Bypass
    }}
    };

}

void AutoTremolandoAudioProcessor::loadPreset(int index)
{
    if (index < 0 || index >= presets.size()) return;

    auto& preset = presets[index];

    const char* paramIDs[] = {
        "INPUT_GAIN", "PRESENCE",
        "SUB_TREMOLO", "BASS_TREMOLO", "MID_TREMOLO", "TREBLE_TREMOLO",
        "MASTER_RATE",
        "SUB_TREM_RATE", "BASS_TREM_RATE", "MID_TREM_RATE", "TREBLE_TREM_RATE",
        "SUB_TREM_DEPTH", "BASS_TREM_DEPTH", "MID_TREM_DEPTH", "TREBLE_TREM_DEPTH",
        "PHASE_OFFSET", "START_PHASE", "RATE_OFFSET", "DEPTH_OFFSET", "PULSE_WIDTH",
        "RATE_LOCK", "RETRIGGER_ON_PLAY", "SURROUND_WIDTH", "DEPTH_MODE",
        "WET", "OUTPUT_GAIN", "BYPASS"
    };

    for (int i = 0; i < (int)preset.fValues.size(); ++i)
        if (auto* param = apvts.getParameter(paramIDs[i]))
            param->setValueNotifyingHost(param->convertTo0to1(preset.fValues[i]));
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
    {
        if (auto* rangedParam = dynamic_cast<juce::RangedAudioParameter*>(baseParam))
            rangedParam->setValueNotifyingHost(rangedParam->getDefaultValue());
    }
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
void AutoTremolandoAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
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

    fPhaseOffset.assign(iNumOutputChannels, 0.0f);
    tremolo.reset(iNumChannels);

    const float fStartPhaseDegrees = *apvts.getRawParameterValue("START_PHASE");
    tremolo.retrigger(juce::degreesToRadians(fStartPhaseDegrees));

    smoothedInputGain.reset(sampleRate, 0.02);
    smoothedOutputGain.reset(sampleRate, 0.02);
    smoothedWet.reset(sampleRate, 0.02);
    smoothedPulseWidth.reset(sampleRate, 0.02);
    smoothedBypass.reset(sampleRate, 0.02);

    smoothedInputGain.setCurrentAndTargetValue(0.5f);
    smoothedOutputGain.setCurrentAndTargetValue(0.5f);
    smoothedWet.setCurrentAndTargetValue(1.0f);
    smoothedPulseWidth.setCurrentAndTargetValue(0.5f);
    smoothedBypass.setCurrentAndTargetValue(0.0f);

    for (int iBand = 0; iBand < 4; ++iBand)
    {
        smoothedRate[iBand].reset(sampleRate, 0.02);
        smoothedDepth[iBand].reset(sampleRate, 0.02);
        smoothedRate[iBand].setCurrentAndTargetValue(5.0f);
        smoothedDepth[iBand].setCurrentAndTargetValue(0.5f);
    }

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

    if (mainOutput.isDisabled())
        return false;

#if ! JucePlugin_IsSynth
    if (mainOutput != layouts.getMainInputChannelSet())
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
	auto iTotalNumInputChannels = getTotalNumInputChannels();
	auto iNumSamples = buffer.getNumSamples();

	const bool bBypass = *apvts.getRawParameterValue("BYPASS") > 0.5f;
	const bool bTempoSync = *apvts.getRawParameterValue("TEMPO_SYNC") > 0.5f;
	const bool bRateLock = *apvts.getRawParameterValue("RATE_LOCK") > 0.5f;
	const bool bRetriggerOnPlay = *apvts.getRawParameterValue("RETRIGGER_ON_PLAY") > 0.5f;
	const float fSurroundWidth = juce::jlimit(0.0f, 1.0f, apvts.getRawParameterValue("SURROUND_WIDTH")->load());
	const int iDepthMode = (int)*apvts.getRawParameterValue("DEPTH_MODE");

	const float fInGainTarget = std::pow(*apvts.getRawParameterValue("INPUT_GAIN"), 3.0f);
	const float fOutGainTarget = std::pow(*apvts.getRawParameterValue("OUTPUT_GAIN"), 3.0f);
	const float fWetDryTarget = std::pow(*apvts.getRawParameterValue("WET"), 3.0f);
	const float fPresence = *apvts.getRawParameterValue("PRESENCE");
	const float fPhaseOffsetDegrees = *apvts.getRawParameterValue("PHASE_OFFSET");
	const float fStartPhaseDegrees = *apvts.getRawParameterValue("START_PHASE");
	const float fRateOffset = *apvts.getRawParameterValue("RATE_OFFSET");
	const float fDepthOffset = *apvts.getRawParameterValue("DEPTH_OFFSET");
	const float fPulseWidthTarget = *apvts.getRawParameterValue("PULSE_WIDTH");

	smoothedInputGain.setTargetValue(fInGainTarget);
	smoothedOutputGain.setTargetValue(fOutGainTarget);
	smoothedWet.setTargetValue(fWetDryTarget);
	smoothedPulseWidth.setTargetValue(fPulseWidthTarget);
	smoothedBypass.setTargetValue(bBypass ? 1.0f : 0.0f);

	fRate[0] = *apvts.getRawParameterValue("SUB_TREM_RATE");
	fRate[1] = *apvts.getRawParameterValue("BASS_TREM_RATE");
	fRate[2] = *apvts.getRawParameterValue("MID_TREM_RATE");
	fRate[3] = *apvts.getRawParameterValue("TREBLE_TREM_RATE");

	const float fMasterRate = *apvts.getRawParameterValue("MASTER_RATE");

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
		const float fTempoMultipliers[] = {
			0.25f, 0.166667f, 0.375f,
			0.5f, 0.333333f, 0.75f,
			1.0f, 0.666667f, 1.5f,
			2.0f, 1.333333f, 3.0f,
			4.0f, 2.666667f, 6.0f
		};
		const int iDivIdx[4] = {
			juce::jlimit(0, 14, (int)*apvts.getRawParameterValue("SUB_NOTE_DIV")),
			juce::jlimit(0, 14, (int)*apvts.getRawParameterValue("BASS_NOTE_DIV")),
			juce::jlimit(0, 14, (int)*apvts.getRawParameterValue("MID_NOTE_DIV")),
			juce::jlimit(0, 14, (int)*apvts.getRawParameterValue("TREBLE_NOTE_DIV"))
		};
		const float fBeatsPerSec = fSyncBpm / 60.0f;
		for (int iBand = 0; iBand < 4; ++iBand)
			fRate[iBand] = fBeatsPerSec * fTempoMultipliers[iDivIdx[iBand]];
	}

	for (int iBand = 0; iBand < 4; ++iBand)
		fRate[iBand] = juce::jlimit(0.5f, 16.0f, fRate[iBand] * fMasterRate);

	if (bRateLock)
		for (int iBand = 1; iBand < 4; ++iBand)
			fRate[iBand] = fRate[0];

	fDepth[0] = *apvts.getRawParameterValue("SUB_TREM_DEPTH");
	fDepth[1] = *apvts.getRawParameterValue("BASS_TREM_DEPTH");
	fDepth[2] = *apvts.getRawParameterValue("MID_TREM_DEPTH");
	fDepth[3] = *apvts.getRawParameterValue("TREBLE_TREM_DEPTH");

	for (int iBand = 0; iBand < 4; ++iBand)
	{
		smoothedRate[iBand].setTargetValue(fRate[iBand]);
		smoothedDepth[iBand].setTargetValue(fDepth[iBand]);
	}

	const float fPhaseOffsetRadians = juce::degreesToRadians(fPhaseOffsetDegrees);
	if (iTotalNumInputChannels <= 1 || fSurroundWidth <= 0.0f)
	{
		for (int iChannel = 0; iChannel < iTotalNumInputChannels; ++iChannel)
			fPhaseOffset[iChannel] = 0.0f;
	}
	else
	{
		const float fMaxOffset = fPhaseOffsetRadians * fSurroundWidth;
		for (int iChannel = 0; iChannel < iTotalNumInputChannels; ++iChannel)
			fPhaseOffset[iChannel] = fMaxOffset * ((float)iChannel / (float)(iTotalNumInputChannels - 1));
	}

	float fPresenceFreq = fPresence * 19000.0f + 1000.0f;
	auto resonanceCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(fSampleRate, fPresenceFreq, 1.41f, 1.41f);
	auto subCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(fSampleRate, 60.0f);
	auto bassLCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(fSampleRate, 250.0f);
	auto bassUCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(fSampleRate, 60.0f);
	auto midLCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(fSampleRate, 2000.0f);
	auto midUCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(fSampleRate, 250.0f);
	auto trebCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(fSampleRate, 2000.0f);

	for (int iChannel = 0; iChannel < iTotalNumInputChannels; ++iChannel)
	{
		resonanceFilter[iChannel]->coefficients = resonanceCoeffs;
		subBass[iChannel]->coefficients = subCoeffs;
		bassLower[iChannel]->coefficients = bassLCoeffs;
		bassUpper[iChannel]->coefficients = bassUCoeffs;
		midLower[iChannel]->coefficients = midLCoeffs;
		midUpper[iChannel]->coefficients = midUCoeffs;
		treble[iChannel]->coefficients = trebCoeffs;
	}

	int iChoice[4];
	iChoice[0] = (int)*apvts.getRawParameterValue("SUB_TREMOLO");
	iChoice[1] = (int)*apvts.getRawParameterValue("BASS_TREMOLO");
	iChoice[2] = (int)*apvts.getRawParameterValue("MID_TREMOLO");
	iChoice[3] = (int)*apvts.getRawParameterValue("TREBLE_TREMOLO");

	float fInputPeak = 0.0f;
	float fOutputPeak = 0.0f;

	for (int iChannel = 0; iChannel < iTotalNumInputChannels; ++iChannel)
	{
		float fChannelScale = 0.0f;
		if (iTotalNumInputChannels > 1)
			fChannelScale = (float)iChannel / (float)(iTotalNumInputChannels - 1);

		auto* channelData = buffer.getWritePointer(iChannel);

		for (int iSample = 0; iSample < iNumSamples; ++iSample)
		{
			using SampleType = std::remove_reference_t<decltype(channelData[iSample])>;

			const float fInGain = smoothedInputGain.getNextValue();
			const float fOutGain = smoothedOutputGain.getNextValue();
			const float fWetDryControl = smoothedWet.getNextValue();
			const float fPulseWidth = smoothedPulseWidth.getNextValue();
			const float fBypassMix = smoothedBypass.getNextValue();

			float fChannelRate[4];
			float fChannelDepth[4];
			for (int iBand = 0; iBand < 4; ++iBand)
			{
				const float fSmoothRate = smoothedRate[iBand].getNextValue();
				const float fSmoothDepth = smoothedDepth[iBand].getNextValue();
				fChannelRate[iBand] = juce::jlimit(0.5f, 16.0f, fSmoothRate + (fRateOffset * fChannelScale));
				fChannelDepth[iBand] = juce::jlimit(0.0f, 1.0f, fSmoothDepth + (fDepthOffset * fChannelScale));
			}

			auto input = channelData[iSample];
			auto dry = input * static_cast<SampleType>(fInGain);
			auto wet = static_cast<SampleType>(resonanceFilter[iChannel]->processSample(static_cast<float>(dry)))
				* static_cast<SampleType>(1.0f - 0.41f);

			float fBand[4];
			fBand[0] = subBass[iChannel]->processSample(static_cast<float>(wet));
			fBand[1] = bassUpper[iChannel]->processSample(static_cast<float>(wet))
				+ bassLower[iChannel]->processSample(static_cast<float>(dry));
			fBand[2] = midUpper[iChannel]->processSample(static_cast<float>(wet))
				+ midLower[iChannel]->processSample(static_cast<float>(dry));
			fBand[3] = treble[iChannel]->processSample(static_cast<float>(wet));

			tremolo.processChannelBands(iChannel, fBand, fChannelDepth, iChoice, fChannelRate, fSampleRate, fPhaseOffset[iChannel], fPulseWidth, iDepthMode);

			auto summedBands = static_cast<SampleType>(fBand[0]) + static_cast<SampleType>(fBand[1])
				+ static_cast<SampleType>(fBand[2]) + static_cast<SampleType>(fBand[3]);
			auto processed = (summedBands * static_cast<SampleType>(fWetDryControl))
				+ (dry * (static_cast<SampleType>(1.0f) - static_cast<SampleType>(fWetDryControl)));
			processed *= static_cast<SampleType>(fOutGain);

			auto output = (processed * static_cast<SampleType>(1.0f - fBypassMix))
				+ (input * static_cast<SampleType>(fBypassMix));
			channelData[iSample] = output;

			fInputPeak = juce::jmax(fInputPeak, std::abs(static_cast<float>(input)));
			fOutputPeak = juce::jmax(fOutputPeak, std::abs(static_cast<float>(output)));
		}
	}

	fInputMeterLevel.store(juce::jmax(fInputPeak, fInputMeterLevel.load() * 0.9f));
	fOutputMeterLevel.store(juce::jmax(fOutputPeak, fOutputMeterLevel.load() * 0.9f));
}

void AutoTremolandoAudioProcessor::processBlock(juce::AudioBuffer<double>& buffer, juce::MidiBuffer&)
{
	juce::ScopedNoDenormals noDenormals;
	auto iTotalNumInputChannels = getTotalNumInputChannels();
	auto iNumSamples = buffer.getNumSamples();

	const bool bBypass = *apvts.getRawParameterValue("BYPASS") > 0.5f;
	const bool bTempoSync = *apvts.getRawParameterValue("TEMPO_SYNC") > 0.5f;
	const bool bRateLock = *apvts.getRawParameterValue("RATE_LOCK") > 0.5f;
	const bool bRetriggerOnPlay = *apvts.getRawParameterValue("RETRIGGER_ON_PLAY") > 0.5f;
	const float fSurroundWidth = juce::jlimit(0.0f, 1.0f, apvts.getRawParameterValue("SURROUND_WIDTH")->load());
	const int iDepthMode = (int)*apvts.getRawParameterValue("DEPTH_MODE");

	const float fInGainTarget = std::pow(*apvts.getRawParameterValue("INPUT_GAIN"), 3.0f);
	const float fOutGainTarget = std::pow(*apvts.getRawParameterValue("OUTPUT_GAIN"), 3.0f);
	const float fWetDryTarget = std::pow(*apvts.getRawParameterValue("WET"), 3.0f);
	const float fPresence = *apvts.getRawParameterValue("PRESENCE");
	const float fPhaseOffsetDegrees = *apvts.getRawParameterValue("PHASE_OFFSET");
	const float fStartPhaseDegrees = *apvts.getRawParameterValue("START_PHASE");
	const float fRateOffset = *apvts.getRawParameterValue("RATE_OFFSET");
	const float fDepthOffset = *apvts.getRawParameterValue("DEPTH_OFFSET");
	const float fPulseWidthTarget = *apvts.getRawParameterValue("PULSE_WIDTH");

	smoothedInputGain.setTargetValue(fInGainTarget);
	smoothedOutputGain.setTargetValue(fOutGainTarget);
	smoothedWet.setTargetValue(fWetDryTarget);
	smoothedPulseWidth.setTargetValue(fPulseWidthTarget);
	smoothedBypass.setTargetValue(bBypass ? 1.0f : 0.0f);

	fRate[0] = *apvts.getRawParameterValue("SUB_TREM_RATE");
	fRate[1] = *apvts.getRawParameterValue("BASS_TREM_RATE");
	fRate[2] = *apvts.getRawParameterValue("MID_TREM_RATE");
	fRate[3] = *apvts.getRawParameterValue("TREBLE_TREM_RATE");

	const float fMasterRate = *apvts.getRawParameterValue("MASTER_RATE");

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
		const float fTempoMultipliers[] = {
			0.25f, 0.166667f, 0.375f,
			0.5f, 0.333333f, 0.75f,
			1.0f, 0.666667f, 1.5f,
			2.0f, 1.333333f, 3.0f,
			4.0f, 2.666667f, 6.0f
		};
		const int iDivIdx[4] = {
			juce::jlimit(0, 14, (int)*apvts.getRawParameterValue("SUB_NOTE_DIV")),
			juce::jlimit(0, 14, (int)*apvts.getRawParameterValue("BASS_NOTE_DIV")),
			juce::jlimit(0, 14, (int)*apvts.getRawParameterValue("MID_NOTE_DIV")),
			juce::jlimit(0, 14, (int)*apvts.getRawParameterValue("TREBLE_NOTE_DIV"))
		};
		const float fBeatsPerSec = fSyncBpm / 60.0f;
		for (int iBand = 0; iBand < 4; ++iBand)
			fRate[iBand] = fBeatsPerSec * fTempoMultipliers[iDivIdx[iBand]];
	}

	for (int iBand = 0; iBand < 4; ++iBand)
		fRate[iBand] = juce::jlimit(0.5f, 16.0f, fRate[iBand] * fMasterRate);

	if (bRateLock)
		for (int iBand = 1; iBand < 4; ++iBand)
			fRate[iBand] = fRate[0];

	fDepth[0] = *apvts.getRawParameterValue("SUB_TREM_DEPTH");
	fDepth[1] = *apvts.getRawParameterValue("BASS_TREM_DEPTH");
	fDepth[2] = *apvts.getRawParameterValue("MID_TREM_DEPTH");
	fDepth[3] = *apvts.getRawParameterValue("TREBLE_TREM_DEPTH");

	for (int iBand = 0; iBand < 4; ++iBand)
	{
		smoothedRate[iBand].setTargetValue(fRate[iBand]);
		smoothedDepth[iBand].setTargetValue(fDepth[iBand]);
	}

	const float fPhaseOffsetRadians = juce::degreesToRadians(fPhaseOffsetDegrees);
	if (iTotalNumInputChannels <= 1 || fSurroundWidth <= 0.0f)
	{
		for (int iChannel = 0; iChannel < iTotalNumInputChannels; ++iChannel)
			fPhaseOffset[iChannel] = 0.0f;
	}
	else
	{
		const float fMaxOffset = fPhaseOffsetRadians * fSurroundWidth;
		for (int iChannel = 0; iChannel < iTotalNumInputChannels; ++iChannel)
			fPhaseOffset[iChannel] = fMaxOffset * ((float)iChannel / (float)(iTotalNumInputChannels - 1));
	}

	float fPresenceFreq = fPresence * 19000.0f + 1000.0f;
	auto resonanceCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(fSampleRate, fPresenceFreq, 1.41f, 1.41f);
	auto subCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(fSampleRate, 60.0f);
	auto bassLCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(fSampleRate, 250.0f);
	auto bassUCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(fSampleRate, 60.0f);
	auto midLCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(fSampleRate, 2000.0f);
	auto midUCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(fSampleRate, 250.0f);
	auto trebCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(fSampleRate, 2000.0f);

	for (int iChannel = 0; iChannel < iTotalNumInputChannels; ++iChannel)
	{
		resonanceFilter[iChannel]->coefficients = resonanceCoeffs;
		subBass[iChannel]->coefficients = subCoeffs;
		bassLower[iChannel]->coefficients = bassLCoeffs;
		bassUpper[iChannel]->coefficients = bassUCoeffs;
		midLower[iChannel]->coefficients = midLCoeffs;
		midUpper[iChannel]->coefficients = midUCoeffs;
		treble[iChannel]->coefficients = trebCoeffs;
	}

	int iChoice[4];
	iChoice[0] = (int)*apvts.getRawParameterValue("SUB_TREMOLO");
	iChoice[1] = (int)*apvts.getRawParameterValue("BASS_TREMOLO");
	iChoice[2] = (int)*apvts.getRawParameterValue("MID_TREMOLO");
	iChoice[3] = (int)*apvts.getRawParameterValue("TREBLE_TREMOLO");

	float fInputPeak = 0.0f;
	float fOutputPeak = 0.0f;

	for (int iChannel = 0; iChannel < iTotalNumInputChannels; ++iChannel)
	{
		float fChannelScale = 0.0f;
		if (iTotalNumInputChannels > 1)
			fChannelScale = (float)iChannel / (float)(iTotalNumInputChannels - 1);

		auto* channelData = buffer.getWritePointer(iChannel);

		for (int iSample = 0; iSample < iNumSamples; ++iSample)
		{
			using SampleType = std::remove_reference_t<decltype(channelData[iSample])>;

			const float fInGain = smoothedInputGain.getNextValue();
			const float fOutGain = smoothedOutputGain.getNextValue();
			const float fWetDryControl = smoothedWet.getNextValue();
			const float fPulseWidth = smoothedPulseWidth.getNextValue();
			const float fBypassMix = smoothedBypass.getNextValue();

			float fChannelRate[4];
			float fChannelDepth[4];
			for (int iBand = 0; iBand < 4; ++iBand)
			{
				const float fSmoothRate = smoothedRate[iBand].getNextValue();
				const float fSmoothDepth = smoothedDepth[iBand].getNextValue();
				fChannelRate[iBand] = juce::jlimit(0.5f, 16.0f, fSmoothRate + (fRateOffset * fChannelScale));
				fChannelDepth[iBand] = juce::jlimit(0.0f, 1.0f, fSmoothDepth + (fDepthOffset * fChannelScale));
			}

			auto input = channelData[iSample];
			auto dry = input * static_cast<SampleType>(fInGain);
			auto wet = static_cast<SampleType>(resonanceFilter[iChannel]->processSample(static_cast<float>(dry)))
				* static_cast<SampleType>(1.0f - 0.41f);

			float fBand[4];
			fBand[0] = subBass[iChannel]->processSample(static_cast<float>(wet));
			fBand[1] = bassUpper[iChannel]->processSample(static_cast<float>(wet))
				+ bassLower[iChannel]->processSample(static_cast<float>(dry));
			fBand[2] = midUpper[iChannel]->processSample(static_cast<float>(wet))
				+ midLower[iChannel]->processSample(static_cast<float>(dry));
			fBand[3] = treble[iChannel]->processSample(static_cast<float>(wet));

			tremolo.processChannelBands(iChannel, fBand, fChannelDepth, iChoice, fChannelRate, fSampleRate, fPhaseOffset[iChannel], fPulseWidth, iDepthMode);

			auto summedBands = static_cast<SampleType>(fBand[0]) + static_cast<SampleType>(fBand[1])
				+ static_cast<SampleType>(fBand[2]) + static_cast<SampleType>(fBand[3]);
			auto processed = (summedBands * static_cast<SampleType>(fWetDryControl))
				+ (dry * (static_cast<SampleType>(1.0f) - static_cast<SampleType>(fWetDryControl)));
			processed *= static_cast<SampleType>(fOutGain);

			auto output = (processed * static_cast<SampleType>(1.0f - fBypassMix))
				+ (input * static_cast<SampleType>(fBypassMix));
			channelData[iSample] = output;

			fInputPeak = juce::jmax(fInputPeak, std::abs(static_cast<float>(input)));
			fOutputPeak = juce::jmax(fOutputPeak, std::abs(static_cast<float>(output)));
		}
	}

	fInputMeterLevel.store(juce::jmax(fInputPeak, fInputMeterLevel.load() * 0.9f));
	fOutputMeterLevel.store(juce::jmax(fOutputPeak, fOutputMeterLevel.load() * 0.9f));
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
    return new AutoTremolandoAudioProcessorEditor(*this);
}

//==============================================================================
void AutoTremolandoAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void AutoTremolandoAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr)
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AutoTremolandoAudioProcessor();
}


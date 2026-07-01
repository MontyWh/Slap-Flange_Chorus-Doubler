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

    juce::StringArray tremoloOptions = { "Sine", "Triangle", "Sawtooth", "Pulse", "Square" };

    // Tremolo type menus
    params.push_back(std::make_unique<juce::AudioParameterChoice>("SUB_TREMOLO", "Sub Tremolo", tremoloOptions, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("BASS_TREMOLO", "Bass Tremolo", tremoloOptions, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("MID_TREMOLO", "Mid Tremolo", tremoloOptions, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("TREBLE_TREMOLO", "Treble Tremolo", tremoloOptions, 0));

    // Input/Output gain
    params.push_back(std::make_unique<juce::AudioParameterFloat>("INPUT_GAIN", "Input Gain", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("OUTPUT_GAIN", "Output Gain", 0.0f, 1.0f, 0.5f));

    // Per-band RATE
    params.push_back(std::make_unique<juce::AudioParameterFloat>("SUB_TREM_RATE", "Sub Rate", 1.0f, 15.0f, 5.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("BASS_TREM_RATE", "Bass Rate", 1.0f, 15.0f, 5.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("MID_TREM_RATE", "Mid Rate", 1.0f, 15.0f, 5.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("TREBLE_TREM_RATE", "Treble Rate", 1.0f, 15.0f, 5.0f));

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

    juce::StringArray stereoModes = { "Mono", "Ping Pong", "Spread" };
    params.push_back(std::make_unique<juce::AudioParameterChoice>("STEREO_MODE", "Stereo Mode", stereoModes, 2));

    juce::StringArray depthModes = { "Unipolar", "Bipolar" };
    params.push_back(std::make_unique<juce::AudioParameterChoice>("DEPTH_MODE", "Depth Mode", depthModes, 0));

    // Bypass
    params.push_back(std::make_unique<juce::AudioParameterBool>("BYPASS", "Bypass", false));

    // Tempo sync
    params.push_back(std::make_unique<juce::AudioParameterBool>("TEMPO_SYNC", "Tempo Sync", false));

    // Note division / time sliders: 0–29 range supports 30 positions
    // Tempo mode: Straight (6) + Triplet (6) + Dotted (6) + Swing (6) + Shuffle (6)
    // Time mode: 0.5 Hz to 16 Hz (2s to 62ms)
    params.push_back(std::make_unique<juce::AudioParameterFloat>("SUB_NOTE_DIV",    "Sub Note Div",    0.0f, 29.0f, 4.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("BASS_NOTE_DIV",   "Bass Note Div",   0.0f, 29.0f, 4.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("MID_NOTE_DIV",    "Mid Note Div",    0.0f, 29.0f, 4.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("TREBLE_NOTE_DIV", "Treble Note Div", 0.0f, 29.0f, 4.0f));

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
        0.0f, 1.0f, 2.0f, 0.0f,         // Rate Lock, Retrigger On Play, Stereo Mode, Depth Mode
        1.0f, 0.5f, 0.0f     // Wet, Output, Bypass
    }},
    { "Preset 2", {
        0.5f, 0.0f,          // Input, Presence
        0, 0, 0, 0,          // Sub/Bass/Mid/Treble Tremolo types
        1.0f,                // Master Rate
        5.0f, 5.0f, 5.0f, 5.0f,   // Sub/Bass/Mid/Treble Rates
        0.5f, 0.5f, 0.5f, 0.5f,   // Sub/Bass/Mid/Treble Depths
        0.0f, 0.0f, 0.0f, 0.0f, 0.5f,   // Phase Offset, Start Phase, Rate Offset, Depth Offset, Pulse Width
        0.0f, 1.0f, 2.0f, 0.0f,         // Rate Lock, Retrigger On Play, Stereo Mode, Depth Mode
        1.0f, 0.5f, 0.0f     // Wet, Output, Bypass
    }},
    { "Preset 3", {
        0.5f, 0.0f,          // Input, Presence
        0, 0, 0, 0,          // Sub/Bass/Mid/Treble Tremolo types
        1.0f,                // Master Rate
        5.0f, 5.0f, 5.0f, 5.0f,   // Sub/Bass/Mid/Treble Rates
        0.5f, 0.5f, 0.5f, 0.5f,   // Sub/Bass/Mid/Treble Depths
        0.0f, 0.0f, 0.0f, 0.0f, 0.5f,   // Phase Offset, Start Phase, Rate Offset, Depth Offset, Pulse Width
        0.0f, 1.0f, 2.0f, 0.0f,         // Rate Lock, Retrigger On Play, Stereo Mode, Depth Mode
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
        "RATE_LOCK", "RETRIGGER_ON_PLAY", "STEREO_MODE", "DEPTH_MODE",
        "WET", "OUTPUT_GAIN", "BYPASS"
    };

    for (int i = 0; i < (int)preset.values.size(); ++i)
        if (auto* param = apvts.getParameter(paramIDs[i]))
            param->setValueNotifyingHost(param->convertTo0to1(preset.values[i]));
}

void AutoTremolandoAudioProcessor::registerTapTempo()
{
    const double fNowMs = juce::Time::getMillisecondCounterHiRes();
    const double fPreviousTapMs = fLastTapTimeMs.exchange(fNowMs);

    if (fPreviousTapMs <= 0.0)
        return;

    const double fDeltaMs = fNowMs - fPreviousTapMs;
    if (fDeltaMs < 120.0 || fDeltaMs > 2000.0)
        return;

    const float fNewBpm = static_cast<float>(60000.0 / fDeltaMs);
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
}

const juce::String AutoTremolandoAudioProcessor::getProgramName(int index)
{
    return {};
}

void AutoTremolandoAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
}

//==============================================================================
void AutoTremolandoAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    int numChannels = getTotalNumInputChannels();
    int numOutputChannels = getTotalNumOutputChannels();

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

    for (int i = 0; i < numChannels; ++i)
    {
        subBass.add(new Filter());      subBass[i]->prepare(spec);
        bassLower.add(new Filter());    bassLower[i]->prepare(spec);
        bassUpper.add(new Filter());    bassUpper[i]->prepare(spec);
        midLower.add(new Filter());     midLower[i]->prepare(spec);
        midUpper.add(new Filter());     midUpper[i]->prepare(spec);
        treble.add(new Filter());       treble[i]->prepare(spec);
        resonanceFilter.add(new Filter()); resonanceFilter[i]->prepare(spec);
    }

    fPhaseOffset.assign(numOutputChannels, 0.0f);
    tremolo.reset(numChannels);

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

    for (int b = 0; b < 4; ++b)
    {
        smoothedRate[b].reset(sampleRate, 0.02);
        smoothedDepth[b].reset(sampleRate, 0.02);
        smoothedRate[b].setCurrentAndTargetValue(5.0f);
        smoothedDepth[b].setCurrentAndTargetValue(0.5f);
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
	auto totalNumInputChannels = getTotalNumInputChannels();
	auto numSamples = buffer.getNumSamples();

	const bool bBypass = *apvts.getRawParameterValue("BYPASS") > 0.5f;
	const bool bTempoSync = *apvts.getRawParameterValue("TEMPO_SYNC") > 0.5f;
	const bool bRateLock = *apvts.getRawParameterValue("RATE_LOCK") > 0.5f;
	const bool bRetriggerOnPlay = *apvts.getRawParameterValue("RETRIGGER_ON_PLAY") > 0.5f;
	const int iStereoMode = (int)*apvts.getRawParameterValue("STEREO_MODE");
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
		const float tempoMultipliers[] = {
			4.0f, 3.0f, 6.0f, 4.8f, 5.33f,
			2.0f, 1.5f, 3.0f, 2.4f, 2.67f,
			1.0f, 0.75f, 1.5f, 1.2f, 1.33f,
			0.5f, 0.375f, 0.75f, 0.6f, 0.67f,
			0.25f, 0.1875f, 0.375f, 0.3f, 0.33f,
			0.125f, 0.09375f, 0.1875f, 0.15f, 0.167f
		};
		const int divIdx[4] = {
			juce::jlimit(0, 29, (int)*apvts.getRawParameterValue("SUB_NOTE_DIV")),
			juce::jlimit(0, 29, (int)*apvts.getRawParameterValue("BASS_NOTE_DIV")),
			juce::jlimit(0, 29, (int)*apvts.getRawParameterValue("MID_NOTE_DIV")),
			juce::jlimit(0, 29, (int)*apvts.getRawParameterValue("TREBLE_NOTE_DIV"))
		};
		const float fBeatsPerSec = fSyncBpm / 60.0f;
		for (int b = 0; b < 4; ++b)
			fRate[b] = fBeatsPerSec * tempoMultipliers[divIdx[b]];
	}
	else
	{
		const int divIdx[4] = {
			juce::jlimit(0, 29, (int)*apvts.getRawParameterValue("SUB_NOTE_DIV")),
			juce::jlimit(0, 29, (int)*apvts.getRawParameterValue("BASS_NOTE_DIV")),
			juce::jlimit(0, 29, (int)*apvts.getRawParameterValue("MID_NOTE_DIV")),
			juce::jlimit(0, 29, (int)*apvts.getRawParameterValue("TREBLE_NOTE_DIV"))
		};
		for (int b = 0; b < 4; ++b)
			fRate[b] = 0.5f + (divIdx[b] / 29.0f) * 15.5f;
	}

	for (int b = 0; b < 4; ++b)
		fRate[b] = juce::jlimit(1.0f, 15.0f, fRate[b] * fMasterRate);

	if (bRateLock)
		for (int b = 1; b < 4; ++b)
			fRate[b] = fRate[0];

	fDepth[0] = *apvts.getRawParameterValue("SUB_TREM_DEPTH");
	fDepth[1] = *apvts.getRawParameterValue("BASS_TREM_DEPTH");
	fDepth[2] = *apvts.getRawParameterValue("MID_TREM_DEPTH");
	fDepth[3] = *apvts.getRawParameterValue("TREBLE_TREM_DEPTH");

	for (int b = 0; b < 4; ++b)
	{
		smoothedRate[b].setTargetValue(fRate[b]);
		smoothedDepth[b].setTargetValue(fDepth[b]);
	}

	const float fPhaseOffsetRadians = juce::degreesToRadians(fPhaseOffsetDegrees);
	if (iStereoMode == 0 || totalNumInputChannels <= 1)
	{
		for (int channel = 0; channel < totalNumInputChannels; ++channel)
			fPhaseOffset[channel] = 0.0f;
	}
	else if (iStereoMode == 1)
	{
		for (int channel = 0; channel < totalNumInputChannels; ++channel)
			fPhaseOffset[channel] = ((channel % 2) == 0) ? 0.0f : fPhaseOffsetRadians;
	}
	else if (totalNumInputChannels == 2)
	{
		fPhaseOffset[0] = 0.0f;
		fPhaseOffset[1] = fPhaseOffsetRadians;
	}
	else
	{
		for (int channel = 0; channel < totalNumInputChannels; ++channel)
			fPhaseOffset[channel] = fPhaseOffsetRadians * ((float)channel / (float)(totalNumInputChannels - 1));
	}

	float presenceFreq = fPresence * 19000.0f + 1000.0f;
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

	int iChoice[4];
	iChoice[0] = (int)*apvts.getRawParameterValue("SUB_TREMOLO");
	iChoice[1] = (int)*apvts.getRawParameterValue("BASS_TREMOLO");
	iChoice[2] = (int)*apvts.getRawParameterValue("MID_TREMOLO");
	iChoice[3] = (int)*apvts.getRawParameterValue("TREBLE_TREMOLO");

	float fInputPeak = 0.0f;
	float fOutputPeak = 0.0f;

	for (int channel = 0; channel < totalNumInputChannels; ++channel)
	{
		float fChannelScale = 0.0f;
		if (totalNumInputChannels > 1)
			fChannelScale = (float)channel / (float)(totalNumInputChannels - 1);

		auto* channelData = buffer.getWritePointer(channel);

		for (int iSample = 0; iSample < numSamples; ++iSample)
		{
			using SampleType = std::remove_reference_t<decltype(channelData[iSample])>;

			const float fInGain = smoothedInputGain.getNextValue();
			const float fOutGain = smoothedOutputGain.getNextValue();
			const float fWetDryControl = smoothedWet.getNextValue();
			const float fPulseWidth = smoothedPulseWidth.getNextValue();
			const float fBypassMix = smoothedBypass.getNextValue();

			float fChannelRate[4];
			float fChannelDepth[4];
			for (int b = 0; b < 4; ++b)
			{
				const float fSmoothRate = smoothedRate[b].getNextValue();
				const float fSmoothDepth = smoothedDepth[b].getNextValue();
				fChannelRate[b] = juce::jlimit(1.0f, 15.0f, fSmoothRate + (fRateOffset * fChannelScale));
				fChannelDepth[b] = juce::jlimit(0.0f, 1.0f, fSmoothDepth + (fDepthOffset * fChannelScale));
			}

			auto input = channelData[iSample];
			auto dry = input * static_cast<SampleType>(fInGain);
			auto wet = static_cast<SampleType>(resonanceFilter[channel]->processSample(static_cast<float>(dry)))
				* static_cast<SampleType>(1.0f - 0.41f);

			float fBand[4];
			fBand[0] = subBass[channel]->processSample(static_cast<float>(wet));
			fBand[1] = bassUpper[channel]->processSample(static_cast<float>(wet))
				+ bassLower[channel]->processSample(static_cast<float>(dry));
			fBand[2] = midUpper[channel]->processSample(static_cast<float>(wet))
				+ midLower[channel]->processSample(static_cast<float>(dry));
			fBand[3] = treble[channel]->processSample(static_cast<float>(wet));

			tremolo.processChannelBands(channel, fBand, fChannelDepth, iChoice, fChannelRate, fSampleRate, fPhaseOffset[channel], fPulseWidth, iDepthMode);

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
	auto totalNumInputChannels = getTotalNumInputChannels();
	auto numSamples = buffer.getNumSamples();

	const bool bBypass = *apvts.getRawParameterValue("BYPASS") > 0.5f;
	const bool bTempoSync = *apvts.getRawParameterValue("TEMPO_SYNC") > 0.5f;
	const bool bRateLock = *apvts.getRawParameterValue("RATE_LOCK") > 0.5f;
	const bool bRetriggerOnPlay = *apvts.getRawParameterValue("RETRIGGER_ON_PLAY") > 0.5f;
	const int iStereoMode = (int)*apvts.getRawParameterValue("STEREO_MODE");
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
		const float tempoMultipliers[] = {
			4.0f, 3.0f, 6.0f, 4.8f, 5.33f,
			2.0f, 1.5f, 3.0f, 2.4f, 2.67f,
			1.0f, 0.75f, 1.5f, 1.2f, 1.33f,
			0.5f, 0.375f, 0.75f, 0.6f, 0.67f,
			0.25f, 0.1875f, 0.375f, 0.3f, 0.33f,
			0.125f, 0.09375f, 0.1875f, 0.15f, 0.167f
		};
		const int divIdx[4] = {
			juce::jlimit(0, 29, (int)*apvts.getRawParameterValue("SUB_NOTE_DIV")),
			juce::jlimit(0, 29, (int)*apvts.getRawParameterValue("BASS_NOTE_DIV")),
			juce::jlimit(0, 29, (int)*apvts.getRawParameterValue("MID_NOTE_DIV")),
			juce::jlimit(0, 29, (int)*apvts.getRawParameterValue("TREBLE_NOTE_DIV"))
		};
		const float fBeatsPerSec = fSyncBpm / 60.0f;
		for (int b = 0; b < 4; ++b)
			fRate[b] = fBeatsPerSec * tempoMultipliers[divIdx[b]];
	}
	else
	{
		const int divIdx[4] = {
			juce::jlimit(0, 29, (int)*apvts.getRawParameterValue("SUB_NOTE_DIV")),
			juce::jlimit(0, 29, (int)*apvts.getRawParameterValue("BASS_NOTE_DIV")),
			juce::jlimit(0, 29, (int)*apvts.getRawParameterValue("MID_NOTE_DIV")),
			juce::jlimit(0, 29, (int)*apvts.getRawParameterValue("TREBLE_NOTE_DIV"))
		};
		for (int b = 0; b < 4; ++b)
			fRate[b] = 0.5f + (divIdx[b] / 29.0f) * 15.5f;
	}

	for (int b = 0; b < 4; ++b)
		fRate[b] = juce::jlimit(1.0f, 15.0f, fRate[b] * fMasterRate);

	if (bRateLock)
		for (int b = 1; b < 4; ++b)
			fRate[b] = fRate[0];

	fDepth[0] = *apvts.getRawParameterValue("SUB_TREM_DEPTH");
	fDepth[1] = *apvts.getRawParameterValue("BASS_TREM_DEPTH");
	fDepth[2] = *apvts.getRawParameterValue("MID_TREM_DEPTH");
	fDepth[3] = *apvts.getRawParameterValue("TREBLE_TREM_DEPTH");

	for (int b = 0; b < 4; ++b)
	{
		smoothedRate[b].setTargetValue(fRate[b]);
		smoothedDepth[b].setTargetValue(fDepth[b]);
	}

	const float fPhaseOffsetRadians = juce::degreesToRadians(fPhaseOffsetDegrees);
	if (iStereoMode == 0 || totalNumInputChannels <= 1)
	{
		for (int channel = 0; channel < totalNumInputChannels; ++channel)
			fPhaseOffset[channel] = 0.0f;
	}
	else if (iStereoMode == 1)
	{
		for (int channel = 0; channel < totalNumInputChannels; ++channel)
			fPhaseOffset[channel] = ((channel % 2) == 0) ? 0.0f : fPhaseOffsetRadians;
	}
	else if (totalNumInputChannels == 2)
	{
		fPhaseOffset[0] = 0.0f;
		fPhaseOffset[1] = fPhaseOffsetRadians;
	}
	else
	{
		for (int channel = 0; channel < totalNumInputChannels; ++channel)
			fPhaseOffset[channel] = fPhaseOffsetRadians * ((float)channel / (float)(totalNumInputChannels - 1));
	}

	float presenceFreq = fPresence * 19000.0f + 1000.0f;
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

	int iChoice[4];
	iChoice[0] = (int)*apvts.getRawParameterValue("SUB_TREMOLO");
	iChoice[1] = (int)*apvts.getRawParameterValue("BASS_TREMOLO");
	iChoice[2] = (int)*apvts.getRawParameterValue("MID_TREMOLO");
	iChoice[3] = (int)*apvts.getRawParameterValue("TREBLE_TREMOLO");

	float fInputPeak = 0.0f;
	float fOutputPeak = 0.0f;

	for (int channel = 0; channel < totalNumInputChannels; ++channel)
	{
		float fChannelScale = 0.0f;
		if (totalNumInputChannels > 1)
			fChannelScale = (float)channel / (float)(totalNumInputChannels - 1);

		auto* channelData = buffer.getWritePointer(channel);

		for (int iSample = 0; iSample < numSamples; ++iSample)
		{
			using SampleType = std::remove_reference_t<decltype(channelData[iSample])>;

			const float fInGain = smoothedInputGain.getNextValue();
			const float fOutGain = smoothedOutputGain.getNextValue();
			const float fWetDryControl = smoothedWet.getNextValue();
			const float fPulseWidth = smoothedPulseWidth.getNextValue();
			const float fBypassMix = smoothedBypass.getNextValue();

			float fChannelRate[4];
			float fChannelDepth[4];
			for (int b = 0; b < 4; ++b)
			{
				const float fSmoothRate = smoothedRate[b].getNextValue();
				const float fSmoothDepth = smoothedDepth[b].getNextValue();
				fChannelRate[b] = juce::jlimit(1.0f, 15.0f, fSmoothRate + (fRateOffset * fChannelScale));
				fChannelDepth[b] = juce::jlimit(0.0f, 1.0f, fSmoothDepth + (fDepthOffset * fChannelScale));
			}

			auto input = channelData[iSample];
			auto dry = input * static_cast<SampleType>(fInGain);
			auto wet = static_cast<SampleType>(resonanceFilter[channel]->processSample(static_cast<float>(dry)))
				* static_cast<SampleType>(1.0f - 0.41f);

			float fBand[4];
			fBand[0] = subBass[channel]->processSample(static_cast<float>(wet));
			fBand[1] = bassUpper[channel]->processSample(static_cast<float>(wet))
				+ bassLower[channel]->processSample(static_cast<float>(dry));
			fBand[2] = midUpper[channel]->processSample(static_cast<float>(wet))
				+ midLower[channel]->processSample(static_cast<float>(dry));
			fBand[3] = treble[channel]->processSample(static_cast<float>(wet));

			tremolo.processChannelBands(channel, fBand, fChannelDepth, iChoice, fChannelRate, fSampleRate, fPhaseOffset[channel], fPulseWidth, iDepthMode);

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


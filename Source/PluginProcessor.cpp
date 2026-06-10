/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "PluginExtra.h"

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

    juce::StringArray tremoloOptions = { "Sine", "Placeholder 2", "Placeholder 3", "Placeholder 4", "Placeholder 5" };

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

    // Per-band DEPTH
    params.push_back(std::make_unique<juce::AudioParameterFloat>("SUB_TREM_DEPTH", "Sub Depth", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("BASS_TREM_DEPTH", "Bass Depth", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("MID_TREM_DEPTH", "Mid Depth", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("TREBLE_TREM_DEPTH", "Treble Depth", 0.0f, 1.0f, 0.5f));

    // Wet & Presence
    params.push_back(std::make_unique<juce::AudioParameterFloat>("WET", "Wet", 0.0f, 1.0f, 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("PRESENCE", "Presence", 0.0f, 1.0f, 0.0f));

    return { params.begin(), params.end() };
}

//==============================================================================
void AutoTremolandoAudioProcessor::initPresets()
{
    presets = {
        { "Preset 1", {
            0, 0, 0, 0,     // Trem types
            0.5f, 0.5f,     // Input/Output
            5.0f, 5.0f, 5.0f, 5.0f,   // Rates
            0.5f, 0.5f, 0.5f, 0.5f,   // Depths
            1.0f, 0.0f      // Wet, Presence
        }},
        { "Preset 2", {
            0, 0, 0, 0,
            0.5f, 0.62f,
            8.0f, 6.0f, 4.0f, 2.0f,
            0.7f, 0.6f, 0.5f, 0.4f,
            1.0f, 0.0f
        }},
        { "Preset 3", {
            0, 0, 0, 0,
            0.75f, 1.0f,
            3.0f, 5.0f, 7.0f, 9.0f,
            0.7f, 0.6f, 0.5f, 0.4f,
            1.0f, 0.0f
        }}
    };
}

void AutoTremolandoAudioProcessor::loadPreset(int index)
{
    if (index < 0 || index >= presets.size()) return;

    auto& preset = presets[index];

    const char* paramIDs[] = {
        "SUB_TREMOLO", "BASS_TREMOLO", "MID_TREMOLO", "TREBLE_TREMOLO",
        "INPUT_GAIN", "OUTPUT_GAIN",
        "SUB_TREM_RATE", "BASS_TREM_RATE", "MID_TREM_RATE", "TREBLE_TREM_RATE",
        "SUB_TREM_DEPTH", "BASS_TREM_DEPTH", "MID_TREM_DEPTH", "TREBLE_TREM_DEPTH",
        "WET", "PRESENCE"
    };

    for (int i = 0; i < (int)preset.values.size(); ++i)
        if (auto* param = apvts.getParameter(paramIDs[i]))
            param->setValueNotifyingHost(param->convertTo0to1(preset.values[i]));
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

    for (int b = 0; b < 4; ++b)
        fPhasePos[b] = 0.0f;
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
// Tremolo waveform generator
static float getTremoloSample(float phase, int /*type*/, float depth)
{
    float osc = std::sin(phase);
    return (osc * depth * 0.5f) + 0.5f;
}

//==============================================================================
void AutoTremolandoAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto numSamples = buffer.getNumSamples();

    float fInGain = std::pow(*apvts.getRawParameterValue("INPUT_GAIN"), 3.0f);
    float fOutGain = std::pow(*apvts.getRawParameterValue("OUTPUT_GAIN"), 3.0f);
    float fWetDryControl = std::pow(*apvts.getRawParameterValue("WET"), 3.0f);
    float fPresence = *apvts.getRawParameterValue("PRESENCE");

    int iTremTypes[4] = {
        *apvts.getRawParameterValue("SUB_TREMOLO"),
        *apvts.getRawParameterValue("BASS_TREMOLO"),
        *apvts.getRawParameterValue("MID_TREMOLO"),
        *apvts.getRawParameterValue("TREBLE_TREMOLO")
    };

    fRate[0] = *apvts.getRawParameterValue("SUB_TREM_RATE");
    fRate[1] = *apvts.getRawParameterValue("BASS_TREM_RATE");
    fRate[2] = *apvts.getRawParameterValue("MID_TREM_RATE");
    fRate[3] = *apvts.getRawParameterValue("TREBLE_TREM_RATE");

    fDepth[0] = *apvts.getRawParameterValue("SUB_TREM_DEPTH");
    fDepth[1] = *apvts.getRawParameterValue("BASS_TREM_DEPTH");
    fDepth[2] = *apvts.getRawParameterValue("MID_TREM_DEPTH");
    fDepth[3] = *apvts.getRawParameterValue("TREBLE_TREM_DEPTH");

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

    const float fTwoPI = static_cast<float>(2.0f * M_PI);

    float fPhaseInc[4];
    for (int b = 0; b < 4; ++b)
        fPhaseInc[b] = (fTwoPI * fRate[b]) / fSampleRate;

    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);

        for (int iSample = 0; iSample < numSamples; ++iSample)
        {
            float fInput = channelData[iSample];
            float fDry = fInput * fInGain;

            float fWet = resonanceFilter[channel]->processSample(fDry) * (1.0f - 0.41f);

            float fBand[4];

            fBand[0] = subBass[channel]->processSample(fWet);
            fBand[1] = bassUpper[channel]->processSample(fWet)
                + bassLower[channel]->processSample(fDry);
            fBand[2] = midUpper[channel]->processSample(fWet)
                + midLower[channel]->processSample(fDry);
            fBand[3] = treble[channel]->processSample(fWet);

            for (int b = 0; b < 4; ++b)
            {
                fPhasePos[b] += fPhaseInc[b];
                if (fPhasePos[b] > fTwoPI)
                    fPhasePos[b] -= fTwoPI;

                float trem = getTremoloSample(fPhasePos[b], iTremTypes[b], fDepth[b]);
                fBand[b] *= trem;
            }

            float fSummedBands = fBand[0] + fBand[1] + fBand[2] + fBand[3];

            float fOutput = (fSummedBands * fWetDryControl) + (fDry * (1.0f - fWetDryControl));
            channelData[iSample] = fOutput * fOutGain;
        }
    }
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


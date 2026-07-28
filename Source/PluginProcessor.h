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

#pragma once

#include <JuceHeader.h>
#include "Effects/Modulation.h"
#include <array>
#include <atomic>
#include <vector>

//==============================================================================
class AutoTremolandoAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    AutoTremolandoAudioProcessor();
    ~AutoTremolandoAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
#endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    juce::UndoManager undoManager;

    struct Preset
    {
        juce::String sName;
        std::vector<float> fValues;
    };

    void loadPreset (int index);
    const std::vector<Preset>& getPresets() const { return presets; }

    void registerTapTempo();
    void resetParametersToDefaults();

    std::atomic<float> fInputMeterLevel { 0.0f };
    std::atomic<float> fOutputMeterLevel { 0.0f };

private:
    static constexpr int iBandCount = Modulation::iBandCount;

    struct FilterCoefficients
    {
        juce::dsp::IIR::Coefficients<float>::Ptr resonance;
        juce::dsp::IIR::Coefficients<float>::Ptr sub;
        juce::dsp::IIR::Coefficients<float>::Ptr bassLower;
        juce::dsp::IIR::Coefficients<float>::Ptr bassUpper;
        juce::dsp::IIR::Coefficients<float>::Ptr midLower;
        juce::dsp::IIR::Coefficients<float>::Ptr midUpper;
        juce::dsp::IIR::Coefficients<float>::Ptr treble;
    };

    std::vector<Preset> presets;
    void initPresets();
    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();

    static const std::array<const char*, iBandCount>& getRateParamIds();
    static const std::array<const char*, iBandCount>& getDepthParamIds();
    static const std::array<const char*, iBandCount>& getChoiceParamIds();
    static const std::array<const char*, iBandCount>& getNoteDivisionParamIds();

    float fSampleRate = 0.0f;

    std::atomic<float> fTapTempoBpm { 120.0f };
	std::atomic<double> dLastTapTimeMs{ 0.0 }; // Time of the last tap in milliseconds
    bool bWasPlaying = false;

    juce::OwnedArray<juce::dsp::IIR::Filter<float>> subBass, bassLower, bassUpper, midLower, midUpper, treble, resonanceFilter;

    Modulation mod;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AutoTremolandoAudioProcessor)
};
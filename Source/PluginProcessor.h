/*
  ==============================================================================

    Plugin: AutoTremolando
    GitHub: MontyWh
    Author: Montague Whishaw
    Date/Time: 24th April 2026

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
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
    void processBlock (juce::AudioBuffer<double>&, juce::MidiBuffer&) override;
    bool supportsDoublePrecisionProcessing() const override;

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
    float getInputMeterLevel() const;
    float getOutputMeterLevel() const;

private:
    static constexpr int iBandCount = 4;
    static constexpr int iNoteDivisionCount = 15;

    using BandFloatArray = std::array<float, iBandCount>;
    using BandIntArray = std::array<int, iBandCount>;

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

    template <typename SampleType>
    void processAudioBlock (juce::AudioBuffer<SampleType>& buffer);

    float fSampleRate = 0.0f;

    BandFloatArray fRate { 5.0f, 5.0f, 5.0f, 5.0f };
    BandFloatArray fDepth { 0.5f, 0.5f, 0.5f, 0.5f };

    std::vector<float> fPhaseOffset;

    std::vector<BandFloatArray> fPhasePos;
    BandFloatArray fPhaseInc { 0.0f, 0.0f, 0.0f, 0.0f };

    juce::LinearSmoothedValue<float> smoothedInputGain;
    juce::LinearSmoothedValue<float> smoothedOutputGain;
    juce::LinearSmoothedValue<float> smoothedWet;
    juce::LinearSmoothedValue<float> smoothedPulseWidth;
    juce::LinearSmoothedValue<float> smoothedBypass;
    juce::LinearSmoothedValue<float> smoothedRate[iBandCount];
    juce::LinearSmoothedValue<float> smoothedDepth[iBandCount];

    std::atomic<float> fTapTempoBpm { 120.0f };
    std::atomic<double> dLastTapTimeMs { 0.0 };
    std::atomic<float> fInputMeterLevel { 0.0f };
    std::atomic<float> fOutputMeterLevel { 0.0f };
    bool bWasPlaying = false;

    using Filter = juce::dsp::IIR::Filter<float>;
    using MultiChannelFilter = juce::OwnedArray<Filter>;

    MultiChannelFilter subBass, bassLower, bassUpper, midLower, midUpper, treble, resonanceFilter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AutoTremolandoAudioProcessor)
};
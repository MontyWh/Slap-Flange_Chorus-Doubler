/*
  ==============================================================================

    AutoTremolando processor declaration.
    This class owns the plugin parameter state, smoothing state, band filters,
    and the realtime tremolo DSP entry points used by the audio thread.

    Plugin: AutoTremolando
    GitHub: MontyWh
    Author: Montague Whishaw

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginExtra.h"
#include <atomic>

//==============================================================================
// Main audio engine: host lifecycle callbacks + multiband tremolo processing.
class AutoTremolandoAudioProcessor : public juce::AudioProcessor
{
public:
    AutoTremolandoAudioProcessor();
    ~AutoTremolandoAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlock(juce::AudioBuffer<double>&, juce::MidiBuffer&) override;
    bool supportsDoublePrecisionProcessing() const override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    juce::UndoManager undoManager;

    struct Preset {
        juce::String sName;
        std::vector<float> fValues;
    };

    void loadPreset(int index);
    const std::vector<Preset>& getPresets() const { return presets; }

    void registerTapTempo();
    void resetParametersToDefaults();
    float getInputMeterLevel() const;
    float getOutputMeterLevel() const;

private:
    std::vector<Preset> presets;
    void initPresets();
    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();

    float fSampleRate = 0.0f;

    TremoloDspUtils::BandFloatArray fRate { 5.0f, 5.0f, 5.0f, 5.0f };
    TremoloDspUtils::BandFloatArray fDepth { 0.5f, 0.5f, 0.5f, 0.5f };

    // Per-channel phase offset (sized in prepareToPlay)
    std::vector<float> fPhaseOffset;

    TremoloProcess tremolo;

    juce::LinearSmoothedValue<float> smoothedInputGain;
    juce::LinearSmoothedValue<float> smoothedOutputGain;
    juce::LinearSmoothedValue<float> smoothedWet;
    juce::LinearSmoothedValue<float> smoothedPulseWidth;
    juce::LinearSmoothedValue<float> smoothedBypass;
    juce::LinearSmoothedValue<float> smoothedRate[4];
    juce::LinearSmoothedValue<float> smoothedDepth[4];

    std::atomic<float> fTapTempoBpm { 120.0f };
    std::atomic<double> dLastTapTimeMs { 0.0 };
    std::atomic<float> fInputMeterLevel { 0.0f };
    std::atomic<float> fOutputMeterLevel { 0.0f };
    bool bWasPlaying = false;

    using Filter = juce::dsp::IIR::Filter<float>;
    using MultiChannelFilter = juce::OwnedArray<Filter>;

    MultiChannelFilter subBass, bassLower, bassUpper, midLower, midUpper, treble, resonanceFilter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutoTremolandoAudioProcessor)
};
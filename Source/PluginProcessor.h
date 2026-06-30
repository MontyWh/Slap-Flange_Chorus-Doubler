/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginExtra.h"

//==============================================================================
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
        juce::String name;
        std::vector<float> values;
    };

    void loadPreset(int index);
    const std::vector<Preset>& getPresets() const { return presets; }

private:
    std::vector<Preset> presets;
    void initPresets();
    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();

    float fSampleRate;

    float fRate[4] = { 5.0f, 5.0f, 5.0f, 5.0f };
    float fDepth[4] = { 0.5f, 0.5f, 0.5f, 0.5f };

    // Per-channel phase offset (sized in prepareToPlay)
    std::vector<float> fPhaseOffset;

    TremoloProcess tremolo;

    using Filter = juce::dsp::IIR::Filter<float>;
    using MultiChannelFilter = juce::OwnedArray<Filter>;

    MultiChannelFilter subBass, bassLower, bassUpper, midLower, midUpper, treble, resonanceFilter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutoTremolandoAudioProcessor)
};
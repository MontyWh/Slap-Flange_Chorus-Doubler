/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
*/
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
    void processFilters(float fSubBassGain, float fBassGain, float fMidGain, float fTrebleGain, int channel,
                         float fDry, float& fWet);
    void additionalProcess(float fSubBassGain, float fBassGain, float fMidGain, float fTrebleGain, float fMixDrop,
                           int channel, float& fWet, float fDry);
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
    juce::UndoManager undoManager; // Don't know where to use this??

    // Preset structure to hold values
    struct Preset {
        juce::String name;
        std::vector<float> values;
    };

    void loadPreset(int index);
    const std::vector<Preset>& getPresets() const { return presets; }

    //==============================================================================

private:
    //==============================================================================
    std::vector<Preset> presets;
    void initPresets(); // Define your hardcoded presets here

    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();

    //==============================================================================
    // Declare shared member variables here
    float fSampleRate;

    float fPhasePos = 0;

	// Use the base Filter class for per-iSample processing in a loop
// In PluginProcessor.h
    using Filter = juce::dsp::IIR::Filter<float>; // Use float for better performance

    // Create arrays for each filter band
    using MultiChannelFilter = juce::OwnedArray<Filter>;
	MultiChannelFilter subBass, bassLower, bassUpper, midLower, midUpper, treble, resonanceFilter;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AutoTremolandoAudioProcessor)
};

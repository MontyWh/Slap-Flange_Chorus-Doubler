/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
const int NUM_OF_PARAMETERS = 7; // Update this to add more parameters

//==============================================================================
/**
*/
class AutophonicAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    AutophonicAudioProcessorEditor (AutophonicAudioProcessor&);
    ~AutophonicAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    juce::Slider parameters[NUM_OF_PARAMETERS];
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> paramAttach[NUM_OF_PARAMETERS];
    juce::Label labels[NUM_OF_PARAMETERS]; // [New] Added labels
    
    juce::ComboBox presetMenu;
    juce::Label presetLabel;

    AutophonicAudioProcessor& audioProcessor; // This reference is provided as a quick way for your editor to access the processor object that created it.

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AutophonicAudioProcessorEditor)
};

/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
const int NUM_OF_PARAMETERS = 11; // Update this to add more parameters

//==============================================================================
/**
*/
class AutoTremolandoAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    AutoTremolandoAudioProcessorEditor (AutoTremolandoAudioProcessor&);
    ~AutoTremolandoAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    juce::Slider parameters[NUM_OF_PARAMETERS];
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> paramAttach[NUM_OF_PARAMETERS];
    juce::Label labels[NUM_OF_PARAMETERS]; // [New] Added labels
	juce::ComboBox subTremMenu, bassTremMenu, midTremMenu, trebleTremMenu;
    juce::ComboBox presetMenu;

    juce::Label subTremLabel, bassTremLabel, midTremLabel, trebleTremLabel, presetLabel;

    // Menu Attachments
    using MenuAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
	std::unique_ptr<MenuAttachment> subTremAttach, bassTremAttach, midTremAttach, trebleTremAttach;

    AutoTremolandoAudioProcessor& audioProcessor; // This reference is provided as a quick way for your editor to access the processor object that created it.

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AutoTremolandoAudioProcessorEditor)
};

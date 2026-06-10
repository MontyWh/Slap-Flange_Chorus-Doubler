/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
// 16 parameters total (menus + gains + per-band rate/depth + wet/presence)
//==============================================================================
const int NUM_OF_PARAMETERS = 16;

//==============================================================================
class AutoTremolandoAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    AutoTremolandoAudioProcessorEditor(AutoTremolandoAudioProcessor&);
    ~AutoTremolandoAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    // Sliders for all 16 parameters
    juce::Slider parameters[NUM_OF_PARAMETERS];
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> paramAttach[NUM_OF_PARAMETERS];
    juce::Label labels[NUM_OF_PARAMETERS];

    // Tremolo type menus
    juce::ComboBox subTremMenu, bassTremMenu, midTremMenu, trebleTremMenu;
    juce::Label subTremLabel, bassTremLabel, midTremLabel, trebleTremLabel;

    using MenuAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<MenuAttachment> subTremAttach, bassTremAttach, midTremAttach, trebleTremAttach;

    // Preset menu
    juce::ComboBox presetMenu;
    juce::Label presetLabel;

    AutoTremolandoAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutoTremolandoAudioProcessorEditor)
};
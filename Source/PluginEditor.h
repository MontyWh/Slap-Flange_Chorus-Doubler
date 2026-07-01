/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
// 22 parameters total (input + presence + tremolo types + master rate + rates + depths + offsets + pulse + wet + output + bypass)
//==============================================================================
const int NUM_OF_PARAMETERS = 22;

//==============================================================================
class AutoTremolandoAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    AutoTremolandoAudioProcessorEditor(AutoTremolandoAudioProcessor&);
    ~AutoTremolandoAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    // Sliders for all 21 parameters
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

    // Bypass button
    juce::TextButton bypassButton;
    juce::Label bypassLabel;

    // Sync mode switch (Time vs Tempo)
    juce::TextButton tempoSyncSlider;
    juce::Label tempoSyncLabel;
    bool bCurrentSync = true;  // Track sync state for label updates

    // Per-band note-division/time rotary sliders (active in both modes)
    juce::Slider subNoteDivSlider, bassNoteDivSlider, midNoteDivSlider, trebleNoteDivSlider;
    juce::Label subNoteDivLabel, subNoteDivValueLabel, bassNoteDivLabel, bassNoteDivValueLabel,
                midNoteDivLabel, midNoteDivValueLabel, trebleNoteDivLabel, trebleNoteDivValueLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> 
        subNoteDivAttach, bassNoteDivAttach, midNoteDivAttach, trebleNoteDivAttach;

    AutoTremolandoAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutoTremolandoAudioProcessorEditor)
};
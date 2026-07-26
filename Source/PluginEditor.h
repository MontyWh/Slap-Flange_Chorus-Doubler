/*
  ==============================================================================

    Plugin: AutoTremolando
    GitHub: MontyWh
    Author: Montague Whishaw
    Date/Time: 24th April 2026
    General Language: English (UK)

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
const int iNumParameters = 22;

class AutoTremolandoAudioProcessorEditor  : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    AutoTremolandoAudioProcessorEditor (AutoTremolandoAudioProcessor&);
    ~AutoTremolandoAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    void updateChannelSpreadUiState();

    juce::Slider parameters[iNumParameters];
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> paramAttach[iNumParameters];
    juce::Label labels[iNumParameters];

    Modulation modulation;

    using MenuAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    juce::ComboBox presetMenu;
    juce::Label presetLabel;

    juce::TextButton tapTempoButton;
    juce::TextButton resetDefaultsButton;

    juce::TextButton bypassButton;
    juce::Label bypassLabel;

    bool bCurrentSync = true;
    bool bUpdatingLinkedRates = false;

    double dInputMeterDisplay = 0.0;
    double dOutputMeterDisplay = 0.0;
    juce::ProgressBar inputMeterBar;
    juce::ProgressBar outputMeterBar;
    juce::Label inputMeterLabel, outputMeterLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        subNoteDivAttach, bassNoteDivAttach, midNoteDivAttach, trebleNoteDivAttach;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> startPhaseAttach;

    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    AutoTremolandoAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AutoTremolandoAudioProcessorEditor)
};

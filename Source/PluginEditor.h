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
// JUCE type aliases
using Slider = juce::Slider;
using Label = juce::Label;
using TextButton = juce::TextButton;
using ComboBox = juce::ComboBox;
using ProgressBar = juce::ProgressBar;
using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

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

    Slider parameters[iNumParameters];
    std::unique_ptr<SliderAttachment> paramAttach[iNumParameters];
    Label labels[iNumParameters];

    Modulation modulation;

    ComboBox presetMenu;
    Label presetLabel;

    TextButton tapTempoButton;
    TextButton resetDefaultsButton;

    TextButton bypassButton;
    Label bypassLabel;

    bool bCurrentSync = true;
    bool bUpdatingLinkedRates = false;

    float fInputMeterDisplay = 0.0f;
    float fOutputMeterDisplay = 0.0f;
    double dInputMeterBarValue = 0.0;
    double dOutputMeterBarValue = 0.0;
    ProgressBar inputMeterBar;
    ProgressBar outputMeterBar;
    Label inputMeterLabel, outputMeterLabel;

    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    AutoTremolandoAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AutoTremolandoAudioProcessorEditor)
};

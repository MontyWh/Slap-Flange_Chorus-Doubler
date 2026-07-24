/*
  ==============================================================================

    Plugin: AutoTremolando
    GitHub: MontyWh
    Author: Montague Whishaw
    Date/Time: 24th April 2026

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

    juce::ComboBox subTremMenu, bassTremMenu, midTremMenu, trebleTremMenu;
    juce::Label subTremLabel, bassTremLabel, midTremLabel, trebleTremLabel;

    using MenuAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<MenuAttachment> subTremAttach, bassTremAttach, midTremAttach, trebleTremAttach;

    juce::ComboBox presetMenu;
    juce::Label presetLabel;

    juce::TextButton tapTempoButton;
    juce::TextButton resetDefaultsButton;

    juce::TextButton bypassButton;
    juce::Label bypassLabel;

    juce::TextButton tempoSyncSlider;
    juce::Label tempoSyncLabel;
    bool bCurrentSync = true;
    bool bUpdatingLinkedRates = false;

    juce::TextButton rateLockButton;
    juce::TextButton retriggerButton;
    juce::Label rateLockLabel, retriggerLabel;

    juce::Slider surroundWidthSlider;
    juce::Label surroundWidthLabel;
    juce::ComboBox depthModeMenu;
    juce::Label depthModeLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> rateLockAttach, retriggerAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> surroundWidthAttach;
    std::unique_ptr<MenuAttachment> depthModeAttach;

    double dInputMeterDisplay = 0.0;
    double dOutputMeterDisplay = 0.0;
    juce::ProgressBar inputMeterBar;
    juce::ProgressBar outputMeterBar;
    juce::Label inputMeterLabel, outputMeterLabel;

    juce::Slider subNoteDivSlider, bassNoteDivSlider, midNoteDivSlider, trebleNoteDivSlider;
    juce::Label subNoteDivLabel, subNoteDivValueLabel, bassNoteDivLabel, bassNoteDivValueLabel,
                midNoteDivLabel, midNoteDivValueLabel, trebleNoteDivLabel, trebleNoteDivValueLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        subNoteDivAttach, bassNoteDivAttach, midNoteDivAttach, trebleNoteDivAttach;

    juce::Slider startPhaseSlider;
    juce::Label startPhaseLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> startPhaseAttach;

    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    AutoTremolandoAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AutoTremolandoAudioProcessorEditor)
};

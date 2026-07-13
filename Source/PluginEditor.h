/*
  ==============================================================================

    AutoTremolando editor declaration.
    This UI binds controls to APVTS parameters and exposes tremolo workflow
    controls such as wave type, sync, rate/depth offsets, and metering.

    Plugin: AutoTremolando
    GitHub: MontyWh
    Author: Montague Whishaw

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
// Number of rotary slider parameters presented in the main panel.
// (Additional controls like sync mode or note division are separate widgets.)
//==============================================================================
const int iNumParameters = 22;

//==============================================================================
class AutoTremolandoAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    AutoTremolandoAudioProcessorEditor(AutoTremolandoAudioProcessor&);
    ~AutoTremolandoAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    void updateChannelSpreadUiState();
    // Sliders for all 22 parameters
    juce::Slider parameters[iNumParameters];
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> paramAttach[iNumParameters];
    juce::Label labels[iNumParameters];

    // Tremolo type menus
    juce::ComboBox subTremMenu, bassTremMenu, midTremMenu, trebleTremMenu;
    juce::Label subTremLabel, bassTremLabel, midTremLabel, trebleTremLabel;

    using MenuAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<MenuAttachment> subTremAttach, bassTremAttach, midTremAttach, trebleTremAttach;

    // Preset menu
    juce::ComboBox presetMenu;
    juce::Label presetLabel;

    // Utility controls
    juce::TextButton tapTempoButton;
    juce::TextButton resetDefaultsButton;

    // Bypass button
    juce::TextButton bypassButton;
    juce::Label bypassLabel;

    // Sync mode switch (Time vs Tempo)
    juce::TextButton tempoSyncSlider;
    juce::Label tempoSyncLabel;
    bool bCurrentSync = true;  // Track sync state for label updates
    bool bUpdatingLinkedRates = false;

    // Basic mode controls
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

    // Basic meter display
    double dInputMeterDisplay = 0.0;
    double dOutputMeterDisplay = 0.0;
    juce::ProgressBar inputMeterBar;
    juce::ProgressBar outputMeterBar;
    juce::Label inputMeterLabel, outputMeterLabel;

    // Per-band note-division/time rotary sliders (active in both modes)
    juce::Slider subNoteDivSlider, bassNoteDivSlider, midNoteDivSlider, trebleNoteDivSlider;
    juce::Label subNoteDivLabel, subNoteDivValueLabel, bassNoteDivLabel, bassNoteDivValueLabel,
                midNoteDivLabel, midNoteDivValueLabel, trebleNoteDivLabel, trebleNoteDivValueLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> 
        subNoteDivAttach, bassNoteDivAttach, midNoteDivAttach, trebleNoteDivAttach;

    juce::Slider startPhaseSlider;
    juce::Label startPhaseLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> startPhaseAttach;

    AutoTremolandoAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutoTremolandoAudioProcessorEditor)
};
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
    //==============================================================================
    // Editor lifecycle and host repaint/timer callbacks
    //==============================================================================
    AutoTremolandoAudioProcessorEditor(AutoTremolandoAudioProcessor&);
    ~AutoTremolandoAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    //==============================================================================
    // UI state synchronisation helpers
    //==============================================================================
    void updateChannelSpreadUiState();

    //==============================================================================
    // Main parameter controls and APVTS attachments
    //==============================================================================
    // Sliders for all 22 parameters
    juce::Slider parameters[iNumParameters];
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> paramAttach[iNumParameters];
    juce::Label labels[iNumParameters];

    //==============================================================================
    // Per-band waveform selection controls
    //======================================================================
    // Tremolo type menus
    juce::ComboBox subTremMenu, bassTremMenu, midTremMenu, trebleTremMenu;
    juce::Label subTremLabel, bassTremLabel, midTremLabel, trebleTremLabel;

    using MenuAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<MenuAttachment> subTremAttach, bassTremAttach, midTremAttach, trebleTremAttach;

    //==============================================================================
    // Preset and quick utility actions
    //==============================================================================
    // Preset menu
    juce::ComboBox presetMenu;
    juce::Label presetLabel;

    // Utility controls
    juce::TextButton tapTempoButton;
    juce::TextButton resetDefaultsButton;

    //==============================================================================
    // Transport/interaction toggles
    //==============================================================================
    // Bypass button
    juce::TextButton bypassButton;
    juce::Label bypassLabel;

    // Sync mode switch (Time vs Tempo)
    juce::TextButton tempoSyncSlider;
    juce::Label tempoSyncLabel;
    bool bCurrentSync = true;  // Track sync state for label updates
    bool bUpdatingLinkedRates = false;

    //==============================================================================
    // Mode-specific controls and routing options
    //==============================================================================
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

    //==============================================================================
    // Meter bridge state (processor -> editor display)
    //==============================================================================
    // Basic meter display
    double dInputMeterDisplay = 0.0;
    double dOutputMeterDisplay = 0.0;
    juce::ProgressBar inputMeterBar;
    juce::ProgressBar outputMeterBar;
    juce::Label inputMeterLabel, outputMeterLabel;

    //==============================================================================
    // Tempo/time per-band controls
    //==============================================================================
    // Per-band note-division/time rotary sliders (active in both modes)
    juce::Slider subNoteDivSlider, bassNoteDivSlider, midNoteDivSlider, trebleNoteDivSlider;
    juce::Label subNoteDivLabel, subNoteDivValueLabel, bassNoteDivLabel, bassNoteDivValueLabel,
                midNoteDivLabel, midNoteDivValueLabel, trebleNoteDivLabel, trebleNoteDivValueLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> 
        subNoteDivAttach, bassNoteDivAttach, midNoteDivAttach, trebleNoteDivAttach;

    //==============================================================================
    // LFO phase controls
    //==============================================================================
    juce::Slider startPhaseSlider;
    juce::Label startPhaseLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> startPhaseAttach;

    //==============================================================================
    // Owning processor reference
    //==============================================================================
    AutoTremolandoAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutoTremolandoAudioProcessorEditor)
};
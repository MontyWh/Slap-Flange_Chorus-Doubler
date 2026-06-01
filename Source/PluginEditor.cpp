/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AutophonicAudioProcessorEditor::AutophonicAudioProcessorEditor (AutophonicAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.

    // Define the names for the labels (should match the display names in createParameters)
    juce::StringArray labelNames = {
        "In Gain", "Out Gain",
        "Sub-Bass", "Bass", "Mid", "Treble",
        "Wet"
    };

    for (int i = 0; i < NUM_OF_PARAMETERS; ++i)
    {
        parameters[i].setSliderStyle(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag);
        parameters[i].setTextBoxStyle(juce::Slider::TextBoxBelow, true, 80, 20);
        addAndMakeVisible(parameters[i]);

        // [New] Setup Labels
        labels[i].setText(labelNames[i], juce::dontSendNotification);
        labels[i].setJustificationType(juce::Justification::centred);
        labels[i].attachToComponent(&parameters[i], false); // false = place label above/below slider
        addAndMakeVisible(labels[i]);
    }

    for (int i = 0; i < NUM_OF_PARAMETERS; ++i)
    {
        parameters[i].setSliderStyle(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag);
        parameters[i].setTextBoxStyle(juce::Slider::TextBoxBelow, true, 100, 50);
        addAndMakeVisible(parameters[i]);
    }

	paramAttach[0] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "INPUT_GAIN", parameters[0]);
	paramAttach[1] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "OUTPUT_GAIN", parameters[1]);

	paramAttach[2] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "SUB-BASS", parameters[2]);
	paramAttach[3] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "BASS", parameters[3]);
	paramAttach[4] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "MID", parameters[4]);
	paramAttach[5] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "TREBLE", parameters[5]);

	paramAttach[6] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "WET", parameters[6]);

    // Setup Preset Menu
    addAndMakeVisible(presetMenu);
    presetMenu.setText("Select Preset...");

    int id = 1;
    for (auto& p : audioProcessor.getPresets())
        presetMenu.addItem(p.name, id++);

    presetMenu.onChange = [this]() {
        audioProcessor.loadPreset(presetMenu.getSelectedItemIndex());
        };

    // Setup Preset Menu Label
    presetLabel.setText("Preset:", juce::dontSendNotification);
    presetLabel.setJustificationType(juce::Justification::left);
    presetLabel.attachToComponent(&presetMenu, true);
    addAndMakeVisible(presetLabel);

    setSize (750, 500);
}

AutophonicAudioProcessorEditor::~AutophonicAudioProcessorEditor()
{
}

//==============================================================================
void AutophonicAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::black);
    //g.setFont (juce::FontOptions (15.0f));
    //g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
}

void AutophonicAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor...

// --- Top Bar (Presets) ---
    presetMenu.setBounds(80, 10, 220, 25);

    // --- Top Row (Gain Controls) ---
    parameters[0].setBounds(20, 60, 80, 90);    // InGain
    parameters[1].setBounds(110, 60, 80, 90);   // OutGain
    parameters[6].setBounds(200, 60, 80, 90);   // Wet

    // --- Middle Row (Band EQ / Tonal Controls) ---
    parameters[2].setBounds(20, 190, 75, 85);   // SubBass
    parameters[3].setBounds(105, 190, 75, 85);  // Bass
    parameters[4].setBounds(190, 190, 75, 85);  // Mid
    parameters[5].setBounds(275, 190, 75, 85);  // Treble
}

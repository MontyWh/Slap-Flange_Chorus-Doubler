/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
DistortionPlusAudioProcessorEditor::DistortionPlusAudioProcessorEditor (DistortionPlusAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.

    // Define the names for the labels (should match the display names in createParameters)
    juce::StringArray labelNames = {
        "In Gain", "Out Gain", "Distortion",
        "Sub-Bass", "Bass", "Mid", "Treble",
        "Wet", "Presence", "Noise Gate",
        "Lo-Fi Blend", "Gate Reduc"
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
	paramAttach[2] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "DISTORTION", parameters[2]);

	paramAttach[3] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "SUB-BASS", parameters[3]);
	paramAttach[4] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "BASS", parameters[4]);
	paramAttach[5] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "MID", parameters[5]);
	paramAttach[6] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "TREBLE", parameters[6]);

	paramAttach[7] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "WET", parameters[7]);
	paramAttach[8] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "PRESENCE", parameters[8]);
	paramAttach[9] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "NOISE_GATE", parameters[9]);

	paramAttach[10] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "LO-FI_BLEND", parameters[10]);
	paramAttach[11] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "GATE_REDUCTION", parameters[11]);

    // Setup Menus
    auto setupMenu = [this](juce::ComboBox& box, std::unique_ptr<MenuAttachment>& attach, juce::String paramID)
        {
            auto* param = dynamic_cast<juce::AudioParameterChoice*>(audioProcessor.apvts.getParameter(paramID));
            if (param != nullptr)
            {
                box.addItemList(param->choices, 1);
                addAndMakeVisible(box);
                attach = std::make_unique<MenuAttachment>(audioProcessor.apvts, paramID, box);
            }
        };

    setupMenu(subDistortMenu, subDistortAttach, "SUB_DISTORT");
    setupMenu(bassDistortMenu, bassDistortAttach, "BASS_DISTORT");
    setupMenu(midDistortMenu, midDistortAttach, "MID_DISTORT");
    setupMenu(trebleDistortMenu, trebleDistortAttach, "TREBLE_DISTORT");
    setupMenu(loFiMenu, loFiAttach, "LO-FI_TYPE");

    // Helper to setup Menu Labels
    auto setupMenuLabel = [this](juce::Label& label, juce::String text, juce::Component& attachTo) {
        label.setText(text, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::left);
        label.attachToComponent(&attachTo, true); // true = place label to the left of the menu
        addAndMakeVisible(label);
        };

    // Link labels to menus
    setupMenuLabel(presetLabel, "Preset:", presetMenu);
    setupMenuLabel(subDistLabel, "Sub-Bass Type:", subDistortMenu);
    setupMenuLabel(bassDistLabel, "Bass Type:", bassDistortMenu);
    setupMenuLabel(midDistLabel, "Mid Type:", midDistortMenu);
    setupMenuLabel(trebleDistLabel, "Treble Type:", trebleDistortMenu);
    setupMenuLabel(loFiLabel, "Lo-Fi Effect:", loFiMenu);

    // Setup Preset Menu
    addAndMakeVisible(presetMenu);
    presetMenu.setText("Select Preset...");

    int id = 1;
    for (auto& p : audioProcessor.getPresets())
        presetMenu.addItem(p.name, id++);

    presetMenu.onChange = [this]() {
        audioProcessor.loadPreset(presetMenu.getSelectedItemIndex());
        };


    setSize (750, 500);
}

DistortionPlusAudioProcessorEditor::~DistortionPlusAudioProcessorEditor()
{
}

//==============================================================================
void DistortionPlusAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::black);
    //g.setFont (juce::FontOptions (15.0f));
    //g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
}

void DistortionPlusAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor...

// --- Top Bar (Presets) ---
    presetMenu.setBounds(80, 10, 220, 25);

    // --- Left Column / Top Row (Gain & Main Distortion) ---
    // Spread out horizontally by 80 pixels instead of 50
    parameters[0].setBounds(20, 60, 80, 90);    // InGain
    parameters[1].setBounds(110, 60, 80, 90);   // OutGain
    parameters[2].setBounds(200, 60, 80, 90);   // Distortion

    parameters[7].setBounds(300, 60, 80, 90);   // Wet (Moved next to Main Dist)
    parameters[9].setBounds(390, 60, 80, 90);   // NoiseGate (Moved to top row)

    // --- Middle Row (Band EQ / Tonal Controls) ---
    // Increased Y and X spacing for clarity
    parameters[3].setBounds(20, 190, 75, 85);   // SubBass
    parameters[4].setBounds(105, 190, 75, 85);  // Bass
    parameters[5].setBounds(190, 190, 75, 85);  // Mid
    parameters[6].setBounds(275, 190, 75, 85);  // Treble
    parameters[8].setBounds(360, 190, 75, 85);  // Presence

    // --- Right Column (Distortion Selection Menus) ---
        // Labels will automatically sit to the left of these bounds
    int menuX = 600;
    int menuWidth = 140; // Slightly narrowed to ensure labels don't hit the sliders
    subDistortMenu.setBounds(menuX, 40, menuWidth, 30);
    bassDistortMenu.setBounds(menuX, 90, menuWidth, 30);
    midDistortMenu.setBounds(menuX, 140, menuWidth, 30);
    trebleDistortMenu.setBounds(menuX, 190, menuWidth, 30);

    // --- Bottom Section (Lo-Fi Controls) ---
    // Moved x to 100 to give "Lo-Fi Effect:" label room
    loFiMenu.setBounds(100, 420, 150, 30);
    parameters[10].setBounds(310, 350, 80, 90); // Lo-Fi Blend
    parameters[11].setBounds(510, 350, 80, 90); // Gate Reduction
}

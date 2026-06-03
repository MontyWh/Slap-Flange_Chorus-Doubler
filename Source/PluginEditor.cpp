/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AutoTremolandoAudioProcessorEditor::AutoTremolandoAudioProcessorEditor (AutoTremolandoAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.

    // Define the names for the labels (should match the display names in createParameters)
    juce::StringArray labelNames = {
        "In Gain", "Out Gain", "Wet/Dry",
		"Sub-Bass Depth", "Sub-Bass Rate", "Bass Depth", "Bass Rate", "Mid Depth", "Mid Rate", "Treble Depth", "Treble Rate"
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

	paramAttach[0] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "INPUT_GAIN", parameters[0]);
	paramAttach[1] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "OUTPUT_GAIN", parameters[1]);
	paramAttach[2] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "WET", parameters[2]);

	paramAttach[3] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "SUB-BASS_DEPTH", parameters[3]);
    paramAttach[4] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "SUB-BASS_RATE", parameters[4]);

	paramAttach[5] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "BASS_DEPTH", parameters[5]);
	paramAttach[6] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "BASS_RATE", parameters[6]);

	paramAttach[7] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "MID_DEPTH", parameters[7]);
	paramAttach[8] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "MID_RATE", parameters[8]);

	paramAttach[9] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "TREBLE_DEPTH", parameters[9]);
	paramAttach[10] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "TREBLE_RATE", parameters[10]);

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

    setupMenu(subTremMenu, subTremAttach, "SUB-BASS_TREMOLO");
    setupMenu(bassTremMenu, bassTremAttach, "BASS_WAVE_TREMOLO");
    setupMenu(midTremMenu, midTremAttach, "MID_WAVE_TREMOLO");
    setupMenu(trebleTremMenu, trebleTremAttach, "TREBLE_WAVE_TREMOLO");

    // Helper to setup Menu Labels
    auto setupMenuLabel = [this](juce::Label& label, juce::String text, juce::Component& attachTo) {
        label.setText(text, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::left);
        label.attachToComponent(&attachTo, true); // true = place label to the left of the menu
        addAndMakeVisible(label);
        };

    // Link labels to menus  
    setupMenuLabel(presetLabel, "Preset:", presetMenu);
    setupMenuLabel(subTremLabel, "Sub-Bass Type:", subTremMenu);
    setupMenuLabel(bassTremLabel, "Bass Type:", bassTremMenu);
    setupMenuLabel(midTremLabel, "Mid Type:", midTremMenu);
    setupMenuLabel(trebleTremLabel, "Treble Type:", trebleTremMenu);

    // Setup Preset Menu
    addAndMakeVisible(presetMenu);
    presetMenu.setText("Select Preset...");

    int id = 1;
    for (auto& p : audioProcessor.getPresets())
        presetMenu.addItem(p.name, id++);

    presetMenu.onChange = [this]() {
        audioProcessor.loadPreset(presetMenu.getSelectedItemIndex());
        };


    setSize(750, 500);
}

AutoTremolandoAudioProcessorEditor::~AutoTremolandoAudioProcessorEditor()
{
}

//==============================================================================
void AutoTremolandoAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::black);
    //g.setFont (juce::FontOptions (15.0f));
    //g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
}

void AutoTremolandoAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor...

// --- Top Bar (Presets) ---
    presetMenu.setBounds(80, 10, 220, 25);

    // --- Top Row (Gain Controls) ---
    parameters[0].setBounds(20, 60, 80, 90);    // InGain
    parameters[1].setBounds(110, 60, 80, 90);   // OutGain
    parameters[2].setBounds(200, 60, 80, 90);   // Wet

    // --- Middle Row (Band EQ / Tonal Controls) ---
    parameters[3].setBounds(20, 190, 75, 85);   // Sub-Bass Depth
    parameters[4].setBounds(105, 190, 75, 85);  // Sub-Bass Rate
    parameters[5].setBounds(190, 190, 75, 85);  // Bass Depth
	parameters[6].setBounds(275, 190, 75, 85);  // Bass Rate
    parameters[7].setBounds(360, 190, 75, 85);  // Mid Depth
	parameters[8].setBounds(445, 190, 75, 85);  // Mid Rate
	parameters[9].setBounds(530, 190, 75, 85); // Treble Depth
	parameters[10].setBounds(615, 190, 75, 85); // Treble Rate

    // --- Right Column (Tremolo Type Menus) ---
    int menuX = 600;
    int menuWidth = 140;
    subTremMenu.setBounds(menuX, 40, menuWidth, 30);
    bassTremMenu.setBounds(menuX, 90, menuWidth, 30);
    midTremMenu.setBounds(menuX, 140, menuWidth, 30);
    trebleTremMenu.setBounds(menuX, 190, menuWidth, 30);
}

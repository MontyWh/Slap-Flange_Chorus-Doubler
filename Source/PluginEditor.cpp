#include "PluginEditor.h"
#include "PluginProcessor.h"

//==============================================================================
// Helper for consistent slider setup
static void setupSlider(juce::Slider& s)
{
    s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
}

//==============================================================================

AutoTremolandoAudioProcessorEditor::AutoTremolandoAudioProcessorEditor(AutoTremolandoAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(900, 600);

    //======================================================================
    // Parameter IDs in the exact order used by the processor
    //======================================================================
    const juce::String paramIDs[NUM_OF_PARAMETERS] =
    {
        "SUB_TREMOLO", "BASS_TREMOLO", "MID_TREMOLO", "TREBLE_TREMOLO",
        "INPUT_GAIN", "OUTPUT_GAIN",
        "SUB_TREM_RATE", "BASS_TREM_RATE", "MID_TREM_RATE", "TREBLE_TREM_RATE",
        "SUB_TREM_DEPTH", "BASS_TREM_DEPTH", "MID_TREM_DEPTH", "TREBLE_TREM_DEPTH",
        "WET", "PRESENCE"
    };

    const juce::String paramLabels[NUM_OF_PARAMETERS] =
    {
        "Sub Trem Type", "Bass Trem Type", "Mid Trem Type", "Treble Trem Type",
        "Input", "Output",
        "Sub Rate", "Bass Rate", "Mid Rate", "Treble Rate",
        "Sub Depth", "Bass Depth", "Mid Depth", "Treble Depth",
        "Wet", "Presence"
    };

    //======================================================================
    // Create sliders + labels
    //======================================================================
    for (int i = 0; i < NUM_OF_PARAMETERS; ++i)
    {
        setupSlider(parameters[i]);
        addAndMakeVisible(parameters[i]);

        labels[i].setText(paramLabels[i], juce::dontSendNotification);
        labels[i].setJustificationType(juce::Justification::centred);
        addAndMakeVisible(labels[i]);

        paramAttach[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.apvts, paramIDs[i], parameters[i]);
    }

    //======================================================================
    // Tremolo type menus
    //======================================================================
    juce::ComboBox* menus[4] =
    {
        &subTremMenu, &bassTremMenu, &midTremMenu, &trebleTremMenu
    };
    const juce::StringArray waveNames{ "Sine", "Triangle", "Sawtooth", "Pulse", "Square" };
    for (auto* m : menus) m->addItemList(waveNames, 1);


    addAndMakeVisible(subTremMenu);
    addAndMakeVisible(bassTremMenu);
    addAndMakeVisible(midTremMenu);
    addAndMakeVisible(trebleTremMenu);

    subTremLabel.setText("Sub Type", juce::dontSendNotification);
    bassTremLabel.setText("Bass Type", juce::dontSendNotification);
    midTremLabel.setText("Mid Type", juce::dontSendNotification);
    trebleTremLabel.setText("Treble Type", juce::dontSendNotification);

    subTremLabel.setJustificationType(juce::Justification::centred);
    bassTremLabel.setJustificationType(juce::Justification::centred);
    midTremLabel.setJustificationType(juce::Justification::centred);
    trebleTremLabel.setJustificationType(juce::Justification::centred);

    addAndMakeVisible(subTremLabel);
    addAndMakeVisible(bassTremLabel);
    addAndMakeVisible(midTremLabel);
    addAndMakeVisible(trebleTremLabel);

    subTremAttach = std::make_unique<MenuAttachment>(audioProcessor.apvts, "SUB_TREMOLO", subTremMenu);
    bassTremAttach = std::make_unique<MenuAttachment>(audioProcessor.apvts, "BASS_TREMOLO", bassTremMenu);
    midTremAttach = std::make_unique<MenuAttachment>(audioProcessor.apvts, "MID_TREMOLO", midTremMenu);
    trebleTremAttach = std::make_unique<MenuAttachment>(audioProcessor.apvts, "TREBLE_TREMOLO", trebleTremMenu);

    //======================================================================
    // Preset menu
    //======================================================================
    presetLabel.setText("Presets", juce::dontSendNotification);
    presetLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(presetLabel);

    presetMenu.addItem("Preset 1", 1);
    presetMenu.addItem("Preset 2", 2);
    presetMenu.addItem("Preset 3", 3);
    addAndMakeVisible(presetMenu);

    presetMenu.onChange = [this]()
        {
            int index = presetMenu.getSelectedId() - 1;
            audioProcessor.loadPreset(index);
        };
}

//==============================================================================

AutoTremolandoAudioProcessorEditor::~AutoTremolandoAudioProcessorEditor() {}

//==============================================================================

void AutoTremolandoAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
    g.setColour(juce::Colours::white);
    g.setFont(18.0f);
    g.drawFittedText("AutoTremolando", getLocalBounds(), juce::Justification::centredTop, 1);
}

//==============================================================================
void AutoTremolandoAudioProcessorEditor::resized()
{
    int margin = 20;
    int sliderW = 120;
    int sliderH = 120;
    int labelH = 20;

    //==============================================================
    // Sliders in literal DSP signal-chain order (left grid)
    //==============================================================
    int x = margin;
    int y = 60;

    // Row 1
    parameters[4].setBounds(x, y, sliderW, sliderH);   // Input
    labels[4].setBounds(x, y + sliderH, sliderW, labelH);
    x += sliderW + margin;

    parameters[15].setBounds(x, y, sliderW, sliderH);  // Presence
    labels[15].setBounds(x, y + sliderH, sliderW, labelH);
    x += sliderW + margin;

    parameters[6].setBounds(x, y, sliderW, sliderH);   // Sub Rate
    labels[6].setBounds(x, y + sliderH, sliderW, labelH);
    x += sliderW + margin;

    parameters[7].setBounds(x, y, sliderW, sliderH);   // Bass Rate
    labels[7].setBounds(x, y + sliderH, sliderW, labelH);

    // Row 2
    x = margin;
    y += sliderH + labelH + margin;

    parameters[8].setBounds(x, y, sliderW, sliderH);   // Mid Rate
    labels[8].setBounds(x, y + sliderH, sliderW, labelH);
    x += sliderW + margin;

    parameters[9].setBounds(x, y, sliderW, sliderH);   // Treble Rate
    labels[9].setBounds(x, y + sliderH, sliderW, labelH);
    x += sliderW + margin;

    parameters[10].setBounds(x, y, sliderW, sliderH);  // Sub Depth
    labels[10].setBounds(x, y + sliderH, sliderW, labelH);
    x += sliderW + margin;

    parameters[11].setBounds(x, y, sliderW, sliderH);  // Bass Depth
    labels[11].setBounds(x, y + sliderH, sliderW, labelH);

    // Row 3
    x = margin;
    y += sliderH + labelH + margin;

    parameters[12].setBounds(x, y, sliderW, sliderH);  // Mid Depth
    labels[12].setBounds(x, y + sliderH, sliderW, labelH);
    x += sliderW + margin;

    parameters[13].setBounds(x, y, sliderW, sliderH);  // Treble Depth
    labels[13].setBounds(x, y + sliderH, sliderW, labelH);
    x += sliderW + margin;

    parameters[14].setBounds(x, y, sliderW, sliderH);  // Wet
    labels[14].setBounds(x, y + sliderH, sliderW, labelH);
    x += sliderW + margin;

    parameters[5].setBounds(x, y, sliderW, sliderH);   // Output (last in chain)
    labels[5].setBounds(x, y + sliderH, sliderW, labelH);

    //==============================================================
    // Right-side menus (unchanged)
    //==============================================================
    int menuW = 140;
    int menuH = 25;

    int menuX = getWidth() - menuW - margin;
    int menuY = 60;

    presetLabel.setBounds(menuX, 20, menuW, 20);
    presetMenu.setBounds(menuX, 45, menuW, 25);

    subTremLabel.setBounds(menuX, menuY, menuW, labelH);
    subTremMenu.setBounds(menuX, menuY + labelH, menuW, menuH);
    menuY += labelH + menuH + margin;

    bassTremLabel.setBounds(menuX, menuY, menuW, labelH);
    bassTremMenu.setBounds(menuX, menuY + labelH, menuW, menuH);
    menuY += labelH + menuH + margin;

    midTremLabel.setBounds(menuX, menuY, menuW, labelH);
    midTremMenu.setBounds(menuX, menuY + labelH, menuW, menuH);
    menuY += labelH + menuH + margin;

    trebleTremLabel.setBounds(menuX, menuY, menuW, labelH);
    trebleTremMenu.setBounds(menuX, menuY + labelH, menuW, menuH);
}

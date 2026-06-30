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
    setSize(1400, 700);

    //======================================================================
    // Parameter IDs in DSP signal-chain order
    //======================================================================
    const juce::String paramIDs[NUM_OF_PARAMETERS] =
    {
        "INPUT_GAIN", "PRESENCE",
        "SUB_TREMOLO", "BASS_TREMOLO", "MID_TREMOLO", "TREBLE_TREMOLO",
        "MASTER_RATE",
        "SUB_TREM_RATE", "BASS_TREM_RATE", "MID_TREM_RATE", "TREBLE_TREM_RATE",
        "SUB_TREM_DEPTH", "BASS_TREM_DEPTH", "MID_TREM_DEPTH", "TREBLE_TREM_DEPTH",
        "PHASE_OFFSET", "RATE_OFFSET", "DEPTH_OFFSET", "PULSE_WIDTH",
        "WET", "OUTPUT_GAIN", "BYPASS"
    };

    const juce::String paramLabels[NUM_OF_PARAMETERS] =
    {
        "Input", "Presence",
        "Sub Type", "Bass Type", "Mid Type", "Treble Type",
        "Master Rate",
        "Sub Rate", "Bass Rate", "Mid Rate", "Treble Rate",
        "Sub Depth", "Bass Depth", "Mid Depth", "Treble Depth",
        "Phase Offset", "Rate Offset", "Depth Offset", "Pulse Width",
        "Wet", "Output", "Bypass"
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

    //======================================================================
    // Bypass button
    //======================================================================
    bypassLabel.setText("Bypass", juce::dontSendNotification);
    bypassLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(bypassLabel);

    bypassButton.setButtonText("OFF");
    addAndMakeVisible(bypassButton);

    bypassButton.onClick = [this]()
        {
            bool bCurrentBypass = *audioProcessor.apvts.getRawParameterValue("BYPASS") > 0.5f;
            bool bNewBypass = !bCurrentBypass;
            if (auto* param = audioProcessor.apvts.getParameter("BYPASS"))
                param->setValueNotifyingHost(bNewBypass ? 1.0f : 0.0f);
            bypassButton.setButtonText(bNewBypass ? "ON" : "OFF");
        };

    //======================================================================
    // Tempo sync slider switch
    //======================================================================
    tempoSyncLabel.setText("Tempo Sync", juce::dontSendNotification);
    tempoSyncLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(tempoSyncLabel);

    tempoSyncSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    tempoSyncSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    tempoSyncSlider.setRange(0.0, 1.0, 1.0);
    addAndMakeVisible(tempoSyncSlider);

    tempoSyncAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "TEMPO_SYNC", tempoSyncSlider);

    //======================================================================
    // Per-band note-division menus
    //======================================================================
    const juce::StringArray noteDivNames{ "1/16", "1/8", "1/4", "1/2", "1/1", "2/1" };

    juce::ComboBox* noteDivMenus[4] = { &subNoteDivMenu, &bassNoteDivMenu, &midNoteDivMenu, &trebleNoteDivMenu };
    for (auto* m : noteDivMenus)
        m->addItemList(noteDivNames, 1);

    subNoteDivLabel.setText("Sub Div", juce::dontSendNotification);
    bassNoteDivLabel.setText("Bass Div", juce::dontSendNotification);
    midNoteDivLabel.setText("Mid Div", juce::dontSendNotification);
    trebleNoteDivLabel.setText("Treble Div", juce::dontSendNotification);

    for (auto* lbl : { &subNoteDivLabel, &bassNoteDivLabel, &midNoteDivLabel, &trebleNoteDivLabel })
    {
        lbl->setJustificationType(juce::Justification::centred);
        addAndMakeVisible(lbl);
    }

    addAndMakeVisible(subNoteDivMenu);
    addAndMakeVisible(bassNoteDivMenu);
    addAndMakeVisible(midNoteDivMenu);
    addAndMakeVisible(trebleNoteDivMenu);

    subNoteDivAttach    = std::make_unique<MenuAttachment>(audioProcessor.apvts, "SUB_NOTE_DIV",    subNoteDivMenu);
    bassNoteDivAttach   = std::make_unique<MenuAttachment>(audioProcessor.apvts, "BASS_NOTE_DIV",   bassNoteDivMenu);
    midNoteDivAttach    = std::make_unique<MenuAttachment>(audioProcessor.apvts, "MID_NOTE_DIV",    midNoteDivMenu);
    trebleNoteDivAttach = std::make_unique<MenuAttachment>(audioProcessor.apvts, "TREBLE_NOTE_DIV", trebleNoteDivMenu);

    // Enable/disable note-division menus based on tempo sync state
    auto updateNoteDivState = [this]()
        {
            bool bSync = tempoSyncSlider.getValue() > 0.5;
            for (auto* m : { &subNoteDivMenu, &bassNoteDivMenu, &midNoteDivMenu, &trebleNoteDivMenu })
                m->setEnabled(bSync);
            for (auto* lbl : { &subNoteDivLabel, &bassNoteDivLabel, &midNoteDivLabel, &trebleNoteDivLabel })
                lbl->setEnabled(bSync);
        };

    tempoSyncSlider.onValueChange = [updateNoteDivState]() { updateNoteDivState(); };
    updateNoteDivState();

    //======================================================================
    // Disable channel spread controls in mono
    //======================================================================
    bool bEnableChannelSpread = audioProcessor.getTotalNumOutputChannels() > 1;
    float fChannelSpreadAlpha = bEnableChannelSpread ? 1.0f : 0.5f;

    for (int i = 15; i <= 17; ++i)
    {
        parameters[i].setEnabled(bEnableChannelSpread);
        labels[i].setEnabled(bEnableChannelSpread);
        parameters[i].setAlpha(fChannelSpreadAlpha);
        labels[i].setAlpha(fChannelSpreadAlpha);
    }
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

    // Row 1 (3 items) - Input, Presence, Pulse Width
    parameters[0].setBounds(x, y, sliderW, sliderH);   // Input Gain
    labels[0].setBounds(x, y + sliderH, sliderW, labelH);
    x += sliderW + margin;

    parameters[1].setBounds(x, y, sliderW, sliderH);   // Presence
    labels[1].setBounds(x, y + sliderH, sliderW, labelH);
    x += sliderW + margin;

    parameters[18].setBounds(x, y, sliderW, sliderH);  // Pulse Width
    labels[18].setBounds(x, y + sliderH, sliderW, labelH);

    // Row 2 (4 items) - Rates grouped
    x = margin;
    y += sliderH + labelH + margin;

    parameters[7].setBounds(x, y, sliderW, sliderH);   // Sub Rate
    labels[7].setBounds(x, y + sliderH, sliderW, labelH);
    x += sliderW + margin;

    parameters[8].setBounds(x, y, sliderW, sliderH);   // Bass Rate
    labels[8].setBounds(x, y + sliderH, sliderW, labelH);
    x += sliderW + margin;

    parameters[9].setBounds(x, y, sliderW, sliderH);   // Mid Rate
    labels[9].setBounds(x, y + sliderH, sliderW, labelH);
    x += sliderW + margin;

    parameters[10].setBounds(x, y, sliderW, sliderH);   // Treble Rate
    labels[10].setBounds(x, y + sliderH, sliderW, labelH);

    // Row 3 (4 items) - Depths grouped
    x = margin;
    y += sliderH + labelH + margin;

    parameters[11].setBounds(x, y, sliderW, sliderH);  // Sub Depth
    labels[11].setBounds(x, y + sliderH, sliderW, labelH);
    x += sliderW + margin;

    parameters[12].setBounds(x, y, sliderW, sliderH);  // Bass Depth
    labels[12].setBounds(x, y + sliderH, sliderW, labelH);
    x += sliderW + margin;

    parameters[13].setBounds(x, y, sliderW, sliderH);  // Mid Depth
    labels[13].setBounds(x, y + sliderH, sliderW, labelH);
    x += sliderW + margin;

    parameters[14].setBounds(x, y, sliderW, sliderH);  // Treble Depth
    labels[14].setBounds(x, y + sliderH, sliderW, labelH);

    // Row 4 (5 items) - Rate Offset, Master Rate, Depth Offset, Phase Offset, Wet/Dry, Output
    x = margin;
    y += sliderH + labelH + margin;

    parameters[16].setBounds(x, y, sliderW, sliderH);  // Rate Offset
    labels[16].setBounds(x, y + sliderH, sliderW, labelH);
    x += sliderW + margin;

    parameters[6].setBounds(x, y, sliderW, sliderH);   // Master Rate
    labels[6].setBounds(x, y + sliderH, sliderW, labelH);
    x += sliderW + margin;

    parameters[17].setBounds(x, y, sliderW, sliderH);  // Depth Offset
    labels[17].setBounds(x, y + sliderH, sliderW, labelH);
    x += sliderW + margin;

    parameters[15].setBounds(x, y, sliderW, sliderH);  // Phase Offset
    labels[15].setBounds(x, y + sliderH, sliderW, labelH);
    x += sliderW + margin;

    parameters[19].setBounds(x, y, sliderW, sliderH);  // Wet
    labels[19].setBounds(x, y + sliderH, sliderW, labelH);
    x += sliderW + margin;

    parameters[20].setBounds(x, y, sliderW, sliderH);  // Output Gain
    labels[20].setBounds(x, y + sliderH, sliderW, labelH);

    //==============================================================
    // Right-side menus and controls
    //==============================================================
    int menuW = 140;
    int menuH = 25;

    int menuX = 622;  // Centre the menus horizontally
    int menuY = 60;

    presetLabel.setBounds(menuX, menuY, menuW, 20);
    presetMenu.setBounds(menuX, menuY + 20, menuW, 25);
    menuY += 45;

    bypassLabel.setBounds(menuX, menuY, menuW, 20);
    bypassButton.setBounds(menuX, menuY + 20, menuW, menuH);
    menuY += 50;

    // Tremolo type menus - positioned after bypass with tighter spacing
    subTremLabel.setBounds(menuX, menuY, menuW, labelH);
    subTremMenu.setBounds(menuX, menuY + labelH, menuW, menuH);
    menuY += labelH + menuH + 8;

    bassTremLabel.setBounds(menuX, menuY, menuW, labelH);
    bassTremMenu.setBounds(menuX, menuY + labelH, menuW, menuH);
    menuY += labelH + menuH + 8;

    midTremLabel.setBounds(menuX, menuY, menuW, labelH);
    midTremMenu.setBounds(menuX, menuY + labelH, menuW, menuH);
    menuY += labelH + menuH + 8;

    trebleTremLabel.setBounds(menuX, menuY, menuW, labelH);
    trebleTremMenu.setBounds(menuX, menuY + labelH, menuW, menuH);
    menuY += labelH + menuH + 16;

    //==============================================================
    // Tempo sync switch + note-division menus (right column, below tremolo types)
    //==============================================================
    tempoSyncLabel.setBounds(menuX, menuY, menuW, labelH);
    tempoSyncSlider.setBounds(menuX, menuY + labelH, menuW, menuH);
    menuY += labelH + menuH + 8;

    subNoteDivLabel.setBounds(menuX, menuY, menuW, labelH);
    subNoteDivMenu.setBounds(menuX, menuY + labelH, menuW, menuH);
    menuY += labelH + menuH + 8;

    bassNoteDivLabel.setBounds(menuX, menuY, menuW, labelH);
    bassNoteDivMenu.setBounds(menuX, menuY + labelH, menuW, menuH);
    menuY += labelH + menuH + 8;

    midNoteDivLabel.setBounds(menuX, menuY, menuW, labelH);
    midNoteDivMenu.setBounds(menuX, menuY + labelH, menuW, menuH);
    menuY += labelH + menuH + 8;

    trebleNoteDivLabel.setBounds(menuX, menuY, menuW, labelH);
    trebleNoteDivMenu.setBounds(menuX, menuY + labelH, menuW, menuH);
}

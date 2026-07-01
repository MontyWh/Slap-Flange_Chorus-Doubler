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
    : AudioProcessorEditor(&p),
      inputMeterBar(fInputMeterDisplay),
      outputMeterBar(fOutputMeterDisplay),
      audioProcessor(p)
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

        if (i == 6 || (i >= 7 && i <= 10))
            parameters[i].setTextValueSuffix(" Hz");

        labels[i].setText(paramLabels[i], juce::dontSendNotification);
        labels[i].setJustificationType(juce::Justification::centred);
        addAndMakeVisible(labels[i]);

        paramAttach[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.apvts, paramIDs[i], parameters[i]);
    }

    setupSlider(startPhaseSlider);
    startPhaseSlider.setTextValueSuffix(" deg");
    addAndMakeVisible(startPhaseSlider);

    startPhaseLabel.setText("Start Phase", juce::dontSendNotification);
    startPhaseLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(startPhaseLabel);

    startPhaseAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "START_PHASE", startPhaseSlider);

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

    tapTempoButton.setButtonText("Tap Tempo");
    addAndMakeVisible(tapTempoButton);
    tapTempoButton.onClick = [this]()
        {
            audioProcessor.registerTapTempo();
        };

    resetDefaultsButton.setButtonText("Reset");
    addAndMakeVisible(resetDefaultsButton);
    resetDefaultsButton.onClick = [this]()
        {
            audioProcessor.resetParametersToDefaults();
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
    // Sync mode: Time vs Tempo
    //======================================================================
    tempoSyncLabel.setText("Sync Mode", juce::dontSendNotification);
    tempoSyncLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(tempoSyncLabel);

    tempoSyncSlider.setButtonText("TIME");
    addAndMakeVisible(tempoSyncSlider);

    tempoSyncSlider.onClick = [this]()
        {
            bool bCurrentSyncState = *audioProcessor.apvts.getRawParameterValue("TEMPO_SYNC") > 0.5f;
            bool bNewSync = !bCurrentSyncState;
            if (auto* param = audioProcessor.apvts.getParameter("TEMPO_SYNC"))
                param->setValueNotifyingHost(bNewSync ? 1.0f : 0.0f);

            bCurrentSync = bNewSync;  // Update member variable
            tempoSyncSlider.setButtonText(bNewSync ? "TEMPO" : "TIME");

            // Update labels based on sync mode
            if (bNewSync)
            {
                subNoteDivLabel.setText("Sub Div", juce::dontSendNotification);
                bassNoteDivLabel.setText("Bass Div", juce::dontSendNotification);
                midNoteDivLabel.setText("Mid Div", juce::dontSendNotification);
                trebleNoteDivLabel.setText("Treble Div", juce::dontSendNotification);
            }
            else
            {
                subNoteDivLabel.setText("Sub Time", juce::dontSendNotification);
                bassNoteDivLabel.setText("Bass Time", juce::dontSendNotification);
                midNoteDivLabel.setText("Mid Time", juce::dontSendNotification);
                trebleNoteDivLabel.setText("Treble Time", juce::dontSendNotification);
            }

            // Update value labels to reflect new interpretation
            auto updateLabel = [bNewSync](juce::Slider& slider, juce::Label& valueLabel)
                {
                    int idx = juce::roundToInt(slider.getValue());
                    if (bNewSync)
                    {
                        const juce::String tempoLabels[] = {
                            // 1/16
                            "1/16", "1/16t", "1/16d", "1/16 swing", "1/16 shuffle",
                            // 1/8
                            "1/8", "1/8t", "1/8d", "1/8 swing", "1/8 shuffle",
                            // 1/4
                            "1/4", "1/4t", "1/4d", "1/4 swing", "1/4 shuffle",
                            // 1/2
                            "1/2", "1/2t", "1/2d", "1/2 swing", "1/2 shuffle",
                            // 1/1
                            "1/1", "1/1t", "1/1d", "1/1 swing", "1/1 shuffle",
                            // 2/1
                            "2/1", "2/1t", "2/1d", "2/1 swing", "2/1 shuffle"
                        };
                        valueLabel.setText(tempoLabels[idx], juce::dontSendNotification);
                    }
                    else
                    {
                        float fHz = 0.5f + (idx / 29.0f) * 15.5f;
                        valueLabel.setText(juce::String(fHz, 2) + " Hz", juce::dontSendNotification);
                    }
                };

            updateLabel(subNoteDivSlider, subNoteDivValueLabel);
            updateLabel(bassNoteDivSlider, bassNoteDivValueLabel);
            updateLabel(midNoteDivSlider, midNoteDivValueLabel);
            updateLabel(trebleNoteDivSlider, trebleNoteDivValueLabel);
        };

    rateLockLabel.setText("Rate Lock", juce::dontSendNotification);
    rateLockLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(rateLockLabel);

    rateLockButton.setButtonText("Link");
    addAndMakeVisible(rateLockButton);
    rateLockAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.apvts, "RATE_LOCK", rateLockButton);

    retriggerLabel.setText("Retrigger", juce::dontSendNotification);
    retriggerLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(retriggerLabel);

    retriggerButton.setButtonText("On Play");
    addAndMakeVisible(retriggerButton);
    retriggerAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.apvts, "RETRIGGER_ON_PLAY", retriggerButton);

    stereoModeLabel.setText("Stereo Mode", juce::dontSendNotification);
    stereoModeLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(stereoModeLabel);

    stereoModeMenu.addItemList({ "Mono", "Ping Pong", "Spread" }, 1);
    addAndMakeVisible(stereoModeMenu);
    stereoModeAttach = std::make_unique<MenuAttachment>(audioProcessor.apvts, "STEREO_MODE", stereoModeMenu);

    depthModeLabel.setText("Depth Mode", juce::dontSendNotification);
    depthModeLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(depthModeLabel);

    depthModeMenu.addItemList({ "Unipolar", "Bipolar" }, 1);
    addAndMakeVisible(depthModeMenu);
    depthModeAttach = std::make_unique<MenuAttachment>(audioProcessor.apvts, "DEPTH_MODE", depthModeMenu);

    inputMeterLabel.setText("Input", juce::dontSendNotification);
    inputMeterLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(inputMeterLabel);
    addAndMakeVisible(inputMeterBar);

    outputMeterLabel.setText("Output", juce::dontSendNotification);
    outputMeterLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(outputMeterLabel);
    addAndMakeVisible(outputMeterBar);

    startTimerHz(30);

    //======================================================================
    // Division rotary sliders (note divisions for Tempo, time values for Time-based)
    //======================================================================
    auto setupDivisionSlider = [this](juce::Slider& slider, juce::Label& label, juce::Label& valueLabel, const juce::String& paramID)
        {
            slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
            slider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
            slider.setRange(0.0, 29.0, 1.0);
            slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::cyan);
            addAndMakeVisible(slider);

            label.setJustificationType(juce::Justification::centred);
            addAndMakeVisible(label);

            valueLabel.setJustificationType(juce::Justification::centred);
            addAndMakeVisible(valueLabel);
        };

    setupDivisionSlider(subNoteDivSlider, subNoteDivLabel, subNoteDivValueLabel, "SUB_NOTE_DIV");
    setupDivisionSlider(bassNoteDivSlider, bassNoteDivLabel, bassNoteDivValueLabel, "BASS_NOTE_DIV");
    setupDivisionSlider(midNoteDivSlider, midNoteDivLabel, midNoteDivValueLabel, "MID_NOTE_DIV");
    setupDivisionSlider(trebleNoteDivSlider, trebleNoteDivLabel, trebleNoteDivValueLabel, "TREBLE_NOTE_DIV");

    subNoteDivAttach    = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "SUB_NOTE_DIV",    subNoteDivSlider);
    bassNoteDivAttach   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "BASS_NOTE_DIV",   bassNoteDivSlider);
    midNoteDivAttach    = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "MID_NOTE_DIV",    midNoteDivSlider);
    trebleNoteDivAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "TREBLE_NOTE_DIV", trebleNoteDivSlider);

    // Lambda to update value labels based on sync mode
    auto updateDivisionLabels = [this]()
        {
            auto updateLabel = [this](juce::Slider& slider, juce::Label& valueLabel)
                {
                    int idx = juce::roundToInt(slider.getValue());
                    if (bCurrentSync)
                    {
                        // Tempo mode: note divisions interleaved by note value
                        // Each note value: straight, triplet, dotted, swing, shuffle
                        const juce::String tempoLabels[] = { 
                            // 1/16
                            "1/16", "1/16t", "1/16d", "1/16 swing", "1/16 shuffle",
                            // 1/8
                            "1/8", "1/8t", "1/8d", "1/8 swing", "1/8 shuffle",
                            // 1/4
                            "1/4", "1/4t", "1/4d", "1/4 swing", "1/4 shuffle",
                            // 1/2
                            "1/2", "1/2t", "1/2d", "1/2 swing", "1/2 shuffle",
                            // 1/1
                            "1/1", "1/1t", "1/1d", "1/1 swing", "1/1 shuffle",
                            // 2/1
                            "2/1", "2/1t", "2/1d", "2/1 swing", "2/1 shuffle"
                        };
                        valueLabel.setText(tempoLabels[idx], juce::dontSendNotification);
                    }
                    else
                    {
                        // Time mode: Hz (mapped from 0.5 Hz to 16 Hz)
                        float fHz = 0.5f + (idx / 29.0f) * 15.5f;
                        valueLabel.setText(juce::String(fHz, 2) + " Hz", juce::dontSendNotification);
                    }
                };

            updateLabel(subNoteDivSlider, subNoteDivValueLabel);
            updateLabel(bassNoteDivSlider, bassNoteDivValueLabel);
            updateLabel(midNoteDivSlider, midNoteDivValueLabel);
            updateLabel(trebleNoteDivSlider, trebleNoteDivValueLabel);
        };

    // Update labels initially
    subNoteDivLabel.setText("Sub Div", juce::dontSendNotification);
    bassNoteDivLabel.setText("Bass Div", juce::dontSendNotification);
    midNoteDivLabel.setText("Mid Div", juce::dontSendNotification);
    trebleNoteDivLabel.setText("Treble Div", juce::dontSendNotification);
    updateDivisionLabels();

    // Add slider value change listeners
    auto makeSliderListener = [updateDivisionLabels](juce::Slider& slider)
        {
            slider.onValueChange = [updateDivisionLabels]() { updateDivisionLabels(); };
        };

    makeSliderListener(subNoteDivSlider);
    makeSliderListener(bassNoteDivSlider);
    makeSliderListener(midNoteDivSlider);
    makeSliderListener(trebleNoteDivSlider);

    // Initialize division menus and labels based on current sync mode
    bCurrentSync = *audioProcessor.apvts.getRawParameterValue("TEMPO_SYNC") > 0.5f;
    tempoSyncSlider.setButtonText(bCurrentSync ? "TEMPO" : "TIME");

    if (!bCurrentSync)
    {
        subNoteDivLabel.setText("Sub Time", juce::dontSendNotification);
        bassNoteDivLabel.setText("Bass Time", juce::dontSendNotification);
        midNoteDivLabel.setText("Mid Time", juce::dontSendNotification);
        trebleNoteDivLabel.setText("Treble Time", juce::dontSendNotification);
    }

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

void AutoTremolandoAudioProcessorEditor::timerCallback()
{
    fInputMeterDisplay = juce::jlimit(0.0, 1.0, (double)audioProcessor.getInputMeterLevel());
    fOutputMeterDisplay = juce::jlimit(0.0, 1.0, (double)audioProcessor.getOutputMeterLevel());
    repaint();
}

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

    // Row 1 (4 items) - Input, Presence, Pulse Width, Start Phase
    parameters[0].setBounds(x, y, sliderW, sliderH);   // Input Gain
    labels[0].setBounds(x, y + sliderH, sliderW, labelH);
    x += sliderW + margin;

    parameters[1].setBounds(x, y, sliderW, sliderH);   // Presence
    labels[1].setBounds(x, y + sliderH, sliderW, labelH);
    x += sliderW + margin;

    parameters[18].setBounds(x, y, sliderW, sliderH);  // Pulse Width
    labels[18].setBounds(x, y + sliderH, sliderW, labelH);
    x += sliderW + margin;

    startPhaseSlider.setBounds(x, y, sliderW, sliderH);
    startPhaseLabel.setBounds(x, y + sliderH, sliderW, labelH);

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
    int colGap = 10;

    int menuX = 622;  // Left column (presets, bypass, tremolo types)
    int menuY = 60;

    presetLabel.setBounds(menuX, menuY, menuW, 20);
    presetMenu.setBounds(menuX, menuY + 20, menuW, 25);
    menuY += 50;

    tapTempoButton.setBounds(menuX, menuY, menuW, menuH);
    menuY += menuH + 8;

    resetDefaultsButton.setBounds(menuX, menuY, menuW, menuH);
    menuY += menuH + 10;

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

    //==============================================================
    // Tempo sync controls - second column to the right, aligned with presets
    //==============================================================
    int tempoX = menuX + menuW + colGap;
    int tempoY = 60;
    int tempoColW = 130;
    int sliderSize = 60;  // Rotary slider diameter

    tempoSyncLabel.setBounds(tempoX, tempoY, tempoColW, labelH);
    tempoSyncSlider.setBounds(tempoX, tempoY + labelH, tempoColW, menuH);
    tempoY += labelH + menuH + 16;

    // Per-band note-division rotary sliders with value displays
    subNoteDivLabel.setBounds(tempoX, tempoY, tempoColW, labelH);
    subNoteDivSlider.setBounds(tempoX + (tempoColW - sliderSize) / 2, tempoY + labelH, sliderSize, sliderSize);
    subNoteDivValueLabel.setBounds(tempoX, tempoY + labelH + sliderSize, tempoColW, labelH);
    tempoY += labelH + sliderSize + labelH + 16;

    bassNoteDivLabel.setBounds(tempoX, tempoY, tempoColW, labelH);
    bassNoteDivSlider.setBounds(tempoX + (tempoColW - sliderSize) / 2, tempoY + labelH, sliderSize, sliderSize);
    bassNoteDivValueLabel.setBounds(tempoX, tempoY + labelH + sliderSize, tempoColW, labelH);
    tempoY += labelH + sliderSize + labelH + 16;

    midNoteDivLabel.setBounds(tempoX, tempoY, tempoColW, labelH);
    midNoteDivSlider.setBounds(tempoX + (tempoColW - sliderSize) / 2, tempoY + labelH, sliderSize, sliderSize);
    midNoteDivValueLabel.setBounds(tempoX, tempoY + labelH + sliderSize, tempoColW, labelH);
    tempoY += labelH + sliderSize + labelH + 16;

    trebleNoteDivLabel.setBounds(tempoX, tempoY, tempoColW, labelH);
    trebleNoteDivSlider.setBounds(tempoX + (tempoColW - sliderSize) / 2, tempoY + labelH, sliderSize, sliderSize);
    trebleNoteDivValueLabel.setBounds(tempoX, tempoY + labelH + sliderSize, tempoColW, labelH);

    //==============================================================
    // Extra controls - third column to the right of tempo sync
    //==============================================================
    int extraX = tempoX + tempoColW + colGap;
    int extraY = 60;
    int extraColW = 140;

    rateLockLabel.setBounds(extraX, extraY, extraColW, labelH);
    rateLockButton.setBounds(extraX, extraY + labelH, extraColW, menuH);
    extraY += labelH + menuH + 8;

    retriggerLabel.setBounds(extraX, extraY, extraColW, labelH);
    retriggerButton.setBounds(extraX, extraY + labelH, extraColW, menuH);
    extraY += labelH + menuH + 8;

    stereoModeLabel.setBounds(extraX, extraY, extraColW, labelH);
    stereoModeMenu.setBounds(extraX, extraY + labelH, extraColW, menuH);
    extraY += labelH + menuH + 8;

    depthModeLabel.setBounds(extraX, extraY, extraColW, labelH);
    depthModeMenu.setBounds(extraX, extraY + labelH, extraColW, menuH);
    extraY += labelH + menuH + 12;

    inputMeterLabel.setBounds(extraX, extraY, extraColW, labelH);
    inputMeterBar.setBounds(extraX, extraY + labelH, extraColW, 14);
    extraY += labelH + 20;

    outputMeterLabel.setBounds(extraX, extraY, extraColW, labelH);
    outputMeterBar.setBounds(extraX, extraY + labelH, extraColW, 14);
}

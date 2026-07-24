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

#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    constexpr int iBaseEditorWidth = 1400;
    constexpr int iBaseEditorHeight = 700;
    constexpr float fTargetScreenCoverage = 0.85f;
    constexpr float fMinUiScale = 0.55f;
    constexpr float fMaxUiScale = 1.60f;

    float fGetScreenLinkedUiScale()
    {
        if (auto* display = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
        {
            const auto userArea = display->userArea;
            const float fScaleX = (static_cast<float>(userArea.getWidth()) * fTargetScreenCoverage) / static_cast<float>(iBaseEditorWidth);
            const float fScaleY = (static_cast<float>(userArea.getHeight()) * fTargetScreenCoverage) / static_cast<float>(iBaseEditorHeight);
            return juce::jlimit(fMinUiScale, fMaxUiScale, juce::jmin(fScaleX, fScaleY));
        }

        return 1.0f;
    }

    int iScaled(const int value, const float uiScale)
    {
        return juce::roundToInt(static_cast<float>(value) * uiScale);
    }
}

static void setupSlider(juce::Slider& s, const float uiScale)
{
    s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, iScaled(60, uiScale), iScaled(20, uiScale));
}

//==============================================================================
AutoTremolandoAudioProcessorEditor::AutoTremolandoAudioProcessorEditor (AutoTremolandoAudioProcessor& p)
    : AudioProcessorEditor (&p),
      inputMeterBar (dInputMeterDisplay),
      outputMeterBar (dOutputMeterDisplay),
      audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    const float fUiScale = fGetScreenLinkedUiScale();
    setSize(iScaled(iBaseEditorWidth, fUiScale), iScaled(iBaseEditorHeight, fUiScale));

    const juce::String sParamIDs[iNumParameters] =
    {
        "INPUT_GAIN", "PRESENCE",
        "SUB_TREMOLO", "BASS_TREMOLO", "MID_TREMOLO", "TREBLE_TREMOLO",
        "MASTER_RATE",
        "SUB_TREM_RATE", "BASS_TREM_RATE", "MID_TREM_RATE", "TREBLE_TREM_RATE",
        "SUB_TREM_DEPTH", "BASS_TREM_DEPTH", "MID_TREM_DEPTH", "TREBLE_TREM_DEPTH",
        "PHASE_OFFSET", "RATE_OFFSET", "DEPTH_OFFSET", "PULSE_WIDTH",
        "WET", "OUTPUT_GAIN", "BYPASS"
    };

    const juce::String sParamLabels[iNumParameters] =
    {
        "Input", "Presence",
        "Sub Type", "Bass Type", "Mid Type", "Treble Type",
        "Master Rate",
        "Sub Rate", "Bass Rate", "Mid Rate", "Treble Rate",
        "Sub Depth", "Bass Depth", "Mid Depth", "Treble Depth",
        "Phase Offset", "Rate Offset", "Depth Offset", "Pulse Width",
        "Wet", "Output", "Bypass"
    };

    for (int i = 0; i < iNumParameters; ++i)
    {
        setupSlider(parameters[i], fUiScale);
        addAndMakeVisible(parameters[i]);

        if (i == 6 || (i >= 7 && i <= 10))
            parameters[i].setTextValueSuffix(" Hz");

        labels[i].setText(sParamLabels[i], juce::dontSendNotification);
        labels[i].setJustificationType(juce::Justification::centred);
        addAndMakeVisible(labels[i]);

        paramAttach[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.apvts, sParamIDs[i], parameters[i]);
    }

    addAndMakeVisible(modulation);

    setupSlider(modulation.startPhaseSlider, fUiScale);
    modulation.startPhaseSlider.setTextValueSuffix(" deg");
    modulation.startPhaseLabel.setText("Start Phase", juce::dontSendNotification);
    modulation.startPhaseLabel.setJustificationType(juce::Justification::centred);
    startPhaseAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "START_PHASE", modulation.startPhaseSlider);

    juce::ComboBox* menus[4] =
    {
        &modulation.subTremMenu, &modulation.bassTremMenu, &modulation.midTremMenu, &modulation.trebleTremMenu
    };
    const juce::StringArray sWaveNames{ "Sine", "Triangle", "Sawtooth", "Pulse", "Square" };
    for (auto* m : menus) m->addItemList(sWaveNames, 1);

    modulation.subTremLabel.setText("Sub Type", juce::dontSendNotification);
    modulation.bassTremLabel.setText("Bass Type", juce::dontSendNotification);
    modulation.midTremLabel.setText("Mid Type", juce::dontSendNotification);
    modulation.trebleTremLabel.setText("Treble Type", juce::dontSendNotification);

    modulation.subTremLabel.setJustificationType(juce::Justification::centred);
    modulation.bassTremLabel.setJustificationType(juce::Justification::centred);
    modulation.midTremLabel.setJustificationType(juce::Justification::centred);
    modulation.trebleTremLabel.setJustificationType(juce::Justification::centred);

    subTremAttach = std::make_unique<MenuAttachment>(audioProcessor.apvts, "SUB_TREMOLO", modulation.subTremMenu);
    bassTremAttach = std::make_unique<MenuAttachment>(audioProcessor.apvts, "BASS_TREMOLO", modulation.bassTremMenu);
    midTremAttach = std::make_unique<MenuAttachment>(audioProcessor.apvts, "MID_TREMOLO", modulation.midTremMenu);
    trebleTremAttach = std::make_unique<MenuAttachment>(audioProcessor.apvts, "TREBLE_TREMOLO", modulation.trebleTremMenu);

    presetLabel.setText("Presets", juce::dontSendNotification);
    presetLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(presetLabel);

    const auto& presets = audioProcessor.getPresets();
    for (int i = 0; i < static_cast<int>(presets.size()); ++i)
        presetMenu.addItem(presets[static_cast<size_t>(i)].sName, i + 1);
    addAndMakeVisible(presetMenu);

    presetMenu.onChange = [this]()
        {
            int iIndex = presetMenu.getSelectedId() - 1;
            audioProcessor.loadPreset(iIndex);
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

    modulation.tempoSyncLabel.setText("Sync Mode", juce::dontSendNotification);
    modulation.tempoSyncLabel.setJustificationType(juce::Justification::centred);

    modulation.tempoSyncSlider.setButtonText("TIME");

    modulation.rateLockLabel.setText("Rate Lock", juce::dontSendNotification);
    modulation.rateLockLabel.setJustificationType(juce::Justification::centred);

    modulation.rateLockButton.setClickingTogglesState(true);
    rateLockAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.apvts, "RATE_LOCK", modulation.rateLockButton);
    modulation.rateLockButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::darkcyan);
    modulation.rateLockButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    modulation.rateLockButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    modulation.rateLockButton.setTooltip("Links Bass, Mid and Treble to the Sub rate or division");
    modulation.rateLockButton.setButtonText(modulation.rateLockButton.getToggleState() ? "Linked" : "Unlinked");

    modulation.retriggerLabel.setText("Retrigger", juce::dontSendNotification);
    modulation.retriggerLabel.setJustificationType(juce::Justification::centred);

    modulation.retriggerButton.setClickingTogglesState(true);
    modulation.retriggerButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::darkcyan);
    modulation.retriggerButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkslategrey);
    modulation.retriggerButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    modulation.retriggerButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    modulation.retriggerButton.setTooltip("When enabled, tremolo restarts from Start Phase each time playback starts");
    retriggerAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.apvts, "RETRIGGER_ON_PLAY", modulation.retriggerButton);

    modulation.rateLockButton.setToggleState(*audioProcessor.apvts.getRawParameterValue("RATE_LOCK") > 0.5f, juce::dontSendNotification);
    modulation.retriggerButton.setToggleState(*audioProcessor.apvts.getRawParameterValue("RETRIGGER_ON_PLAY") > 0.5f, juce::dontSendNotification);
    modulation.retriggerButton.setButtonText(modulation.retriggerButton.getToggleState() ? "Retrig" : "Free");

    modulation.surroundWidthLabel.setText("Surround Width", juce::dontSendNotification);
    modulation.surroundWidthLabel.setJustificationType(juce::Justification::centred);

    setupSlider(modulation.surroundWidthSlider, fUiScale);
    modulation.surroundWidthSlider.setTextValueSuffix(" %");
    surroundWidthAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "SURROUND_WIDTH", modulation.surroundWidthSlider);

    modulation.depthModeLabel.setText("Depth Mode", juce::dontSendNotification);
    modulation.depthModeLabel.setJustificationType(juce::Justification::centred);

    modulation.depthModeMenu.addItemList({ "Unipolar", "Bipolar" }, 1);
    depthModeAttach = std::make_unique<MenuAttachment>(audioProcessor.apvts, "DEPTH_MODE", modulation.depthModeMenu);

    inputMeterLabel.setText("Input", juce::dontSendNotification);
    inputMeterLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(inputMeterLabel);
    addAndMakeVisible(inputMeterBar);

    outputMeterLabel.setText("Output", juce::dontSendNotification);
    outputMeterLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(outputMeterLabel);
    addAndMakeVisible(outputMeterBar);

    startTimerHz(30);

    auto setupDivisionSlider = [](juce::Slider& slider, juce::Label& label, juce::Label& valueLabel)
        {
            slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
            slider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
            slider.setRange(0.0, 14.0, 1.0);
            slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::cyan);

            label.setJustificationType(juce::Justification::centred);
            valueLabel.setJustificationType(juce::Justification::centred);
        };

    setupDivisionSlider(modulation.subNoteDivSlider, modulation.subNoteDivLabel, modulation.subNoteDivValueLabel);
    setupDivisionSlider(modulation.bassNoteDivSlider, modulation.bassNoteDivLabel, modulation.bassNoteDivValueLabel);
    setupDivisionSlider(modulation.midNoteDivSlider, modulation.midNoteDivLabel, modulation.midNoteDivValueLabel);
    setupDivisionSlider(modulation.trebleNoteDivSlider, modulation.trebleNoteDivLabel, modulation.trebleNoteDivValueLabel);

    subNoteDivAttach    = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "SUB_NOTE_DIV",    modulation.subNoteDivSlider);
    bassNoteDivAttach   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "BASS_NOTE_DIV",   modulation.bassNoteDivSlider);
    midNoteDivAttach    = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "MID_NOTE_DIV",    modulation.midNoteDivSlider);
    trebleNoteDivAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "TREBLE_NOTE_DIV", modulation.trebleNoteDivSlider);

    auto updateDivisionLabels = [this]()
        {
            auto updateLabel = [this](juce::Slider& slider, juce::Label& valueLabel)
                {
                    const int iIdx = juce::roundToInt(slider.getValue());
                    if (bCurrentSync)
                    {
                        const juce::String sTempoLabels[] = {
                            "1/1", "1/1d", "1/1t",
                            "1/2", "1/2d", "1/2t",
                            "1/4", "1/4d", "1/4t",
                            "1/8", "1/8d", "1/8t",
                            "1/16", "1/16d", "1/16t"
                        };
                        valueLabel.setText(sTempoLabels[iIdx], juce::dontSendNotification);
                    }
                    else
                    {
                        const float fHz = 0.5f + (iIdx / 14.0f) * 15.5f;
                        valueLabel.setText(juce::String(fHz, 2) + " Hz", juce::dontSendNotification);
                    }
                };

            updateLabel(modulation.subNoteDivSlider, modulation.subNoteDivValueLabel);
            updateLabel(modulation.bassNoteDivSlider, modulation.bassNoteDivValueLabel);
            updateLabel(modulation.midNoteDivSlider, modulation.midNoteDivValueLabel);
            updateLabel(modulation.trebleNoteDivSlider, modulation.trebleNoteDivValueLabel);
        };

    auto syncLinkedRateControls = [this]()
        {
            if (bUpdatingLinkedRates || !modulation.rateLockButton.getToggleState())
                return;

            const juce::ScopedValueSetter<bool> updatingLinkedRates(bUpdatingLinkedRates, true);
            const double dSubRate = parameters[7].getValue();

            parameters[8].setValue(dSubRate, juce::sendNotificationSync);
            parameters[9].setValue(dSubRate, juce::sendNotificationSync);
            parameters[10].setValue(dSubRate, juce::sendNotificationSync);
        };

    auto syncLinkedTimeControls = [this]()
        {
            if (bUpdatingLinkedRates || !modulation.rateLockButton.getToggleState())
                return;

            const juce::ScopedValueSetter<bool> updatingLinkedRates(bUpdatingLinkedRates, true);
            const double dSubDivision = modulation.subNoteDivSlider.getValue();

            modulation.bassNoteDivSlider.setValue(dSubDivision, juce::sendNotificationSync);
            modulation.midNoteDivSlider.setValue(dSubDivision, juce::sendNotificationSync);
            modulation.trebleNoteDivSlider.setValue(dSubDivision, juce::sendNotificationSync);
        };

    auto updateSyncModeUi = [this, updateDivisionLabels]()
        {
            modulation.tempoSyncSlider.setButtonText(bCurrentSync ? "TEMPO" : "TIME");

            if (bCurrentSync)
            {
                modulation.subNoteDivLabel.setText("Sub Div", juce::dontSendNotification);
                modulation.bassNoteDivLabel.setText("Bass Div", juce::dontSendNotification);
                modulation.midNoteDivLabel.setText("Mid Div", juce::dontSendNotification);
                modulation.trebleNoteDivLabel.setText("Treble Div", juce::dontSendNotification);
            }
            else
            {
                modulation.subNoteDivLabel.setText("Sub Time", juce::dontSendNotification);
                modulation.bassNoteDivLabel.setText("Bass Time", juce::dontSendNotification);
                modulation.midNoteDivLabel.setText("Mid Time", juce::dontSendNotification);
                modulation.trebleNoteDivLabel.setText("Treble Time", juce::dontSendNotification);
            }

            updateDivisionLabels();
        };

    auto makeDivisionSliderListener = [syncLinkedTimeControls, updateDivisionLabels](juce::Slider& slider)
        {
            slider.onValueChange = [syncLinkedTimeControls, updateDivisionLabels]()
                {
                    syncLinkedTimeControls();
                    updateDivisionLabels();
                };
        };

    auto makeRateSliderListener = [syncLinkedRateControls](juce::Slider& slider)
        {
            slider.onValueChange = [syncLinkedRateControls]()
                {
                    syncLinkedRateControls();
                };
        };

    makeDivisionSliderListener(modulation.subNoteDivSlider);
    makeDivisionSliderListener(modulation.bassNoteDivSlider);
    makeDivisionSliderListener(modulation.midNoteDivSlider);
    makeDivisionSliderListener(modulation.trebleNoteDivSlider);

    makeRateSliderListener(parameters[7]);
    makeRateSliderListener(parameters[8]);
    makeRateSliderListener(parameters[9]);
    makeRateSliderListener(parameters[10]);

    modulation.rateLockButton.onStateChange = [this, syncLinkedRateControls, syncLinkedTimeControls, updateDivisionLabels]()
        {
            modulation.rateLockButton.setButtonText(modulation.rateLockButton.getToggleState() ? "Linked" : "Unlinked");
            syncLinkedRateControls();
            syncLinkedTimeControls();
            updateDivisionLabels();
        };

    modulation.retriggerButton.onStateChange = [this]()
        {
            modulation.retriggerButton.setButtonText(modulation.retriggerButton.getToggleState() ? "Retrig" : "Free");
        };

    modulation.tempoSyncSlider.onClick = [this, updateSyncModeUi]()
        {
            bool bCurrentSyncState = *audioProcessor.apvts.getRawParameterValue("TEMPO_SYNC") > 0.5f;
            bool bNewSync = !bCurrentSyncState;
            if (auto* param = audioProcessor.apvts.getParameter("TEMPO_SYNC"))
                param->setValueNotifyingHost(bNewSync ? 1.0f : 0.0f);

            bCurrentSync = bNewSync;
            updateSyncModeUi();
        };

    bCurrentSync = *audioProcessor.apvts.getRawParameterValue("TEMPO_SYNC") > 0.5f;
    updateSyncModeUi();

    updateChannelSpreadUiState();
}

//==============================================================================

AutoTremolandoAudioProcessorEditor::~AutoTremolandoAudioProcessorEditor()
{
}

void AutoTremolandoAudioProcessorEditor::updateChannelSpreadUiState()
{
    const bool bEnableChannelSpread = audioProcessor.getTotalNumOutputChannels() > 1;
    const float fChannelSpreadAlpha = bEnableChannelSpread ? 1.0f : 0.35f;

    for (int i = 15; i <= 17; ++i)
    {
        parameters[i].setEnabled(bEnableChannelSpread);
        labels[i].setEnabled(bEnableChannelSpread);
        parameters[i].setAlpha(fChannelSpreadAlpha);
        labels[i].setAlpha(fChannelSpreadAlpha);
    }

    modulation.surroundWidthSlider.setEnabled(bEnableChannelSpread);
    modulation.surroundWidthLabel.setEnabled(bEnableChannelSpread);
    modulation.surroundWidthSlider.setAlpha(fChannelSpreadAlpha);
    modulation.surroundWidthLabel.setAlpha(fChannelSpreadAlpha);
}

void AutoTremolandoAudioProcessorEditor::timerCallback()
{
    updateChannelSpreadUiState();
    dInputMeterDisplay = juce::jlimit(0.0, 1.0, (double)audioProcessor.getInputMeterLevel());
    dOutputMeterDisplay = juce::jlimit(0.0, 1.0, (double)audioProcessor.getOutputMeterLevel());
    repaint();
}

//==============================================================================
void AutoTremolandoAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (15.0f));
    g.drawFittedText ("AutoTremolando", getLocalBounds(), juce::Justification::centredTop, 1);
}

void AutoTremolandoAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    const float fUiScale = juce::jmin(
        static_cast<float>(getWidth()) / static_cast<float>(iBaseEditorWidth),
        static_cast<float>(getHeight()) / static_cast<float>(iBaseEditorHeight));

    const int iMargin = iScaled(20, fUiScale);
    const int iSliderW = iScaled(120, fUiScale);
    const int iSliderH = iScaled(120, fUiScale);
    const int iLabelH = iScaled(20, fUiScale);

    int iX = iMargin;
    int iY = iScaled(60, fUiScale);

    parameters[0].setBounds(iX, iY, iSliderW, iSliderH);
    labels[0].setBounds(iX, iY + iSliderH, iSliderW, iLabelH);
    iX += iSliderW + iMargin;

    parameters[1].setBounds(iX, iY, iSliderW, iSliderH);
    labels[1].setBounds(iX, iY + iSliderH, iSliderW, iLabelH);
    iX += iSliderW + iMargin;

    parameters[18].setBounds(iX, iY, iSliderW, iSliderH);
    labels[18].setBounds(iX, iY + iSliderH, iSliderW, iLabelH);

    iX = iMargin;
    iY += iSliderH + iLabelH + iMargin;

    parameters[7].setBounds(iX, iY, iSliderW, iSliderH);
    labels[7].setBounds(iX, iY + iSliderH, iSliderW, iLabelH);
    iX += iSliderW + iMargin;

    parameters[8].setBounds(iX, iY, iSliderW, iSliderH);
    labels[8].setBounds(iX, iY + iSliderH, iSliderW, iLabelH);
    iX += iSliderW + iMargin;

    parameters[9].setBounds(iX, iY, iSliderW, iSliderH);
    labels[9].setBounds(iX, iY + iSliderH, iSliderW, iLabelH);
    iX += iSliderW + iMargin;

    parameters[10].setBounds(iX, iY, iSliderW, iSliderH);
    labels[10].setBounds(iX, iY + iSliderH, iSliderW, iLabelH);

    iX = iMargin;
    iY += iSliderH + iLabelH + iMargin;

    parameters[11].setBounds(iX, iY, iSliderW, iSliderH);
    labels[11].setBounds(iX, iY + iSliderH, iSliderW, iLabelH);
    iX += iSliderW + iMargin;

    parameters[12].setBounds(iX, iY, iSliderW, iSliderH);
    labels[12].setBounds(iX, iY + iSliderH, iSliderW, iLabelH);
    iX += iSliderW + iMargin;

    parameters[13].setBounds(iX, iY, iSliderW, iSliderH);
    labels[13].setBounds(iX, iY + iSliderH, iSliderW, iLabelH);
    iX += iSliderW + iMargin;

    parameters[14].setBounds(iX, iY, iSliderW, iSliderH);
    labels[14].setBounds(iX, iY + iSliderH, iSliderW, iLabelH);

    iX = iMargin;
    iY += iSliderH + iLabelH + iMargin;

    parameters[16].setBounds(iX, iY, iSliderW, iSliderH);
    labels[16].setBounds(iX, iY + iSliderH, iSliderW, iLabelH);
    iX += iSliderW + iMargin;

    parameters[6].setBounds(iX, iY, iSliderW, iSliderH);
    labels[6].setBounds(iX, iY + iSliderH, iSliderW, iLabelH);
    iX += iSliderW + iMargin;

    parameters[17].setBounds(iX, iY, iSliderW, iSliderH);
    labels[17].setBounds(iX, iY + iSliderH, iSliderW, iLabelH);
    iX += iSliderW + iMargin;

    parameters[15].setBounds(iX, iY, iSliderW, iSliderH);
    labels[15].setBounds(iX, iY + iSliderH, iSliderW, iLabelH);
    iX += iSliderW + iMargin;

    parameters[19].setBounds(iX, iY, iSliderW, iSliderH);
    labels[19].setBounds(iX, iY + iSliderH, iSliderW, iLabelH);
    iX += iSliderW + iMargin;

    parameters[20].setBounds(iX, iY, iSliderW, iSliderH);
    labels[20].setBounds(iX, iY + iSliderH, iSliderW, iLabelH);

    const int iMenuW = iScaled(140, fUiScale);
    const int iMenuH = iScaled(25, fUiScale);

    const int iMenuX = iScaled(622, fUiScale);
    int iMenuY = iScaled(60, fUiScale);

    presetLabel.setBounds(iMenuX, iMenuY, iMenuW, iLabelH);
    presetMenu.setBounds(iMenuX, iMenuY + iLabelH, iMenuW, iMenuH);
    iMenuY += iLabelH + iMenuH + iScaled(5, fUiScale);

    tapTempoButton.setBounds(iMenuX, iMenuY, iMenuW, iMenuH);
    iMenuY += iMenuH + iScaled(8, fUiScale);

    resetDefaultsButton.setBounds(iMenuX, iMenuY, iMenuW, iMenuH);
    iMenuY += iMenuH + iScaled(10, fUiScale);

    bypassLabel.setBounds(iMenuX, iMenuY, iMenuW, iLabelH);
    bypassButton.setBounds(iMenuX, iMenuY + iLabelH, iMenuW, iMenuH);

    const int iModulationX = iMenuX + iMenuW + iScaled(10, fUiScale);
    const int iModulationY = iScaled(60, fUiScale);
    const int iModulationW = getWidth() - iModulationX - iMargin;
    const int iModulationH = getHeight() - iModulationY - iMargin;
    modulation.setBounds(iModulationX, iModulationY, juce::jmax(iScaled(420, fUiScale), iModulationW), iModulationH);

    const int iMeterW = iScaled(140, fUiScale);
    const int iMeterX = iModulationX + modulation.getWidth() - iMeterW;
    int iMeterY = iModulationY + iScaled(260, fUiScale);

    inputMeterLabel.setBounds(iMeterX, iMeterY, iMeterW, iLabelH);
    inputMeterBar.setBounds(iMeterX, iMeterY + iLabelH, iMeterW, iScaled(14, fUiScale));
    iMeterY += iLabelH + iScaled(20, fUiScale);

    outputMeterLabel.setBounds(iMeterX, iMeterY, iMeterW, iLabelH);
    outputMeterBar.setBounds(iMeterX, iMeterY + iLabelH, iMeterW, iScaled(14, fUiScale));
}

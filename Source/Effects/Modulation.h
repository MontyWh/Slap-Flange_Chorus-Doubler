/*
  ==============================================================================

    Plugin: AutoTremolando
    GitHub: MontyWh
    Author: Montague Whishaw
    Date/Time: 24th April 2026
    General Language: English (UK)

    This file contains the basic framework code for a custom JUCE component.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Tremolo.h"
#include <algorithm>
#include <array>
#include <vector>

//==============================================================================
/*
*/
class Modulation  : public juce::Component
{
public:
    static constexpr int iBandCount = 4;
	static constexpr int iNoteDivisionCount = 15;

    Modulation()
    {
        // In your constructor, you should add any child components, and
        // initialise any special settings that your component needs.

        addAndMakeVisible(subTremMenu);
        addAndMakeVisible(bassTremMenu);
        addAndMakeVisible(midTremMenu);
        addAndMakeVisible(trebleTremMenu);

        addAndMakeVisible(subTremLabel);
        addAndMakeVisible(bassTremLabel);
        addAndMakeVisible(midTremLabel);
        addAndMakeVisible(trebleTremLabel);

        addAndMakeVisible(startPhaseSlider);
        addAndMakeVisible(startPhaseLabel);

        addAndMakeVisible(tempoSyncButton);
        addAndMakeVisible(tempoSyncLabel);

        addAndMakeVisible(rateLockButton);
        addAndMakeVisible(retriggerButton);
        addAndMakeVisible(rateLockLabel);
        addAndMakeVisible(retriggerLabel);

        addAndMakeVisible(surroundWidthSlider);
        addAndMakeVisible(surroundWidthLabel);
        addAndMakeVisible(depthModeMenu);
        addAndMakeVisible(depthModeLabel);

        addAndMakeVisible(subNoteDivSlider);
        addAndMakeVisible(bassNoteDivSlider);
        addAndMakeVisible(midNoteDivSlider);
        addAndMakeVisible(trebleNoteDivSlider);

        addAndMakeVisible(subNoteDivLabel);
        addAndMakeVisible(subNoteDivValueLabel);
        addAndMakeVisible(bassNoteDivLabel);
        addAndMakeVisible(bassNoteDivValueLabel);
        addAndMakeVisible(midNoteDivLabel);
        addAndMakeVisible(midNoteDivValueLabel);
        addAndMakeVisible(trebleNoteDivLabel);
        addAndMakeVisible(trebleNoteDivValueLabel);
    }

    ~Modulation() override
    {
    }

    //==============================================================================
    void initialisePhaseState(std::vector<float>& phaseOffsets,
        std::vector<std::array<float, iBandCount>>& phasePositions,
        int numInputChannels,
        int numOutputChannels,
		float startPhaseDegrees) // Initialise the phase state for the tremolo effect
    {
        trem.initialisePhaseState(phaseOffsets, phasePositions, numInputChannels, numOutputChannels, startPhaseDegrees);
    }

	void retriggerPhases(std::vector<std::array<float, iBandCount>>& phasePositions, float startPhaseDegrees) // Retrigger the phases for the tremolo effect
    {
        trem.retriggerPhases(phasePositions, startPhaseDegrees);
    }

    void processRates(std::array<float, iBandCount>& rates, const std::array<int, iBandCount>& divisionIndices, float bpm, bool bApplyTempoSync, bool bApplyRateLock)
    {
		trem.processRates(rates, divisionIndices, bpm, bApplyTempoSync, bApplyRateLock); // Process the rates for the tremolo effect
    }

    void prepareModulation(std::vector<float>& phaseOffsets,
        std::array<float, iBandCount>& channelRates,
        std::array<float, iBandCount>& channelDepths,
        const std::array<float, iBandCount>& rates,
        const std::array<float, iBandCount>& depths,
        float phaseOffsetDegrees,
        float surroundWidth,
        float rateOffset,
        float depthOffset,
        int channelIndex,
        int totalNumInputChannels,
		int totalNumOutputChannels) // Prepare the mod for the tremolo effect
    {
        trem.prepareModulation(phaseOffsets,
            channelRates,
            channelDepths,
            rates,
            depths,
            phaseOffsetDegrees,
            surroundWidth,
            rateOffset,
            depthOffset,
            channelIndex,
            totalNumInputChannels,
            totalNumOutputChannels);
    }

    void applyBandTremolo(std::array<float, iBandCount>& bandValues,
        std::vector<std::array<float, iBandCount>>& phasePositions,
        int channelIndex,
        const std::array<float, iBandCount>& channelRates,
        const std::vector<float>& phaseOffsets,
        const std::array<int, iBandCount>& choices,
        float pulseWidth,
        const std::array<float, iBandCount>& channelDepths,
        int depthMode,
        float sampleRate) // Apply the band tremolo effect
    {
        trem.applyBandTremolo(bandValues,
            phasePositions,
            channelIndex,
            channelRates,
            phaseOffsets,
            choices,
            pulseWidth,
            channelDepths,
            depthMode,
            sampleRate);
    }

	void initialiseControls(juce::AudioProcessorValueTreeState& apvts, float fUiScale) // Initialise the controls for the mod component
    {

        // Helper lambda for slider setup
        auto setupSlider = [fUiScale](juce::Slider& s)
        {
            s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 
                             juce::roundToInt(60.0f * fUiScale), 
                             juce::roundToInt(20.0f * fUiScale));
        };

        setupSlider(startPhaseSlider);
        startPhaseSlider.setTextValueSuffix(" deg");
        startPhaseLabel.setText("Start Phase", juce::dontSendNotification);
        startPhaseLabel.setJustificationType(juce::Justification::centred);
        startPhaseAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, "START_PHASE", startPhaseSlider);

        juce::ComboBox* menus[4] = {
            &subTremMenu, &bassTremMenu, &midTremMenu, &trebleTremMenu
        };
        const juce::StringArray sWaveNames{ "Sine", "Triangle", "Sawtooth", "Pulse", "Square" };
        for (auto* m : menus)
			m->addItemList(sWaveNames, 1); // Add wave type options to the combo boxes

        subTremLabel.setText("Sub Type", juce::dontSendNotification);
        bassTremLabel.setText("Bass Type", juce::dontSendNotification);
        midTremLabel.setText("Mid Type", juce::dontSendNotification);
        trebleTremLabel.setText("Treble Type", juce::dontSendNotification);

        subTremLabel.setJustificationType(juce::Justification::centred);
        bassTremLabel.setJustificationType(juce::Justification::centred);
        midTremLabel.setJustificationType(juce::Justification::centred);
        trebleTremLabel.setJustificationType(juce::Justification::centred);

        subTremAttach = std::make_unique<MenuAttachment>(apvts, "SUB_TREMOLO", subTremMenu);
        bassTremAttach = std::make_unique<MenuAttachment>(apvts, "BASS_TREMOLO", bassTremMenu);
        midTremAttach = std::make_unique<MenuAttachment>(apvts, "MID_TREMOLO", midTremMenu);
        trebleTremAttach = std::make_unique<MenuAttachment>(apvts, "TREBLE_TREMOLO", trebleTremMenu);

        tempoSyncLabel.setText("Sync Mode", juce::dontSendNotification);
        tempoSyncLabel.setJustificationType(juce::Justification::centred);
        tempoSyncButton.setButtonText("TIME");

        rateLockLabel.setText("Rate Lock", juce::dontSendNotification);
        rateLockLabel.setJustificationType(juce::Justification::centred);

        rateLockButton.setClickingTogglesState(true);
        rateLockAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            apvts, "RATE_LOCK", rateLockButton);
        rateLockButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::darkcyan);
        rateLockButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
        rateLockButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        rateLockButton.setTooltip("Links Bass, Mid and Treble to the Sub rate or division");
        rateLockButton.setButtonText(rateLockButton.getToggleState() ? "Linked" : "Unlinked");

        retriggerLabel.setText("Retrigger", juce::dontSendNotification);
        retriggerLabel.setJustificationType(juce::Justification::centred);

        retriggerButton.setClickingTogglesState(true);
        retriggerButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::darkcyan);
        retriggerButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkslategrey);
        retriggerButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
        retriggerButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        retriggerButton.setTooltip("When enabled, tremolo restarts from Start Phase each time playback starts");
        retriggerAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            apvts, "RETRIGGER_ON_PLAY", retriggerButton);

        rateLockButton.setToggleState(*apvts.getRawParameterValue("RATE_LOCK") > 0.5f, juce::dontSendNotification);
        retriggerButton.setToggleState(*apvts.getRawParameterValue("RETRIGGER_ON_PLAY") > 0.5f, juce::dontSendNotification);
        retriggerButton.setButtonText(retriggerButton.getToggleState() ? "Retrig" : "Free");
        surroundWidthLabel.setText("Surround Width", juce::dontSendNotification);
        surroundWidthLabel.setJustificationType(juce::Justification::centred);

        setupSlider(surroundWidthSlider);
        surroundWidthSlider.setTextValueSuffix(" %");
        surroundWidthAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, "SURROUND_WIDTH", surroundWidthSlider);

        depthModeLabel.setText("Depth Mode", juce::dontSendNotification);
        depthModeLabel.setJustificationType(juce::Justification::centred);

        depthModeMenu.addItemList({ "Unipolar", "Bipolar" }, 1);
        depthModeAttach = std::make_unique<MenuAttachment>(apvts, "DEPTH_MODE", depthModeMenu);

        auto setupDivisionSlider = [](juce::Slider& slider, juce::Label& label, juce::Label& valueLabel)
        {
            slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
            slider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
            slider.setRange(0.0, 14.0, 1.0);
            slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::cyan);

            label.setJustificationType(juce::Justification::centred);
            valueLabel.setJustificationType(juce::Justification::centred);
        };

        setupDivisionSlider(subNoteDivSlider, subNoteDivLabel, subNoteDivValueLabel);
        setupDivisionSlider(bassNoteDivSlider, bassNoteDivLabel, bassNoteDivValueLabel);
        setupDivisionSlider(midNoteDivSlider, midNoteDivLabel, midNoteDivValueLabel);
        setupDivisionSlider(trebleNoteDivSlider, trebleNoteDivLabel, trebleNoteDivValueLabel);

        subNoteDivAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, "SUB_NOTE_DIV", subNoteDivSlider);
        bassNoteDivAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, "BASS_NOTE_DIV", bassNoteDivSlider);
        midNoteDivAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, "MID_NOTE_DIV", midNoteDivSlider);
        trebleNoteDivAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, "TREBLE_NOTE_DIV", trebleNoteDivSlider);
    }

    //==============================================================================
    void paint (juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::darkslategrey);
        g.setColour(juce::Colours::white);
        g.drawRect(getLocalBounds(), 1);

        g.setColour(juce::Colours::lightgrey);
        g.setFont(juce::Font(13.0f));
        auto titleArea = getLocalBounds();
        titleArea = titleArea.withTrimmedTop(5).withHeight(20);
        g.drawText("Modulation Matrix", titleArea, juce::Justification::centred, true);
    }

    void resized() override
    {
        // This method is where you should set the bounds of any child
        // components that your component contains..

        const int iLabelH = 20;
        const int iMenuH = 25;
        const int iColGap = 10;
        const int iSliderSize = 60;
        const int iTitleOffset = 30;

        const int iMenuX = 8 + iTitleOffset;
        int iMenuY = 10 + iTitleOffset;
        const int iMenuW = 140;

        startPhaseSlider.setBounds(iMenuX, iMenuY, iMenuW, 100);
        startPhaseLabel.setBounds(iMenuX, iMenuY + 100, iMenuW, iLabelH);
        iMenuY += 130;

        subTremLabel.setBounds(iMenuX, iMenuY, iMenuW, iLabelH);
        subTremMenu.setBounds(iMenuX, iMenuY + iLabelH, iMenuW, iMenuH);
        iMenuY += iLabelH + iMenuH + 8;

        bassTremLabel.setBounds(iMenuX, iMenuY, iMenuW, iLabelH);
        bassTremMenu.setBounds(iMenuX, iMenuY + iLabelH, iMenuW, iMenuH);
        iMenuY += iLabelH + iMenuH + 8;

        midTremLabel.setBounds(iMenuX, iMenuY, iMenuW, iLabelH);
        midTremMenu.setBounds(iMenuX, iMenuY + iLabelH, iMenuW, iMenuH);
        iMenuY += iLabelH + iMenuH + 8;

        trebleTremLabel.setBounds(iMenuX, iMenuY, iMenuW, iLabelH);
        trebleTremMenu.setBounds(iMenuX, iMenuY + iLabelH, iMenuW, iMenuH);

        const int iTempoX = iMenuX + iMenuW + iColGap;
        int iTempoY = 10;
        const int iTempoColW = 130;

        tempoSyncLabel.setBounds(iTempoX, iTempoY, iTempoColW, iLabelH);
        tempoSyncButton.setBounds(iTempoX, iTempoY + iLabelH, iTempoColW, iMenuH);
        iTempoY += iLabelH + iMenuH + 16;

        subNoteDivLabel.setBounds(iTempoX, iTempoY, iTempoColW, iLabelH);
        subNoteDivSlider.setBounds(iTempoX + (iTempoColW - iSliderSize) / 2, iTempoY + iLabelH, iSliderSize, iSliderSize);
        subNoteDivValueLabel.setBounds(iTempoX, iTempoY + iLabelH + iSliderSize, iTempoColW, iLabelH);
        iTempoY += iLabelH + iSliderSize + iLabelH + 16;

        bassNoteDivLabel.setBounds(iTempoX, iTempoY, iTempoColW, iLabelH);
        bassNoteDivSlider.setBounds(iTempoX + (iTempoColW - iSliderSize) / 2, iTempoY + iLabelH, iSliderSize, iSliderSize);
        bassNoteDivValueLabel.setBounds(iTempoX, iTempoY + iLabelH + iSliderSize, iTempoColW, iLabelH);
        iTempoY += iLabelH + iSliderSize + iLabelH + 16;

        midNoteDivLabel.setBounds(iTempoX, iTempoY, iTempoColW, iLabelH);
        midNoteDivSlider.setBounds(iTempoX + (iTempoColW - iSliderSize) / 2, iTempoY + iLabelH, iSliderSize, iSliderSize);
        midNoteDivValueLabel.setBounds(iTempoX, iTempoY + iLabelH + iSliderSize, iTempoColW, iLabelH);
        iTempoY += iLabelH + iSliderSize + iLabelH + 16;

        trebleNoteDivLabel.setBounds(iTempoX, iTempoY, iTempoColW, iLabelH);
        trebleNoteDivSlider.setBounds(iTempoX + (iTempoColW - iSliderSize) / 2, iTempoY + iLabelH, iSliderSize, iSliderSize);
        trebleNoteDivValueLabel.setBounds(iTempoX, iTempoY + iLabelH + iSliderSize, iTempoColW, iLabelH);

        const int iExtraX = iTempoX + iTempoColW + iColGap;
        int iExtraY = 10;
        const int iExtraColW = 140;

        rateLockLabel.setBounds(iExtraX, iExtraY, iExtraColW, iLabelH);
        rateLockButton.setBounds(iExtraX, iExtraY + iLabelH, iExtraColW, iMenuH);
        iExtraY += iLabelH + iMenuH + 8;

        retriggerLabel.setBounds(iExtraX, iExtraY, iExtraColW, iLabelH);
        retriggerButton.setBounds(iExtraX, iExtraY + iLabelH, iExtraColW, iMenuH);
        iExtraY += iLabelH + iMenuH + 8;

        surroundWidthLabel.setBounds(iExtraX, iExtraY, iExtraColW, iLabelH);
        surroundWidthSlider.setBounds(iExtraX, iExtraY + iLabelH, iExtraColW, 120);
        iExtraY += iLabelH + 120 + 8;

        depthModeLabel.setBounds(iExtraX, iExtraY, iExtraColW, iLabelH);
        depthModeMenu.setBounds(iExtraX, iExtraY + iLabelH, iExtraColW, iMenuH);
    }

public:
    juce::ComboBox subTremMenu, bassTremMenu, midTremMenu, trebleTremMenu;
    juce::Label subTremLabel, bassTremLabel, midTremLabel, trebleTremLabel;

    juce::Slider startPhaseSlider;
    juce::Label startPhaseLabel;

    juce::TextButton tempoSyncButton;
    juce::Label tempoSyncLabel;

    juce::TextButton rateLockButton;
    juce::TextButton retriggerButton;
    juce::Label rateLockLabel, retriggerLabel;

    juce::Slider surroundWidthSlider;
    juce::Label surroundWidthLabel;
    juce::ComboBox depthModeMenu;
    juce::Label depthModeLabel;

    juce::Slider subNoteDivSlider, bassNoteDivSlider, midNoteDivSlider, trebleNoteDivSlider;
    juce::Label subNoteDivLabel, subNoteDivValueLabel, bassNoteDivLabel, bassNoteDivValueLabel,
                midNoteDivLabel, midNoteDivValueLabel, trebleNoteDivLabel, trebleNoteDivValueLabel;

    using MenuAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<MenuAttachment> subTremAttach, bassTremAttach, midTremAttach, trebleTremAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> startPhaseAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> rateLockAttach, retriggerAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> surroundWidthAttach;
    std::unique_ptr<MenuAttachment> depthModeAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        subNoteDivAttach, bassNoteDivAttach, midNoteDivAttach, trebleNoteDivAttach;

    std::vector<float> fPhaseOffset;
    std::vector<std::array<float, iBandCount>> fPhasePos;

    Tremolo trem;

private:

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Modulation)
};


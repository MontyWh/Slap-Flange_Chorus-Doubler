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
#include <algorithm>
#include <array>
#include <vector>
#include <cmath>

//==============================================================================
/*
*/
class Modulation  : public juce::Component
{
public:
    static constexpr int iBandCount = 4;
    static constexpr int iNoteDivisionCount = 15;

    using BandFloatArray = std::array<float, iBandCount>;
    using BandIntArray = std::array<int, iBandCount>;

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

        addAndMakeVisible(tempoSyncSlider);
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

    static void initialisePhaseState(std::vector<float>& phaseOffsets,
        std::vector<BandFloatArray>& phasePositions,
        int numInputChannels,
        int numOutputChannels,
        float startPhaseDegrees)
    {
        phaseOffsets.assign(numOutputChannels, 0.0f);
        phasePositions.assign(static_cast<size_t>(numInputChannels), { 0.0f, 0.0f, 0.0f, 0.0f });
        retriggerPhases(phasePositions, startPhaseDegrees);
    }

    static void retriggerPhases(std::vector<BandFloatArray>& phasePositions, float startPhaseDegrees)
    {
        const float fStartPhaseRadians = juce::degreesToRadians(startPhaseDegrees);

        for (auto& fChannelPhases : phasePositions)
            for (int iBand = 0; iBand < iBandCount; ++iBand)
                fChannelPhases[static_cast<size_t>(iBand)] = fStartPhaseRadians;
    }

    static void applyTempoSync(BandFloatArray& rates, const BandIntArray& divisionIndices, float bpm)
    {
        static constexpr float fTempoMultipliers[iNoteDivisionCount] =
        {
            0.25f, 0.166667f, 0.375f,
            0.5f, 0.333333f, 0.75f,
            1.0f, 0.666667f, 1.5f,
            2.0f, 1.333333f, 3.0f,
            4.0f, 2.666667f, 6.0f
        };

        const float fBeatsPerSecond = bpm / 60.0f;

        for (int iBand = 0; iBand < iBandCount; ++iBand)
        {
            const int iClampedIndex = std::clamp(divisionIndices[static_cast<size_t>(iBand)], 0, iNoteDivisionCount - 1);
            rates[static_cast<size_t>(iBand)] = fBeatsPerSecond * fTempoMultipliers[iClampedIndex];
        }
    }

    static void applyRateLock(BandFloatArray& rates)
    {
        for (int iBand = 1; iBand < iBandCount; ++iBand)
            rates[static_cast<size_t>(iBand)] = rates[0];
    }

    static void updatePhaseOffsets(std::vector<float>& phaseOffsets,
        int totalNumInputChannels,
        float phaseOffsetDegrees,
        float surroundWidth)
    {
        const float fPhaseOffsetRadians = juce::degreesToRadians(phaseOffsetDegrees);

        if (surroundWidth <= 0.0f || totalNumInputChannels <= 1)
        {
            for (int iChannel = 0; iChannel < totalNumInputChannels; ++iChannel)
                phaseOffsets[static_cast<size_t>(iChannel)] = 0.0f;

            return;
        }

        const float fMaxOffset = fPhaseOffsetRadians * surroundWidth;
        for (int iChannel = 0; iChannel < totalNumInputChannels; ++iChannel)
            phaseOffsets[static_cast<size_t>(iChannel)] = fMaxOffset * (static_cast<float>(iChannel) / static_cast<float>(totalNumInputChannels - 1));
    }

    static float wrapPhase(float phase)
    {
        const float fDoublePi = juce::MathConstants<float>::twoPi;

        while (phase > fDoublePi)
            phase -= fDoublePi;

        return phase;
    }

    static float getOscillatorValue(int choice, float phase, float pulseWidth)
    {
        if (choice == 0)
            return std::sin(phase);

        if (choice == 1)
            return (phase < juce::MathConstants<float>::pi) ? (-1.0f + ((2.0f / juce::MathConstants<float>::pi) * phase)) : (3.0f - ((2.0f / juce::MathConstants<float>::pi) * phase));

        if (choice == 2)
            return (phase / juce::MathConstants<float>::pi) - 1.0f;

        if (choice == 3)
            return (phase < pulseWidth * juce::MathConstants<float>::twoPi) ? 1.0f : -1.0f;

        if (choice == 4)
            return (phase < juce::MathConstants<float>::pi) ? 1.0f : -1.0f;

        return 0.0f;
    }

    static float getTremoloGain(float oscillatorValue, float depth, int depthMode)
    {
        if (depthMode == 0)
            return (1.0f - depth) + (depth * ((oscillatorValue + 1.0f) * 0.5f));

        return std::max(0.0f, 1.0f + (oscillatorValue * depth));
    }

    void paint (juce::Graphics& g) override
    {
        /* This demo code just fills the component's background and
           draws some placeholder text to get you started.

           You should replace everything in this method with your own
           drawing code..
        */
    }

    void resized() override
    {
        // This method is where you should set the bounds of any child
        // components that your component contains..

        const int iLabelH = 20;
        const int iMenuH = 25;
        const int iColGap = 10;
        const int iSliderSize = 60;

        const int iMenuX = 8;
        int iMenuY = 10;
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
        tempoSyncSlider.setBounds(iTempoX, iTempoY + iLabelH, iTempoColW, iMenuH);
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

    juce::TextButton tempoSyncSlider;
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

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Modulation)
};

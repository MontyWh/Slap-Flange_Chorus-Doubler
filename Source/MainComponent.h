#pragma once

#include <JuceHeader.h>
#include "Data/EffectExtra.h"

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent  : public juce::AudioAppComponent
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    //==============================================================================
    void paint (juce::Graphics& g) override;
    void resized() override;

    //==============================================================================
    float NoiseGate(float monoMix, float control, float fReduction);

private:
    //==============================================================================
    // Your private member variables go here...

    juce::dsp::IIR::Filter<float> subBass; // LPF
    juce::dsp::IIR::Filter<float> bassLower; // BPF
    juce::dsp::IIR::Filter<float> bassUpper; // BPF
    juce::dsp::IIR::Filter<float> midLower; // BPF
    juce::dsp::IIR::Filter<float> midUpper; // BPF
    juce::dsp::IIR::Filter<float> treble; // HPF

    juce::dsp::IIR::Filter<float> resonanceFilter; // BPF

    float fSampleRate;
	
	int iMeasuredLength = fSampleRate; // to later get the sample rate instead
    int iMeasuredItems = 0;
    float fPeak = 0.0f; // initially there is no peak value
    float fGateGain = 0; // initially the gate is closed
    float fGateTarget = 0.0f; // the gate is opening/closing
    juce::dsp::IIR::Filter<float> fFilter;

    float vinylCounter;

    float meterCounter = 0;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};

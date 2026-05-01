#pragma once

#include <JuceHeader.h>
#include "Data/EffectExtra.h"

const int iNUMBER_OF_PARAMETERS = 12;

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
    double NoiseGate(double dMonoMix, double dControl, double dReduction);

private:
    //==============================================================================
    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();
    juce::AudioProcessorValueTreeState APVTS;
    juce::Slider parameters[iNUMBER_OF_PARAMETERS];
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, iNUMBER_OF_PARAMETERS> paramAttach;

    //==============================================================================
    // Your private member variables go here...

    juce::dsp::IIR::Filter<double> subBass; // LPF
    juce::dsp::IIR::Filter<double> bassLower; // BPF
    juce::dsp::IIR::Filter<double> bassUpper; // BPF
    juce::dsp::IIR::Filter<double> midLower; // BPF
    juce::dsp::IIR::Filter<double> midUpper; // BPF
    juce::dsp::IIR::Filter<double> treble; // HPF

    juce::dsp::IIR::Filter<double> resonanceFilter; // BPF

    double dSampleRate;
	
	int iMeasuredLength = dSampleRate; // to later get the sample rate instead
    int iMeasuredItems = 0;
    double dPeak = 0.0; // initially there is no peak value
    double dGateGain = 0; // initially the gate is closed
    double dGateTarget = 0.0; // the gate is opening/closing
    juce::dsp::IIR::Filter<double> dFilter;

    double dVinylCounter;

    double dMeterCounter = 0;

    double dPi = juce::MathConstants<double>::pi;

    


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};

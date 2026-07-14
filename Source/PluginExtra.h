/*
  ==============================================================================

	Tremolo DSP utilities used by the processor.
	TremoloEffect encapsulates oscillator phase tracking and per-band
	modulation gain calculation (sine/triangle/saw/pulse/square).

	Plugin: AutoTremolando
	GitHub: MontyWh
	Author: Montague Whishaw

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <corecrt_math_defines.h>
#include <vector>

class TremoloEffect
{
public:
	//==============================================================================
	// Core tremolo layout and shared data types
	//==============================================================================
	static constexpr int iBandCount = 4; // Sub, bass, mid, treble processing lanes
	static constexpr int iNoteDivisionCount = 15; // Supported tempo-synced note divisions
	static constexpr float fDoublePi = static_cast<float>(2.0f * M_PI); // Full oscillator cycle in radians

	using BandFloatArray = std::array<float, iBandCount>;
	using BandIntArray = std::array<int, iBandCount>;

	struct FilterCoefficients
	{
		juce::dsp::IIR::Coefficients<float>::Ptr resonance;
		juce::dsp::IIR::Coefficients<float>::Ptr sub;
		juce::dsp::IIR::Coefficients<float>::Ptr bassLower;
		juce::dsp::IIR::Coefficients<float>::Ptr bassUpper;
		juce::dsp::IIR::Coefficients<float>::Ptr midLower;
		juce::dsp::IIR::Coefficients<float>::Ptr midUpper;
		juce::dsp::IIR::Coefficients<float>::Ptr treble;
	};

	//==============================================================================
	// UI/control mapping and tempo extraction
	//==============================================================================
	TremoloEffect()
	{
		fPhaseInc.fill(0.0f);
	}


	static bool tryCalculateTapTempoBpm(const double currentTapMs, const double previousTapMs, float& bpmOut)
	{
		if (previousTapMs <= 0.0)
			return false;

		const double dDeltaMs = currentTapMs - previousTapMs; // Time between taps
		if (dDeltaMs < 120.0 || dDeltaMs > 2000.0) // Reject accidental double-taps / stale taps
			return false;

		const float fNewBpm = static_cast<float>(60000.0 / dDeltaMs); // ms period -> BPM
		bpmOut = juce::jlimit(40.0f, 240.0f, fNewBpm); // Keep host-safe musical tempo range
		return true;
	}

	//==============================================================================
	// Parameter loading from APVTS
	//==============================================================================
	static void loadFloatParams(
		const juce::AudioProcessorValueTreeState& apvts,
		const std::array<const char*, iBandCount>& paramIds,
		BandFloatArray& values)
	{
		for (int iBand = 0; iBand < iBandCount; ++iBand)
			values[static_cast<size_t>(iBand)] = *apvts.getRawParameterValue(paramIds[static_cast<size_t>(iBand)]); // Fast per-band APVTS read
	}

	static void loadIntParams(
		const juce::AudioProcessorValueTreeState& apvts,
		const std::array<const char*, iBandCount>& paramIds,
		BandIntArray& values)
	{
		for (int iBand = 0; iBand < iBandCount; ++iBand)
			values[static_cast<size_t>(iBand)] = static_cast<int>(*apvts.getRawParameterValue(paramIds[static_cast<size_t>(iBand)]));
	}

	static void loadNoteDivisionIndices(
		const juce::AudioProcessorValueTreeState& apvts,
		const std::array<const char*, iBandCount>& paramIds,
		BandIntArray& values)
	{
		loadIntParams(apvts, paramIds, values);

		for (int iBand = 0; iBand < iBandCount; ++iBand)
			values[static_cast<size_t>(iBand)] = std::clamp(values[static_cast<size_t>(iBand)], 0, iNoteDivisionCount - 1);
	}

	//==============================================================================
	// Rate generation and channel phase spread
	//==============================================================================
	static void applyTempoSyncRates(BandFloatArray& rate, const BandIntArray& divisionIndex, const float bpm)
	{
		static constexpr float fTempoMultipliers[iNoteDivisionCount] = {
			0.25f, 0.166667f, 0.375f,
			0.5f, 0.333333f, 0.75f,
			1.0f, 0.666667f, 1.5f,
			2.0f, 1.333333f, 3.0f,
			4.0f, 2.666667f, 6.0f
		};

		const float fBeatsPerSec = bpm / 60.0f; // Host/global tempo base
		for (int iBand = 0; iBand < iBandCount; ++iBand)
			rate[static_cast<size_t>(iBand)] = fBeatsPerSec * fTempoMultipliers[std::clamp(divisionIndex[static_cast<size_t>(iBand)], 0, iNoteDivisionCount - 1)]; // Division index -> tremolo Hz
	}

	static void applyMasterRateAndLock(BandFloatArray& rate, const float masterRate, const bool rateLock)
	{
		for (int iBand = 0; iBand < iBandCount; ++iBand)
			rate[static_cast<size_t>(iBand)] = std::clamp(rate[static_cast<size_t>(iBand)] * masterRate, 0.5f, 16.0f); // Global rate macro + safety bounds

		if (rateLock)
			for (int iBand = 1; iBand < iBandCount; ++iBand)
				rate[static_cast<size_t>(iBand)] = rate[0]; // Link all bands to band-0 rhythm
	}

	static void computePhaseOffsets(std::vector<float>& phaseOffsetPerChannel, const int numChannels, const float phaseOffsetRadians, const float surroundWidth)
	{
		if (numChannels <= 1 || surroundWidth <= 0.0f)
		{
			for (int iChannel = 0; iChannel < numChannels; ++iChannel)
				phaseOffsetPerChannel[static_cast<size_t>(iChannel)] = 0.0f;
			return;
		}

		const float fMaxOffset = phaseOffsetRadians * surroundWidth; // Width scales the maximum channel spread
		for (int iChannel = 0; iChannel < numChannels; ++iChannel)
			phaseOffsetPerChannel[static_cast<size_t>(iChannel)] = fMaxOffset * (static_cast<float>(iChannel) / static_cast<float>(numChannels - 1)); // Linear L->R phase gradient
	}


	//==============================================================================
	// Smoothing setup and per-block smoothing updates
	//==============================================================================
	static void initialiseGlobalSmoothers(
		juce::LinearSmoothedValue<float>& smoothedInputGain,
		juce::LinearSmoothedValue<float>& smoothedOutputGain,
		juce::LinearSmoothedValue<float>& smoothedWet,
		juce::LinearSmoothedValue<float>& smoothedPulseWidth,
		juce::LinearSmoothedValue<float>& smoothedBypass,
		const double sampleRate)
	{
		smoothedInputGain.reset(sampleRate, 0.02); // 20 ms smoothing to avoid zipper noise
		smoothedOutputGain.reset(sampleRate, 0.02);
		smoothedWet.reset(sampleRate, 0.02);
		smoothedPulseWidth.reset(sampleRate, 0.02);
		smoothedBypass.reset(sampleRate, 0.02);

		smoothedInputGain.setCurrentAndTargetValue(0.5f); // Unity-ish startup balance
		smoothedOutputGain.setCurrentAndTargetValue(0.5f);
		smoothedWet.setCurrentAndTargetValue(1.0f); // Start fully wet (effect path)
		smoothedPulseWidth.setCurrentAndTargetValue(0.5f); // Symmetric pulse/square threshold
		smoothedBypass.setCurrentAndTargetValue(0.0f); // Effect engaged by default
	}

	static void initialiseBandSmoothers(
		juce::LinearSmoothedValue<float> smoothedRate[iBandCount],
		juce::LinearSmoothedValue<float> smoothedDepth[iBandCount],
		const double sampleRate)
	{
		for (int iBand = 0; iBand < iBandCount; ++iBand)
		{
			smoothedRate[iBand].reset(sampleRate, 0.02);
			smoothedDepth[iBand].reset(sampleRate, 0.02);
			smoothedRate[iBand].setCurrentAndTargetValue(5.0f);
			smoothedDepth[iBand].setCurrentAndTargetValue(0.5f);
		}
	}

	static void setGlobalSmoothingTargets(
		juce::LinearSmoothedValue<float>& smoothedInputGain,
		juce::LinearSmoothedValue<float>& smoothedOutputGain,
		juce::LinearSmoothedValue<float>& smoothedWet,
		juce::LinearSmoothedValue<float>& smoothedPulseWidth,
		juce::LinearSmoothedValue<float>& smoothedBypass,
		const float inputGainTarget,
		const float outputGainTarget,
		const float wetDryTarget,
		const float pulseWidthTarget,
		const bool bypass)
	{
		smoothedInputGain.setTargetValue(inputGainTarget);
		smoothedOutputGain.setTargetValue(outputGainTarget);
		smoothedWet.setTargetValue(wetDryTarget);
		smoothedPulseWidth.setTargetValue(pulseWidthTarget);
		smoothedBypass.setTargetValue(bypass ? 1.0f : 0.0f);
	}

	static void setBandSmoothingTargets(
		juce::LinearSmoothedValue<float> smoothedRate[iBandCount],
		juce::LinearSmoothedValue<float> smoothedDepth[iBandCount],
		const BandFloatArray& rate,
		const BandFloatArray& depth)
	{
		for (int iBand = 0; iBand < iBandCount; ++iBand)
		{
			smoothedRate[iBand].setTargetValue(rate[static_cast<size_t>(iBand)]);
			smoothedDepth[iBand].setTargetValue(depth[static_cast<size_t>(iBand)]);
		}
	}

	static void captureSmoothedBandValues(
		juce::LinearSmoothedValue<float> smoothedRate[iBandCount],
		juce::LinearSmoothedValue<float> smoothedDepth[iBandCount],
		BandFloatArray& rate,
		BandFloatArray& depth)
	{
		for (int iBand = 0; iBand < iBandCount; ++iBand)
		{
			rate[static_cast<size_t>(iBand)] = smoothedRate[iBand].getNextValue();
			depth[static_cast<size_t>(iBand)] = smoothedDepth[iBand].getNextValue();
		}
	}

	static void computeChannelRateDepth(
		BandFloatArray& channelRate,
		BandFloatArray& channelDepth,
		const BandFloatArray& smoothedRate,
		const BandFloatArray& smoothedDepth,
		const float rateOffset,
		const float depthOffset,
		const float channelScale)
	{
		for (int iBand = 0; iBand < iBandCount; ++iBand)
		{
			channelRate[static_cast<size_t>(iBand)] = std::clamp(smoothedRate[static_cast<size_t>(iBand)] + (rateOffset * channelScale), 0.5f, 16.0f);
			channelDepth[static_cast<size_t>(iBand)] = std::clamp(smoothedDepth[static_cast<size_t>(iBand)] + (depthOffset * channelScale), 0.0f, 1.0f);
		}
	}

	//==============================================================================
	// Mixing, filter setup, metering, and per-channel DSP
	//==============================================================================
	template <typename SampleType>
	static SampleType mixWetDryAndBypass(const SampleType input, const SampleType dry, const SampleType summedBands, const float wetDryControl, const float outGain, const float bypassMix)
	{
		auto processed = (summedBands * static_cast<SampleType>(wetDryControl))
			+ (dry * (static_cast<SampleType>(1.0f) - static_cast<SampleType>(wetDryControl)));
		processed *= static_cast<SampleType>(outGain);

		return (processed * static_cast<SampleType>(1.0f - bypassMix))
			+ (input * static_cast<SampleType>(bypassMix));
	}

	static FilterCoefficients createFilterCoefficients(const float sampleRate, const float presence)
	{
		FilterCoefficients coeffs;
		const float fPresenceFreq = (presence * 19000.0f) + 1000.0f;

		coeffs.resonance = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, fPresenceFreq, 1.41f, 1.41f);
		coeffs.sub = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 60.0f);
		coeffs.bassLower = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 250.0f);
		coeffs.bassUpper = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 60.0f);
		coeffs.midLower = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 2000.0f);
		coeffs.midUpper = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 250.0f);
		coeffs.treble = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 2000.0f);
		return coeffs;
	}

	static void updateMeterLevel(std::atomic<float>& meter, const float blockPeak)
	{
		meter.store(juce::jmax(blockPeak, meter.load() * 0.9f));
	}

	template <typename FilterArray>
	static void applyFilterCoefficients(
		FilterArray& resonanceFilter,
		FilterArray& subBass,
		FilterArray& bassLower,
		FilterArray& bassUpper,
		FilterArray& midLower,
		FilterArray& midUpper,
		FilterArray& treble,
		const FilterCoefficients& coeffs,
		const int numChannels)
	{
		for (int iChannel = 0; iChannel < numChannels; ++iChannel)
		{
			resonanceFilter[iChannel]->coefficients = coeffs.resonance;
			subBass[iChannel]->coefficients = coeffs.sub;
			bassLower[iChannel]->coefficients = coeffs.bassLower;
			bassUpper[iChannel]->coefficients = coeffs.bassUpper;
			midLower[iChannel]->coefficients = coeffs.midLower;
			midUpper[iChannel]->coefficients = coeffs.midUpper;
			treble[iChannel]->coefficients = coeffs.treble;
		}
	}

	template <typename SampleType, typename FilterArray>
	static void processChannel(
		juce::AudioBuffer<SampleType>& buffer,
		const int channel,
		const int totalChannels,
		juce::LinearSmoothedValue<float>& smoothedInputGain,
		juce::LinearSmoothedValue<float>& smoothedOutputGain,
		juce::LinearSmoothedValue<float>& smoothedWet,
		juce::LinearSmoothedValue<float>& smoothedPulseWidth,
		juce::LinearSmoothedValue<float>& smoothedBypass,
		juce::LinearSmoothedValue<float> smoothedRate[iBandCount],
		juce::LinearSmoothedValue<float> smoothedDepth[iBandCount],
		TremoloEffect& tremolo,
		FilterArray& resonanceFilter,
		FilterArray& subBass,
		FilterArray& bassLower,
		FilterArray& bassUpper,
		FilterArray& midLower,
		FilterArray& midUpper,
		FilterArray& treble,
		const BandIntArray& choice,
		const float sampleRate,
		const float rateOffset,
		const float depthOffset,
		const int depthMode,
		const float phaseOffset,
		float& inputPeak,
		float& outputPeak)
	{
		const float fChannelScale = totalChannels <= 1
			? 0.0f
			: static_cast<float>(channel) / static_cast<float>(totalChannels - 1);
		auto* channelData = buffer.getWritePointer(channel);
		const int iNumSamples = buffer.getNumSamples();

		// Per-sample path: smooth controls -> split bands -> modulate -> recombine
		for (int iSample = 0; iSample < iNumSamples; ++iSample)
		{
			const float fInGain = smoothedInputGain.getNextValue(); // Click-free gain ramps
			const float fOutGain = smoothedOutputGain.getNextValue();
			const float fWetDryControl = smoothedWet.getNextValue();
			const float fPulseWidth = smoothedPulseWidth.getNextValue();
			const float fBypassMix = smoothedBypass.getNextValue();

			BandFloatArray fSmoothRate;
			BandFloatArray fSmoothDepth;
			captureSmoothedBandValues(smoothedRate, smoothedDepth, fSmoothRate, fSmoothDepth);

			BandFloatArray fChannelRate;
			BandFloatArray fChannelDepth;
			computeChannelRateDepth(fChannelRate, fChannelDepth, fSmoothRate, fSmoothDepth, rateOffset, depthOffset, fChannelScale);

			const auto input = channelData[iSample];
			const auto dry = input * static_cast<SampleType>(fInGain);
			const auto wet = static_cast<SampleType>(resonanceFilter[channel]->processSample(static_cast<float>(dry)))
				* static_cast<SampleType>(1.0f - 0.41f);

			BandFloatArray fBand;
			fBand[0] = subBass[channel]->processSample(static_cast<float>(wet)); // Sub lane
			fBand[1] = bassUpper[channel]->processSample(static_cast<float>(wet))
				+ bassLower[channel]->processSample(static_cast<float>(dry)); // Bass band-pass blend
			fBand[2] = midUpper[channel]->processSample(static_cast<float>(wet))
				+ midLower[channel]->processSample(static_cast<float>(dry)); // Mid band-pass blend
			fBand[3] = treble[channel]->processSample(static_cast<float>(wet)); // Treble lane

			tremolo.processChannelBands(channel, fBand, fChannelDepth, choice, fChannelRate, sampleRate, phaseOffset, fPulseWidth, depthMode);

			const auto summedBands = static_cast<SampleType>(fBand[0]) + static_cast<SampleType>(fBand[1])
				+ static_cast<SampleType>(fBand[2]) + static_cast<SampleType>(fBand[3]); // Reconstruct full-spectrum signal
			const auto output = mixWetDryAndBypass(input, dry, summedBands, fWetDryControl, fOutGain, fBypassMix); // Wet/dry and soft bypass crossfade
			channelData[iSample] = output;

			inputPeak = juce::jmax(inputPeak, std::abs(static_cast<float>(input)));
			outputPeak = juce::jmax(outputPeak, std::abs(static_cast<float>(output)));
		}
	}

	//==============================================================================
	// Oscillator state, waveform generation, and tremolo modulation
	//==============================================================================
	void reset(int numChannels)
	{
		fPhasePos.assign(static_cast<size_t>(numChannels), { 0.0f, 0.0f, 0.0f, 0.0f });
	}

	void computePhaseIncrements(const BandFloatArray& rate, const float sampleRate)
	{
		for (int iBand = 0; iBand < iBandCount; ++iBand)
			fPhaseInc[static_cast<size_t>(iBand)] = (fDoublePi * rate[static_cast<size_t>(iBand)]) / sampleRate; // Hz -> radians/sample
	}

	void triangleProcess(const float phase, float& osc)
	{
		if (0 <= phase && phase < M_PI)
			osc = -1.0f + (2.0f / M_PI) * phase;
		else if (M_PI <= phase && phase < 2 * M_PI)
			osc = 3.0f - (2.0f / M_PI) * phase;
	}

	void retrigger(const float startPhase)
	{
		for (auto& fChannelPhases : fPhasePos)
			for (int iBand = 0; iBand < iBandCount; ++iBand)
				fChannelPhases[static_cast<size_t>(iBand)] = startPhase;
	}

	void processBands(
		const int channel,
		BandFloatArray& band,
		const BandFloatArray& depth,
		const BandIntArray& choice,
		const float offset,
		const float pulseWidth,
		const int depthMode)
	{
		for (int iBand = 0; iBand < iBandCount; ++iBand)
		{
			fPhasePos[static_cast<size_t>(channel)][static_cast<size_t>(iBand)] += fPhaseInc[static_cast<size_t>(iBand)]; // Advance LFO phase
			if (fPhasePos[static_cast<size_t>(channel)][static_cast<size_t>(iBand)] > fDoublePi)
				fPhasePos[static_cast<size_t>(channel)][static_cast<size_t>(iBand)] -= fDoublePi; // Wrap [0, 2pi]

			float fPhase = fPhasePos[static_cast<size_t>(channel)][static_cast<size_t>(iBand)] + offset; // Apply per-channel phase offset
			while (fPhase > fDoublePi)
				fPhase -= fDoublePi; // Keep waveform phase bounded

			float fOsc = 0.0f;
			if (choice[static_cast<size_t>(iBand)] == 0)
				fOsc = std::sin(fPhase); // Sine
			else if (choice[static_cast<size_t>(iBand)] == 1)
				triangleProcess(fPhase, fOsc); // Triangle
			else if (choice[static_cast<size_t>(iBand)] == 2)
				fOsc = (fPhase / M_PI) - 1.0f; // Saw
			else if (choice[static_cast<size_t>(iBand)] == 3)
				fOsc = (fPhase < pulseWidth * fDoublePi) ? 1.0f : -1.0f; // Pulse (duty from pulseWidth)
			else if (choice[static_cast<size_t>(iBand)] == 4)
				fOsc = (fPhase < M_PI) ? 1.0f : -1.0f; // Square

			float fTrem = 1.0f;
			if (depthMode == 0)
			{
				const float fUnipolar = (fOsc + 1.0f) * 0.5f; // Map [-1, 1] -> [0, 1]
				fTrem = (1.0f - depth[static_cast<size_t>(iBand)]) + (depth[static_cast<size_t>(iBand)] * fUnipolar); // Classic AM depth blend
			}
			else
			{
				fTrem = std::max(0.0f, 1.0f + (fOsc * depth[static_cast<size_t>(iBand)])); // Bipolar modulation with floor clamp
			}

			band[static_cast<size_t>(iBand)] *= fTrem;
		}
	}

	void processChannelBands(
		const int channel,
		BandFloatArray& band,
		const BandFloatArray& depth,
		const BandIntArray& choice,
		const BandFloatArray& rate,
		const float sampleRate,
		const float offset,
		const float pulseWidth,
		const int depthMode)
	{
		computePhaseIncrements(rate, sampleRate);
		processBands(channel, band, depth, choice, offset, pulseWidth, depthMode);
	}

private:
	//==============================================================================
	// Persistent per-channel LFO state
	//==============================================================================
	std::vector<BandFloatArray> fPhasePos; // Per-channel, per-band oscillator phase state
	BandFloatArray fPhaseInc; // Per-band radians/sample increments for current block
};

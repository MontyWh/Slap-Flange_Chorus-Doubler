/*
  ==============================================================================

	Tremolo DSP utilities used by the processor.
	TremoloProcess encapsulates oscillator phase tracking and per-band
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

constexpr float DOUBLE_PI = static_cast<float>(2.0f * M_PI);

struct TremoloDspUtils
{
	static constexpr int iBandCount = 4;
	static constexpr int iNoteDivisionCount = 15;

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

	static const std::array<const char*, iBandCount>& getRateParamIds()
	{
		static const std::array<const char*, iBandCount> sParamIds = {
			"SUB_TREM_RATE", "BASS_TREM_RATE", "MID_TREM_RATE", "TREBLE_TREM_RATE"
		};

		return sParamIds;
	}

	static const std::array<const char*, iBandCount>& getDepthParamIds()
	{
		static const std::array<const char*, iBandCount> sParamIds = {
			"SUB_TREM_DEPTH", "BASS_TREM_DEPTH", "MID_TREM_DEPTH", "TREBLE_TREM_DEPTH"
		};

		return sParamIds;
	}

	static const std::array<const char*, iBandCount>& getChoiceParamIds()
	{
		static const std::array<const char*, iBandCount> sParamIds = {
			"SUB_TREMOLO", "BASS_TREMOLO", "MID_TREMOLO", "TREBLE_TREMOLO"
		};

		return sParamIds;
	}

	static const std::array<const char*, iBandCount>& getNoteDivisionParamIds()
	{
		static const std::array<const char*, iBandCount> sParamIds = {
			"SUB_NOTE_DIV", "BASS_NOTE_DIV", "MID_NOTE_DIV", "TREBLE_NOTE_DIV"
		};

		return sParamIds;
	}

	static float applyControlCurve(const float value)
	{
		return std::pow(value, 3.0f);
	}

	static float clampRate(const float value)
	{
		return std::clamp(value, 0.5f, 16.0f);
	}

	static float clampDepth(const float value)
	{
		return std::clamp(value, 0.0f, 1.0f);
	}

	static int clampNoteDivisionIndex(const int value)
	{
		return std::clamp(value, 0, iNoteDivisionCount - 1);
	}

	static bool tryCalculateTapTempoBpm(const double currentTapMs, const double previousTapMs, float& bpmOut)
	{
		if (previousTapMs <= 0.0)
			return false;

		const double dDeltaMs = currentTapMs - previousTapMs;
		if (dDeltaMs < 120.0 || dDeltaMs > 2000.0)
			return false;

		const float fNewBpm = static_cast<float>(60000.0 / dDeltaMs);
		bpmOut = juce::jlimit(40.0f, 240.0f, fNewBpm);
		return true;
	}

	static void loadFloatParams(
		const juce::AudioProcessorValueTreeState& apvts,
		const std::array<const char*, iBandCount>& paramIds,
		BandFloatArray& values)
	{
		for (int iBand = 0; iBand < iBandCount; ++iBand)
			values[static_cast<size_t>(iBand)] = *apvts.getRawParameterValue(paramIds[static_cast<size_t>(iBand)]);
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
			values[static_cast<size_t>(iBand)] = clampNoteDivisionIndex(values[static_cast<size_t>(iBand)]);
	}

	static void applyTempoSyncRates(BandFloatArray& rate, const BandIntArray& divisionIndex, const float bpm)
	{
		static constexpr float fTempoMultipliers[iNoteDivisionCount] = {
			0.25f, 0.166667f, 0.375f,
			0.5f, 0.333333f, 0.75f,
			1.0f, 0.666667f, 1.5f,
			2.0f, 1.333333f, 3.0f,
			4.0f, 2.666667f, 6.0f
		};

		const float fBeatsPerSec = bpm / 60.0f;
		for (int iBand = 0; iBand < iBandCount; ++iBand)
			rate[static_cast<size_t>(iBand)] = fBeatsPerSec * fTempoMultipliers[clampNoteDivisionIndex(divisionIndex[static_cast<size_t>(iBand)])];
	}

	static void applyMasterRateAndLock(BandFloatArray& rate, const float masterRate, const bool rateLock)
	{
		for (int iBand = 0; iBand < iBandCount; ++iBand)
			rate[static_cast<size_t>(iBand)] = clampRate(rate[static_cast<size_t>(iBand)] * masterRate);

		if (rateLock)
			for (int iBand = 1; iBand < iBandCount; ++iBand)
				rate[static_cast<size_t>(iBand)] = rate[0];
	}

	static void computePhaseOffsets(std::vector<float>& phaseOffsetPerChannel, const int numChannels, const float phaseOffsetRadians, const float surroundWidth)
	{
		if (numChannels <= 1 || surroundWidth <= 0.0f)
		{
			for (int iChannel = 0; iChannel < numChannels; ++iChannel)
				phaseOffsetPerChannel[static_cast<size_t>(iChannel)] = 0.0f;
			return;
		}

		const float fMaxOffset = phaseOffsetRadians * surroundWidth;
		for (int iChannel = 0; iChannel < numChannels; ++iChannel)
			phaseOffsetPerChannel[static_cast<size_t>(iChannel)] = fMaxOffset * (static_cast<float>(iChannel) / static_cast<float>(numChannels - 1));
	}

	static float computeChannelScale(const int channel, const int totalChannels)
	{
		if (totalChannels <= 1)
			return 0.0f;

		return static_cast<float>(channel) / static_cast<float>(totalChannels - 1);
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
			channelRate[static_cast<size_t>(iBand)] = clampRate(smoothedRate[static_cast<size_t>(iBand)] + (rateOffset * channelScale));
			channelDepth[static_cast<size_t>(iBand)] = clampDepth(smoothedDepth[static_cast<size_t>(iBand)] + (depthOffset * channelScale));
		}
	}

	template <typename SampleType>
	static SampleType mixWetDryAndBypass(const SampleType input, const SampleType dry, const SampleType summedBands, const float wetDryControl, const float outGain, const float bypassMix)
	{
		auto processed = (summedBands * static_cast<SampleType>(wetDryControl))
			+ (dry * (static_cast<SampleType>(1.0f) - static_cast<SampleType>(wetDryControl)));
		processed *= static_cast<SampleType>(outGain);

		return (processed * static_cast<SampleType>(1.0f - bypassMix))
			+ (input * static_cast<SampleType>(bypassMix));
	}

	static float mapPresenceToFrequency(const float presence)
	{
		return (presence * 19000.0f) + 1000.0f;
	}

	static FilterCoefficients createFilterCoefficients(const float sampleRate, const float presence)
	{
		FilterCoefficients coeffs;
		const float fPresenceFreq = mapPresenceToFrequency(presence);

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
};

// Stateless waveform helpers + stateful per-channel/per-band phase storage.
class TremoloProcess
{
public:
	TremoloProcess()
	{
		fPhaseInc.fill(0.0f);
	}

	// Reset phase positions (called in prepareToPlay)
	void reset(int numChannels)
	{
		fPhasePos.assign(static_cast<size_t>(numChannels), { 0.0f, 0.0f, 0.0f, 0.0f });
	}

	// Compute phase increments for this block
	void computePhaseIncrements(const TremoloDspUtils::BandFloatArray& rate, const float sampleRate)
	{
		for (int iBand = 0; iBand < TremoloDspUtils::iBandCount; ++iBand)
			fPhaseInc[static_cast<size_t>(iBand)] = (DOUBLE_PI * rate[static_cast<size_t>(iBand)]) / sampleRate;
	}

	void sineProcess(const float phase, float& osc)
	{
		osc = std::sin(phase);
	}

	void triangleProcess(const float phase, float& osc)
	{
		if (0 <= phase && phase < M_PI)
			osc = -1.0f + (2.0f / M_PI) * phase;
		else if (M_PI <= phase && phase < 2 * M_PI)
			osc = 3.0f - (2.0f / M_PI) * phase;
	}

	void sawtoothProcess(const float phase, float& osc)
	{
		osc = (phase / M_PI) - 1.0f;
	}

	void pulseProcess(const float phase, const float pulseWidth, float& osc)
	{
		osc = (phase < pulseWidth * DOUBLE_PI) ? 1.0f : -1.0f;
	}

	void squareProcess(const float phase, float& osc)
	{
		juce::ignoreUnused(DOUBLE_PI);
		osc = (phase < M_PI) ? 1.0f : -1.0f;
	}

	void retrigger(const float startPhase)
	{
		for (auto& fChannelPhases : fPhasePos)
			for (int iBand = 0; iBand < TremoloDspUtils::iBandCount; ++iBand)
				fChannelPhases[static_cast<size_t>(iBand)] = startPhase;
	}

	// Apply tremolo to each band
	void processBands(
		const int channel,
		TremoloDspUtils::BandFloatArray& band,
		const TremoloDspUtils::BandFloatArray& depth,
		const TremoloDspUtils::BandIntArray& choice,
		const float offset,
		const float pulseWidth,
		const int depthMode)
	{
		for (int iBand = 0; iBand < TremoloDspUtils::iBandCount; ++iBand)
		{
			fPhasePos[static_cast<size_t>(channel)][static_cast<size_t>(iBand)] += fPhaseInc[static_cast<size_t>(iBand)];
			if (fPhasePos[static_cast<size_t>(channel)][static_cast<size_t>(iBand)] > DOUBLE_PI)
				fPhasePos[static_cast<size_t>(channel)][static_cast<size_t>(iBand)] -= DOUBLE_PI;

			float fPhase = fPhasePos[static_cast<size_t>(channel)][static_cast<size_t>(iBand)] + offset;
			while (fPhase > DOUBLE_PI)
				fPhase -= DOUBLE_PI;

			float fOsc = 0.0f;
			if (choice[static_cast<size_t>(iBand)] == 0)
				sineProcess(fPhase, fOsc);
			else if (choice[static_cast<size_t>(iBand)] == 1)
				triangleProcess(fPhase, fOsc);
			else if (choice[static_cast<size_t>(iBand)] == 2)
				sawtoothProcess(fPhase, fOsc);
			else if (choice[static_cast<size_t>(iBand)] == 3)
				pulseProcess(fPhase, pulseWidth, fOsc);
			else if (choice[static_cast<size_t>(iBand)] == 4)
				squareProcess(fPhase, fOsc);

			float fTrem = 1.0f;
			if (depthMode == 0)
			{
				const float fUnipolar = (fOsc + 1.0f) * 0.5f;
				fTrem = (1.0f - depth[static_cast<size_t>(iBand)]) + (depth[static_cast<size_t>(iBand)] * fUnipolar);
			}
			else
			{
				fTrem = std::max(0.0f, 1.0f + (fOsc * depth[static_cast<size_t>(iBand)]));
			}

			band[static_cast<size_t>(iBand)] *= fTrem;
		}
	}

	// Apply tremolo to this channel using effective values
	void processChannelBands(
		const int channel,
		TremoloDspUtils::BandFloatArray& band,
		const TremoloDspUtils::BandFloatArray& depth,
		const TremoloDspUtils::BandIntArray& choice,
		const TremoloDspUtils::BandFloatArray& rate,
		const float sampleRate,
		const float offset,
		const float pulseWidth,
		const int depthMode)
	{
		computePhaseIncrements(rate, sampleRate);
		processBands(channel, band, depth, choice, offset, pulseWidth, depthMode);
	}

private:
	std::vector<TremoloDspUtils::BandFloatArray> fPhasePos;
	TremoloDspUtils::BandFloatArray fPhaseInc;
};

template <typename FilterArray>
void applyTremoloFilterCoefficients(
	FilterArray& resonanceFilter,
	FilterArray& subBass,
	FilterArray& bassLower,
	FilterArray& bassUpper,
	FilterArray& midLower,
	FilterArray& midUpper,
	FilterArray& treble,
	const TremoloDspUtils::FilterCoefficients& coeffs,
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
void processTremoloChannel(
	juce::AudioBuffer<SampleType>& buffer,
	const int channel,
	const int totalChannels,
	juce::LinearSmoothedValue<float>& smoothedInputGain,
	juce::LinearSmoothedValue<float>& smoothedOutputGain,
	juce::LinearSmoothedValue<float>& smoothedWet,
	juce::LinearSmoothedValue<float>& smoothedPulseWidth,
	juce::LinearSmoothedValue<float>& smoothedBypass,
	juce::LinearSmoothedValue<float> smoothedRate[TremoloDspUtils::iBandCount],
	juce::LinearSmoothedValue<float> smoothedDepth[TremoloDspUtils::iBandCount],
	TremoloProcess& tremolo,
	FilterArray& resonanceFilter,
	FilterArray& subBass,
	FilterArray& bassLower,
	FilterArray& bassUpper,
	FilterArray& midLower,
	FilterArray& midUpper,
	FilterArray& treble,
	const TremoloDspUtils::BandIntArray& choice,
	const float sampleRate,
	const float rateOffset,
	const float depthOffset,
	const int depthMode,
	const float phaseOffset,
	float& inputPeak,
	float& outputPeak)
{
	const float fChannelScale = TremoloDspUtils::computeChannelScale(channel, totalChannels);
	auto* channelData = buffer.getWritePointer(channel);
	const int iNumSamples = buffer.getNumSamples();

	for (int iSample = 0; iSample < iNumSamples; ++iSample)
	{
		const float fInGain = smoothedInputGain.getNextValue();
		const float fOutGain = smoothedOutputGain.getNextValue();
		const float fWetDryControl = smoothedWet.getNextValue();
		const float fPulseWidth = smoothedPulseWidth.getNextValue();
		const float fBypassMix = smoothedBypass.getNextValue();

		TremoloDspUtils::BandFloatArray fSmoothRate;
		TremoloDspUtils::BandFloatArray fSmoothDepth;
		TremoloDspUtils::captureSmoothedBandValues(smoothedRate, smoothedDepth, fSmoothRate, fSmoothDepth);

		TremoloDspUtils::BandFloatArray fChannelRate;
		TremoloDspUtils::BandFloatArray fChannelDepth;
		TremoloDspUtils::computeChannelRateDepth(fChannelRate, fChannelDepth, fSmoothRate, fSmoothDepth, rateOffset, depthOffset, fChannelScale);

		const auto input = channelData[iSample];
		const auto dry = input * static_cast<SampleType>(fInGain);
		const auto wet = static_cast<SampleType>(resonanceFilter[channel]->processSample(static_cast<float>(dry)))
			* static_cast<SampleType>(1.0f - 0.41f);

		TremoloDspUtils::BandFloatArray fBand;
		fBand[0] = subBass[channel]->processSample(static_cast<float>(wet));
		fBand[1] = bassUpper[channel]->processSample(static_cast<float>(wet))
			+ bassLower[channel]->processSample(static_cast<float>(dry));
		fBand[2] = midUpper[channel]->processSample(static_cast<float>(wet))
			+ midLower[channel]->processSample(static_cast<float>(dry));
		fBand[3] = treble[channel]->processSample(static_cast<float>(wet));

		tremolo.processChannelBands(channel, fBand, fChannelDepth, choice, fChannelRate, sampleRate, phaseOffset, fPulseWidth, depthMode);

		const auto summedBands = static_cast<SampleType>(fBand[0]) + static_cast<SampleType>(fBand[1])
			+ static_cast<SampleType>(fBand[2]) + static_cast<SampleType>(fBand[3]);
		const auto output = TremoloDspUtils::mixWetDryAndBypass(input, dry, summedBands, fWetDryControl, fOutGain, fBypassMix);
		channelData[iSample] = output;

		inputPeak = juce::jmax(inputPeak, std::abs(static_cast<float>(input)));
		outputPeak = juce::jmax(outputPeak, std::abs(static_cast<float>(output)));
	}
}

#pragma once

// GEMS v2 — Bogaudio voice/filter module ports used by the Cardinal patch (chunk 4.3):
//   FMOp  (phase-modulation operator, 8x oversampled feedback path) — from FMOp.cpp/.hpp
//   LVCF  (single multimode filter w/ fixed pole count)             — from LVCF.cpp/.hpp
// Transcribed 1:1 from BogaudioModules commit 656eaae4 (GPL-3.0, see NOTICE.md);
// rack:: plumbing replaced with plain floats. Patch-relevant paths only are wired
// (unpatched CV inputs omitted; their params kept).

#include "BogModules.h"
#include "bogaudio/oscillator.hpp"
#include "bogaudio/resample.hpp"

namespace gemsv2 {

//==============================================================================
// FMOp — patch instance #5851: ratio -0.5 (=0.5x), fine 0, feedback 0.246, level 1,
// depth 0, env routings off, linearLevel=false, antialias feedback+depth on.
// No GATE/FM/DEPTH/SUSTAIN inputs patched.
struct FMOpVoice {
	static constexpr float amplitude = 5.0f;
	static constexpr int oversample = 8;
	static constexpr float oversampleMixIncrement = 0.01f;

	// params
	float ratioKnob = -0.5f;
	float fineKnob = 0.0f;
	float feedbackKnob = 0.245783f;
	float levelKnob = 1.0f;
	bool linearLevel = false;
	bool antiAliasFeedback = true;

	// engine state
	float feedback = 0.0f;
	float feedbackDelayedSample = 0.0f;
	float level = 0.0f;
	float maxFrequency = 0.0f;
	float buffer[oversample] {};
	float oversampleMix = 0.0f;
	Phasor phasor;
	SineTableOscillator sineTable;
	CICDecimator decimator;
	SlewLimiter feedbackSL;
	SlewLimiter levelSL;
	Amplifier amplifier;

	float out = 0.0f;   // AUDIO_OUTPUT, volts

	void setSampleRate (float sampleRate) {
		phasor.setSampleRate (sampleRate);
		decimator.setParams (sampleRate, oversample);
		maxFrequency = 0.475f * sampleRate;
		feedbackSL.setParams (sampleRate, 5.0f, 1.0f);
		levelSL.setParams (sampleRate, 10.0f, 1.0f);
		sineTable.setInterpolation (SineTableOscillator::INTERPOLATION_OFF);
	}

	void reset() {
		feedbackDelayedSample = 0.0f;
		oversampleMix = 0.0f;
		phasor.resetPhase();
	}

	// modulateChannel — call at Bogaudio modulation cadence (~2.5 ms)
	void modulate (float pitchIn) {
		float ratio = ratioKnob;
		if (ratio < 0.0f) {
			ratio = std::max (1.0f + ratio, 0.01f);
		}
		else {
			ratio *= 9.0f;
			ratio += 1.0f;
		}
		float frequency = pitchIn;
		frequency += fineKnob / 12.0f;
		frequency = cvToFrequency (frequency);
		frequency *= ratio;
		frequency = clampf (frequency, -maxFrequency, maxFrequency);
		phasor.setFrequency (frequency / (float) oversample);

		feedback = feedbackKnob;
		level = levelKnob;
	}

	// processChannel (envelope disabled per patch: no gate, env routings off)
	float process() {
		float fb = feedbackSL.next (feedback);
		bool feedbackOn = fb > 0.001f;

		float lvl = levelSL.next (level);

		float offset = 0.0f;
		if (feedbackOn) {
			offset = fb * feedbackDelayedSample;
		}

		float sample = 0.0f;
		if (lvl > 0.0001f) {
			Phasor::phase_delta_t o = Phasor::radiansToPhase (offset);
			if (feedbackOn && antiAliasFeedback) {
				if (oversampleMix < 1.0f) {
					oversampleMix += oversampleMixIncrement;
				}
			}
			else if (oversampleMix > 0.0f) {
				oversampleMix -= oversampleMixIncrement;
			}

			if (oversampleMix > 0.0f) {
				for (int i = 0; i < oversample; ++i) {
					phasor.advancePhase();
					buffer[i] = sineTable.nextFromPhasor (phasor, o);
				}
				sample = oversampleMix * decimator.next (buffer);
			}
			else {
				phasor.advancePhase (oversample);
			}
			if (oversampleMix < 1.0f) {
				sample += (1.0f - oversampleMix) * sineTable.nextFromPhasor (phasor, o);
			}

			if (linearLevel) {
				sample *= lvl;
			}
			else {
				lvl = (1.0f - lvl) * Amplifier::minDecibels;
				amplifier.setLevel (lvl);
				sample = amplifier.next (sample);
			}
		}
		else {
			phasor.advancePhase (oversample);
		}

		return out = feedbackDelayedSample = amplitude * sample;
	}
};

//==============================================================================
// LVCF — patch instance #9943: freq 0.0790317 (125 Hz), Q 0.4, mode 1 (HP), 4 poles.
struct LVCFPort {
	static constexpr float maxFrequency = 20000.0f;
	static constexpr float minFrequency = MultimodeFilter::minFrequency;

	float frequencyKnob = 0.0790317f;
	float qKnob = 0.4f;
	int modeParam = 1;   // 0 LP, 1 HP, 2 BP, 3 BR
	int poles = 4;       // json "poles"

	MultimodeFilter16 _filter;
	SlewLimiter _frequencySL;
	MultimodeFilter4 _finalHP;
	float _sampleRate = 48000.0f;

	void setSampleRate (float sr) {
		_sampleRate = sr;
		_frequencySL.setParams (sr, 0.5f, frequencyToSemitone (maxFrequency - minFrequency));
		_finalHP.setParams (sr, MultimodeFilter::BUTTERWORTH_TYPE, 2, MultimodeFilter::HIGHPASS_MODE, 80.0f,
		                    MultimodeFilter::minQbw, MultimodeFilter::LINEAR_BANDWIDTH_MODE, MultimodeFilter::MINIMUM_DELAY_MODE);
		reset();
	}

	void reset() { _filter.reset(); _finalHP.reset(); }

	void modulate() {
		float f = clampf (frequencyKnob, 0.0f, 1.0f);
		f *= f;
		f *= maxFrequency;
		f = clampf (f, minFrequency, maxFrequency);
		float frequency = clampf (semitoneToFrequency (_frequencySL.next (frequencyToSemitone (f))), minFrequency, maxFrequency);
		_filter.setParams (_sampleRate, MultimodeFilter::BUTTERWORTH_TYPE, poles,
		                   (MultimodeFilter::Mode) (1 + clampf ((float) modeParam, 0.0f, 4.0f)),
		                   frequency, clampf (qKnob, 0.0f, 1.0f), MultimodeFilter::PITCH_BANDWIDTH_MODE);
	}

	inline float process (float in) { return _finalHP.next (_filter.next (in)); }
};

} // namespace gemsv2

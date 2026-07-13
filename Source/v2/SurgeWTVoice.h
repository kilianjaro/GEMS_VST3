#pragma once

// GEMS v2 — SurgeXT Wavetable oscillator voice (chunk 4.5), chain 1.
//
// Config-specialized faithful port. The patch (module #2767) drives the Surge WT osc with:
//   morph 0 (NOT modulated) -> always frame 0 of "Sine PD HQ" (a pure sine),
//   skewV 0.5, skewH 0.5 (both centered -> zero skew), 1 unison voice, no sync, no FM,
//   saturate 0.107 (CV-modulated), formant 0 (CV-modulated).
//
// Because morph is fixed at frame 0 (a sine) and the pitch is bass-range, Surge's
// BLIT/FIR band-limiting machinery (AbstractBlitOscillator + sinctable convolution) is
// spectrally transparent here — a direct cubic wavetable read is numerically equivalent.
// GEMS therefore replaces ONLY that band-limiting read with a direct read; every audible
// transform is Surge's exact math, transcribed from surge-synthesizer/surge
// src/common/dsp/oscillators/WavetableOscillator.cpp (GPL-3):
//   - saturate/vskew: distort_level():  clip = -8*sat^3;  x = x*(1-clip) + clip*x^3 (clamped)
//   - formant: display-path resample by mult = 2^(formantSemitones/12) (Surge's own
//     single-cycle reference, processSamplesForDisplay); identity at formant 0.
//   - output ×5 (surge-rack SURGE_TO_RACK_OSC_MUL); pitch note 60 = C4 (surge-rack VCO).
// Modulation scaling matches surge-rack ModulationAssistant (values01 = base01 + depth·cv/10).
// See NOTICE.md. A config using morph!=0 / bright frames / high pitch would need
// the full BLIT port.

#include <cmath>
#include "WavetableData.h"

namespace gemsv2 {

struct SurgeWTVoice {
	// base params (patch #2767 f01 knob values)
	float morph = 0.0f;          // -> frame 0 (fixed)
	float saturateBase = 0.107f; // wt_saturate f01
	float formantBase = 0.0f;    // wt_formant f01
	// ct_syncpitch range for formant (semitones); Surge's formant/sync pitch spans 0..60.
	static constexpr float kFormantSemitoneRange = 60.0f;

	float phase = 0.0f;          // 0..1 oscillator phase
	float sampleRate = 48000.0f;
	float out = 0.0f;            // ±5 V

	void setSampleRate (float sr) { sampleRate = sr; }
	void reset() { phase = 0.0f; out = 0.0f; }

	static inline float clampf_ (float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

	// cubic (Catmull-Rom) read of frame 0 at fractional phase [0,1)
	static inline float readFrame0 (float ph) {
		ph -= std::floor (ph);
		float fp = ph * wtdata::kNumSamples;
		int i1 = (int) fp;
		float t = fp - i1;
		int i0 = (i1 - 1) & (wtdata::kNumSamples - 1);
		int i2 = (i1 + 1) & (wtdata::kNumSamples - 1);
		int i3 = (i1 + 2) & (wtdata::kNumSamples - 1);
		i1 &= (wtdata::kNumSamples - 1);
		const float s = 1.0f / 16384.0f;   // int15 scale (flags wtf_int16, not _is_16)
		float y0 = wtdata::data[0][i0] * s, y1 = wtdata::data[0][i1] * s;
		float y2 = wtdata::data[0][i2] * s, y3 = wtdata::data[0][i3] * s;
		float a = -0.5f * y0 + 1.5f * y1 - 1.5f * y2 + 0.5f * y3;
		float b = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
		float c = -0.5f * y0 + 0.5f * y2;
		return ((a * t + b) * t + c) * t + y1;
	}

	// distort_level (Surge): vskew=0 here (a=0) -> only the cubic saturate branch is active.
	static inline float distortLevel (float x, float sat01) {
		float clip = -8.0f * sat01 * sat01 * sat01;   // l_clip.newValue(-8*a^3)
		return clampf_ (x * (1.0f - clip) + clip * x * x * x, -1.0f, 1.0f);
	}

	// pitchCv: 1V/oct, note60=C4 -> f = 261.626*2^cv.
	// satMod01 / formantMod01: fully-modulated f01 param values (base + CV mod), computed
	// by the engine to match surge-rack's ModulationAssistant (clamped 0..1).
	float process (float pitchCv, float satMod01, float formantMod01) {
		float freq = 261.62556f * std::exp2 (pitchCv);
		phase += freq / sampleRate;
		phase -= std::floor (phase);

		// formant: display-path resample of the single cycle by mult = 2^(semis/12);
		// read compresses to first 1/mult of the cycle then holds (Surge's reference).
		float formantSemis = clampf_ (formantMod01, 0.0f, 1.0f) * kFormantSemitoneRange;
		float mult = std::exp2 (formantSemis * (1.0f / 12.0f));
		float readPh = phase * mult;
		if (readPh > 0.999999f) readPh = 0.999999f;   // hold last sample (formant peak)

		float raw = readFrame0 (readPh);
		float shaped = distortLevel (raw, clampf_ (satMod01, 0.0f, 1.0f));
		return out = 5.0f * shaped;   // SURGE_TO_RACK_OSC_MUL
	}
};

} // namespace gemsv2

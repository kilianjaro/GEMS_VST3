#pragma once

// GEMS v2 — two standalone VCO voices used by the Cardinal patch:
//   OsculumVoice  — from Sonus Modular (Valerio Orlandini / Sonus Dept.) src/osculum.cpp
//                   (https://gitlab.com/sonusdept/sonusmodular, GPL-3.0)
//   PalmLoopVoice — from 21kHz (Zach Bornheimer) src/PalmLoop.cpp + src/dsp/math.hpp
//                   (https://github.com/21kHz/21kHz-rack-plugins, GPL-3.0)
// Logic transcribed 1:1 from the originals; rack:: Param/Input/Output plumbing
// replaced with plain floats. Rack conventions kept: audio outputs ±5 V,
// pitch input = 1 V/oct CV in volts. Local adaptations marked `// GEMS:`.

#include <cmath>
#include <array>
#include <algorithm>

namespace gemsv2 {

// GEMS: clampf shared with BogModules.h when both are included; guard against redefinition
#ifndef GEMS_HAVE_CLAMPF
inline float clampf (float v, float lo, float hi) { return std::min (hi, std::max (lo, v)); }
#endif

//==============================================================================
// Osculum (osculum.cpp) — single-phasor VCO with two exotic waveshapers.
// wave3 (cos(cosh(e^sin(2*pi*phase)))) is transcribed for completeness though
// unused by the patch. wave4 depends on the module's constructor-time
// `srand(time(0))`-seeded `arand[8]` table (non-deterministic across runs /
// unrelated to sample-accurate synthesis) — GEMS: omitted, and it is unused by
// the patch (only WAVE1_OUT/WAVE2_OUT are wired).
struct OsculumVoice {
	// params
	float octaveKnob = 0.0f;   // OCTAVE param, range -3..3 (patch: 0.0)

	float phase = 0.0f;
	float sampleRate = 48000.0f;

	float wave1 = 0.0f;   // WAVE1_OUT, ±5 V — used by the patch
	float wave2 = 0.0f;   // WAVE2_OUT, ±5 V — used by the patch
	float wave3 = 0.0f;   // WAVE3_OUT, ±5 V — unused by the patch, kept for completeness

	void setSampleRate (float sr) { sampleRate = sr; }
	void reset() { phase = 0.0f; }

	// Osculum::process, verbatim
	void process (float pitchCv) {
		float pitch = octaveKnob;
		pitch += pitchCv;
		pitch = clampf (pitch, -4.0f, 4.0f);
		float freq = 261.626f * powf (2.0f, pitch);

		phase += freq / sampleRate;
		if (phase >= 1.0f)
			phase -= 1.0f;

		// GEMS: M_E / M_PI (POSIX <math.h> extensions used by the original) replaced
		// with explicit constants of the same value, for strict C++17 portability.
		constexpr float kE = 2.71828182845904523536f;
		constexpr float kPi = 3.14159265358979323846f;

		wave1 = cosf (powf (kE, sinf (2.0f * kPi * phase)));
		wave2 = 0.45f + (2.0f * phase) * sinf (2.0f * kPi * phase);
		if (fabsf (wave2) > 1.0f)
			wave2 = 1.0f * copysignf (1.0f, wave2);
		wave3 = cosf (coshf (powf (kE, sinf (2.0f * kPi * phase))));
		// wave4 = arand[(unsigned int) floorf(phase * 8.0)] * copysign(1.0, phase - 0.5);
		// — omitted, see class comment.

		wave1 *= 5.0f;
		wave2 *= 5.0f;
		wave3 *= 5.0f;
	}
};

//==============================================================================
// PalmLoop (PalmLoop.cpp + dsp/math.hpp) — naive-saw-driven phasor feeding
// four-sample buffers for saw/square/triangle, with polyBLEP (saw/square) and
// polyBLAMP (triangle) discontinuity correction; sine and sub-octave sine are
// closed-form. EXP_FM_INPUT / LIN_FM_INPUT / RESET_INPUT are unpatched in the
// GEMS patch: FM params are kept (unused) and process() takes only the 1V/oct
// pitch CV — this matches the original with isConnected()==false on both FM
// inputs (LIN FM's incr-clamping branch is skipped; EXP FM contributes 0) and
// resetTrigger never firing (constant 0 V never crosses the Schmitt threshold).
// All five outputs are always computed (original gates each on isConnected()).
struct PalmLoopVoice {
	// params (patch: OCT_PARAM=7, COARSE_PARAM=0, FINE_PARAM=0)
	float octKnob = 7.0f;      // OCT_PARAM, range 4..12
	float coarseKnob = 0.0f;   // COARSE_PARAM, range -7..7
	float fineKnob = 0.0f;     // FINE_PARAM, range ±0.083333
	float expFmKnob = 0.0f;    // EXP_FM_PARAM, range ±1.0 — unused, EXP_FM_INPUT unpatched
	float linFmKnob = 0.0f;    // LIN_FM_PARAM, range ±11.7 — unused, LIN_FM_INPUT unpatched

	float phase = 0.0f;
	float oldPhase = 0.0f;
	float square = 1.0f;
	int discont = 0;
	int oldDiscont = 0;

	std::array<float, 4> sawBuffer {};   // GEMS: zero-initialized (original leaves these uninitialized)
	std::array<float, 4> sqrBuffer {};
	std::array<float, 4> triBuffer {};

	float sampleRate = 48000.0f;
	float sampleTime = 1.0f / 48000.0f;
	float log2sampleFreq = 15.4284f;   // PalmLoop's default member initializer

	float sawOut = 0.0f;   // SAW_OUTPUT, ±5 V
	float sqrOut = 0.0f;   // SQR_OUTPUT, ±5 V
	float triOut = 0.0f;   // TRI_OUTPUT, ±5 V
	float sinOut = 0.0f;   // SIN_OUTPUT, ±5 V
	float subOut = 0.0f;   // SUB_OUTPUT, ±5 V — used by the patch

	void setSampleRate (float sr) {
		sampleRate = sr;
		sampleTime = 1.0f / sr;
		// PalmLoop::onSampleRateChange, verbatim
		log2sampleFreq = log2f (sr) - 0.00009f;
	}

	void reset() {
		phase = oldPhase = 0.0f;
		square = 1.0f;
		discont = oldDiscont = 0;
		sawBuffer.fill (0.0f);
		sqrBuffer.fill (0.0f);
		triBuffer.fill (0.0f);
	}

	// dsp/math.hpp: four point, fourth-order b-spline polyblep, verbatim. From:
	// Valimaki, Pekonen, Nam. "Perceptually informed synthesis of bandlimited
	// classical waveforms using integrated polynomial interpolation"
	static void polyblep4 (std::array<float, 4>& buffer, float d, float u) {
		if (d > 1.0f) {
			d = 1.0f;
		}
		else if (d < 0.0f) {
			d = 0.0f;
		}

		float d2 = d * d;
		float d3 = d2 * d;
		float d4 = d3 * d;
		float dd3 = 0.16667f * (d + d3);
		float cd2 = 0.041667f + 0.25f * d2;
		float d4_1 = 0.041667f * d4;

		buffer[3] += u * (d4_1);
		buffer[2] += u * (cd2 + dd3 - 0.125f * d4);
		buffer[1] += u * (-0.5f + 0.66667f * d - 0.33333f * d3 + 0.125f * d4);
		buffer[0] += u * (-cd2 + dd3 - d4_1);
	}

	// dsp/math.hpp: four point, fourth-order b-spline polyblamp, verbatim. From:
	// Esqueda, Valimaki, Bilbao. "Rounding Corners with BLAMP"
	static void polyblamp4 (std::array<float, 4>& buffer, float d, float u) {
		if (d > 1.0f) {
			d = 1.0f;
		}
		else if (d < 0.0f) {
			d = 0.0f;
		}

		float d2 = d * d;
		float d3 = d2 * d;
		float d4 = d3 * d;
		float d5 = d4 * d;
		float d5_1 = 0.0083333f * d5;
		float d5_2 = 0.025f * d5;

		buffer[3] += u * (d5_1);
		buffer[2] += u * (0.0083333f + 0.083333f * (d2 + d3) + 0.041667f * (d + d4) - d5_2);
		buffer[1] += u * (0.23333f - 0.5f * d + 0.33333f * d2 - 0.083333f * d4 + d5_2);
		buffer[0] += u * (0.0083333f + 0.041667f * (d4 - d) + 0.083333f * (d2 - d3) - d5_1);
	}

	// dsp/math.hpp sin_01, verbatim: fast sine calculation, modified from the
	// Reaktor 6 core library. Takes a [0, 1] range and folds it to a triangle
	// on a [0, 0.5] range.
	static float sin01 (float t) {
		if (t > 1.0f) {
			t = 1.0f;
		}
		else if (t > 0.5f) {
			t = 1.0f - t;
		}
		else if (t < 0.0f) {
			t = 0.0f;
		}
		t = 2.0f * t - 0.5f;
		float t2 = t * t;
		t = (((-0.540347f * t2 + 2.53566f) * t2 - 5.16651f) * t2 + 3.14159f) * t;
		return t;
	}

	// PalmLoop::process, verbatim (see class comment for the FM/reset input adaptation)
	void process (float pitchCv) {
		for (int i = 0; i <= 2; ++i) {
			sawBuffer[i] = sawBuffer[i + 1];
			sqrBuffer[i] = sqrBuffer[i + 1];
			triBuffer[i] = triBuffer[i + 1];
		}

		float freq = octKnob + 0.031360f + 0.083333f * coarseKnob + fineKnob + pitchCv;
		// GEMS: `freq += expFmKnob * inputs[EXP_FM_INPUT].getVoltage();` omitted —
		// EXP_FM_INPUT is unpatched (voltage 0), so this term is always 0.
		if (freq >= log2sampleFreq) {
			freq = log2sampleFreq;
		}
		freq = powf (2.0f, freq);
		float incr;
		// GEMS: LIN_FM_INPUT unpatched -> inputs[LIN_FM_INPUT].isConnected() is always
		// false, so only the plain (unclamped) increment branch applies.
		incr = sampleTime * freq;

		phase += incr;
		if (phase >= 0.0f && phase < 1.0f) {
			discont = 0;
		}
		else if (phase >= 1.0f) {
			discont = 1;
			--phase;
			square *= -1.0f;
		}
		else {
			discont = -1;
			++phase;
			square *= -1.0f;
		}

		sawBuffer[3] = phase;
		sqrBuffer[3] = square;
		if (square >= 0.0f) {
			triBuffer[3] = phase;
		}
		else {
			triBuffer[3] = 1.0f - phase;
		}

		// SAW
		if (oldDiscont == 1) {
			polyblep4 (sawBuffer, 1.0f - oldPhase / incr, 1.0f);
		}
		else if (oldDiscont == -1) {
			polyblep4 (sawBuffer, 1.0f - (oldPhase - 1.0f) / incr, -1.0f);
		}
		sawOut = clampf (10.0f * (sawBuffer[0] - 0.5f), -5.0f, 5.0f);

		// SQR
		if (discont == 0) {
			if (oldDiscont == 1) {
				polyblep4 (sqrBuffer, 1.0f - oldPhase / incr, -2.0f * square);
			}
			else if (oldDiscont == -1) {
				polyblep4 (sqrBuffer, 1.0f - (oldPhase - 1.0f) / incr, -2.0f * square);
			}
		}
		else {
			if (oldDiscont == 1) {
				polyblep4 (sqrBuffer, 1.0f - oldPhase / incr, 2.0f * square);
			}
			else if (oldDiscont == -1) {
				polyblep4 (sqrBuffer, 1.0f - (oldPhase - 1.0f) / incr, 2.0f * square);
			}
		}
		sqrOut = clampf (4.9999f * sqrBuffer[0], -5.0f, 5.0f);

		// TRI
		if (discont == 0) {
			if (oldDiscont == 1) {
				polyblamp4 (triBuffer, 1.0f - oldPhase / incr, 2.0f * square * incr);
			}
			else if (oldDiscont == -1) {
				polyblamp4 (triBuffer, 1.0f - (oldPhase - 1.0f) / incr, 2.0f * square * incr);
			}
		}
		else {
			if (oldDiscont == 1) {
				polyblamp4 (triBuffer, 1.0f - oldPhase / incr, -2.0f * square * incr);
			}
			else if (oldDiscont == -1) {
				polyblamp4 (triBuffer, 1.0f - (oldPhase - 1.0f) / incr, -2.0f * square * incr);
			}
		}
		triOut = clampf (10.0f * (triBuffer[0] - 0.5f), -5.0f, 5.0f);

		// SIN
		sinOut = 5.0f * sin01 (phase);

		// SUB
		if (square >= 0.0f) {
			subOut = 5.0f * sin01 (0.5f * phase);
		}
		else {
			subOut = 5.0f * sin01 (0.5f * (1.0f - phase));
		}

		oldPhase = phase;
		oldDiscont = discont;
	}
};

} // namespace gemsv2

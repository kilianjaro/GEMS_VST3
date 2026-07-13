#pragma once

// GEMS v2 — pitch spine of the Cardinal patch (chunk 4.2):
//   AudioToCVPitch (Cardinal, aubio "yinfast") → Befaco SlewLimiter → Quack (Aria QQQQ engine)
//
// Ports are transcribed 1:1 from:
//  - DISTRHO/Cardinal plugins/Cardinal/src/AudioToCVPitch.cpp (GPL-3)
//  - aubio src/pitch/pitchyinfast.c + pitch/pitch.c + mathutils.c (GPL-3),
//    FFT backend replaced by pocketfft (BSD-3) implementing aubio's exact
//    halfcomplex compspec layout and 1/N inverse normalization
//  - VCVRack/Befaco src/SlewLimiter.cpp (GPL-3)
//  - AriaSalvatrice/AriaModules src/Qqqq.cpp + quantizer.hpp (WTFPL/GPL-3)
// See NOTICE.md.

#include <cmath>
#include <vector>
#include <array>
#include <complex>
#include <algorithm>

#include "pocketfft/pocketfft_hdronly.h"

namespace gemsv2 {

//==============================================================================
// aubio yinfast — verbatim logic. Works on a window of B samples, W = B/2.
// With the patch's tolerance = 0, detection always uses the global YIN minimum.
struct YinFast {
	int B = 0, W = 0;
	float tol = 0.15f;
	int peakPos = 0;

	std::vector<float> yin, tmpdata, sqdiff, kernel, samplesFft, kernelFft;
	// pocketfft plan data
	std::vector<std::complex<double>> spec;   // fft_size = B/2+1 bins
	std::vector<double> fftIn, fftOut;

	void init (int bufsize) {
		B = bufsize; W = bufsize / 2;
		yin.assign ((size_t) W, 0.0f);
		tmpdata.assign ((size_t) B, 0.0f);
		sqdiff.assign ((size_t) W, 0.0f);
		kernel.assign ((size_t) B, 0.0f);
		samplesFft.assign ((size_t) B, 0.0f);
		kernelFft.assign ((size_t) B, 0.0f);
		spec.assign ((size_t) (B / 2 + 1), {});
		fftIn.assign ((size_t) B, 0.0);
		fftOut.assign ((size_t) B, 0.0);
	}

	// aubio_fft_do_complex: real input (length B) -> compspec [re0..reW, im(W-1)..im1]
	void fftDoComplex (const float* input, float* compspec) {
		for (int i = 0; i < B; ++i) fftIn[(size_t) i] = (double) input[i];
		pocketfft::r2c ({ (size_t) B }, { sizeof (double) }, { sizeof (std::complex<double>) },
		                0, true, fftIn.data(), spec.data(), 1.0);
		const int fftSize = B / 2 + 1;
		compspec[0] = (float) spec[0].real();
		for (int i = 1; i < fftSize - 1; ++i) {
			compspec[i] = (float) spec[(size_t) i].real();
			compspec[B - i] = (float) spec[(size_t) i].imag();
		}
		compspec[fftSize - 1] = (float) spec[(size_t) (fftSize - 1)].real();
	}

	// aubio_fft_rdo_complex: compspec -> real output, normalized 1/B
	void fftRdoComplex (const float* compspec, float* output) {
		const int fftSize = B / 2 + 1;
		spec[0] = { (double) compspec[0], 0.0 };
		for (int i = 1; i < fftSize - 1; ++i)
			spec[(size_t) i] = { (double) compspec[i], (double) compspec[B - i] };
		spec[(size_t) (fftSize - 1)] = { (double) compspec[fftSize - 1], 0.0 };
		pocketfft::c2r ({ (size_t) B }, { sizeof (std::complex<double>) }, { sizeof (double) },
		                0, false, spec.data(), fftOut.data(), 1.0);
		const float renorm = 1.0f / (float) B;
		for (int i = 0; i < B; ++i) output[i] = (float) fftOut[(size_t) i] * renorm;
	}

	static float quadraticPeakPos (const std::vector<float>& x, int pos) {
		// aubio fvec_quadratic_peak_pos
		float s0, s1, s2;
		if (pos == 0 || pos == (int) x.size() - 1) return (float) pos;
		int x0 = (pos < 1) ? pos : pos - 1;
		int x2 = (pos + 1 < (int) x.size()) ? pos + 1 : pos;
		if (x0 == pos) return (x[(size_t) pos] <= x[(size_t) x2]) ? (float) pos : (float) x2;
		if (x2 == pos) return (x[(size_t) pos] <= x[(size_t) x0]) ? (float) pos : (float) x0;
		s0 = x[(size_t) x0]; s1 = x[(size_t) pos]; s2 = x[(size_t) x2];
		return (float) pos + 0.5f * (s0 - s2) / (s0 - 2.0f * s1 + s2);
	}

	// returns interpolated period in samples (0 if none) — aubio_pitchyinfast_do
	float process (const float* input) {
		const int length = W;
		float tmp2 = 0.0f;

		// r_t(0) + r_t+tau(0)
		{
			for (int i = 0; i < B; ++i) tmpdata[(size_t) i] = input[i] * input[i];  // fvec_weighted_copy(in,in)
			float fs = 0.0f;   // aubio fvec_sum accumulates in smpl_t (float)
			for (int i = 0; i < W; ++i) fs += tmpdata[(size_t) i];
			sqdiff[0] = fs;
			for (int tau = 1; tau < W; ++tau) {
				sqdiff[(size_t) tau] = sqdiff[(size_t) (tau - 1)];
				sqdiff[(size_t) tau] -= tmpdata[(size_t) (tau - 1)];
				sqdiff[(size_t) tau] += tmpdata[(size_t) (W + tau - 1)];
			}
			const float add = sqdiff[0];
			for (int i = 0; i < W; ++i) sqdiff[(size_t) i] += add;
		}
		// r_t(tau) via FFT correlation
		{
			fftDoComplex (input, samplesFft.data());
			// kernel = reversed copy of first half of samples, at offset 1
			std::fill (kernel.begin(), kernel.end(), 0.0f);
			for (int i = 0; i < W; ++i) kernel[(size_t) (1 + i)] = input[W - 1 - i];
			fftDoComplex (kernel.data(), kernelFft.data());

			float* compmul = tmpdata.data();
			compmul[0] = kernelFft[0] * samplesFft[0];
			for (int tau = 1; tau < W; ++tau) {
				compmul[tau] = kernelFft[(size_t) tau] * samplesFft[(size_t) tau];
				compmul[tau] -= kernelFft[(size_t) (B - tau)] * samplesFft[(size_t) (B - tau)];
			}
			compmul[W] = kernelFft[(size_t) W] * samplesFft[(size_t) W];
			for (int tau = 1; tau < W; ++tau) {
				compmul[B - tau] = kernelFft[(size_t) (B - tau)] * samplesFft[(size_t) tau];
				compmul[B - tau] += kernelFft[(size_t) tau] * samplesFft[(size_t) (B - tau)];
			}
			fftRdoComplex (compmul, samplesFft.data());
			for (int tau = 0; tau < W; ++tau)
				yin[(size_t) tau] = sqdiff[(size_t) tau] - 2.0f * samplesFft[(size_t) (tau + W)];
		}

		// cumulative-mean normalize + first-minimum search
		yin[0] = 1.0f;
		for (int tau = 1; tau < length; ++tau) {
			tmp2 += yin[(size_t) tau];
			if (tmp2 != 0.0f) yin[(size_t) tau] *= (float) tau / tmp2;
			else yin[(size_t) tau] = 1.0f;
			int period = tau - 3;
			if (tau > 4 && yin[(size_t) period] < tol && yin[(size_t) period] < yin[(size_t) (period + 1)]) {
				peakPos = period;
				return quadraticPeakPos (yin, peakPos);
			}
		}
		peakPos = (int) (std::min_element (yin.begin(), yin.end()) - yin.begin());
		return quadraticPeakPos (yin, peakPos);
	}

	float confidence() const { return 1.0f - yin[(size_t) peakPos]; }
};

//==============================================================================
// Cardinal AudioToCVPitch — verbatim module logic. Analysis every 1408 samples.
struct AudioToCVPitch {
	static constexpr int kBufferSize = 1024 + 256 + 128;   // 1408

	// params
	float sensitivity = 50.0f;          // 0.1..99 %
	float confidenceThreshold = 12.5f;  // 0..99 %
	float tolerance = 6.25f;            // 0..99 % → aubio tol = ×0.01
	int octave = 0;
	bool holdOutputPitch = true;
	bool smooth = true;

	YinFast yin;
	std::vector<float> inputBuffer;
	int inputBufferPos = 0;
	float sampleRate = 48000.0f;
	float silenceDb = -30.0f;

	float lastKnownPitchInHz = 0.0f;
	float lastKnownPitchConfidence = 0.0f;
	float lastUsedOutputPitch = 0.0f;
	float lastUsedOutputSignal = 0.0f;

	// rack::dsp::SlewLimiter
	float slewOut = 0.0f, slewRise = 0.0f, slewFall = 0.0f;

	// outputs
	float cvPitchOut = 0.0f;   // V (1V/oct, C4 = 0 V + 0.75 ref shift; see formula)
	float gateOut = 0.0f;      // 0/10 V

	void prepare (float sr) {
		sampleRate = sr;
		yin.init (kBufferSize);
		yin.tol = tolerance * 0.01f;
		inputBuffer.assign (kBufferSize, 0.0f);
		inputBufferPos = 0;
		lastKnownPitchInHz = lastKnownPitchConfidence = 0.0f;
		lastUsedOutputPitch = lastUsedOutputSignal = 0.0f;
		const float fall = 1.0f / ((float) kBufferSize / sr);   // 1 V per analysis window
		slewOut = 0.0f; slewRise = fall; slewFall = fall;
		cvPitchOut = gateOut = 0.0f;
	}

	inline float slewProcess (float dt, float in) {
		slewOut = std::max (std::min (in, slewOut + slewRise * dt), slewOut - slewFall * dt);
		return slewOut;
	}

	void processSample (float inVolts) {
		float cvPitch = lastUsedOutputPitch;
		float cvSignal = lastUsedOutputSignal;

		inputBuffer[(size_t) inputBufferPos] = inVolts * 0.1f * sensitivity;

		if (++inputBufferPos == kBufferSize) {
			inputBufferPos = 0;
			yin.tol = tolerance * 0.01f;

			// aubio_pitch_do (slideblock = full replace; yinfast; silence gate; Hz)
			float period = yin.process (inputBuffer.data());
			float detectedPitchInHz = (period > 0.0f) ? sampleRate / period : 0.0f;
			// silence: 10*log10(mean(x^2)) < -30 dB → pitch 0
			{
				float energy = 0.0f;
				for (int i = 0; i < kBufferSize; ++i) energy += inputBuffer[(size_t) i] * inputBuffer[(size_t) i];
				energy /= (float) kBufferSize;
				if (10.0f * std::log10 (energy) < silenceDb)
					detectedPitchInHz = 0.0f;
			}
			const float pitchConfidence = yin.confidence();

			if (detectedPitchInHz > 0.0f && pitchConfidence >= confidenceThreshold * 0.01f) {
				const float linearPitch = 12.0f * (std::log2 (detectedPitchInHz / 440.0f) + (float) octave - 5.0f) + 69.0f;
				cvPitch = std::max (-10.0f, std::min (10.0f, linearPitch * (1.0f / 12.0f)));
				lastKnownPitchInHz = detectedPitchInHz;
				cvSignal = 10.0f;
			}
			else {
				if (! holdOutputPitch)
					lastKnownPitchInHz = cvPitch = 0.0f;
				cvSignal = 0.0f;
			}

			lastKnownPitchConfidence = pitchConfidence;
			lastUsedOutputPitch = cvPitch;
			lastUsedOutputSignal = cvSignal;
		}

		cvPitchOut = smooth ? slewProcess (1.0f / sampleRate, cvPitch) : cvPitch;
		gateOut = cvSignal;
	}
};

//==============================================================================
// Befaco SlewLimiter — verbatim math (mono path of the SIMD original).
struct BefacoSlew {
	float shapeKnob = 0.0f, riseKnob = 0.0f, fallKnob = 0.0f;
	float out = 0.0f;

	static constexpr float slewMin = 0.1f;
	static constexpr float slewMax = 10000.0f;
	static constexpr float shapeScale = 1.0f / 10.0f;

	float process (float in, float dt) {
		const float riseCV = riseKnob * 10.0f;
		const float fallCV = fallKnob * 10.0f;
		float delta = in - out;
		float rateCV = 0.0f;
		if (delta > 0.0f) rateCV = riseCV;
		else if (delta < 0.0f) rateCV = fallCV;
		rateCV *= 0.1f;
		float pmOne = (delta > 0.0f) ? 1.0f : (delta < 0.0f ? -1.0f : 0.0f);
		float slew = slewMax * std::pow (slewMin / slewMax, rateCV);
		float shaped = pmOne + (shapeScale * delta - pmOne) * shapeKnob;   // crossfade
		out += slew * shaped * dt;
		if (delta > 0.0f && out > in) out = in;
		if (delta < 0.0f && out < in) out = in;
		return out;
	}
};

//==============================================================================
// Aria Quack — QQQQ engine, column processing (T&H every sample in this patch):
// v = in*scaling/100 + offset → quantize(clamp ±5V, nearest note) → ±1 V per octave step.
struct Quack {
	std::array<bool, 12> validNotes {};   // C..B
	float scaling = 100.0f;               // %
	float offset = 0.0f;                  // V
	int transposeOctaves = 0;             // TRANSPOSE_PARAM with mode 0

	static constexpr float FUDGEOFFSET = 0.001f;

	static float quantize (float voltage, const std::array<bool, 12>& validNotes) {
		voltage += FUDGEOFFSET;
		float octave = std::floor (voltage);
		float voltageOnFirstOctave = voltage - octave;
		float currentComparison, currentDistance;
		float closestNoteFound = 10.0f;
		float closestNoteDistance = 10.0f;
		for (int note = 0; note < 12; ++note) {
			if (validNotes[(size_t) note]) {
				currentComparison = note / 12.0f;
				currentDistance = std::fabs (voltageOnFirstOctave - currentComparison);
				if (currentDistance < closestNoteDistance) {
					closestNoteFound = currentComparison;
					closestNoteDistance = currentDistance;
				}
			}
		}
		for (int note = 0; note < 12; ++note) {
			if (validNotes[(size_t) note]) {
				currentComparison = note / 12.0f + 1.0f;
				currentDistance = std::fabs (voltageOnFirstOctave - currentComparison);
				if (currentDistance < closestNoteDistance) {
					closestNoteFound = currentComparison;
					closestNoteDistance = currentDistance;
				}
				break;
			}
		}
		if (closestNoteDistance < 10.0f)
			voltage = octave + closestNoteFound;
		return std::max (-10.0f, std::min (10.0f, voltage));
	}

	float process (float in) const {
		float v = in * scaling / 100.0f + offset;
		v = quantize (std::max (-5.0f, std::min (5.0f, v)), validNotes);
		int t = transposeOctaves;
		for (int j = 0; j < std::abs (t); ++j) {
			if (t > 0 && v <= 5.0f) v += 1.0f;
			if (t < 0 && v >= -5.0f) v -= 1.0f;
		}
		return v;
	}
};

} // namespace gemsv2

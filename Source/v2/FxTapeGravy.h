#pragma once

// GEMS v2 — two audio-FX module ports used by the Cardinal patch (chunk 4.7):
//   RackwindowsTape — Airwindows "Tape" saturator/warmer, via rackwindows' VCV port
//   SapphireGravy   — stereo state-variable filter (LP/BP/HP) w/ mix, gain, AGC
// rack:: / VCV Module plumbing replaced with plain floats; poly/CV paths not used by
// the patch are omitted. See NOTICE.md for origins/commits/licenses.

#include <cmath>
#include <cstdint>

namespace gemsv2 {

//==============================================================================
// RackwindowsTape — VCV wrapper `Tape` (rackwindows src/tape.cpp) around the
// Airwindows "Tape" DSP core `rwlib::Tape` (rackwindows src/rwlib.h, struct Tape).
// GPL-3.0, see NOTICE.md. Two independent per-channel engines (as tapeL[]/tapeR[]
// in the original), each fed the same slam/bump params.
// Patch: audio -> IN_L/IN_R, slamCv = band-env*0.166 V (SLAM_CV), bumpCv unpatched
// (BUMP_CV = 0 V). Params: SLAM (0) default 0.697369, BUMP (1) default 0.705264.
// GEMS: MAX_POLY_CHANNELS arrays dropped — GEMS engine is single-voice (channel 0
// of the original's poly arrays).
// GEMS: rackwindows' "quality" preference is a *global* VCV user setting (a
// Rackwindows.json file under asset::user(), not stored in the patch) that gates
// the denormal-guard + 32-bit dither path; plugin.cpp's loadQuality() defaults to
// Eco (dither OFF) when that file is absent. Per this port's brief, the dither is
// included verbatim (deterministic given the fpd=17 seed from Tape::onReset()), so
// quality is hardcoded HIGH here, matching what a "High" quality preference gives.
struct RackwindowsTape {
	static constexpr double gainCut = 0.1;
	static constexpr double gainBoost = 10.0;

	// params (knob 0..1, patch defaults)
	float slamParam = 0.697369f;
	float bumpParam = 0.705264f;

	// rwlib::Tape (rackwindows src/rwlib.h) — transcribed verbatim, one instance per
	// audio channel.
	struct Engine {
		double iirMidRollerA = 0.0;
		double iirMidRollerB = 0.0;
		double iirHeadBumpA = 0.0;
		double iirHeadBumpB = 0.0;

		long double biquadA[9] {};
		long double biquadB[9] {};
		long double biquadC[9] {};
		long double biquadD[9] {};

		bool flip = false;

		long double lastSample = 0.0;

		double inputgain = 0.0;
		double bumpgain = 0.0;
		double headBumpFreq = 0.0;
		double rollAmount = 0.0;
		double softness = 0.618033988749894848204586;

		float lastSlamParam = 0.0f;
		float lastBumpParam = 0.0f;

		void onSampleRateChange (double overallscale = 1.0) {
			headBumpFreq = 0.12 / overallscale;
			rollAmount = (1.0 - softness) / overallscale;

			biquadA[0] = biquadB[0] = 0.0072 / overallscale;
			biquadA[1] = biquadB[1] = 0.0009;
			double K = std::tan (M_PI * biquadB[0]);
			double norm = 1.0 / (1.0 + K / biquadB[1] + K * K);
			biquadA[2] = biquadB[2] = K / biquadB[1] * norm;
			biquadA[4] = biquadB[4] = -biquadB[2];
			biquadA[5] = biquadB[5] = 2.0 * (K * K - 1.0) * norm;
			biquadA[6] = biquadB[6] = (1.0 - K / biquadB[1] + K * K) * norm;

			biquadC[0] = biquadD[0] = 0.032 / overallscale;
			biquadC[1] = biquadD[1] = 0.0007;
			K = std::tan (M_PI * biquadD[0]);
			norm = 1.0 / (1.0 + K / biquadD[1] + K * K);
			biquadC[2] = biquadD[2] = K / biquadD[1] * norm;
			biquadC[4] = biquadD[4] = -biquadD[2];
			biquadC[5] = biquadD[5] = 2.0 * (K * K - 1.0) * norm;
			biquadC[6] = biquadD[6] = (1.0 - K / biquadD[1] + K * K) * norm;
		}

		void reset (double overallscale) {
			iirMidRollerA = iirMidRollerB = 0.0;
			iirHeadBumpA = iirHeadBumpB = 0.0;
			for (int i = 0; i < 9; ++i)
				biquadA[i] = biquadB[i] = biquadC[i] = biquadD[i] = 0.0;
			flip = false;
			lastSample = 0.0;
			inputgain = 0.0;
			bumpgain = 0.0;
			lastSlamParam = lastBumpParam = 0.0f;
			onSampleRateChange (overallscale);
		}

		// rwlib::Tape::process — verbatim (long double intermediate, as original).
		long double process (long double inputSample, float slamP, float bumpP, double overallscale) {
			(void) overallscale;

			if (slamP != lastSlamParam) {
				inputgain = std::pow (10.0, ((slamP - 0.5) * 24.0) / 20.0);
				lastSlamParam = slamP;
			}
			if (bumpP != lastBumpParam) {
				bumpgain = bumpP * 0.1;
				lastBumpParam = bumpP;
			}

			long double drySample = inputSample;

			long double highsSample = 0.0;
			long double nonHighsSample = 0.0;
			long double tempSample;

			if (flip) {
				iirMidRollerA = (iirMidRollerA * (1.0 - rollAmount)) + ((double) inputSample * rollAmount);
				highsSample = inputSample - iirMidRollerA;
				nonHighsSample = iirMidRollerA;

				iirHeadBumpA += ((double) inputSample * 0.05);
				iirHeadBumpA -= (iirHeadBumpA * iirHeadBumpA * iirHeadBumpA * headBumpFreq);
				iirHeadBumpA = std::sin (iirHeadBumpA);
				// interleaved biquad
				tempSample = (iirHeadBumpA * biquadA[2]) + biquadA[7];
				biquadA[7] = (iirHeadBumpA * biquadA[3]) - (tempSample * biquadA[5]) + biquadA[8];
				biquadA[8] = (iirHeadBumpA * biquadA[4]) - (tempSample * biquadA[6]);
				iirHeadBumpA = (double) tempSample;
				if (iirHeadBumpA > 1.0) iirHeadBumpA = 1.0;
				if (iirHeadBumpA < -1.0) iirHeadBumpA = -1.0;
				iirHeadBumpA = std::asin (iirHeadBumpA);

				inputSample = std::sin (inputSample);
				// interleaved biquad
				tempSample = (inputSample * biquadC[2]) + biquadC[7];
				biquadC[7] = (inputSample * biquadC[3]) - (tempSample * biquadC[5]) + biquadC[8];
				biquadC[8] = (inputSample * biquadC[4]) - (tempSample * biquadC[6]);
				inputSample = tempSample;
				if (inputSample > 1.0) inputSample = 1.0;
				if (inputSample < -1.0) inputSample = -1.0;
				inputSample = std::asin (inputSample);
			}
			else {
				iirMidRollerB = (iirMidRollerB * (1.0 - rollAmount)) + ((double) inputSample * rollAmount);
				highsSample = inputSample - iirMidRollerB;
				nonHighsSample = iirMidRollerB;

				iirHeadBumpB += ((double) inputSample * 0.05);
				iirHeadBumpB -= (iirHeadBumpB * iirHeadBumpB * iirHeadBumpB * headBumpFreq);
				iirHeadBumpB = std::sin (iirHeadBumpB);
				// interleaved biquad
				tempSample = (iirHeadBumpB * biquadB[2]) + biquadB[7];
				biquadB[7] = (iirHeadBumpB * biquadB[3]) - (tempSample * biquadB[5]) + biquadB[8];
				biquadB[8] = (iirHeadBumpB * biquadB[4]) - (tempSample * biquadB[6]);
				iirHeadBumpB = (double) tempSample;
				if (iirHeadBumpB > 1.0) iirHeadBumpB = 1.0;
				if (iirHeadBumpB < -1.0) iirHeadBumpB = -1.0;
				iirHeadBumpB = std::asin (iirHeadBumpB);

				inputSample = std::sin (inputSample);
				// interleaved biquad
				tempSample = (inputSample * biquadD[2]) + biquadD[7];
				biquadD[7] = (inputSample * biquadD[3]) - (tempSample * biquadD[5]) + biquadD[8];
				biquadD[8] = (inputSample * biquadD[4]) - (tempSample * biquadD[6]);
				inputSample = tempSample;
				if (inputSample > 1.0) inputSample = 1.0;
				if (inputSample < -1.0) inputSample = -1.0;
				inputSample = std::asin (inputSample);
			}
			flip = !flip;

			// set up UnBox
			long double groundSampleL = drySample - inputSample;

			// gain boost inside UnBox: do not boost fringe audio
			if (inputgain != 1.0) {
				inputSample *= inputgain;
			}

			// apply Soften depending on polarity
			long double applySoften = std::fabs ((double) highsSample) * 1.57079633;
			if (applySoften > 1.57079633)
				applySoften = 1.57079633;
			applySoften = 1 - std::cos ((double) applySoften);
			if (highsSample > 0) inputSample -= applySoften;
			if (highsSample < 0) inputSample += applySoften;

			// clip to 1.2533141373155 to reach maximum output
			if (inputSample > 1.2533141373155)
				inputSample = 1.2533141373155;
			if (inputSample < -1.2533141373155)
				inputSample = -1.2533141373155;

			// Spiral, for cleanest most optimal tape effect
			inputSample = std::sin (inputSample * std::fabs ((double) inputSample)) / ((inputSample == 0.0) ? 1.0 : std::fabs ((double) inputSample));

			// restrain resonant quality of head bump algorithm
			double suppress = (1.0 - std::fabs ((double) inputSample)) * 0.00013;
			if (iirHeadBumpA > suppress) iirHeadBumpA -= suppress;
			if (iirHeadBumpA < -suppress) iirHeadBumpA += suppress;
			if (iirHeadBumpB > suppress) iirHeadBumpB -= suppress;
			if (iirHeadBumpB < -suppress) iirHeadBumpB += suppress;

			// apply UnBox processing
			inputSample += groundSampleL;

			// apply head bump
			inputSample += ((iirHeadBumpA + iirHeadBumpB) * bumpgain);

			// ADClip
			if (lastSample >= 0.99) {
				if (inputSample < 0.99)
					lastSample = ((0.99 * softness) + ((double) inputSample * (1.0 - softness)));
				else
					lastSample = 0.99;
			}
			if (lastSample <= -0.99) {
				if (inputSample > -0.99)
					lastSample = ((-0.99 * softness) + ((double) inputSample * (1.0 - softness)));
				else
					lastSample = -0.99;
			}
			if (inputSample > 0.99) {
				if (lastSample < 0.99)
					inputSample = ((0.99 * softness) + (lastSample * (1.0 - softness)));
				else
					inputSample = 0.99;
			}
			if (inputSample < -0.99) {
				if (lastSample > -0.99)
					inputSample = ((-0.99 * softness) + (lastSample * (1.0 - softness)));
				else
					inputSample = -0.99;
			}
			lastSample = inputSample;

			// final iron bar
			if (inputSample > 0.99) inputSample = 0.99;
			if (inputSample < -0.99) inputSample = -0.99;

			return inputSample;
		}
	};

	Engine chL, chR;
	uint32_t fpdL = 17, fpdR = 17;   // Tape::onReset() seed
	double overallscale = 1.0;

	static inline float clampf_ (float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

	// Tape::onSampleRateChange: overallscale = sampleRate / 44100.
	void setSampleRate (float sampleRate) {
		overallscale = 1.0;
		overallscale /= 44100.0;
		overallscale *= sampleRate;
		chL.onSampleRateChange (overallscale);
		chR.onSampleRateChange (overallscale);
	}

	void reset() {
		chL.reset (overallscale);
		chR.reset (overallscale);
		fpdL = fpdR = 17;
	}

	// slamCv/bumpCv: volts (module does param += cv/10). inL/inR: audio, +-5 V.
	void process (float inL, float inR, float slamCv, float bumpCv, float& outL, float& outR) {
		float slam = clampf_ (slamParam + slamCv / 10.0f, 0.01f, 0.99f);
		float bump = clampf_ (bumpParam + bumpCv / 10.0f, 0.01f, 0.99f);

		// left
		{
			long double s = (long double) inL * gainCut;
			if (std::fabs ((double) s) < 1.18e-37)
				s = (long double) fpdL * 1.18e-37L;

			s = chL.process (s, slam, bump, overallscale);

			// 32-bit stereo floating point dither (quality == HIGH, hardcoded — see header comment)
			int expon;
			std::frexp ((float) s, &expon);
			fpdL ^= fpdL << 13;
			fpdL ^= fpdL >> 17;
			fpdL ^= fpdL << 5;
			s += ((double) fpdL - (double) (uint32_t) 0x7fffffff) * 5.5e-36L * std::pow (2.0, expon + 62);

			s *= gainBoost;
			outL = (float) s;
		}

		// right
		{
			long double s = (long double) inR * gainCut;
			if (std::fabs ((double) s) < 1.18e-37)
				s = (long double) fpdR * 1.18e-37L;

			s = chR.process (s, slam, bump, overallscale);

			int expon;
			std::frexp ((float) s, &expon);
			fpdR ^= fpdR << 13;
			fpdR ^= fpdR >> 17;
			fpdR ^= fpdR << 5;
			s += ((double) fpdR - (double) (uint32_t) 0x7fffffff) * 5.5e-36L * std::pow (2.0, expon + 62);

			s *= gainBoost;
			outR = (float) s;
		}
	}
};

//==============================================================================
// SapphireGravy — stereo state-variable filter (LP/BP/HP select) w/ wet/dry mix and
// output gain. Ported from Sapphire (cosinekitty/sapphire @ d884e795, GPL-3.0, see
// NOTICE.md): engine `Sapphire::Gravy::GravyEngine` (src/gravy_engine.hpp) driving
// `Sapphire::StateVariableFilter<value_t>` (src/sapphire_engine.hpp — Cytomic
// trapezoidal-integrator SVF, https://cytomic.com/files/dsp/SvfLinearTrapOptimised2.pdf),
// with the knob/CV -> engine-param mapping from `GravyModule::process` (src/gravy_vcv.cpp).
// Patch instance: FREQ knob 1.86747 (-> cornerFreqHz = 523.2511*2^1.86747 ~= 1908 Hz),
// RES 0.224096, MIX 0.436145, GAIN 0.5 (unity), FILTER_MODE 0 (Lowpass); no CV patched
// (attenuverters 0), so knob values pass straight through unmodified.
// GEMS: source's StateVariableFilter<PhysicsVector> processes 4 channels per instance
// via SIMD lanes that are fully independent (elementwise +,-,*), with filter
// coefficients (a1/a2/a3/k, computed from scalar freq/resonance) broadcast equally to
// all 4 lanes. Two scalar-float SVF instances (filterL/filterR) are therefore bit-
// equivalent to using 2 of the 4 lanes of the original.
// GEMS: AGC (Sapphire::AutomaticGainLimiter, applied in GravyModule::process only
// `if (enableAgc)`) is omitted. gravy_vcv.cpp's AgcLevelQuantity::isAgcEnabled() is
// `value < disableMin`, and the ctor call
// `addAgcLevelQuantity(AGC_LEVEL_PARAM, /*levelMin*/5, /*levelDef*/50.5, /*levelMax*/50,
// /*disableMin*/50.5, /*disableMax*/51.0)` sets the *default* param value equal to
// disableMin. The patch stores agcLevel=50.5 (the default) -> isAgcEnabled() is false
// (50.5 < 50.5 is false), so the AGC/limiter is inactive for this instance; omitting it
// is exact for the patch's stored value, not just an approximation.
// GEMS: the FILTER_MODE crossfade (EnumSmoother, gravy_vcv.cpp) is a ~10 ms click-
// suppression ramp for *live* mode changes; the patch never changes mode at runtime, so
// `mode` is applied directly without the smoother's fade-through-silence transient.
struct SapphireGravy {
	static constexpr double DefaultFrequencyHz = 523.2511306011972;   // C5 = 440*2^0.25
	static constexpr float OctaveRange = 5.0f;

	enum FilterMode { LOWPASS = 0, BANDPASS = 1, HIGHPASS = 2 };

	// params
	float freqKnob = 1.86747f;    // octaves relative to DefaultFrequencyHz, clamp +-OctaveRange
	float resKnob = 0.224096f;    // 0..1
	float mixKnob = 0.436145f;    // 0..1, cubic wet/dry taper
	float gainKnob = 0.5f;        // 0..1, cubic taper (0.5 == unity)
	int mode = LOWPASS;

	// Sapphire::StateVariableFilter<value_t> (sapphire_engine.hpp) — scalar per channel.
	struct SVF {
		float c1 = 0.0f, c2 = 0.0f, bandpass = 0.0f, lowpass = 0.0f, v3 = 0.0f;
		float a1 = 0.0f, a2 = 0.0f, a3 = 0.0f;
		float prevFreqRatio = 0.0f, prevResonance = 0.0f, k = 0.0f;

		void initialize() { c1 = c2 = bandpass = lowpass = v3 = 0.0f; }

		void process (float sampleRateHz, float cornerFreqHz, float resonance, float input,
		              float& lp, float& bp, float& hp, float& nx) {
			const float ratio = cornerFreqHz / sampleRateHz;
			if (ratio != prevFreqRatio || resonance != prevResonance) {
				prevFreqRatio = ratio;
				prevResonance = resonance;

				float g = std::tan ((float) M_PI * ratio);
				const float cushion = 0.002f;   // values of `k` too close to zero cause excessive ringing and aliasing
				k = cushion + ((2.0f - cushion) * cube (1.0f - resonance));
				a1 = 1.0f / (1.0f + g * (g + k));
				a2 = g * a1;
				a3 = g * a2;
			}

			v3 = input - c2;
			bandpass = a1 * c1 + a2 * v3;
			lowpass = c2 + a2 * c1 + a3 * v3;
			c1 = 2.0f * bandpass - c1;
			c2 = 2.0f * lowpass - c2;

			const float notch = input - k * bandpass;
			lp = lowpass;
			bp = bandpass;
			hp = notch - lowpass;
			nx = notch;
		}
	};

	SVF filterL, filterR;
	float sampleRate = 48000.0f;

	static inline float cube (float x) { return x * x * x; }
	static inline float clampf_ (float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

	void setSampleRate (float sr) { sampleRate = sr; }

	void reset() { filterL.initialize(); filterR.initialize(); }

	void process (float inL, float inR, float& outL, float& outR) {
		float freq = clampf_ (freqKnob, -OctaveRange, OctaveRange);
		float cornerFreqHz = std::exp2 (freq) * (float) DefaultFrequencyHz;
		float gain = cube (clampf_ (gainKnob, 0.0f, 1.0f) * 2.0f);
		float res = clampf_ (resKnob, 0.0f, 1.0f);
		float mix = 1.0f - cube (1.0f - clampf_ (mixKnob, 0.0f, 1.0f));

		float lpL, bpL, hpL, nxL, lpR, bpR, hpR, nxR;
		filterL.process (sampleRate, cornerFreqHz, res, inL, lpL, bpL, hpL, nxL);
		filterR.process (sampleRate, cornerFreqHz, res, inR, lpR, bpR, hpR, nxR);

		float yL = (mode == LOWPASS) ? lpL : (mode == BANDPASS) ? bpL : (mode == HIGHPASS) ? hpL : 0.0f;
		float yR = (mode == LOWPASS) ? lpR : (mode == BANDPASS) ? bpR : (mode == HIGHPASS) ? hpR : 0.0f;

		outL = gain * (mix * yL + (1.0f - mix) * inL);
		outR = gain * (mix * yR + (1.0f - mix) * inR);
	}
};

} // namespace gemsv2

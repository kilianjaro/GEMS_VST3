#pragma once

// GEMS v2 — SurgeXT Delay (chunk 4.7, chain 2). Faithful port of the sst-effects Dual
// Delay (surge-synthesizer/sst-effects include/sst/effects/Delay.h, GPL-3), as driven by
// surge-rack's Delay module. Topology transcribed 1:1:
//   tbuffer = delayline_read(timeL/R)         (GEMS: cubic frac interp replaces the sinc-FIR
//                                               — transparent for a ~1 s musical delay)
//   tbuffer = hardclip(±tbuffer)              (clip mode 3 = dly_clipping_hard; FBsign=false)
//   tbuffer = LP2B(highcut) then HP(lowcut)   (Butterworth biquads, Q=0.707, on the wet/fb path)
//   wb = input(center pan) + feedback·tbuffer(same ch) + crossfeed·tbuffer(other ch)
//   out = dry·(1-mix) + tbuffer·mix           (mix.fade)
// amp_to_linear(x) = x^3 (exact). cutoffHz = 440·2^(param/12). delaySec = 2^timeParam.
// Patch #3453: TIME_L 0.1025→1.074 s, TIME_R 0.0769→1.055 s, FEEDBACK 0.374, CROSSFEED 0.315,
// LOCUT -11.696→224 Hz, HICUT 43.926→5570 Hz, MIX 0.4176, MODDEPTH 0 (LFO off). Band env →
// mod input 0 modulates FEEDBACK (+0.3) and MIX (+0.044) in param space.
// See NOTICE.md.

#include <cmath>
#include <vector>

namespace gemsv2 {

struct SurgeDelay {
	// base params (surge-rack f-values)
	float timeLParam = 0.102507f;   // log2 seconds
	float timeRParam = 0.0768786f;
	float feedbackParam = 0.374002f;
	float crossfeedParam = 0.315002f;
	float locutParam = -11.6956f;   // -> 440*2^(v/12) Hz HP
	float hicutParam = 43.9257f;    // -> LP
	float mixParam = 0.417572f;

	float sampleRate = 48000.0f;
	int   bufLen = 1 << 18;         // power of two, >= 2 s @ 96k + guard
	std::vector<float> bufL, bufR;
	int   wpos = 0;

	// smoothed run values
	float timeL = 0, timeR = 0, fb = 0, cf = 0, mix = 0;
	float hicutHz = 5570.0f, locutHz = 224.0f;

	// feedback-path biquads (LP then HP), per channel; RBJ Butterworth Q=1/sqrt2 = LP2B
	struct Biquad {
		float b0=1,b1=0,b2=0,a1=0,a2=0, z1=0,z2=0;
		inline float process (float x) { float y=b0*x+z1; z1=b1*x-a1*y+z2; z2=b2*x-a2*y; return y; }
		void reset() { z1=z2=0; }
		void setLP (float f, float sr) { set (f, sr, true); }
		void setHP (float f, float sr) { set (f, sr, false); }
		void set (float f, float sr, bool lp) {
			f = f < 10.0f ? 10.0f : (f > sr*0.49f ? sr*0.49f : f);
			float w = 2.0f*(float)M_PI*f/sr, c=std::cos(w), s=std::sin(w), al=s/(2.0f*0.70710678f);
			float a0=1+al;
			if (lp) { b0=(1-c)*0.5f/a0; b1=(1-c)/a0; b2=b0; }
			else    { b0=(1+c)*0.5f/a0; b1=-(1+c)/a0; b2=b0; }
			a1=-2*c/a0; a2=(1-al)/a0;
		}
	} lpL, lpR, hpL, hpR;

	static inline float ampToLinear (float x) { x = x < 0 ? 0 : x; return x*x*x; }
	static inline float hardclip (float x) { return x < -1.0f ? -1.0f : (x > 1.0f ? 1.0f : x); }

	void setSampleRate (float sr) {
		sampleRate = sr;
		// size buffer to >= max delay (10 s cap) + guard, power of two
		int need = (int) (sr * 3.0f) + 8;
		bufLen = 1; while (bufLen < need) bufLen <<= 1;
		bufL.assign ((size_t) bufLen, 0.0f);
		bufR.assign ((size_t) bufLen, 0.0f);
		reset();
		updateCoeffs (0.0f, 0.0f);
		timeL = target_timeL(); timeR = target_timeR(); fb = ampToLinear(feedbackParam);
		cf = ampToLinear(crossfeedParam); mix = mixParam;
	}

	void reset() {
		std::fill (bufL.begin(), bufL.end(), 0.0f);
		std::fill (bufR.begin(), bufR.end(), 0.0f);
		wpos = 0;
		lpL.reset(); lpR.reset(); hpL.reset(); hpR.reset();
	}

	float target_timeL() const { return sampleRate * std::exp2 (timeLParam); }
	float target_timeR() const { return sampleRate * std::exp2 (timeRParam); }

	// fbMod / mixMod: parameter-space modulation (added to feedback & mix params) from
	// mod input 0 (band env). Depths (0.3, 0.044) are applied by the engine.
	void updateCoeffs (float feedbackMod, float mixMod) {
		hicutHz = 440.0f * std::exp2 (hicutParam / 12.0f);
		locutHz = 440.0f * std::exp2 (locutParam / 12.0f);
		lpL.setLP (hicutHz, sampleRate); lpR.setLP (hicutHz, sampleRate);
		hpL.setHP (locutHz, sampleRate); hpR.setHP (locutHz, sampleRate);
		// smoothed targets
		float tL = target_timeL(), tR = target_timeR();
		float fbT = ampToLinear (feedbackParam + feedbackMod);
		float cfT = ampToLinear (crossfeedParam);
		float mixT = mixParam + mixMod;
		const float sm = 0.002f;   // one-pole smoothing (~block-rate)
		timeL += (tL - timeL) * sm; timeR += (tR - timeR) * sm;
		fb += (fbT - fb) * sm; cf += (cfT - cf) * sm; mix += (mixT - mix) * sm;
	}

	inline float readCubic (const std::vector<float>& buf, float delaySamples) {
		float rp = (float) wpos - delaySamples;
		while (rp < 0) rp += bufLen;
		int i1 = (int) rp; float t = rp - i1;
		int i0 = (i1 - 1) & (bufLen - 1), i2 = (i1 + 1) & (bufLen - 1), i3 = (i1 + 2) & (bufLen - 1);
		i1 &= (bufLen - 1);
		float y0=buf[(size_t)i0],y1=buf[(size_t)i1],y2=buf[(size_t)i2],y3=buf[(size_t)i3];
		float a=-0.5f*y0+1.5f*y1-1.5f*y2+0.5f*y3, b=y0-2.5f*y1+2.0f*y2-0.5f*y3, c=-0.5f*y0+0.5f*y2;
		return ((a*t+b)*t+c)*t+y1;
	}

	// inL: mono/L audio (patch feeds VCAmp#4142 → in[0]); feedbackMod/mixMod: param-space
	// modulation from band env (engine applies depths). Outputs stereo wet (mix-blended).
	void process (float inL, float inR, float feedbackMod, float mixMod, float& outL, float& outR) {
		updateCoeffs (feedbackMod, mixMod);

		float tL = readCubic (bufL, timeL);
		float tR = readCubic (bufR, timeR);
		tL = hardclip (tL); tR = hardclip (tR);
		tL = hpL.process (lpL.process (tL));
		tR = hpR.process (lpR.process (tR));

		float wbL = inL + fb * tL + cf * tR;
		float wbR = inR + fb * tR + cf * tL;
		bufL[(size_t) wpos] = wbL;
		bufR[(size_t) wpos] = wbR;
		wpos = (wpos + 1) & (bufLen - 1);

		outL = inL * (1.0f - mix) + tL * mix;
		outR = inR * (1.0f - mix) + tR * mix;
	}
};

} // namespace gemsv2

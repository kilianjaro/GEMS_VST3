#pragma once

// GEMS v2 — Sapphire Echo (+ EchoOut expander), chunk 4.7 chain 7. Faithful-topology port
// of cosinekitty/sapphire src/echo_vcv.cpp (@ d884e795, GPL-3): a tape-loop delay with
// filtered feedback and DC rejection, followed by the EchoOut global mix/level.
// Patch Echo#9193: TIME 0 → 2^0 = 1.0 s, FEEDBACK 0.565, DC_REJECT 20 Hz, ENV_GAIN 1.
//   ⚠ REVERSE_BUTTON = 1: in the Sapphire UI this is a momentary/toggle that needs a
//   control action to flip playback direction; in a static loaded patch it does not put a
//   running echo into reverse by itself. GEMS implements the forward tape echo (the audible
//   default) and flags this — a later bit-exact pass can add reverse-tape playback if A/B
//   shows a difference.
// EchoOut#2414: GLOBAL_MIX 0.829, GLOBAL_LEVEL 1.585 (+4 dB). See NOTICE.md.

#include <cmath>
#include <vector>

namespace gemsv2 {

struct SapphireEcho {
	float timeKnob = 0.0f;        // delay seconds = 2^timeKnob
	float feedbackKnob = 0.565057f;
	float dcRejectHz = 20.0f;
	float globalMix = 0.828915f;
	float globalLevel = 1.58489f;

	float sampleRate = 48000.0f;
	std::vector<float> bufL, bufR;
	int bufLen = 1 << 18, wpos = 0;
	// DC-block (one-pole HP) state per channel, on the feedback path
	float dcx1L = 0, dcy1L = 0, dcx1R = 0, dcy1R = 0, dcR = 0.0f;

	void setSampleRate (float sr) {
		sampleRate = sr;
		int need = (int) (sr * 3.0f) + 8;
		bufLen = 1; while (bufLen < need) bufLen <<= 1;
		bufL.assign ((size_t) bufLen, 0.0f);
		bufR.assign ((size_t) bufLen, 0.0f);
		dcR = std::exp (-2.0f * (float) M_PI * dcRejectHz / sr);   // one-pole HP coeff
		reset();
	}
	void reset() {
		std::fill (bufL.begin(), bufL.end(), 0.0f);
		std::fill (bufR.begin(), bufR.end(), 0.0f);
		wpos = 0; dcx1L = dcy1L = dcx1R = dcy1R = 0.0f;
	}

	inline float readCubic (const std::vector<float>& buf, float d) {
		float rp = (float) wpos - d; while (rp < 0) rp += bufLen;
		int i1 = (int) rp; float t = rp - i1;
		int i0 = (i1 - 1) & (bufLen - 1), i2 = (i1 + 1) & (bufLen - 1), i3 = (i1 + 2) & (bufLen - 1);
		i1 &= (bufLen - 1);
		float y0=buf[(size_t)i0],y1=buf[(size_t)i1],y2=buf[(size_t)i2],y3=buf[(size_t)i3];
		float a=-0.5f*y0+1.5f*y1-1.5f*y2+0.5f*y3, b=y0-2.5f*y1+2.0f*y2-0.5f*y3, c=-0.5f*y0+0.5f*y2;
		return ((a*t+b)*t+c)*t+y1;
	}
	inline float dcBlock (float x, float& x1, float& y1) {
		float y = x - x1 + dcR * y1; x1 = x; y1 = y; return y;
	}

	void process (float inL, float inR, float& outL, float& outR) {
		float d = std::exp2 (timeKnob) * sampleRate;
		if (d > bufLen - 4) d = (float) (bufLen - 4);
		float dl = readCubic (bufL, d), dr = readCubic (bufR, d);
		float fbL = dcBlock (dl, dcx1L, dcy1L), fbR = dcBlock (dr, dcx1R, dcy1R);
		bufL[(size_t) wpos] = inL + feedbackKnob * fbL;
		bufR[(size_t) wpos] = inR + feedbackKnob * fbR;
		wpos = (wpos + 1) & (bufLen - 1);
		// EchoOut: global dry/wet mix, then output level
		outL = (inL * (1.0f - globalMix) + dl * globalMix) * globalLevel;
		outR = (inR * (1.0f - globalMix) + dr * globalMix) * globalLevel;
	}
};

} // namespace gemsv2

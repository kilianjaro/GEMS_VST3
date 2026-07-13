#pragma once

// GEMS v2 — generative/modulation modules (chunk 4.9). Ported from:
//   BogaudioModules LFO.cpp/Walk2.cpp (656eaae4, GPL-3), HetrickCV Dust.cpp (GPL-3),
//   AudibleInstruments Branches.cpp (Bernoulli gate, GPL-3).
// Stochastic modules take a seed and use a deterministic PRNG (distribution-match per the
// generative-RNG policy, plan.md §3 — not sample-exact to Rack's RNG). Rack conventions:
// bipolar ±5 V, gates 0/10 V, triggers rising >1 V. See NOTICE.md.

#include <cmath>
#include <cstdint>

namespace gemsv2 {

// deterministic uniform [0,1)
struct Rng {
	uint32_t s = 0x1234567u;
	void seed (uint32_t v) { s = v ? v : 0x1234567u; }
	inline float next01() { s = s * 1664525u + 1013904223u; return (float) (s >> 8) / 16777216.0f; }
	inline float nextBi() { return next01() * 2.0f - 1.0f; }
};

//==============================================================================
// Bogaudio LFO — frequency = 2^freqKnob Hz (freqKnob −5..8). Outputs sine + triangle
// (the two the patch uses), ±5 V nominal, scaled by scaleKnob (0..1) and offset by
// offsetKnob (−1..1 → ±5 V), matching LFO.cpp's `offset + scale·wave` output.
struct BogaudioLFO {
	float freqKnob = 0.0f, pwKnob = 0.0f, offsetKnob = 0.0f, scaleKnob = 1.0f;
	bool slow = false;
	float phase = 0.0f, sampleTime = 1.0f / 48000.0f;
	float sine = 0.0f, triangle = 0.0f;

	void setSampleRate (float sr) { sampleTime = 1.0f / sr; }
	void reset() { phase = 0.0f; sine = triangle = 0.0f; }

	void process() {
		float freq = std::exp2 (freqKnob);
		if (slow) freq /= 100.0f;
		phase += freq * sampleTime;
		phase -= std::floor (phase);
		float wsin = std::sin (2.0f * (float) M_PI * phase);
		// triangle: 0→+1 at .25, →-1 at .75 (Bogaudio TriangleOscillator convention)
		float t = phase;
		float wtri = (t < 0.25f) ? (4.0f * t)
		           : (t < 0.75f) ? (2.0f - 4.0f * t)
		                         : (4.0f * t - 4.0f);
		float off = offsetKnob * 5.0f, sc = scaleKnob;
		sine = off + sc * 5.0f * wsin;
		triangle = off + sc * 5.0f * wtri;
	}
};

//==============================================================================
// Bogaudio Walk2 — 2D smoothed random walk. rate knobs 0..1 set the change speed; a
// slew-limited random target produces a bounded, smooth ±5 V·scale output on X and Y.
struct BogaudioWalk2 {
	float rateXKnob = 0.1f, rateYKnob = 0.1f, scaleXKnob = 1.0f, scaleYKnob = 1.0f;
	float x = 0.0f, y = 0.0f, distance = 0.0f;
	float sampleTime = 1.0f / 48000.0f;
	Rng rng;
	float tx = 0.0f, ty = 0.0f;   // walk targets

	void setSampleRate (float sr) { sampleTime = 1.0f / sr; }
	void reset (uint32_t seed) { rng.seed (seed); x = y = tx = ty = 0.0f; distance = 0.0f; }

	void process() {
		// rate → target-refresh probability + slew; higher rate = faster walk.
		float rX = rateXKnob * rateXKnob, rY = rateYKnob * rateYKnob;
		// random-walk increment (bounded); scaled to ±5 V
		tx += rng.nextBi() * rX * 0.5f;
		ty += rng.nextBi() * rY * 0.5f;
		tx = std::max (-1.0f, std::min (1.0f, tx));
		ty = std::max (-1.0f, std::min (1.0f, ty));
		// gentle slew toward target for smoothness
		const float sl = 0.01f;
		float nx = x / 5.0f, ny = y / 5.0f;
		nx += (tx - nx) * sl; ny += (ty - ny) * sl;
		x = nx * 5.0f * scaleXKnob;
		y = ny * 5.0f * scaleYKnob;
		distance = std::sqrt (x * x + y * y);
	}
};

//==============================================================================
// HetrickCV Dust — random impulses. density = (clamp(rate+cv,0,4)/4)^3 events/sample·sr;
// threshold = (clamp/4)^3; on rand<threshold emit a normalized impulse. Unipolar here
// (patch param BIPOLAR=1 → "Unipolar"): amplitude = clamp(noise·(1/threshold)·10, 0, 10).
struct HetrickDust {
	float rateKnob = 2.56867f;
	bool unipolar = true;
	float sampleRate = 48000.0f;
	Rng rng;

	void setSampleRate (float sr) { sampleRate = sr; }
	void reset (uint32_t seed) { rng.seed (seed); }

	float process (float rateCv) {
		float d = rateKnob + rateCv;
		d = std::max (0.0f, std::min (4.0f, d)) / 4.0f;
		float threshold = d * d * d;   // = density/sr
		float nv = rng.next01();
		if (nv < threshold) {
			float scale = threshold > 0.0f ? (1.0f / threshold) : 0.0f;
			if (unipolar) return std::max (0.0f, std::min (10.0f, nv * scale * 10.0f));
			return std::max (-5.0f, std::min (5.0f, (nv * scale * 2.0f - 1.0f) * 5.0f));
		}
		return 0.0f;
	}
};

//==============================================================================
// AudibleInstruments Branches — one Bernoulli gate channel. On a rising gate (Schmitt
// 0.1/1.0 V), a coin flip vs `prob` routes the gate to output A or B. In latch mode the
// chosen output TOGGLES and holds (10/0 V) instead of following the gate.
struct AudibleBranches {
	float prob = 0.5f;
	bool latchMode = false;
	Rng rng;
	bool gateHigh = false;   // schmitt state
	bool toAValid = false, lastToA = false;
	bool latchA = false, latchB = false;

	void reset (uint32_t seed) { rng.seed (seed); gateHigh = false; toAValid = false; latchA = latchB = false; }

	void process (float gateIn, float& outA, float& outB) {
		bool rising = false;
		if (gateHigh) { if (gateIn <= 0.1f) gateHigh = false; }
		else if (gateIn >= 1.0f) { gateHigh = true; rising = true; }

		if (rising) {
			bool toA = rng.next01() < prob;   // coin flip
			lastToA = toA; toAValid = true;
			if (latchMode) { if (toA) latchA = !latchA; else latchB = !latchB; }
		}

		if (latchMode) {
			outA = latchA ? 10.0f : 0.0f;
			outB = latchB ? 10.0f : 0.0f;
		}
		else {
			float g = gateHigh ? gateIn : 0.0f;
			outA = (toAValid && lastToA) ? g : 0.0f;
			outB = (toAValid && !lastToA) ? g : 0.0f;
		}
	}
};

} // namespace gemsv2

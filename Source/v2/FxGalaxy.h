#pragma once

// GEMS v2 — Sapphire Galaxy reverb (chunk 4.7, chain 5, before Gravy). Wraps the vendored
// Sapphire Galaxy engine (Airwindows Galactic adaptation) verbatim — `sapphire/galaxy_engine.hpp`
// (cosinekitty/sapphire @ d884e795, GPL-3). CV applied as Sapphire's cvGetControlValue does:
//   value = clamp(baseKnob + attenu·(cv/5), 0, 1).
// Patch Galaxy#2449: replace 0.127, brightness 0.752 (atten 0.917 ← env CV in3),
// detune 0.042, bigness 0.429 (atten 0.272 ← env CV in5), mix 0.420. See NOTICE.md.

#include "sapphire/galaxy_engine.hpp"

namespace gemsv2 {

struct SapphireGalaxy {
	Sapphire::Galaxy::Engine engine;
	float sampleRate = 48000.0f;
	// base knobs
	float replaceKnob = 0.126506f;
	float brightnessKnob = 0.751807f, brightnessAtten = 0.917334f;
	float detuneKnob = 0.042169f;
	float bignessKnob = 0.428916f, bignessAtten = 0.272f;
	float mixKnob = 0.420482f;

	void setSampleRate (float sr) { sampleRate = sr; }
	void reset() { engine.initialize(); }

	static inline float clampf01 (float v) { return v < 0 ? 0 : (v > 1 ? 1 : v); }

	// brightnessCv / bignessCv in volts (env-derived); others static in this patch.
	void process (float inL, float inR, float brightnessCv, float bignessCv, float& outL, float& outR) {
		engine.setReplace (replaceKnob);
		engine.setBrightness (clampf01 (brightnessKnob + brightnessAtten * (brightnessCv / 5.0f)));
		engine.setDetune (detuneKnob);
		engine.setBigness (clampf01 (bignessKnob + bignessAtten * (bignessCv / 5.0f)));
		engine.setMix (mixKnob);
		engine.process (sampleRate, inL, inR, outL, outR);
	}
};

} // namespace gemsv2

#pragma once

// GEMS v2 — Sapphire Elastika (chunk 4.8, chain 3): hexagonal mass-spring mesh physical
// model. Wraps the vendored Sapphire Elastika engine + mesh VERBATIM (cosinekitty/sapphire
// @ d884e795, GPL-3): `sapphire/elastika_engine.hpp` + `elastika_mesh.*` + `mesh_physics.hpp`
// + `sapphire_engine.hpp` + `sapphire_simd.hpp` (arm64 __m128 scalar shim). CV applied as
// Sapphire's cvGetControlValue does: value = clamp(baseSlider + attenu·(cv/5)·(max−min), min, max).
// Patch Elastika#9717: friction 0.774, stiffness 0.588, span 0.196 (atten 0.0853 ← Branches CV in2),
// curl 0.432 (atten 0.128 ← Branches CV in3), mass −1 (atten −0.608 ← env CV in4), drive/level 1,
// tilts 0.5, AGC level 4, DC-reject 20 Hz, mix 1. Audio in ← LVCF (in5). See NOTICE.md.

#include "sapphire/elastika_engine.hpp"

namespace gemsv2 {

struct SapphireElastika {
	Sapphire::ElastikaEngine engine;
	float sampleRate = 48000.0f;

	// base sliders (patch values)
	float frictionKnob = 0.774001f, stiffnessKnob = 0.587999f;
	float spanKnob = 0.196001f, spanAtten = 0.0853332f;
	float curlKnob = 0.432f, curlAtten = 0.128f;
	float massKnob = -1.0f, massAtten = -0.608001f;
	float driveKnob = 1.0f, levelKnob = 1.0f;
	float inTiltKnob = 0.5f, outTiltKnob = 0.5f;

	void setSampleRate (float sr) {
		sampleRate = sr;
		engine.initialize();
		engine.setDcRejectFrequency (20.0f);
		engine.setAgcLevel (4.0f);
		engine.setAgcEnabled (true);
		engine.setFriction (frictionKnob);
		engine.setStiffness (stiffnessKnob);
		engine.setDrive (driveKnob);
		engine.setGain (levelKnob);
		engine.setInputTilt (inTiltKnob);
		engine.setOutputTilt (outTiltKnob);
		engine.setMix (1.0f);
	}
	void reset() { engine.initialize(); }

	static inline float clampf_ (float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

	// audio in (mono from LVCF, fed to both L/R); spanCv/curlCv (Branches gate), massCv (env), in volts.
	void process (float inL, float inR, float spanCv, float curlCv, float massCv, float& outL, float& outR) {
		engine.setSpan (clampf_ (spanKnob + spanAtten * (spanCv / 5.0f), 0.0f, 1.0f));
		engine.setCurl (clampf_ (curlKnob + curlAtten * (curlCv / 5.0f) * 2.0f, -1.0f, 1.0f));
		engine.setMass (clampf_ (massKnob + massAtten * (massCv / 5.0f) * 2.0f, -1.0f, 1.0f));
		engine.process (sampleRate, inL, inR, outL, outR);
	}
};

} // namespace gemsv2

#pragma once

// GEMS v2 — one band-analysis chain of the Cardinal patch (chunk 4.1 scope):
//   VCF [→ VCF2] → Follow → Offset → Cmp ─┐
//                                          AND → DADSRH  (env CV 0..10 V)
//   pitch gate (AudioToCVPitch, chunk 4.2) ┘
// The chain exposes taps used elsewhere in the patch: band audio (pitch tracking,
// AnalyzerXL), env CV (voice/FX modulation), trigger gate, envelope, and applies
// the VCA law for the voice level (audio itself is added in chunks 4.3+).

#include "BogModules.h"
#include "PitchSpine.h"
#include "PatchConstants.h"

namespace gemsv2 {

struct AnalysisChain {
	// modules
	VCF bandVcf, bandVcf2;
	bool hasSecondVcf = false;
	Follow follow;
	Offset offset;
	Cmp cmp;
	bool hasPitchGate = true;
	ANDGate gateAnd;
	DADSRH env;
	VCAmp vca;

	// pitch spine (absent on the Braids chain)
	AudioToCVPitch pitchTracker;
	BefacoSlew pitchSlew;
	Quack quack;

	// per-sample taps (updated by process())
	float bandAudio = 0.0f;   // post band-VCF audio (feeds AudioToCVPitch + voice paths)
	float rawEnv = 0.0f;      // Follow output *before* Offset (Galaxy bigness tap, chain 5)
	float envCv = 0.0f;       // Offset output (comparator + modulation CV taps)
	float cmpGate = 0.0f;     // Cmp GREATER output 0/10 V
	float trigGate = 0.0f;    // post-AND (or Cmp direct for Braids chain) 0/10 V
	float envOut = 0.0f;      // DADSRH envelope 0..10 V
	float pitchCv = 0.0f;     // AudioToCVPitch CV out (post smooth), V
	float pitchGate = 0.0f;   // AudioToCVPitch gate out 0/10 V
	float quantizedCv = 0.0f; // post slew + Quack → voice V/oct

	void configure (const patch::ChainConst& c, const patch::PitchConst* p) {
		bandVcf.frequencyKnob = c.vcf.freq;  bandVcf.qKnob = c.vcf.q;
		bandVcf.modeParam = c.vcf.mode;      bandVcf.slopeKnob = c.vcf.slope;
		hasSecondVcf = c.hasSecondVcf;
		if (hasSecondVcf) {
			bandVcf2.frequencyKnob = c.vcf2.freq;  bandVcf2.qKnob = c.vcf2.q;
			bandVcf2.modeParam = c.vcf2.mode;      bandVcf2.slopeKnob = c.vcf2.slope;
		}
		follow.responseKnob = c.follow.response;  follow.gainKnob = c.follow.gain;
		offset.offsetKnob = c.offset.offset;      offset.scaleKnob = c.offset.scale;
		cmp.aKnob = c.cmp.a;  cmp.bKnob = c.cmp.b;  cmp.windowKnob = c.cmp.window;  cmp.lagKnob = c.cmp.lag;
		hasPitchGate = c.hasPitchGate;
		env.delayKnob = c.env.del;    env.attackKnob = c.env.atk;  env.decayKnob = c.env.dec;
		env.sustainKnob = c.env.sus;  env.releaseKnob = c.env.rel; env.holdKnob = c.env.hold;
		env.attackShape = c.env.shapeA;  env.decayShape = c.env.shapeD;  env.releaseShape = c.env.shapeR;
		vca.levelKnob = c.vcaLevel;

		if (p != nullptr) {
			pitchTracker.sensitivity = p->sensitivity;
			pitchTracker.confidenceThreshold = p->confidence;
			pitchTracker.tolerance = 0.0f;   // all patch instances
			pitchSlew.shapeKnob = p->slewShape;
			pitchSlew.riseKnob = p->slewRise;
			pitchSlew.fallKnob = p->slewFall;
			for (int i = 0; i < 12; ++i) quack.validNotes[(size_t) i] = p->notes[i];
			quack.transposeOctaves = p->transposeOct;
		}
	}

	void setSampleRate (float sr) {
		bandVcf.setSampleRate (sr);
		if (hasSecondVcf) bandVcf2.setSampleRate (sr);
		follow.setSampleRate (sr);
		cmp.setSampleRate (sr);
		env.setSampleRate (sr);
		vca.setSampleRate (sr);
		_sampleTime = 1.0f / sr;
		if (hasPitchGate) pitchTracker.prepare (sr);
	}

	void reset() {
		bandVcf.reset();
		if (hasSecondVcf) bandVcf2.reset();
		cmp.reset();
		env.reset();
		pitchSlew.out = 0.0f;
	}

	// control-rate updates (call every ~2.5 ms, Bogaudio cadence)
	void modulate() {
		bandVcf.modulate();
		if (hasSecondVcf) bandVcf2.modulate();
		follow.modulate();
		cmp.modulate();
	}

	// in: analysis feed sample (left channel of host input, volts ±5-ish)
	void process (float in) {
		bandAudio = bandVcf.process (in);
		if (hasSecondVcf) bandAudio = bandVcf2.process (bandAudio);

		rawEnv = follow.process (bandAudio);
		envCv = offset.process (rawEnv);
		cmpGate = cmp.process (envCv);

		if (hasPitchGate) {
			pitchTracker.processSample (bandAudio);
			pitchCv = pitchTracker.cvPitchOut;
			pitchGate = pitchTracker.gateOut;
			quantizedCv = quack.process (pitchSlew.process (pitchCv, _sampleTime));
			trigGate = gateAnd.process (pitchGate, cmpGate);
		}
		else {
			trigGate = cmpGate;
		}
		envOut = env.step (trigGate);
	}

	// voice audio through the chain's VCA (env CV drives level; audio from chunks 4.3+)
	inline float applyVca (float voiceIn) { return vca.process (voiceIn, envOut); }

private:
	float _sampleTime = 1.0f / 48000.0f;
};

} // namespace gemsv2

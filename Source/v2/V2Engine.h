#pragma once

// GEMS v2 engine.
// Chunk 4.1: analysis front-end + trigger spine (7 chains). Voices/FX follow in 4.3+.
// Pure C++ (no JUCE deps) so the same code compiles in the offline test harness.

#include "AnalysisChain.h"
#include "SimpleVoices.h"
#include "BogVoices.h"
#include "OurosVoice.h"
#include "MutableVoices.h"
#include "SurgeWTVoice.h"
#include "FxSurgeDelay.h"
#include "FxTapeGravy.h"
#include "FxGalaxy.h"
#include "FxEcho.h"
#include "FxElastika.h"
#include "NeilVoice.h"
#include "GenerativeModules.h"
#include "MasterBus.h"

namespace gemsv2 {

class V2Engine {
public:
	static constexpr int kNumChains = 7;
	// chain indices (ascending band frequency)
	enum { kWT = 0, kOuros = 1, kOsculum = 2, kPalm = 3, kFMOp = 4, kPlaits = 5, kBraids = 6 };

	void prepare (double sampleRate) {
		_sampleRate = (float) (sampleRate > 0.0 ? sampleRate : 48000.0);
		for (int i = 0; i < kNumChains; ++i) {
			// chains 1..6 have a pitch spine; chain 7 (Braids) does not
			_chains[i].configure (patch::chains[i], i < 6 ? &patch::pitch[i] : nullptr);
			_chains[i].setSampleRate (_sampleRate);
			_chains[i].reset();
		}
		_modClock.setSampleRate (_sampleRate);

		// voices (chunks 4.3/4.6)
		_ouros.setSampleRate (_sampleRate);  _ouros.reset();
		_osculum.setSampleRate (_sampleRate); _osculum.reset();
		_lvcf.setSampleRate (_sampleRate);
		_palm.setSampleRate (_sampleRate);   _palm.reset();
		_palmVcf.frequencyKnob = 0.316379f;  _palmVcf.qKnob = 0.301206f;
		_palmVcf.modeParam = 0;              _palmVcf.slopeKnob = 0.751149f;   // VCF#7322
		_palmVcf.setSampleRate (_sampleRate);
		_neil.setSampleRate (_sampleRate);   _neil.reset();
		_fmop.setSampleRate (_sampleRate);   _fmop.reset();
		_plaits.setSampleRate (_sampleRate); _plaits.reset();
		_braids.setSampleRate (_sampleRate); _braids.reset();
		_braidsTrigWasHigh = false;
		_wt.setSampleRate (_sampleRate); _wt.reset();
		_hissState = 0x2f6e2b1u;
		_ourosDelay.setSampleRate (_sampleRate); _ourosDelay.reset();
		_galaxy.setSampleRate (_sampleRate); _galaxy.reset();
		_elastika.setSampleRate (_sampleRate); _elastika.reset();
		_branchElastika1.prob = 0.595181f; _branchElastika1.reset (0x77a1u);
		_branchElastika2.prob = 0.597591f; _branchElastika2.reset (0x33c9u);
		_osculumEnv2.delayKnob = patch::osculumEnv2.del; _osculumEnv2.attackKnob = patch::osculumEnv2.atk;
		_osculumEnv2.decayKnob = patch::osculumEnv2.dec; _osculumEnv2.sustainKnob = patch::osculumEnv2.sus;
		_osculumEnv2.releaseKnob = patch::osculumEnv2.rel; _osculumEnv2.holdKnob = patch::osculumEnv2.hold;
		_osculumEnv2.attackShape = patch::osculumEnv2.shapeA; _osculumEnv2.decayShape = patch::osculumEnv2.shapeD;
		_osculumEnv2.releaseShape = patch::osculumEnv2.shapeR; _osculumEnv2.setSampleRate (_sampleRate); _osculumEnv2.reset();
		_osculumVca2.levelKnob = patch::osculumVca2Level; _osculumVca2.setSampleRate (_sampleRate);
		_gravy.setSampleRate (_sampleRate); _gravy.reset();
		_tape.setSampleRate (_sampleRate); _tape.reset();
		_plaitsVcf.frequencyKnob = 0.353553f; _plaitsVcf.qKnob = 0.492771f;   // VCF#6226
		_plaitsVcf.modeParam = 0; _plaitsVcf.slopeKnob = 0.522233f;
		_plaitsVcf.setSampleRate (_sampleRate);
		_echo.setSampleRate (_sampleRate); _echo.reset();
		_lfo1.freqKnob = -4.30723f; _lfo1.scaleKnob = 0.901204f; _lfo1.setSampleRate (_sampleRate); _lfo1.reset();
		_lfo2.freqKnob = -4.5735f; _lfo2.pwKnob = -0.289156f; _lfo2.offsetKnob = 0.139759f; _lfo2.scaleKnob = 0.761444f;
		_lfo2.setSampleRate (_sampleRate); _lfo2.reset();
		_walk2.rateXKnob = 0.328916f; _walk2.rateYKnob = 0.307229f; _walk2.scaleXKnob = 0.837348f; _walk2.scaleYKnob = 0.783132f;
		_walk2.setSampleRate (_sampleRate); _walk2.reset (0x51a3b7u);
		_dust.rateKnob = 2.56867f; _dust.unipolar = true; _dust.setSampleRate (_sampleRate); _dust.reset (0x9e12f4u);
		_branchesColor.prob = 0.20482f; _branchesColor.latchMode = true; _branchesColor.reset (0x2bd6a1u);
		_master.setSampleRate (_sampleRate); _master.reset();

		for (int i = 0; i < kNumChains; ++i) _voiceOut[i] = 0.0f;

		// prime control-rate state
		modulateAll();
	}

	// Analysis feed = LEFT channel of host input (patch finding: SplitAddMerge poly
	// {L,R,0}, every consumer reads ch 0).
	void processSample (float inLeft) {
		if (_modClock.tick())
			modulateAll();

		for (int i = 0; i < kNumChains; ++i)
			_chains[i].process (inLeft);

		// ── generative/mod sources (chunk 4.9) ──
		_lfo1.process();   // 0.05 Hz sine → Mix8 ch4 autopan (applied at mixer, 4.10)
		_lfo2.process();   // 0.042 Hz → Dust rate
		_walk2.process();  // → Plaits morph/timbre
		float dustV = _dust.process (_lfo2.sine);              // LFO#9420 sine → Dust rate CV
		float bcA, bcB; _branchesColor.process (dustV, bcA, bcB);   // Dust → Branches#2090 (latch)
		_braids.colorCv = bcA * 0.195181f;                    // → Braids COLOR_INPUT (DualAtenuverter#4205 ch2)
		_plaits.morphCv  = _walk2.x * 0.362651f;              // DualAtenuverter#5785 out0 → MORPH
		_plaits.timbreCv = _walk2.y * 0.406024f;              // DualAtenuverter#5785 out1 → TIMBRE

		// ── voices (pre/post-FX) ──
		// Chain 1 — SurgeXT Wavetable + Hiss → Mix4#0778 → VCAmp#5564.
		// saturate/formant modulated by band env via DualAtenuverter#6293 → mod inputs,
		// scaled per surge-rack ModulationAssistant (base01 + depth·cv/10). Mod matrix:
		// saturate ← in4(dep 0.57)+in5(dep 0.004); formant ← in5(dep 0.838).
		{
			auto& c = _chains[kWT];
			float modCv = clampf (c.envCv * 0.8f - 1.56626f, -10.0f, 10.0f);   // DualAtenuverter#6293 out0
			float sat01 = 0.107f + (0.57f + 0.004f) * (modCv / 10.0f);
			float formant01 = 0.837998f * (modCv / 10.0f);
			float wtv = _wt.process (c.quantizedCv, sat01, formant01);
			float hiss = nextHiss();
			// Mix4#0778: WT (ch1) 0 dB, Hiss (ch2) -42 dB, master 0 dB (center pan → mono sum)
			float mixed = wtv * 1.0f + hiss * kHissMinus42dB;
			_voiceOut[kWT] = c.applyVca (mixed);   // VCAmp#5564
		}
		// Chain 2 — Ouros → VCAmp#4142 → SurgeXT Delay#3453 → (Splirge sum) → Mix8 ch8.
		// env CV → Ouros NODE/SPREAD (DualAtenuverter#8944); band env (Offset#6469 = envCv)
		// → Delay mod input 0: FEEDBACK depth 0.3, MIX depth 0.044 (÷10 CV scaling).
		{
			auto& c = _chains[kOuros];
			_ouros.process (c.quantizedCv, _ourosNodeAtt.process (c.envCv), _ourosSpreadAtt.process (c.envCv));
			float vca = c.applyVca (_ouros.outL);   // VCAmp#4142
			float dl, dr;
			_ourosDelay.process (vca, 0.0f, 0.3f * (c.envCv / 10.0f), 0.044f * (c.envCv / 10.0f), dl, dr);
			_voiceOut[kOuros] = dl + dr;   // VU → Splirge 2-ch merge → Mix8 ch8 (poly sum)
		}
		// Chain 3 — Osculum → VCAmp#8870 → LVCF#9943 → { Mix4#8233 in1, Elastika#9717 in5 };
		// two-channel Branches#1623: ch1(p.595, gate=env) → Elastika span/curl excitation;
		// ch2(p.598, gate=trig) → DADSRH#9786 → VCAmp#2571 → Osculum wave2 → Mix4 in3.
		// Mix4 → VCAmp#8974 (−15 dB) → VU → Mix8 ch7.
		{
			auto& c = _chains[kOsculum];
			_osculum.process (c.quantizedCv);
			float bA1, bB1; _branchElastika1.process (c.envOut, bA1, bB1);   // Branches#1623 ch1
			float bA2, bB2; _branchElastika2.process (c.trigGate, bA2, bB2); // Branches#1623 ch2
			float lvcfOut = _lvcf.process (c.applyVca (_osculum.wave1));     // VCAmp#8870 → LVCF#9943
			float massCv = _elastikaMassAtt.process (c.envCv);              // DualAtenuverter#2475
			float el, er; _elastika.process (lvcfOut, lvcfOut, bA1, bA1, massCv, el, er);
			float env2 = _osculumEnv2.step (bA2);                           // DADSRH#9786
			float w2 = _osculumVca2.process (_osculum.wave2, env2);         // VCAmp#2571
			// Mix4#8233: LVCF 0 dB + Elastika −12 dB + wave2 −12 dB, master 0 dB; VCAmp#8974 −15 dB
			float mixed = lvcfOut * 1.0f + el * 0.251189f + w2 * 0.251189f;
			_voiceOut[kOsculum] = mixed * 0.177828f;
		}
		// Chain 4 — PalmLoop SUB → VCF#7322 (LP 2 kHz) → VCAmp#2011 → Neil (Clouds granular)
		// → VU → Lmtr#9115 → Mix8 ch5/6. Neil size ← env (DualAtenuverter#2449 out0),
		// texture ← env (out1).
		{
			auto& c = _chains[kPalm];
			_palm.process (c.quantizedCv);
			float pv = c.applyVca (_palmVcf.process (_palm.subOut));   // VCAmp#2011
			float sizeCv = _neilSizeAtt.process (c.envCv);
			float texCv = _neilTexAtt.process (c.envCv);
			_neil.process (pv, pv, sizeCv, texCv);
			_neilL = _neil.outL; _neilR = _neil.outR;     // Neil L/R → Mix8 ch5+ch6 separately
			_voiceOut[kPalm] = _neilL + _neilR;           // mono tap (tests)
		}
		// Chain 5 — FMOp → VCAmp#0229 → Sapphire Galaxy#2449 → Gravy#5933 (LP 1908) → Lmtr → Mix8 ch2.
		// Galaxy brightness ← env CV (DualAtenuverter#6183 out0), bigness ← rawEnv (out1).
		{
			auto& c = _chains[kFMOp];
			float v = c.applyVca (_fmop.process());   // VCAmp#0229
			float brightCv = _galaxyBrightAtt.process (c.envCv);
			float bigCv = _galaxyBigAtt.process (c.rawEnv);
			float gxl, gxr; _galaxy.process (v, v, brightCv, bigCv, gxl, gxr);   // Sapphire Galaxy
			float gl, gr; _gravy.process (gxl, gxr, gl, gr);                     // Sapphire Gravy
			_voiceOut[kFMOp] = gl;
		}
		// Chain 6 — Plaits (VA) → VCAmp#5569 → VCF#6226 (LP ~2.5 kHz; generative cutoff
		// sweep DEFERRED to 4.9) → rackwindows Tape (slam+env, bump) → Lmtr → Mix8 ch1.
		{
			auto& c = _chains[kPlaits];
			_plaits.process (c.quantizedCv);            // morph/timbre CV set above (Walk2)
			float v = c.applyVca (_plaits.out);         // VCAmp#5569
			v = _plaitsVcf.process (v);                 // VCF#6226
			float tl, tr; _tape.process (v, v, c.envCv * 0.166f * 0.1f, 0.0f, tl, tr);   // Tape slam ← env
			_voiceOut[kPlaits] = tl;
		}
		// Chain 7 — Braids (PARTICLE_NOISE) → VCAmp#5951 → Sapphire Echo (+EchoOut) → Mix8 ch4.
		// Pitch env-derived (DualAtenuverter#4205 ch1); color from Dust/Branches (set above);
		// trigger = chain gate rising edge.
		{
			auto& c = _chains[kBraids];
			float braidsPitch = _braidsPitchAtt.process (c.envCv);
			bool trigHigh = c.trigGate > 1.0f;
			bool rise = trigHigh && !_braidsTrigWasHigh;
			_braidsTrigWasHigh = trigHigh;
			_braids.process (braidsPitch, rise);
			float v = c.applyVca (_braids.out);         // VCAmp#5951
			float el, er; _echo.process (v, 0.0f, el, er);   // Sapphire Echo + EchoOut
			_voiceOut[kBraids] = el + er;
		}

		// ── master bus (chunk 4.10): Mix8 → Pressor → Plateau → host out ──
		// Mix8 channels (patch §4): 1 Plaits, 2 FMOp, 3 WT, 4 Braids(LFO autopan),
		// 5 Neil L, 6 Neil R, 7 Osculum, 8 Ouros+Delay.
		float mixCh[8] = {
			_voiceOut[kPlaits], _voiceOut[kFMOp], _voiceOut[kWT], _voiceOut[kBraids],
			_neilL, _neilR, _voiceOut[kOsculum], _voiceOut[kOuros]
		};
		_master.process (mixCh, _lfo1.sine, _masterL, _masterR);   // ch4 autopan = LFO#0793
	}

	AnalysisChain& chain (int i) { return _chains[i]; }
	const AnalysisChain& chain (int i) const { return _chains[i]; }
	float voiceOut (int i) const { return _voiceOut[i]; }
	// stereo master output (volts). Dry field-recording path is muted in the patch (HostAudio
	// level 0), so the output is the wet synth mix only.
	float masterL() const { return _masterL; }
	float masterR() const { return _masterR; }
	float sampleRate() const { return _sampleRate; }

private:
	void modulateAll() {
		for (int i = 0; i < kNumChains; ++i)
			_chains[i].modulate();
		_palmVcf.modulate();
		_lvcf.modulate();
		_plaitsVcf.modulate();   // VCF#6226 (chain 6)
		_fmop.modulate (_chains[kFMOp].quantizedCv);
	}

	float _sampleRate = 48000.0f;
	AnalysisChain _chains[kNumChains];
	ModulationClock _modClock;

	// chain voices + their dedicated processors
	OurosVoice _ouros;
	SurgeDelay _ourosDelay;                                // SurgeXTDelay#3453
	BefacoAtten _ourosNodeAtt { 0.460241f, -1.25301f };    // DualAtenuverter#8944 ch1
	BefacoAtten _ourosSpreadAtt { 0.450603f, 4.86747f };   // DualAtenuverter#8944 ch2
	OsculumVoice _osculum;
	LVCFPort _lvcf;                                        // LVCF#9943
	SapphireElastika _elastika;                           // Elastika#9717
	AudibleBranches _branchElastika1, _branchElastika2;   // Branches#1623 ch1/ch2
	BefacoAtten _elastikaMassAtt { 0.889157f, 1.85542f }; // DualAtenuverter#2475 (mass CV)
	DADSRH _osculumEnv2;                                   // DADSRH#9786
	VCAmp _osculumVca2;                                    // VCAmp#2571
	PalmLoopVoice _palm;
	VCF _palmVcf;                                          // VCF#7322
	NeilVoice _neil;                                       // Neil#1829 (Clouds granular)
	BefacoAtten _neilSizeAtt { 0.915663f, 0.79518f };     // DualAtenuverter#2449 out0
	BefacoAtten _neilTexAtt { 0.710844f, 3.75903f };      // DualAtenuverter#2449 out1
	float _neilL = 0.0f, _neilR = 0.0f;
	FMOpVoice _fmop;
	PlaitsVoice _plaits;
	BraidsVoice _braids;
	BefacoAtten _braidsPitchAtt { 0.320482f, 4.65061f };   // DualAtenuverter#4205 ch1
	bool _braidsTrigWasHigh = false;
	SurgeWTVoice _wt;

	// per-voice FX (chunk 4.7) + generative (4.9)
	SapphireGalaxy _galaxy;       // Galaxy#2449 (chain 5, before Gravy)
	BefacoAtten _galaxyBrightAtt { 0.691566f, -0.554217f };   // DualAtenuverter#6183 out0
	BefacoAtten _galaxyBigAtt { 0.306024f, -1.10843f };       // DualAtenuverter#6183 out1
	SapphireGravy _gravy;         // Gravy#5933 (chain 5)
	RackwindowsTape _tape;        // tape#8798 (chain 6)
	VCF _plaitsVcf;               // VCF#6226 (chain 6; generative cutoff sweep deferred)
	SapphireEcho _echo;           // Echo#9193 + EchoOut#2414 (chain 7)
	BogaudioLFO _lfo1, _lfo2;     // LFO#0793 (autopan), LFO#9420 (Dust rate)
	BogaudioWalk2 _walk2;         // Walk2#5894 (Plaits morph/timbre)
	HetrickDust _dust;            // Dust#0219 (Braids color)
	AudibleBranches _branchesColor;   // Branches#2090 (latch → Braids color)

	// Sapphire Hiss (chain-1 noise layer, −42 dB under the wavetable). GEMS: Gaussian
	// white noise (approx-Gaussian via central-limit of 4 uniforms); distribution-match
	// per the generative-RNG policy (plan.md §3). Sapphire Hiss ≈ unit-ish Gaussian volts.
	static constexpr float kHissMinus42dB = 0.0079433f;   // 10^(-42/20)
	uint32_t _hissState = 0x2f6e2b1u;
	inline float nextHiss() {
		float acc = 0.0f;
		for (int i = 0; i < 4; ++i) {
			_hissState = _hissState * 1664525u + 1013904223u;
			acc += ((float) (_hissState >> 8) / 8388608.0f) - 1.0f;   // uniform ±1
		}
		return acc;   // ~N(0, 4/3), a few volts pk — matched to Sapphire Hiss scale
	}

	float _voiceOut[kNumChains] {};

	MasterBus _master;
	float _masterL = 0.0f, _masterR = 0.0f;
};

} // namespace gemsv2

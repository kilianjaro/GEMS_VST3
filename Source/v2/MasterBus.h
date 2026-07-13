#pragma once

// GEMS v2 — master bus (chunk 4.10): Mix8 → Pressor (comp) → Plateau (plate reverb) → out.
// Mix8 (Bogaudio): per-channel level (dB = knob·66−60) via Amplifier + constant-power Panner,
//   summed → master level. Pressor (Bogaudio): RMS detector, attack/release slew, Compressor
//   curve, in/out gain, saturator. Plateau (Valley): vendored Dattorro plate reverb with the
//   patch's (effectively static) params. All ported/vendored verbatim — see NOTICE.md.
// Mixer / compressor / reverb parameter values are configured in PatchConstants.h.

#include "bogaudio/signal.hpp"
#include "valley/Dattorro.hpp"

namespace gemsv2 {

using namespace bogaudio::dsp;

struct MasterBus {
	// ── Mix8 ── 8 channels: {levelKnob, panKnob}; ch4 pan gets the LFO#0793 autopan CV.
	static constexpr int kCh = 8;
	float levelKnob[kCh] = { 0.545455f, 0.848485f, 0.772727f, 0.590909f, 0.69697f, 0.727273f, 0.80303f, 0.772727f };
	float panKnob[kCh]   = { -0.73253f, 0.879518f, 0.13494f, 0.0f, -1.0f, 1.0f, -0.99759f, 1.0f };
	float masterKnob = 0.909091f;

	Amplifier chAmp[kCh], masterAmp;
	Panner chPan[kCh];

	// ── Pressor#4047 ──
	float pressThreshDb = -18.0f;       // 0.2·30−24
	float pressRatio = 9.0f;            // 1/tan(π/4·(1−0.903715^1.5))
	float pressInLevel = 3.98107f;      // +12 dB (INPUT_GAIN 1)
	float pressOutLevel = 1.0f;         // 0 dB
	RootMeanSquare pressRMS;
	SlewLimiter pressAtkSL, pressRelSL;
	Compressor pressComp;
	Amplifier pressAmp;
	Saturator pressSatL, pressSatR;
	float pressLastEnv = 0.0f;

	// ── Plateau#0640 (Dattorro) ──
	Dattorro reverb;
	float revDry = 1.0f;        // DRY 1
	float revWet = 3.36363f;    // WET 0.336363 · 10
	static constexpr float kMinus20dB = 0.1f;

	float sampleRate = 48000.0f;

	void setSampleRate (float sr) {
		sampleRate = sr;
		float minDb = Amplifier::minDecibels, maxDb = 6.0f;
		for (int i = 0; i < kCh; ++i) {
			chAmp[i].setLevel (levelKnob[i] * (maxDb - minDb) + minDb);
			chPan[i].setPan (panKnob[i]);
		}
		masterAmp.setLevel (masterKnob * (maxDb - minDb) + minDb);

		pressRMS.setSampleRate (sr);
		pressAtkSL.setParams (sr, 10.0f);    // 0.141421²·500 ms
		pressRelSL.setParams (sr, 200.0f);   // 0.31623²·2000 ms
		pressAmp.setLevel (0.0f);
		pressLastEnv = 0.0f;

		// Plateau reverb — decoded static params (patch §4; the one env-CV mod saturates its clamp).
		reverb.setSampleRate (sr);
		reverb.setTimeScale (1.52197f);              // SIZE 0.6156: rescale(size², 0..1, 0.01..4)
		reverb.setPreDelay (0.0324675f);             // 32.5 ms
		reverb.setInputFilterLowCutoffPitch (0.0f);  // 10 − clamp(10+5) = 0
		reverb.setInputFilterHighCutoffPitch (10.0f);// clamp(9.299+5) = 10
		reverb.setDecay (0.820544f);                 // DECAY 0.4763 → 1−(1−0.5763)²
		reverb.setTankDiffusion (9.4935f);           // DIFFUSION
		reverb.setTankFilterLowCutFrequency (0.0f);  // REVERB_LOW_DAMP 5.195: 10−clamp(5.195+5+envmod)=0
		reverb.setTankFilterHighCutFrequency (10.0f);// REVERB_HIGH_DAMP 10: clamp(10+5)=10
		reverb.setTankModSpeed (1.0f);               // MOD_SPEED 0 → 0²·99+1
		reverb.setTankModDepth (5.05065f);           // MOD_DEPTH
		reverb.setTankModShape (0.5f);               // MOD_SHAPE
	}

	void reset() {
		pressLastEnv = 0.0f;
		reverb.clear();
	}

	// ch[8]: the 8 Mix8 channel samples (volts). ch4PanCv: LFO#0793 sine (volts) for autopan.
	void process (const float ch[kCh], float ch4PanCv, float& outL, float& outR) {
		// Mix8: level + constant-power pan, summed
		float mixL = 0.0f, mixR = 0.0f;
		for (int i = 0; i < kCh; ++i) {
			if (i == 3) chPan[3].setPan (std::max (-1.0f, std::min (1.0f, panKnob[3] + ch4PanCv / 5.0f)));
			float s = chAmp[i].next (ch[i]);
			float l, r; chPan[i].next (s, l, r);
			mixL += l; mixR += r;
		}
		mixL = masterAmp.next (mixL);
		mixR = masterAmp.next (mixR);

		// Pressor (stereo-linked comp)
		float env = (mixL + mixR) * pressInLevel;
		env = pressRMS.next (env);
		env = (env > pressLastEnv) ? pressAtkSL.next (env, pressLastEnv) : pressRelSL.next (env, pressLastEnv);
		pressLastEnv = env;
		float detectorDb = amplitudeToDecibels (env / 5.0f);
		float compDb = pressComp.compressionDb (detectorDb, pressThreshDb, pressRatio, true /*soft knee*/);
		pressAmp.setLevel (-compDb);
		float pL = pressSatL.next (pressAmp.next (mixL * pressInLevel) * pressOutLevel);
		float pR = pressSatR.next (pressAmp.next (mixR * pressInLevel) * pressOutLevel);

		// Plateau reverb: dry + wet
		reverb.process (pL * kMinus20dB, pR * kMinus20dB);
		float rl = (float) reverb.getLeftOutput(), rr = (float) reverb.getRightOutput();
		outL = std::max (-10.0f, std::min (10.0f, pL * revDry + rl * revWet));
		outR = std::max (-10.0f, std::min (10.0f, pR * revDry + rr * revWet));
	}
};

} // namespace gemsv2

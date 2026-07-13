#pragma once

// GEMS v2 — module-level ports of Bogaudio modules used by the Cardinal patch.
// Logic is transcribed 1:1 from BogaudioModules src/*.cpp (commit 656eaae4, GPL-3.0,
// see NOTICE.md); rack:: Param/Input/Output plumbing replaced with plain floats.
// Rack conventions kept: voltages in V (audio ±5 V, CV 0..10 V), params in knob units.

#include <cmath>
#include <algorithm>

#include "bogaudio/multimode.hpp"
#include "bogaudio/signal.hpp"
#include "bogaudio/utility.hpp"
#include "bogaudio/pitch.hpp"

namespace gemsv2 {

using namespace bogaudio::dsp;

#define GEMS_HAVE_CLAMPF
inline float clampf (float v, float lo, float hi) { return std::min (hi, std::max (lo, v)); }

// rack_overrides.hpp: Schmitt trigger, high >= 1.0 V, low <= 0.1 V.
struct Trigger {
	float _high, _low;
	bool _state = false;
	Trigger (float high = 1.0f, float low = 0.1f) : _high (high), _low (low) {}
	void reset() { _state = false; }
	bool isHigh() const { return _state; }
	bool process (float in) {
		if (_state) { if (in <= _low) _state = false; }
		else if (in >= _high) { _state = true; return true; }
		return false;
	}
};

// module.cpp: Bogaudio modules recompute control-rate state every ~2.5 ms.
// (`++_steps; if (_steps >= _modulationSteps) { _steps = 0; modulate(); }`)
struct ModulationClock {
	int _interval = 120, _steps = 120;
	void setSampleRate (float sr) { _interval = std::max (1, (int) (sr * (2.5f / 1000.f))); _steps = _interval; }
	bool tick() { ++_steps; if (_steps >= _interval) { _steps = 0; return true; } return false; }
};

//==============================================================================
// VCF (VCF.cpp / VCF.hpp) — multimode Butterworth, 1..12 poles w/ slope crossfade,
// pitched bandwidth, semitone-domain frequency slew, final 2-pole 80 Hz HP.
struct VCF {
	static constexpr int maxPoles = 12;
	static constexpr int nFilters = maxPoles;
	static constexpr float maxFrequency = 20000.0f;
	static constexpr float minFrequency = MultimodeFilter::minFrequency;

	// params (knob units, patch constants)
	float frequencyKnob = 0.22361f;   // f = knob^2 * 20000
	float fmKnob = 0.0f;              // FM depth (unused unless fm input wired)
	float qKnob = 0.0f;               // resonance/bandwidth 0..1
	int   modeParam = 0;              // 0 LP, 1 HP, 2 BP, 3 BR
	float slopeKnob = 0.522233f;      // poles = slope^2 * 11 + 1

	// optional CV inputs (Rack volts); NaN-free defaults = unconnected
	bool  pitchInConnected = false;   // 1V/oct additive: f += cvToFrequency(clamp(pitch,-5,5))
	float pitchIn = 0.0f;

	MultimodeFilter16 _filters[nFilters];
	float _gains[nFilters] {};
	SlewLimiter _gainSLs[nFilters];
	SlewLimiter _frequencySL;
	MultimodeFilter4 _finalHP;
	float _sampleRate = 48000.0f;
	MultimodeFilter::Mode _mode = MultimodeFilter::BANDPASS_MODE;

	void setSampleRate (float sr) {
		_sampleRate = sr;
		_frequencySL.setParams (sr, 0.5f, frequencyToSemitone (maxFrequency - minFrequency));
		_finalHP.setParams (sr, MultimodeFilter::BUTTERWORTH_TYPE, 2, MultimodeFilter::HIGHPASS_MODE, 80.0f,
		                    MultimodeFilter::minQbw, MultimodeFilter::LINEAR_BANDWIDTH_MODE, MultimodeFilter::MINIMUM_DELAY_MODE);
		for (int i = 0; i < nFilters; ++i) _gainSLs[i].setParams (sr, 50.0f, 1.0f);
		reset();
	}

	void reset() {
		for (int i = 0; i < nFilters; ++i) _filters[i].reset();
		_finalHP.reset();
	}

	// VCF::modulate + modulateChannel + Engine::setParams — call at modulation cadence.
	void modulate() {
		_mode = (MultimodeFilter::Mode) (1 + clampf ((float) modeParam, 0.0f, 4.0f));

		float slope = clampf (slopeKnob, 0.0f, 1.0f);
		slope *= slope;

		float q = clampf (qKnob, 0.0f, 1.0f);

		float f = clampf (frequencyKnob, 0.0f, 1.0f);
		f *= f;
		f *= maxFrequency;
		if (pitchInConnected) {
			float pitch = clampf (pitchIn, -5.0f, 5.0f);
			f += cvToFrequency (pitch);
		}
		f = clampf (f, minFrequency, maxFrequency);

		// Engine::setParams
		float frequency = clampf (semitoneToFrequency (_frequencySL.next (frequencyToSemitone (f))), minFrequency, maxFrequency);

		int i = -1, j = -1;
		std::fill (_gains, _gains + nFilters, 0.0f);
		if (slope >= 1.0f) {
			_gains[i = nFilters - 1] = 1.0f;
		}
		else {
			slope *= nFilters - 1;
			float r = std::fmod (slope, 1.0f);
			_gains[i = (int) slope] = 1.0f - r;
			_gains[j = i + 1] = r;
		}

		_filters[i].setParams (_sampleRate, MultimodeFilter::BUTTERWORTH_TYPE, i + 1, _mode, frequency, q,
		                       MultimodeFilter::PITCH_BANDWIDTH_MODE);
		if (j >= 0) {
			_filters[j].setParams (_sampleRate, MultimodeFilter::BUTTERWORTH_TYPE, j + 1, _mode, frequency, q,
			                       MultimodeFilter::PITCH_BANDWIDTH_MODE);
		}
	}

	// Engine::next
	inline float process (float sample) {
		float out = 0.0f;
		for (int i = 0; i < nFilters; ++i) {
			float g = _gainSLs[i].next (_gains[i]);
			if (g > 0.0f)
				out += g * _filters[i].next (sample);
		}
		return _finalHP.next (out);
	}
};

//==============================================================================
// Follow (Follow.cpp + follower_base.cpp) — Puckette envelope follower + dB gain + saturator.
struct Follow {
	float responseKnob = 0.3f;  // 0..1 smoothing
	float gainKnob = 0.0f;      // -1..1 → -36..+12 dB

	EnvelopeFollower _follower;
	Amplifier _gain;
	Saturator _saturator;
	float _sampleRate = 48000.0f;

	void setSampleRate (float sr) { _sampleRate = sr; modulate(); }

	void modulate() {
		// follower_base.cpp sensitivity(): ((1-response)^2)^2
		float response = clampf (responseKnob, 0.0f, 1.0f);
		response = 1.0f - response;
		response *= response;
		response *= response;
		_follower.setParams (_sampleRate, response);

		// follower_base.cpp gain(): -1..0 → * -cutDb(=-36), 0..1 → * min(12, gainDb)
		float db = clampf (gainKnob, -1.0f, 1.0f);
		if (db < 0.0f) db = -db * -36.0f;
		else db *= 12.0f;
		_gain.setLevel (db);
	}

	inline float process (float in) {
		return _saturator.next (_gain.next (_follower.next (in)));
	}
};

//==============================================================================
// Offset (Offset.cpp) — out = in*scale + offset (scale-then-offset), clamp ±12 V.
struct Offset {
	float offsetKnob = 0.0f;  // -1..1 → ±10 V
	float scaleKnob = 1.0f;   // -1..1 → sign·knob²·10

	inline float process (float in) const {
		float offset = offsetKnob * 10.0f;
		float scale = scaleKnob < 0.0f ? -(scaleKnob * scaleKnob) : (scaleKnob * scaleKnob);
		scale *= 10.0f;
		float out = in * scale + offset;   // json offset_first=false in all patch instances
		return clampf (out, -12.0f, 12.0f);
	}
};

//==============================================================================
// Cmp (Cmp.cpp) — window comparator with lag (debounce) state machine; GREATER output only.
struct Cmp {
	float aKnob = 0.0f;       // -1..1 → ±10 V added to A input
	float bKnob = 0.0f;       // -1..1 → ±10 V
	float windowKnob = 0.5f;  // 0..1 → ×10 V (EQUAL outputs; unused in patch)
	float lagKnob = 0.1f;     // lag seconds = knob²
	// OUTPUT param = 0 in all instances → 0/10 V

	enum State { LOW, HIGH, LAG_LOW, LAG_HIGH };
	State _thresholdState = LOW;
	int _thresholdLag = 0;
	int _lagInSamples = 0;
	float _sampleRate = 48000.0f;

	void setSampleRate (float sr) { _sampleRate = sr; modulate(); }
	void reset() { _thresholdState = LOW; _thresholdLag = 0; }

	void modulate() {
		float lag = lagKnob;
		_lagInSamples = (int) (lag * lag * _sampleRate);
	}

	// returns GREATER output voltage (0 or 10)
	inline float process (float aIn) {
		float a = clampf (aKnob * 10.0f + aIn, -12.0f, 12.0f);
		float b = bKnob * 10.0f;
		bool high = a >= b;

		switch (_thresholdState) {
			case LOW:      if (high)  { if (_lagInSamples < 1) _thresholdState = HIGH; else { _thresholdState = LAG_HIGH; _thresholdLag = _lagInSamples; } } break;
			case HIGH:     if (!high) { if (_lagInSamples < 1) _thresholdState = LOW;  else { _thresholdState = LAG_LOW;  _thresholdLag = _lagInSamples; } } break;
			case LAG_LOW:  if (!high) { if (--_thresholdLag == 0) _thresholdState = LOW; }  else _thresholdState = HIGH; break;
			case LAG_HIGH: if (high)  { if (--_thresholdLag == 0) _thresholdState = HIGH; } else _thresholdState = LOW;  break;
		}
		switch (_thresholdState) {
			case LOW: case LAG_HIGH: return 0.0f;
			default:                 return 10.0f;
		}
	}
};

//==============================================================================
// MockbaModular DualAND (one gate) — out = 10 V iff a > 0 && b > 0.
struct ANDGate {
	inline float process (float a, float b) const { return (a > 0.0f && b > 0.0f) ? 10.0f : 0.0f; }
};

//==============================================================================
// Befaco DualAtenuverter (one half) — out = clamp(in*att + offset, ±10 V).
struct BefacoAtten {
	float att = 0.0f, offset = 0.0f;
	inline float process (float in) const { return clampf (in * att + offset, -10.0f, 10.0f); }
};

//==============================================================================
// DADSRH (dadsrh_core.cpp) — one-shot triggered DADSR(hold) envelope.
// Patch settings baked as defaults: triggered mode, stop, normal speed, resume-attack.
struct DADSRH {
	enum Stage { STOPPED_STAGE, DELAY_STAGE, ATTACK_STAGE, DECAY_STAGE, SUSTAIN_STAGE, RELEASE_STAGE };

	// params (knob units)
	float delayKnob = 0.0f;
	float attackKnob = 0.14142f;
	float decayKnob = 0.31623f;
	float sustainKnob = 0.5f;
	float releaseKnob = 0.31623f;
	float holdKnob = 0.44721f;
	int attackShape = 1, decayShape = 1, releaseShape = 1;  // 1..3 as stored
	bool gateMode = false;       // MODE_PARAM > 0.5 (all patch instances: false = triggered)
	bool loop = false;           // LOOP_PARAM <= 0.5 (patch: param=1 → loop=false)
	bool slow = false;           // SPEED_PARAM <= 0.5 (patch: param=1 → normal)
	bool resumeAttack = true;    // RETRIGGER_PARAM > 0.5 (patch: 1)

	Trigger _trigger;
	Stage _stage = STOPPED_STAGE;
	float _envelope = 0.0f, _stageProgress = 0.0f, _holdProgress = 0.0f, _releaseLevel = 0.0f;
	float _sampleTime = 1.0f / 48000.0f;

	void setSampleRate (float sr) { _sampleTime = 1.0f / sr; }
	void reset() { _trigger.reset(); _stage = STOPPED_STAGE; _releaseLevel = _holdProgress = _stageProgress = _envelope = 0.0f; }

	static constexpr float shapeExponent = 2.0f;
	static constexpr float shapeInverseExponent = 0.5f;

	float knobTime (float knob, bool allowZero = false) const {
		float t = clampf (knob, 0.0f, 1.0f);
		t = t * t;
		t = std::max (t, allowZero ? 0.0f : 0.001f);
		return t * (slow ? 100.0f : 10.0f);
	}
	float stepAmount (float knob, bool allowZero = false) const { return _sampleTime / knobTime (knob, allowZero); }

	// returns env in volts 0..10
	float step (float triggerIn) {
		const int SHAPE2 = 2, SHAPE3 = 3;

		if (_trigger.process (triggerIn)) {
			if (_stage == STOPPED_STAGE || !resumeAttack) {
				_stage = DELAY_STAGE;
				_holdProgress = _stageProgress = _envelope = 0.0f;
			}
			else {
				switch (_stage) {
					case STOPPED_STAGE:
					case ATTACK_STAGE: break;
					case DELAY_STAGE: {
						_stage = ATTACK_STAGE;
						_stageProgress = _envelope = 0.0f;
						float delayTime = knobTime (delayKnob, true);
						float holdTime = knobTime (holdKnob);
						_holdProgress = std::fmin (1.0f, delayTime / holdTime);
						break;
					}
					case DECAY_STAGE:
					case SUSTAIN_STAGE:
					case RELEASE_STAGE: {
						_stage = ATTACK_STAGE;
						switch (attackShape) {
							case SHAPE2: _stageProgress = _envelope; break;
							case SHAPE3: _stageProgress = std::pow (_envelope, shapeInverseExponent); break;
							default:     _stageProgress = std::pow (_envelope, shapeExponent); break;
						}
						float delayTime = knobTime (delayKnob, true);
						float attackTime = knobTime (attackKnob);
						float holdTime = knobTime (holdKnob);
						_holdProgress = std::fmin (1.0f, (delayTime + _stageProgress * attackTime) / holdTime);
						break;
					}
				}
			}
		}
		else {
			switch (_stage) {
				case STOPPED_STAGE:
				case RELEASE_STAGE: break;
				case DELAY_STAGE:
				case ATTACK_STAGE:
				case DECAY_STAGE:
				case SUSTAIN_STAGE: {
					bool holdComplete = _holdProgress >= 1.0f;
					if (!holdComplete) {
						_holdProgress += stepAmount (holdKnob);
						holdComplete = _holdProgress >= 1.0f;
					}
					if (gateMode ? !_trigger.isHigh() : holdComplete) {
						_stage = RELEASE_STAGE;
						_stageProgress = 0.0f;
						_releaseLevel = _envelope;
					}
					break;
				}
			}
		}

		switch (_stage) {
			case STOPPED_STAGE: break;
			case DELAY_STAGE: {
				_stageProgress += stepAmount (delayKnob, true);
				if (_stageProgress >= 1.0f) { _stage = ATTACK_STAGE; _stageProgress = 0.0f; }
				break;
			}
			case ATTACK_STAGE: {
				_stageProgress += stepAmount (attackKnob);
				switch (attackShape) {
					case SHAPE2: _envelope = _stageProgress; break;
					case SHAPE3: _envelope = std::pow (_stageProgress, shapeExponent); break;
					default:     _envelope = std::pow (_stageProgress, shapeInverseExponent); break;
				}
				if (_envelope >= 1.0f) { _stage = DECAY_STAGE; _stageProgress = 0.0f; }
				break;
			}
			case DECAY_STAGE: {
				float sustainLevel = clampf (sustainKnob, 0.0f, 1.0f);
				_stageProgress += stepAmount (decayKnob);
				switch (decayShape) {
					case SHAPE2: _envelope = 1.0f - _stageProgress; break;
					case SHAPE3: _envelope = _stageProgress >= 1.0f ? 0.0f : std::pow (1.0f - _stageProgress, shapeInverseExponent); break;
					default:     _envelope = _stageProgress >= 1.0f ? 0.0f : std::pow (1.0f - _stageProgress, shapeExponent); break;
				}
				_envelope *= 1.0f - sustainLevel;
				_envelope += sustainLevel;
				if (_envelope <= sustainLevel) _stage = SUSTAIN_STAGE;
				break;
			}
			case SUSTAIN_STAGE: {
				_envelope = clampf (sustainKnob, 0.0f, 1.0f);
				break;
			}
			case RELEASE_STAGE: {
				_stageProgress += stepAmount (releaseKnob);
				switch (releaseShape) {
					case SHAPE2: _envelope = 1.0f - _stageProgress; break;
					case SHAPE3: _envelope = _stageProgress >= 1.0f ? 0.0f : std::pow (1.0f - _stageProgress, shapeInverseExponent); break;
					default:     _envelope = _stageProgress >= 1.0f ? 0.0f : std::pow (1.0f - _stageProgress, shapeExponent); break;
				}
				_envelope *= _releaseLevel;
				if (_envelope <= 0.001f) {
					_envelope = 0.0f;
					if (!gateMode && (loop || _trigger.isHigh())) {
						_stage = DELAY_STAGE;
						_holdProgress = _stageProgress = 0.0f;
					}
					else {
						_stage = STOPPED_STAGE;
					}
				}
				break;
			}
		}

		return _envelope * 10.0f;
	}
};

//==============================================================================
// VCAmp (VCAmp.cpp) — dB-domain VCA: dB = (knob·cv/10)·72 − 60, slewed, saturated output.
struct VCAmp {
	static constexpr float maxDecibels = 12.0f;
	static constexpr float minDecibels = -60.0f;   // Amplifier::minDecibels

	float levelKnob = 0.8333f;
	bool cvConnected = true;

	Amplifier _amplifier;
	SlewLimiter _levelSL;
	Saturator _saturator;

	void setSampleRate (float sr) {
		_levelSL.setParams (sr, 5.0f /*MixerChannel::levelSlewTimeMS*/, maxDecibels - minDecibels);
	}

	inline float process (float in, float cv) {
		float level = levelKnob;
		if (cvConnected)
			level *= clampf (cv, 0.0f, 10.0f) / 10.0f;
		level *= maxDecibels - minDecibels;
		level += minDecibels;
		_amplifier.setLevel (_levelSL.next (level));
		return _saturator.next (_amplifier.next (in));
	}
};

} // namespace gemsv2

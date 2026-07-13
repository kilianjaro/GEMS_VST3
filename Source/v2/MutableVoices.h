#pragma once

// GEMS v2 — Braids + Plaits voices (chunk 4.4). Drive the vendored Mutable
// Instruments DSP (Source/v2/eurorack/, MIT) exactly as the AudibleInstruments VCV
// wrappers do (src/Braids.cpp, src/Plaits.cpp, GPL-3.0 wrapper logic), including the
// internal-rate → host-rate resampling via speexdsp at quality 4
// (SPEEX_RESAMPLER_QUALITY_DEFAULT — matches rack::dsp::SampleRateConverter; see
// speex/GEMS_BUILD_NOTES.md). Both voices are mono in the patch (Braids OUT; Plaits OUT).

#include <cstring>
#include <cmath>
#include <cstdint>

#include "braids/macro_oscillator.h"
#include "braids/vco_jitter_source.h"
#include "braids/signature_waveshaper.h"
#include "plaits/dsp/voice.h"
#include "stmlib/utils/buffer_allocator.h"
#include "SpeexSRC.h"   // GEMS: shared SpeexSRC + SampleFifo (also used by NeilVoice.h)

namespace gemsv2 {

//==============================================================================
// Braids — patch #1010: shape 0.976→45 (PARTICLE_NOISE), coarse -1, fine 0, timbre 0.5,
// modulation(timbre-CV atten) 0, color 0.363. Renders 24-sample @96k blocks, SRC→host,
// output 5·sample. Pitch is env-derived (fed as pitchV volts). vco_drift=0, signature=0
// (both jitter + waveshaper are exact no-ops here but kept for fidelity).
struct BraidsVoice {
	braids::MacroOscillator osc;
	braids::SettingsData settings;
	braids::VcoJitterSource jitter_source;
	braids::SignatureWaveshaper ws;
	SpeexSRC src;
	SampleFifo<512> fifo;
	float sampleRate = 48000.0f;

	// params (patch defaults)
	float shapeParam = 0.975904f;   // *46 -> 45 PARTICLE_NOISE
	float coarseParam = -1.0f;      // semitones-ish (Braids COARSE, -5..3)
	float fineParam = 0.0f;
	float timbreParam = 0.5f;
	float colorParam = 0.362651f;
	// live modulation inputs (volts); colorCv from Dust/Branches path (chunk 4.9; 0 for now)
	float timbreCv = 0.0f;          // TIMBRE_INPUT; MODULATION_PARAM=0 so has no effect
	float modulationParam = 0.0f;
	float colorCv = 0.0f;           // COLOR_INPUT

	float out = 0.0f;               // ±5 V

	void setSampleRate (float sr) {
		sampleRate = sr;
		src.setRates (96000, (int) sr);
	}

	void reset() {
		std::memset (&osc, 0, sizeof (osc)); osc.Init();
		std::memset (&jitter_source, 0, sizeof (jitter_source)); jitter_source.Init();
		std::memset (&ws, 0, sizeof (ws)); ws.Init (0x0000);
		std::memset (&settings, 0, sizeof (settings));
		settings.meta_modulation = 0;
		settings.vco_drift = 0;
		settings.signature = 0;
		fifo.clear();
		out = 0.0f;
	}

	// strike: rising trigger. pitchV: 1V/oct (env-derived in the patch).
	void process (float pitchV, bool trigRise) {
		if (trigRise) osc.Strike();

		if (fifo.empty()) {
			int shape = (int) std::round (shapeParam * braids::MACRO_OSC_SHAPE_LAST_ACCESSIBLE_FROM_META);
			settings.shape = (uint8_t) std::max (0, std::min (shape, (int) braids::MACRO_OSC_SHAPE_LAST_ACCESSIBLE_FROM_META));
			osc.set_shape ((braids::MacroOscillatorShape) settings.shape);

			float timbre = timbreParam + modulationParam * timbreCv / 5.0f;
			float modulation = colorParam + colorCv / 5.0f;
			int16_t param1 = (int16_t) (std::max (0.0f, std::min (1.0f, timbre)) * 32767.0f);
			int16_t param2 = (int16_t) (std::max (0.0f, std::min (1.0f, modulation)) * 32767.0f);
			osc.set_parameters (param1, param2);

			float pv = pitchV + coarseParam + fineParam / 12.0f;
			int32_t pitch = (int32_t) ((pv * 12.0 + 60) * 128);
			pitch += jitter_source.Render (settings.vco_drift);
			pitch = std::max (0, std::min (pitch, 16383));
			osc.set_pitch ((int16_t) pitch);

			uint8_t sync_buffer[24] = {};
			int16_t render_buffer[24];
			osc.Render (sync_buffer, render_buffer, 24);

			uint16_t signature = settings.signature * settings.signature * 4095;
			float in[24];
			for (int i = 0; i < 24; ++i) {
				int16_t sample = render_buffer[i];
				int16_t warped = ws.Transform (sample);
				render_buffer[i] = stmlib::Mix (sample, warped, signature);
				in[i] = render_buffer[i] / 32768.0f;
			}
			float tmp[512]; unsigned inLen = 24, outLen = (unsigned) fifo.capacity();
			src.process (in, inLen, tmp, outLen);
			fifo.pushN (tmp, (int) outLen);
		}

		out = fifo.empty() ? 0.0f : 5.0f * fifo.shift();
	}
};

//==============================================================================
// Plaits — patch #4830: model 0 (virtual analog), freq 0, harmonics 0, timbre 0.79,
// morph 0.74, timbre-CV atten 0.341, morph-CV atten 0.272, LPG colour/decay 0.5.
// note from Quack (V/oct). timbre/morph CV from Walk2 (chunk 4.9; 0 for now — but the
// inputs ARE patched, so patched-flags are true and the mod amounts are live). TRIGGER
// unpatched -> LPG free-runs. Renders 12-sample @48k blocks, SRC→host, output -5·out.
struct PlaitsVoice {
	plaits::Voice voice;
	plaits::Patch patch = {};
	char shared_buffer[16384] = {};
	SpeexSRC src;
	SampleFifo<512> fifo;
	float sampleRate = 48000.0f;

	// params
	float freqParam = 0.0f;
	float harmonicsParam = 0.0f;
	float timbreParam = 0.792773f;
	float morphParam = 0.743374f;
	float lpgColorParam = 0.5f;
	float lpgDecayParam = 0.5f;
	float timbreCvAmount = 0.341335f;   // TIMBRE_CV_PARAM
	float freqCvAmount = 0.0f;
	float morphCvAmount = 0.272f;       // MORPH_CV_PARAM
	// live CV inputs (volts); from Walk2 in the patch (chunk 4.9; 0 for now)
	float timbreCv = 0.0f;   // TIMBRE_INPUT (idx1)
	float morphCv = 0.0f;    // MORPH_INPUT (idx3)

	float out = 0.0f;        // ±5 V (OUT, inverted per wrapper)

	void setSampleRate (float sr) {
		sampleRate = sr;
		src.setRates (48000, (int) sr);
	}

	void reset() {
		stmlib::BufferAllocator allocator (shared_buffer, sizeof (shared_buffer));
		voice.Init (&allocator);
		patch.engine = 0;
		patch.lpg_colour = 0.5f;
		patch.decay = 0.5f;
		fifo.clear();
		out = 0.0f;
	}

	// noteCv: 1V/oct from Quack (NOTE_INPUT).
	void process (float noteCv) {
		if (fifo.empty()) {
			const int blockSize = 12;
			patch.engine = 0;
			patch.note = 60.0f + freqParam * 12.0f;
			patch.harmonics = harmonicsParam;
			patch.timbre = timbreParam;
			patch.morph = morphParam;
			patch.lpg_colour = lpgColorParam;
			patch.decay = lpgDecayParam;
			patch.frequency_modulation_amount = freqCvAmount;
			patch.timbre_modulation_amount = timbreCvAmount;
			patch.morph_modulation_amount = morphCvAmount;

			plaits::Modulations modulations;
			std::memset (&modulations, 0, sizeof (modulations));
			modulations.engine = 0.0f;
			modulations.note = noteCv * 12.0f;         // NOTE_INPUT * 12
			modulations.frequency = 0.0f;
			modulations.harmonics = 0.0f;
			modulations.timbre = timbreCv / 8.0f;      // TIMBRE_INPUT / 8
			modulations.morph = morphCv / 8.0f;        // MORPH_INPUT / 8
			modulations.trigger = 0.0f;
			modulations.level = 0.0f;
			modulations.frequency_patched = false;     // FREQ_INPUT unpatched
			modulations.timbre_patched = true;         // TIMBRE_INPUT wired (Walk2)
			modulations.morph_patched = true;          // MORPH_INPUT wired (Walk2)
			modulations.trigger_patched = false;       // TRIGGER unpatched -> LPG free-run
			modulations.level_patched = false;

			plaits::Voice::Frame output[blockSize];
			voice.Render (patch, modulations, output, blockSize);

			float in[blockSize];
			for (int i = 0; i < blockSize; ++i) in[i] = output[i].out / 32768.0f;
			float tmp[512]; unsigned inLen = (unsigned) blockSize, outLen = (unsigned) fifo.capacity();
			src.process (in, inLen, tmp, outLen);
			fifo.pushN (tmp, (int) outLen);
		}
		out = fifo.empty() ? 0.0f : -5.0f * fifo.shift();
	}
};

} // namespace gemsv2

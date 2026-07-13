#pragma once

// GEMS v2 — Neil = Clouds "Parasite" granular processor (chunk 4.8, chain 4). Drives the
// vendored Clouds parasite DSP (Source/v2/clouds/, MIT; parasite firmware Matthias Puech)
// exactly as the ArableInstruments VCV wrapper does (src/Clouds.cpp, GPL-3): input ×gain/5
// → SRC to 32 kHz → int16 blocks of 32 → GranularProcessor::Process → int16/32768 → SRC to
// host. PLAYBACK_MODE_GRANULAR, 2 channels, buffersize 1. See NOTICE.md.
// Patch Neil#1829: position 0.618, size 0.778 (+SIZE_CV in3), pitch 0, in-gain 0.931,
// density 0.735, texture 0.800 (+TEXTURE_CV in12), blend 0.612, spread 0.813, feedback 0.786,
// reverb 0. Audio in ← VCF#7322 (in9/in10). Param CV = knob + input/5, clamped 0..1.

// GEMS: PARASITES selects the parasite firmware; TEST makes the Clouds hardware headers
// (debug_pin.h) skip the STM32 includes. TEST is confined to the clouds include so it can't
// leak into other code in the same TU. The vendored clouds .cc are compiled with the same
// defines — keep them consistent.
#ifndef PARASITES
#define PARASITES
#endif
#ifndef TEST
#define TEST
#define GEMS_UNDEF_TEST
#endif
#include "clouds/dsp/granular_processor.h"
#ifdef GEMS_UNDEF_TEST
#undef TEST
#undef GEMS_UNDEF_TEST
#endif

#include "SpeexSRC.h"
#include <cstring>
#include <cstdint>
#include <algorithm>

namespace gemsv2 {

struct NeilVoice {
	clouds::GranularProcessor processor;
	uint8_t* blockMem = nullptr;
	uint8_t* blockCcm = nullptr;
	static constexpr int kMemLen = 118784;      // 118784 * buffersize(1)
	static constexpr int kCcmLen = 65536 - 128;

	SpeexSRC inSrc, outSrcL, outSrcR;
	// input accumulation at host rate → resample to 32 k in blocks of 32
	float inAccumL[64] = {}, inAccumR[64] = {};
	int inAccum = 0;
	SampleFifo<512> fifoL, fifoR;
	float sampleRate = 48000.0f;

	// params (patch defaults)
	float position = 0.618073f, size = 0.778313f, pitch = 0.0f, inGain = 0.931325f;
	float density = 0.734937f, texture = 0.799997f, blend = 0.612048f;
	float spread = 0.813254f, feedback = 0.785538f, reverb = 0.0f;

	float outL = 0.0f, outR = 0.0f;

	~NeilVoice() { delete[] blockMem; delete[] blockCcm; }

	void setSampleRate (float sr) {
		sampleRate = sr;
		if (!blockMem) { blockMem = new uint8_t[kMemLen](); blockCcm = new uint8_t[kCcmLen](); }
		std::memset (&processor, 0, sizeof (processor));
		processor.Init (blockMem, kMemLen, blockCcm, kCcmLen);
		processor.set_num_channels (2);
		processor.set_low_fidelity (false);
		processor.set_playback_mode (clouds::PLAYBACK_MODE_GRANULAR);
		reset();
	}

	void reset() {
		inAccum = 0;
		fifoL.clear(); fifoR.clear();
		outL = outR = 0.0f;
	}

	static inline float clamp01 (float v) { return v < 0 ? 0 : (v > 1 ? 1 : v); }

	// inL/inR: audio volts (VCF#7322, same mono into both); sizeCv/textureCv: CV volts.
	void process (float inL, float inR, float sizeCv, float textureCv) {
		// accumulate host-rate input; when we have a chunk, resample to 32 k and render a block.
		if (inAccum < 64) {
			inAccumL[inAccum] = inL * inGain / 5.0f;
			inAccumR[inAccum] = inR * inGain / 5.0f;
			++inAccum;
		}

		if (fifoL.empty()) {
			// resample accumulated host-rate input down to 32 k (interleave via two mono SRCs)
			clouds::ShortFrame input[32] = {};
			float in32L[32], in32R[32];
			inSrc.setRates ((int) sampleRate, 32000);
			unsigned inLen = (unsigned) inAccum, outLenL = 32;
			inSrc.process (inAccumL, inLen, in32L, outLenL);
			unsigned inLen2 = (unsigned) inAccum, outLenR = 32;
			// GEMS: use a second SRC state for R so both channels stay phase-aligned
			static thread_local SpeexSRC inSrcR; inSrcR.setRates ((int) sampleRate, 32000);
			inSrcR.process (inAccumR, inLen2, in32R, outLenR);
			inAccum = 0;
			int nin = (int) outLenL;
			for (int i = 0; i < 32; ++i) {
				float l = i < nin ? in32L[i] : 0.0f, r = i < (int) outLenR ? in32R[i] : 0.0f;
				input[i].l = (short) std::max (-32768.0f, std::min (32767.0f, l * 32767.0f));
				input[i].r = (short) std::max (-32768.0f, std::min (32767.0f, r * 32767.0f));
			}

			clouds::Parameters* p = processor.mutable_parameters();
			p->trigger = false; p->gate = false; p->freeze = false;
			p->position = clamp01 (position);
			p->size = clamp01 (size + sizeCv / 5.0f);
			p->pitch = std::max (-48.0f, std::min (48.0f, (pitch) * 12.0f));
			p->density = clamp01 (density);
			p->texture = clamp01 (texture + textureCv / 5.0f);
			p->dry_wet = clamp01 (blend);
			p->stereo_spread = clamp01 (spread);
			p->feedback = clamp01 (feedback);
			p->reverb = clamp01 (reverb);
			p->granular.reverse = false;

			processor.Prepare();
			clouds::ShortFrame output[32];
			processor.Process (input, output, 32);

			float o32L[32], o32R[32];
			for (int i = 0; i < 32; ++i) { o32L[i] = output[i].l / 32768.0f; o32R[i] = output[i].r / 32768.0f; }
			float hostL[512]; unsigned il = 32, olL = (unsigned) fifoL.capacity();
			outSrcL.setRates (32000, (int) sampleRate); outSrcL.process (o32L, il, hostL, olL);
			float hostR[512]; unsigned ir = 32, olR = (unsigned) fifoR.capacity();
			outSrcR.setRates (32000, (int) sampleRate); outSrcR.process (o32R, ir, hostR, olR);
			fifoL.pushN (hostL, (int) olL);
			fifoR.pushN (hostR, (int) olR);
		}

		outL = fifoL.empty() ? 0.0f : 5.0f * fifoL.shift();
		outR = fifoR.empty() ? 0.0f : 5.0f * fifoR.shift();
	}
};

} // namespace gemsv2

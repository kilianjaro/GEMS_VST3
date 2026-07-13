#pragma once

// GEMS v2 — shared speex sample-rate converter + output FIFO, used by the internal-rate
// voices (Braids 96 k, Plaits 48 k, Neil/Clouds 32 k) to resample to the host rate exactly
// as the VCV wrappers do via rack::dsp::SampleRateConverter (speexdsp, quality 4). See
// speex/GEMS_BUILD_NOTES.md and NOTICE.md.

extern "C" {
#ifndef OUTSIDE_SPEEX
#define OUTSIDE_SPEEX
#endif
#ifndef RANDOM_PREFIX
#define RANDOM_PREFIX speex
#endif
#include "speex/speex_resampler.h"
}

namespace gemsv2 {

// Thin wrapper matching rack::dsp::SampleRateConverter<1> defaults (quality 4).
struct SpeexSRC {
	SpeexResamplerState* st = nullptr;
	int inRate = 0, outRate = 0;

	~SpeexSRC() { if (st) speex_resampler_destroy (st); }

	void setRates (int in, int out) {
		if (in == inRate && out == outRate && st) return;
		inRate = in; outRate = out;
		if (st) { speex_resampler_destroy (st); st = nullptr; }
		int err = 0;
		st = speex_resampler_init (1, (spx_uint32_t) in, (spx_uint32_t) out,
		                           SPEEX_RESAMPLER_QUALITY_DEFAULT, &err);
	}

	// process one block: inLen in-samples -> up to *outLen out-samples (updated)
	void process (const float* in, unsigned& inLen, float* out, unsigned& outLen) {
		spx_uint32_t il = inLen, ol = outLen;
		speex_resampler_process_interleaved_float (st, in, &il, out, &ol);
		inLen = il; outLen = ol;
	}
};

// Host-rate output FIFO fed by resampled blocks (render a block when empty, shift one/sample).
template <int CAP>
struct SampleFifo {
	float buf[CAP];
	int head = 0, count = 0;
	bool empty() const { return count == 0; }
	int capacity() const { return CAP; }
	void clear() { head = count = 0; }
	void pushN (const float* x, int n) {
		for (int i = 0; i < n && count < CAP; ++i) { buf[(head + count) % CAP] = x[i]; ++count; }
	}
	float shift() { float v = buf[head]; head = (head + 1) % CAP; --count; return v; }
};

} // namespace gemsv2

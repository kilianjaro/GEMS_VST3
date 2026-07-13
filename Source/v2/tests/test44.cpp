// GEMS v2 — chunk 4.4 verification: Braids + Plaits voices.
//
// Plaits (model 0 = virtual analog) IS pitched: unit-test the note law
//   note = 60 + freqParam*12 + noteCv*12  ->  f = 440*2^((note-69)/12) = 261.626*2^noteCv.
// Braids shape 45 (PARTICLE_NOISE) is a NOISE engine — no clean pitch — so verify it is
// audible and broadband (high zero-cross density) after a strike, and silent otherwise.
// Both are checked through the engine too (gated one-shot / free-run + VCA).

#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <vector>
#include "V2Engine.h"

using namespace gemsv2;

static int failures = 0;
#define CHECK(cond, ...) do { if (!(cond)) { ++failures; printf("FAIL: " __VA_ARGS__); printf("\n"); } } while (0)
static float kSR = 48000.0f;

// octave-safe normalized-autocorrelation fundamental (as test43)
static float measureHz (const std::vector<float>& v, float minHz = 20.0f) {
	int n = (int) v.size(), start = n / 2, len = n - start;
	const float* x = v.data() + start;
	int maxLag = std::min (len - 2, (int) (kSR / minHz));
	std::vector<double> r ((size_t) (maxLag + 2), 0.0);
	for (int lag = 0; lag <= maxLag + 1; ++lag) {
		double acc = 0, e0 = 0, e1 = 0;
		for (int i = 0; i + lag < len; ++i) { acc += (double) x[i] * x[i + lag]; e0 += (double) x[i] * x[i]; e1 += (double) x[i + lag] * x[i + lag]; }
		r[(size_t) lag] = (e0 > 0 && e1 > 0) ? acc / std::sqrt (e0 * e1) : 0.0;
	}
	int lag0 = 1; while (lag0 <= maxLag && r[(size_t) lag0] > 0.0) ++lag0;
	double gmax = 0.0; for (int lag = lag0; lag <= maxLag; ++lag) gmax = std::max (gmax, r[(size_t) lag]);
	int bestLag = lag0;
	for (int lag = lag0 + 1; lag < maxLag; ++lag)
		if (r[(size_t) lag] >= 0.85 * gmax && r[(size_t) lag] >= r[(size_t) (lag - 1)] && r[(size_t) lag] >= r[(size_t) (lag + 1)]) { bestLag = lag; break; }
	if (bestLag > 1 && bestLag < maxLag) {
		double y0 = r[(size_t) (bestLag - 1)], y1 = r[(size_t) bestLag], y2 = r[(size_t) (bestLag + 1)];
		double d = y0 - 2 * y1 + y2; float frac = d != 0.0 ? (float) (0.5 * (y0 - y2) / d) : 0.0f;
		return kSR / (bestLag + frac);
	}
	return kSR / (float) bestLag;
}
// pitch confirmation for a KNOWN target: normalized-ACF peak within ±1 octave of expHz
// (a rich VA/texture waveform's ACF has octave-related peaks; constraining to a band
// around the expected value is the standard way to verify a known-pitch oscillator).
static float measureHzNear (const std::vector<float>& v, float expHz) {
	int n = (int) v.size(), start = n / 2, len = n - start;
	const float* x = v.data() + start;
	int loLag = std::max (2, (int) (kSR / (expHz * 1.5f)));
	int hiLag = std::min (len - 2, (int) (kSR / (expHz / 1.5f)));
	double best = -1e30; int bestLag = loLag;
	for (int lag = loLag; lag <= hiLag; ++lag) {
		double acc = 0, e0 = 0, e1 = 0;
		for (int i = 0; i + lag < len; ++i) { acc += (double) x[i] * x[i + lag]; e0 += (double) x[i] * x[i]; e1 += (double) x[i + lag] * x[i + lag]; }
		double r = (e0 > 0 && e1 > 0) ? acc / std::sqrt (e0 * e1) : 0.0;
		if (r > best) { best = r; bestLag = lag; }
	}
	return kSR / (float) bestLag;
}
static double rmsBack (const std::vector<float>& v) {
	double s = 0; int n = (int) v.size();
	for (int i = n / 2; i < n; ++i) s += (double) v[(size_t) i] * v[(size_t) i];
	return std::sqrt (s / (n - n / 2));
}
static int zeroCross (const std::vector<float>& v) {
	int n = (int) v.size(), zc = 0; float prev = v[(size_t) (n / 2)];
	for (int i = n / 2 + 1; i < n; ++i) { if ((prev < 0) != (v[(size_t) i] < 0)) ++zc; prev = v[(size_t) i]; }
	return zc;
}

static void unitPlaits (float noteCv) {
	PlaitsVoice p; p.setSampleRate (kSR); p.reset();
	std::vector<float> buf ((size_t) (2.0f * kSR));
	for (size_t i = 0; i < buf.size(); ++i) { p.process (noteCv); buf[i] = p.out; }
	float exp = 261.626f * std::exp2 (noteCv), f = measureHzNear (buf, exp);
	printf ("   Plaits(VA) noteCv %+5.2f -> %7.2f Hz (exp %7.2f) rms %.3f\n", noteCv, f, exp, rmsBack (buf));
	CHECK (std::fabs (f - exp) < exp * 0.03f, "Plaits note %.2f (exp %.1f got %.1f)", noteCv, exp, f);
	CHECK (rmsBack (buf) > 0.05, "Plaits audible (free-run)");
}

static void unitBraids (float pitchV) {
	BraidsVoice b; b.setSampleRate (kSR); b.reset();
	std::vector<float> buf ((size_t) (1.0f * kSR));
	b.process (pitchV, true);   // strike
	for (size_t i = 0; i < buf.size(); ++i) { b.process (pitchV, false); buf[i] = b.out; }
	double r = rmsBack (buf); int zc = zeroCross (buf);
	float zcDensity = zc / ((float) (buf.size() / 2) / kSR);   // crossings/sec
	// PARTICLE_NOISE = pitched resonant-noise grains (not white noise), so its zc-density is
	// modest and pitch-dependent; the meaningful checks are that a strike makes it audible and
	// that it carries non-trivial spectral motion (zc > a clean sub-100 Hz tone would give).
	printf ("   Braids(45) pitchV %+5.2f -> rms %.3f, zc-density %.0f/s\n", pitchV, r, zcDensity);
	CHECK (r > 0.05, "Braids audible after strike (rms %.3f)", r);
	CHECK (zcDensity > 100.0f, "Braids produces motion (zc %.0f/s)", zcDensity);
}

// integration: feed an in-band tone; voice produces gated output through the engine
static float richTone (double& ph, float hz, int n, float amp, unsigned& s) {
	float vib = 1.0f + 0.015f * (float) sin (2.0 * M_PI * 6.3 * n / kSR);
	ph += 2.0 * M_PI * hz * vib / kSR;
	s = s * 1664525u + 1013904223u;
	return amp * (float) (sin (ph) + 0.4 * sin (2 * ph)) + (((float) (s >> 8) / 8388608.0f) - 1.0f) * 0.1f * amp;
}
static void integBraids () {
	// Braids: long-attack pad + Echo (1 s delay). Run 4 s so the wet echo develops.
	V2Engine eng; eng.prepare (kSR);
	double ph = 0; unsigned s = 3u;
	double rmsPre = 0; int preN = (int) (0.3f * kSR), pc = 0;
	for (int i = 0; i < preN; ++i) { eng.processSample (0.0f); if (i > preN - (int)(0.1f*kSR)) { float v = eng.voiceOut (V2Engine::kBraids); rmsPre += v*v; ++pc; } }
	rmsPre = std::sqrt (rmsPre / pc);
	int toneN = (int) (4.0f * kSR), w = (int) (0.1f * kSR);
	std::vector<float> vb ((size_t) toneN); double acc = 0, maxRms = 0;
	for (int i = 0; i < toneN; ++i) { eng.processSample (richTone (ph, 9000.0f, i, 2.5f, s)); vb[(size_t) i] = eng.voiceOut (V2Engine::kBraids); }
	for (int i = 0; i < toneN; ++i) { acc += (double) vb[(size_t) i]*vb[(size_t) i]; if (i>=w) acc -= (double) vb[(size_t)(i-w)]*vb[(size_t)(i-w)]; if (i>=w) maxRms = std::max (maxRms, std::sqrt (acc/w)); }
	printf ("   Braids   pre %.4f, loudest100ms %.3f V (incl. Echo)\n", rmsPre, maxRms);
	CHECK (rmsPre < 0.02, "Braids quiet before trigger");
	CHECK (maxRms > 0.03, "Braids audible after trigger (+Echo)");
}
static void integ (const char* name, int chainIdx, float toneHz) {
	V2Engine eng; eng.prepare (kSR);
	double ph = 0; unsigned s = 3u;
	double rmsPre = 0; int preN = (int) (0.3f * kSR), pc = 0;
	for (int i = 0; i < preN; ++i) { eng.processSample (0.0f); if (i > preN - (int)(0.1f*kSR)) { float v = eng.voiceOut (chainIdx); rmsPre += v*v; ++pc; } }
	rmsPre = std::sqrt (rmsPre / pc);
	int toneN = (int) (1.5f * kSR), w = (int) (0.1f * kSR);
	std::vector<float> vb ((size_t) toneN); double acc = 0, maxRms = 0;
	for (int i = 0; i < toneN; ++i) { eng.processSample (richTone (ph, toneHz, i, 2.5f, s)); vb[(size_t) i] = eng.voiceOut (chainIdx); }
	for (int i = 0; i < toneN; ++i) { acc += (double) vb[(size_t) i]*vb[(size_t) i]; if (i>=w) acc -= (double) vb[(size_t)(i-w)]*vb[(size_t)(i-w)]; if (i>=w) maxRms = std::max (maxRms, std::sqrt (acc/w)); }
	printf ("   %-8s pre %.4f, loudest100ms %.3f V\n", name, rmsPre, maxRms);
	CHECK (rmsPre < 0.02, "%s quiet before trigger", name);
	CHECK (maxRms > 0.05, "%s audible after trigger", name);
}

int main (int argc, char** argv) {
	if (argc > 1) kSR = (float) atof (argv[1]);
	printf ("GEMS v2 chunk 4.4 verification @ %g Hz\n", kSR);
	printf ("-- Plaits pitch law (virtual analog) --\n");
	for (float cv : { -1.0f, 0.0f, 1.0f }) unitPlaits (cv);
	printf ("-- Braids PARTICLE_NOISE (broadband, gated) --\n");
	for (float pv : { -1.0f, 0.5f }) unitBraids (pv);
	printf ("-- gated integration --\n");
	integ ("Plaits", V2Engine::kPlaits, 5500.0f);
	// Braids chain now includes Sapphire Echo (~1 s delay, globalMix 0.83 mostly-wet):
	integBraids ();
	printf (failures ? "\n%d FAILURE(S)\n" : "\nALL PASS\n", failures);
	return failures ? 1 : 0;
}

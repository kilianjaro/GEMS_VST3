// GEMS v2 — chunk 4.9 verification: generative/mod modules + FX standalone sanity.
//   LFO periods, Dust event rate, Branches A/B split + latch, Walk2 boundedness,
//   Echo delay timing.

#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <vector>
#include "GenerativeModules.h"
#include "FxEcho.h"

using namespace gemsv2;
static int failures = 0;
#define CHECK(cond, ...) do { if (!(cond)) { ++failures; printf("FAIL: " __VA_ARGS__); printf("\n"); } } while (0)
static float kSR = 48000.0f;

static void testLFO() {
	printf ("-- Bogaudio LFO --\n");
	// #0793: 2^-4.30723 = 0.0503 Hz sine, scale 0.901 → ±4.5 V
	BogaudioLFO l; l.freqKnob = -4.30723f; l.scaleKnob = 0.901204f; l.setSampleRate (kSR); l.reset();
	int n = (int) (60.0f * kSR); float prev = 0, amax = 0; int ups = 0;
	for (int i = 0; i < n; ++i) { l.process(); if (prev < 0 && l.sine >= 0) ++ups; amax = std::max (amax, std::fabs (l.sine)); prev = l.sine; }
	float measHz = ups / 60.0f, expHz = std::exp2 (-4.30723f);
	printf ("   LFO#0793 sine: %.4f Hz (exp %.4f), amp %.2f V (exp ~4.5)\n", measHz, expHz, amax);
	CHECK (std::fabs (measHz - expHz) < expHz * 0.1f, "LFO freq");
	CHECK (std::fabs (amax - 4.5f) < 0.6f, "LFO amplitude (scale·5)");
}

static void testDust() {
	printf ("-- HetrickCV Dust --\n");
	HetrickDust d; d.rateKnob = 2.56867f; d.setSampleRate (kSR); d.reset (0x1234u);
	int n = (int) (10.0f * kSR), events = 0; bool bad = false;
	for (int i = 0; i < n; ++i) { float v = d.process (0.0f); if (v != 0.0f) ++events; if (v < 0 || v > 10.001f) bad = true; }
	float measRate = events / 10.0f;
	float x = 2.56867f / 4.0f; float expRate = x*x*x * kSR;
	printf ("   Dust: %.0f events/s (exp %.0f = (rate/4)^3·sr), unipolar range OK=%d\n", measRate, expRate, !bad);
	CHECK (std::fabs (measRate - expRate) < expRate * 0.15f, "Dust event rate");
	CHECK (!bad, "Dust unipolar 0..10 V");
}

static void testBranches() {
	printf ("-- Audible Branches --\n");
	AudibleBranches b; b.prob = 0.595181f; b.latchMode = false; b.reset (0x99u);
	int toA = 0, tot = 0;
	for (int k = 0; k < 2000; ++k) {
		// a gate pulse: high 100 samples, low 100
		float a1=0,b1=0;
		for (int i = 0; i < 100; ++i) b.process (5.0f, a1, b1);
		bool wasA = a1 > 1.0f;
		for (int i = 0; i < 100; ++i) b.process (0.0f, a1, b1);
		if (wasA) ++toA; ++tot;
	}
	float frac = (float) toA / tot;
	printf ("   Branches p=0.595: %.1f%% to A (exp ~59.5)\n", frac * 100.0f);
	CHECK (std::fabs (frac - 0.595f) < 0.05f, "Branches A/B split");
	// latch mode toggles
	AudibleBranches bl; bl.prob = 1.0f; bl.latchMode = true; bl.reset (0x7u);   // p=1 → always "A"
	float la=0,lb=0; bl.process (5.0f, la, lb); bl.process (0.0f, la, lb);
	bool s1 = la > 1.0f;
	bl.process (5.0f, la, lb); bl.process (0.0f, la, lb);
	bool s2 = la > 1.0f;
	printf ("   latch: after 1 trig A=%d, after 2 A=%d (should toggle)\n", s1, s2);
	CHECK (s1 != s2, "Branches latch toggles");
}

static void testWalk2() {
	printf ("-- Bogaudio Walk2 --\n");
	BogaudioWalk2 w; w.rateXKnob = 0.328916f; w.rateYKnob = 0.307229f; w.scaleXKnob = 0.837348f; w.scaleYKnob = 0.783132f;
	w.setSampleRate (kSR); w.reset (0x51a3u);
	float xmax = 0, ymax = 0, xmin = 0, ymin = 0, maxStep = 0, px = 0;
	int n = (int) (30.0f * kSR);
	for (int i = 0; i < n; ++i) { w.process(); xmax = std::max (xmax, w.x); xmin = std::min (xmin, w.x); ymax = std::max (ymax, w.y); maxStep = std::max (maxStep, std::fabs (w.x - px)); px = w.x; }
	printf ("   Walk2 X range [%.2f,%.2f], max step %.4f (bounded, smooth)\n", xmin, xmax, maxStep);
	CHECK (xmax <= 5.0f && xmin >= -5.0f, "Walk2 bounded ±5 V");
	CHECK (maxStep < 0.5f, "Walk2 smooth (small steps)");
	CHECK (xmax - xmin > 0.3f, "Walk2 actually moves");
}

static void testEcho() {
	printf ("-- Sapphire Echo --\n");
	SapphireEcho e; e.setSampleRate (kSR); e.reset();
	int n = (int) (3.0f * kSR); std::vector<float> o ((size_t) n);
	for (int i = 0; i < n; ++i) { float in = (i < 48) ? 3.0f : 0.0f; float l, r; e.process (in, 0.0f, l, r); o[(size_t) i] = l; }
	// first echo near 1.0 s
	int pk = -1; float mx = 0;
	for (int i = (int)(0.3f*kSR); i < n; ++i) if (std::fabs (o[(size_t) i]) > mx) { mx = std::fabs (o[(size_t) i]); pk = i; }
	printf ("   Echo first repeat at %.3f s (exp ~1.0), amp %.3f\n", pk / kSR, mx);
	CHECK (std::fabs (pk / kSR - 1.0f) < 0.05f, "Echo 1 s delay");
}

int main (int argc, char** argv) {
	if (argc > 1) kSR = (float) atof (argv[1]);
	printf ("GEMS v2 chunk 4.9 verification @ %g Hz\n", kSR);
	testLFO(); testDust(); testBranches(); testWalk2(); testEcho();
	printf (failures ? "\n%d FAILURE(S)\n" : "\nALL PASS\n", failures);
	return failures ? 1 : 0;
}

// GEMS v2 — chunk 4.1 verification harness (offline, no JUCE).
// 1. Band VCF magnitude responses vs decoded expectations
// 2. DADSRH stage timing vs knob^2*10s
// 3. VCA dB law
// 4. Full-chain trigger behavior on a tone burst

#include <cstdio>
#include <cmath>
#include <vector>
#include <cstdlib>
#include "V2Engine.h"

using namespace gemsv2;

static int failures = 0;
#define CHECK(cond, ...) do { if (!(cond)) { ++failures; printf("FAIL: " __VA_ARGS__); printf("\n"); } } while (0)

static float kSR = 48000.0f;

// steady-state gain of a chain's band filter at frequency f (feeds sine, measures RMS out/in)
static float bandGainDb (int chainIdx, float f) {
	AnalysisChain c;
	c.configure (patch::chains[chainIdx], nullptr);
	c.setSampleRate (kSR);
	c.reset();
	c.modulate();
	// settle: gain slews (50 ms) + filter warmup
	int settle = (int) (0.5f * kSR), measure = (int) (0.5f * kSR);
	double phase = 0.0, w = 2.0 * M_PI * f / kSR;
	double sumIn = 0.0, sumOut = 0.0;
	ModulationClock clk; clk.setSampleRate (kSR);
	for (int n = 0; n < settle + measure; ++n) {
		float x = 5.0f * (float) sin (phase);   // Rack-level audio ±5 V
		phase += w;
		if (clk.tick()) c.modulate();
		float y = c.bandVcf.process (x);
		if (c.hasSecondVcf) y = c.bandVcf2.process (y);
		if (n >= settle) { sumIn += (double) x * x; sumOut += (double) y * y; }
	}
	return 20.0f * (float) log10 (sqrt (sumOut / sumIn) + 1e-12);
}

static void testFilters() {
	printf ("-- band filter responses --\n");
	struct Exp { int chain; float f; float minDb, maxDb; const char* what; };
	const Exp cases[] = {
		// chain 0: LP 240 Hz, 10-pole butterworth → passband ~0 dB, -3 dB @ 240, steep above
		// note: VCF has built-in 2-pole 80 Hz HP on output -> passband checked at 120 Hz;
		// Bogaudio's iq damping tweak puts ~0 dB at the knob frequency (not textbook -3 dB)
		{ 0, 120.0f,  -2.5f,  2.5f, "WT LP passband 120 Hz (iq ripple ~+1.7 dB)" },
		{ 0, 240.0f,  -3.0f,  1.5f, "WT LP knob freq 240 Hz" },
		{ 0, 480.0f, -75.0f, -45.0f, "WT LP +1 oct (10-pole ~-60 dB)" },
		// chain 1: HP440 → LP1100 cascade → band 440..1100 near 0 dB, steep outside
		{ 1, 700.0f,  -4.0f,  5.0f, "Ouros band center ~700 Hz (iq bump ~+3.5 dB)" },
		{ 1, 220.0f, -75.0f, -40.0f, "Ouros -1 oct below HP" },
		{ 1, 2200.0f, -75.0f, -40.0f, "Ouros +1 oct above LP" },
		// chain 2: BP 500 Hz, Q .596 → bw ±1.19 oct; center ~0 dB
		{ 2, 500.0f,  -2.0f,  2.0f, "Osculum BP center 500 Hz" },
		{ 2, 125.0f, -70.0f, -20.0f, "Osculum BP -2 oct" },
		// chain 5: BP 5500, Q .2 → bw ±0.4 oct
		{ 5, 5500.0f, -2.0f,  2.0f, "Plaits BP center 5.5 kHz" },
		{ 5, 1375.0f, -300.0f, -60.0f, "Plaits BP -2 oct (very steep)" },
		// chain 6: HP 8000
		// near-cutoff ripple varies with SR (bilinear warping — same in the original module)
		{ 6, 12000.0f, -3.0f, 3.0f, "Braids HP passband 12 kHz" },
		{ 6, 4000.0f, -75.0f, -45.0f, "Braids HP -1 oct" },
	};
	for (auto& e : cases) {
		float db = bandGainDb (e.chain, e.f);
		printf ("   chain %d (%s) @ %7.1f Hz: %+7.2f dB\n", e.chain + 1, patch::chains[e.chain].name, e.f, db);
		CHECK (db >= e.minDb && db <= e.maxDb, "%s: %.2f dB not in [%.1f, %.1f]", e.what, db, e.minDb, e.maxDb);
	}
}

static void testDADSRH() {
	printf ("-- DADSRH timing (chain 1 WT: A=1.854s D=0.510s S=48.7%% H=1.099s R=4.908s) --\n");
	DADSRH env;
	auto& c = patch::chains[0].env;
	env.attackKnob = c.atk; env.decayKnob = c.dec; env.sustainKnob = c.sus;
	env.releaseKnob = c.rel; env.holdKnob = c.hold; env.delayKnob = c.del;
	env.attackShape = c.shapeA; env.decayShape = c.shapeD; env.releaseShape = c.shapeR;
	env.setSampleRate (kSR);
	env.reset();

	// one 10 V trigger pulse (1 ms), then run
	int n = 0, nAtkEnd = -1, nRelStart = -1, nStop = -1;
	float peak = 0.0f;
	const int maxN = (int) (12.0f * kSR);
	DADSRH::Stage prev = DADSRH::STOPPED_STAGE;
	for (; n < maxN; ++n) {
		float trig = (n < (int) (0.001f * kSR)) ? 10.0f : 0.0f;
		float v = env.step (trig);
		peak = std::max (peak, v);
		if (prev == DADSRH::ATTACK_STAGE && env._stage == DADSRH::DECAY_STAGE && nAtkEnd < 0) nAtkEnd = n;
		if (env._stage == DADSRH::RELEASE_STAGE && nRelStart < 0) nRelStart = n;
		if (prev == DADSRH::RELEASE_STAGE && env._stage == DADSRH::STOPPED_STAGE && nStop < 0) { nStop = n; break; }
		prev = env._stage;
	}
	float tHoldEnd = nRelStart / kSR, tRel = (nStop - nRelStart) / kSR;
	float expAtk = c.atk * c.atk * 10.0f;
	float expHold = c.hold * c.hold * 10.0f;
	float expRel = c.rel * c.rel * 10.0f;
	// WT chain: hold (1.099 s) expires BEFORE the 1.854 s attack completes -> envelope
	// peaks at linear-attack progress hold/attack = 0.593 (5.93 V) and goes to release.
	float expPeak = 10.0f * (expHold / expAtk);
	printf ("   release starts %.3fs (exp hold %.3f) | peak %.2f V (exp %.2f) | release len %.3fs (exp < %.3f)\n",
	        tHoldEnd, expHold, peak, expPeak, tRel, expRel);
	CHECK (nAtkEnd < 0, "attack must NOT complete (hold < attack)");
	CHECK (fabsf (tHoldEnd - expHold) < 0.05f, "hold end -> release start");
	CHECK (fabsf (peak - expPeak) < 0.15f, "peak = attack progress at hold expiry");
	CHECK (tRel < expRel, "exp release terminates at 0.1%% early");
	CHECK (tRel > 0.3f * expRel, "release not absurdly short");
}

static void testVCA() {
	printf ("-- VCA dB law --\n");
	VCAmp vca; vca.levelKnob = 0.833333f; vca.setSampleRate (kSR);
	// settle slew (5 ms), then measure gain on a 1 kHz tone with env = 10 V and 5 V
	for (float cv : { 10.0f, 5.0f }) {
		double sumIn = 0, sumOut = 0; double ph = 0, w = 2.0 * M_PI * 1000.0 / kSR;
		int settle = (int) (0.1f * kSR), meas = (int) (0.2f * kSR);
		for (int n = 0; n < settle + meas; ++n) {
			float x = 1.0f * (float) sin (ph); ph += w;
			float y = vca.process (x, cv);
			if (n >= settle) { sumIn += (double) x * x; sumOut += (double) y * y; }
		}
		float db = 20.0f * (float) log10 (sqrt (sumOut / sumIn));
		float expDb = (0.833333f * cv / 10.0f) * 72.0f - 60.0f;
		printf ("   env %4.1f V: %+7.2f dB (exp %+7.2f)\n", cv, db, expDb);
		CHECK (fabsf (db - expDb) < 0.2f, "VCA gain at cv=%.0f", cv);
	}
}

static void testChainTrigger() {
	printf ("-- full chain trigger on tone burst (chain 3 Palm: 1400 Hz band) --\n");
	V2Engine eng;
	eng.prepare (kSR);
	// 1400 Hz tone burst, 0.5 s on, 1.5 s off; amplitude 2.5 V (≈ -6 dBFS field-recording-ish)
	int on = (int) (0.5f * kSR), total = (int) (2.0f * kSR);
	double ph = 0, w = 2.0 * M_PI * 1400.0 / kSR;
	bool triggered = false; float maxEnv = 0; int trigSample = -1;
	for (int n = 0; n < total; ++n) {
		float x = (n < on) ? 2.5f * (float) sin (ph) : 0.0f;
		ph += w;
		eng.processSample (x);           // pitch gates bypassed (10 V) in 4.1
		auto& c = eng.chain (3);
		if (c.trigGate > 5.0f && !triggered) { triggered = true; trigSample = n; }
		maxEnv = std::max (maxEnv, c.envOut);
	}
	printf ("   triggered at %.1f ms after burst start; env peak %.2f V\n",
	        trigSample >= 0 ? 1000.0f * trigSample / kSR : -1.0f, maxEnv);
	CHECK (triggered, "chain 4 triggers on in-band burst");
	CHECK (trigSample >= (int) (0.010f * kSR), "debounce >= 10 ms");
	CHECK (maxEnv > 5.0f, "envelope developed");

	// out-of-band burst must NOT trigger chain 4 (5.5 kHz tone)
	V2Engine eng2; eng2.prepare (kSR);
	ph = 0; w = 2.0 * M_PI * 5500.0 / kSR;
	bool falseTrig = false;
	for (int n = 0; n < total; ++n) {
		float x = (n < on) ? 2.5f * (float) sin (ph) : 0.0f; ph += w;
		eng2.processSample (x);
		if (eng2.chain (3).trigGate > 5.0f) falseTrig = true;
	}
	CHECK (!falseTrig, "chain 4 does not trigger on 5.5 kHz (out of band)");
	printf ("   out-of-band burst: %s\n", falseTrig ? "TRIGGERED (bad)" : "no trigger (good)");
}

int main (int argc, char** argv) {
	if (argc > 1) kSR = (float) atof (argv[1]);
	printf ("GEMS v2 chunk 4.1 verification @ %g Hz\n", kSR);
	testFilters();
	testDADSRH();
	testVCA();
	testChainTrigger();
	printf (failures ? "\n%d FAILURE(S)\n" : "\nALL PASS\n", failures);
	return failures ? 1 : 0;
}

// GEMS v2 — chunk 4.5 verification: SurgeXT Wavetable voice (chain 1) + Hiss layer.
//   Frame-0 sine, note60=C4 -> f = 261.626*2^cv (narrow-band ACF around expected pitch).
//   saturate raises harmonic content (crest/odd-harmonic change); formant shifts spectrum up.
//   Hiss adds a faint (-42 dB) floor; gated integration through the engine.

#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <vector>
#include "V2Engine.h"
#include "SurgeWTVoice.h"

using namespace gemsv2;
static int failures = 0;
#define CHECK(cond, ...) do { if (!(cond)) { ++failures; printf("FAIL: " __VA_ARGS__); printf("\n"); } } while (0)
static float kSR = 48000.0f;

static float measureHzNear (const std::vector<float>& v, float expHz) {
	int n = (int) v.size(), start = n / 2, len = n - start;
	const float* x = v.data() + start;
	int loLag = std::max (2, (int) (kSR / (expHz * 1.5f))), hiLag = std::min (len - 2, (int) (kSR / (expHz / 1.5f)));
	double best = -1e30; int bestLag = loLag;
	for (int lag = loLag; lag <= hiLag; ++lag) {
		double acc = 0, e0 = 0, e1 = 0;
		for (int i = 0; i + lag < len; ++i) { acc += (double) x[i]*x[i+lag]; e0 += (double) x[i]*x[i]; e1 += (double) x[i+lag]*x[i+lag]; }
		double r = (e0>0&&e1>0)? acc/std::sqrt(e0*e1):0.0; if (r>best){best=r;bestLag=lag;}
	}
	return kSR / (float) bestLag;
}
static double rmsBack (const std::vector<float>& v) { double s=0; int n=(int)v.size(); for (int i=n/2;i<n;++i) s+=(double)v[(size_t)i]*v[(size_t)i]; return std::sqrt(s/(n-n/2)); }
static float crest (const std::vector<float>& v) { double pk=0,s=0; int n=(int)v.size(); for (int i=n/2;i<n;++i){ float a=std::fabs(v[(size_t)i]); pk=std::max(pk,(double)a); s+=(double)v[(size_t)i]*v[(size_t)i]; } double r=std::sqrt(s/(n-n/2)); return r>0? (float)(pk/r):0; }

static void unitWTpitch (float cv) {
	SurgeWTVoice w; w.setSampleRate (kSR); w.reset();
	std::vector<float> buf ((size_t) (2.0f * kSR));
	for (size_t i = 0; i < buf.size(); ++i) buf[i] = w.process (cv, 0.107f, 0.0f);
	float exp = 261.62556f * std::exp2 (cv), f = measureHzNear (buf, exp);
	printf ("   WT cv %+5.2f -> %7.2f Hz (exp %7.2f) rms %.3f\n", cv, f, exp, rmsBack (buf));
	CHECK (std::fabs (f - exp) < exp * 0.03f, "WT pitch %.2f (exp %.1f got %.1f)", cv, exp, f);
	CHECK (rmsBack (buf) > 1.0f, "WT audible (rms %.2f)", rmsBack (buf));
}

static void unitWTshape() {
	// frame 0 is a sine -> low saturate ~ sine crest (~1.41); heavy saturate raises crest/harmonics
	SurgeWTVoice w; w.setSampleRate (kSR); w.reset();
	std::vector<float> lo ((size_t)(kSR)), hi ((size_t)(kSR)), fm ((size_t)(kSR));
	for (size_t i=0;i<lo.size();++i) lo[i]=w.process(0.0f,0.05f,0.0f);
	w.reset(); for (size_t i=0;i<hi.size();++i) hi[i]=w.process(0.0f,0.9f,0.0f);
	w.reset(); for (size_t i=0;i<fm.size();++i) fm[i]=w.process(0.0f,0.107f,0.6f);
	float cLo=crest(lo), cHi=crest(hi); double rFm=rmsBack(fm), rLo=rmsBack(lo);
	printf ("   saturate: crest lo(0.05)=%.2f hi(0.9)=%.2f | formant 0.6 rms=%.2f vs base=%.2f\n", cLo, cHi, rFm, rLo);
	// Surge distort_level: clip=-8*sat^3 -> heavy saturate applies 6.83x-5.83x^3 then clamps,
	// squaring the sine (crest -> ~1.0, i.e. drops toward a square; adds odd harmonics/brightness).
	CHECK (std::fabs (cLo - 1.414f) < 0.25f, "low-saturate ~ sine crest (%.2f)", cLo);
	CHECK (cHi < cLo - 0.2f && cHi < 1.25f, "high saturate squares the sine (crest %.2f -> ~1.0)", cHi);
	CHECK (rFm > 0.5f, "formant-shifted still audible (%.2f)", rFm);
}

static float richTone (double& ph, float hz, int n, float amp, unsigned& s) {
	float vib = 1.0f + 0.015f * (float) sin (2.0 * M_PI * 6.3 * n / kSR);
	ph += 2.0 * M_PI * hz * vib / kSR; s = s*1664525u+1013904223u;
	return amp*(float)(sin(ph)+0.4*sin(2*ph)) + (((float)(s>>8)/8388608.0f)-1.0f)*0.1f*amp;
}
static void integ() {
	V2Engine eng; eng.prepare (kSR);
	double ph=0; unsigned s=5u; double rmsPre=0; int preN=(int)(0.3f*kSR),pc=0;
	for (int i=0;i<preN;++i){ eng.processSample(0.0f); if (i>preN-(int)(0.1f*kSR)){ float v=eng.voiceOut(V2Engine::kWT); rmsPre+=v*v; ++pc; } }
	rmsPre=std::sqrt(rmsPre/pc);
	int toneN=(int)(1.5f*kSR),w=(int)(0.1f*kSR); std::vector<float> vb((size_t)toneN); double acc=0,maxRms=0;
	for (int i=0;i<toneN;++i){ eng.processSample(richTone(ph,180.0f,i,2.5f,s)); vb[(size_t)i]=eng.voiceOut(V2Engine::kWT); }
	for (int i=0;i<toneN;++i){ acc+=(double)vb[(size_t)i]*vb[(size_t)i]; if(i>=w)acc-=(double)vb[(size_t)(i-w)]*vb[(size_t)(i-w)]; if(i>=w)maxRms=std::max(maxRms,std::sqrt(acc/w)); }
	printf ("   integ (180 Hz -> WT chain): pre %.4f, loudest100ms %.3f V\n", rmsPre, maxRms);
	// pre isn't perfectly silent: the -42 dB Hiss floor is always present under the (closed) VCA.
	CHECK (rmsPre < 0.05, "WT chain near-silent before trigger (only Hiss floor) (%.4f)", rmsPre);
	CHECK (maxRms > 0.05, "WT chain audible after trigger (%.3f)", maxRms);
	CHECK (maxRms > rmsPre * 10.0, "WT gating contrast");
}

int main (int argc, char** argv) {
	if (argc > 1) kSR = (float) atof (argv[1]);
	printf ("GEMS v2 chunk 4.5 verification @ %g Hz\n", kSR);
	printf ("-- WT pitch law (frame-0 sine) --\n");
	for (float cv : { -1.0f, 0.0f, 1.0f }) unitWTpitch (cv);
	printf ("-- WT saturate/formant response --\n");
	unitWTshape();
	printf ("-- gated integration (chain 1 = WT + Hiss) --\n");
	integ();
	printf (failures ? "\n%d FAILURE(S)\n" : "\nALL PASS\n", failures);
	return failures ? 1 : 0;
}

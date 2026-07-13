// Full-engine stability: run all 7 chains with a broadband field-recording-like input
// for 10 s at each rate; confirm every voiceOut stays finite and bounded (FX feedback OK).
#include <cstdio>
#include <cmath>
#include <cstdlib>
#include "V2Engine.h"
using namespace gemsv2;
int main (int argc, char** argv) {
	float sr = argc > 1 ? atof (argv[1]) : 48000.0f;
	V2Engine eng; eng.prepare (sr);
	unsigned s = 12345u; double ph1=0, ph2=0;
	int N = (int) (10.0f * sr);
	float peak[7] = {0}; int bad = 0;
	for (int i = 0; i < N; ++i) {
		// mix of two swept tones + noise, ±3 V (drives all bands over time)
		ph1 += 2*M_PI*(200.0 + 180.0*sin(2*M_PI*0.05*i/sr))/sr;
		ph2 += 2*M_PI*(3000.0 + 2500.0*sin(2*M_PI*0.07*i/sr))/sr;
		s = s*1664525u+1013904223u;
		float noise = (((float)(s>>8)/8388608.0f)-1.0f);
		float in = 1.2f*(float)sin(ph1) + 1.0f*(float)sin(ph2) + 0.6f*noise;
		eng.processSample (in);
		for (int c = 0; c < 7; ++c) {
			float v = eng.voiceOut (c);
			if (!std::isfinite (v)) ++bad;
			peak[c] = std::max (peak[c], std::fabs (v));
		}
	}
	printf ("@ %g Hz, 10 s: nonfinite=%d\n", sr, bad);
	const char* nm[7] = {"WT","Ouros","Osculum","Palm","FMOp","Plaits","Braids"};
	for (int c = 0; c < 7; ++c) printf ("   %-8s peak %.2f V\n", nm[c], peak[c]);
	bool ok = bad == 0;
	for (int c = 0; c < 7; ++c) if (peak[c] > 50.0f || peak[c] < 1e-4f) ok = false;
	printf ("%s\n", ok ? "STABLE" : "CHECK");
	return ok ? 0 : 1;
}

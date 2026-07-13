// GEMS v2 — chunk 4.10 verification: master bus (Mix8 → Pressor → Plateau).
// Drive the full engine with a broadband input; confirm the stereo master output is
// non-silent during activity, finite/bounded, stereo (L≠R from pans), and has a reverb
// tail after input stops.
#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <vector>
#include "V2Engine.h"
using namespace gemsv2;
static int failures = 0;
#define CHECK(c, ...) do { if(!(c)){++failures; printf("FAIL: " __VA_ARGS__); printf("\n");} } while(0)

int main (int argc, char** argv) {
	float sr = argc > 1 ? atof (argv[1]) : 48000.0f;
	V2Engine eng; eng.prepare (sr);
	unsigned s = 999u; double p1=0, p2=0;
	int N = (int) (12.0f * sr), onN = (int) (6.0f * sr);
	double duringL=0, duringR=0, tailL=0, stereoDiff=0; int bad=0; float peak=0;
	for (int i = 0; i < N; ++i) {
		float in = 0.0f;
		if (i < onN) {
			p1 += 2*M_PI*(200.0 + 150.0*sin(2*M_PI*0.1*i/sr))/sr;
			p2 += 2*M_PI*(2500.0 + 2000.0*sin(2*M_PI*0.13*i/sr))/sr;
			s = s*1664525u+1013904223u;
			in = 1.2f*(float)sin(p1) + 1.0f*(float)sin(p2) + 0.5f*((((float)(s>>8))/8388608.0f)-1.0f);
		}
		eng.processSample (in);
		float l = eng.masterL(), r = eng.masterR();
		if (!std::isfinite(l) || !std::isfinite(r)) ++bad;
		peak = std::max (peak, std::max (std::fabs(l), std::fabs(r)));
		if (i >= onN/2 && i < onN) { duringL += (double)l*l; duringR += (double)r*r; stereoDiff += (double)(l-r)*(l-r); }
		if (i >= onN + (int)(0.5f*sr)) tailL += (double)l*l;   // >0.5 s after input stops
	}
	printf ("@ %g Hz: master peak %.2f V, rms L %.3f R %.3f, stereo-diff %.2f, reverb tail %.4f, nonfinite %d\n",
	        sr, peak, std::sqrt(duringL/(onN/2)), std::sqrt(duringR/(onN/2)), std::sqrt(stereoDiff/(onN/2)), tailL, bad);
	CHECK (bad == 0, "master finite");
	CHECK (peak > 0.1f, "master audible");
	CHECK (peak <= 10.01f, "master bounded (Plateau clamps ±10 V)");
	CHECK (std::sqrt(duringL/(onN/2)) > 0.05, "L non-silent");
	CHECK (std::sqrt(stereoDiff/(onN/2)) > 0.01, "stereo (pans differ L/R)");
	CHECK (tailL > 1e-4, "reverb tail after input stops");
	printf (failures ? "\n%d FAILURE(S)\n" : "\nALL PASS\n", failures);
	return failures ? 1 : 0;
}

#pragma once

// GEMS v2 — the raw stored knob values that configure the engine's modules.
// Ports consume knob units.

namespace gemsv2 {
namespace patch {

struct VCFConst  { float freq, q; int mode; float slope; };
struct FollowConst { float response, gain; };
struct OffsetConst { float offset, scale; };
struct CmpConst  { float a, b, window, lag; };
struct DADSRHConst { float del, atk, dec, sus, rel, hold; int shapeA, shapeD, shapeR; };
struct PitchConst {
	float sensitivity, confidence;      // AudioToCVPitch params (%, tolerance = 0 in all instances)
	float slewShape, slewRise, slewFall; // Befaco SlewLimiter knobs
	bool  notes[12];                     // Quack scene 0 (C..B)
	int   transposeOct;                  // Quack transpose (mode 0 = octaves)
};

struct ChainConst {
	const char* name;
	VCFConst  vcf;          // band filter (or first of the HP→LP cascade)
	bool      hasSecondVcf;
	VCFConst  vcf2;         // Ouros chain only (LP after HP)
	FollowConst follow;
	OffsetConst offset;
	CmpConst  cmp;
	bool      hasPitchGate; // false = Braids chain (Cmp triggers DADSRH directly)
	DADSRHConst env;
	float     vcaLevel;     // VCAmp level knob
};

// Chains ordered by ascending band frequency (spec §3).
static constexpr ChainConst chains[7] = {
	// 1 — 240 Hz LP → SurgeXT Wavetable + Hiss     (VCF#6950 …)
	{ "WT",      { 0.109545f, 0.15f,     0, 0.904534f }, false, {},
	             { 0.7f, 1.0f }, { 0.01f, 1.0f },      { 0.08f, 0.18f,  0.02f,  0.1f },      true,
	             { 0.0f, 0.430577f, 0.225869f, 0.486747f, 0.700568f, 0.331547f, 2, 1, 1 }, 0.833333f },
	// 2 — 440 Hz HP → 1100 Hz LP → Ouros           (VCF#0533 → VCF#9836 …)
	{ "Ouros",   { 0.148324f, 0.116867f, 1, 0.904534f }, true,
	             { 0.234521f, 0.115663f, 0, 0.904534f },
	             { 0.7f, 1.0f }, { 0.01f, 1.0f },      { 0.18f, 0.27f,  0.02f,  0.1f },      true,
	             { 0.0f, 0.0149141f, 0.353579f, 0.0638554f, 0.339759f, 0.290583f, 2, 1, 1 }, 0.5f },
	// 3 — 500 Hz BP → Osculum → LVCF → Elastika    (VCF#5996 …)
	{ "Osculum", { 0.158114f, 0.595781f, 2, 0.904534f }, false, {},
	             { 0.7f, 1.0f }, { 0.01f, 0.774597f }, { 0.18f, 0.23f,  0.02f,  0.1f },      true,
	             { 0.0f, 0.142625f, 0.239122f, 0.651808f, 0.4837f, 0.31227f, 2, 1, 1 }, 0.833333f },
	// 4 — 1400 Hz BP → PalmLoop → VCF → Neil       (VCF#0519 …)
	{ "Palm",    { 0.264575f, 0.428312f, 2, 0.904534f }, false, {},
	             { 0.7f, 1.0f }, { 0.01f, 0.894427f }, { 0.18f, 0.2f,   0.02f,  0.1f },      true,
	             { 0.0f, 0.124553f, 0.301772f, 0.10241f, 0.41623f, 0.283355f, 1, 1, 1 }, 0.708333f },
	// 5 — 2800 Hz BP → FMOp → Galaxy → Gravy       (VCF#2433 …)
	{ "FMOp",    { 0.374166f, 0.3f,      2, 0.904534f }, false, {},
	             { 0.6f, 1.0f }, { 0.01f, 0.894427f }, { 0.15f, 0.26f,  0.025f, 0.1f },      true,
	             { 0.00240964f, 0.10648f, 0.263218f, 0.586746f, 0.41623f, 0.201427f, 1, 2, 1 }, 0.833333f },
	// 6 — 5500 Hz BP → Plaits → VCF → Tape         (VCF#1681 …)
	{ "Plaits",  { 0.524404f, 0.2f,      2, 0.904534f }, false, {},
	             { 0.4f, 1.0f }, { 0.2f,  0.707107f }, { 0.12f, 0.3f,   0.005f, 0.141421f }, true,
	             { 0.0f, 0.0f, 0.349965f, 0.0614458f, 0.443373f, 0.0060241f, 1, 2, 1 }, 0.75f },
	// 7 — 8000 Hz HP → Braids → Echo               (VCF#5965 …)
	{ "Braids",  { 0.632456f, 0.272289f, 1, 0.904534f }, false, {},
	             { 0.7f, 1.0f }, { 0.01f, 1.0f },      { 0.08f, 0.146f, 0.05f,  0.1f },      false,
	             { 0.0f, 0.378769f, 0.659604f, 0.826506f, 0.880086f, 0.525523f, 2, 2, 2 }, 0.833333f },
};

// Osculum chain second envelope (DADSRH#9786, triggered via Branches#1623 OUT2A — chunk 4.9)
static constexpr DADSRHConst osculumEnv2 = { 0.0f, 0.140215f, 0.327073f, 0.0f, 0.109001f, 0.233735f, 2, 1, 1 };
static constexpr float osculumVca2Level = 0.833333f;   // VCAmp#2571

// Pitch spine per chain (AudioToCVPitch → SlewLimiter → Quack); chain 7 (Braids) has none.
// Note masks are Quack scene 0 from the patch json. All tolerance params are 0.
#define GEMS_CMAJ { true,false,true,false,true,true,false,true,false,true,false,true }
static constexpr PitchConst pitch[6] = {
	// 1 WT      (A2CV#0406, Slew#6894, Quack#7279 {C,E,G})
	{ 81.3513f, 86.0f,    0.414458f,  0.01f, 0.02f, { true,false,false,false,true,false,false,true,false,false,false,false }, 0 },
	// 2 Ouros   (A2CV#1362, Slew#6624 shape 0, Quack#1966 Cmaj −1 oct)
	{ 94.0105f, 92.0f,    0.0f,       0.01f, 0.02f, GEMS_CMAJ, -1 },
	// 3 Osculum (A2CV#0113, Slew#0570, Quack#5012 {C,D,E,A,B})
	{ 88.121f,  88.0299f, 0.414458f,  0.01f, 0.02f, { true,false,true,false,true,false,false,false,false,true,false,true }, 0 },
	// 4 Palm    (A2CV#0646, Slew#1380, Quack#9649 {C,E,F,G,A})
	{ 89.5158f, 84.03f,   0.414458f,  0.01f, 0.02f, { true,false,false,false,true,true,false,true,false,true,false,false }, 0 },
	// 5 FMOp    (A2CV#7479, Slew#9310 shape 0.018, Quack#9084 Cmaj −3 oct)
	{ 90.6478f, 89.099f,  0.0180723f, 0.01f, 0.02f, GEMS_CMAJ, -3 },
	// 6 Plaits  (A2CV#0428, Slew#7657, Quack#9696 Cmaj −1 oct)
	{ 88.4504f, 89.0f,    0.414458f,  0.01f, 0.02f, GEMS_CMAJ, -1 },
};
#undef GEMS_CMAJ

} // namespace patch
} // namespace gemsv2

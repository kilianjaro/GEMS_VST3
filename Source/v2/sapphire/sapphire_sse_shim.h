#pragma once
// GEMS v2 — scalar emulation of the 6 SSE intrinsics Sapphire's PhysicsVector uses, so the
// vendored Sapphire mesh DSP compiles on arm64 without vendoring all of simde. PhysicsVector
// packs a 3-D vector in the low 3 lanes of a 4-float register; these are exact scalar
// equivalents of the x86 _mm_*_ps ops (element-wise). See ../NOTICE.md.

typedef struct { float f[4]; } __m128;

static inline __m128 _mm_set1_ps (float x) { __m128 r; r.f[0]=r.f[1]=r.f[2]=r.f[3]=x; return r; }
static inline __m128 _mm_setr_ps (float a, float b, float c, float d) { __m128 r; r.f[0]=a; r.f[1]=b; r.f[2]=c; r.f[3]=d; return r; }
static inline __m128 _mm_add_ps (__m128 a, __m128 b) { __m128 r; for (int i=0;i<4;++i) r.f[i]=a.f[i]+b.f[i]; return r; }
static inline __m128 _mm_sub_ps (__m128 a, __m128 b) { __m128 r; for (int i=0;i<4;++i) r.f[i]=a.f[i]-b.f[i]; return r; }
static inline __m128 _mm_mul_ps (__m128 a, __m128 b) { __m128 r; for (int i=0;i<4;++i) r.f[i]=a.f[i]*b.f[i]; return r; }
static inline __m128 _mm_div_ps (__m128 a, __m128 b) { __m128 r; for (int i=0;i<4;++i) r.f[i]=a.f[i]/b.f[i]; return r; }

# GEMS v2 speexdsp vendoring — build notes

## Origin

- Upstream: https://github.com/xiph/speexdsp (GitHub mirror of
  `https://gitlab.xiph.org/xiph/speexdsp.git`, which is the exact submodule
  URL pinned by VCV Rack's `dep/speexdsp`).
- Commit vendored: `27ddf98b627ab425d5337b1a0d8b80d7528f9b30` (2022-07-18) —
  this is the exact SHA pinned by Rack's `dep/speexdsp` submodule at the time
  of vendoring (`git ls-tree HEAD dep/speexdsp` in
  https://github.com/VCVRack/Rack @ v2, commit
  `061ccf63c1758599396ac1bb10d47345d9d34076`).
- License: BSD (3-clause, Xiph.org style — see header of each vendored file).
- Files vendored (flattened, no `libspeexdsp/`/`include/speex/` subdirs):
  - `resample.c` (from `libspeexdsp/resample.c`)
  - `arch.h` (from `libspeexdsp/arch.h`)
  - `speex_resampler.h` (from `include/speex/speex_resampler.h`)
- `resample_sse.h` was **not** vendored: it is only pulled in when `USE_SSE`
  is defined (see `resample.c`), and GEMS's build never defines `USE_SSE`, so
  the file would be dead weight. If GEMS later wants an SSE-accelerated path
  on x86_64, re-vendor `libspeexdsp/resample_sse.h` from the same commit and
  define `USE_SSE`.
- `os_support.h` / `speex/speexdsp_types.h` / `config.h` are **not** needed —
  see "Standalone build defines" below; the `OUTSIDE_SPEEX` path in
  `resample.c` / `arch.h` / `speex_resampler.h` bypasses all three.

## Standalone build defines

`resample.c` supports being compiled as a single translation unit "outside"
the full speex/speexdsp autotools build via an `OUTSIDE_SPEEX` code path
(this is a first-class, intentionally-supported mode in the upstream source,
not a hack — see the `#ifdef OUTSIDE_SPEEX` blocks in `resample.c`,
`arch.h`, and `speex_resampler.h`).

GEMS checked how VCV Rack itself builds speexdsp (`Rack/dep/Makefile`,
`Rack/dep.mk`) to see what defines Rack passes. Rack's build turned out to
run the **full autotools build** (`./autogen.sh && ./configure && make &&
make install`) with no project-specific `CFLAGS`/defines beyond the default
`./configure --prefix=...` — i.e. Rack does not use the `OUTSIDE_SPEEX`
single-file mode at all; it builds and links the complete `libspeexdsp.a`
and its own generated `config.h`.

Since GEMS vendors only `resample.c` (no autotools, no generated
`config.h`), we fall back to the documented standalone-embedding defines
(as anticipated in the vendoring task spec), applied as a `// GEMS:`-marked
preamble at the top of `resample.c` itself (so the file compiles with a
plain `clang -c resample.c -I<this dir>` and needs no special `-D` flags on
the command line):

```c
#ifndef OUTSIDE_SPEEX
#define OUTSIDE_SPEEX
#endif
#ifndef RANDOM_PREFIX
#define RANDOM_PREFIX speex
#endif
#ifndef FLOATING_POINT
#define FLOATING_POINT
#endif
#ifndef EXPORT
#define EXPORT
#endif
```

- `OUTSIDE_SPEEX` — skips `speex/speexdsp_types.h` and `os_support.h`
  (neither is vendored); `arch.h`/`speex_resampler.h` self-provide
  `spx_int16_t`/`spx_int32_t`/`spx_uint32_t`/`spx_uint16_t` as plain
  `short`/`int`/`unsigned int`/`unsigned short`, and `resample.c`
  self-provides `speex_alloc`/`speex_realloc`/`speex_free` as thin
  `calloc`/`realloc`/`free` wrappers.
- `RANDOM_PREFIX speex` — required by `speex_resampler.h`'s `OUTSIDE_SPEEX`
  branch to rename the public `speex_resampler_*` symbols via
  `CAT_PREFIX(RANDOM_PREFIX, _resampler_init)` etc., to avoid clashing with a
  real libspeex if one is ever linked into the same binary. GEMS uses
  `speex` (matching the task's specified fallback value), which happens to
  be a no-op rename (`speex_resampler_init` → `speex_resampler_init`) since
  GEMS never links a real libspeex — the define is still required or
  `speex_resampler.h` raises `#error "Please define RANDOM_PREFIX..."`.
- `FLOATING_POINT` — selects the `float`-based (`spx_word16_t = float`) code
  path in `arch.h`/`resample.c` rather than the Q15 fixed-point path;
  matches Braids/Plaits DSP, which is all `float`.
- `EXPORT` (empty) — the vendored TU is compiled directly into the plugin
  binary (no shared-library boundary to annotate), so the export-visibility
  macro is a no-op.

Compile-tested as C: `clang -std=c99 -O2 -c resample.c -I<this dir>` —
succeeds with zero warnings and zero errors, producing a valid arm64
Mach-O object.

## Rack's `SampleRateConverter` — quality setting GEMS must match

Braids/Plaits wrappers in `AudibleInstruments` (`src/Braids.cpp`,
`src/Plaits.cpp`) resample their output through
`rack::dsp::SampleRateConverter<N>` (declared in
`Rack/include/dsp/resampler.hpp`). GEMS must reproduce the same resampling
quality to match upstream's audio behavior.

Fetched verbatim from
`https://raw.githubusercontent.com/VCVRack/Rack/v2/include/dsp/resampler.hpp`:

```cpp
/** Resamples by a fixed rational factor. */
template <int MAX_CHANNELS>
struct SampleRateConverter {
	SpeexResamplerState* st = NULL;
	int channels = MAX_CHANNELS;
	int quality = SPEEX_RESAMPLER_QUALITY_DEFAULT;
	int inRate = 44100;
	int outRate = 44100;
	...
	/** From 0 (worst, fastest) to 10 (best, slowest) */
	void setQuality(int quality) {
		if (quality == this->quality)
			return;
		this->quality = quality;
		refreshState();
	}
	...
};
```

**Finding: `SampleRateConverter` defaults its `quality` member to
`SPEEX_RESAMPLER_QUALITY_DEFAULT`, *not* `SPEEX_RESAMPLER_QUALITY_DESKTOP`.**
(`speex_resampler.h` defines `SPEEX_RESAMPLER_QUALITY_DEFAULT = 4` and
`SPEEX_RESAMPLER_QUALITY_DESKTOP = 5`.)

Checked whether the Braids/Plaits wrappers override this default via
`setQuality()`:

```
$ grep -n 'SampleRateConverter\|setQuality\|SPEEX_RESAMPLER_QUALITY\|setRates' src/Braids.cpp src/Plaits.cpp
src/Braids.cpp:36:	dsp::SampleRateConverter<1> src;
src/Braids.cpp:139:				src.setRates(96000, args.sampleRate);
src/Plaits.cpp:52:	dsp::SampleRateConverter<16 * 2> outputSrc;
src/Plaits.cpp:238:				outputSrc.setRates(48000, (int) args.sampleRate);
```

Neither wrapper calls `setQuality()` — only `setRates()`. **The Braids and
Plaits modules resample their internal-rate output (96 kHz for Braids,
48 kHz for Plaits) up/down to the host sample rate at speex quality level
`SPEEX_RESAMPLER_QUALITY_DEFAULT` (4), the class's untouched default.**

**GEMS must construct its resampler(s) with `quality = 4`
(`SPEEX_RESAMPLER_QUALITY_DEFAULT`), not 5 (`_DESKTOP`), to match upstream
AudibleInstruments' audio output.**

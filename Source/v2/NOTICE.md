# GEMS v2 vendored / ported DSP — origins & licenses

The GEMS synthesis engine vendors or ports module DSP verbatim from the original
open-source Eurorack / VCV module implementations listed below. Because most of these
sources are licensed **GPL-3.0**, GEMS is distributed under **GPL-3.0** (see `LICENSE`).

| Directory / file | Origin | Commit | License |
|---|---|---|---|
| `bogaudio/` | https://github.com/bogaudio/BogaudioModules (`src/dsp/…`) | 656eaae4 | GPL-3.0 |
| `BogModules.h` | ported module-level logic from BogaudioModules `src/*.cpp` (VCF, Follow, Offset, Cmp, DADSRH, VCAmp; Mix/Lmtr/Pressor/LFO/FMOp to follow) | 656eaae4 | GPL-3.0 |
| `pocketfft/` | https://github.com/mreineck/pocketfft (`cpp` branch) | — | BSD-3-Clause |
| `PitchSpine.h` | ported: Cardinal `AudioToCVPitch.cpp` (DISTRHO/Cardinal, GPL-3); aubio `pitchyinfast.c`/`pitch.c`/`mathutils.c` (aubio.org, GPL-3, FFT backend replaced by pocketfft implementing aubio's halfcomplex API); Befaco `SlewLimiter.cpp` (VCVRack/Befaco, GPL-3); Aria Salvatrice `Qqqq.cpp`/`quantizer.hpp` (AriaModules, GPL-3) | — | GPL-3.0 |
| `BogVoices.h` | ported FMOp + LVCF module logic from BogaudioModules `src/FMOp.cpp/.hpp`, `src/LVCF.cpp/.hpp` (uses vendored `bogaudio/oscillator`, `resample`, `noise`, `experiments`, `envelope`) | 656eaae4 | GPL-3.0 |
| `SimpleVoices.h` | ported Osculum (sonusmodular `src/osculum.cpp`, GitLab) + PalmLoop (21kHz `src/PalmLoop.cpp` + `dsp/math.hpp`) | — | GPL-3.0 |
| `OurosVoice.h` | ported CVfunk `src/Ouros.cpp` (mono channel, patch-wired inputs; `rack::simd::sin`→`std::sin`) | — | GPL-3.0 |
| `WavetableData.h/.cpp` | "Sine PD HQ" wavetable (32×512 int16); decode verified vs surge-rack + surge core | — | data |
| `MutableVoices.h` | Braids + Plaits wrapper voices (drive the vendored eurorack DSP as AudibleInstruments' `src/Braids.cpp`/`Plaits.cpp` do, incl. speex-based output SRC at quality 4) | 1f279a0 | GPL-3.0 (wrapper) |
| `SurgeWTVoice.h` | SurgeXT Wavetable osc, config-specialized port (frame-0 sine; exact Surge `distort_level` saturate + display-path formant from `WavetableOscillator.cpp`; BLIT→direct-read simplification, transparent for a bass sine — see file header + progress.md 4.5) | surge main | GPL-3.0 |
| `eurorack/` (stmlib, braids, plaits) | https://github.com/VCVRack/eurorack (submodule of AudibleInstruments) — DSP-only subtrees for Braids + Plaits; hardware/UI/driver files excluded; +braids `vco_jitter_source.h`/`signature_waveshaper.h` (no-ops at drift/signature=0) | (see git ls-tree of AudibleInstruments@1f279a0 `eurorack`) | **MIT** |
| `speex/` | https://github.com/xiph/speexdsp `libspeexdsp/resample.c` + headers (for Braids/Plaits output SRC, matching rack::dsp::SampleRateConverter) — see `speex/GEMS_BUILD_NOTES.md` | 27ddf98b | BSD-3 |
| `FxSurgeDelay.h` | SurgeXT Delay (chain 2): faithful port of sst-effects Dual Delay (surge-synthesizer/sst-effects `include/sst/effects/Delay.h`) — cross-feed stereo delay, hardclip feedback, LP2B/HP filters; sinc-FIR read → cubic (transparent for ~1 s delay) | sst-effects main | GPL-3.0 |
| `FxTapeGravy.h` | rackwindows Tape (Airwindows Tape port, chain 6) + Sapphire Gravy (SVF filter, chain 5) | rackwindows master; Sapphire d884e795 | GPL-3.0 |
| `FxEcho.h` | Sapphire Echo + EchoOut (chain 7): faithful-topology tape-loop echo (1 s delay, filtered feedback, DC reject, global mix/level). REVERSE handling flagged — see file header | Sapphire d884e795 | GPL-3.0 |
| `sapphire/galaxy_engine.hpp` + `FxGalaxy.h` | Sapphire Galaxy reverb (chain 5): engine vendored VERBATIM (Airwindows Galactic adaptation), wrapped by FxGalaxy.h; `sapphire_shim.hpp` supplies Square/Cube/TwoToPower so it builds without the full sapphire_engine.hpp | Sapphire d884e795 | GPL-3.0 |
| `sapphire/` (elastika_engine/mesh, mesh_physics, sapphire_engine, sapphire_simd + `sapphire_sse_shim.h`) + `FxElastika.h` | Sapphire Elastika mass-spring mesh (chain 3), vendored verbatim; arm64 `__m128` scalar shim replaces simde | Sapphire d884e795 | GPL-3.0 |
| `clouds/` (parasites clouds dsp + mqtthiqs stmlib) + `NeilVoice.h` | Neil = Clouds "Parasite" granular (chain 4), vendored like eurorack; wrapper mirrors ArableInstruments Clouds.cpp, reuses SpeexSRC (32k↔host). Note: shares the eurorack Mutable stmlib `units`/`random` objects at link (same tables) — clouds-only `atan` kept | Clouds parasite (Puech) via CardinalModules; mqtthiqs/stmlib | MIT (DSP) / GPL-3 (wrapper) |
| `SpeexSRC.h` | shared speex SRC + FIFO (factored out of MutableVoices.h; used by Braids/Plaits/Neil) | — | see speex row |
| `GenerativeModules.h` | Bogaudio LFO/Walk2 (656eaae4), HetrickCV Dust, AudibleInstruments Branches — generative/mod sources; seeded PRNG (distribution-match per plan.md §3) | various | GPL-3.0 |
| `valley/` (Dattorro, AllpassFilter, InterpDelay, OnePoleFilters, LFO, Utilities) | https://github.com/ValleyAudio/ValleyRackFree `src/Plateau/Dattorro.*` + dsp helpers — plate reverb engine for Plateau (master bus); include-path flattened | ValleyRackFree v2.x | GPL-3.0 |
| `MasterBus.h` | Mix8 → Pressor → Plateau master bus (chunk 4.10): Bogaudio Mix8/Pressor logic (level dB=knob·66−60, constant-power pan, RMS-detector comp) via vendored `bogaudio/signal.hpp`; Valley Dattorro (Plateau) reverb. | 656eaae4 / ValleyRackFree | GPL-3.0 (wrapper) |

Local modifications are marked with `// GEMS:` comments and limited to:
- include-path flattening,
- `window_min.hpp` (extracted Window classes so `table.cpp` doesn't pull the FFT analyzer),
- replacement of `rack::` Param/Input/Output plumbing with plain floats in ported module logic.

Pinned upstream versions are used where later releases changed behaviour
(e.g. Sapphire @ d884e795).

## UI typefaces (Source/GemsTypeData.h)

The v2 UI embeds two typefaces, both licensed under the **SIL Open Font
License 1.1** (embedding in commercial software is permitted; the fonts may
not be sold on their own; this notice must ship with the product):

- **Space Grotesk** — Copyright 2020 The Space Grotesk Project Authors,
  https://github.com/floriankarsten/space-grotesk
- **Space Mono** — Copyright 2016 The Space Mono Project Authors,
  https://github.com/googlefonts/spacemono

Full license text: https://openfontlicense.org

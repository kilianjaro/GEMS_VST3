# GEMS

> A generative ecoacoustic instrument. Field recordings become their own generative echo — *the ecosystem as composer*.

GEMS is a VST3 audio plugin (with a macOS standalone) that listens to environmental sound and re-synthesizes it in real time. It is the software instrument behind the **GEMS** sound-art works by [Totemphonia](https://totemphonia.com/en/gems/) — *"not a soundtrack about nature, but rather its generative echo."*

---

## The idea

Most "nature music" points a microphone at a landscape and calls it a recording. GEMS does the opposite: it treats the soundscape as a **score already being performed** and lets the ecosystem write the music.

Instead of imposing melody and rhythm onto a field recording, GEMS extracts the patterns that are *already there* — the pulse of a dawn chorus, the density of insects, the grain of wind through a canopy — and uses them to drive a synthesis engine. The result reflects the rhythm and harmony of a complex ecological soundscape without ever quoting it literally.

The existing GEMS pieces were built this way from recordings made in the Bolivian Amazon (*mono II*), the Colombian Tatacoa Desert (*tatacoa II*), and the Elbe Sandstone Mountains (*Sandstein ii*).

## How it works

GEMS is a JUCE 8 / VST3 plugin (Apple Silicon / arm64). Signal flow:

1. **Spectral decomposition** — the incoming audio is split into a set of frequency bands.
2. **Listening** — each band is continuously tracked for **loudness** and **pitch**. When a band becomes active, that energy is turned into a *trigger gesture* plus a pitch.
3. **Voicing** — those gestures drive **8 distinct synthesis voices**, each with its own timbral character, envelope behaviour, and dedicated effects chain (granular, wavetable, FM, physical-modelling, delay, tape, echo…).
4. **Master bus** — all voices are summed, compressed, and passed through a Dattorro plate reverb.
5. The **8 voice colours** are the identity of the instrument — visually and sonically.

The dry input is **not** passed through to the output. What you hear is the generated echo, not the recording that seeded it.

### Visualizations

The UI follows a "field laboratory" design — a bioacoustic measurement instrument rather than a synth faceplate. Three live views are driven purely by real signal data:

- a **spectrum analyzer** with phosphor-decay history and live capture markers,
- a time × pitch **event field** (an *ethogram* of detected impulses over a 6-second window),
- a **waveform** view showing the primary voice's amplitude envelope.

## How to use it

1. Install the plugin (see below) and open it in your DAW, or run the standalone.
2. Put GEMS as an **insert on a track that carries audio** — ideally a field recording — or route a live input into it in standalone mode.
3. Press play. GEMS analyzes the incoming signal and generates from it. Remember: **the output is the generative echo, not the input**, so you usually want GEMS on its own track/return rather than in series with material you want to keep dry.
4. **Feed it texture.** Dense, evolving, natural material works best — dawn choruses, insects, wind, water, rain. Sparse or highly rhythmic pop material gives sparser, less characteristic results.
5. **Shape the voices.** Each voice has its own knobs; click a voice to select it, or `cmd`/`shift`-click to select several at once and adjust them together. The voices share a common harmonic field, so they stay consonant with each other as the input drives them.
6. The generated events are also available as **MIDI output**, if you want to re-voice them with other instruments.

## Install (prebuilt VST3)

Prebuilt binaries are published on the [Releases](../../releases) page (they are not committed to the repo). To install a downloaded build:

```sh
# macOS, Apple Silicon
cp -R GEMS_plug.vst3 ~/Library/Audio/Plug-Ins/VST3/
```

Then rescan plugins in your DAW.

## Build from source

Requires **Xcode**, **JUCE 8**, and an Apple Silicon Mac. The vendored DSP is prebuilt into `Source/v2/libgemsv2dsp.a` and linked by the VST3 target, so you don't need to compile the DSP tree yourself.

```sh
# Open in Xcode:
open "Builds/MacOSX/GEMS_PLUG.xcodeproj"

# …or build the VST3 from the command line (arm64 only):
xcodebuild -project "Builds/MacOSX/GEMS_PLUG.xcodeproj" \
  -target "GEMS_PLUG - VST3" -configuration Release \
  ARCHS=arm64 ONLY_ACTIVE_ARCH=YES
```

> Build arm64-only (`ARCHS=arm64 ONLY_ACTIVE_ARCH=YES`). `Builds/MacOSX/GEMS_PLUG.xcodeproj` is the project to use; point the JUCE module path at your local JUCE 8 install.

## Credits & license

GEMS is released under the **GPL-3.0** license (see [`LICENSE`](LICENSE)).

The synthesis engine vendors and ports DSP from a number of open-source Eurorack / VCV module projects. Every source, commit, and license is documented in [`Source/v2/NOTICE.md`](Source/v2/NOTICE.md).

The UI embeds **Space Grotesk** and **Space Mono**, both licensed under the SIL Open Font License 1.1.

---

*GEMS is a project by [Totemphonia Studio Berlin](https://totemphonia.com/en/gems/)*

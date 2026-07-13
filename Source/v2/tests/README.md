# GEMS v2 offline verification harnesses

Pure-C++ tests (no JUCE) exercising the v2 engine ports against decoded patch expectations.

Build & run (from repo root):
```sh
clang++ -std=c++17 -O2 -ISource/v2 -o /tmp/test41 Source/v2/tests/test41.cpp Source/v2/bogaudio/{filter,multimode,utility,signal,table,math}.cpp
/tmp/test41            # default 48000
/tmp/test41 44100
/tmp/test41 96000
```

- `test41.cpp` — chunk 4.1: band VCF magnitude responses, DADSRH stage timing
  (incl. the hold-truncates-attack case), VCAmp dB law, full-chain trigger/debounce.
- `test42.cpp` — chunk 4.2: yinfast port correctness vs direct O(n²) YIN reference,
  AudioToCVPitch detection/gate/silence/hold on realistic (vibrato+noise) tones with
  harmonic-lock criteria, Quack quantization + octave transpose, Befaco slew, and
  full-engine integration (band-filtered tone → quantized voice CV; Plaits chain's
  always-open comparator).

## Note (from chunk 4.4 onward)
Once `V2Engine` embeds Braids/Plaits, EVERY test links the eurorack + speex + bogaudio
objects. Fastest workflow: pre-build them once into an object dir, then link tests
against `*.o`:
```sh
LIB=/tmp/gemslib; mkdir -p $LIB; cd $LIB
# eurorack (path-unique names — braids/ and plaits/ both have resources.cc!)
find Source/v2/eurorack -name '*.cc' -print0 | while IFS= read -r -d '' f; do
  rel=$(echo "$f" | sed 's|.*/eurorack/||; s|/|_|g; s|.cc$||')
  clang++ -std=c++17 -O2 -w -c "$f" -ISource/v2/eurorack -o "$rel.o"; done
clang -std=c99 -O2 -w -c Source/v2/speex/resample.c -ISource/v2/speex -o speex_resample.o
for f in filter multimode utility signal table math oscillator resample experiments noise; do
  clang++ -std=c++17 -O2 -w -c Source/v2/bogaudio/$f.cpp -ISource/v2/bogaudio -o bog_$f.o; done
clang++ -std=c++17 -O2 -w -c Source/v2/WavetableData.cpp -ISource/v2 -o wavetabledata.o
# then, per test:
clang++ -std=c++17 -O2 -w -ISource/v2 -ISource/v2/eurorack -o /tmp/testNN Source/v2/tests/testNN.cpp $LIB/*.o
```

- `test43.cpp` — chunk 4.3/4.6: Osculum, PalmLoop, FMOp, Ouros pitch laws (autocorrelation,
  octave-safe) + gated integration. Ouros pitch checked at cv>=0 only (texture voice — see comment).
- `test44.cpp` — chunk 4.4: Plaits (VA) note law (narrow-band ACF around expected pitch) +
  Braids PARTICLE_NOISE audibility/motion + gated integration.
- `test45.cpp` — chunk 4.5: SurgeWT frame-0 pitch law (narrow-band ACF), saturate squares
  the sine (crest 1.41→~1.0), formant response, Hiss floor, gated chain-1 integration.
- `test49.cpp` — chunk 4.9: generative modules (LFO period+amplitude, Dust event rate,
  Branches A/B split + latch toggle, Walk2 boundedness) + Sapphire Echo 1 s delay timing.
  (Standalone — no engine link needed: `clang++ -std=c++17 -O2 -ISource/v2 tests/test49.cpp -o /tmp/t49`.)
- `stab.cpp` — full-engine stability: runs all 7 chains (incl. FX feedback: Delay/Galaxy/Echo)
  with a swept-tone+noise input for 10 s at each rate; asserts every voiceOut stays finite
  and bounded. (Links the full object lib like test41–45.)

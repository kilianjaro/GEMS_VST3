#!/bin/bash
# GEMS v2 — rebuild the vendored-DSP static library `libgemsv2dsp.a`.
#
# This archive holds every vendored .cc/.cpp/.c that is NOT header-only (Braids/Plaits,
# Clouds, Speex resampler, Bogaudio non-inline, Sapphire mesh, Valley Dattorro, the
# wavetable data). The rest of the v2 engine is header-only and compiles inside the
# plugin's own translation unit (PluginProcessor.cpp). The committed `libgemsv2dsp.a`
# was produced by this recipe; you only need to re-run it if you change a vendored
# .cc/.cpp/.c (editing headers or PluginProcessor.cpp does NOT require a relink).
#
# Output: ./libgemsv2dsp.a (arm64). Run from this directory:  bash build_lib.sh
#
# KEY GOTCHAS baked in below:
#  • Clouds and eurorack both vendor Mutable "stmlib". Compiling both stmlib/dsp/units.cc
#    and stmlib/utils/random.cc twice → duplicate symbols. We keep eurorack's and SKIP
#    clouds' (same tables); we DO keep clouds-only stmlib/dsp/atan.cc.
#  • Clouds .cc need -DPARASITES (parasite firmware) and -DTEST (skip STM32 hardware
#    includes in debug_pin.h — does not change DSP/layout).
#  • bogaudio/envelope.cpp is intentionally excluded (its symbols are unused; the ADSR
#    path FMOp uses is inline). Including it is harmless but we match the shipped lib.
#  • The project path contains a space ("GEMS plugin "), so we use null-delimited loops
#    and inline -I flags (never shell variables holding lists).
set -e
cd "$(dirname "$0")"                       # = Source/v2
OUT="libgemsv2dsp.a"
OBJ="$(mktemp -d)"
CXX="clang++ -std=c++17 -O2 -arch arm64 -fPIC -w -mmacosx-version-min=10.13"
CC="clang -O2 -arch arm64 -fPIC -w -mmacosx-version-min=10.13"
n=0
compile_cxx() { # $1=src  $2=objname  $3..=extra flags (e.g. -Ieurorack -DTEST)
	local src="$1"; local obj="$OBJ/$2.o"; shift 2
	$CXX "$@" -c "$src" -o "$obj"; n=$((n+1))
}

echo ">> eurorack (Braids + Plaits + stmlib units/random) — all .cc, -Ieurorack"
find eurorack -name '*.cc' -print0 | while IFS= read -r -d '' f; do
	clang++ -std=c++17 -O2 -arch arm64 -fPIC -w -mmacosx-version-min=10.13 \
		-Ieurorack -c "$f" -o "$OBJ/eu_$(echo "$f" | tr '/.' '__').o"
done

echo ">> clouds — clouds/clouds/**/*.cc + stmlib/dsp/atan.cc, -Iclouds -DPARASITES -DTEST (skip clouds stmlib units/random)"
find clouds/clouds -name '*.cc' -print0 | while IFS= read -r -d '' f; do
	clang++ -std=c++17 -O2 -arch arm64 -fPIC -w -mmacosx-version-min=10.13 \
		-Iclouds -DPARASITES -DTEST -c "$f" -o "$OBJ/cl_$(echo "$f" | tr '/.' '__').o"
done
clang++ -std=c++17 -O2 -arch arm64 -fPIC -w -mmacosx-version-min=10.13 \
	-Iclouds -DPARASITES -DTEST -c clouds/stmlib/dsp/atan.cc -o "$OBJ/cl_stmlib_atan.o"

echo ">> bogaudio — all .cpp EXCEPT envelope.cpp, -Ibogaudio"
find bogaudio -name '*.cpp' ! -name 'envelope.cpp' -print0 | while IFS= read -r -d '' f; do
	clang++ -std=c++17 -O2 -arch arm64 -fPIC -w -mmacosx-version-min=10.13 \
		-Ibogaudio -c "$f" -o "$OBJ/bog_$(basename "$f" .cpp).o"
done

echo ">> sapphire (Elastika mesh) — -Isapphire (arm64 __m128 scalar shim is unconditional)"
find sapphire -name '*.cpp' -print0 | while IFS= read -r -d '' f; do
	clang++ -std=c++17 -O2 -arch arm64 -fPIC -w -mmacosx-version-min=10.13 \
		-Isapphire -DNO_RACK_DEPENDENCY -c "$f" -o "$OBJ/sap_$(basename "$f" .cpp).o"
done

echo ">> valley (Dattorro plate reverb) — -Ivalley"
find valley -name '*.cpp' -print0 | while IFS= read -r -d '' f; do
	clang++ -std=c++17 -O2 -arch arm64 -fPIC -w -mmacosx-version-min=10.13 \
		-Ivalley -c "$f" -o "$OBJ/valley_$(basename "$f" .cpp).o"
done

echo ">> speex resampler (C) — -Ispeex (resample.c self-defines OUTSIDE_SPEEX/RANDOM_PREFIX)"
clang -O2 -arch arm64 -fPIC -w -mmacosx-version-min=10.13 \
	-Ispeex -c speex/resample.c -o "$OBJ/speex_resample.o"

echo ">> wavetable data — -I. (Source/v2 root)"
clang++ -std=c++17 -O2 -arch arm64 -fPIC -w -mmacosx-version-min=10.13 \
	-I. -c WavetableData.cpp -o "$OBJ/wavetabledata.o"

echo ">> archive $(ls "$OBJ"/*.o | wc -l | tr -d ' ') objects -> $OUT"
rm -f "$OUT"
ar rcs "$OUT" "$OBJ"/*.o
ranlib "$OUT" 2>/dev/null || true
lipo -info "$OUT"
rm -rf "$OBJ"
echo ">> done."

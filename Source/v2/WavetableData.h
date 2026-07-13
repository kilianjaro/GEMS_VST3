#pragma once
// GEMS v2 — wavetable "Sine PD HQ" (SurgeXTOSCWavetable; 32 tables x 512 samples,
// int16, flags=4).
//
// Decoded from the SurgeXTOSCWavetable wavetable data (base64) as: wt_header (12 bytes,
// packed: char tag[4]; uint32 n_samples; uint16 n_tables; uint16 flags) followed
// by n_tables * n_samples raw little-endian int16 samples in [table][sample]
// (table-major) order — i.e. table j occupies samples[j*n_samples .. j*n_samples+n_samples-1].
//
// Decode verified against:
//  - surge-rack src/VCO.h, VCO<oscType>::makeModuleSpecificJson() (~line 763-808):
//      builds wt_header wth with wth.flags = (wt.flags | wtf_int16) & ~wtf_int16_is_16;
//      writes header then, per table j: memcpy from
//      wt.TableI16WeakPointers[0][j][FIRoffsetI16], n_samples * sizeof(uint16_t) bytes;
//      base64-encodes the whole (header + data) buffer via rack::string::toBase64.
//    and VCO<oscType>::readModuleSpecificJson() (~line 842-874): base64-decodes,
//      memcpy's the first sizeof(wt_header) bytes into wt_header wth, advances the
//      pointer past the header, then calls oscstorage->wt.BuildWT(data, wth, false)
//      on the remaining raw int16 buffer.
//  - surge src/common/dsp/Wavetable.h: struct wt_header (packed, tag[4]+n_samples(u32)
//      +n_tables(u16)+flags(u16) = 12 bytes); enum wtflags { wtf_is_sample=1,
//      wtf_loop_sample=2, wtf_int16=4, wtf_int16_is_16=8, wtf_has_metadata=0x10 }.
//      flags=4 here means only wtf_int16 is set (wtf_int16_is_16 is NOT set).
//  - surge src/common/dsp/Wavetable.cpp, Wavetable::BuildWT() (~line 168-260):
//      when (this->flags & wtf_int16), copies each table's raw shorts directly
//      (mech::endian_copyblock16LE(&TableI16WeakPointers[0][j][FIRoffsetI16],
//       &((short*)wdata)[this->size * j], this->size)) — confirming table-major
//      [table][sample] int16 layout with no header inside the per-table data.
//      Then, since wtf_int16_is_16 is NOT set, it calls i152float_block (not
//      i162float_block) to convert to float.
//  - surge src/common/dsp/vembertech/basic_dsp.h:
//      i152float_block: "const float scale = 1.f / 16384.f;" (15-bit signed scale,
//      NOT /32768) — i.e. when wtf_int16_is_16 is absent, raw int16 sample s maps to
//      float s/16384. (i162float_block, used only when wtf_int16_is_16 IS set, uses
//      "const float scale = 1.f / (16384.f * 2)" i.e. s/32768.) The raw data stored
//      in this header is left un-rescaled (verbatim int16), matching the on-disk /
//      in-JSON representation; apply the /16384 scale yourself if you need float
//      samples matching Surge's own playback.
//
// Verified byte count: base64-decoded length was 12 (header) + 32*512*2 (data)
// = 32780 bytes, matching exactly. Header tag bytes were all zero (surge-rack
// memsets wth.tag to 0 rather than writing a real tag when JSON-streaming).
//
// Sanity (table 0): raw int16 min -16384, max 16384; scaled by 1/16384 that is
// [-1.0, 1.0]; DFT bin-1 magnitude ~256.0 vs bins 2-5 ~0.0 (pure-sine dominant,
// as expected for "Sine PD HQ" table 0). Global min/max across all 32 tables
// stayed within the 16-bit signed range.

namespace gemsv2 { namespace wtdata {

constexpr int kNumTables = 32;
constexpr int kNumSamples = 512;
constexpr int kFlags = 4;

// raw int16 samples, [table][sample] order as stored (verbatim from the patch
// JSON's base64 blob, header stripped; NOT rescaled to float)
extern const short data[kNumTables][kNumSamples];

} } // namespace gemsv2::wtdata

#pragma once

#include <JuceHeader.h>
#include <vector>
#include <cmath>

//==============================================================================
// Master FX chain for the wet synth bus — captures the colour of the reference
// Cardinal patch's output section:
//   Drive/Tape (rackwindows tape)  → saturation + gentle HF roll-off
//   Stereo Delay (SurgeXTDelay / Sapphire Echo) → ping-pong, filtered feedback
//   Reverb (Valley Plateau / Sapphire Galaxy)
//   Glue compressor (Bogaudio Pressor)
//
// Order: Drive → Delay → Reverb → Comp. All processing is block-based and
// allocation-free; buffers are sized in prepare().
class MasterFx
{
public:
    void prepare (double sampleRate, int /*maxBlock*/)
    {
        sr = (sampleRate > 0.0 ? sampleRate : 48000.0);
        delayLen = (int) (sr * 1.6) + 8;
        for (int ch = 0; ch < 2; ++ch)
            delayBuf[ch].assign ((size_t) delayLen, 0.0f);

        reverb.setSampleRate (sr);
        reverb.reset();
        reset();

        // attack/release for the glue compressor
        compAtk = std::exp (-1.0f / (0.005f * (float) sr));   // 5 ms
        compRel = std::exp (-1.0f / (0.120f * (float) sr));   // 120 ms
        // feedback / tape filter coefficients
        fbCoeff   = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 4500.0f / (float) sr);
        tapeCoeff = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 8000.0f / (float) sr);
    }

    void reset()
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            std::fill (delayBuf[ch].begin(), delayBuf[ch].end(), 0.0f);
            tapeState[ch] = 0.0f; fbState[ch] = 0.0f;
        }
        writeIdx = 0; compEnv = 0.0f;
        reverb.reset();
    }

    void setParams (float drive, float delayMix, float delayTimeMs, float delayFb,
                    float reverbMix, float comp)
    {
        driveAmt = juce::jlimit (0.0f, 1.0f, drive);
        dlyMix   = juce::jlimit (0.0f, 1.0f, delayMix);
        dlyFb    = juce::jlimit (0.0f, 0.95f, delayFb);
        revMix   = juce::jlimit (0.0f, 1.0f, reverbMix);
        compAmt  = juce::jlimit (0.0f, 1.0f, comp);

        targetDelay = juce::jlimit (1.0f, (float) (delayLen - 4),
                                    delayTimeMs * 0.001f * (float) sr);

        juce::Reverb::Parameters rp;
        rp.roomSize   = 0.50f + revMix * 0.48f;
        rp.damping    = 0.40f;
        rp.width      = 1.0f;
        rp.wetLevel   = revMix * 0.9f;
        rp.dryLevel   = 1.0f - revMix * 0.5f;
        rp.freezeMode = 0.0f;
        reverb.setParameters (rp);
    }

    void process (juce::AudioBuffer<float>& buf, int numSamples)
    {
        float* L = buf.getWritePointer (0);
        float* R = (buf.getNumChannels() > 1) ? buf.getWritePointer (1) : L;

        // ── 1. Drive / tape colour ──
        if (driveAmt > 0.001f)
        {
            const float drive   = 1.0f + driveAmt * 6.0f;
            const float invSat  = 1.0f / std::tanh (drive);
            const float makeup  = 1.0f / (1.0f + driveAmt * 0.6f);
            const float tilt    = 0.45f * driveAmt;
            for (int n = 0; n < numSamples; ++n)
            {
                for (int ch = 0; ch < 2; ++ch)
                {
                    float* buffp = (ch == 0) ? L : R;
                    float x = std::tanh (buffp[n] * drive) * invSat;
                    tapeState[ch] += tapeCoeff * (x - tapeState[ch]);   // HF roll-off
                    buffp[n] = (x * (1.0f - tilt) + tapeState[ch] * tilt) * makeup;
                }
                if (buf.getNumChannels() == 1) { /* mono: R aliases L */ }
            }
        }

        // ── 2. Stereo ping-pong delay ──
        if (dlyMix > 0.001f)
        {
            for (int n = 0; n < numSamples; ++n)
            {
                curDelay += 0.0008f * (targetDelay - curDelay);   // glide time changes

                const float dL = readInterp (0, curDelay);
                const float dR = readInterp (1, curDelay);

                fbState[0] += fbCoeff * (dL - fbState[0]);        // darken feedback
                fbState[1] += fbCoeff * (dR - fbState[1]);

                const float inL = L[n], inR = R[n];
                // cross-feed → ping-pong
                delayBuf[0][(size_t) writeIdx] = inL + fbState[1] * dlyFb;
                delayBuf[1][(size_t) writeIdx] = inR + fbState[0] * dlyFb;

                L[n] = inL * (1.0f - dlyMix) + dL * dlyMix;
                R[n] = inR * (1.0f - dlyMix) + dR * dlyMix;

                if (++writeIdx >= delayLen) writeIdx = 0;
            }
        }

        // ── 3. Reverb ──
        if (revMix > 0.001f)
            reverb.processStereo (L, R, numSamples);

        // ── 4. Glue compressor (stereo-linked, soft) ──
        if (compAmt > 0.001f)
        {
            const float thrDb  = -compAmt * 22.0f;
            const float thrLin = juce::Decibels::decibelsToGain (thrDb);
            const float ratio  = 1.0f + compAmt * 5.0f;
            const float slope  = 1.0f - 1.0f / ratio;
            const float makeup = juce::Decibels::decibelsToGain (compAmt * 7.0f);
            for (int n = 0; n < numSamples; ++n)
            {
                const float key = juce::jmax (std::abs (L[n]), std::abs (R[n]));
                compEnv = (key > compEnv) ? (compAtk * compEnv + (1.0f - compAtk) * key)
                                          : (compRel * compEnv + (1.0f - compRel) * key);
                float gain = 1.0f;
                if (compEnv > thrLin && compEnv > 1.0e-6f)
                {
                    const float overDb = juce::Decibels::gainToDecibels (compEnv) - thrDb;
                    gain = juce::Decibels::decibelsToGain (-overDb * slope);
                }
                L[n] *= gain * makeup;
                R[n] *= gain * makeup;
            }
        }
    }

private:
    inline float readInterp (int ch, float delaySamples)
    {
        float rp = (float) writeIdx - delaySamples;
        while (rp < 0.0f) rp += (float) delayLen;
        int i0 = (int) rp;
        const float fr = rp - (float) i0;
        i0 %= delayLen;
        int i1 = i0 + 1; if (i1 >= delayLen) i1 = 0;
        const auto& b = delayBuf[ch];
        return b[(size_t) i0] + fr * (b[(size_t) i1] - b[(size_t) i0]);
    }

    double sr = 48000.0;

    // delay
    std::vector<float> delayBuf[2];
    int   delayLen = 1, writeIdx = 0;
    float curDelay = 0.0f, targetDelay = 1.0f;
    float dlyMix = 0.0f, dlyFb = 0.0f, fbCoeff = 0.0f, fbState[2] = { 0, 0 };

    // drive/tape
    float driveAmt = 0.0f, tapeCoeff = 0.0f, tapeState[2] = { 0, 0 };

    // reverb
    juce::Reverb reverb;
    float revMix = 0.0f;

    // comp
    float compAmt = 0.0f, compEnv = 0.0f, compAtk = 0.0f, compRel = 0.0f;
};

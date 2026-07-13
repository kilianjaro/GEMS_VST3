#pragma once

#include <JuceHeader.h>
#include <utility>
#include <array>
#include <cmath>

//==============================================================================
enum class SynthWaveform : int
{
    Sine = 0,
    Triangle,
    Saw,
    Square
};

//==============================================================================
// Voice engines. Each captures the *character* of a module family from the
// reference Cardinal patch rather than cloning it exactly.
//   Blend     – dual-osc morph (analog-ish)            [original GEMS voice]
//   FM        – 2-operator FM                          (Bogaudio FMOp / TZFM)
//   Wavetable – additive scan, evolving/digital        (SurgeXT Wavetable)
//   PhaseFB   – phase-feedback oscillator, vocal/rich  (CVfunk Ouros)
//   Harmonic  – additive drawbar / organ-drone         (Sonus Osculum)
//   Modal     – modal resonator bank, plucked/struck   (Parable Neil = Rings)
//   Swarm     – detuned saw swarm, lush                (Plaits / Braids VA)
//   Airwave   – pitched resonant noise, breathy/windy  (wind / waves / city)
enum class VoiceEngine : int
{
    Blend = 0,
    FM,
    Wavetable,
    PhaseFB,
    Harmonic,
    Modal,
    Swarm,
    Airwave,
    Count
};
static constexpr int kNumEngines = (int) VoiceEngine::Count;

static inline const char* voiceEngineName (int e)
{
    static const char* names[kNumEngines] =
        { "BLEND", "FM", "WAVETABLE", "PHASE-FB", "HARMONIC", "MODAL", "SWARM", "AIRWAVE" };
    return names[juce::jlimit (0, kNumEngines - 1, e)];
}

//==============================================================================
// polyBLEP band-limited step (for saw / square edges)
static inline float polyBlep (float t, float dt)
{
    if (dt <= 0.0f) return 0.0f;
    if (t < dt)            { float x = t / dt;        return x + x - x * x - 1.0f; }
    else if (t > 1.0f - dt){ float x = (t - 1.0f) / dt; return x * x + x + x + 1.0f; }
    return 0.0f;
}

//==============================================================================
// Cytomic TPT state-variable filter — stable under per-block retuning.
// Used as a high-Q resonator (modal voice) and pitched band-pass (airwave).
struct SvfTPT
{
    void reset() { ic1eq = ic2eq = 0.0; }

    // g = tan(pi*fc/sr), k = 1/Q
    void setCoeffs (double fcHz, double sr, double Q)
    {
        fcHz = juce::jlimit (10.0, sr * 0.45, fcHz);
        const double g = std::tan (juce::MathConstants<double>::pi * fcHz / sr);
        const double k = 1.0 / juce::jmax (0.05, Q);
        a1 = 1.0 / (1.0 + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }

    // returns band-pass output
    inline float processBP (float x)
    {
        const double v0 = (double) x;
        const double v3 = v0 - ic2eq;
        const double v1 = a1 * ic1eq + a2 * v3;
        const double v2 = ic2eq + a2 * ic1eq + a3 * v3;
        ic1eq = 2.0 * v1 - ic1eq;
        ic2eq = 2.0 * v2 - ic2eq;
        return (float) v1;
    }

    double a1 = 0, a2 = 0, a3 = 0;
    double ic1eq = 0, ic2eq = 0;
};

//==============================================================================
// 24dB/oct filter (two cascaded biquad sections)
struct SynthFilter
{
    void prepare (double sampleRate) { sr = sampleRate; reset(); }

    void reset()
    {
        for (auto& s : sec) s = {};
    }

    // type: 0 = highpass, 1 = lowpass
    float process (float in, float cutoffHz, int type)
    {
        cutoffHz = juce::jlimit (20.0f, juce::jmin (20000.0f, (float) (sr * 0.45)), cutoffHz);
        float out = in;
        for (int i = 0; i < 2; ++i)
            out = biquad (out, cutoffHz, type, sec[i]);
        return out;
    }

private:
    struct BQ { double z1 = 0, z2 = 0; };

    float biquad (float in, float cutHz, int type, BQ& s)
    {
        const double w0 = juce::MathConstants<double>::twoPi * (double) cutHz / sr;
        const double cosw = std::cos (w0), sinw = std::sin (w0);
        const double alpha = sinw / (2.0 * 0.7071); // Butterworth Q

        double b0, b1, b2, a0, a1, a2;
        if (type == 0) { b0 = (1+cosw)/2; b1 = -(1+cosw); b2 = (1+cosw)/2; }
        else           { b0 = (1-cosw)/2; b1 =  (1-cosw); b2 = (1-cosw)/2; }
        a0 = 1 + alpha; a1 = -2*cosw; a2 = 1 - alpha;
        b0/=a0; b1/=a0; b2/=a0; a1/=a0; a2/=a0;

        double out = b0 * (double)in + s.z1;
        s.z1 = b1 * (double)in - a1 * out + s.z2;
        s.z2 = b2 * (double)in - a2 * out;
        return (float) out;
    }

    double sr = 48000.0;
    BQ sec[2];
};

//==============================================================================
// Pink noise (Paul Kellet method)
struct PinkNoise
{
    float next()
    {
        float white = whiteNext();
        b0 = 0.99886f * b0 + white * 0.0555179f;
        b1 = 0.99332f * b1 + white * 0.0750759f;
        b2 = 0.96900f * b2 + white * 0.1538520f;
        b3 = 0.86650f * b3 + white * 0.3104856f;
        b4 = 0.55000f * b4 + white * 0.5329522f;
        b5 = -0.7616f * b5 - white * 0.0168980f;
        float pink = b0 + b1 + b2 + b3 + b4 + b5 + b6 + white * 0.5362f;
        b6 = white * 0.115926f;
        return pink * 0.11f;
    }
    float whiteNext()
    {
        // xorshift32 — cheap, deterministic, no global rand() contention
        state ^= state << 13; state ^= state >> 17; state ^= state << 5;
        return (float) ((int32_t) state) * (1.0f / 2147483648.0f);
    }
    void reset() { b0=b1=b2=b3=b4=b5=b6=0; }
private:
    uint32_t state = 0x1234567u;
    float b0=0,b1=0,b2=0,b3=0,b4=0,b5=0,b6=0;
};

//==============================================================================
class TrackSynth
{
public:
    void prepare (double sampleRate)
    {
        sr = (sampleRate > 0.0 ? sampleRate : 48000.0);
        adsr.setSampleRate (sr);
        hpFilter.prepare (sr);
        lpFilter.prepare (sr);
        pinkNoise.reset();

        // Modal delay/resonator buffers
        for (auto& f : modalSvf) f.reset();

        reset();
    }

    void reset()
    {
        phase01 = 0.0f; phaseB = 0.0f; currentHz = 0.0f;
        fbLast = 0.0f;
        for (auto& p : swarmPhase) p = 0.0f;
        for (auto& f : modalSvf) f.reset();
        airSvf.reset();
        exciteEnv = 0.0f; triggerExcite = false;
        adsr.reset(); hpFilter.reset(); lpFilter.reset(); pinkNoise.reset();
        lastWaveL = lastWaveR = -1; lastMix = 999.0f;
        lastA = lastD = lastS = lastR = -1.0f;
        coeffsDirty = true;
    }

    void setEngine (int e)
    {
        e = juce::jlimit (0, kNumEngines - 1, e);
        if (e != (int) engine) { engine = (VoiceEngine) e; coeffsDirty = true; }
    }

    // morph: bipolar -1..+1 (from Wave Mix knob, reused as primary timbre macro)
    // timbre: 0..1 (new secondary macro)
    void setMacros (float morphBipolar, float timbre01)
    {
        float m = juce::jlimit (-1.0f, 1.0f, morphBipolar);
        float t = juce::jlimit (0.0f, 1.0f, timbre01);
        if (std::abs (m - morph) > 1e-6f || std::abs (t - timbre) > 1e-6f)
        { morph = m; timbre = t; coeffsDirty = true; }
    }

    void setWaveParams (int waveLeft, int waveRight, float bipolarMix)
    {
        waveLeft  = juce::jlimit (0, 3, waveLeft);
        waveRight = juce::jlimit (0, 3, waveRight);
        if (waveRight == waveLeft) waveRight = (waveRight + 1) & 3;
        bipolarMix = juce::jlimit (-1.0f, 1.0f, bipolarMix);
        if (waveLeft != lastWaveL || waveRight != lastWaveR || std::abs (bipolarMix - lastMix) > 1e-6f)
        { lastWaveL=waveLeft; lastWaveR=waveRight; lastMix=bipolarMix;
          wfL=(SynthWaveform)waveLeft; wfR=(SynthWaveform)waveRight; mix=bipolarMix; }
    }

    void setAdsrParams (float a, float d, float s, float r)
    {
        a=juce::jlimit(0.f,8.f,a); d=juce::jlimit(0.f,8.f,d);
        s=juce::jlimit(0.f,1.f,s); r=juce::jlimit(0.f,8.f,r);
        if(std::abs(a-lastA)<1e-6f && std::abs(d-lastD)<1e-6f &&
           std::abs(s-lastS)<1e-6f && std::abs(r-lastR)<1e-6f) return;
        lastA=a;lastD=d;lastS=s;lastR=r; susLevel=s;
        juce::ADSR::Parameters p; p.attack=a;p.decay=d;p.sustain=s;p.release=r;
        adsr.setParameters(p);
    }

    void setSynthShaping (float noise01, float tone01, float hpHz, float lpHz)
    {
        noiseAmt = juce::jlimit (0.0f, 1.0f, noise01);
        toneAmt  = juce::jlimit (0.0f, 1.0f, tone01);
        hpCutoff = juce::jlimit (20.0f, 20000.0f, hpHz);
        lpCutoff = juce::jlimit (20.0f, 20000.0f, lpHz);
    }

    void noteOn (float hz)
    {
        currentHz = juce::jlimit (0.f, 20000.f, hz);
        phase01 = 0; phaseB = 0; fbLast = 0;
        triggerExcite = true;        // pluck/strike physical engines
        coeffsDirty = true;
        adsr.noteOn();
    }
    void noteOff() { adsr.noteOff(); }
    void setFrequency (float hz)
    {
        float h = juce::jlimit (0.f, 20000.f, hz);
        if (std::abs (h - currentHz) > 0.01f) { currentHz = h; coeffsDirty = true; }
    }

    // Signal chain: engine core → noise → tone → HP → LP → envelope → pan
    std::pair<float, float> renderSample (float velGain, float pan, float width)
    {
        const float env = adsr.getNextSample();
        if (env <= 1e-6f || currentHz <= 0.0f || sr <= 0.0)
            return { 0.0f, 0.0f };

        if (coeffsDirty) updateCoeffs();

        // 1. Engine core (raw, ~-1..1)
        float sig = renderEngine();

        // 2. Pink noise mix
        if (noiseAmt > 0.001f)
            sig = sig * (1.0f - noiseAmt) + pinkNoise.next() * noiseAmt;

        // 3. Tone (saturation)
        if (toneAmt > 0.001f)
            sig = applySaturation(sig, toneAmt);

        // 4. High-pass (24dB/oct)
        if (hpCutoff > 21.0f)
            sig = hpFilter.process(sig, hpCutoff, 0);

        // 5. Low-pass (24dB/oct)
        if (lpCutoff < 19990.0f)
            sig = lpFilter.process(sig, lpCutoff, 1);

        // Envelope + velocity
        const float mono = sig * env * velGain;

        // Pan with width
        pan = juce::jlimit(-1.f,1.f,pan) * juce::jlimit(0.f,1.f,width);
        const float gL = (pan >= 0.f) ? (1.f-pan) : 1.f;
        const float gR = (pan <= 0.f) ? (1.f+pan) : 1.f;
        return { mono*gL, mono*gR };
    }

    float getNoiseAmt() const { return noiseAmt; }
    float getToneAmt() const { return toneAmt; }

    static float osc (SynthWaveform wf, float phase01)
    {
        const float t = phase01 - std::floor(phase01);
        switch (wf)
        {
            case SynthWaveform::Sine:     return std::sin(juce::MathConstants<float>::twoPi * t);
            case SynthWaveform::Triangle: return 4.f * std::abs(t - 0.5f) - 1.f;
            case SynthWaveform::Saw:      return 2.f * t - 1.f;
            case SynthWaveform::Square:   return (t < 0.5f) ? 1.f : -1.f;
        }
        return 0.f;
    }

    // Saturation used by both renderSample and waveform display
    static float applySaturation (float x, float amt)
    {
        if (amt <= 0.5f)
        {
            // Harmonic: smooth tanh drive (1..5)
            float drive = 1.0f + amt * 8.0f;
            return std::tanh(x * drive) / std::tanh(drive);
        }
        // Harmonic base + disharmonic (asymmetric clip)
        float drive = 5.0f + (amt - 0.5f) * 20.0f;
        float sat = std::tanh(x * drive) / std::tanh(drive);
        float disharm = (amt - 0.5f) * 2.0f;
        float asym = x * drive * 0.5f;
        float clip = (asym > 0) ? std::tanh(asym*1.5f) : std::tanh(asym*0.7f);
        return sat * (1.f - disharm*0.5f) + clip * 0.5f * disharm * 0.5f;
    }

    // Static preview of an engine's single-cycle shape, for the UI waveform display.
    // Stateless approximation (no feedback history) — good enough as an icon.
    static float previewEngine (int engineIdx, float phase01, float morphBipolar, float timbre01,
                                int waveL, int waveR, float waveMix)
    {
        const float t = phase01 - std::floor (phase01);
        const float TWO_PI = juce::MathConstants<float>::twoPi;
        switch ((VoiceEngine) juce::jlimit (0, kNumEngines - 1, engineIdx))
        {
            case VoiceEngine::Blend:
            {
                const float wL = 0.5f * (1.0f - waveMix), wR = 0.5f * (1.0f + waveMix);
                return osc ((SynthWaveform) waveL, t) * wL + osc ((SynthWaveform) waveR, t) * wR;
            }
            case VoiceEngine::FM:
            {
                float ratio = fmRatioFor (morphBipolar);
                float index = timbre01 * 6.0f;
                return std::sin (TWO_PI * t + index * std::sin (TWO_PI * ratio * t));
            }
            case VoiceEngine::Wavetable:
            {
                float v = 0.0f, norm = 0.0f;
                int K = 16;
                for (int k = 1; k <= K; ++k)
                {
                    float tilt = std::pow ((float) k, -(0.4f + (1.0f - timbre01) * 1.6f));
                    float oddBias = (k % 2 == 1) ? 1.0f : (0.5f + 0.5f * (morphBipolar * 0.5f + 0.5f));
                    float a = tilt * oddBias;
                    v += a * std::sin (TWO_PI * k * t); norm += a;
                }
                return (norm > 0) ? v / norm * 1.4f : 0.0f;
            }
            case VoiceEngine::PhaseFB:
            {
                float fb = 0.6f + timbre01 * 2.4f;
                static thread_local float last = 0.0f;
                float o = std::sin (TWO_PI * t + fb * last);
                last = o;
                return o;
            }
            case VoiceEngine::Harmonic:
            {
                float v = 0.0f, norm = 0.0f;
                int K = 8 + (int) (timbre01 * 8.0f);
                for (int k = 1; k <= K; ++k)
                {
                    bool keep = (morphBipolar < 0.0f) ? (k % 2 == 1) : true;
                    if (!keep) continue;
                    float a = std::pow ((float) k, -(0.7f + (1.0f - timbre01)));
                    v += a * std::sin (TWO_PI * k * t); norm += a;
                }
                return (norm > 0) ? v / norm * 1.3f : 0.0f;
            }
            case VoiceEngine::Modal:
            {
                // show a fast-decaying struck shape
                float decay = std::exp (-t * (3.0f + (1.0f - timbre01) * 8.0f));
                float inh = 1.0f + morphBipolar * 0.25f;
                return decay * (std::sin (TWO_PI * t) + 0.5f * std::sin (TWO_PI * 2.76f * inh * t)
                                                      + 0.3f * std::sin (TWO_PI * 5.40f * inh * t));
            }
            case VoiceEngine::Swarm:
            {
                float v = 0.0f; int N = 5;
                for (int i = 0; i < N; ++i)
                {
                    float det = (i - (N - 1) * 0.5f) * (0.004f + timbre01 * 0.02f);
                    float tt = t * (1.0f + det); tt -= std::floor (tt);
                    v += 2.0f * tt - 1.0f;
                }
                return v / N;
            }
            case VoiceEngine::Airwave:
            default:
            {
                // breathy: sine + noise-ish ripple
                float ripple = 0.3f * std::sin (TWO_PI * 7.0f * t) * (0.4f + timbre01);
                return std::sin (TWO_PI * t) * 0.7f + ripple;
            }
        }
    }

private:
    //==========================================================================
    static float fmRatioFor (float morphBipolar)
    {
        // map -1..+1 to a set of musical ratios
        static const float ratios[] = { 0.5f, 1.0f, 1.5f, 2.0f, 3.0f, 4.0f, 5.0f, 7.0f };
        int n = (int) (sizeof (ratios) / sizeof (float));
        int idx = juce::jlimit (0, n - 1, (int) std::round ((morphBipolar * 0.5f + 0.5f) * (n - 1)));
        return ratios[idx];
    }

    void updateCoeffs()
    {
        coeffsDirty = false;
        const float f0 = juce::jlimit (20.0f, 12000.0f, currentHz);

        if (engine == VoiceEngine::FM)
        {
            fmRatio = fmRatioFor (morph);
            fmIndex = timbre * 8.0f;
        }
        else if (engine == VoiceEngine::PhaseFB)
        {
            fbAmount = 0.4f + timbre * 2.8f;          // feedback depth
            fbColour = 0.5f + (morph * 0.5f + 0.5f);  // second-phase emphasis
        }
        else if (engine == VoiceEngine::Wavetable || engine == VoiceEngine::Harmonic)
        {
            updateAdditive (f0);
        }
        else if (engine == VoiceEngine::Modal)
        {
            // partial ratios with morph-controlled inharmonicity (string→bar→bell)
            const float inh = morph; // -1..1
            const float baseR[4] = { 1.0f, 2.0f, 3.0f, 4.5f };
            const float bellR[4] = { 1.0f, 2.76f, 5.40f, 8.93f };
            const float Q = 60.0f + (1.0f - timbre) * 240.0f; // timbre → decay (low timbre = long ring)
            for (int i = 0; i < kModal; ++i)
            {
                float blend = inh * 0.5f + 0.5f;
                float r = baseR[i] * (1.0f - blend) + bellR[i] * blend;
                float fr = juce::jlimit (20.0f, (float) sr * 0.45f, f0 * r);
                modalSvf[i].setCoeffs (fr, sr, Q);
                modalGain[i] = 1.0f / (1.0f + i * 0.8f);
            }
            modalDecay = std::exp (-1.0f / ((0.05f + (1.0f - timbre) * 2.5f) * (float) sr));
        }
        else if (engine == VoiceEngine::Airwave)
        {
            const float Q = 4.0f + (morph * 0.5f + 0.5f) * 60.0f; // morph → resonance
            airSvf.setCoeffs (f0, sr, Q);
            airSecond = timbre; // amount of a second partial whistle
            airQ = Q;
        }
    }

    void updateAdditive (float f0)
    {
        const float nyq = 0.45f * (float) sr;
        const int maxK = juce::jlimit (1, kMaxHarm, (int) (nyq / juce::jmax (20.0f, f0)));
        additiveK = maxK;
        float norm = 0.0f;

        if (engine == VoiceEngine::Wavetable)
        {
            // scan: morph sweeps from hollow (square-ish, odd only) → full saw → formant peak
            const float scan = morph * 0.5f + 0.5f;       // 0..1
            const float bright = 0.3f + timbre * 1.4f;     // rolloff exponent
            const float formantK = 1.0f + scan * (maxK - 1); // moving emphasis
            for (int k = 1; k <= maxK; ++k)
            {
                float tilt = std::pow ((float) k, -bright);
                float oddFull = (k % 2 == 1) ? 1.0f : scan;          // bring in evens as we scan
                float formant = 1.0f + 0.8f * std::exp (-0.5f * ((k - formantK) * (k - formantK)) / 4.0f);
                harmAmp[k] = tilt * oddFull * formant;
                norm += harmAmp[k];
            }
        }
        else // Harmonic (organ drawbar / Osculum)
        {
            const float tilt = 0.6f + (1.0f - timbre) * 1.6f;  // timbre → brightness
            const bool oddOnly = (morph < -0.33f);
            const float inharm = juce::jmax (0.0f, morph) * 0.02f; // slight stretch when morph>0
            for (int k = 1; k <= maxK; ++k)
            {
                if (oddOnly && (k % 2 == 0)) { harmAmp[k] = 0.0f; continue; }
                harmAmp[k] = std::pow ((float) k, -tilt);
                norm += harmAmp[k];
            }
            harmStretch = inharm;
        }

        const float inv = (norm > 1e-6f) ? (1.3f / norm) : 1.0f;
        for (int k = 1; k <= maxK; ++k) harmAmp[k] *= inv;
    }

    //==========================================================================
    float renderEngine()
    {
        const float dt = (float) (currentHz / (float) sr);
        const float TWO_PI = juce::MathConstants<float>::twoPi;

        switch (engine)
        {
            case VoiceEngine::Blend:
            {
                advance (phase01, dt);
                const float wL = 0.5f * (1.0f - mix), wR = 0.5f * (1.0f + mix);
                return blepOsc (wfL, phase01, dt) * wL + blepOsc (wfR, phase01, dt) * wR;
            }

            case VoiceEngine::FM:
            {
                advance (phase01, dt);
                advance (phaseB, dt * fmRatio);
                const float modSig = std::sin (TWO_PI * phaseB);
                return std::sin (TWO_PI * phase01 + fmIndex * modSig);
            }

            case VoiceEngine::Wavetable:
            case VoiceEngine::Harmonic:
            {
                advance (phase01, dt);
                return additiveSample (phase01);
            }

            case VoiceEngine::PhaseFB:
            {
                advance (phase01, dt);
                float o = std::sin (TWO_PI * (phase01 + fbColour * 0.15f) + fbAmount * fbLast);
                fbLast = 0.5f * (fbLast + o); // smoothed feedback (DC-safe)
                return o;
            }

            case VoiceEngine::Modal:
            {
                // excitation: short noise burst on trigger, plus optional sustain bow
                float exc = 0.0f;
                if (triggerExcite) { exciteEnv = 1.0f; triggerExcite = false; }
                if (exciteEnv > 1e-4f)
                {
                    exc += pinkNoise.whiteNext() * exciteEnv;
                    exciteEnv *= 0.992f; // ~few ms burst
                }
                // continuous bow scaled by sustain (lets it drone)
                exc += pinkNoise.whiteNext() * susLevel * 0.04f;

                float y = 0.0f;
                for (int i = 0; i < kModal; ++i)
                    y += modalSvf[i].processBP (exc) * modalGain[i];
                return juce::jlimit (-1.5f, 1.5f, y * 1.5f);
            }

            case VoiceEngine::Swarm:
            {
                const int N = kSwarm;
                float v = 0.0f;
                const float spread = 0.003f + timbre * 0.03f; // detune
                for (int i = 0; i < N; ++i)
                {
                    float det = (i - (N - 1) * 0.5f) * spread;
                    float d = dt * (1.0f + det);
                    advance (swarmPhase[i], d);
                    v += blepSaw (swarmPhase[i], d);
                }
                v /= (float) N;
                // morph blends in a sub-octave sine for body
                if (morph > -0.99f)
                {
                    float sub = std::sin (TWO_PI * phase01 * 0.5f);
                    advance (phase01, dt);
                    v = v * (1.0f - (morph * 0.5f + 0.5f) * 0.4f) + sub * (morph * 0.5f + 0.5f) * 0.4f;
                }
                return v;
            }

            case VoiceEngine::Airwave:
            default:
            {
                float n = pinkNoise.whiteNext();
                float y = airSvf.processBP (n);
                // a touch of pure tone for pitch clarity + optional second whistle
                float tone = std::sin (TWO_PI * phase01);
                advance (phase01, dt);
                float blend = 0.55f + 0.35f * (airQ / 64.0f);
                float out = y * (airQ * 0.06f) * blend + tone * (1.0f - blend) * 0.5f;
                if (airSecond > 0.01f)
                {
                    advance (phaseB, dt * 1.5f);
                    out += std::sin (TWO_PI * phaseB) * airSecond * 0.25f;
                }
                return juce::jlimit (-1.5f, 1.5f, out);
            }
        }
    }

    inline float additiveSample (float ph)
    {
        // sum a_k sin(2*pi*k*ph) via Chebyshev recurrence (cheap, alias-capped K)
        const float theta = juce::MathConstants<float>::twoPi * ph;
        const float c = std::cos (theta);
        float s_prev = 0.0f;                 // sin(0)
        float s_cur  = std::sin (theta);     // sin(1*theta)
        float acc = harmAmp[1] * s_cur;
        for (int k = 2; k <= additiveK; ++k)
        {
            float s_next = 2.0f * c * s_cur - s_prev;
            s_prev = s_cur; s_cur = s_next;
            float kk = (harmStretch > 0.0f) ? (k * (1.0f + harmStretch * k)) : (float) k;
            (void) kk;
            acc += harmAmp[k] * s_cur;
        }
        return acc;
    }

    static inline void advance (float& ph, float dt) { ph += dt; ph -= std::floor (ph); }

    static inline float blepSaw (float ph, float dt)
    {
        float v = 2.0f * ph - 1.0f;
        v -= polyBlep (ph, dt);
        return v;
    }

    static inline float blepOsc (SynthWaveform wf, float ph, float dt)
    {
        switch (wf)
        {
            case SynthWaveform::Sine:     return std::sin (juce::MathConstants<float>::twoPi * ph);
            case SynthWaveform::Triangle: return 4.0f * std::abs (ph - 0.5f) - 1.0f;
            case SynthWaveform::Saw:      return blepSaw (ph, dt);
            case SynthWaveform::Square:
            {
                float v = (ph < 0.5f) ? 1.0f : -1.0f;
                v += polyBlep (ph, dt);
                float ph2 = ph + 0.5f; ph2 -= std::floor (ph2);
                v -= polyBlep (ph2, dt);
                return v;
            }
        }
        return 0.0f;
    }

    //==========================================================================
    double sr = 48000.0;
    VoiceEngine engine = VoiceEngine::Blend;
    bool coeffsDirty = true;

    float phase01 = 0.0f, phaseB = 0.0f, currentHz = 0.0f;
    float morph = -1.0f, timbre = 0.5f;

    // Blend
    SynthWaveform wfL = SynthWaveform::Sine, wfR = SynthWaveform::Square;
    float mix = -1.0f;

    // FM
    float fmRatio = 1.0f, fmIndex = 0.0f;

    // PhaseFB
    float fbAmount = 1.0f, fbColour = 1.0f, fbLast = 0.0f;

    // Additive (Wavetable / Harmonic)
    static constexpr int kMaxHarm = 48;
    float harmAmp[kMaxHarm + 1] = {};
    int   additiveK = 8;
    float harmStretch = 0.0f;

    // Modal
    static constexpr int kModal = 4;
    SvfTPT modalSvf[kModal];
    float  modalGain[kModal] = { 1, 0.6f, 0.4f, 0.25f };
    float  modalDecay = 0.0f;
    float  exciteEnv = 0.0f;
    bool   triggerExcite = false;

    // Swarm
    static constexpr int kSwarm = 7;
    float swarmPhase[kSwarm] = {};

    // Airwave
    SvfTPT airSvf;
    float  airSecond = 0.0f, airQ = 8.0f;

    // Shaping
    float noiseAmt = 0.0f, toneAmt = 0.0f;
    float hpCutoff = 20.0f, lpCutoff = 20000.0f;
    float susLevel = 0.5f;

    PinkNoise pinkNoise;
    SynthFilter hpFilter, lpFilter;

    int lastWaveL = -1, lastWaveR = -1;
    float lastMix = 999.0f;
    float lastA = -1.f, lastD = -1.f, lastS = -1.f, lastR = -1.f;
    juce::ADSR adsr;
};

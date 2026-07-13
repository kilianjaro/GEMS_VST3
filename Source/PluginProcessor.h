#pragma once
#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <cmath>
#include <complex>
#include "SynthModule.h"
#include "FxChain.h"

// GEMS v2 DSP engine (PIMPL): the faithful Cardinal-patch clone. Forward-declared here so the
// heavy vendored headers (braids/plaits/clouds/…) only compile in PluginProcessor.cpp. When
// kUseV2Engine is true, processBlock routes audio through this instead of the v1 engine.
#include <memory>
namespace gemsv2 { class V2Engine; }

//==============================================================================
// Parameter-IDs (Bands)
static inline juce::String bandOnId   (int band) { return "b" + juce::String (band + 1) + "On";    }
static inline juce::String bandFreqId (int band) { return "b" + juce::String (band + 1) + "Freq";  }
static inline juce::String bandResId  (int band) { return "b" + juce::String (band + 1) + "Res";   }
static inline juce::String bandPoleId (int band) { return "b" + juce::String (band + 1) + "Poles"; }

// Impulse Module params (pro Band)
static inline juce::String bandGateId (int band) { return "b" + juce::String (band + 1) + "Gate";  } // dB
static inline juce::String bandSensId (int band) { return "b" + juce::String (band + 1) + "Sens";  } // 0..1
static inline juce::String bandTolId  (int band) { return "b" + juce::String (band + 1) + "Tol";   } // 0..1
static inline juce::String bandConfId (int band) { return "b" + juce::String (band + 1) + "Conf";  } // 0..1

// Global Scale + Mix
static constexpr const char* kScaleRootId  = "scaleRoot";   // 0..11
static constexpr const char* kScaleMinorId = "scaleMinor";  // bool (true = minor)
static constexpr const char* kDryWetId     = "dryWet";      // 0..1 (0=dry only, 1=wet only)
static constexpr const char* kWidthId = "width";             // 0..1 (0=mono synth, 1=stereo pan)
static constexpr const char* kMidiOutId = "midiOut";         // bool (true = MIDI output mode)

// Quantizer params (per Band)
static inline juce::String bandSnapId     (int band) { return "b" + juce::String (band + 1) + "Snap"; }     // -1..+1
static inline juce::String bandNoteMaskId (int band) { return "b" + juce::String (band + 1) + "Notes"; }    // 0..4095 (12-bit)
static inline juce::String bandOctId      (int band) { return "b" + juce::String (band + 1) + "Oct"; }      // -4..+4

// Mixer params (per Band)
static inline juce::String bandVolId (int band) { return "b" + juce::String (band + 1) + "Vol"; }           // 0..2 (gain)

//==============================================================================
// Synth params (per Band/Track)
static inline juce::String bandWaveMixId (int band) { return "b" + juce::String (band + 1) + "WaveMix"; } // -1..+1
static inline juce::String bandWaveLId   (int band) { return "b" + juce::String (band + 1) + "WaveL"; }   // choice 0..3
static inline juce::String bandWaveRId   (int band) { return "b" + juce::String (band + 1) + "WaveR"; }   // choice 0..3
static inline juce::String bandAtkId     (int band) { return "b" + juce::String (band + 1) + "Atk"; }     // 0..8s
static inline juce::String bandDecId     (int band) { return "b" + juce::String (band + 1) + "Dec"; }     // 0..8s
static inline juce::String bandSusId     (int band) { return "b" + juce::String (band + 1) + "Sus"; }     // 0..1
static inline juce::String bandRelId     (int band) { return "b" + juce::String (band + 1) + "Rel"; }     // 0..8s

// Synth shaping per band
static inline juce::String bandNoiseId   (int band) { return "b" + juce::String (band + 1) + "Noise"; }   // 0..1
static inline juce::String bandToneId    (int band) { return "b" + juce::String (band + 1) + "Tone"; }    // 0..1
static inline juce::String bandHPId      (int band) { return "b" + juce::String (band + 1) + "HP"; }      // 20..20000 Hz
static inline juce::String bandLPId      (int band) { return "b" + juce::String (band + 1) + "LP"; }      // 20..20000 Hz

// Voice engine + macros per band
static inline juce::String bandEngineId  (int band) { return "b" + juce::String (band + 1) + "Engine"; }  // choice 0..7
static inline juce::String bandTimbreId  (int band) { return "b" + juce::String (band + 1) + "Timbre"; }  // 0..1

// Global generative + space
static constexpr const char* kReverbId = "reverb";   // 0..1 wet
static constexpr const char* kRandomId = "random";   // 0..1 generative amount

// Master FX chain
static constexpr const char* kDriveId     = "fxDrive";     // 0..1 tape/saturation
static constexpr const char* kDelayMixId  = "fxDelayMix";  // 0..1
static constexpr const char* kDelayTimeId = "fxDelayTime"; // ms
static constexpr const char* kDelayFbId   = "fxDelayFb";   // 0..0.95
static constexpr const char* kCompId      = "fxComp";      // 0..1 glue

//==============================================================================
// Bandpass-Filterbank (8 Bands, max 12 Poles -> 6 BiQuad Sektionen), Stereo
// WICHTIG:
// - echte Band-Outputs (pro Band & pro Channel) werden im Processor in bandBuffers geschrieben
// - coeff updates resetten NICHT dauerhaft (nur neu hinzugefügte Sektionen bei Slope-Increase werden reset)
//
class BandpassBank
{
public:
    static constexpr int kMaxBands    = 8;
    static constexpr int kMaxPoles    = 12;
    static constexpr int kMaxSections = kMaxPoles / 2; // 6
    static constexpr int kMaxChannels = 2;

    void prepare (double sampleRate, int channels)
    {
        sr = sampleRate;
        numChannels = juce::jlimit (1, kMaxChannels, channels);

        for (int b = 0; b < kMaxBands; ++b)
        {
            lastFreq[b]  = -1.0f;
            lastRes[b]   = -1.0f;
            lastPoles[b] = -1;

            sectionsUsed[b]     = 1;
            prevSectionsUsed[b] = 1;
            gainComp[b] = 1.0f;

            for (int ch = 0; ch < kMaxChannels; ++ch)
                for (int s = 0; s < kMaxSections; ++s)
                    filters[b][ch][s].reset();
        }
    }

    static int polesToSections (int poles)
    {
        poles = juce::jlimit (1, kMaxPoles, poles);
        return (poles + 1) / 2; // 1-2->1, 3-4->2, ... 11-12->6
    }

    void updateBandIfNeeded (int band, float freqHz, float resonance01, int poles)
    {
        if (sr <= 0.0 || band < 0 || band >= kMaxBands)
            return;

        const float fMax = juce::jmin (20000.0f, (float) (sr * 0.45)); // stay below Nyquist
        freqHz      = juce::jlimit (20.0f, fMax, freqHz);
        resonance01 = juce::jlimit (0.0f, 1.0f, resonance01);
        poles       = juce::jlimit (1, kMaxPoles, poles);

        const bool changed =
            (std::abs (freqHz - lastFreq[band]) > 0.01f) ||
            (std::abs (resonance01 - lastRes[band]) > 0.0005f) ||
            (poles != lastPoles[band]);

        if (! changed)
            return;

        lastFreq[band]  = freqHz;
        lastRes[band]   = resonance01;
        lastPoles[band] = poles;

        const int newSections = polesToSections (poles);
        prevSectionsUsed[band] = sectionsUsed[band];
        sectionsUsed[band] = newSections;

        // Resonanz 0..1 -> Q
        const float Q = 0.5f * std::pow (50.0f, resonance01); // ~0.5 .. 25

        // einfache Kompensation (verhindert "exploding peaks" bei vielen Sektionen)
        if (newSections > 1)
            gainComp[band] = 1.0f / std::pow (juce::jmax (Q, 0.5f), (float) (newSections - 1));
        else
            gainComp[band] = 1.0f;

        gainComp[band] = juce::jlimit (0.02f, 1.0f, gainComp[band]);

        auto coeff = juce::dsp::IIR::Coefficients<float>::makeBandPass (sr, freqHz, Q);

        // KEIN reset() bei normalen Updates -> Filterzustand bleibt stabil
        for (int ch = 0; ch < numChannels; ++ch)
            for (int s = 0; s < kMaxSections; ++s)
                filters[band][ch][s].coefficients = coeff;

        // nur neue Sektionen resetten (wenn Slope erhöht wurde)
        if (newSections > prevSectionsUsed[band])
        {
            for (int ch = 0; ch < numChannels; ++ch)
                for (int s = prevSectionsUsed[band]; s < newSections; ++s)
                    filters[band][ch][s].reset();
        }
    }

    inline float processSample (int band, int ch, float x)
    {
        const int sections = sectionsUsed[band];
        float y = x;

        for (int s = 0; s < sections; ++s)
            y = filters[band][ch][s].processSample (y);

        return y * gainComp[band];
    }

private:
    double sr = 0.0;
    int numChannels = 1;

    juce::dsp::IIR::Filter<float> filters[kMaxBands][kMaxChannels][kMaxSections];
    int   sectionsUsed[kMaxBands]     = { 1,1,1,1,1,1,1,1 };
    int   prevSectionsUsed[kMaxBands] = { 1,1,1,1,1,1,1,1 };
    float gainComp[kMaxBands]         = { 1,1,1,1,1,1,1,1 };

    float lastFreq[kMaxBands]  = {};
    float lastRes[kMaxBands]   = {};
    int   lastPoles[kMaxBands] = {};
};

//==============================================================================
// Spectrum (Input FFT), log-x mapping, dB y-range (-50..+6)
class SpectrumAnalyser
{
public:
    static constexpr int   fftOrder  = 11;                 // 2048
    static constexpr int   fftSize   = 1 << fftOrder;
    static constexpr int   scopeSize = 256;
    static constexpr int   hopSize   = fftSize / 4;        // overlap
    static constexpr float minDb     = -50.0f;
    static constexpr float maxDb     =  6.0f;

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        writeIndex = 0;
        hopCounter = 0;

        fifo.fill (0.0f);
        fftData.fill (0.0f);
        for (auto& v : scopeDb) v.store (minDb);

        computeScopeBinMap();
    }

    void pushSample (float x)
    {
        if (sr <= 0.0) return;

        fifo[(size_t) writeIndex] = x;
        writeIndex = (writeIndex + 1) % fftSize;

        if (++hopCounter >= hopSize)
        {
            hopCounter = 0;
            computeFrame();
        }
    }

    float getScopeDb (int i) const
    {
        i = juce::jlimit (0, scopeSize - 1, i);
        return scopeDb[(size_t) i].load();
    }

private:
    void computeScopeBinMap()
    {
        if (sr <= 0.0) return;

        const double minF = 20.0, maxF = 20000.0;
        const double ratio = maxF / minF;
        const double binHz = sr / (double) fftSize;

        for (int i = 0; i < scopeSize; ++i)
        {
            const double t = (double) i / (double) (scopeSize - 1);
            const double f = minF * std::pow (ratio, t);
            const double bin = juce::jlimit (0.0, (double) (fftSize / 2 - 1), f / binHz);
            scopeBin[(size_t) i] = (float) bin;
        }
    }

    inline float magAtBinInt (int bin) const
    {
        if (bin <= 0) return std::abs (fftData[0]);
        if (bin >= fftSize / 2) return std::abs (fftData[1]);

        const float re = fftData[(size_t) (2 * bin)];
        const float im = fftData[(size_t) (2 * bin + 1)];
        return std::sqrt (re * re + im * im);
    }

    inline float magAtBinFloat (float binF) const
    {
        const int bin0 = (int) std::floor (binF);
        const int bin1 = juce::jmin (bin0 + 1, fftSize / 2 - 1);
        const float frac = binF - (float) bin0;
        return magAtBinInt (bin0) + frac * (magAtBinInt (bin1) - magAtBinInt (bin0));
    }

    void computeFrame()
    {
        for (int i = 0; i < fftSize; ++i)
            fftData[(size_t) i] = fifo[(size_t) ((writeIndex + i) % fftSize)];

        std::fill (fftData.begin() + fftSize, fftData.end(), 0.0f);

        window.multiplyWithWindowingTable (fftData.data(), fftSize);
        fft.performRealOnlyForwardTransform (fftData.data());

        const float magScale = 4.0f / (float) fftSize;

        for (int i = 0; i < scopeSize; ++i)
        {
            float mag = magAtBinFloat (scopeBin[(size_t) i]) * magScale;
            mag = juce::jmax (mag, 1.0e-9f);

            float db = juce::Decibels::gainToDecibels (mag);
            db = juce::jlimit (minDb, maxDb, db);

            const float old = scopeDb[(size_t) i].load();
            const float sm  = 0.65f * old + 0.35f * db;
            scopeDb[(size_t) i].store (sm);
        }
    }

    double sr = 0.0;

    juce::dsp::FFT fft { fftOrder };
    juce::dsp::WindowingFunction<float> window { fftSize, juce::dsp::WindowingFunction<float>::hann };

    std::array<float, fftSize> fifo {};
    std::array<float, fftSize * 2> fftData {};
    int writeIndex = 0;
    int hopCounter = 0;

    std::array<float, scopeSize> scopeBin {};
    std::array<std::atomic<float>, scopeSize> scopeDb {};
};

//==============================================================================
// Pitch Detector (FFT peak, lightweight)
class PitchDetectorFFT
{
public:
    static constexpr int fftOrder = 11; // 2048
    static constexpr int fftSize  = 1 << fftOrder;
    static constexpr int hopSize  = fftSize / 4;

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        writeIndex = 0;
        hopCounter = 0;
        lastHz = 0.0f;
        ring.fill (0.0f);
        fftData.fill (0.0f);
    }

    void reset()
    {
        writeIndex = 0;
        hopCounter = 0;
        lastHz = 0.0f;
        ring.fill (0.0f);
    }

    bool process (float x, bool gateOpen, float expectedHz,
                  float tol01, float sens01, float confThresh01,
                  float& outHz, float& outConf01)
    {
        outHz = lastHz;
        outConf01 = 0.0f;

        ring[(size_t) writeIndex] = x;
        writeIndex = (writeIndex + 1) % fftSize;

        if (!gateOpen)
        {
            if (++hopCounter >= hopSize) hopCounter = 0;
            return false;
        }

        if (++hopCounter < hopSize)
            return false;

        hopCounter = 0;

        for (int i = 0; i < fftSize; ++i)
            fftData[(size_t) i] = ring[(size_t) ((writeIndex + i) % fftSize)];

        std::fill (fftData.begin() + fftSize, fftData.end(), 0.0f);

        window.multiplyWithWindowingTable (fftData.data(), fftSize);
        fft.performFrequencyOnlyForwardTransform (fftData.data());

        const float tolSemis = 1.0f + tol01 * 23.0f;
        const float factor = std::pow (2.0f, tolSemis / 12.0f);

        float minHz = juce::jlimit (30.0f, 8000.0f, expectedHz / factor);
        float maxHz = juce::jlimit (30.0f, 8000.0f, expectedHz * factor);
        if (maxHz <= minHz) { minHz = 30.0f; maxHz = 8000.0f; }

        const int minBin = juce::jlimit (1, fftSize / 2 - 2, (int) std::floor (minHz * (float) fftSize / (float) sr));
        const int maxBin = juce::jlimit (minBin + 1, fftSize / 2 - 2, (int) std::ceil  (maxHz * (float) fftSize / (float) sr));

        float peak = 0.0f;
        int peakBin = minBin;

        double sum = 0.0;
        int count = 0;

        for (int b = minBin; b <= maxBin; ++b)
        {
            const float m = fftData[(size_t) b];
            sum += m;
            ++count;
            if (m > peak) { peak = m; peakBin = b; }
        }

        const float avg = (count > 0) ? (float) (sum / (double) count) : 0.0f;
        const float ratio = peak / (avg + 1.0e-9f);

        float conf = (ratio - 1.5f) / (18.0f - 1.5f);
        conf = juce::jlimit (0.0f, 1.0f, conf);
        outConf01 = conf;

        if (conf < confThresh01)
            return false;

        const float a = 0.05f + 0.90f * sens01;
        const float hz = (float) peakBin * (float) sr / (float) fftSize;
        lastHz = (lastHz <= 0.0f) ? hz : (1.0f - a) * lastHz + a * hz;

        outHz = lastHz;
        return true;
    }

private:
    double sr = 48000.0;

    juce::dsp::FFT fft { fftOrder };
    juce::dsp::WindowingFunction<float> window { fftSize, juce::dsp::WindowingFunction<float>::hann };

    std::array<float, fftSize> ring {};
    std::array<float, fftSize * 2> fftData {};
    int writeIndex = 0;
    int hopCounter = 0;

    float lastHz = 0.0f;
};

//==============================================================================
// Quantizer (global scale)
struct ScaleQuantizer
{
    // Returns true if pitch class (0=C .. 11=B) belongs to the given scale
    static bool isNoteInScale (int pitchClass, int root, bool minor)
    {
        return (scaleToBitmask (root, minor) >> pitchClass) & 1;
    }

    // Returns a 12-bit mask for the given scale (bit 0 = C, bit 11 = B)
    static int scaleToBitmask (int root, bool minor)
    {
        static constexpr int majorSteps[7] = { 0,2,4,5,7,9,11 };
        static constexpr int minorSteps[7] = { 0,2,3,5,7,8,10 };
        const int* steps = minor ? minorSteps : majorSteps;
        int mask = 0;
        for (int i = 0; i < 7; ++i)
            mask |= (1 << ((root + steps[i]) % 12));
        return mask;
    }

    // Quantize Hz using a 12-bit note mask
    static float quantizeHzWithMask (float hz, int noteMask)
    {
        if (hz <= 0.0f || noteMask == 0) return hz;

        const float midi = 69.0f + 12.0f * std::log2 (hz / 440.0f);
        const float baseOct = std::floor (midi / 12.0f) * 12.0f;

        float best = midi;
        float bestDist = 1.0e9f;

        for (int oct = -1; oct <= 1; ++oct)
        {
            const float octBase = baseOct + 12.0f * (float) oct;
            for (int pc = 0; pc < 12; ++pc)
            {
                if (! ((noteMask >> pc) & 1)) continue;
                const float cand = octBase + (float) pc;
                const float d = std::abs (cand - midi);
                if (d < bestDist) { bestDist = d; best = cand; }
            }
        }

        return 440.0f * std::pow (2.0f, (best - 69.0f) / 12.0f);
    }

    // Legacy: quantize using root + minor scale
    static float quantizeHz (float hz, int root, bool minor)
    {
        return quantizeHzWithMask (hz, scaleToBitmask (root, minor));
    }
};

//==============================================================================

class NewProjectAudioProcessor  : public juce::AudioProcessor
{
public:
    NewProjectAudioProcessor();
    ~NewProjectAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlockExpected) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    bool  isBandEnabled (int band) const;
    float getBandFreqHz (int band) const;
    float getBandRes01  (int band) const;
    int   getBandPoles  (int band) const;
    
    float getBandGateDb (int band) const
    { band = juce::jlimit (0, 7, band); return pGateDb[band]->load(); }

    float getScopeDb (int i) const { return spectrum.getScopeDb (i); }

    const juce::AudioBuffer<float>& getBandBuffer (int band) const
    {
        band = juce::jlimit (0, BandpassBank::kMaxBands - 1, band);
        return bandBuffers[band];
    }

    // Note glow: 0..1 per pitch class (0=C, 1=C#, ... 11=B)
    // Updated per audio block; decays in processBlock.
    float getNoteGlow (int pitchClass) const
    {
        pitchClass = juce::jlimit (0, 11, pitchClass);
        return noteGlow[(size_t) pitchClass].load (std::memory_order_relaxed);
    }

    // Per-band stereo levels for mixer meters (0..1, smoothed)
    float getBandLevelL (int band) const
    {
        band = juce::jlimit (0, BandpassBank::kMaxBands - 1, band);
        return meterL[(size_t) band].load (std::memory_order_relaxed);
    }
    float getBandLevelR (int band) const
    {
        band = juce::jlimit (0, BandpassBank::kMaxBands - 1, band);
        return meterR[(size_t) band].load (std::memory_order_relaxed);
    }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // band enable + filter params
    std::atomic<float>* pBandOn  [BandpassBank::kMaxBands] {};
    std::atomic<float>* pBandFreq[BandpassBank::kMaxBands] {};
    std::atomic<float>* pBandRes [BandpassBank::kMaxBands] {};
    std::atomic<float>* pBandPole[BandpassBank::kMaxBands] {};

    // impulse params per band
    std::atomic<float>* pGateDb[BandpassBank::kMaxBands] {};
    std::atomic<float>* pSens01[BandpassBank::kMaxBands] {};
    std::atomic<float>* pTol01 [BandpassBank::kMaxBands] {};
    std::atomic<float>* pConf01[BandpassBank::kMaxBands] {};

    // global scale + mix
    std::atomic<float>* pScaleRoot  = nullptr;
    std::atomic<float>* pScaleMinor = nullptr;
    std::atomic<float>* pDryWet     = nullptr;
    std::atomic<float>* pWidth        = nullptr;
    std::atomic<float>* pMidiOut      = nullptr;

    // quantizer per band
    std::atomic<float>* pSnap[BandpassBank::kMaxBands] {};
    std::atomic<float>* pNoteMask[BandpassBank::kMaxBands] {};
    std::atomic<float>* pOct[BandpassBank::kMaxBands] {};

    // mixer per band
    std::atomic<float>* pVol[BandpassBank::kMaxBands] {};


    // synth params per band
    std::atomic<float>* pWaveMix[BandpassBank::kMaxBands] {};
    std::atomic<float>* pWaveL  [BandpassBank::kMaxBands] {};
    std::atomic<float>* pWaveR  [BandpassBank::kMaxBands] {};
    std::atomic<float>* pAtk    [BandpassBank::kMaxBands] {};
    std::atomic<float>* pDec    [BandpassBank::kMaxBands] {};
    std::atomic<float>* pSus    [BandpassBank::kMaxBands] {};
    std::atomic<float>* pRel    [BandpassBank::kMaxBands] {};

    // Synth shaping per band
    std::atomic<float>* pNoise  [BandpassBank::kMaxBands] {};
    std::atomic<float>* pTone   [BandpassBank::kMaxBands] {};
    std::atomic<float>* pHP     [BandpassBank::kMaxBands] {};
    std::atomic<float>* pLP     [BandpassBank::kMaxBands] {};

    // Voice engine + macros per band
    std::atomic<float>* pEngine [BandpassBank::kMaxBands] {};
    std::atomic<float>* pTimbre [BandpassBank::kMaxBands] {};

    // Global generative + space
    std::atomic<float>* pReverb = nullptr;
    std::atomic<float>* pRandom = nullptr;

    // Master FX chain
    std::atomic<float>* pDrive     = nullptr;
    std::atomic<float>* pDelayMix  = nullptr;
    std::atomic<float>* pDelayTime = nullptr;
    std::atomic<float>* pDelayFb   = nullptr;
    std::atomic<float>* pComp      = nullptr;

    MasterFx     masterFx;
    juce::Random rng;

    BandpassBank bank;
    SpectrumAnalyser spectrum;

    juce::AudioBuffer<float> bandBuffers[BandpassBank::kMaxBands];
    juce::AudioBuffer<float> synthBus;   // wet synth voices, pre-reverb
    int preparedMaxBlock = 0;
    int preparedChannels = 0;

    struct BandImpulseState
    {
        bool  wasEnabled = false;

        // mono envelope for gate / drawing
        float env  = 0.0f;

        // stereo envelopes for pan detection
        float envL = 0.0f;
        float envR = 0.0f;

        bool  gateOpen = false;

        // Velocity (near-original): short-time RMS during onset window
        double velSumSq = 0.0;
        double panSumL2 = 0.0;
        double panSumR2 = 0.0;
        int    velCount = 0;
        bool   velLocked = false;

        float  velHoldRms = 0.0f;  // held per impulse
        float  panHold    = 0.0f;  // -1..+1, held per impulse
        float  lastConf   = 0.0f;  // last detected confidence (0..1)

        float oscHz = 220.0f;

        // Generative per-note state (set on each onset from the Random macro)
        float randDetune = 1.0f;   // pitch multiplier (cents jitter)
        float randVel    = 1.0f;   // velocity multiplier
        bool  noteActive = true;   // false = probabilistically skipped (sparse)

        bool  hasQuant = false;
        float lastQuantHz = 0.0f;
        float lastQuantMidi = 0.0f;

        TrackSynth synth;
        PitchDetectorFFT pitch;
    };

    BandImpulseState impulse[BandpassBank::kMaxBands];
    double sr = 48000.0;

    // MIDI OUT mode per-band tracking
    bool  midiGateWasOpen[BandpassBank::kMaxBands] {};
    int   midiLastNote[BandpassBank::kMaxBands] {};
    float midiLastVel[BandpassBank::kMaxBands] {};

    // Per-pitch-class glow (0..1), for UI visualisation of active quantizer notes
    std::array<std::atomic<float>, 12> noteGlow {};

    // Per-band stereo meters (smoothed peak, 0..1)
    std::array<std::atomic<float>, BandpassBank::kMaxBands> meterL {};
    std::array<std::atomic<float>, BandpassBank::kMaxBands> meterR {};

    // Per-band visualization data (written in processBlock, read by UI)
    std::array<std::atomic<float>, BandpassBank::kMaxBands> vizOscHz {};
    std::array<std::atomic<float>, BandpassBank::kMaxBands> vizConf {};
    std::array<std::atomic<float>, BandpassBank::kMaxBands> vizEnvLevel {};
    std::array<std::atomic<int>,   BandpassBank::kMaxBands> vizGateOpen {};

public:
    float getBandOscHz    (int b) const { b = juce::jlimit(0,7,b); return vizOscHz[(size_t)b].load(std::memory_order_relaxed); }
    float getBandConfViz  (int b) const { b = juce::jlimit(0,7,b); return vizConf[(size_t)b].load(std::memory_order_relaxed); }
    float getBandEnvLevel (int b) const { b = juce::jlimit(0,7,b); return vizEnvLevel[(size_t)b].load(std::memory_order_relaxed); }
    bool  getBandGateOpen (int b) const { b = juce::jlimit(0,7,b); return vizGateOpen[(size_t)b].load(std::memory_order_relaxed) != 0; }

    int   getBandEngine   (int b) const { b = juce::jlimit(0,7,b); return pEngine[b] ? (int) std::round (pEngine[b]->load()) : 0; }

    // Apply one of the built-in presets (0=Rainforest,1=Waves,2=Desert,3=Noise)
    void applyPreset (int index);
    static constexpr int kNumPresets = 4;
    static const char* presetName (int index);

    // User presets (stored as .gemspreset XML in the app-data folder)
    static juce::File   getUserPresetDir();
    juce::StringArray   getUserPresetNames() const;
    bool                saveUserPreset (const juce::String& name);
    bool                loadUserPreset (const juce::String& name);

    bool isMidiOutMode() const { return pMidiOut && pMidiOut->load() >= 0.5f; }
    double getSampleRate() const { return sr; }
    static constexpr int kScopeSize = SpectrumAnalyser::scopeSize;
    static constexpr float kMinDb = SpectrumAnalyser::minDb;
    static constexpr float kMaxDb = SpectrumAnalyser::maxDb;
private:

    // GEMS v2: when true, processBlock runs the faithful patch clone (v2) instead of the v1
    // engine. Kept as a flag so v1 stays available for A/B during development.
    static constexpr bool kUseV2Engine = true;
    std::unique_ptr<gemsv2::V2Engine> v2Engine;   // allocated in prepareToPlay

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NewProjectAudioProcessor)
};

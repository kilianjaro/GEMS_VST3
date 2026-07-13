#include "PluginProcessor.h"
#include "PluginEditor.h"

// GEMS v2 engine — the faithful Cardinal-patch clone. Included here (only) so its heavy
// vendored DSP headers compile in this one translation unit. See Source/v2/ and NOTICE.md.
#include "v2/V2Engine.h"

// Cardinal HostAudio level convention: host audio ±1.0 ↔ ±10 V inside the patch (that ±10 V
// full-scale is exactly why the master bus clamps at ±10 V). If a level A/B against Cardinal
// ever shows a constant gain offset, these two constants are the single knob to adjust.
static constexpr float kAudioToVolts = 10.0f;   // host sample → patch volts (input)
static constexpr float kVoltsToAudio = 0.1f;    // patch volts → host sample (output)

//==============================================================================
NewProjectAudioProcessor::NewProjectAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
    , apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
    for (int b = 0; b < BandpassBank::kMaxBands; ++b)
    {
        pBandOn[b]   = apvts.getRawParameterValue (bandOnId   (b));
        pBandFreq[b] = apvts.getRawParameterValue (bandFreqId (b));
        pBandRes [b] = apvts.getRawParameterValue (bandResId  (b));
        pBandPole[b] = apvts.getRawParameterValue (bandPoleId (b));

        pGateDb[b] = apvts.getRawParameterValue (bandGateId (b));
        pSens01[b] = apvts.getRawParameterValue (bandSensId (b));
        pTol01 [b] = apvts.getRawParameterValue (bandTolId  (b));
        pConf01[b] = apvts.getRawParameterValue (bandConfId (b));

        // synth params
        pWaveMix[b] = apvts.getRawParameterValue (bandWaveMixId (b));
        pWaveL  [b] = apvts.getRawParameterValue (bandWaveLId   (b));
        pWaveR  [b] = apvts.getRawParameterValue (bandWaveRId   (b));
        pAtk    [b] = apvts.getRawParameterValue (bandAtkId     (b));
        pDec    [b] = apvts.getRawParameterValue (bandDecId     (b));
        pSus    [b] = apvts.getRawParameterValue (bandSusId     (b));
        pRel    [b] = apvts.getRawParameterValue (bandRelId     (b));
    }

    pScaleRoot  = apvts.getRawParameterValue (kScaleRootId);
    pScaleMinor = apvts.getRawParameterValue (kScaleMinorId);
    pDryWet     = apvts.getRawParameterValue (kDryWetId);
    pWidth = apvts.getRawParameterValue (kWidthId);
    pMidiOut = apvts.getRawParameterValue (kMidiOutId);

    for (int b = 0; b < BandpassBank::kMaxBands; ++b)
    {
        pSnap[b]     = apvts.getRawParameterValue (bandSnapId (b));
        pNoteMask[b] = apvts.getRawParameterValue (bandNoteMaskId (b));
        pOct[b]      = apvts.getRawParameterValue (bandOctId (b));
        pVol[b]      = apvts.getRawParameterValue (bandVolId (b));
        pNoise[b]    = apvts.getRawParameterValue (bandNoiseId (b));
        pTone[b]     = apvts.getRawParameterValue (bandToneId (b));
        pHP[b]       = apvts.getRawParameterValue (bandHPId (b));
        pLP[b]       = apvts.getRawParameterValue (bandLPId (b));
        pEngine[b]   = apvts.getRawParameterValue (bandEngineId (b));
        pTimbre[b]   = apvts.getRawParameterValue (bandTimbreId (b));
    }

    pReverb = apvts.getRawParameterValue (kReverbId);
    pRandom = apvts.getRawParameterValue (kRandomId);

    pDrive     = apvts.getRawParameterValue (kDriveId);
    pDelayMix  = apvts.getRawParameterValue (kDelayMixId);
    pDelayTime = apvts.getRawParameterValue (kDelayTimeId);
    pDelayFb   = apvts.getRawParameterValue (kDelayFbId);
    pComp      = apvts.getRawParameterValue (kCompId);
}

NewProjectAudioProcessor::~NewProjectAudioProcessor() {}

//==============================================================================
const juce::String NewProjectAudioProcessor::getName() const { return JucePlugin_Name; }
bool NewProjectAudioProcessor::acceptsMidi() const  { return false; }
bool NewProjectAudioProcessor::producesMidi() const { return true; }
bool NewProjectAudioProcessor::isMidiEffect() const { return false; }
double NewProjectAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int NewProjectAudioProcessor::getNumPrograms() { return 1; }
int NewProjectAudioProcessor::getCurrentProgram() { return 0; }
void NewProjectAudioProcessor::setCurrentProgram (int) {}
const juce::String NewProjectAudioProcessor::getProgramName (int) { return {}; }
void NewProjectAudioProcessor::changeProgramName (int, const juce::String&) {}

//==============================================================================
void NewProjectAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlockExpected)
{
    sr = sampleRate;

    // GEMS v2: (re)allocate and prepare the clone engine. All of its internal buffers are
    // sized here (RT-safe: no allocation on the audio thread thereafter).
    if (kUseV2Engine)
    {
        if (! v2Engine)
            v2Engine = std::make_unique<gemsv2::V2Engine>();
        v2Engine->prepare (sampleRate);
    }

    preparedChannels = juce::jlimit (1, 2, (int) getTotalNumInputChannels());
    preparedMaxBlock = juce::jmax (1, samplesPerBlockExpected);

    bank.prepare (sampleRate, preparedChannels);
    spectrum.prepare (sampleRate);

    masterFx.prepare (sampleRate, preparedMaxBlock);

    synthBus.setSize (2, preparedMaxBlock, false, false, true);
    synthBus.clear();

    for (int b = 0; b < BandpassBank::kMaxBands; ++b)
    {
        bandBuffers[b].setSize (preparedChannels, preparedMaxBlock, false, false, true);
        bandBuffers[b].clear();

        impulse[b].wasEnabled = false;
        impulse[b].randDetune = 1.0f; impulse[b].randVel = 1.0f; impulse[b].noteActive = true;
        impulse[b].env = 0.0f;
        impulse[b].gateOpen = false;
        
        impulse[b].envL = 0.0f;
        impulse[b].envR = 0.0f;

        impulse[b].velSumSq  = 0.0;
        impulse[b].panSumL2  = 0.0;
        impulse[b].panSumR2  = 0.0;
        impulse[b].velCount  = 0;
        impulse[b].velLocked = false;
        impulse[b].velHoldRms = 0.0f;
        impulse[b].panHold    = 0.0f;
        
        impulse[b].oscHz = 220.0f;

        impulse[b].pitch.prepare (sampleRate);
        impulse[b].synth.prepare (sampleRate);
        impulse[b].synth.reset();
        impulse[b].synth.setWaveParams ((int) std::round (pWaveL[b]->load()),
                                      (int) std::round (pWaveR[b]->load()),
                                      pWaveMix[b]->load());
        impulse[b].synth.setAdsrParams (pAtk[b]->load(), pDec[b]->load(), pSus[b]->load(), pRel[b]->load());
        impulse[b].synth.setEngine ((int) std::round (pEngine[b]->load()));
        impulse[b].synth.setMacros (pWaveMix[b]->load(), pTimbre[b]->load());
    }

    for (int b = 0; b < BandpassBank::kMaxBands; ++b)
        bank.updateBandIfNeeded (b, getBandFreqHz (b), getBandRes01 (b), getBandPoles (b));
}

void NewProjectAudioProcessor::releaseResources() {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool NewProjectAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
}
#endif

//==============================================================================
void NewProjectAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    midiMessages.clear();

    const bool midiOutMode = pMidiOut && pMidiOut->load() >= 0.5f;

    const int numSamples = buffer.getNumSamples();
    const int numInCh    = juce::jlimit (1, 2, (int) getTotalNumInputChannels());
    const int numOutCh   = (int) getTotalNumOutputChannels();

    for (int ch = numInCh; ch < numOutCh; ++ch)
        buffer.clear (ch, 0, numSamples);

    // GEMS v2: faithful Cardinal-patch clone path. Analysis is L-only (matches the patch),
    // so we drive the engine from channel 0 and write the stereo master back out. This
    // bypasses the v1 engine entirely. MIDI-out and other additive features come later.
    if (kUseV2Engine && v2Engine != nullptr)
    {
        const float* in   = buffer.getReadPointer (0);
        float*       outL = buffer.getWritePointer (0);
        float*       outR = numOutCh > 1 ? buffer.getWritePointer (1) : nullptr;
        for (int i = 0; i < numSamples; ++i)
        {
            v2Engine->processSample (in[i] * kAudioToVolts);   // read in[i] before overwriting
            outL[i] = v2Engine->masterL() * kVoltsToAudio;
            if (outR != nullptr) outR[i] = v2Engine->masterR() * kVoltsToAudio;
        }
        return;
    }

    if (numSamples > preparedMaxBlock || numInCh != preparedChannels)
    {
        preparedChannels = numInCh;
        preparedMaxBlock = juce::jmax (preparedMaxBlock, numSamples);

        bank.prepare (getSampleRate(), preparedChannels);

        for (int b = 0; b < BandpassBank::kMaxBands; ++b)
            bandBuffers[b].setSize (preparedChannels, preparedMaxBlock, false, false, true);

        synthBus.setSize (2, preparedMaxBlock, false, false, true);

        for (int b = 0; b < BandpassBank::kMaxBands; ++b)
            bank.updateBandIfNeeded (b, getBandFreqHz (b), getBandRes01 (b), getBandPoles (b));
    }

    synthBus.clear (0, numSamples);

    for (int b = 0; b < BandpassBank::kMaxBands; ++b)
        for (int ch = 0; ch < preparedChannels; ++ch)
            bandBuffers[b].clear (ch, 0, numSamples);

    const float wet   = juce::jlimit (0.0f, 1.0f, pDryWet->load());
    const float dry   = 1.0f - wet;
    const float width = juce::jlimit (0.0f, 1.0f, pWidth->load()); // 0=mono synth, 1=full stereo pan

    const float synthGain = 0.18f;


// --- Update synth params once per block (cached internally, so this is cheap)
for (int b = 0; b < BandpassBank::kMaxBands; ++b)
{
    if (pWaveL[b] != nullptr && pWaveR[b] != nullptr && pWaveMix[b] != nullptr)
        impulse[b].synth.setWaveParams ((int) std::round (pWaveL[b]->load()),
                                        (int) std::round (pWaveR[b]->load()),
                                        pWaveMix[b]->load());

    if (pAtk[b] != nullptr && pDec[b] != nullptr && pSus[b] != nullptr && pRel[b] != nullptr)
        impulse[b].synth.setAdsrParams (pAtk[b]->load(), pDec[b]->load(), pSus[b]->load(), pRel[b]->load());

    if (pEngine[b] != nullptr) impulse[b].synth.setEngine ((int) std::round (pEngine[b]->load()));
    if (pTimbre[b] != nullptr && pWaveMix[b] != nullptr)
        impulse[b].synth.setMacros (pWaveMix[b]->load(), pTimbre[b]->load());
}

// Generative "Random" macro: amount of humanise/spray applied at note onsets.
const float randomAmt = pRandom ? juce::jlimit (0.0f, 1.0f, pRandom->load()) : 0.0f;


    // env follower coeffs
    const float att = std::exp (-1.0f / (0.005f * (float) sr));
    const float rel = std::exp (-1.0f / (0.060f * (float) sr));

    // Velocity/Pan measurement window (onset RMS): ~40ms
    const int velWindow = juce::jmax (16, (int) std::round (0.040 * sr));

    auto lockVelPan = [&] (int b)
    {
        auto& st = impulse[b];
        if (st.velLocked)
            return;

        if (st.velCount <= 0)
        {
            st.velHoldRms = 0.0f;
            st.panHold    = 0.0f;
            st.velLocked  = true;
            return;
        }

        const double invN = 1.0 / (double) st.velCount;

        st.velHoldRms = (float) std::sqrt (st.velSumSq * invN);

        const double denom = st.panSumL2 + st.panSumR2 + 1.0e-12;
        const double pan   = (st.panSumR2 - st.panSumL2) / denom;

        st.panHold   = juce::jlimit (-1.0f, 1.0f, (float) pan);
        st.velLocked = true;
    };

    auto velGainFromRms = [&] (float rms, float gateOnLin)
    {
        // RMS -> approx peak (für sinus-ähnliche Signale)
        const float peakEst = rms * 1.41421356f;

        float ratio = peakEst / (gateOnLin + 1.0e-6f);    // wie viel über Gate-Schwelle
        ratio = juce::jlimit (0.0f, 8.0f, ratio);

        // leichte Kompression, damit es musikalisch bleibt
        float g = std::sqrt (ratio);
        return juce::jlimit (0.0f, 3.0f, g);
    };

    // Per-band peak trackers for mixer meters
    float peakL[BandpassBank::kMaxBands] = {};
    float peakR[BandpassBank::kMaxBands] = {};

    for (int n = 0; n < numSamples; ++n)
    {
        const float inL = buffer.getSample (0, n);
        const float inR = (numInCh > 1) ? buffer.getSample (1, n) : inL;
        const float inMono = 0.5f * (inL + inR);

        spectrum.pushSample (inMono);

        // Decay noteGlow per sample (~80ms decay time)
        {
            const float glowDecay = std::exp (-1.0f / (0.08f * (float) sr));
            for (int pc = 0; pc < 12; ++pc)
            {
                float g = noteGlow[(size_t) pc].load (std::memory_order_relaxed);
                g *= glowDecay;
                noteGlow[(size_t) pc].store (g, std::memory_order_relaxed);
            }
        }

        float synthL = 0.0f;
        float synthR = 0.0f;

        for (int b = 0; b < BandpassBank::kMaxBands; ++b)
        {
            const bool enabled = isBandEnabled (b);

            if (! enabled)
            {
                if (impulse[b].wasEnabled)
                {
                    impulse[b].wasEnabled = false;

                    impulse[b].env  = 0.0f;
                    impulse[b].envL = 0.0f;
                    impulse[b].envR = 0.0f;

                    impulse[b].gateOpen = false;

                    impulse[b].velSumSq  = 0.0;
                    impulse[b].panSumL2  = 0.0;
                    impulse[b].panSumR2  = 0.0;
                    impulse[b].velCount  = 0;
                    impulse[b].velLocked = false;
                    impulse[b].velHoldRms = 0.0f;
                    impulse[b].panHold    = 0.0f;

                    impulse[b].synth.reset();
                    impulse[b].pitch.reset();
                }
                continue;
            }

            impulse[b].wasEnabled = true;

            bank.updateBandIfNeeded (b, getBandFreqHz (b), getBandRes01 (b), getBandPoles (b));

            // band signals (stereo) + store to bandBuffers
            const float x0 = inL;
            const float y0 = bank.processSample (b, 0, x0);
            bandBuffers[b].setSample (0, n, y0);

            float y1 = y0;
            if (numInCh > 1)
            {
                const float x1 = inR;
                y1 = bank.processSample (b, 1, x1);
                bandBuffers[b].setSample (1, n, y1);
            }

            const float bandMono = (numInCh > 1) ? (0.5f * (y0 + y1)) : y0;

            // (1) Gate (Envelope follower) - jetzt stereo envs für pan + mono env für gate
            const float gateOnLin  = juce::Decibels::decibelsToGain (pGateDb[b]->load());
            const float gateOffLin = 0.80f * gateOnLin;

            auto follow = [&] (float env, float mag)
            {
                return (mag > env) ? (att * env + (1.0f - att) * mag)
                                   : (rel * env + (1.0f - rel) * mag);
            };

            auto& st = impulse[b];

            st.envL = follow (st.envL, std::abs (y0));
            st.envR = follow (st.envR, std::abs (y1));
            st.env  = 0.5f * (st.envL + st.envR);

            bool& gateOpen = st.gateOpen;

            if (! gateOpen && st.env > gateOnLin)
            {
                gateOpen = true;

                st.pitch.reset();

                // Generative spray: jitter pitch/velocity and (sparsely) skip notes
                if (randomAmt > 0.0001f)
                {
                    const float cents  = (rng.nextFloat() - 0.5f) * randomAmt * 50.0f;
                    st.randDetune      = std::pow (2.0f, cents / 1200.0f);
                    st.randVel         = 1.0f + (rng.nextFloat() - 0.5f) * randomAmt * 0.9f;
                    st.noteActive      = (rng.nextFloat() > randomAmt * 0.55f); // higher random = sparser
                }
                else { st.randDetune = 1.0f; st.randVel = 1.0f; st.noteActive = true; }

                // Immediately quantize the initial band frequency so noteGlow fires right away
                const float initHz = juce::jlimit (20.0f, 20000.0f, getBandFreqHz (b));
                const int noteMask0 = (int) std::round (pNoteMask[b]->load());
                const float qHz0 = ScaleQuantizer::quantizeHzWithMask (initHz, noteMask0);
                st.oscHz = (qHz0 > 0.0f) ? qHz0 : initHz;
                st.hasQuant = true;
                st.lastQuantHz = st.oscHz;
                st.lastQuantMidi = 69.0f + 12.0f * std::log2 (st.oscHz / 440.0f);

                const int oct = (int) std::round (pOct[b]->load());
                const float octShift = std::pow (2.0f, (float) oct);
                if (st.noteActive)
                    st.synth.noteOn (juce::jlimit (20.0f, 20000.0f, st.oscHz * octShift * st.randDetune));

                // onset measurement reset
                st.velSumSq  = 0.0;
                st.panSumL2  = 0.0;
                st.panSumR2  = 0.0;
                st.velCount  = 0;
                st.velLocked = false;

                st.velHoldRms = 0.0f;

                // initial pan guess from envelopes (wird später per RMS-window genauer)
                const float denom = st.envL + st.envR + 1.0e-6f;
                st.panHold = juce::jlimit (-1.0f, 1.0f, (st.envR - st.envL) / denom);
            }
            else if (gateOpen && st.env < gateOffLin)
            {
                // falls sehr kurze Note -> Velocity/Pan finalisieren, bevor Release läuft
                if (! st.velLocked)
                    lockVelPan (b);

                gateOpen = false;
                st.synth.noteOff();
            }

            // while gate open: collect onset RMS (velocity) + energy L/R (pan)
            if (gateOpen && ! st.velLocked)
            {
                st.velSumSq += (double) (bandMono * bandMono);
                st.panSumL2 += (double) (y0 * y0);
                st.panSumR2 += (double) (y1 * y1);
                ++st.velCount;

                if (st.velCount >= velWindow)
                    lockVelPan (b);
            }

            // (2) Pitch detect (wie zuvor)
            const float sens  = juce::jlimit (0.0f, 1.0f, pSens01[b]->load());
            const float tol   = juce::jlimit (0.0f, 1.0f, pTol01 [b]->load());
            const float confT = juce::jlimit (0.0f, 1.0f, pConf01[b]->load());

            float detectedHz = 0.0f, detectedConf = 0.0f;

            const float expectedHz = (st.oscHz > 0.0f) ? st.oscHz
                                                       : juce::jlimit (20.0f, 20000.0f, getBandFreqHz (b));

            const bool updated = st.pitch.process (bandMono, gateOpen, expectedHz,
                                                  tol, sens, confT,
                                                  detectedHz, detectedConf);

            if (updated && detectedConf > 0.0f)
                st.lastConf = detectedConf;

            if (updated && detectedHz > 0.0f)
            {
                const float snap = juce::jlimit (-1.0f, 1.0f, pSnap[b]->load());
                const int noteMask = (int) std::round (pNoteMask[b]->load());

                const float qHz = ScaleQuantizer::quantizeHzWithMask (detectedHz, noteMask);
                if (qHz > 0.0f) // (safety) niemals return; nur bandweise skip
                {
                    const float detMidi = 69.0f + 12.0f * std::log2 (detectedHz / 440.0f);
                    const float qMidi   = 69.0f + 12.0f * std::log2 (qHz / 440.0f);

                    if (!st.hasQuant)
                    {
                        st.hasQuant = true;
                        st.lastQuantHz = qHz;
                        st.lastQuantMidi = qMidi;
                    }

                    float outHz = qHz;

                    if (snap < 0.0f)
                    {
                        const float mix = 1.0f + snap; // 0..1
                        outHz = detectedHz * (1.0f - mix) + qHz * mix;

                        st.lastQuantHz = qHz;
                        st.lastQuantMidi = qMidi;
                    }
                    else if (snap > 0.0f)
                    {
                        const float thresholdSemis = 0.5f + snap * 1.0f; // 0.5 .. 1.5
                        const float currentMidi = st.lastQuantMidi;

                        if (std::abs (qMidi - currentMidi) > 0.001f)
                        {
                            if (std::abs (detMidi - currentMidi) < thresholdSemis)
                            {
                                outHz = st.lastQuantHz; // stay
                            }
                            else
                            {
                                st.lastQuantHz = qHz;
                                st.lastQuantMidi = qMidi;
                                outHz = qHz;
                            }
                        }
                        else
                        {
                            st.lastQuantHz = qHz;
                            st.lastQuantMidi = qMidi;
                            outHz = qHz;
                        }
                    }
                    else
                    {
                        st.lastQuantHz = qHz;
                        st.lastQuantMidi = qMidi;
                        outHz = qHz;
                    }

                    st.oscHz = outHz;
                    if (gateOpen)
                    {
                        const int oct2 = (int) std::round (pOct[b]->load());
                        const float octShift2 = std::pow (2.0f, (float) oct2);
                        st.synth.setFrequency (juce::jlimit (20.0f, 20000.0f, outHz * octShift2 * st.randDetune));
                    }
                }
            }
            // --- MIDI OUT: generate note events at gate transitions ---
            if (midiOutMode)
            {
                const bool wasOpen = midiGateWasOpen[b];
                if (gateOpen && !wasOpen)
                {
                    // Note-on
                    const int oct = (int) std::round (pOct[b]->load());
                    float midiNote = st.lastQuantMidi + 12.0f * (float) oct;
                    midiNote = juce::jlimit (0.0f, 127.0f, midiNote);
                    int noteNum = (int) std::round (midiNote);

                    float rmsUse = st.velHoldRms;
                    if (! st.velLocked && st.velCount > 0)
                        rmsUse = (float) std::sqrt (st.velSumSq / (double) st.velCount);
                    float vel = velGainFromRms (rmsUse, gateOnLin);
                    int midiVel = juce::jlimit (1, 127, (int) (vel * 127.0f));

                    midiMessages.addEvent (juce::MidiMessage::noteOn (b + 1, noteNum, (juce::uint8) midiVel), n);
                    midiLastNote[b] = noteNum;
                    midiLastVel[b] = vel;
                }
                else if (!gateOpen && wasOpen)
                {
                    // Note-off
                    midiMessages.addEvent (juce::MidiMessage::noteOff (b + 1, midiLastNote[b]), n);
                }
                // Update pitch while gate open (pitch bend or note change)
                else if (gateOpen && wasOpen && st.hasQuant)
                {
                    const int oct = (int) std::round (pOct[b]->load());
                    float midiNote = st.lastQuantMidi + 12.0f * (float) oct;
                    midiNote = juce::jlimit (0.0f, 127.0f, midiNote);
                    int noteNum = (int) std::round (midiNote);
                    if (noteNum != midiLastNote[b])
                    {
                        midiMessages.addEvent (juce::MidiMessage::noteOff (b + 1, midiLastNote[b]), n);
                        float rmsUse = st.velHoldRms;
                        int midiVel = juce::jlimit (1, 127, (int) (velGainFromRms (rmsUse, gateOnLin) * 127.0f));
                        midiMessages.addEvent (juce::MidiMessage::noteOn (b + 1, noteNum, (juce::uint8) midiVel), n);
                        midiLastNote[b] = noteNum;
                    }
                }
                midiGateWasOpen[b] = gateOpen;

                // Also add pan as MIDI CC10 (0-127)
                if (gateOpen && wasOpen == false)
                {
                    float panMidi = (st.panHold + 1.0f) * 63.5f;
                    midiMessages.addEvent (juce::MidiMessage::controllerEvent (b + 1, 10, juce::jlimit (0, 127, (int) panMidi)), n);
                }
            }

            // Synth: WaveBlend + Noise + Tone + HP + LP + ADSR + Velocity + Pan(width)
            if (!midiOutMode)
            {
                // Set shaping params
                st.synth.setSynthShaping (
                    pNoise[b]->load(),
                    pTone[b]->load(),
                    pHP[b]->load(),
                    pLP[b]->load()
                );

                // velocity: use locked onset RMS if available, else current partial RMS
                float rmsUse = st.velHoldRms;
                if (! st.velLocked && st.velCount > 0)
                    rmsUse = (float) std::sqrt (st.velSumSq / (double) st.velCount);

                const float velGain = velGainFromRms (rmsUse, gateOnLin) * st.randVel;
                auto lr = st.synth.renderSample (velGain, st.panHold, width);

                // Apply per-band volume
                const float vol = juce::jlimit (0.0f, 2.0f, pVol[b]->load());
                const float sL = lr.first  * vol;
                const float sR = lr.second * vol;
                synthL += sL;
                synthR += sR;

                // Track peaks for mixer meters
                peakL[b] = juce::jmax (peakL[b], std::abs (sL));
                peakR[b] = juce::jmax (peakR[b], std::abs (sR));

                // Pulse noteGlow for the active quantized pitch class
                if (gateOpen && st.hasQuant && st.lastQuantHz > 0.0f)
                {
                    const float midi = st.lastQuantMidi;
                    const int pc = ((int) std::round (midi)) % 12;
                    const int pcSafe = ((pc % 12) + 12) % 12;
                    noteGlow[(size_t) pcSafe].store (1.0f, std::memory_order_relaxed);
                }
            }
        }

        // Collect dry synth voices into the wet bus (reverb + mix happen post-loop)
        synthBus.setSample (0, n, synthL * synthGain);
        synthBus.setSample (1, n, synthR * synthGain);
    }

    // ── Post-loop: master FX on the synth bus, then dry/wet against the input ──
    if (! midiOutMode)
    {
        masterFx.setParams (pDrive     ? pDrive->load()     : 0.0f,
                            pDelayMix  ? pDelayMix->load()  : 0.0f,
                            pDelayTime ? pDelayTime->load() : 320.0f,
                            pDelayFb   ? pDelayFb->load()   : 0.0f,
                            pReverb    ? pReverb->load()    : 0.0f,
                            pComp      ? pComp->load()      : 0.0f);
        masterFx.process (synthBus, numSamples);
    }

    for (int n = 0; n < numSamples; ++n)
    {
        const float inL = buffer.getSample (0, n);
        const float inR = (numInCh > 1) ? buffer.getSample (1, n) : inL;
        const float inMono = 0.5f * (inL + inR);

        if (midiOutMode)
        {
            // pass through dry audio only (no synth)
            if (numOutCh == 1)
                buffer.setSample (0, n, inMono);
            else
            {
                buffer.setSample (0, n, inL);
                buffer.setSample (1, n, inR);
                for (int ch = 2; ch < numOutCh; ++ch)
                    buffer.setSample (ch, n, 0.5f * (inL + inR));
            }
            continue;
        }

        const float sL = synthBus.getSample (0, n);
        const float sR = synthBus.getSample (1, n);

        // Dry/Wet: 0% -> input only, 100% -> synth only
        if (numOutCh == 1)
        {
            buffer.setSample (0, n, inMono * dry + (0.5f * (sL + sR)) * wet);
        }
        else
        {
            const float outL = inL * dry + sL * wet;
            const float outR = inR * dry + sR * wet;
            buffer.setSample (0, n, outL);
            buffer.setSample (1, n, outR);
            for (int ch = 2; ch < numOutCh; ++ch)
                buffer.setSample (ch, n, 0.5f * (outL + outR));
        }
    }

    // Update per-band meter atomics (peak-hold with exponential fall, Ableton-style)
    // Time constant ~300ms gives ~26dB/s fall rate
    const float meterDecay = std::exp (-(float) numSamples / (0.3f * (float) sr));
    for (int b = 0; b < BandpassBank::kMaxBands; ++b)
    {
        float curL = meterL[(size_t) b].load (std::memory_order_relaxed);
        float curR = meterR[(size_t) b].load (std::memory_order_relaxed);
        curL = juce::jmax (peakL[b], curL * meterDecay);
        curR = juce::jmax (peakR[b], curR * meterDecay);
        meterL[(size_t) b].store (curL, std::memory_order_relaxed);
        meterR[(size_t) b].store (curR, std::memory_order_relaxed);

        // Visualization data for UI
        vizOscHz[(size_t)b].store (impulse[b].oscHz, std::memory_order_relaxed);
        vizConf[(size_t)b].store (impulse[b].lastConf, std::memory_order_relaxed);
        vizEnvLevel[(size_t)b].store (impulse[b].env, std::memory_order_relaxed);
        vizGateOpen[(size_t)b].store (impulse[b].gateOpen ? 1 : 0, std::memory_order_relaxed);
    }
}

//==============================================================================
bool NewProjectAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* NewProjectAudioProcessor::createEditor()
{
    return new NewProjectAudioProcessorEditor (*this);
}

//==============================================================================
void NewProjectAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void NewProjectAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout NewProjectAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    const double minF = 20.0, maxF = 20000.0;
    const double ratio = maxF / minF;

    for (int b = 0; b < BandpassBank::kMaxBands; ++b)
    {
        const bool defaultOn = (b < 6);
        layout.add (std::make_unique<juce::AudioParameterBool> (bandOnId (b), "Band On", defaultOn));

        const double t = (double) b / (double) (BandpassBank::kMaxBands - 1);
        const float defaultFreq = (float) (minF * std::pow (ratio, t));

        juce::NormalisableRange<float> fRange (20.0f, 20000.0f);
        fRange.setSkewForCentre (1000.0f);

        layout.add (std::make_unique<juce::AudioParameterFloat> (bandFreqId (b), "Freq",  fRange, defaultFreq));
        layout.add (std::make_unique<juce::AudioParameterFloat> (bandResId  (b), "Res",   juce::NormalisableRange<float> (0.0f, 1.0f), 0.25f));
        layout.add (std::make_unique<juce::AudioParameterInt>   (bandPoleId (b), "Poles", 1, 12, 4));

        layout.add (std::make_unique<juce::AudioParameterFloat> (bandGateId (b), "Gate",
                                                                 juce::NormalisableRange<float> (-42.0f, 0.0f),
                                                                 -24.0f));
        layout.add (std::make_unique<juce::AudioParameterFloat> (bandSensId (b), "Sensitivity",
                                                                 juce::NormalisableRange<float> (0.0f, 1.0f),
                                                                 0.60f));
        layout.add (std::make_unique<juce::AudioParameterFloat> (bandTolId (b), "Tolerance",
                                                                 juce::NormalisableRange<float> (0.0f, 1.0f),
                                                                 0.50f));
        layout.add (std::make_unique<juce::AudioParameterFloat> (bandConfId (b), "Confidence",
                                                                 juce::NormalisableRange<float> (0.0f, 1.0f),
                                                                 0.35f));

        // ---------------------------------------------------------------------
        // Synth params (per band)
        layout.add (std::make_unique<juce::AudioParameterChoice> (
            bandWaveLId (b), "Wave L",
            juce::StringArray { "Sine", "Triangle", "Saw", "Square" }, 0));

        layout.add (std::make_unique<juce::AudioParameterChoice> (
            bandWaveRId (b), "Wave R",
            juce::StringArray { "Sine", "Triangle", "Saw", "Square" }, 3));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            bandWaveMixId (b), "Wave Mix",
            juce::NormalisableRange<float> (-1.0f, 1.0f, 0.001f),
            -1.0f));

        // ADSR time range: piecewise mapping
        // 0-25% knob = 0s-100ms, 25-50% = 100ms-1s, 50-100% = 1s-8s
        juce::NormalisableRange<float> tRange (
            0.0f, 8.0f,
            // convertFrom0to1
            [] (float, float, float n) -> float
            {
                if (n <= 0.25f)
                    return (n / 0.25f) * 0.1f;
                if (n <= 0.5f)
                    return 0.1f + ((n - 0.25f) / 0.25f) * 0.9f;
                return 1.0f + ((n - 0.5f) / 0.5f) * 7.0f;
            },
            // convertTo0to1
            [] (float, float, float v) -> float
            {
                if (v <= 0.1f)
                    return (v / 0.1f) * 0.25f;
                if (v <= 1.0f)
                    return 0.25f + ((v - 0.1f) / 0.9f) * 0.25f;
                return 0.5f + ((v - 1.0f) / 7.0f) * 0.5f;
            },
            // snapToLegalValue
            [] (float, float, float v) -> float
            {
                return juce::jlimit (0.0f, 8.0f, v);
            }
        );
        layout.add (std::make_unique<juce::AudioParameterFloat> (bandAtkId (b), "Attack",  tRange, 0.05f));
        layout.add (std::make_unique<juce::AudioParameterFloat> (bandDecId (b), "Decay",   tRange, 0.20f));
        layout.add (std::make_unique<juce::AudioParameterFloat> (bandSusId (b), "Sustain",
                                                                 juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f),
                                                                 0.50f));
        layout.add (std::make_unique<juce::AudioParameterFloat> (bandRelId (b), "Release", tRange, 0.50f));

        // Synth shaping per band
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            bandNoiseId (b), "Noise",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            bandToneId (b), "Tone",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            bandHPId (b), "HP",
            juce::NormalisableRange<float> (20.0f, 20000.0f, 0.0f, 0.3f), 20.0f));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            bandLPId (b), "LP",
            juce::NormalisableRange<float> (20.0f, 20000.0f, 0.0f, 0.3f), 20000.0f));

        // Voice engine selector + secondary macro
        layout.add (std::make_unique<juce::AudioParameterChoice> (
            bandEngineId (b), "Engine",
            juce::StringArray { "Blend", "FM", "Wavetable", "Phase-FB",
                                "Harmonic", "Modal", "Swarm", "Airwave" }, 0));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            bandTimbreId (b), "Timbre",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));

        // Quantizer params (per band)
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            bandSnapId (b), "Snap",
            juce::NormalisableRange<float> (-1.0f, 1.0f, 0.001f), 0.0f));

        // Note mask: 12-bit integer, each bit = one pitch class (bit0=C .. bit11=B)
        // Default: C major = C,D,E,F,G,A,B = 2741
        layout.add (std::make_unique<juce::AudioParameterInt> (
            bandNoteMaskId (b), "Notes", 0, 4095, 2741));

        // Octave shift per band
        layout.add (std::make_unique<juce::AudioParameterInt> (
            bandOctId (b), "Octave", -4, 4, 0));

        // Mixer volume per band (0 = mute, 1 = unity, 2 = +6dB)
        // Skew 4.8 puts -6dB (gain 0.5) at 75% of fader travel
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            bandVolId (b), "Volume",
            juce::NormalisableRange<float> (0.0f, 2.0f,
                // convertFrom0to1: fader position → gain
                [] (float, float, float n) -> float {
                    if (n < 0.01f) return 0.0f; // dead zone → silence
                    float dB = -48.0f + (n / 1.0f) * 54.0f; // 0→-48dB, 1→+6dB
                    return std::pow (10.0f, dB / 20.0f);
                },
                // convertTo0to1: gain → fader position
                [] (float, float, float g) -> float {
                    if (g < 1.0e-6f) return 0.0f;
                    float dB = 20.0f * std::log10 (g);
                    return juce::jlimit (0.0f, 1.0f, (dB + 48.0f) / 54.0f);
                },
                // snapToLegalValue
                [] (float, float, float v) -> float {
                    return juce::jlimit (0.0f, 2.0f, v);
                }
            ), 1.0f));
    }

    layout.add (std::make_unique<juce::AudioParameterInt>  (kScaleRootId,  "Scale Root",  0, 11, 0));
    layout.add (std::make_unique<juce::AudioParameterBool> (kScaleMinorId, "Scale Minor", false));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        kDryWetId, "DryWet",
        juce::NormalisableRange<float> (0.0f, 1.0f),
        0.5f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        kWidthId, "Width",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f),
        1.0f));

    layout.add (std::make_unique<juce::AudioParameterBool> (kMidiOutId, "MIDI Out", false));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        kReverbId, "Reverb", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.18f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        kRandomId, "Random", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

    // Master FX
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        kDriveId, "Drive", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        kDelayMixId, "Delay Mix", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));
    {
        juce::NormalisableRange<float> dtRange (40.0f, 1200.0f, 1.0f);
        dtRange.setSkewForCentre (300.0f);
        layout.add (std::make_unique<juce::AudioParameterFloat> (kDelayTimeId, "Delay Time", dtRange, 320.0f));
    }
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        kDelayFbId, "Delay Fb", juce::NormalisableRange<float> (0.0f, 0.95f, 0.001f), 0.35f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        kCompId, "Glue", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

    return layout;
}

//==============================================================================
bool NewProjectAudioProcessor::isBandEnabled (int band) const
{
    band = juce::jlimit (0, 7, band);
    return (pBandOn[band]->load() >= 0.5f);
}

float NewProjectAudioProcessor::getBandFreqHz (int band) const
{
    band = juce::jlimit (0, 7, band);
    return pBandFreq[band]->load();
}

float NewProjectAudioProcessor::getBandRes01 (int band) const
{
    band = juce::jlimit (0, 7, band);
    return pBandRes[band]->load();
}

int NewProjectAudioProcessor::getBandPoles (int band) const
{
    band = juce::jlimit (0, 7, band);
    return (int) std::round (pBandPole[band]->load());
}

//==============================================================================
//  Presets — four field-recording soundscapes
//==============================================================================
const char* NewProjectAudioProcessor::presetName (int index)
{
    static const char* names[kNumPresets] = { "RAINFOREST", "WAVES", "DESERT", "NOISE" };
    return names[juce::jlimit (0, kNumPresets - 1, index)];
}

void NewProjectAudioProcessor::applyPreset (int index)
{
    index = juce::jlimit (0, kNumPresets - 1, index);

    auto set = [this] (const juce::String& id, float v)
    {
        if (auto* p = dynamic_cast<juce::RangedAudioParameter*> (apvts.getParameter (id)))
            p->setValueNotifyingHost (p->convertTo0to1 (v));
    };

    // Per-band engine recipe: { engine, morph(-1..1), timbre(0..1) }
    struct Voice { int eng; float morph; float timbre; };
    // Per-band envelope/shape: { atk, dec, sus, rel, noise, tone }
    struct Shape { float a, d, s, r, noise, tone; };

    // log-spaced band centres 70 Hz .. 7 kHz
    auto bandFreq = [] (int b) { return 70.0f * std::pow (7000.0f / 70.0f, (float) b / 7.0f); };

    // ----- global defaults (overridden per preset below) -----
    int   numOn      = 7;
    int   scaleRoot  = 0;
    bool  scaleMinor = false;
    int   noteMask   = 2741;   // C major
    float snap       = 0.30f;
    float dryWet     = 0.60f;
    float width      = 1.0f;
    float reverbAmt  = 0.30f;
    float randomAmt  = 0.12f;
    float driveAmt   = 0.15f;
    float delayMix   = 0.20f;
    float delayTime  = 360.0f;
    float delayFb    = 0.35f;
    float compAmt    = 0.20f;
    float resv       = 0.30f;
    int   poles      = 4;
    float gateDb     = -26.0f;
    float sens       = 0.6f, tol = 0.5f, conf = 0.35f;

    Voice voices[8];
    Shape shapes[8];
    int   octave[8] = { 0,0,0,0,0,0,0,0 };

    switch (index)
    {
        case 0: // RAINFOREST — recreation of the Cardinal patch (curated voices)
        {
            numOn = 7; reverbAmt = 0.34f; randomAmt = 0.14f; dryWet = 0.60f; snap = 0.30f;
            driveAmt = 0.20f; delayMix = 0.22f; delayTime = 380.0f; delayFb = 0.36f; compAmt = 0.22f;
            scaleRoot = 0; scaleMinor = false; noteMask = 2741;
            const Voice v[8] = {
                {6, -0.2f, 0.55f}, // Swarm   (Plaits VA, lush bass)
                {1,  0.0f, 0.45f}, // FM      (FMOp)
                {2,  0.1f, 0.55f}, // Wavetable (SurgeXT)
                {5, -0.1f, 0.40f}, // Modal   (Braids/Rings)
                {5,  0.3f, 0.30f}, // Modal   (PalmLoop pluck, brighter+longer)
                {4, -0.2f, 0.55f}, // Harmonic (Osculum organ)
                {3,  0.2f, 0.60f}, // Phase-FB (Ouros)
                {7,  0.0f, 0.50f}, // Airwave  (canopy texture)
            };
            const Shape s[8] = {
                {0.02f,0.6f,0.45f,1.4f, 0.05f,0.20f},
                {0.01f,0.5f,0.30f,1.0f, 0.00f,0.25f},
                {0.05f,0.8f,0.40f,1.6f, 0.08f,0.15f},
                {0.002f,0.9f,0.10f,1.2f,0.00f,0.10f},
                {0.002f,1.2f,0.05f,1.8f,0.00f,0.05f},
                {0.30f,0.7f,0.70f,2.0f, 0.10f,0.10f},
                {0.04f,0.6f,0.45f,1.5f, 0.05f,0.30f},
                {0.50f,1.0f,0.65f,2.5f, 0.35f,0.05f},
            };
            for (int b=0;b<8;++b){voices[b]=v[b];shapes[b]=s[b];}
            break;
        }
        case 1: // WAVES — steady, noisy, slowly breathing wash
        {
            numOn = 8; reverbAmt = 0.62f; randomAmt = 0.10f; dryWet = 0.55f; snap = 0.20f;
            driveAmt = 0.15f; delayMix = 0.18f; delayTime = 620.0f; delayFb = 0.46f; compAmt = 0.26f;
            scaleRoot = 9; scaleMinor = true; noteMask = ScaleQuantizer::scaleToBitmask (9, true); // A minor
            gateDb = -34.0f; conf = 0.20f; resv = 0.22f; poles = 3;
            const Voice base[8] = {
                {7,-0.3f,0.4f},{4,-0.2f,0.6f},{7,0.0f,0.5f},{4,0.1f,0.5f},
                {7,0.2f,0.6f},{3,0.0f,0.5f},{7,0.4f,0.7f},{7,0.6f,0.8f},
            };
            for (int b=0;b<8;++b){
                voices[b]=base[b];
                shapes[b]={1.2f,1.5f,0.85f,2.8f, 0.30f+0.04f*b, 0.08f};
            }
            break;
        }
        case 2: // DESERT — sparse, plucked events in a vast space
        {
            numOn = 6; reverbAmt = 0.72f; randomAmt = 0.48f; dryWet = 0.52f; snap = 0.55f;
            driveAmt = 0.10f; delayMix = 0.34f; delayTime = 520.0f; delayFb = 0.52f; compAmt = 0.15f;
            scaleRoot = 2; scaleMinor = false;
            noteMask = (1<<2)|(1<<4)|(1<<6)|(1<<9)|(1<<11); // D major pentatonic-ish
            gateDb = -16.0f; conf = 0.50f; tol = 0.4f; sens = 0.5f;
            const Voice base[8] = {
                {5,-0.4f,0.3f},{5,-0.2f,0.35f},{1,0.0f,0.3f},{5,0.2f,0.25f},
                {3,0.1f,0.4f},{1,0.3f,0.35f},{5,0.4f,0.2f},{5,0.5f,0.2f},
            };
            const int oct[8]={-1,0,0,1,0,1,2,2};
            for (int b=0;b<8;++b){
                voices[b]=base[b];
                shapes[b]={0.003f,0.35f,0.0f,0.9f, 0.0f, 0.12f};
                octave[b]=oct[b];
            }
            break;
        }
        case 3: // NOISE — city / industrial, dense and gritty, chromatic
        default:
        {
            numOn = 8; reverbAmt = 0.20f; randomAmt = 0.26f; dryWet = 0.55f; snap = 0.05f;
            driveAmt = 0.52f; delayMix = 0.26f; delayTime = 180.0f; delayFb = 0.42f; compAmt = 0.42f;
            scaleRoot = 0; scaleMinor = false; noteMask = 4095; // chromatic
            gateDb = -30.0f; conf = 0.28f; tol = 0.7f; resv = 0.35f; poles = 5;
            const Voice base[8] = {
                {2,-0.2f,0.7f},{1,0.4f,0.7f},{2,0.2f,0.8f},{7,0.0f,0.6f},
                {1,0.6f,0.8f},{2,0.5f,0.9f},{3,0.3f,0.7f},{7,0.4f,0.7f},
            };
            for (int b=0;b<8;++b){
                voices[b]=base[b];
                shapes[b]={0.005f,0.25f,0.25f,0.5f, 0.20f+0.03f*b, 0.55f};
            }
            break;
        }
    }

    // ----- write globals -----
    set (kScaleRootId,  (float) scaleRoot);
    set (kScaleMinorId, scaleMinor ? 1.0f : 0.0f);
    set (kDryWetId,     dryWet);
    set (kWidthId,      width);
    set (kReverbId,     reverbAmt);
    set (kRandomId,     randomAmt);
    set (kDriveId,      driveAmt);
    set (kDelayMixId,   delayMix);
    set (kDelayTimeId,  delayTime);
    set (kDelayFbId,    delayFb);
    set (kCompId,       compAmt);
    set (kMidiOutId,    0.0f);

    // ----- write per band -----
    for (int b = 0; b < BandpassBank::kMaxBands; ++b)
    {
        set (bandOnId (b),   (b < numOn) ? 1.0f : 0.0f);
        set (bandFreqId (b), bandFreq (b));
        set (bandResId (b),  resv);
        set (bandPoleId (b), (float) poles);

        set (bandGateId (b), gateDb);
        set (bandSensId (b), sens);
        set (bandTolId (b),  tol);
        set (bandConfId (b), conf);

        set (bandEngineId (b),  (float) voices[b].eng);
        set (bandWaveMixId (b), voices[b].morph);
        set (bandTimbreId (b),  voices[b].timbre);

        set (bandAtkId (b), shapes[b].a);
        set (bandDecId (b), shapes[b].d);
        set (bandSusId (b), shapes[b].s);
        set (bandRelId (b), shapes[b].r);
        set (bandNoiseId (b), shapes[b].noise);
        set (bandToneId (b),  shapes[b].tone);
        set (bandHPId (b), 20.0f);
        set (bandLPId (b), 20000.0f);

        set (bandSnapId (b), snap);
        set (bandNoteMaskId (b), (float) noteMask);
        set (bandOctId (b), (float) octave[b]);
        set (bandVolId (b), 1.0f);
    }
}

//==============================================================================
//  User presets (.gemspreset XML — full APVTS state)
//==============================================================================
juce::File NewProjectAudioProcessor::getUserPresetDir()
{
    auto dir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                   .getChildFile ("GEMS").getChildFile ("Presets");
    if (! dir.exists()) dir.createDirectory();
    return dir;
}

juce::StringArray NewProjectAudioProcessor::getUserPresetNames() const
{
    juce::StringArray names;
    auto files = getUserPresetDir().findChildFiles (juce::File::findFiles, false, "*.gemspreset");
    for (auto& f : files) names.add (f.getFileNameWithoutExtension());
    names.sortNatural();
    return names;
}

bool NewProjectAudioProcessor::saveUserPreset (const juce::String& name)
{
    auto safe = juce::File::createLegalFileName (name).trim();
    if (safe.isEmpty()) return false;

    auto file = getUserPresetDir().getChildFile (safe + ".gemspreset");
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        return xml->writeTo (file);
    return false;
}

bool NewProjectAudioProcessor::loadUserPreset (const juce::String& name)
{
    auto file = getUserPresetDir().getChildFile (name + ".gemspreset");
    if (! file.existsAsFile()) return false;

    if (auto xml = juce::XmlDocument::parse (file))
        if (xml->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
            return true;
        }
    return false;
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NewProjectAudioProcessor();
}

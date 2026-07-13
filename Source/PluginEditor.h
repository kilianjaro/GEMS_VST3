#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "HemingLookAndFeel.h"

using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

//==============================================================================
// The eight voice colours — the only colour in the instrument.
static const juce::Colour kBandColours[8] =
{
    juce::Colour (0xffFF3366), juce::Colour (0xffFF6633),
    juce::Colour (0xffFFD500), juce::Colour (0xff33FF77),
    juce::Colour (0xff00FFCC), juce::Colour (0xff3399FF),
    juce::Colour (0xffAA44FF), juce::Colour (0xffFF44CC),
};

//==============================================================================
class BandSelectButton : public juce::Button
{
public:
    int bandIndex = 0;
    bool multiSelected = false;   // true when part of multi-selection (not primary)
    const HemingLookAndFeel* lnf = nullptr;
    BandSelectButton() : juce::Button ("") {}
    void paintButton (juce::Graphics&, bool, bool) override;
};

class BandLampButton : public juce::ToggleButton
{ public: int bandIndex = 0; void paintButton (juce::Graphics&, bool, bool) override; };

class ScaleModeButton : public juce::ToggleButton
{ public: const HemingLookAndFeel* lnf = nullptr; void paintButton (juce::Graphics&, bool, bool) override; };

class QuantizerNoteButton : public juce::Button
{ public: int pitchClass = 0; float glow = 0.0f; const HemingLookAndFeel* lnf = nullptr;
  QuantizerNoteButton() : juce::Button ("") {} void paintButton (juce::Graphics&, bool, bool) override; };

class WaveformButton : public juce::Button
{ public: int currentWave = 0; WaveformButton() : juce::Button ("") {} void paintButton (juce::Graphics&, bool, bool) override;
private: static void drawWaveIcon (juce::Graphics&, juce::Rectangle<float>, int); };

//==============================================================================
// Engine selector — stepper cell: [<] ENGINE NAME [>]
// Click name = popup menu, click arrows / wheel = step, for the selected band(s).
class EngineSelector : public juce::Component
{
public:
    EngineSelector() { setWantsKeyboardFocus (false); }
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

    int           current = 0;                 // engine index 0..7
    juce::Colour  accent { 0xffe8e6de };
    std::function<void (int)> onPick;          // called with new engine index
    HemingLookAndFeel* lnfPtr = nullptr;       // fonts + popup styling
private:
    static constexpr int kArrowW = 17;
    float wheelAccum = 0.0f;
};

//==============================================================================
class OctaveControl : public juce::Component
{
public:
    OctaveControl();
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void attachToParam (juce::AudioProcessorValueTreeState&, const juce::String&);
    void detachParam();
    int getValue() const;

    juce::Colour accent { 0xffe8e6de };
    const HemingLookAndFeel* lnf = nullptr;
private:
    juce::Slider internalSlider;          // must outlive att
    std::unique_ptr<SliderAttachment> att;
    float dragStartY = 0, dragStartVal = 0, wheelAccum = 0.0f;
};

//==============================================================================
class MixerComponent : public juce::Component
{
public:
    MixerComponent();
    void paint (juce::Graphics&) override;
    void resized() override;
    void setLevels (int band, float L, float R);
    void setBandOn (int band, bool on);
    juce::Slider volSliders[8];
    std::unique_ptr<SliderAttachment> volAtts[8];
    void attachVolumes (juce::AudioProcessorValueTreeState&);
    static float gainToPosition (float gain);

    const HemingLookAndFeel* lnf = nullptr;
    static constexpr int kRulerW  = 16;   // dB ruler on the left
    static constexpr int kHeadH   = 15;   // colour chip + band number
    static constexpr int kFootH   = 11;   // dB readout
private:
    float meterL[8]={}, meterR[8]={};
    bool bandOn[8]={};
};

//==============================================================================
//  (1) Analysis field — input spectrum (with phosphor history), band filter
//      curves, detection thresholds, and live capture markers
//==============================================================================
class SpectrumAnalyzerComponent : public juce::Component
{
public:
    void paint (juce::Graphics&) override;
    void update (NewProjectAudioProcessor& proc, int selectedBand, int selMask = -1);
    const HemingLookAndFeel* lnf = nullptr;
private:
    float scopeDb[256] = {};
    int selBand = 0;
    int selMask = 1;            // bitmask of all selected bands

    // Spectrum history ghosts (phosphor decay) — decimated snapshots
    static constexpr int kGhosts = 4, kGhostPts = 128;
    float ghost[kGhosts][kGhostPts] = {};
    int   ghostPos = 0, ghostTick = 0;
    bool  ghostValid[kGhosts] = {};

    static constexpr int kResponsePts = 256;
    float filterResponse[8][kResponsePts] = {};
    bool  bandEnabled[8] = {};
    float bandFreq[8] = {}, bandRes[8] = {}, bandPoles[8] = {};
    float bandGateDb[8] = {};
    float bandOscHz[8] = {}, bandEnv[8] = {};
    bool  bandGate[8] = {};
    double sampleRate = 48000.0;

    static float hzToX (float hz, float w);
    static float dbToY (float dB, float h);
    void computeFilterResponse (int band);
};

//==============================================================================
//  (2) Event field — scrolling time × pitch ethogram of the ecosystem:
//      every detection event of every voice, sized by level, aged to the left
//==============================================================================
class ImpulseSpaceComponent : public juce::Component
{
public:
    void paint (juce::Graphics&) override;
    void update (NewProjectAudioProcessor& proc, int selectedBand, int selMask = -1);
    const HemingLookAndFeel* lnf = nullptr;

    static constexpr int kHist = 120;     // frames @ 20 Hz  →  6 s window
private:
    int selBand = 0;
    int selMaskLocal = 1;

    struct Frame { float hz = 0, conf = 0, level = 0; bool gate = false, onset = false; };
    Frame  hist[8][kHist] = {};
    int    head = 0;                      // shared write head (one update() per tick)
    bool   prevGate[8] = {};
    bool   bandEnabled[8] = {};

    // last registered onset, for the readout line
    int    lastBand = -1;
    float  lastHz = 0, lastConf = 0;

    static float hzToNorm (float hz);     // 40 Hz .. 4 kHz, log2
};

//==============================================================================
//  (3) Voice view — engine-aware single-cycle waveform of the selected
//      voice(s) + the primary voice's ADSR envelope profile
//==============================================================================
class WaveformDisplayComponent : public juce::Component
{
public:
    void paint (juce::Graphics&) override;
    void update (NewProjectAudioProcessor& proc, int selectedBand, int selMask = -1);
    const HemingLookAndFeel* lnf = nullptr;
private:
    int selBand = 0;
    int selMaskLocal = 1;

    struct BandWaveState {
        int waveL = 0, waveR = 3;
        float mix = -1.0f;
        float noiseAmt = 0.0f;
        float toneAmt = 0.0f;
        int engine = 0;
        float timbre = 0.5f;
        float env = 0.0f;                 // live excitation 0..1
        juce::Colour bandCol { 0xffe8e6de };
    };
    BandWaveState bandStates[8];
    float adsr[4] = { 0.01f, 0.3f, 0.7f, 0.5f };   // primary band A/D/S/R

    static float osc (int waveType, float phase01);
};

//==============================================================================
class NewProjectAudioProcessorEditor : public juce::AudioProcessorEditor,
                                       private juce::Timer
{
public:
    explicit NewProjectAudioProcessorEditor (NewProjectAudioProcessor&);
    ~NewProjectAudioProcessorEditor() override;
    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void handleBandClick (int band, const juce::ModifierKeys& mods);
    void setSelectedBand (int);
    void rebindPrimaryBand (int band);
    void applyVoiceTint (int band);
    void propagateKnobToSelectedBands (juce::String (*paramIdFunc)(int), juce::Slider& knob);
    void installKnobPropagation();
    void cycleWave (bool);
    void updateWaveButtons();
    void setEngineForSelectedBands (int engineIdx);
    void showPresetMenu();
    void syncNoteButtonsFromParam();
    void writeNoteButtonsToParam();
    void resetAllBandNotesToScale();

    NewProjectAudioProcessor& audioProcessor;
    HemingLookAndFeel lnf;

    std::unique_ptr<juce::Drawable> toucanDrawable;
    void createToucanDrawable();

    int selectedBand = 0;       // primary band (for attachments)
    int bandSelMask = 1;        // bitmask: bit i = band i selected
    int lastClickedBand = 0;    // for SHIFT range selection
    bool propagating = false;   // guard against recursive propagation

    BandSelectButton    bandSelBtns[8];
    BandLampButton      bandLampBtns[8];
    std::unique_ptr<ButtonAttachment> bandOnAtts[8];
    ScaleModeButton     scaleModeBtn;
    std::unique_ptr<ButtonAttachment> scaleModeAtt;
    WaveformButton      waveBtnL, waveBtnR;
    QuantizerNoteButton noteBtns[12];
    OctaveControl       octaveCtrl;
    MixerComponent      mixer;
    EngineSelector      engineSel;

    // Header buttons
    juce::TextButton midiOutBtn;
    std::unique_ptr<ButtonAttachment> midiOutAtt;
    juce::TextButton presetBtn;

    // Visualizations
    SpectrumAnalyzerComponent spectrumViz;
    ImpulseSpaceComponent     spaceViz;
    WaveformDisplayComponent  waveformViz;

    // Knobs
    juce::Slider knobScaleRoot, knobWidth, knobDryWet;
    std::unique_ptr<SliderAttachment> attScaleRoot, attWidth, attDryWet;
    juce::Slider knobRandom, knobReverb, knobHP, knobTP, knobTone, knobNoise;
    std::unique_ptr<SliderAttachment> attReverb, attRandom;
    juce::Slider knobDrive, knobDelayMix, knobDelayTime, knobDelayFb, knobComp;
    std::unique_ptr<SliderAttachment> attDrive, attDelayMix, attDelayTime, attDelayFb, attComp;
    juce::Slider knobSnap, knobFreq, knobRes, knobSlope;
    juce::Slider knobThreshold, knobTolerance, knobSensitivity, knobConfidence;
    juce::Slider knobWaveform, knobTimbre, knobA, knobD, knobS, knobR;
    std::unique_ptr<SliderAttachment> attSnap, attFreq, attRes, attSlope;
    std::unique_ptr<SliderAttachment> attThreshold, attTolerance, attSensitivity, attConfidence;
    std::unique_ptr<SliderAttachment> attWaveform, attTimbre, attA, attD, attS, attR;
    std::unique_ptr<SliderAttachment> attNoise, attTone, attHP, attLP;

    int  lastScaleRoot = -1;
    bool lastScaleMinor = false;

    //==========================================================================
    // Layout — module zones (computed once in resized(), shared by paint())
    juce::Rectangle<int> zHeader, zVoices, zAnalysis, zQuant, zSynth,
                         zFilter, zImpulse, zEvents, zMix, zFx, zFooter;

    void drawHeader   (juce::Graphics&) const;
    void drawFrames   (juce::Graphics&) const;
    void drawCaptions (juce::Graphics&) const;

    float readParam (const juce::String&) const;
    static juce::String fmtPercent01 (float);
    static juce::String fmtHz (float);
    static juce::String fmtInt (float);
    static juce::String fmtNote (int);
    static juce::String fmtGatePercent (float);
    static juce::String fmtSeconds (float);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NewProjectAudioProcessorEditor)
};

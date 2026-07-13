#pragma once
#include <JuceHeader.h>

//==============================================================================
//  GEMS v2 — "Field Laboratory" design system.
//
//  Thesis: GEMS is a bioacoustic laboratory instrument. The UI is typeset like
//  measurement, not decorated like a synth: ink on carbon, hairline frames,
//  numbered process stages, scientific mono for every datum. Colour exists
//  only to tag the eight voices.
//
//  Type:   Space Grotesk (titles / wordmark) + Space Mono (all data).
//          Both SIL OFL 1.1 — embeddable in commercial software.
//          (Class keeps its historical name so the project files stay put.)
//==============================================================================
class HemingLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    struct Theme
    {
        juce::Colour carbon   { 0xff0b0c0b };   // window ground
        juce::Colour well     { 0xff111211 };   // recessed instrument wells (plots)
        juce::Colour ink      { 0xffe8e6de };   // warm ink / phosphor
        juce::Colour hairline { 0xff30312e };   // frame lines (used at full alpha)
    };

    HemingLookAndFeel();
    ~HemingLookAndFeel() override = default;

    const Theme& theme() const noexcept { return th; }

    // Ink at standard emphasis steps
    juce::Colour ink   (float alpha = 1.0f) const { return th.ink.withAlpha (alpha); }

    //==========================================================================
    // Typography
    juce::Font grot     (float height, float tracking = 0.06f) const;  // titles, caps labels
    juce::Font mono     (float height, float tracking = 0.0f)  const;  // data, values, axes
    juce::Font monoBold (float height, float tracking = 0.0f)  const;  // emphatic data

    //==========================================================================
    // Module frame: hairline rect whose top edge carries the stage tag
    //   [NN] TITLE ................................. NOTE
    void drawModuleFrame (juce::Graphics&, juce::Rectangle<int> r,
                          const juce::String& index, const juce::String& title,
                          const juce::String& note = {}) const;

    // Caption under a knob: TITLE (grot, dim) over VALUE (mono, ink)
    void drawKnobCaption (juce::Graphics&, const juce::Rectangle<int>& knobBounds,
                          const juce::String& title, const juce::String& value,
                          int width = 0) const;

    //==========================================================================
    // LookAndFeel overrides
    juce::Typeface::Ptr getTypefaceForFont (const juce::Font&) override;

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h,
                           float sliderPosProportional,
                           float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider&) override;

    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           const juce::Slider::SliderStyle, juce::Slider&) override;

    void drawButtonBackground (juce::Graphics&, juce::Button&,
                               const juce::Colour& backgroundColour,
                               bool highlighted, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&,
                         bool highlighted, bool down) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&,
                           bool highlighted, bool down) override;

    void drawLabel (juce::Graphics&, juce::Label&) override;

    // Popup menu (engine picker, presets)
    void drawPopupMenuBackground (juce::Graphics&, int width, int height) override;
    void drawPopupMenuItem (juce::Graphics&, const juce::Rectangle<int>& area,
                            bool isSeparator, bool isActive, bool isHighlighted,
                            bool isTicked, bool hasSubMenu,
                            const juce::String& text, const juce::String& shortcutKeyText,
                            const juce::Drawable* icon, const juce::Colour* textColour) override;
    juce::Font getPopupMenuFont() override;
    void getIdealPopupMenuItemSize (const juce::String& text, bool isSeparator,
                                    int standardMenuItemHeight, int& idealWidth,
                                    int& idealHeight) override;

    juce::Font getAlertWindowTitleFont() override;
    juce::Font getAlertWindowMessageFont() override;
    juce::Font getAlertWindowFont() override;

private:
    Theme th;

    juce::Typeface::Ptr tfGrot, tfMono, tfMonoBold;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HemingLookAndFeel)
};

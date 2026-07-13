#include "HemingLookAndFeel.h"
#include "GemsTypeData.h"

//==============================================================================
HemingLookAndFeel::HemingLookAndFeel()
{
    tfGrot     = juce::Typeface::createSystemTypefaceFor (GemsType::kGroteskMedium, GemsType::kGroteskMediumSize);
    tfMono     = juce::Typeface::createSystemTypefaceFor (GemsType::kMonoRegular,   GemsType::kMonoRegularSize);
    tfMonoBold = juce::Typeface::createSystemTypefaceFor (GemsType::kMonoBold,      GemsType::kMonoBoldSize);

    setColour (juce::ResizableWindow::backgroundColourId, th.carbon);

    setColour (juce::Slider::rotarySliderFillColourId,    th.ink);
    setColour (juce::Slider::rotarySliderOutlineColourId, th.ink.withAlpha (0.5f));
    setColour (juce::Slider::textBoxTextColourId,         th.ink);
    setColour (juce::Slider::textBoxOutlineColourId,      th.hairline);

    setColour (juce::Label::textColourId,       th.ink);
    setColour (juce::TextButton::textColourOffId, th.ink);
    setColour (juce::TextButton::textColourOnId,  th.carbon);
    setColour (juce::TextButton::buttonColourId,   juce::Colours::transparentBlack);
    setColour (juce::TextButton::buttonOnColourId, th.ink);
    setColour (juce::ToggleButton::textColourId,  th.ink);

    // Popup menu
    setColour (juce::PopupMenu::backgroundColourId,            th.carbon);
    setColour (juce::PopupMenu::textColourId,                  th.ink.withAlpha (0.85f));
    setColour (juce::PopupMenu::highlightedBackgroundColourId, th.ink);
    setColour (juce::PopupMenu::highlightedTextColourId,       th.carbon);

    // Alert window / text editor (preset save dialog)
    setColour (juce::AlertWindow::backgroundColourId, th.carbon);
    setColour (juce::AlertWindow::textColourId,       th.ink);
    setColour (juce::AlertWindow::outlineColourId,    th.hairline);
    setColour (juce::TextEditor::backgroundColourId,      th.well);
    setColour (juce::TextEditor::textColourId,            th.ink);
    setColour (juce::TextEditor::highlightColourId,       th.ink.withAlpha (0.25f));
    setColour (juce::TextEditor::outlineColourId,         th.hairline);
    setColour (juce::TextEditor::focusedOutlineColourId,  th.ink.withAlpha (0.6f));
    setColour (juce::CaretComponent::caretColourId,       th.ink);
}

//==============================================================================
// Typography
static juce::Font makeFont (const juce::Typeface::Ptr& tf, float h, float tracking)
{
    if (tf != nullptr)
        return juce::Font (juce::FontOptions (tf).withHeight (h).withKerningFactor (tracking));

    return juce::Font (juce::FontOptions ((float) h));
}

juce::Font HemingLookAndFeel::grot     (float h, float tr) const { return makeFont (tfGrot,     h, tr); }
juce::Font HemingLookAndFeel::mono     (float h, float tr) const { return makeFont (tfMono,     h, tr); }
juce::Font HemingLookAndFeel::monoBold (float h, float tr) const { return makeFont (tfMonoBold, h, tr); }

juce::Typeface::Ptr HemingLookAndFeel::getTypefaceForFont (const juce::Font& f)
{
    if (tfMono != nullptr && f.getTypefaceName().containsIgnoreCase ("mono"))
        return tfMono;
    if (tfGrot != nullptr)
        return tfGrot;
    return juce::LookAndFeel_V4::getTypefaceForFont (f);
}

//==============================================================================
// Module frame
void HemingLookAndFeel::drawModuleFrame (juce::Graphics& g, juce::Rectangle<int> r,
                                         const juce::String& index, const juce::String& title,
                                         const juce::String& note) const
{
    g.setColour (th.hairline);
    g.drawRect (r, 1);

    const float lineY = (float) r.getY();
    float x = (float) r.getX() + 8.0f;

    // Stage index chip — small filled square of ink sitting on the line
    if (index.isNotEmpty())
    {
        auto f = monoBold (8.0f);
        const float tw = juce::GlyphArrangement::getStringWidth (f, index);
        juce::Rectangle<float> chip (x, lineY - 6.0f, tw + 8.0f, 12.0f);

        g.setColour (th.carbon);
        g.fillRect (chip.expanded (2.0f, 0.0f));
        g.setColour (th.ink);
        g.fillRect (chip);
        g.setColour (th.carbon);
        g.setFont (f);
        g.drawText (index, chip, juce::Justification::centred, false);
        x = chip.getRight() + 5.0f;
    }

    if (title.isNotEmpty())
    {
        auto f = grot (10.0f, 0.10f);
        const float tw = juce::GlyphArrangement::getStringWidth (f, title);
        juce::Rectangle<float> band (x, lineY - 6.0f, tw + 6.0f, 12.0f);

        g.setColour (th.carbon);
        g.fillRect (band.expanded (2.0f, 0.0f));
        g.setColour (ink (0.88f));
        g.setFont (f);
        g.drawText (title, band, juce::Justification::centredLeft, false);
    }

    if (note.isNotEmpty())
    {
        auto f = mono (7.0f);
        const float tw = juce::GlyphArrangement::getStringWidth (f, note);
        juce::Rectangle<float> band ((float) r.getRight() - tw - 14.0f, lineY - 5.0f, tw + 6.0f, 10.0f);

        g.setColour (th.carbon);
        g.fillRect (band.expanded (2.0f, 0.0f));
        g.setColour (ink (0.38f));
        g.setFont (f);
        g.drawText (note, band, juce::Justification::centred, false);
    }
}

void HemingLookAndFeel::drawKnobCaption (juce::Graphics& g, const juce::Rectangle<int>& kb,
                                         const juce::String& title, const juce::String& value,
                                         int width) const
{
    const int w  = (width > 0 ? width : juce::jmax (kb.getWidth(), 56));
    const int cx = kb.getCentreX();

    juce::Rectangle<int> t (cx - w / 2, kb.getBottom() + 2, w, 9);
    g.setColour (ink (0.52f));
    g.setFont (grot (8.5f, 0.10f));
    g.drawText (title, t, juce::Justification::centred, false);

    if (value.isNotEmpty())
    {
        juce::Rectangle<int> v (cx - w / 2, t.getBottom() + 1, w, 10);
        g.setColour (ink (0.92f));
        g.setFont (mono (8.5f));
        g.drawText (value, v, juce::Justification::centred, false);
    }
}

//==============================================================================
// Rotary — flat laboratory dial: hairline track ring, accent value arc, needle
void HemingLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                          float sliderPos, float start, float end,
                                          juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const float d  = juce::jmin (bounds.getWidth(), bounds.getHeight());
    const float cx = bounds.getCentreX(), cy = bounds.getCentreY();

    const float angle = start + sliderPos * (end - start);
    const float alpha = slider.isEnabled() ? 1.0f : 0.28f;
    const bool  hot   = slider.isMouseOverOrDragging() && slider.isEnabled();

    const auto accent = slider.findColour (juce::Slider::rotarySliderFillColourId);

    const float arcW = juce::jlimit (1.6f, 2.4f, d * 0.055f);
    const float arcR = d * 0.5f - arcW;

    // Track ring (full sweep, hairline)
    {
        juce::Path track;
        track.addCentredArc (cx, cy, arcR, arcR, 0.0f, start, end, true);
        g.setColour (ink (0.16f * alpha));
        g.strokePath (track, juce::PathStrokeType (1.0f));
    }

    // Range end ticks
    {
        g.setColour (ink (0.30f * alpha));
        for (float a : { start, end })
        {
            const float ca = std::cos (a - juce::MathConstants<float>::halfPi);
            const float sa = std::sin (a - juce::MathConstants<float>::halfPi);
            g.drawLine (cx + ca * (arcR - 2.0f), cy + sa * (arcR - 2.0f),
                        cx + ca * (arcR + 2.0f), cy + sa * (arcR + 2.0f), 1.0f);
        }
    }

    // Value arc
    if (sliderPos > 0.002f)
    {
        juce::Path arc;
        arc.addCentredArc (cx, cy, arcR, arcR, 0.0f, start, angle, true);
        g.setColour (accent.withAlpha ((hot ? 1.0f : 0.85f) * alpha));
        g.strokePath (arc, juce::PathStrokeType (arcW));
    }

    // Needle — centre dot to arc
    {
        const float ca = std::cos (angle - juce::MathConstants<float>::halfPi);
        const float sa = std::sin (angle - juce::MathConstants<float>::halfPi);
        g.setColour (ink ((hot ? 1.0f : 0.80f) * alpha));
        g.drawLine (cx, cy, cx + ca * (arcR - 2.5f), cy + sa * (arcR - 2.5f), 1.2f);
        g.fillEllipse (cx - 1.4f, cy - 1.4f, 2.8f, 2.8f);
    }
}

//==============================================================================
// Linear slider — mixer faders: caliper thumb over the meters drawn behind
void HemingLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h,
                                          float sliderPos, float, float,
                                          const juce::Slider::SliderStyle style, juce::Slider& slider)
{
    const float a = slider.isMouseButtonDown() ? 1.0f : slider.isMouseOver() ? 0.9f : 0.75f;

    if (style == juce::Slider::LinearVertical)
    {
        g.setColour (ink (a));
        g.fillRect ((float) x, sliderPos - 1.0f, (float) w, 2.0f);
        // centre grip block
        g.fillRect ((float) x + w * 0.5f - 2.5f, sliderPos - 3.5f, 5.0f, 7.0f);
    }
    else
    {
        const float cy = (float) y + (float) h * 0.5f;
        g.setColour (ink (0.14f));
        g.drawLine ((float) x, cy, (float) (x + w), cy, 1.0f);
        g.setColour (ink (a));
        g.fillRect (sliderPos - 1.0f, cy - 4.0f, 2.0f, 8.0f);
    }
}

//==============================================================================
// Buttons — square hairline cells; toggled = solid ink, carbon text
void HemingLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b,
                                              const juce::Colour&, bool highlighted, bool down)
{
    auto r = b.getLocalBounds();
    const bool on = b.getToggleState();

    if (on || down)
    {
        g.setColour (th.ink.withAlpha (down && ! on ? 0.18f : 0.92f));
        g.fillRect (r);
    }

    g.setColour (on ? th.ink : ink (highlighted ? 0.75f : 0.35f));
    g.drawRect (r, 1);
}

juce::Font HemingLookAndFeel::getTextButtonFont (juce::TextButton&, int)
{
    return mono (8.0f, 0.08f);
}

void HemingLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool highlighted, bool down)
{
    const bool on = b.getToggleState() || (down && b.getClickingTogglesState());
    g.setColour (on ? th.carbon : ink (highlighted ? 1.0f : 0.85f));
    g.setFont (getTextButtonFont (b, b.getHeight()));
    g.drawText (b.getButtonText().toUpperCase(), b.getLocalBounds(), juce::Justification::centred, false);
}

void HemingLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b,
                                          bool highlighted, bool down)
{
    auto r = b.getLocalBounds().toFloat();
    auto box = r.removeFromLeft (juce::jmin (16.0f, r.getHeight())).reduced (3.0f);
    r.removeFromLeft (5.0f);

    const bool on = b.getToggleState();

    if (on) { g.setColour (ink (0.92f)); g.fillRect (box); }
    g.setColour (ink (on ? 0.95f : (highlighted || down ? 0.8f : 0.4f)));
    g.drawRect (box, 1.0f);

    g.setColour (ink (b.isEnabled() ? 0.85f : 0.3f));
    g.setFont (mono (9.0f));
    g.drawFittedText (b.getButtonText().toUpperCase(), r.toNearestInt(), juce::Justification::centredLeft, 1);
}

void HemingLookAndFeel::drawLabel (juce::Graphics& g, juce::Label& l)
{
    g.setColour (ink (l.isEnabled() ? 0.9f : 0.35f));
    g.setFont (mono ((float) l.getFont().getHeight()));
    g.drawFittedText (l.getText(), l.getLocalBounds().reduced (2),
                      l.getJustificationType(), 1);
}

//==============================================================================
// Popup menu
void HemingLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int w, int h)
{
    g.fillAll (th.carbon);
    g.setColour (ink (0.45f));
    g.drawRect (0, 0, w, h, 1);
}

juce::Font HemingLookAndFeel::getPopupMenuFont() { return mono (10.0f); }

void HemingLookAndFeel::getIdealPopupMenuItemSize (const juce::String& text, bool isSeparator,
                                                   int, int& idealWidth, int& idealHeight)
{
    if (isSeparator) { idealWidth = 40; idealHeight = 7; return; }
    auto f = getPopupMenuFont();
    idealHeight = 19;
    idealWidth  = (int) juce::GlyphArrangement::getStringWidth (f, text) + 40;
}

void HemingLookAndFeel::drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area,
                                           bool isSeparator, bool isActive, bool isHighlighted,
                                           bool isTicked, bool hasSubMenu,
                                           const juce::String& text, const juce::String&,
                                           const juce::Drawable*, const juce::Colour*)
{
    if (isSeparator)
    {
        g.setColour (th.hairline);
        g.fillRect (area.reduced (6, 0).withHeight (1).withY (area.getCentreY()));
        return;
    }

    auto r = area.reduced (1);

    if (isHighlighted && isActive)
    {
        g.setColour (ink (0.92f));
        g.fillRect (r);
    }

    const auto fg = (isHighlighted && isActive) ? th.carbon
                                                : ink (isActive ? 0.85f : 0.3f);

    // tick — small filled square
    if (isTicked)
    {
        g.setColour (fg);
        g.fillRect (r.getX() + 7, r.getCentreY() - 2, 4, 4);
    }

    g.setColour (fg);
    g.setFont (getPopupMenuFont());
    g.drawText (text.toUpperCase(), r.withTrimmedLeft (18).withTrimmedRight (12),
                juce::Justification::centredLeft, false);

    if (hasSubMenu)
    {
        juce::Path p;
        const float cx = (float) r.getRight() - 9.0f, cy = (float) r.getCentreY();
        p.addTriangle (cx, cy - 3.0f, cx, cy + 3.0f, cx + 4.0f, cy);
        g.setColour (fg);
        g.fillPath (p);
    }
}

//==============================================================================
juce::Font HemingLookAndFeel::getAlertWindowTitleFont()   { return grot (15.0f, 0.04f); }
juce::Font HemingLookAndFeel::getAlertWindowMessageFont() { return mono (11.0f); }
juce::Font HemingLookAndFeel::getAlertWindowFont()        { return mono (10.0f); }

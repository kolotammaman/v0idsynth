// =============================================================================
// Phase 5 / UI-01 — V0idSynthLookAndFeel painting implementation.
// Per .planning/phases/05-ui-and-aesthetic/05-UI-SPEC.md
// Decisions: D-14 (knob), D-15 (hover/focus), D-16 (label), D-17/D-18 (tooltip),
//            D-20 (preset browser buttons/combo).
// =============================================================================

#include "V0idSynthLookAndFeel.h"
#include <cmath>

// =============================================================================
// Constructor — cache typeface and set global colour IDs
// =============================================================================

V0idSynthLookAndFeel::V0idSynthLookAndFeel()
{
    setColour(juce::ResizableWindow::backgroundColourId, V0idColors::background);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::ComboBox::backgroundColourId, V0idColors::background);
    setColour(juce::ComboBox::outlineColourId, V0idColors::accentDivider());
    setColour(juce::ComboBox::textColourId, V0idColors::accentLabel());
    setColour(juce::ComboBox::arrowColourId, V0idColors::accentLabel());
    setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    setColour(juce::TextButton::textColourOnId, V0idColors::accent);
    setColour(juce::TextButton::textColourOffId, V0idColors::accentLabel());
    setColour(juce::Label::textColourId, V0idColors::accentLabel());
    setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::TooltipWindow::backgroundColourId, V0idColors::background);
    setColour(juce::TooltipWindow::textColourId, V0idColors::accent);
    setColour(juce::TooltipWindow::outlineColourId, V0idColors::accent);

    // Popup menus (preset browser, every combo dropdown) were still painting in
    // LookAndFeel_V4's stock blue-grey scheme.
    setColour(juce::PopupMenu::backgroundColourId,            V0idColors::background);
    setColour(juce::PopupMenu::textColourId,                  V0idColors::accentLabel());
    setColour(juce::PopupMenu::headerTextColourId,            V0idColors::accent);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, V0idColors::accent.withAlpha(0.12f));
    setColour(juce::PopupMenu::highlightedTextColourId,       V0idColors::accent);
    setColour(juce::ScrollBar::thumbColourId,                 V0idColors::accentMid());

    monoTypeface = juce::Typeface::createSystemTypefaceFor(
        juce::Font(juce::FontOptions("Courier New", 12.0f, juce::Font::plain)));
    monoBoldTypeface = juce::Typeface::createSystemTypefaceFor(
        juce::Font(juce::FontOptions("Courier New", 12.0f, juce::Font::bold)));
}

// =============================================================================
// Typography — D-05: Courier New monospace, Menlo fallback via JUCE
// =============================================================================

juce::Typeface::Ptr V0idSynthLookAndFeel::getTypefaceForFont(const juce::Font& font)
{
    // Honour bold: returning the plain face for every font flattened the bold
    // title, section headers and hovered button text to regular weight.
    if (font.isBold() && monoBoldTypeface != nullptr) return monoBoldTypeface;
    if (monoTypeface != nullptr) return monoTypeface;
    return juce::LookAndFeel_V4::getTypefaceForFont(
        juce::Font(juce::FontOptions("Courier New", 12.0f, juce::Font::plain)));
}

// =============================================================================
// Rotary knob — UI-SPEC "Rotary Knob Contract" (D-14, D-15, D-16)
// =============================================================================

void V0idSynthLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                            float sliderPos, float rotaryStartAngle,
                                            float rotaryEndAngle, juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();
    const float diameter = juce::jmin(bounds.getWidth(), bounds.getHeight()) - 8.0f;
    const auto centre = bounds.getCentre();
    const float radius = diameter * 0.5f;

    const float trackRadius = radius - 3.0f;
    const float trackThick  = 2.0f;
    const float valueThick  = 2.0f;
    const float pointerR    = 2.5f; // 5px diameter
    const float bodyRadius  = radius - 6.0f;
    const float innerR      = bodyRadius * 0.35f;

    const bool isHot = slider.isMouseOverOrDragging() || slider.hasKeyboardFocus(false);
    const auto trackCol   = V0idColors::accentDim();
    const auto valueCol   = isHot ? V0idColors::accent : V0idColors::accentMid();
    const auto pointerCol = isHot ? V0idColors::accent : V0idColors::accentMid();
    const auto bodyCol    = V0idColors::surface;
    const auto innerCol   = V0idColors::highlight;

    // Track arc (full sweep — dim accent)
    juce::Path track;
    track.addCentredArc(centre.x, centre.y, trackRadius, trackRadius, 0.0f,
                        rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(trackCol);
    g.strokePath(track, juce::PathStrokeType(trackThick));

    // Value arc — from the start, or from 12 o'clock for bipolar knobs (depth /
    // env amount) so "none" reads as an empty ring and inversion sweeps left.
    const float currentAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    const bool  bipolar      = (bool) slider.getProperties().getWithDefault("bipolar", false);
    const float arcOrigin    = bipolar ? (rotaryStartAngle + rotaryEndAngle) * 0.5f : rotaryStartAngle;
    juce::Path value;
    value.addCentredArc(centre.x, centre.y, trackRadius, trackRadius, 0.0f,
                        juce::jmin(arcOrigin, currentAngle), juce::jmax(arcOrigin, currentAngle), true);
    if (bipolar)
    {
        // Centre detent tick on the track
        const float tx = centre.x + std::cos(arcOrigin - juce::MathConstants<float>::halfPi) * trackRadius;
        const float ty = centre.y + std::sin(arcOrigin - juce::MathConstants<float>::halfPi) * trackRadius;
        g.setColour(V0idColors::accentLabel());
        g.fillRect(juce::Rectangle<float>(1.0f, 5.0f).withCentre({ tx, ty }));
    }

    // Phosphor bloom under the hot value arc — a wide low-alpha pass, like a CRT
    // trace lighting the glass around it.
    if (isHot)
    {
        g.setColour(V0idColors::accent.withAlpha(0.18f));
        g.strokePath(value, juce::PathStrokeType(valueThick + 4.0f,
                                                 juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));
    }

    g.setColour(valueCol);
    g.strokePath(value, juce::PathStrokeType(valueThick));

    // Knob body — surface fill + inner highlight circle
    g.setColour(bodyCol);
    g.fillEllipse(centre.x - bodyRadius, centre.y - bodyRadius, bodyRadius * 2, bodyRadius * 2);
    g.setColour(innerCol);
    g.fillEllipse(centre.x - innerR, centre.y - innerR, innerR * 2, innerR * 2);

    // Pointer dot at the current value position on the track ring
    const float pointerX = centre.x + std::cos(currentAngle - juce::MathConstants<float>::halfPi) * trackRadius;
    const float pointerY = centre.y + std::sin(currentAngle - juce::MathConstants<float>::halfPi) * trackRadius;
    if (isHot)
    {
        const float bloomR = pointerR + 3.5f;
        g.setColour(V0idColors::accent.withAlpha(0.22f));
        g.fillEllipse(pointerX - bloomR, pointerY - bloomR, bloomR * 2, bloomR * 2);
    }
    g.setColour(pointerCol);
    g.fillEllipse(pointerX - pointerR, pointerY - pointerR, pointerR * 2, pointerR * 2);

    // Label below knob: all-caps, 9pt Courier New, 45% accent, 8-char max + ellipsis
    auto name = slider.getName().toUpperCase();
    if (name.length() > 8)
        name = name.substring(0, 7) + juce::String(juce::CharPointer_UTF8("\xe2\x80\xa6"));
    g.setColour(V0idColors::accentLabel());
    g.setFont(juce::Font(juce::FontOptions("Courier New", 9.0f, juce::Font::plain)));
    g.drawText(name, x, y + height - 12, width, 12, juce::Justification::centred, false);
}

// =============================================================================
// Linear bar sliders — name on the left, value on the right
// =============================================================================

void V0idSynthLookAndFeel::drawLinearSlider(juce::Graphics& g,
                                             int x, int y, int width, int height,
                                             float sliderPos, float /*minSliderPos*/,
                                             float /*maxSliderPos*/,
                                             juce::Slider::SliderStyle style,
                                             juce::Slider& slider)
{
    if (style != juce::Slider::LinearBar)
    {
        LookAndFeel_V4::drawLinearSlider(g, x, y, width, height,
                                          sliderPos, 0, 0, style, slider);
        return;
    }

    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();

    // Background
    g.setColour(V0idColors::background);
    g.fillRect(bounds);

    // Filled portion — from left up to sliderPos; brightens on hover/drag to
    // match the rotary knob hot state.
    const bool isHot = slider.isMouseOverOrDragging() || slider.hasKeyboardFocus(false);
    float fillW = juce::jlimit(0.0f, (float)width, sliderPos - (float)x);
    g.setColour(isHot ? V0idColors::accent.withAlpha(0.25f) : V0idColors::accentDim());
    g.fillRect(bounds.withWidth(fillW));

    // Border
    g.setColour(V0idColors::accentDivider());
    g.drawRect(bounds, 1.0f);

    const auto labelFont = juce::Font(juce::FontOptions("Courier New", 9.0f, juce::Font::plain));
    g.setFont(labelFont);

    // Name — left-aligned, dim accent
    auto name = slider.getName().toUpperCase();
    g.setColour(V0idColors::accentLabel());
    g.drawText(name, bounds.withTrimmedLeft(4.0f), juce::Justification::centredLeft, false);

    // Value — right-aligned, full accent
    auto valueText = slider.getTextFromValue(slider.getValue());
    g.setColour(V0idColors::accent);
    g.drawText(valueText, bounds.withTrimmedRight(4.0f), juce::Justification::centredRight, false);
}

// =============================================================================
// Buttons — flat, no fill (preset strip Prev / Next)
// =============================================================================

void V0idSynthLookAndFeel::drawButtonBackground(juce::Graphics&, juce::Button&,
                                                const juce::Colour&, bool, bool)
{
    // Intentionally empty — preset Prev/Next buttons have no background fill
}

void V0idSynthLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& b,
                                          bool isOver, bool isDown)
{
    const auto col = (isOver || isDown) ? V0idColors::accent : V0idColors::accentLabel();
    g.setColour(col);
    g.setFont(juce::Font(juce::FontOptions("Courier New", 13.0f,
                                            isOver || isDown ? juce::Font::bold : juce::Font::plain)));
    g.drawText(b.getButtonText(), b.getLocalBounds(), juce::Justification::centred, false);
}

// =============================================================================
// ComboBox — dark background, dim accent text, ">" glyph arrow
// =============================================================================

void V0idSynthLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool,
                                        int buttonX, int buttonY, int buttonW, int buttonH,
                                        juce::ComboBox&)
{
    auto r = juce::Rectangle<int>(0, 0, width, height).toFloat();
    g.setColour(V0idColors::background);
    g.fillRect(r);
    g.setColour(V0idColors::accentDivider());
    g.drawRect(r, 1.0f);

    // Arrow drawn as ">" glyph at far right
    g.setColour(V0idColors::accentLabel());
    g.setFont(juce::Font(juce::FontOptions("Courier New", 10.0f, juce::Font::plain)));
    g.drawText(">", juce::Rectangle<int>(buttonX, buttonY, buttonW, buttonH),
               juce::Justification::centred, false);
}

// =============================================================================
// Labels — monospace, color from label's own colour-id properties
// =============================================================================

void V0idSynthLookAndFeel::drawLabel(juce::Graphics& g, juce::Label& label)
{
    if (! label.isBeingEdited())
    {
        g.setColour(label.findColour(juce::Label::textColourId));
        const auto labelFont = juce::Font(juce::FontOptions("Courier New",
                                                             label.getFont().getHeight(),
                                                             label.getFont().getStyleFlags()));
        g.setFont(labelFont);
        g.drawFittedText(label.getText(), label.getLocalBounds(),
                          label.getJustificationType(), 1, 1.0f);
    }
}

// =============================================================================
// Tooltip — UI-SPEC "Tooltip Contract" (D-17, D-18)
// #0d0b08 fill, 1px #b8ff6a border, sharp corners, 10pt Courier New
// =============================================================================

// Shared by drawTooltip and getTooltipBounds so the measured box always matches
// what gets drawn. Wraps at kMaxContentWidth.
static juce::TextLayout createTooltipLayout(const juce::String& text)
{
    constexpr float kMaxContentWidth = 308.0f; // 320 total − 6px padding each side
    const juce::Font tooltipFont(juce::FontOptions("Courier New", 10.0f, juce::Font::plain));

    juce::AttributedString s;
    s.setJustification(juce::Justification::topLeft);
    s.append(text, tooltipFont, V0idColors::accent);

    juce::TextLayout layout;
    layout.createLayout(s, kMaxContentWidth);
    return layout;
}

void V0idSynthLookAndFeel::drawTooltip(juce::Graphics& g, const juce::String& text,
                                       int width, int height)
{
    const auto r = juce::Rectangle<int>(0, 0, width, height).toFloat();
    g.setColour(V0idColors::background);
    g.fillRect(r);
    g.setColour(V0idColors::accent);
    g.drawRect(r, 1.0f);
    createTooltipLayout(text).draw(g, r.reduced(6.0f, 4.0f));
}

juce::Rectangle<int> V0idSynthLookAndFeel::getTooltipBounds(const juce::String& tipText,
                                                             juce::Point<int> screenPos,
                                                             juce::Rectangle<int> parentArea)
{
    const auto layout = createTooltipLayout(tipText);
    const int width  = juce::jlimit(40, 320, (int) std::ceil(layout.getWidth())  + 12);
    const int height = (int) std::ceil(layout.getHeight()) + 8;
    return juce::Rectangle<int>(screenPos.x, screenPos.y + 18, width, height)
        .constrainedWithin(parentArea);
}

// =============================================================================
// Popup menus — terminal-style list: flat near-black panel, 1px phosphor frame,
// sharp corners, ">" marker on the hovered row and the current selection.
// =============================================================================

void V0idSynthLookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int width, int height)
{
    const auto r = juce::Rectangle<int>(0, 0, width, height).toFloat();
    g.fillAll(V0idColors::background);
    g.setColour(V0idColors::accent.withAlpha(0.35f));
    g.drawRect(r, 1.0f);
}

void V0idSynthLookAndFeel::drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                                             bool isSeparator, bool isActive, bool isHighlighted,
                                             bool isTicked, bool hasSubMenu,
                                             const juce::String& text, const juce::String&,
                                             const juce::Drawable*, const juce::Colour*)
{
    if (isSeparator)
    {
        g.setColour(V0idColors::accentDivider());
        g.fillRect(area.reduced(6, 0).withHeight(1).withY(area.getCentreY()));
        return;
    }

    auto r = area.reduced(1, 0);
    if (isHighlighted && isActive)
    {
        g.setColour(findColour(juce::PopupMenu::highlightedBackgroundColourId));
        g.fillRect(r);
    }

    const auto textCol = ! isActive         ? V0idColors::accentDim()
                       : (isHighlighted || isTicked) ? V0idColors::accent
                                                     : V0idColors::accentLabel();
    g.setColour(textCol);
    g.setFont(getPopupMenuFont());

    auto marker = r.removeFromLeft(18);
    if (isTicked || (isHighlighted && isActive))
        g.drawText(">", marker, juce::Justification::centred, false);

    if (hasSubMenu)
    {
        g.drawText("+", r.removeFromRight(16), juce::Justification::centred, false);
    }

    g.drawFittedText(text.toUpperCase(), r.reduced(2, 0), juce::Justification::centredLeft, 1);
}

juce::Font V0idSynthLookAndFeel::getPopupMenuFont()
{
    return juce::Font(juce::FontOptions("Courier New", 12.0f, juce::Font::plain));
}

int V0idSynthLookAndFeel::getPopupMenuBorderSize()
{
    return 2;
}

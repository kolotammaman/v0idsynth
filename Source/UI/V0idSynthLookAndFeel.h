#pragma once
// =============================================================================
// Phase 5 / UI-01 — visual contract per
// .planning/phases/05-ui-and-aesthetic/05-UI-SPEC.md
//
// Applied globally by V0idSynthAudioProcessorEditor::setLookAndFeel (plan 05-03)
//
// V0idSynthLookAndFeel subclasses juce::LookAndFeel_V4 and paints all standard
// JUCE components in the v0idsynth aesthetic:
//   warm near-black background  (#0d0b08)
//   fluorescent green accent    (#b8ff6a)
//   monospace Courier New / Menlo typography
//   CRT-scan-line-friendly flat surfaces
// =============================================================================

#include <juce_gui_basics/juce_gui_basics.h>

// =============================================================================
// V0idColors — single source of truth for the UI palette (UI-SPEC § Color)
// =============================================================================

namespace V0idColors
{
    // -----------------------------------------------------------------------
    // Base colours (60 / 30 / 10 distribution)
    // -----------------------------------------------------------------------
    inline const juce::Colour background  { (uint8_t) 0x0d, (uint8_t) 0x0b, (uint8_t) 0x08 };  // #0d0b08 — 60% dominant
    inline const juce::Colour surface     { (uint8_t) 0x16, (uint8_t) 0x13, (uint8_t) 0x0e };  // #16130e — 30% secondary
    inline const juce::Colour highlight   { (uint8_t) 0x2a, (uint8_t) 0x25, (uint8_t) 0x20 };  // #2a2520 — knob inner highlight
    inline const juce::Colour accent      { (uint8_t) 0xb8, (uint8_t) 0xff, (uint8_t) 0x6a };  // #b8ff6a — 10% accent (full)
    inline const juce::Colour destructive { (uint8_t) 0xff, (uint8_t) 0x45, (uint8_t) 0x45 };  // unused in Phase 5
    inline const juce::Colour scanLine    { (uint8_t) 0x00, (uint8_t) 0x00, (uint8_t) 0x00 };  // used at 7% alpha

    // -----------------------------------------------------------------------
    // Pre-computed alpha variants (factory wrappers — not constexpr in JUCE 8
    // because Colour::withAlpha() is a non-trivial member function)
    // -----------------------------------------------------------------------
    inline juce::Colour accentDim()      { return accent.withAlpha(0.13f); } // dim routing dot / unlit ring
    inline juce::Colour accentMid()      { return accent.withAlpha(0.60f); } // resting knob arc + pointer
    inline juce::Colour accentLabel()    { return accent.withAlpha(0.45f); } // labels + section headers
    inline juce::Colour accentDivider()  { return accent.withAlpha(0.10f); } // section divider lines
    inline juce::Colour scanLineAlpha()  { return scanLine.withAlpha(0.07f); } // CRT line overlay

} // namespace V0idColors

// =============================================================================
// V0idSynthLookAndFeel
// =============================================================================

class V0idSynthLookAndFeel : public juce::LookAndFeel_V4
{
public:
    V0idSynthLookAndFeel();
    ~V0idSynthLookAndFeel() override = default;

    // -----------------------------------------------------------------------
    // Knob — UI-SPEC "Rotary Knob Contract"
    // -----------------------------------------------------------------------
    void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider&) override;

    // -----------------------------------------------------------------------
    // Buttons (Prev/Next in preset strip, etc.) — UI-SPEC "Preset Browser Strip Contract"
    // -----------------------------------------------------------------------
    void drawButtonBackground(juce::Graphics&, juce::Button&,
                              const juce::Colour& backgroundColour,
                              bool isMouseOverButton, bool isButtonDown) override;
    void drawButtonText(juce::Graphics&, juce::TextButton&,
                        bool isMouseOverButton, bool isButtonDown) override;

    // -----------------------------------------------------------------------
    // ComboBox (Category combo) — UI-SPEC "Preset Browser Strip Contract"
    // -----------------------------------------------------------------------
    void drawComboBox(juce::Graphics&, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox&) override;

    // -----------------------------------------------------------------------
    // Linear bar sliders (ROOT/TUNE/GAIN/HPF/LPF strips) — name left, value right
    // -----------------------------------------------------------------------
    void drawLinearSlider(juce::Graphics&, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          juce::Slider::SliderStyle, juce::Slider&) override;

    // -----------------------------------------------------------------------
    // Labels (section headers, knob labels, preset name) — typography per UI-SPEC
    // -----------------------------------------------------------------------
    void drawLabel(juce::Graphics&, juce::Label&) override;

    // -----------------------------------------------------------------------
    // Typography — D-05: monospace Courier New / Menlo fallback
    // -----------------------------------------------------------------------
    juce::Typeface::Ptr getTypefaceForFont(const juce::Font&) override;

    // -----------------------------------------------------------------------
    // Tooltip — UI-SPEC "Tooltip Contract"
    // -----------------------------------------------------------------------
    void drawTooltip(juce::Graphics&, const juce::String& text, int width, int height) override;
    juce::Rectangle<int> getTooltipBounds(const juce::String& tipText,
                                          juce::Point<int> screenPos,
                                          juce::Rectangle<int> parentArea) override;

    // -----------------------------------------------------------------------
    // Popup menus (preset browser + combo dropdowns)
    // -----------------------------------------------------------------------
    void drawPopupMenuBackground(juce::Graphics&, int width, int height) override;
    void drawPopupMenuItem(juce::Graphics&, const juce::Rectangle<int>& area,
                           bool isSeparator, bool isActive, bool isHighlighted,
                           bool isTicked, bool hasSubMenu,
                           const juce::String& text, const juce::String& shortcutKeyText,
                           const juce::Drawable* icon, const juce::Colour* textColour) override;
    juce::Font getPopupMenuFont() override;
    int getPopupMenuBorderSize() override;

private:
    juce::Typeface::Ptr monoTypeface;     // cached on construction
    juce::Typeface::Ptr monoBoldTypeface; // bold weight for titles / headers

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(V0idSynthLookAndFeel)
};

#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "V0idSynthLookAndFeel.h"

class SectionLabel : public juce::Component
{
public:
    explicit SectionLabel (juce::String headerText) : text (std::move (headerText)) {}

    void paint (juce::Graphics& g) override
    {
        g.setColour (V0idColors::accentLabel());
        g.setFont (juce::Font (juce::FontOptions ("Courier New", 10.0f, juce::Font::bold)));
        g.drawText (text.toUpperCase(), getLocalBounds().reduced (16, 0),
                    juce::Justification::centredLeft, false);
    }

private:
    juce::String text;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SectionLabel)
};

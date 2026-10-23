#include "MasterSectionComponent.h"
#include "V0idSynthLookAndFeel.h"
#include "Tooltips.h"
#include "../Parameters.h"

MasterSectionComponent::MasterSectionComponent (juce::AudioProcessorValueTreeState& apvts)
{
    addAndMakeVisible (header);

    masterGainKnob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    masterGainKnob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    masterGainKnob.setName ("MASTER");
    addAndMakeVisible (masterGainKnob);

    masterGainAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, ParamIDs::MASTER_GAIN, masterGainKnob);
}

MasterSectionComponent::~MasterSectionComponent()
{
    masterGainAtt.reset();
}

void MasterSectionComponent::setTooltips()
{
    masterGainKnob.setTooltip (juce::String (Tooltips::forParamId (ParamIDs::MASTER_GAIN)));
}

void MasterSectionComponent::paint (juce::Graphics& g)
{
    g.setColour (V0idColors::surface);
    g.fillRect (getLocalBounds());
}

void MasterSectionComponent::resized()
{
    auto bounds = getLocalBounds();
    header.setBounds (bounds.removeFromTop (16));

    // Knob fills the remaining space centered
    const int sz = juce::jmin (bounds.getWidth(), bounds.getHeight());
    masterGainKnob.setBounds (
        bounds.getX() + (bounds.getWidth()  - sz) / 2,
        bounds.getY() + (bounds.getHeight() - sz) / 2,
        sz, sz);
}

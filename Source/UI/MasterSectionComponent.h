#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "SectionLabel.h"

class MasterSectionComponent : public juce::Component
{
public:
    explicit MasterSectionComponent (juce::AudioProcessorValueTreeState& apvts);
    ~MasterSectionComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void setTooltips();

private:
    SectionLabel header { "OUT" };
    juce::Slider masterGainKnob;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> masterGainAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MasterSectionComponent)
};

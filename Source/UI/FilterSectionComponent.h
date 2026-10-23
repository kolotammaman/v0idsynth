#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "SectionLabel.h"

class FilterSectionComponent : public juce::Component
{
public:
    explicit FilterSectionComponent (juce::AudioProcessorValueTreeState& apvts);
    ~FilterSectionComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void setTooltips();

private:
    SectionLabel header { "FILTER" };

    juce::Slider cutoffKnob;
    juce::Slider resonanceKnob;
    juce::Slider modeBlendKnob;
    juce::Slider driveKnob;
    juce::Slider envAmountKnob;
    juce::Slider envAttackKnob;
    juce::Slider envDecayKnob;
    juce::Slider envSustainKnob;
    juce::Slider envReleaseKnob;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> cutoffAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> resonanceAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> modeBlendAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> driveAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> envAmountAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> envAttackAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> envDecayAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> envSustainAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> envReleaseAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FilterSectionComponent)
};

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "SectionLabel.h"

class VoiceAmpEnvSectionComponent : public juce::Component
{
public:
    explicit VoiceAmpEnvSectionComponent (juce::AudioProcessorValueTreeState& apvts);
    ~VoiceAmpEnvSectionComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void setTooltips();

private:
    SectionLabel header { "VOICE / AMP" };

    juce::ComboBox modeCombo;
    juce::Slider   glideKnob;
    juce::Slider   unisonVoicesKnob;
    juce::Slider   unisonDetuneKnob;
    juce::Slider   unisonSpreadKnob;
    juce::Slider   driftKnob;
    juce::Slider   ampAttackKnob;
    juce::Slider   ampDecayKnob;
    juce::Slider   ampSustainKnob;
    juce::Slider   ampReleaseKnob;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   glideAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   unisonVoicesAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   unisonDetuneAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   unisonSpreadAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   driftAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   ampAttackAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   ampDecayAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   ampSustainAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   ampReleaseAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoiceAmpEnvSectionComponent)
};

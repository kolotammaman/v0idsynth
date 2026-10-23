#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "SectionLabel.h"

class ModSourceColumnComponent : public juce::Component
{
public:
    enum class Kind { LFO, Env };

    // index is 1..4 for LFO1..LFO4 or Env1..Env4.
    // onRecompile is called after the user changes the LFO destination routing.
    ModSourceColumnComponent (juce::AudioProcessorValueTreeState& apvts, Kind kind, int index,
                               std::function<void()> onRecompile = {});
    ~ModSourceColumnComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void setTooltips();

    // Reads the current ModMatrix state and updates the dest combo selection.
    // Call this after preset load to reflect the loaded routing.
    void refreshDestCombo();

private:
    juce::AudioProcessorValueTreeState& apvtsRef;
    std::function<void()>               recompileCallback;

    Kind kind;
    int  index;
    SectionLabel header;

    // LFO controls (used when kind == LFO)
    juce::Slider   lfoRateKnob, lfoDepthKnob;
    juce::ComboBox lfoWaveformCombo;
    juce::ComboBox destCombo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   lfoRateAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   lfoDepthAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> lfoWaveformAtt;

    // Env controls (used when kind == Env)
    juce::Slider envAttackKnob, envDecayKnob, envSustainKnob, envReleaseKnob, envDepthKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> envAttackAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> envDecayAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> envSustainAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> envReleaseAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> envDepthAtt;

    // Writes the selected destination into the ModMatrix ValueTree child and triggers recompile.
    void onDestChanged();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModSourceColumnComponent)
};

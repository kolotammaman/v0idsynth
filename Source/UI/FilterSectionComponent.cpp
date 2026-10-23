#include "FilterSectionComponent.h"
#include "V0idSynthLookAndFeel.h"
#include "Tooltips.h"
#include "../Parameters.h"

static void initKnob (juce::Slider& s, const char* name)
{
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    s.setName (name);
}

FilterSectionComponent::FilterSectionComponent (juce::AudioProcessorValueTreeState& apvts)
{
    addAndMakeVisible (header);

    initKnob (cutoffKnob,     "CUTOFF");   addAndMakeVisible (cutoffKnob);
    initKnob (resonanceKnob,  "RES");      addAndMakeVisible (resonanceKnob);
    initKnob (modeBlendKnob,  "MODE");     addAndMakeVisible (modeBlendKnob);
    initKnob (driveKnob,      "DRIVE");    addAndMakeVisible (driveKnob);
    initKnob (envAmountKnob,  "ENV AMT");  addAndMakeVisible (envAmountKnob);
    envAmountKnob.getProperties().set ("bipolar", true);
    initKnob (envAttackKnob,  "ATTACK");   addAndMakeVisible (envAttackKnob);
    initKnob (envDecayKnob,   "DECAY");    addAndMakeVisible (envDecayKnob);
    initKnob (envSustainKnob, "SUSTAIN");  addAndMakeVisible (envSustainKnob);
    initKnob (envReleaseKnob, "RELEASE");  addAndMakeVisible (envReleaseKnob);

    cutoffAtt     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, ParamIDs::FILTER_CUTOFF,      cutoffKnob);
    resonanceAtt  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, ParamIDs::FILTER_RESONANCE,   resonanceKnob);
    modeBlendAtt  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, ParamIDs::FILTER_MODE_BLEND,  modeBlendKnob);
    driveAtt      = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, ParamIDs::FILTER_DRIVE,       driveKnob);
    envAmountAtt  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, ParamIDs::FILTER_ENV_AMOUNT,  envAmountKnob);
    envAttackAtt  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, ParamIDs::FILTER_ENV_ATTACK,  envAttackKnob);
    envDecayAtt   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, ParamIDs::FILTER_ENV_DECAY,   envDecayKnob);
    envSustainAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, ParamIDs::FILTER_ENV_SUSTAIN, envSustainKnob);
    envReleaseAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, ParamIDs::FILTER_ENV_RELEASE, envReleaseKnob);
}

FilterSectionComponent::~FilterSectionComponent()
{
    cutoffAtt.reset();
    resonanceAtt.reset();
    modeBlendAtt.reset();
    driveAtt.reset();
    envAmountAtt.reset();
    envAttackAtt.reset();
    envDecayAtt.reset();
    envSustainAtt.reset();
    envReleaseAtt.reset();
}

void FilterSectionComponent::setTooltips()
{
    cutoffKnob    .setTooltip (juce::String (Tooltips::forParamId (ParamIDs::FILTER_CUTOFF)));
    resonanceKnob .setTooltip (juce::String (Tooltips::forParamId (ParamIDs::FILTER_RESONANCE)));
    modeBlendKnob .setTooltip (juce::String (Tooltips::forParamId (ParamIDs::FILTER_MODE_BLEND)));
    driveKnob     .setTooltip (juce::String (Tooltips::forParamId (ParamIDs::FILTER_DRIVE)));
    envAmountKnob .setTooltip (juce::String (Tooltips::forParamId (ParamIDs::FILTER_ENV_AMOUNT)));
    envAttackKnob .setTooltip (juce::String (Tooltips::forParamId (ParamIDs::FILTER_ENV_ATTACK)));
    envDecayKnob  .setTooltip (juce::String (Tooltips::forParamId (ParamIDs::FILTER_ENV_DECAY)));
    envSustainKnob.setTooltip (juce::String (Tooltips::forParamId (ParamIDs::FILTER_ENV_SUSTAIN)));
    envReleaseKnob.setTooltip (juce::String (Tooltips::forParamId (ParamIDs::FILTER_ENV_RELEASE)));
}

void FilterSectionComponent::paint (juce::Graphics& g)
{
    g.setColour (V0idColors::surface);
    g.fillRect (getLocalBounds());
}

void FilterSectionComponent::resized()
{
    auto bounds = getLocalBounds();
    header.setBounds (bounds.removeFromTop (16));

    const int totalW  = bounds.getWidth();
    const int halfH   = bounds.getHeight() / 2;
    const int row1Y   = bounds.getY();
    const int row2Y   = row1Y + halfH;

    // Row 1: 5 knobs across the width
    const int knob5W  = totalW / 5;
    cutoffKnob.setBounds    (bounds.getX() + 0 * knob5W, row1Y, knob5W, halfH);
    resonanceKnob.setBounds (bounds.getX() + 1 * knob5W, row1Y, knob5W, halfH);
    modeBlendKnob.setBounds (bounds.getX() + 2 * knob5W, row1Y, knob5W, halfH);
    driveKnob.setBounds     (bounds.getX() + 3 * knob5W, row1Y, knob5W, halfH);
    envAmountKnob.setBounds (bounds.getX() + 4 * knob5W, row1Y, knob5W, halfH);

    // Row 2: 4 ADSR knobs across the width
    const int knob4W  = totalW / 4;
    envAttackKnob.setBounds  (bounds.getX() + 0 * knob4W, row2Y, knob4W, halfH);
    envDecayKnob.setBounds   (bounds.getX() + 1 * knob4W, row2Y, knob4W, halfH);
    envSustainKnob.setBounds (bounds.getX() + 2 * knob4W, row2Y, knob4W, halfH);
    envReleaseKnob.setBounds (bounds.getX() + 3 * knob4W, row2Y, knob4W, halfH);
}

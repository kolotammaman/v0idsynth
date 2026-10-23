#include "VoiceAmpEnvSectionComponent.h"
#include "V0idSynthLookAndFeel.h"
#include "Tooltips.h"
#include "../Parameters.h"

static void initKnob (juce::Slider& s, const char* name)
{
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    s.setName (name);
}

VoiceAmpEnvSectionComponent::VoiceAmpEnvSectionComponent (juce::AudioProcessorValueTreeState& apvts)
{
    addAndMakeVisible (header);

    modeCombo.setName ("MODE");
    addAndMakeVisible (modeCombo);

    initKnob (glideKnob,         "GLIDE");   addAndMakeVisible (glideKnob);
    initKnob (unisonVoicesKnob,  "VOICES");  addAndMakeVisible (unisonVoicesKnob);
    initKnob (unisonDetuneKnob,  "DETUNE");  addAndMakeVisible (unisonDetuneKnob);
    initKnob (unisonSpreadKnob,  "SPREAD");  addAndMakeVisible (unisonSpreadKnob);
    initKnob (driftKnob,         "DRIFT");   addAndMakeVisible (driftKnob);
    initKnob (ampAttackKnob,     "ATTACK");  addAndMakeVisible (ampAttackKnob);
    initKnob (ampDecayKnob,      "DECAY");   addAndMakeVisible (ampDecayKnob);
    initKnob (ampSustainKnob,    "SUSTAIN"); addAndMakeVisible (ampSustainKnob);
    initKnob (ampReleaseKnob,    "RELEASE"); addAndMakeVisible (ampReleaseKnob);

    // Items MUST be added before ComboBoxAttachment. Order matches AudioParameterChoice: Poly/Mono/Unison.
    modeCombo.addItem ("Poly",   1);
    modeCombo.addItem ("Mono",   2);
    modeCombo.addItem ("Unison", 3);

    modeAtt         = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        apvts, ParamIDs::VOICE_MODE,          modeCombo);
    glideAtt        = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, ParamIDs::VOICE_GLIDE,         glideKnob);
    unisonVoicesAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, ParamIDs::VOICE_UNISON_VOICES, unisonVoicesKnob);
    unisonDetuneAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, ParamIDs::VOICE_UNISON_DETUNE, unisonDetuneKnob);
    unisonSpreadAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, ParamIDs::VOICE_UNISON_SPREAD, unisonSpreadKnob);
    driftAtt        = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, ParamIDs::VOICE_DRIFT,         driftKnob);
    ampAttackAtt    = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, ParamIDs::AMP_ENV_ATTACK,      ampAttackKnob);
    ampDecayAtt     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, ParamIDs::AMP_ENV_DECAY,       ampDecayKnob);
    ampSustainAtt   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, ParamIDs::AMP_ENV_SUSTAIN,     ampSustainKnob);
    ampReleaseAtt   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, ParamIDs::AMP_ENV_RELEASE,     ampReleaseKnob);
}

VoiceAmpEnvSectionComponent::~VoiceAmpEnvSectionComponent()
{
    modeAtt.reset();
    glideAtt.reset();
    unisonVoicesAtt.reset();
    unisonDetuneAtt.reset();
    unisonSpreadAtt.reset();
    driftAtt.reset();
    ampAttackAtt.reset();
    ampDecayAtt.reset();
    ampSustainAtt.reset();
    ampReleaseAtt.reset();
}

void VoiceAmpEnvSectionComponent::setTooltips()
{
    modeCombo       .setTooltip (juce::String (Tooltips::forParamId (ParamIDs::VOICE_MODE)));
    glideKnob       .setTooltip (juce::String (Tooltips::forParamId (ParamIDs::VOICE_GLIDE)));
    unisonVoicesKnob.setTooltip (juce::String (Tooltips::forParamId (ParamIDs::VOICE_UNISON_VOICES)));
    unisonDetuneKnob.setTooltip (juce::String (Tooltips::forParamId (ParamIDs::VOICE_UNISON_DETUNE)));
    unisonSpreadKnob.setTooltip (juce::String (Tooltips::forParamId (ParamIDs::VOICE_UNISON_SPREAD)));
    driftKnob       .setTooltip (juce::String (Tooltips::forParamId (ParamIDs::VOICE_DRIFT)));
    ampAttackKnob   .setTooltip (juce::String (Tooltips::forParamId (ParamIDs::AMP_ENV_ATTACK)));
    ampDecayKnob    .setTooltip (juce::String (Tooltips::forParamId (ParamIDs::AMP_ENV_DECAY)));
    ampSustainKnob  .setTooltip (juce::String (Tooltips::forParamId (ParamIDs::AMP_ENV_SUSTAIN)));
    ampReleaseKnob  .setTooltip (juce::String (Tooltips::forParamId (ParamIDs::AMP_ENV_RELEASE)));
}

void VoiceAmpEnvSectionComponent::paint (juce::Graphics& g)
{
    g.setColour (V0idColors::surface);
    g.fillRect (getLocalBounds());
}

void VoiceAmpEnvSectionComponent::resized()
{
    auto bounds = getLocalBounds();
    header.setBounds (bounds.removeFromTop (16));

    const int totalW = bounds.getWidth();
    const int halfH  = bounds.getHeight() / 2;
    const int row1Y  = bounds.getY();
    const int row2Y  = row1Y + halfH;
    const int x0     = bounds.getX();

    // Row 1: mode combo + 5 knobs (6 controls across width)
    const int ctrl6W = totalW / 6;
    modeCombo.setBounds       (x0 + 0 * ctrl6W, row1Y, ctrl6W, halfH);
    glideKnob.setBounds       (x0 + 1 * ctrl6W, row1Y, ctrl6W, halfH);
    unisonVoicesKnob.setBounds(x0 + 2 * ctrl6W, row1Y, ctrl6W, halfH);
    unisonDetuneKnob.setBounds(x0 + 3 * ctrl6W, row1Y, ctrl6W, halfH);
    unisonSpreadKnob.setBounds(x0 + 4 * ctrl6W, row1Y, ctrl6W, halfH);
    driftKnob.setBounds       (x0 + 5 * ctrl6W, row1Y, ctrl6W, halfH);

    // Row 2: 4 amp ADSR knobs
    const int knob4W = totalW / 4;
    ampAttackKnob.setBounds  (x0 + 0 * knob4W, row2Y, knob4W, halfH);
    ampDecayKnob.setBounds   (x0 + 1 * knob4W, row2Y, knob4W, halfH);
    ampSustainKnob.setBounds (x0 + 2 * knob4W, row2Y, knob4W, halfH);
    ampReleaseKnob.setBounds (x0 + 3 * knob4W, row2Y, knob4W, halfH);
}

#include "ModSourceColumnComponent.h"
#include "V0idSynthLookAndFeel.h"
#include "Tooltips.h"
#include "../Parameters.h"

// ---------------------------------------------------------------------------
// Param ID helpers (index 1..4)
// ---------------------------------------------------------------------------

static const char* lfoRateParamId (int i)
{
    switch (i) {
        case 1: return ParamIDs::MOD_LFO1_RATE;
        case 2: return ParamIDs::MOD_LFO2_RATE;
        case 3: return ParamIDs::MOD_LFO3_RATE;
        case 4: return ParamIDs::MOD_LFO4_RATE;
        default: return ParamIDs::MOD_LFO1_RATE;
    }
}

static const char* lfoWaveformParamId (int i)
{
    switch (i) {
        case 1: return ParamIDs::MOD_LFO1_WAVEFORM;
        case 2: return ParamIDs::MOD_LFO2_WAVEFORM;
        case 3: return ParamIDs::MOD_LFO3_WAVEFORM;
        case 4: return ParamIDs::MOD_LFO4_WAVEFORM;
        default: return ParamIDs::MOD_LFO1_WAVEFORM;
    }
}

static const char* lfoDepthParamId (int i)
{
    switch (i) {
        case 1: return ParamIDs::MOD_LFO1_DEPTH;
        case 2: return ParamIDs::MOD_LFO2_DEPTH;
        case 3: return ParamIDs::MOD_LFO3_DEPTH;
        case 4: return ParamIDs::MOD_LFO4_DEPTH;
        default: return ParamIDs::MOD_LFO1_DEPTH;
    }
}

static const char* envAttackParamId (int i)
{
    switch (i) {
        case 1: return ParamIDs::MOD_ENV1_ATTACK;
        case 2: return ParamIDs::MOD_ENV2_ATTACK;
        case 3: return ParamIDs::MOD_ENV3_ATTACK;
        case 4: return ParamIDs::MOD_ENV4_ATTACK;
        default: return ParamIDs::MOD_ENV1_ATTACK;
    }
}

static const char* envDecayParamId (int i)
{
    switch (i) {
        case 1: return ParamIDs::MOD_ENV1_DECAY;
        case 2: return ParamIDs::MOD_ENV2_DECAY;
        case 3: return ParamIDs::MOD_ENV3_DECAY;
        case 4: return ParamIDs::MOD_ENV4_DECAY;
        default: return ParamIDs::MOD_ENV1_DECAY;
    }
}

static const char* envSustainParamId (int i)
{
    switch (i) {
        case 1: return ParamIDs::MOD_ENV1_SUSTAIN;
        case 2: return ParamIDs::MOD_ENV2_SUSTAIN;
        case 3: return ParamIDs::MOD_ENV3_SUSTAIN;
        case 4: return ParamIDs::MOD_ENV4_SUSTAIN;
        default: return ParamIDs::MOD_ENV1_SUSTAIN;
    }
}

static const char* envReleaseParamId (int i)
{
    switch (i) {
        case 1: return ParamIDs::MOD_ENV1_RELEASE;
        case 2: return ParamIDs::MOD_ENV2_RELEASE;
        case 3: return ParamIDs::MOD_ENV3_RELEASE;
        case 4: return ParamIDs::MOD_ENV4_RELEASE;
        default: return ParamIDs::MOD_ENV1_RELEASE;
    }
}

static const char* envDepthParamId (int i)
{
    switch (i) {
        case 1: return ParamIDs::MOD_ENV1_DEPTH;
        case 2: return ParamIDs::MOD_ENV2_DEPTH;
        case 3: return ParamIDs::MOD_ENV3_DEPTH;
        case 4: return ParamIDs::MOD_ENV4_DEPTH;
        default: return ParamIDs::MOD_ENV1_DEPTH;
    }
}

// ---------------------------------------------------------------------------

static void initKnob (juce::Slider& s, const char* name)
{
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    s.setName (name);
}

ModSourceColumnComponent::ModSourceColumnComponent (juce::AudioProcessorValueTreeState& apvts,
                                                    Kind k, int idx,
                                                    std::function<void()> onRecompile)
    : apvtsRef (apvts), recompileCallback (std::move (onRecompile)),
      kind (k), index (idx),
      header (k == Kind::LFO ? "LFO" + juce::String (idx) : "ENV" + juce::String (idx))
{
    addAndMakeVisible (header);

    if (kind == Kind::LFO)
    {
        initKnob (lfoRateKnob,  "RATE");   addAndMakeVisible (lfoRateKnob);
        lfoWaveformCombo.setName ("WAVE"); addAndMakeVisible (lfoWaveformCombo);
        initKnob (lfoDepthKnob, "DEPTH");  addAndMakeVisible (lfoDepthKnob);

        // Items MUST be added before ComboBoxAttachment is created.
        // JUCE ComboBoxAttachment does not auto-populate from AudioParameterChoice.
        // Order matches the choice strings: Sine/Triangle/Saw/Square/S&H.
        lfoWaveformCombo.addItem ("Sine",     1);
        lfoWaveformCombo.addItem ("Triangle", 2);
        lfoWaveformCombo.addItem ("Saw",      3);
        lfoWaveformCombo.addItem ("Square",   4);
        lfoWaveformCombo.addItem ("S&H",      5);

        lfoRateAtt     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            apvts, lfoRateParamId (index),     lfoRateKnob);
        lfoWaveformAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
            apvts, lfoWaveformParamId (index), lfoWaveformCombo);
        lfoDepthAtt    = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            apvts, lfoDepthParamId (index),    lfoDepthKnob);

        // Destination routing combo.
        // itemId 1="–" (none), 2=CUTOFF(dest 0), 3=RESON(dest 1),
        //               4=PITCH(dest 2), 5=MIX(dest 3), 6=AMP(dest 4)
        destCombo.addItem (juce::String (juce::CharPointer_UTF8 ("\xe2\x80\x93")), 1); // "–" (en-dash)
        destCombo.addItem ("CUTOFF",       2);
        destCombo.addItem ("RESON",        3);
        destCombo.addItem ("PITCH",        4);
        destCombo.addItem ("MIX",          5);
        destCombo.addItem ("AMP",          6);
        destCombo.setName ("DEST");
        addAndMakeVisible (destCombo);
        destCombo.onChange = [this]() { onDestChanged(); };
        refreshDestCombo();
    }
    else // Kind::Env
    {
        initKnob (envAttackKnob,  "A"); addAndMakeVisible (envAttackKnob);
        initKnob (envDecayKnob,   "D"); addAndMakeVisible (envDecayKnob);
        initKnob (envSustainKnob, "S"); addAndMakeVisible (envSustainKnob);
        initKnob (envReleaseKnob, "R"); addAndMakeVisible (envReleaseKnob);
        initKnob (envDepthKnob, "DEPTH");  addAndMakeVisible (envDepthKnob);
        envDepthKnob.getProperties().set ("bipolar", true); // arc grows from 12 o'clock
        envDepthKnob.setDoubleClickReturnValue (true, 1.0);

        envAttackAtt  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            apvts, envAttackParamId  (index), envAttackKnob);
        envDecayAtt   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            apvts, envDecayParamId   (index), envDecayKnob);
        envSustainAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            apvts, envSustainParamId (index), envSustainKnob);
        envReleaseAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            apvts, envReleaseParamId (index), envReleaseKnob);
        envDepthAtt   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            apvts, envDepthParamId   (index), envDepthKnob);

        // Destination routing combo — mirrors LFO dest combo so the user can see and
        // set what parameter this envelope is shaping.
        destCombo.addItem (juce::String (juce::CharPointer_UTF8 ("\xe2\x80\x93")), 1); // "–" (en-dash)
        destCombo.addItem ("CUTOFF",       2);
        destCombo.addItem ("RESON",        3);
        destCombo.addItem ("PITCH",        4);
        destCombo.addItem ("MIX",          5);
        destCombo.addItem ("AMP",          6);
        destCombo.setName ("DEST");
        addAndMakeVisible (destCombo);
        destCombo.onChange = [this]() { onDestChanged(); };
        refreshDestCombo();
    }
}

ModSourceColumnComponent::~ModSourceColumnComponent()
{
    lfoRateAtt.reset();
    lfoWaveformAtt.reset();
    lfoDepthAtt.reset();
    envAttackAtt.reset();
    envDecayAtt.reset();
    envSustainAtt.reset();
    envReleaseAtt.reset();
    envDepthAtt.reset();
}

void ModSourceColumnComponent::refreshDestCombo()
{
    // LFO1..4 → srcIdx 0..3;  Env1..4 → srcIdx 4..7
    const int srcIdx = (kind == Kind::LFO) ? (index - 1) : (index + 3);
    auto matrix   = apvtsRef.state.getChildWithName ("ModMatrix");
    int  foundDest = -1;

    if (matrix.isValid())
    {
        for (int i = 0; i < matrix.getNumChildren(); ++i)
        {
            auto child = matrix.getChild (i);
            if ((int) child.getProperty ("source", -1) == srcIdx)
            {
                foundDest = (int) child.getProperty ("dest", -1);
                break;
            }
        }
    }

    // itemId = dest + 2 (because itemId 1 is "–")
    const int itemId = (foundDest >= 0 && foundDest < 5) ? foundDest + 2 : 1;
    destCombo.setSelectedId (itemId, juce::dontSendNotification);
}

void ModSourceColumnComponent::onDestChanged()
{
    // LFO1..4 → srcIdx 0..3;  Env1..4 → srcIdx 4..7
    const int srcIdx = (kind == Kind::LFO) ? (index - 1) : (index + 3);
    auto      matrix = apvtsRef.state.getOrCreateChildWithName ("ModMatrix", nullptr);

    // Remove any existing route from this LFO source
    for (int i = matrix.getNumChildren() - 1; i >= 0; --i)
    {
        auto child = matrix.getChild (i);
        if ((int) child.getProperty ("source", -1) == srcIdx)
            matrix.removeChild (child, nullptr);
    }

    const int selectedId = destCombo.getSelectedId();
    if (selectedId > 1) // Not "–"
    {
        const int destIdx = selectedId - 2; // 2→0(CUTOFF) … 6→4(AMP)
        juce::ValueTree route ("Route");
        route.setProperty ("source", srcIdx,  nullptr);
        route.setProperty ("dest",   destIdx, nullptr);
        route.setProperty ("depth",  1.0f,    nullptr);
        matrix.appendChild (route, nullptr);
    }

    if (recompileCallback)
        recompileCallback();
}

void ModSourceColumnComponent::setTooltips()
{
    if (kind == Kind::LFO)
    {
        lfoRateKnob    .setTooltip (juce::String (Tooltips::forParamId (lfoRateParamId     (index))));
        lfoWaveformCombo.setTooltip(juce::String (Tooltips::forParamId (lfoWaveformParamId (index))));
        lfoDepthKnob   .setTooltip (juce::String (Tooltips::forParamId (lfoDepthParamId    (index))));
    }
    else // Kind::Env
    {
        envAttackKnob .setTooltip (juce::String (Tooltips::forParamId (envAttackParamId  (index))));
        envDecayKnob  .setTooltip (juce::String (Tooltips::forParamId (envDecayParamId   (index))));
        envSustainKnob.setTooltip (juce::String (Tooltips::forParamId (envSustainParamId (index))));
        envReleaseKnob.setTooltip (juce::String (Tooltips::forParamId (envReleaseParamId (index))));
        envDepthKnob  .setTooltip (juce::String (Tooltips::forParamId (envDepthParamId   (index))));
    }
}

void ModSourceColumnComponent::paint (juce::Graphics& g)
{
    g.setColour (V0idColors::surface);
    g.fillRect (getLocalBounds());
}

void ModSourceColumnComponent::resized()
{
    auto bounds = getLocalBounds();
    header.setBounds (bounds.removeFromTop (16));

    const int totalH = bounds.getHeight();
    const int x      = bounds.getX();
    const int w      = bounds.getWidth();

    if (kind == Kind::LFO)
    {
        // 4 controls stacked: rate knob (109px), wave combo (24px),
        //                     dest combo (24px), depth knob (remaining)
        constexpr int comboH = 24;
        constexpr int rateH  = 109;
        const int     depthH = totalH - rateH - comboH - comboH;

        lfoRateKnob.setBounds      (x, bounds.getY(),                         w, rateH);
        lfoWaveformCombo.setBounds (x, bounds.getY() + rateH,                 w, comboH);
        destCombo.setBounds        (x, bounds.getY() + rateH + comboH,        w, comboH);
        lfoDepthKnob.setBounds     (x, bounds.getY() + rateH + comboH * 2,    w, juce::jmax (depthH, 1));
    }
    else // Kind::Env
    {
        // Mirrors the LFO column row-for-row so the strip reads as one grid:
        //   A/D/S/R 2x2 grid  (= LFO rate knob + wave combo, 133px)
        //   DEST combo        (same y as the LFO dest combo)
        //   DEPTH knob        (same y and size as the LFO depth knob)
        constexpr int comboH = 24;
        constexpr int gridH  = 109 + comboH;
        const int     y0     = bounds.getY();
        const int     cellW  = w / 2;
        const int     cellH  = gridH / 2;
        envAttackKnob.setBounds  (x,         y0,         cellW,     cellH);
        envDecayKnob.setBounds   (x + cellW, y0,         w - cellW, cellH);
        envSustainKnob.setBounds (x,         y0 + cellH, cellW,     gridH - cellH);
        envReleaseKnob.setBounds (x + cellW, y0 + cellH, w - cellW, gridH - cellH);
        destCombo.setBounds      (x, y0 + gridH, w, comboH);
        envDepthKnob.setBounds   (x, y0 + gridH + comboH, w, juce::jmax (totalH - gridH - comboH, 1));
    }
}

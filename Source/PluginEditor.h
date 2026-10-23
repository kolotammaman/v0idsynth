#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "Preset/PresetBrowserComponent.h"
#include "UI/V0idSynthLookAndFeel.h"
#include "UI/OscillatorSectionComponent.h"
#include "UI/FilterSectionComponent.h"
#include "UI/VoiceAmpEnvSectionComponent.h"
#include "UI/MasterSectionComponent.h"
#include "UI/ModSourceColumnComponent.h"
#include "UI/ModRoutingDisplayComponent.h"

// Phase 5 editor: 1000x700 custom canvas with full V0idSynthLookAndFeel applied.
// Row layout (top to bottom):
//   Row 1: PresetBrowserComponent      —  36px
//   Row 2: Synth engine strip          — 220px (Osc | Filter | Voice+Amp | Master)
//   Row 3: ModRoutingDisplayComponent  —  80px
//   Row 4: Modulation sources strip    — 364px (LFO1..4 + Env1..4)
// Total: 36 + 220 + 80 + 364 = 700px.
class V0idSynthAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit V0idSynthAudioProcessorEditor(V0idSynthAudioProcessor&);
    ~V0idSynthAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void paintOverChildren(juce::Graphics&) override;
    void resized() override;

private:
    V0idSynthAudioProcessor& processorRef;

    V0idSynthLookAndFeel lookAndFeel; // applied globally in ctor

    // Static CRT texture (scan lines + vignette), cached — see paintOverChildren
    void renderCrtOverlay();
    juce::Image crtOverlay;
    float crtOverlayScale = 1.0f;

    // Row 1
    PresetBrowserComponent browser;

    // Row 2 — Synth engine strip
    OscillatorSectionComponent  oscSection;
    FilterSectionComponent      filterSection;
    VoiceAmpEnvSectionComponent voiceAmpSection;
    MasterSectionComponent      masterSection;

    // Row 3 — Routing display (UI-02)
    ModRoutingDisplayComponent  routingDisplay;

    // Row 4 — 8 mod source columns
    ModSourceColumnComponent lfo1Column;
    ModSourceColumnComponent lfo2Column;
    ModSourceColumnComponent lfo3Column;
    ModSourceColumnComponent lfo4Column;
    ModSourceColumnComponent env1Column;
    ModSourceColumnComponent env2Column;
    ModSourceColumnComponent env3Column;
    ModSourceColumnComponent env4Column;

    // Tooltip plumbing (UI-03) — TooltipWindow is a child component; JUCE auto-routes hover events.
    juce::TooltipWindow tooltipWindow { this, 700 }; // 700ms hover delay

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(V0idSynthAudioProcessorEditor)
};

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

class ModRoutingDisplayComponent : public juce::Component,
                                   private juce::ValueTree::Listener
{
public:
    explicit ModRoutingDisplayComponent (juce::AudioProcessorValueTreeState& apvts);
    ~ModRoutingDisplayComponent() override;

    void paint (juce::Graphics&) override;

private:
    // ValueTree::Listener overrides
    // The listener sits on the whole APVTS tree, which also carries every
    // parameter's "value" property — repaint only for ModMatrix changes, not on
    // each automation tick.
    static bool isMatrixTree (const juce::ValueTree& t)
    {
        return t.hasType ("ModMatrix") || t.getParent().hasType ("ModMatrix");
    }
    void valueTreePropertyChanged   (juce::ValueTree& t, const juce::Identifier&) override { if (isMatrixTree (t)) repaint(); }
    void valueTreeChildAdded        (juce::ValueTree& p, juce::ValueTree&)        override { if (isMatrixTree (p)) repaint(); }
    void valueTreeChildRemoved      (juce::ValueTree& p, juce::ValueTree&, int)   override { if (isMatrixTree (p)) repaint(); }
    void valueTreeChildOrderChanged (juce::ValueTree& p, int, int)                override { if (isMatrixTree (p)) repaint(); }
    void valueTreeParentChanged     (juce::ValueTree&)                            override {}
    // Fires when APVTS::replaceState() swaps the whole tree (preset load) — the
    // child add/remove callbacks above do NOT fire in that case.
    void valueTreeRedirected        (juce::ValueTree&)                          override { repaint(); }

    void resized() override { titleCache = {}; }
    void renderTitle (int width, int height);

    juce::AudioProcessorValueTreeState& apvtsRef;
    juce::Image titleCache; // phosphor-glow wordmark, rendered once per size
    float titleCacheScale = 1.0f;

    static constexpr int kNumSources = 9; // LFO1..4, Env1..4, Drift
    static constexpr int kNumDests   = 5; // FilterCutoff, FilterResonance, OscPitch, OscMix, AmpGain

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModRoutingDisplayComponent)
};

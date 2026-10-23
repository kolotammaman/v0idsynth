#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "PresetManager.h"
#include "../UI/V0idSynthLookAndFeel.h"

class V0idSynthAudioProcessor; // fwd decl

class PresetBrowserComponent : public juce::Component,
                                private juce::Button::Listener
{
public:
    explicit PresetBrowserComponent(V0idSynthAudioProcessor& proc);
    ~PresetBrowserComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    // Refresh name button from current PresetManager state.
    // Called by the editor after processor.loadPresetByIndex or on construction.
    void refresh();

    static constexpr int kPreferredHeight = 36;

private:
    void buttonClicked(juce::Button*) override;
    void showPresetMenu();

    V0idSynthAudioProcessor& processor;

    juce::TextButton prevButton  { "<" };
    juce::TextButton nextButton  { ">" };
    juce::TextButton nameButton;   // clickable — opens hierarchical preset popup

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetBrowserComponent)
};

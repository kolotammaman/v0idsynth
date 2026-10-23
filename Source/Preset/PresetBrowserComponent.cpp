#include "PresetBrowserComponent.h"
#include "../PluginProcessor.h"

PresetBrowserComponent::PresetBrowserComponent(V0idSynthAudioProcessor& proc)
    : processor(proc)
{
    addAndMakeVisible(prevButton);
    addAndMakeVisible(nextButton);
    addAndMakeVisible(nameButton);

    prevButton.addListener(this);
    nextButton.addListener(this);

    nameButton.setColour(juce::TextButton::textColourOffId,  V0idColors::accent);
    nameButton.setColour(juce::TextButton::textColourOnId,   V0idColors::accent);
    nameButton.setColour(juce::TextButton::buttonColourId,   juce::Colours::transparentBlack);
    nameButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);
    nameButton.onClick = [this] { showPresetMenu(); };

    refresh();
}

PresetBrowserComponent::~PresetBrowserComponent() = default;

void PresetBrowserComponent::paint(juce::Graphics& g)
{
    g.fillAll(V0idColors::background);
    g.setColour(V0idColors::accentDivider());
    g.fillRect(0, getHeight() - 1, getWidth(), 1);
}

void PresetBrowserComponent::resized()
{
    auto bounds = getLocalBounds().reduced(4);
    const int buttonWidth = 30;
    prevButton.setBounds(bounds.removeFromLeft(buttonWidth));
    bounds.removeFromLeft(4);
    nextButton.setBounds(bounds.removeFromRight(buttonWidth));
    bounds.removeFromRight(4);
    nameButton.setBounds(bounds);
}

void PresetBrowserComponent::refresh()
{
    auto& mgr = processor.getPresetManager();
    int idx = mgr.getCurrentIndex();
    if (idx >= 0 && idx < mgr.getNumPresets())
        nameButton.setButtonText(mgr.getPresetInfo(idx).name);
    else
        nameButton.setButtonText("-- No preset --");
}

void PresetBrowserComponent::buttonClicked(juce::Button* b)
{
    auto& mgr = processor.getPresetManager();
    int newIdx = -1;
    if (b == &prevButton)
        newIdx = mgr.previousPreset();
    else if (b == &nextButton)
        newIdx = mgr.nextPreset();

    if (newIdx >= 0)
    {
        processor.loadPresetByIndex(newIdx);
        refresh();
    }
}

void PresetBrowserComponent::showPresetMenu()
{
    auto& mgr = processor.getPresetManager();
    const int currentIdx = mgr.getCurrentIndex();
    const int total = mgr.getNumPresets();

    // One submenu per category. Item ID = preset index + 1 (0 = dismissed).
    static const PresetCategory categoryOrder[] = {
        PresetCategory::Pad, PresetCategory::Bass, PresetCategory::Lead,
        PresetCategory::FX,  PresetCategory::Organic, PresetCategory::Sample
    };

    juce::PopupMenu root;

    for (auto cat : categoryOrder)
    {
        juce::PopupMenu sub;
        bool hasItems = false;

        for (int i = 0; i < total; ++i)
        {
            const auto& info = mgr.getPresetInfo(i);
            if (info.category != cat) continue;
            sub.addItem(i + 1, info.name, true, i == currentIdx);
            hasItems = true;
        }

        if (hasItems)
            root.addSubMenu(PresetManager::categoryToDisplayString(cat), sub);
    }

    root.showMenuAsync(
        juce::PopupMenu::Options()
            .withTargetComponent(&nameButton)
            .withMinimumWidth(nameButton.getWidth()),
        [safeThis = juce::Component::SafePointer<PresetBrowserComponent>(this)](int result)
        {
            // The editor can close while the menu is open — don't touch a dead component.
            if (safeThis == nullptr || result <= 0) return;
            safeThis->processor.loadPresetByIndex(result - 1);
            safeThis->refresh();
        });
}

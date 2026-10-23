#pragma once
#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <memory>

// Category enum — string mapping defined in PresetManager.cpp.
// Order locked: index = ComboBox display order (D-05 right-side filter).
// "All" is the unfiltered sentinel and is always the first ComboBox item.
enum class PresetCategory : int
{
    All     = 0,
    Pad     = 1,
    Bass    = 2,
    Lead    = 3,
    FX      = 4,
    Organic = 5,
    Sample  = 6
};

struct PresetInfo
{
    juce::String     name;        // e.g. "Fluorescent Hum"
    PresetCategory   category;    // parsed from <metadata category="..."/>
    juce::StringArray tags;       // parsed from <metadata tags="a,b,c"/>; may be empty
};

class PresetManager
{
public:
    PresetManager() = default;
    ~PresetManager() = default;

    // ---- Loading ----
    // Each input string is the FULL <V0idPreset>...</V0idPreset> XML for one preset.
    // Invalid/unparseable entries are silently skipped (logged via DBG).
    // Loading clears previous state and resets the current index to -1 (no selection).
    void loadFromXmlStrings(const juce::StringArray& presetXmls);

    // ---- Inspection ----
    int  getNumPresets() const noexcept;                   // total presets ignoring filter
    int  getCurrentIndex() const noexcept;                 // -1 if no current selection
    const PresetInfo& getPresetInfo(int index) const;      // bounds-checked via jassert; UB on OOB
    juce::StringArray getAllPresetNames() const;           // names in load order
    juce::StringArray getPresetNamesForCurrentFilter() const; // names matching current filter

    // ---- State access for processor ----
    // Returns an OWNED XmlElement for the inner <v0idSynthParams> tree of presets[index].
    // Returns nullptr if index is out of range or the preset has no APVTS state child.
    std::unique_ptr<juce::XmlElement> getApvtsStateXmlForIndex(int index) const;

    // ---- Navigation (respects current category filter) ----
    // Returns the new index, or -1 if filter yields zero presets.
    int nextPreset();
    int previousPreset();
    bool setCurrentIndex(int index);     // false if index is OOR or filtered out
    bool selectByName(const juce::String& name); // false if not found

    // ---- Filtering ----
    void setCategoryFilter(PresetCategory cat) noexcept;
    PresetCategory getCategoryFilter() const noexcept;

    // ---- Static helpers exposed for tests ----
    static PresetCategory parseCategory(const juce::String& str) noexcept; // case-insensitive
    static juce::String   categoryToDisplayString(PresetCategory cat);     // "All","Pad",...

private:
    struct Entry
    {
        PresetInfo info;
        std::unique_ptr<juce::XmlElement> rawXml; // owns the parsed <V0idPreset>
    };

    std::vector<Entry> entries;
    int currentIndex = -1;
    PresetCategory activeFilter = PresetCategory::All;

    // Returns indices of entries matching activeFilter (or all entries if All).
    std::vector<int> indicesMatchingFilter() const;
    // Returns position of currentIndex within indicesMatchingFilter(); -1 if not present.
    int currentFilteredPos(const std::vector<int>& filtered) const;
};

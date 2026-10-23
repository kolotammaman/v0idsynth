#include "PresetManager.h"
#include <algorithm>

// ============================================================================
// Static helpers
// ============================================================================

PresetCategory PresetManager::parseCategory(const juce::String& str) noexcept
{
    if (str.equalsIgnoreCase("pad"))     return PresetCategory::Pad;
    if (str.equalsIgnoreCase("bass"))    return PresetCategory::Bass;
    if (str.equalsIgnoreCase("lead"))    return PresetCategory::Lead;
    if (str.equalsIgnoreCase("fx"))      return PresetCategory::FX;
    if (str.equalsIgnoreCase("organic")) return PresetCategory::Organic;
    if (str.equalsIgnoreCase("sample"))  return PresetCategory::Sample;
    return PresetCategory::All; // "all", empty, or unrecognised
}

juce::String PresetManager::categoryToDisplayString(PresetCategory cat)
{
    switch (cat)
    {
        case PresetCategory::All:     return "All";
        case PresetCategory::Pad:     return "Pad";
        case PresetCategory::Bass:    return "Bass";
        case PresetCategory::Lead:    return "Lead";
        case PresetCategory::FX:      return "FX";
        case PresetCategory::Organic: return "Organic";
        case PresetCategory::Sample:  return "Sample";
        default:                      return "All";
    }
}

// ============================================================================
// Loading
// ============================================================================

void PresetManager::loadFromXmlStrings(const juce::StringArray& presetXmls)
{
    entries.clear();
    currentIndex = -1;
    // activeFilter intentionally preserved across loads (D-12)

    for (int i = 0; i < presetXmls.size(); ++i)
    {
        auto parsed = juce::XmlDocument::parse(presetXmls[i]);

        if (parsed == nullptr)
        {
            DBG("PresetManager: skipping malformed preset (index=" << i << ") — parse failed");
            continue;
        }

        if (!parsed->hasTagName("V0idPreset"))
        {
            DBG("PresetManager: skipping malformed preset (index=" << i << ") — wrong root tag: "
                << parsed->getTagName());
            continue;
        }

        auto* metadata = parsed->getChildByName("metadata");
        if (metadata == nullptr)
        {
            DBG("PresetManager: skipping preset (index=" << i << ") — missing <metadata> child");
            continue;
        }

        auto* apvtsChild = parsed->getChildByName("v0idSynthParams");
        if (apvtsChild == nullptr)
        {
            DBG("PresetManager: skipping preset (index=" << i << ") — missing <v0idSynthParams> child");
            continue;
        }

        PresetInfo info;
        info.name     = metadata->getStringAttribute("name", "Unnamed");
        info.category = parseCategory(metadata->getStringAttribute("category", "all"));

        // Parse comma-separated tags; trim whitespace from each
        auto rawTags = metadata->getStringAttribute("tags", "");
        if (rawTags.isNotEmpty())
        {
            info.tags = juce::StringArray::fromTokens(rawTags, ",", "");
            for (auto& tag : info.tags)
                tag = tag.trim();
        }

        Entry entry;
        entry.info   = std::move(info);
        entry.rawXml = std::move(parsed);
        entries.push_back(std::move(entry));
    }

    // Sort entries by category so navigation order matches the popup menu grouping.
    static const PresetCategory kCatOrder[] = {
        PresetCategory::Pad, PresetCategory::Bass, PresetCategory::Lead,
        PresetCategory::FX,  PresetCategory::Organic, PresetCategory::Sample,
        PresetCategory::All
    };
    auto catRank = [](PresetCategory c) {
        for (int i = 0; i < 7; ++i)
            if (kCatOrder[i] == c) return i;
        return 6;
    };
    std::stable_sort(entries.begin(), entries.end(), [&catRank](const Entry& a, const Entry& b) {
        return catRank(a.info.category) < catRank(b.info.category);
    });
}

// ============================================================================
// Inspection
// ============================================================================

int PresetManager::getNumPresets() const noexcept
{
    return static_cast<int>(entries.size());
}

int PresetManager::getCurrentIndex() const noexcept
{
    return currentIndex;
}

const PresetInfo& PresetManager::getPresetInfo(int index) const
{
    jassert(index >= 0 && index < static_cast<int>(entries.size()));
    return entries[static_cast<size_t>(index)].info;
}

juce::StringArray PresetManager::getAllPresetNames() const
{
    juce::StringArray names;
    for (const auto& entry : entries)
        names.add(entry.info.name);
    return names;
}

juce::StringArray PresetManager::getPresetNamesForCurrentFilter() const
{
    juce::StringArray names;
    for (int idx : indicesMatchingFilter())
        names.add(entries[static_cast<size_t>(idx)].info.name);
    return names;
}

// ============================================================================
// State access
// ============================================================================

std::unique_ptr<juce::XmlElement> PresetManager::getApvtsStateXmlForIndex(int index) const
{
    if (index < 0 || index >= static_cast<int>(entries.size()))
        return nullptr;

    auto* apvtsChild = entries[static_cast<size_t>(index)].rawXml->getChildByName("v0idSynthParams");
    if (apvtsChild == nullptr)
        return nullptr;

    // Return a deep copy so caller owns the element
    return std::make_unique<juce::XmlElement>(*apvtsChild);
}

// ============================================================================
// Navigation
// ============================================================================

int PresetManager::nextPreset()
{
    auto filtered = indicesMatchingFilter();
    if (filtered.empty())
    {
        currentIndex = -1;
        return -1;
    }

    int pos    = currentFilteredPos(filtered);
    int sz     = static_cast<int>(filtered.size());
    int newPos = (pos == -1) ? 0 : (pos + 1) % sz;
    currentIndex = filtered[static_cast<size_t>(newPos)];
    return currentIndex;
}

int PresetManager::previousPreset()
{
    auto filtered = indicesMatchingFilter();
    if (filtered.empty())
    {
        currentIndex = -1;
        return -1;
    }

    int pos    = currentFilteredPos(filtered);
    int sz2    = static_cast<int>(filtered.size());
    int newPos = (pos == -1) ? sz2 - 1 : (pos - 1 + sz2) % sz2;
    currentIndex = filtered[static_cast<size_t>(newPos)];
    return currentIndex;
}

bool PresetManager::setCurrentIndex(int index)
{
    if (index < 0 || index >= static_cast<int>(entries.size()))
        return false;

    auto filtered = indicesMatchingFilter();
    if (std::find(filtered.begin(), filtered.end(), index) == filtered.end())
        return false;

    currentIndex = index;
    return true;
}

bool PresetManager::selectByName(const juce::String& name)
{
    auto filtered = indicesMatchingFilter();
    for (int idx : filtered)
    {
        if (entries[static_cast<size_t>(idx)].info.name == name)
        {
            currentIndex = idx;
            return true;
        }
    }
    return false;
}

// ============================================================================
// Filtering
// ============================================================================

void PresetManager::setCategoryFilter(PresetCategory cat) noexcept
{
    activeFilter = cat;

    // If the current selection is no longer visible under the new filter, deselect.
    if (currentIndex >= 0)
    {
        auto filtered = indicesMatchingFilter();
        if (std::find(filtered.begin(), filtered.end(), currentIndex) == filtered.end())
            currentIndex = -1;
    }
}

PresetCategory PresetManager::getCategoryFilter() const noexcept
{
    return activeFilter;
}

// ============================================================================
// Private helpers
// ============================================================================

std::vector<int> PresetManager::indicesMatchingFilter() const
{
    std::vector<int> result;
    result.reserve(entries.size());

    if (activeFilter == PresetCategory::All)
    {
        for (int i = 0; i < static_cast<int>(entries.size()); ++i)
            result.push_back(i);
    }
    else
    {
        for (int i = 0; i < static_cast<int>(entries.size()); ++i)
        {
            if (entries[static_cast<size_t>(i)].info.category == activeFilter)
                result.push_back(i);
        }
    }

    return result;
}

int PresetManager::currentFilteredPos(const std::vector<int>& filtered) const
{
    for (int pos = 0; pos < static_cast<int>(filtered.size()); ++pos)
    {
        if (filtered[static_cast<size_t>(pos)] == currentIndex)
            return pos;
    }
    return -1;
}

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../Source/Preset/PresetManager.h"

// ---------------------------------------------------------------------------
// Helper: build a synthetc <V0idPreset> XML string for testing.
// No dependency on BinaryData or real factory preset files.
// ---------------------------------------------------------------------------
static juce::String makePresetXml(const juce::String& name,
                                  const juce::String& category,
                                  const juce::String& tags = "",
                                  const juce::String& extraParams = "")
{
    return "<V0idPreset>"
           "<metadata name=\"" + name + "\" category=\"" + category + "\" tags=\"" + tags + "\"/>"
           "<v0idSynthParams>" + extraParams + "</v0idSynthParams>"
           "</V0idPreset>";
}

// ---------------------------------------------------------------------------
// Test 1: Default-constructed manager has 0 presets, no selection, All filter
// ---------------------------------------------------------------------------
TEST_CASE("PresetManager default state — empty, no current selection", "[PresetManager][PRE-02]")
{
    PresetManager mgr;
    REQUIRE(mgr.getNumPresets() == 0);
    REQUIRE(mgr.getCurrentIndex() == -1);
    REQUIRE(mgr.getCategoryFilter() == PresetCategory::All);
}

// ---------------------------------------------------------------------------
// Test 2: loadFromXmlStrings parses metadata correctly
// ---------------------------------------------------------------------------
TEST_CASE("PresetManager::loadFromXmlStrings — parses metadata", "[PresetManager][PRE-02]")
{
    PresetManager mgr;
    juce::StringArray xmls;
    xmls.add(makePresetXml("Alpha", "pad", "slow,evolving"));
    xmls.add(makePresetXml("Beta",  "bass", "sub"));
    xmls.add(makePresetXml("Gamma", "lead", ""));
    mgr.loadFromXmlStrings(xmls);

    REQUIRE(mgr.getNumPresets() == 3);
    REQUIRE(mgr.getPresetInfo(0).name == "Alpha");
    REQUIRE(mgr.getPresetInfo(0).category == PresetCategory::Pad);
    REQUIRE(mgr.getPresetInfo(0).tags.size() == 2);
    REQUIRE(mgr.getPresetInfo(2).category == PresetCategory::Lead);
}

// ---------------------------------------------------------------------------
// Test 3: getApvtsStateXmlForIndex returns inner <v0idSynthParams>
// ---------------------------------------------------------------------------
TEST_CASE("PresetManager::getApvtsStateXmlForIndex — returns inner v0idSynthParams",
          "[PresetManager][PRE-02]")
{
    PresetManager mgr;
    juce::StringArray xmls;
    xmls.add(makePresetXml("TestPad", "pad", "",
                           "<PARAM id=\"master.gain\" value=\"-3.0\"/>"));
    mgr.loadFromXmlStrings(xmls);

    auto inner = mgr.getApvtsStateXmlForIndex(0);
    REQUIRE(inner != nullptr);
    REQUIRE(inner->hasTagName("v0idSynthParams"));
    REQUIRE(inner->getNumChildElements() == 1);
    REQUIRE(inner->getChildElement(0)->getStringAttribute("id") == "master.gain");
}

// ---------------------------------------------------------------------------
// Test 4: nextPreset / previousPreset wrap-around
// ---------------------------------------------------------------------------
TEST_CASE("PresetManager::nextPreset / previousPreset wrap-around", "[PresetManager][PRE-02]")
{
    PresetManager mgr;
    juce::StringArray xmls;
    xmls.add(makePresetXml("P1", "pad"));
    xmls.add(makePresetXml("P2", "pad"));
    xmls.add(makePresetXml("P3", "pad"));
    mgr.loadFromXmlStrings(xmls);

    REQUIRE(mgr.nextPreset() == 0); // first call from -1 lands on index 0
    REQUIRE(mgr.nextPreset() == 1);
    REQUIRE(mgr.nextPreset() == 2);
    REQUIRE(mgr.nextPreset() == 0); // wrap forward
    REQUIRE(mgr.previousPreset() == 2); // wrap backward
}

// ---------------------------------------------------------------------------
// Test 5: setCategoryFilter Bass-only navigation skips non-Bass entries
// ---------------------------------------------------------------------------
TEST_CASE("PresetManager::setCategoryFilter — Bass-only navigation skips others",
          "[PresetManager][PRE-03]")
{
    PresetManager mgr;
    juce::StringArray xmls;
    xmls.add(makePresetXml("A", "pad"));   // index 0
    xmls.add(makePresetXml("B", "bass"));  // index 1
    xmls.add(makePresetXml("C", "lead"));  // index 2
    xmls.add(makePresetXml("D", "bass"));  // index 3
    xmls.add(makePresetXml("E", "fx"));    // index 4
    mgr.loadFromXmlStrings(xmls);

    // Entries are stable-sorted by category (Pad, Bass, Lead, FX, ...) so
    // navigation matches the popup grouping: A, B, D, C, E -> Bass at 1 and 2.
    mgr.setCategoryFilter(PresetCategory::Bass);
    REQUIRE(mgr.nextPreset() == 1); // first Bass  (B)
    REQUIRE(mgr.nextPreset() == 2); // second Bass (D)
    REQUIRE(mgr.nextPreset() == 1); // wrap to first Bass
    REQUIRE(mgr.getPresetInfo(2).name == "D");
    REQUIRE(mgr.getPresetNamesForCurrentFilter().size() == 2);
}

// ---------------------------------------------------------------------------
// Test 6: setCategoryFilter(All) restores full navigation
// ---------------------------------------------------------------------------
TEST_CASE("PresetManager::setCategoryFilter(All) — restores full navigation",
          "[PresetManager][PRE-03]")
{
    PresetManager mgr;
    juce::StringArray xmls;
    xmls.add(makePresetXml("X", "pad"));
    xmls.add(makePresetXml("Y", "lead"));
    xmls.add(makePresetXml("Z", "bass"));
    mgr.loadFromXmlStrings(xmls);

    mgr.setCategoryFilter(PresetCategory::Lead);
    // navigate through Lead presets
    mgr.nextPreset();

    mgr.setCategoryFilter(PresetCategory::All);
    REQUIRE(mgr.getPresetNamesForCurrentFilter().size() == 3);

    // Confirm all 3 are visited via nextPreset
    juce::StringArray visited;
    for (int i = 0; i < 3; ++i)
    {
        int idx = mgr.nextPreset();
        visited.add(mgr.getPresetInfo(idx).name);
    }
    REQUIRE(visited.contains("X"));
    REQUIRE(visited.contains("Y"));
    REQUIRE(visited.contains("Z"));
}

// ---------------------------------------------------------------------------
// Test 7: Empty filter bucket returns -1 from navigation
// ---------------------------------------------------------------------------
TEST_CASE("PresetManager::setCategoryFilter — empty filter bucket returns -1 from navigation",
          "[PresetManager][PRE-03]")
{
    PresetManager mgr;
    juce::StringArray xmls;
    xmls.add(makePresetXml("P1", "pad"));
    xmls.add(makePresetXml("P2", "pad"));
    mgr.loadFromXmlStrings(xmls);

    mgr.setCategoryFilter(PresetCategory::Bass);
    REQUIRE(mgr.nextPreset() == -1);
    REQUIRE(mgr.previousPreset() == -1);
}

// ---------------------------------------------------------------------------
// Test 8: selectByName finds preset and obeys filter
// ---------------------------------------------------------------------------
TEST_CASE("PresetManager::selectByName finds preset and obeys filter", "[PresetManager][PRE-02]")
{
    PresetManager mgr;
    juce::StringArray xmls;
    xmls.add(makePresetXml("Fluorescent Hum", "pad"));  // index 0
    xmls.add(makePresetXml("Sub Drone",       "bass")); // index 1
    xmls.add(makePresetXml("Wrong Room",      "fx"));   // index 2
    mgr.loadFromXmlStrings(xmls);

    REQUIRE(mgr.selectByName("Sub Drone") == true);
    REQUIRE(mgr.getCurrentIndex() == 1);

    mgr.setCategoryFilter(PresetCategory::Pad);
    REQUIRE(mgr.selectByName("Sub Drone") == false);  // filtered out
    REQUIRE(mgr.selectByName("DoesNotExist") == false);
}

// ---------------------------------------------------------------------------
// Test 9: loadFromXmlStrings skips malformed entries without crash
// ---------------------------------------------------------------------------
TEST_CASE("PresetManager::loadFromXmlStrings — skips malformed entries without crash",
          "[PresetManager][PRE-02]")
{
    PresetManager mgr;
    juce::StringArray arr;
    arr.add(makePresetXml("Good1", "pad"));            // valid
    arr.add("not xml at all");                         // invalid XML
    arr.add("<UnknownRoot/>");                         // wrong root tag
    arr.add(makePresetXml("Good2", "bass"));           // valid
    arr.add("<V0idPreset><v0idSynthParams/></V0idPreset>"); // missing metadata

    mgr.loadFromXmlStrings(arr);
    REQUIRE(mgr.getNumPresets() == 2); // only the two valid ones
}

// ---------------------------------------------------------------------------
// Test 10: parseCategory is case-insensitive
// ---------------------------------------------------------------------------
TEST_CASE("PresetManager::parseCategory is case-insensitive", "[PresetManager][PRE-03]")
{
    REQUIRE(PresetManager::parseCategory("PAD")     == PresetCategory::Pad);
    REQUIRE(PresetManager::parseCategory("Bass")    == PresetCategory::Bass);
    REQUIRE(PresetManager::parseCategory("organic") == PresetCategory::Organic);
    REQUIRE(PresetManager::parseCategory("garbage") == PresetCategory::All);
    REQUIRE(PresetManager::parseCategory("")        == PresetCategory::All);
}

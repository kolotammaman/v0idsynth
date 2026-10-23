#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../Source/PluginProcessor.h"
#include "../Source/Preset/PresetManager.h"
#include "../Source/Parameters.h"
#include <set>
#include <cmath>

TEST_CASE("Integration: processor loads 39 factory presets from BinaryData on construction", "[PresetIntegration][PRE-01]")
{
    V0idSynthAudioProcessor proc;
    REQUIRE(proc.getPresetManager().getNumPresets() == 39);
}

TEST_CASE("Integration: every factory preset loads via loadPresetByIndex without error", "[PresetIntegration][PRE-01]")
{
    V0idSynthAudioProcessor proc;
    for (int i = 0; i < proc.getPresetManager().getNumPresets(); ++i)
    {
        INFO("Preset index " << i << " name=" << proc.getPresetManager().getPresetInfo(i).name);
        REQUIRE(proc.loadPresetByIndex(i) == true);
        REQUIRE(proc.getLiveModMatrix() != nullptr);
    }
}

TEST_CASE("Integration: loadPresetByIndex(-1) and out-of-range return false safely", "[PresetIntegration][PRE-02]")
{
    V0idSynthAudioProcessor proc;
    REQUIRE(proc.loadPresetByIndex(-1) == false);
    REQUIRE(proc.loadPresetByIndex(999) == false);
    REQUIRE(proc.getPresetManager().getNumPresets() == 39); // state intact
}

TEST_CASE("Integration: loading a preset changes parameter values", "[PresetIntegration][PRE-01]")
{
    V0idSynthAudioProcessor proc;
    // Snapshot master.gain before
    float before = *proc.apvts.getRawParameterValue(ParamIDs::MASTER_GAIN);
    // Find a preset whose master.gain differs (scan)
    int diffIdx = -1;
    for (int i = 0; i < proc.getPresetManager().getNumPresets(); ++i)
    {
        proc.loadPresetByIndex(i);
        float v = *proc.apvts.getRawParameterValue(ParamIDs::MASTER_GAIN);
        if (std::abs(v - before) > 0.01f) { diffIdx = i; break; }
    }
    INFO("Expected at least one preset to set master.gain to a value different from default");
    REQUIRE(diffIdx >= 0);
}

TEST_CASE("Integration: category filter restricts navigation to Pad presets (PRE-03)", "[PresetIntegration][PRE-03]")
{
    V0idSynthAudioProcessor proc;
    auto& mgr = proc.getPresetManager();
    mgr.setCategoryFilter(PresetCategory::Pad);
    auto padNames = mgr.getPresetNamesForCurrentFilter();
    REQUIRE(padNames.size() == 6);
    // Cycle next 6 times -- expect 6 distinct indices, all Pad
    std::set<int> seen;
    for (int i = 0; i < 6; ++i)
    {
        int idx = mgr.nextPreset();
        REQUIRE(idx >= 0);
        REQUIRE(mgr.getPresetInfo(idx).category == PresetCategory::Pad);
        seen.insert(idx);
    }
    REQUIRE(seen.size() == 6);
}

TEST_CASE("Integration: sample-template presets set osc.mix to 0.0 (D-07 analog fallback)", "[PresetIntegration][PRE-01]")
{
    // Static templates must start fully analog so they still sound right before a
    // sample is loaded. Templates that sweep osc.mix with a mod route (e.g. Borrowed
    // Tape's saw <-> sample morph) need a centred base value, so they're exempt.
    V0idSynthAudioProcessor proc;
    auto& mgr = proc.getPresetManager();
    mgr.setCategoryFilter(PresetCategory::Sample);
    REQUIRE(mgr.getPresetNamesForCurrentFilter().size() == 6);
    for (int i = 0; i < 6; ++i)
    {
        int idx = mgr.nextPreset();
        proc.loadPresetByIndex(idx);

        bool modulatesMix = false;
        if (auto* table = proc.getLiveModMatrix())
            for (int r = 0; r < table->numRoutes; ++r)
                modulatesMix |= table->routes[r].dest == ModDest::OscMix;

        if (modulatesMix)
            continue;

        float oscMix = *proc.apvts.getRawParameterValue(ParamIDs::OSC_MIX);
        INFO("Sample preset '" << mgr.getPresetInfo(idx).name << "' osc.mix=" << oscMix);
        REQUIRE_THAT(oscMix, Catch::Matchers::WithinAbs(0.0f, 0.001f));
    }
}

TEST_CASE("Integration: parameters a preset omits reset to default instead of leaking", "[PresetIntegration][PRE-01]")
{
    // Factory presets predate mod.envN.depth / osc2.*; a user tweak to those
    // must not survive into the next preset load.
    V0idSynthAudioProcessor proc;
    auto* depth = proc.apvts.getParameter(ParamIDs::MOD_ENV1_DEPTH);
    auto* tune  = proc.apvts.getParameter(ParamIDs::OSC2_TUNE);
    depth->setValueNotifyingHost(depth->convertTo0to1(-0.7f));
    tune ->setValueNotifyingHost(tune ->convertTo0to1(42.0f));

    REQUIRE(proc.loadPresetByIndex(0));
    REQUIRE_THAT(proc.apvts.getRawParameterValue(ParamIDs::MOD_ENV1_DEPTH)->load(), Catch::Matchers::WithinAbs(1.0f, 0.001f));
    REQUIRE_THAT(proc.apvts.getRawParameterValue(ParamIDs::OSC2_TUNE)->load(),      Catch::Matchers::WithinAbs(0.0f, 0.001f));
}

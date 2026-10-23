#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../Source/Modulation/ModMatrixTable.h"
#include "../Source/PluginProcessor.h"
#include <thread>
#include <atomic>

// Contract sanity (Wave 0 — kept from plan 03-01)
TEST_CASE("ModMatrixTable contract — types and constants", "[ModMatrixTable][MOD-03]")
{
    STATIC_REQUIRE(ModMatrixTable::kMaxRoutes == 32);
    STATIC_REQUIRE((int)ModSource::LFO1 == 0);
    STATIC_REQUIRE((int)ModDest::FilterCutoff == 0);

    ModMatrixTable t;
    REQUIRE(t.numRoutes == 0);

    ModOutputs out;
    REQUIRE(out.filterCutoffOffset == 0.0f);
    REQUIRE(out.filterResOffset == 0.0f);
    REQUIRE(out.pitchCentsOffset == 0.0f);
    REQUIRE(out.oscMixOffset == 0.0f);
    REQUIRE(out.ampGainOffset == 0.0f);
}

// MOD-03: routing compilation from ValueTree
TEST_CASE("PluginProcessor::addModRoute — single route compiles into live table",
          "[ModMatrixTable][MOD-03]")
{
    V0idSynthAudioProcessor proc;

    const ModMatrixTable* before = proc.getLiveModMatrix();
    REQUIRE(before != nullptr);
    REQUIRE(before->numRoutes == 0);

    proc.addModRoute(ModSource::LFO1, ModDest::FilterCutoff, 0.7f);

    const ModMatrixTable* after = proc.getLiveModMatrix();
    REQUIRE(after != nullptr);
    REQUIRE(after->numRoutes == 1);
    REQUIRE(after->routes[0].source == ModSource::LFO1);
    REQUIRE(after->routes[0].dest   == ModDest::FilterCutoff);
    REQUIRE_THAT(after->routes[0].depth, Catch::Matchers::WithinAbs(0.7f, 0.0001f));

    proc.clearModRoutes();
    REQUIRE(proc.getLiveModMatrix()->numRoutes == 0);
}

TEST_CASE("PluginProcessor — addModRoute appends multiple routes",
          "[ModMatrixTable][MOD-03]")
{
    V0idSynthAudioProcessor proc;
    proc.addModRoute(ModSource::LFO1, ModDest::FilterCutoff,    0.5f);
    proc.addModRoute(ModSource::LFO2, ModDest::OscPitch,        0.25f);
    proc.addModRoute(ModSource::Env1, ModDest::FilterResonance, -0.3f);

    const ModMatrixTable* t = proc.getLiveModMatrix();
    REQUIRE(t->numRoutes == 3);
    REQUIRE(t->routes[2].source == ModSource::Env1);
    REQUIRE_THAT(t->routes[2].depth, Catch::Matchers::WithinAbs(-0.3f, 0.0001f));
}

// MOD-03: atomic pointer swap mid-playback — stress test
TEST_CASE("PluginProcessor — recompile during simulated audio reads is crash-free",
          "[ModMatrixTable][MOD-03]")
{
    V0idSynthAudioProcessor proc;
    std::atomic<bool> stop { false };
    std::atomic<int>  loadCount { 0 };
    std::atomic<int>  nonNullCount { 0 };
    std::atomic<int>  validRangeCount { 0 };

    // Simulated audio thread: tight loop calling getLiveModMatrix and reading numRoutes
    std::thread audio([&]() {
        while (!stop.load(std::memory_order_acquire))
        {
            const ModMatrixTable* t = proc.getLiveModMatrix();
            loadCount.fetch_add(1, std::memory_order_relaxed);
            if (t != nullptr) {
                nonNullCount.fetch_add(1, std::memory_order_relaxed);
                int n = t->numRoutes;
                if (n >= 0 && n <= ModMatrixTable::kMaxRoutes)
                    validRangeCount.fetch_add(1, std::memory_order_relaxed);
            }
        }
    });

    // Message thread: rapid add/clear cycles
    for (int i = 0; i < 1000; ++i)
    {
        proc.addModRoute(ModSource::LFO1, ModDest::FilterCutoff, 0.5f);
        proc.clearModRoutes();
    }

    stop.store(true, std::memory_order_release);
    audio.join();

    REQUIRE(loadCount.load() > 0);
    REQUIRE(nonNullCount.load() == loadCount.load());        // never observed null
    REQUIRE(validRangeCount.load() == loadCount.load());     // numRoutes always in range
}

TEST_CASE("PluginProcessor — getStateInformation / setStateInformation restores routes",
          "[ModMatrixTable][MOD-03]")
{
    V0idSynthAudioProcessor proc;
    proc.addModRoute(ModSource::LFO2, ModDest::OscPitch, -0.25f);
    proc.addModRoute(ModSource::Env3, ModDest::AmpGain,   0.6f);

    juce::MemoryBlock saved;
    proc.getStateInformation(saved);

    proc.clearModRoutes();
    REQUIRE(proc.getLiveModMatrix()->numRoutes == 0);

    proc.setStateInformation(saved.getData(), (int)saved.getSize());

    const ModMatrixTable* t = proc.getLiveModMatrix();
    REQUIRE(t->numRoutes == 2);
    REQUIRE(t->routes[0].source == ModSource::LFO2);
    REQUIRE(t->routes[0].dest   == ModDest::OscPitch);
    REQUIRE_THAT(t->routes[0].depth, Catch::Matchers::WithinAbs(-0.25f, 0.0001f));
    REQUIRE(t->routes[1].source == ModSource::Env3);
    REQUIRE_THAT(t->routes[1].depth, Catch::Matchers::WithinAbs(0.6f, 0.0001f));
}

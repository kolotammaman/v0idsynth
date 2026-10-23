#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../Source/Modulation/ModEnvelope.h"
#include "../Source/Modulation/ModulationEngine.h"
#include "../Source/Modulation/ModMatrixTable.h"

static constexpr float kSR = 44100.0f;

// ----- MOD-02 (from plan 03-02 — keep as-is) -----

TEST_CASE("ModEnvelope — silent before noteOn, ramps during attack", "[ModEnvelope][MOD-02]")
{
    ModEnvelope env;
    env.prepare(kSR);
    env.setParameters(0.01f, 0.1f, 0.5f, 0.2f);
    REQUIRE_FALSE(env.isActive());
    REQUIRE(env.getNextSample() == 0.0f);
    env.noteOn();
    REQUIRE(env.isActive());
    float mid = 0.0f;
    for (int i = 0; i < 200; ++i) mid = env.getNextSample();
    REQUIRE(mid > 0.0f);
    REQUIRE(mid < 1.0f);
}

TEST_CASE("ModEnvelope — reaches sustain and decays on noteOff", "[ModEnvelope][MOD-02]")
{
    ModEnvelope env;
    env.prepare(kSR);
    env.setParameters(0.01f, 0.1f, 0.5f, 0.2f);
    env.noteOn();
    for (int i = 0; i < 5000; ++i) env.getNextSample();
    float sum = 0.0f;
    for (int i = 0; i < 1000; ++i) sum += env.getNextSample();
    REQUIRE_THAT(sum / 1000.0f, Catch::Matchers::WithinAbs(0.5f, 0.01f));
    env.noteOff();
    for (int i = 0; i < 100000; ++i) env.getNextSample();
    REQUIRE(env.getNextSample() < 0.001f);
}

TEST_CASE("ModEnvelope — 4 independent instances", "[ModEnvelope][MOD-02]")
{
    ModEnvelope envs[4];
    for (auto& e : envs) { e.prepare(kSR); e.setParameters(0.01f, 0.1f, 0.5f, 0.2f); }
    envs[0].noteOn();
    for (int i = 0; i < 500; ++i) for (auto& e : envs) e.getNextSample();
    REQUIRE(envs[0].isActive());
    REQUIRE_FALSE(envs[1].isActive());
    REQUIRE_FALSE(envs[2].isActive());
    REQUIRE_FALSE(envs[3].isActive());
}

// ----- MOD-03 / FLT-04 (this plan 03-03) -----

TEST_CASE("ModulationEngine::process — nullptr table returns zero ModOutputs",
          "[ModulationEngine][MOD-03]")
{
    ModulationEngine eng;
    eng.prepare(kSR, /*voiceIdx*/ 0);
    ModOutputs out = eng.process(nullptr);
    REQUIRE(out.filterCutoffOffset == 0.0f);
    REQUIRE(out.filterResOffset    == 0.0f);
    REQUIRE(out.pitchCentsOffset   == 0.0f);
    REQUIRE(out.oscMixOffset       == 0.0f);
    REQUIRE(out.ampGainOffset      == 0.0f);
}

TEST_CASE("ModulationEngine::process — empty table returns zero ModOutputs",
          "[ModulationEngine][MOD-03]")
{
    ModulationEngine eng;
    eng.prepare(kSR, 0);
    ModMatrixTable t; // numRoutes == 0
    ModOutputs out = eng.process(&t);
    REQUIRE(out.filterCutoffOffset == 0.0f);
    REQUIRE(out.pitchCentsOffset   == 0.0f);
}

TEST_CASE("ModulationEngine::process — two routes accumulate to both destinations",
          "[ModulationEngine][MOD-03]")
{
    ModulationEngine eng;
    eng.prepare(kSR, 0);

    // Configure LFO1 Sine 10Hz, depth 1.0 (set by engine internally)
    eng.setLFOShape(0, VoiceLFO::Shape::Sine);
    eng.setLFORate(0, 10.0f);
    eng.setLFODepth(0, 1.0f);

    // Configure Env1 fast attack so it produces signal quickly
    eng.setEnvParameters(0, 0.001f, 0.01f, 0.8f, 0.1f);

    ModMatrixTable t;
    t.numRoutes = 2;
    t.routes[0] = { ModSource::LFO1, ModDest::FilterCutoff, 0.5f };
    t.routes[1] = { ModSource::Env1, ModDest::OscPitch,     0.3f };

    eng.startNote(); // triggers env1
    // Advance ~100 samples so LFO has phase > 0 and env1 has ramped
    ModOutputs out;
    for (int i = 0; i < 100; ++i) out = eng.process(&t);

    // Both destinations have non-zero accumulated mod
    REQUIRE(std::abs(out.filterCutoffOffset) > 1.0f);
    REQUIRE(std::abs(out.pitchCentsOffset)   > 1.0f);
    // Untouched destinations remain zero
    REQUIRE(out.filterResOffset == 0.0f);
    REQUIRE(out.oscMixOffset    == 0.0f);
    REQUIRE(out.ampGainOffset   == 0.0f);
}

TEST_CASE("ModulationEngine — LFO->FilterCutoff produces positive cutoff offset at sine peak",
          "[ModulationEngine][FLT-04]")
{
    ModulationEngine eng;
    eng.prepare(kSR, 0);
    eng.setLFOShape(0, VoiceLFO::Shape::Sine);
    eng.setLFORate(0, 1.0f);
    eng.setLFODepth(0, 1.0f);

    ModMatrixTable t;
    t.numRoutes = 1;
    t.routes[0] = { ModSource::LFO1, ModDest::FilterCutoff, 1.0f };

    // The first call to process() steps the LFO from phase 0 -> phaseInc (≈sin(0)≈0)
    // After 11024 process calls we are at the sine peak (quarter-period).
    ModOutputs out;
    for (int i = 0; i < 11025; ++i) out = eng.process(&t);

    // Expect ≈ +10000.0f (within 200 Hz tolerance for accumulated phase rounding)
    REQUIRE(out.filterCutoffOffset > 9800.0f);
    REQUIRE(out.filterCutoffOffset < 10200.0f);
}

TEST_CASE("ModulationEngine — Env->FilterResonance produces positive res offset at sustain",
          "[ModulationEngine][FLT-04]")
{
    ModulationEngine eng;
    eng.prepare(kSR, 0);
    eng.setEnvParameters(0, 0.001f, 0.01f, 0.5f, 0.1f); // sustain == 0.5

    ModMatrixTable t;
    t.numRoutes = 1;
    t.routes[0] = { ModSource::Env1, ModDest::FilterResonance, 1.0f };

    eng.startNote();
    // Advance well past attack+decay to land in sustain
    ModOutputs out;
    for (int i = 0; i < 5000; ++i) out = eng.process(&t);

    // sustain 0.5 * depth 1.0 * kResModScale 0.5 = 0.25
    REQUIRE_THAT(out.filterResOffset, Catch::Matchers::WithinAbs(0.25f, 0.02f));
}

TEST_CASE("ModulationEngine::setDriftSource — Drift->OscPitch routes drift to cents",
          "[ModulationEngine][MOD-03]")
{
    ModulationEngine eng;
    eng.prepare(kSR, 0);
    eng.setDriftSource(0.25f);

    ModMatrixTable t;
    t.numRoutes = 1;
    t.routes[0] = { ModSource::Drift, ModDest::OscPitch, 1.0f };

    ModOutputs out = eng.process(&t);
    REQUIRE_THAT(out.pitchCentsOffset, Catch::Matchers::WithinAbs(300.0f, 0.01f)); // 0.25 * 1.0 * 1200 == 300
}

TEST_CASE("ModulationEngine::setEnvDepth — scales and inverts the envelope source",
          "[ModulationEngine][MOD-ENV-DEPTH]")
{
    auto runAtSustain = [](float depth)
    {
        ModulationEngine eng;
        eng.prepare(kSR, 0);
        eng.setEnvParameters(0, 0.001f, 0.01f, 0.5f, 0.1f); // sustain == 0.5
        eng.setEnvDepth(0, depth);

        ModMatrixTable t;
        t.numRoutes = 1;
        t.routes[0] = { ModSource::Env1, ModDest::OscPitch, 1.0f };

        eng.startNote();
        ModOutputs out;
        for (int i = 0; i < 5000; ++i) out = eng.process(&t);
        return out.pitchCentsOffset;
    };

    // sustain 0.5 * route 1.0 * 1200 cents == 600 at full depth
    REQUIRE_THAT(runAtSustain( 1.0f),  Catch::Matchers::WithinAbs( 600.0f, 5.0f));
    REQUIRE_THAT(runAtSustain( 0.25f), Catch::Matchers::WithinAbs( 150.0f, 2.0f));
    REQUIRE_THAT(runAtSustain(-0.5f),  Catch::Matchers::WithinAbs(-300.0f, 3.0f));
    REQUIRE_THAT(runAtSustain( 0.0f),  Catch::Matchers::WithinAbs(   0.0f, 0.001f));
}

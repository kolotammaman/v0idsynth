#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../Source/Modulation/VoiceLFO.h"
#include "../Source/Modulation/ModMatrixTable.h"

static constexpr float kSR = 44100.0f;

// MOD-01: VoiceLFO produces correct waveform values at known phase points
TEST_CASE("VoiceLFO::processSample — sine waveform shape", "[VoiceLFO][MOD-01]")
{
    VoiceLFO lfo;
    lfo.prepare(kSR);
    lfo.setShape(VoiceLFO::Shape::Sine);
    lfo.setRate(1.0f); // 1 Hz — period is 44100 samples
    lfo.setPhaseOffset(0.0f);

    float s0 = lfo.processSample(); // phase 0 — sin(0) = 0
    for (int i = 1; i < 11025; ++i) lfo.processSample(); // advance to quarter period (11025 samples)
    float sQuarter = lfo.processSample(); // phase ~0.25 — sin(pi/2) = 1

    REQUIRE_THAT(s0,       Catch::Matchers::WithinAbs(0.0f, 0.001f));
    REQUIRE_THAT(sQuarter, Catch::Matchers::WithinAbs(1.0f, 0.01f));
}

// MOD-01: VoiceLFO supports all 5 waveforms — all stay within [-1, +1]
TEST_CASE("VoiceLFO::setShape — all 5 waveforms stay within [-1, +1]", "[VoiceLFO][MOD-01]")
{
    for (int s = 0; s < (int)VoiceLFO::Shape::Count; ++s)
    {
        VoiceLFO lfo;
        lfo.prepare(kSR);
        lfo.setShape(static_cast<VoiceLFO::Shape>(s));
        lfo.setRate(1.0f);
        lfo.setPhaseOffset(0.0f);
        lfo.seedRNG(12345);
        for (int i = 0; i < 88200; ++i) // 2 seconds at 44.1 kHz
        {
            float v = lfo.processSample();
            REQUIRE(v >= -1.0001f);
            REQUIRE(v <=  1.0001f);
        }
    }
}

// MOD-01: 4 LFOs per voice advance independently at different rates
TEST_CASE("VoiceLFO — independent rates produce divergent phase", "[VoiceLFO][MOD-01]")
{
    VoiceLFO slow, fast;
    slow.prepare(kSR); fast.prepare(kSR);
    slow.setShape(VoiceLFO::Shape::Sine); fast.setShape(VoiceLFO::Shape::Sine);
    slow.setRate(1.0f);  fast.setRate(5.0f);
    slow.setPhaseOffset(0.0f); fast.setPhaseOffset(0.0f);

    float lastSlow = 0.0f, lastFast = 0.0f;
    for (int i = 0; i < 4410; ++i)
    {
        lastSlow = slow.processSample();
        lastFast = fast.processSample();
    }

    // After 0.1 sec: slow is at phase ~0.1 (sin≈0.59), fast is at phase ~0.5 (sin≈0)
    // Their outputs must differ by more than 0.1
    REQUIRE(std::abs(lastSlow - lastFast) > 0.1f);
}

// MOD-04: Per-voice phase offset — voice 0 and voice 1 differ at sample 0
TEST_CASE("VoiceLFO::setPhaseOffset — voice 0 vs voice 1 produce different first sample",
          "[VoiceLFO][MOD-04]")
{
    VoiceLFO v0, v1;
    v0.prepare(kSR); v1.prepare(kSR);
    v0.setShape(VoiceLFO::Shape::Sine); v1.setShape(VoiceLFO::Shape::Sine);
    v0.setRate(1.0f); v1.setRate(1.0f);

    // Golden ratio phase offset formula (matching plan 03-04 phaseOffset(voiceIdx, lfoIdx)):
    // phaseOffset(v, i) = fmod(v * kGolden + i * 0.25, 1.0)
    static constexpr float kGolden = 0.6180339887f;
    v0.setPhaseOffset(std::fmod(0.0f * kGolden + 0.0f * 0.25f, 1.0f)); // = 0.0  -> sin(0) = 0
    v1.setPhaseOffset(std::fmod(1.0f * kGolden + 0.0f * 0.25f, 1.0f)); // = 0.618 -> sin(0.618*2pi) ~ -0.93

    float a = v0.processSample();
    float b = v1.processSample();

    REQUIRE(std::abs(a - b) > 0.5f);
}

// MOD-01: S&H LFO holds value within a period and draws new value on wrap
TEST_CASE("VoiceLFO Shape::SampleHold — holds within period, changes on wrap",
          "[VoiceLFO][MOD-01]")
{
    // Use a high rate so fewer samples per period — avoids fp accumulation concerns.
    // At 10 Hz: period = 4410 samples. Run 1.5 periods (6615 samples) to ensure a wrap happens.
    VoiceLFO lfo;
    lfo.prepare(kSR);
    lfo.setShape(VoiceLFO::Shape::SampleHold);
    lfo.setRate(10.0f); // 10 Hz — period ≈ 4410 samples
    lfo.setPhaseOffset(0.0f);
    lfo.seedRNG(42);

    // Capture the first held value
    float firstHeld = lfo.processSample();

    // Run for 4000 samples (within the first period): must all be the same
    for (int i = 1; i < 4000; ++i)
    {
        float v = lfo.processSample();
        REQUIRE(v == firstHeld);
    }

    // Run for another 2700 samples (well past the first period boundary into the second period)
    // The S&H must have drawn a new value during the wrap
    float secondHeld = 0.0f;
    bool sawChange = false;
    for (int i = 0; i < 2700; ++i)
    {
        float v = lfo.processSample();
        if (v != firstHeld)
        {
            sawChange = true;
            secondHeld = v;
        }
    }
    REQUIRE(sawChange); // S&H must have changed value after the period wrapped
    (void)secondHeld;   // used implicitly via sawChange
}

// MOD-01: Square wave outputs exactly +1 and -1 at known phase points
TEST_CASE("VoiceLFO Shape::Square — exact +/-1 at phase 0 and 0.5",
          "[VoiceLFO][MOD-01]")
{
    VoiceLFO lfo;
    lfo.prepare(kSR);
    lfo.setShape(VoiceLFO::Shape::Square);
    lfo.setRate(1.0f);
    lfo.setPhaseOffset(0.0f);

    float s0 = lfo.processSample(); // phase 0: should be +1
    REQUIRE(s0 == 1.0f);

    // Advance to sample 22050 (half period — phase = 0.5)
    for (int i = 1; i < 22050; ++i) lfo.processSample();
    float sHalf = lfo.processSample(); // phase ~0.5: should be -1
    REQUIRE(sHalf == -1.0f);
}

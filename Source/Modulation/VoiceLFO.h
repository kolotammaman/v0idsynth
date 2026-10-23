#pragma once
#include <juce_core/juce_core.h>
#include <cmath>
#include <cstdint>

// Per-voice LFO generator for v0idsynth modulation matrix.
// Header-only, matching FilterEnvelope/DriveStage/OscMixer project convention.
// Four instances live per SynthVoice as VoiceLFO lfo[4].
// Shape enum order MUST match Parameters.h MOD_LFO1_WAVEFORM choice array
// ("Sine", "Triangle", "Saw", "Square", "S&H") so a direct int->Shape cast works.
class VoiceLFO
{
public:
    enum class Shape : uint8_t { Sine = 0, Triangle, Saw, Square, SampleHold, Count };

    void prepare(double sampleRate)
    {
        sr = static_cast<float>(sampleRate);
        updatePhaseInc();
    }

    // offset01: [0, 1) — called at startNote with golden-ratio formula
    // e.g. phaseOffset(voiceIdx, lfoIdx) = fmod(voiceIdx * 0.6180339887 + lfoIdx * 0.25, 1.0)
    void setPhaseOffset(float offset01)
    {
        phase = offset01 - std::floor(offset01); // wrap to [0, 1)
    }

    // Clamped silently to [0, 20] Hz matching MOD_LFO1_RATE parameter range
    void setRate(float hz)
    {
        rateHz = juce::jlimit(0.0f, 20.0f, hz);
        updatePhaseInc();
    }

    void setShape(Shape s) { shape = s; }

    // Seed the RNG used by SampleHold — enables deterministic per-voice values
    // when called with voiceIdx*1000+lfoIdx from SynthVoice::startNote
    void seedRNG(juce::int64 seed) { rng.setSeed(seed); }

    // Reset phase to 0 — used in tests; not called by SynthVoice in normal use
    void resetPhase() { phase = 0.0f; }

    // Phase-only tick for an LFO nothing is listening to: keeps it free-running
    // in sync without paying for the waveform (std::sin per voice per sample).
    void advance() noexcept
    {
        phase += phaseInc;
        if (phase >= 1.0f) phase -= 1.0f;
    }

    // Returns value in [-1, +1]. Advances internal phase by one sample.
    float processSample()
    {
        float out = 0.0f;
        switch (shape)
        {
            case Shape::Sine:
                out = std::sin(phase * juce::MathConstants<float>::twoPi);
                break;
            case Shape::Triangle:
                // 1 at phase 0.25, -1 at phase 0.75, 0 at edges
                out = 1.0f - 4.0f * std::abs(phase - 0.5f);
                break;
            case Shape::Saw:
                // rises from -1 to +1 over one period
                out = 2.0f * phase - 1.0f;
                break;
            case Shape::Square:
                out = (phase < 0.5f) ? 1.0f : -1.0f;
                break;
            case Shape::SampleHold:
                // Draw new random value on phase wrap (phase < phaseInc means we just wrapped)
                // shInitialized guard ensures the very first call produces a value.
                if (phase < phaseInc || !shInitialized)
                {
                    shValue = rng.nextFloat() * 2.0f - 1.0f;
                    shInitialized = true;
                }
                out = shValue;
                break;
            default:
                break;
        }

        phase += phaseInc;
        if (phase >= 1.0f) phase -= 1.0f;
        return out;
    }

private:
    void updatePhaseInc()
    {
        phaseInc = (sr > 0.0f) ? (rateHz / sr) : 0.0f;
    }

    float        sr            = 44100.0f;
    float        rateHz        = 1.0f;
    float        phase         = 0.0f;
    float        phaseInc      = 1.0f / 44100.0f;
    float        shValue       = 0.0f;
    bool         shInitialized = false;
    Shape        shape         = Shape::Sine;
    juce::Random rng;
};

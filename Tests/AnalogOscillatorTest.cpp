#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "DSP/AnalogOscillator.h"
#include "DSP/DriveStage.h"
#include <cmath>
#include <vector>
#include <algorithm>
#include <numeric>

static std::vector<float> renderOscillator(AnalogOscillator::Waveform waveform,
                                            float freq, double sampleRate, int numSamples)
{
    AnalogOscillator osc;
    osc.prepare(sampleRate);
    osc.setFrequency(freq);
    osc.setWaveform(waveform);
    std::vector<float> buffer(numSamples);
    for (int i = 0; i < numSamples; ++i)
        buffer[i] = osc.processSample();
    return buffer;
}

static float computeRMS(const std::vector<float>& buf)
{
    float sum = 0.0f;
    for (auto s : buf) sum += s * s;
    return std::sqrt(sum / static_cast<float>(buf.size()));
}

TEST_CASE("Saw waveform produces non-silent output")
{
    auto buf = renderOscillator(AnalogOscillator::Waveform::Saw, 440.0f, 44100.0, 44100);
    REQUIRE(computeRMS(buf) > 0.01f);
}

TEST_CASE("Square waveform produces non-silent output")
{
    auto buf = renderOscillator(AnalogOscillator::Waveform::Square, 440.0f, 44100.0, 44100);
    REQUIRE(computeRMS(buf) > 0.01f);
}

TEST_CASE("Triangle waveform produces non-silent output")
{
    // Skip initial transient (leaky integrator needs a few cycles to settle)
    AnalogOscillator osc;
    osc.prepare(44100.0);
    osc.setFrequency(440.0f);
    osc.setWaveform(AnalogOscillator::Waveform::Triangle);
    // Warm up for 0.1s
    for (int i = 0; i < 4410; ++i) osc.processSample();
    std::vector<float> buf(44100);
    for (int i = 0; i < 44100; ++i) buf[i] = osc.processSample();
    REQUIRE(computeRMS(buf) > 0.01f);
}

TEST_CASE("Sine waveform produces non-silent output")
{
    auto buf = renderOscillator(AnalogOscillator::Waveform::Sine, 440.0f, 44100.0, 44100);
    REQUIRE(computeRMS(buf) > 0.01f);
}

TEST_CASE("All waveforms stay within amplitude bounds")
{
    const float limit = 1.05f;
    for (auto wf : { AnalogOscillator::Waveform::Saw, AnalogOscillator::Waveform::Square,
                     AnalogOscillator::Waveform::Sine })
    {
        auto buf = renderOscillator(wf, 440.0f, 44100.0, 44100);
        for (float s : buf)
            REQUIRE(std::abs(s) <= limit);
    }
    // Triangle with warmup
    AnalogOscillator osc;
    osc.prepare(44100.0);
    osc.setFrequency(440.0f);
    osc.setWaveform(AnalogOscillator::Waveform::Triangle);
    for (int i = 0; i < 4410; ++i) osc.processSample();
    for (int i = 0; i < 44100; ++i)
        REQUIRE(std::abs(osc.processSample()) <= limit);
}

TEST_CASE("Sine RMS matches theoretical value")
{
    auto buf = renderOscillator(AnalogOscillator::Waveform::Sine, 440.0f, 44100.0, 44100);
    float rms = computeRMS(buf);
    float expected = 1.0f / std::sqrt(2.0f); // ~0.7071
    REQUIRE(rms > expected * 0.9f);
    REQUIRE(rms < expected * 1.1f);
}

TEST_CASE("Sine frequency accuracy via zero crossings")
{
    auto buf = renderOscillator(AnalogOscillator::Waveform::Sine, 440.0f, 44100.0, 44100);
    int crossings = 0;
    for (int i = 1; i < static_cast<int>(buf.size()); ++i)
    {
        if (buf[i - 1] <= 0.0f && buf[i] > 0.0f)
            ++crossings;
    }
    // 440Hz over 1 second should have ~440 positive-going zero crossings
    REQUIRE(crossings >= 438);
    REQUIRE(crossings <= 442);
}

TEST_CASE("DriveStage clean pass-through")
{
    DriveStage drive;
    AnalogOscillator osc;
    osc.prepare(44100.0);
    osc.setFrequency(440.0f);
    osc.setWaveform(AnalogOscillator::Waveform::Sine);

    for (int i = 0; i < 1000; ++i)
    {
        float sample = osc.processSample();
        float processed = drive.processSample(sample, 0.0f);
        REQUIRE(std::abs(processed - sample) < 0.001f);
    }
}

TEST_CASE("DriveStage saturation clips relative to linear gain")
{
    DriveStage drive;
    // With drive=1.0, gain=5x is applied then tanh-clipped and normalized.
    // The output should be well below what raw 5x gain would produce,
    // confirming soft-clip saturation is occurring.
    AnalogOscillator osc;
    osc.prepare(44100.0);
    osc.setFrequency(440.0f);
    osc.setWaveform(AnalogOscillator::Waveform::Sine);

    const float driveAmt = 1.0f;
    const float linearGain = 1.0f + driveAmt * 4.0f; // = 5.0
    float maxLinear = 0.0f;
    float maxDriven = 0.0f;
    for (int i = 0; i < 44100; ++i)
    {
        float sample = osc.processSample();
        maxLinear = std::max(maxLinear, std::abs(sample * linearGain));
        maxDriven = std::max(maxDriven, std::abs(drive.processSample(sample, driveAmt)));
    }
    // Tanh soft clip should hold output well below linear gain would produce
    REQUIRE(maxDriven < maxLinear * 0.5f);
    // And output stays within unity range
    REQUIRE(maxDriven <= 1.01f);
}

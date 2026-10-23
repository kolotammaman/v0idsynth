#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "DSP/MultiModeFilter.h"
#include "DSP/FilterEnvelope.h"
#include <vector>
#include <random>
#include <cmath>
#include <numeric>
#include <algorithm>

static std::vector<float> generateWhiteNoise(int numSamples, unsigned seed = 42)
{
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    std::vector<float> buf(numSamples);
    for (auto& s : buf) s = dist(rng);
    return buf;
}

static float computeRMS(const std::vector<float>& buf)
{
    float sum = 0.0f;
    for (auto s : buf) sum += s * s;
    return std::sqrt(sum / static_cast<float>(buf.size()));
}

static void setupFilter(MultiModeFilter& f, float cutoff, float modeBlend, float drive = 0.0f, float resonance = 0.0f)
{
    f.prepare(44100.0, 512);
    f.setCutoffFrequency(cutoff);
    f.setResonance(resonance);
    f.setModeBlend(modeBlend);
    f.setDrive(drive);
}

static std::vector<float> processNoise(MultiModeFilter& filter, const std::vector<float>& noise)
{
    std::vector<float> out(noise.size());
    for (size_t i = 0; i < noise.size(); ++i)
        out[i] = filter.processSample(noise[i]);
    return out;
}

TEST_CASE("LP mode attenuates signal")
{
    auto noise = generateWhiteNoise(44100);
    float inputRMS = computeRMS(noise);

    MultiModeFilter filter;
    setupFilter(filter, 1000.0f, 0.0f);
    float outputRMS = computeRMS(processNoise(filter, noise));

    REQUIRE(outputRMS < inputRMS);
}

TEST_CASE("HP mode attenuates signal")
{
    auto noise = generateWhiteNoise(44100);
    float inputRMS = computeRMS(noise);

    MultiModeFilter filter;
    setupFilter(filter, 1000.0f, 1.0f);
    float outputRMS = computeRMS(processNoise(filter, noise));

    REQUIRE(outputRMS < inputRMS);
}

TEST_CASE("BP mode attenuates signal")
{
    auto noise = generateWhiteNoise(44100);
    float inputRMS = computeRMS(noise);

    MultiModeFilter filter;
    setupFilter(filter, 1000.0f, 2.0f);
    float outputRMS = computeRMS(processNoise(filter, noise));

    REQUIRE(outputRMS < inputRMS);
}

TEST_CASE("Notch mode attenuates signal")
{
    auto noise = generateWhiteNoise(44100);
    float inputRMS = computeRMS(noise);

    MultiModeFilter filter;
    setupFilter(filter, 1000.0f, 3.0f);
    float outputRMS = computeRMS(processNoise(filter, noise));

    REQUIRE(outputRMS < inputRMS);
}

TEST_CASE("Mode crossfade produces blended output")
{
    auto noise = generateWhiteNoise(44100);

    MultiModeFilter filterLP, filterHP, filterMid;
    setupFilter(filterLP,  1000.0f, 0.0f);
    setupFilter(filterHP,  1000.0f, 1.0f);
    setupFilter(filterMid, 1000.0f, 0.5f);

    float rmsLP  = computeRMS(processNoise(filterLP,  noise));
    float rmsHP  = computeRMS(processNoise(filterHP,  noise));
    float rmsMid = computeRMS(processNoise(filterMid, noise));

    // The blended output should differ from both pure LP and pure HP
    REQUIRE(rmsMid != Catch::Approx(rmsLP).epsilon(0.01));
    REQUIRE(rmsMid != Catch::Approx(rmsHP).epsilon(0.01));
}

TEST_CASE("Drive increases harmonic energy")
{
    // Process a 440Hz sine through the filter with drive=0 vs drive=1
    // Saturation adds harmonics, increasing RMS
    const int numSamples = 44100;
    std::vector<float> sine(numSamples);
    for (int i = 0; i < numSamples; ++i)
        sine[i] = 0.5f * std::sin(2.0f * static_cast<float>(M_PI) * 440.0f * i / 44100.0f);

    MultiModeFilter filterClean, filterDriven;
    setupFilter(filterClean,  20000.0f, 0.0f, 0.0f);
    setupFilter(filterDriven, 20000.0f, 0.0f, 1.0f);

    float rmsClean  = computeRMS(processNoise(filterClean,  sine));
    float rmsDriven = computeRMS(processNoise(filterDriven, sine));

    REQUIRE(rmsDriven >= rmsClean);
}

TEST_CASE("FilterEnvelope rises after noteOn")
{
    FilterEnvelope env;
    env.prepare(44100.0);
    env.setParameters(0.01f, 0.5f, 0.5f, 0.5f); // fast attack
    env.noteOn();

    float maxVal = 0.0f;
    for (int i = 0; i < 2000; ++i) // 2000 samples at 44100 = ~45ms, well past 10ms attack
        maxVal = std::max(maxVal, env.getNextSample());

    REQUIRE(maxVal > 0.5f);
}

TEST_CASE("FilterEnvelope modulateCutoff positive direction")
{
    float result = FilterEnvelope::modulateCutoff(1000.0f, 1.0f, 1.0f);
    REQUIRE(result > 1000.0f);
}

TEST_CASE("FilterEnvelope modulateCutoff negative direction")
{
    float result = FilterEnvelope::modulateCutoff(1000.0f, -1.0f, 1.0f);
    REQUIRE(result < 1000.0f);
}

TEST_CASE("FilterEnvelope modulateCutoff clamps to valid range")
{
    float upperClamped = FilterEnvelope::modulateCutoff(20000.0f, 1.0f, 1.0f);
    REQUIRE(upperClamped <= 20000.0f);

    float lowerClamped = FilterEnvelope::modulateCutoff(20.0f, -1.0f, 1.0f);
    REQUIRE(lowerClamped >= 20.0f);
}

TEST_CASE("Notch mode nulls a sine at the cutoff frequency")
{
    // The old notch (input - BP) only dipped ~6 dB at low resonance; the
    // corrected notch (input - k*BP == LP + HP) should remove the tone.
    const int numSamples = 44100;
    std::vector<float> sine(numSamples);
    for (int i = 0; i < numSamples; ++i)
        sine[i] = std::sin(2.0f * static_cast<float>(M_PI) * 1000.0f * i / 44100.0f);

    MultiModeFilter filter;
    setupFilter(filter, 1000.0f, 3.0f);
    auto out = processNoise(filter, sine);

    // Skip the settling transient, then the tone should be > 30 dB down.
    std::vector<float> tail(out.begin() + 22050, out.end());
    std::vector<float> inTail(sine.begin() + 22050, sine.end());
    REQUIRE(computeRMS(tail) < computeRMS(inTail) * 0.03f);
}

#include <catch2/catch_test_macros.hpp>
#include "../Source/Engine/SampleBank.h"

TEST_CASE("SampleBank — getCurrentBuffer returns nullptr before load", "[SampleBank]")
{
    SampleBank bank;
    REQUIRE(bank.getCurrentBuffer() == nullptr);
}

TEST_CASE("SampleBank — setBuffer then getCurrentBuffer returns same buffer", "[SampleBank]")
{
    SampleBank bank;
    ReferenceCountedBuffer::Ptr buf(new ReferenceCountedBuffer(1, 64));
    bank.setBuffer(buf);

    auto retrieved = bank.getCurrentBuffer();
    REQUIRE(retrieved != nullptr);
    REQUIRE(retrieved.get() == buf.get());
}

TEST_CASE("SampleBank — loadAsync returns immediately without blocking", "[SampleBank]")
{
    SampleBank bank;
    juce::File bogusFile("/tmp/v0idsynth_nonexistent_test_file.wav");

    auto start = juce::Time::getMillisecondCounterHiRes();
    bank.loadAsync(bogusFile);
    auto elapsed = juce::Time::getMillisecondCounterHiRes() - start;

    REQUIRE(elapsed < 10.0);
}

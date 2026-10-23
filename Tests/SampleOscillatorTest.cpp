#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../Source/DSP/SampleOscillator.h"
#include "../Source/DSP/OscMixer.h"

TEST_CASE("SampleOscillator — silence when no buffer loaded", "[SampleOscillator]")
{
    SampleOscillator osc;
    osc.prepare(44100.0);
    osc.setFrequency(440.0f);

    float sum = 0.0f;
    for (int i = 0; i < 1024; ++i)
        sum += std::abs(osc.processSample());

    REQUIRE(sum == 0.0f);
}

TEST_CASE("SampleOscillator — 1:1 playback at root note", "[SampleOscillator]")
{
    ReferenceCountedBuffer::Ptr buf(new ReferenceCountedBuffer(1, 512));
    float* data = buf->buffer.getWritePointer(0);
    for (int i = 0; i < 512; ++i)
        data[i] = (float)i / 512.0f;
    buf->rootMidiNote = 60;

    SampleOscillator osc;
    osc.prepare(44100.0);
    osc.setBuffer(buf);
    osc.setFrequency(SampleOscillator::midiNoteToHz(60));

    float s0 = osc.processSample();
    REQUIRE_THAT(s0, Catch::Matchers::WithinAbs(0.0f, 0.001f));

    float s1 = osc.processSample();
    REQUIRE_THAT(s1, Catch::Matchers::WithinAbs(1.0f / 512.0f, 0.001f));
}

TEST_CASE("SampleOscillator — 2x playback rate at one octave above root", "[SampleOscillator]")
{
    ReferenceCountedBuffer::Ptr buf(new ReferenceCountedBuffer(1, 1024));
    float* data = buf->buffer.getWritePointer(0);
    for (int i = 0; i < 1024; ++i)
        data[i] = (float)i;
    buf->rootMidiNote = 60;

    SampleOscillator osc;
    osc.prepare(44100.0);
    osc.setBuffer(buf);
    osc.setFrequency(SampleOscillator::midiNoteToHz(72));

    float s0 = osc.processSample();
    float s1 = osc.processSample();

    REQUIRE_THAT(s0, Catch::Matchers::WithinAbs(0.0f, 0.001f));
    // After s0 (pos=0), position advances to 2.0; linear interp at 2.0 = data[2] = 2.0
    REQUIRE_THAT(s1, Catch::Matchers::WithinAbs(2.0f, 0.01f));
}

TEST_CASE("SampleOscillator — silence after buffer end (one-shot)", "[SampleOscillator]")
{
    ReferenceCountedBuffer::Ptr buf(new ReferenceCountedBuffer(1, 4));
    float* data = buf->buffer.getWritePointer(0);
    data[0] = 1.0f; data[1] = 1.0f; data[2] = 1.0f; data[3] = 1.0f;
    buf->rootMidiNote = 60;

    SampleOscillator osc;
    osc.prepare(44100.0);
    osc.setBuffer(buf);
    osc.setFrequency(SampleOscillator::midiNoteToHz(60));

    osc.processSample(); osc.processSample();
    osc.processSample(); osc.processSample();

    REQUIRE(osc.processSample() == 0.0f);
    REQUIRE(osc.isFinished());
}

TEST_CASE("OscMixer — mix=0 outputs analog only", "[OscMixer]")
{
    OscMixer mixer;
    REQUIRE_THAT(mixer.mix(0.8f, 0.3f, 0.0f), Catch::Matchers::WithinAbs(0.8f, 0.0001f));
}

TEST_CASE("OscMixer — mix=1 outputs sample only", "[OscMixer]")
{
    OscMixer mixer;
    REQUIRE_THAT(mixer.mix(0.8f, 0.3f, 1.0f), Catch::Matchers::WithinAbs(0.3f, 0.0001f));
}

TEST_CASE("OscMixer — mix=0.5 outputs equal blend", "[OscMixer]")
{
    OscMixer mixer;
    float expected = (0.8f + 0.3f) * 0.5f;
    REQUIRE_THAT(mixer.mix(0.8f, 0.3f, 0.5f), Catch::Matchers::WithinAbs(expected, 0.0001f));
}

TEST_CASE("SampleOscillator — compensates for the file's recorded sample rate", "[SampleOscillator]")
{
    // A 48 kHz file at its root note must advance 48000/44100 source samples per
    // output sample in a 44.1 kHz session, or it plays ~1.5 semitones flat.
    ReferenceCountedBuffer::Ptr buf(new ReferenceCountedBuffer(1, 4096));
    float* data = buf->buffer.getWritePointer(0);
    for (int i = 0; i < 4096; ++i)
        data[i] = (float)i;
    buf->sourceSampleRate = 48000.0;

    SampleOscillator osc;
    osc.prepare(44100.0);
    osc.setBuffer(buf.get());
    osc.setFrequency(SampleOscillator::midiNoteToHz(60));

    float last = 0.0f;
    for (int i = 0; i < 1001; ++i)
        last = osc.processSample();

    // After 1000 steps the read position is 1000 * 48000/44100 ≈ 1088.4
    REQUIRE_THAT(last, Catch::Matchers::WithinAbs(1000.0f * 48000.0f / 44100.0f, 0.05f));
}

TEST_CASE("SampleOscillator — playback rate stays exact deep into a long sample", "[SampleOscillator]")
{
    // Float positions lose sub-sample resolution past ~2^17 samples, which
    // quantised the step and detuned long samples. Position is now double.
    const int len = 1 << 21; // ~47 s at 44.1 kHz
    ReferenceCountedBuffer::Ptr buf(new ReferenceCountedBuffer(1, len));
    float* data = buf->buffer.getWritePointer(0);
    for (int i = 0; i < len; ++i)
        data[i] = (float)i;

    SampleOscillator osc;
    osc.prepare(44100.0);
    osc.setBuffer(buf.get());
    osc.setFrequency(SampleOscillator::midiNoteToHz(61)); // +1 semitone: rate ≈ 1.05946

    const int steps = 1500000;
    float last = 0.0f;
    for (int i = 0; i <= steps; ++i)
        last = osc.processSample();

    const double expected = steps * std::exp2(1.0 / 12.0);
    REQUIRE(std::abs((double)last - expected) < 2.0);
}

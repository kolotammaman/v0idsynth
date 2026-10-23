#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../Source/Engine/SynthVoice.h"
#include "../Source/Engine/SynthSound.h"
#include "../Source/Parameters.h"
#include "../Source/Modulation/ModMatrixTable.h"

// Minimal AudioProcessor to host APVTS with real parameter layout
struct TestProcessor : public juce::AudioProcessor
{
    TestProcessor()
        : AudioProcessor(BusesProperties()
              .withOutput("Out", juce::AudioChannelSet::stereo(), true)),
          apvts(*this, nullptr, "params", createParameterLayout())
    {}

    const juce::String getName() const override { return "Test"; }
    void prepareToPlay(double, int) override {}
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }

    juce::AudioProcessorValueTreeState apvts;
};

static float getRMS(const juce::AudioBuffer<float>& buf, int channel)
{
    float sum = 0.0f;
    const float* data = buf.getReadPointer(channel);
    for (int i = 0; i < buf.getNumSamples(); ++i)
        sum += data[i] * data[i];
    return std::sqrt(sum / buf.getNumSamples());
}

TEST_CASE("Note-on produces audio", "[SynthVoice][basic]")
{
    TestProcessor proc;
    juce::Synthesiser synth;
    synth.setCurrentPlaybackSampleRate(44100.0);
    synth.addSound(new SynthSound());
    synth.addVoice(new SynthVoice(proc.apvts, 0));

    juce::AudioBuffer<float> buffer(2, 512);
    buffer.clear();

    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, 60, (uint8_t)100), 0);

    synth.renderNextBlock(buffer, midi, 0, 512);

    REQUIRE(getRMS(buffer, 0) > 0.0f);
}

TEST_CASE("16 voices polyphony", "[SynthVoice][polyphony]")
{
    TestProcessor proc;
    juce::Synthesiser synth;
    synth.setCurrentPlaybackSampleRate(44100.0);
    synth.addSound(new SynthSound());
    for (int i = 0; i < 16; ++i)
        synth.addVoice(new SynthVoice(proc.apvts, i));

    juce::MidiBuffer midi;
    for (int n = 0; n < 16; ++n)
        midi.addEvent(juce::MidiMessage::noteOn(1, 48 + n, (uint8_t)100), 0);

    juce::AudioBuffer<float> buffer(2, 512);
    buffer.clear();
    synth.renderNextBlock(buffer, midi, 0, 512);

    REQUIRE(getRMS(buffer, 0) > 0.0f);

    int activeCount = 0;
    for (int i = 0; i < synth.getNumVoices(); ++i)
        if (synth.getVoice(i)->isVoiceActive())
            ++activeCount;
    REQUIRE(activeCount == 16);
}

TEST_CASE("Tail-off continues rendering", "[SynthVoice][tailoff]")
{
    TestProcessor proc;
    juce::Synthesiser synth;
    synth.setCurrentPlaybackSampleRate(44100.0);
    synth.addSound(new SynthSound());
    synth.addVoice(new SynthVoice(proc.apvts, 0));

    // Note on
    juce::MidiBuffer midiOn;
    midiOn.addEvent(juce::MidiMessage::noteOn(1, 60, (uint8_t)100), 0);
    juce::AudioBuffer<float> buf1(2, 512);
    buf1.clear();
    synth.renderNextBlock(buf1, midiOn, 0, 512);

    // Note off (allowTailOff=true via Synthesiser)
    juce::MidiBuffer midiOff;
    midiOff.addEvent(juce::MidiMessage::noteOff(1, 60), 0);
    juce::AudioBuffer<float> buf2(2, 512);
    buf2.clear();
    synth.renderNextBlock(buf2, midiOff, 0, 512);

    // Voice should still produce audio during release phase
    REQUIRE(getRMS(buf2, 0) > 0.0f);
}

TEST_CASE("Hard stop clears voice", "[SynthVoice][tailoff]")
{
    TestProcessor proc;
    juce::Synthesiser synth;
    synth.setCurrentPlaybackSampleRate(44100.0);
    synth.addSound(new SynthSound());
    synth.addVoice(new SynthVoice(proc.apvts, 0));

    juce::MidiBuffer midiOn;
    midiOn.addEvent(juce::MidiMessage::noteOn(1, 60, (uint8_t)100), 0);
    juce::AudioBuffer<float> buf1(2, 512);
    buf1.clear();
    synth.renderNextBlock(buf1, midiOn, 0, 512);

    // Hard stop
    auto* voice = dynamic_cast<SynthVoice*>(synth.getVoice(0));
    REQUIRE(voice != nullptr);
    voice->stopNote(0.0f, false);

    juce::AudioBuffer<float> buf2(2, 512);
    buf2.clear();
    juce::MidiBuffer emptyMidi;
    synth.renderNextBlock(buf2, emptyMidi, 0, 512);

    REQUIRE(getRMS(buf2, 0) == 0.0f);
}

TEST_CASE("Portamento glide between notes", "[SynthVoice][portamento]")
{
    TestProcessor proc;
    // Set mono mode + short glide (0.1 -> ~0.031 sec = ~1367 samples, completes within 2048)
    if (auto* p = proc.apvts.getParameter(ParamIDs::VOICE_MODE))
        p->setValueNotifyingHost(p->convertTo0to1(1.0f)); // Mono=1
    if (auto* p = proc.apvts.getParameter(ParamIDs::VOICE_GLIDE))
        p->setValueNotifyingHost(0.1f); // short glide: 0.001 + 0.01 * 2.999 ≈ 0.031 sec

    juce::Synthesiser synth;
    synth.setCurrentPlaybackSampleRate(44100.0);
    synth.addSound(new SynthSound());
    synth.addVoice(new SynthVoice(proc.apvts, 0));

    // Play C4 and let it settle
    juce::MidiBuffer midiC4;
    midiC4.addEvent(juce::MidiMessage::noteOn(1, 60, (uint8_t)100), 0);
    juce::AudioBuffer<float> settle(2, 4096);
    settle.clear();
    synth.renderNextBlock(settle, midiC4, 0, 4096);

    // Retrigger to C5 — mono voice glides up (no new MIDI through Synthesiser; call startNote directly)
    auto* voice = dynamic_cast<SynthVoice*>(synth.getVoice(0));
    REQUIRE(voice != nullptr);
    auto* sound = synth.getSound(0).get();
    voice->startNote(72, 1.0f, sound, 0); // C5

    // Render 2048 samples during the glide
    juce::AudioBuffer<float> gliding(2, 2048);
    gliding.clear();
    juce::MidiBuffer empty;
    synth.renderNextBlock(gliding, empty, 0, 2048);

    // Count zero crossings in first 512 samples (near C4) vs last 512 samples (near C5)
    // C4 = 261.6 Hz → ~15 crossings/512 samples; C5 = 523.2 Hz → ~30 crossings/512 samples
    const float* data = gliding.getReadPointer(0);
    int first512 = 0, last512 = 0;
    for (int i = 1; i < 512; ++i)
        if (data[i] * data[i - 1] < 0.0f) ++first512;
    for (int i = 1537; i < 2048; ++i)
        if (data[i] * data[i - 1] < 0.0f) ++last512;

    // Pitch gliding up: zero crossing rate should be higher in the last window
    REQUIRE(last512 > first512);
}

TEST_CASE("Unison detune spread formula is symmetric", "[SynthVoice][unison]")
{
    // Test the locked formula from CONTEXT.md directly
    int N = 4;
    float detune = 1.0f;
    float maxDetuneCents = detune * 50.0f;

    std::vector<float> offsets(N);
    for (int i = 0; i < N; ++i)
        offsets[i] = maxDetuneCents * ((i - (N - 1) / 2.0f) / std::max((N - 1) / 2.0f, 1.0f));

    // Symmetric: voice 0 == -voice 3, voice 1 == -voice 2
    REQUIRE_THAT(offsets[0], Catch::Matchers::WithinAbs(-offsets[3], 0.01f));
    REQUIRE_THAT(offsets[1], Catch::Matchers::WithinAbs(-offsets[2], 0.01f));

    // Total spread is 100 cents at detune=1.0
    REQUIRE_THAT(offsets[3] - offsets[0], Catch::Matchers::WithinAbs(100.0f, 0.1f));

    // Center voices have smaller offset than outer voices
    REQUIRE(std::abs(offsets[1]) < std::abs(offsets[0]));
}

TEST_CASE("Unison produces stereo output", "[SynthVoice][unison]")
{
    TestProcessor proc;

    // Set unison mode, 4 voices, full spread
    if (auto* p = proc.apvts.getParameter(ParamIDs::VOICE_MODE))
        p->setValueNotifyingHost(p->convertTo0to1(2.0f)); // Unison=2
    if (auto* p = proc.apvts.getParameter(ParamIDs::VOICE_UNISON_VOICES))
        p->setValueNotifyingHost(p->convertTo0to1(4.0f));
    if (auto* p = proc.apvts.getParameter(ParamIDs::VOICE_UNISON_SPREAD))
        p->setValueNotifyingHost(1.0f);
    if (auto* p = proc.apvts.getParameter(ParamIDs::VOICE_UNISON_DETUNE))
        p->setValueNotifyingHost(0.5f);

    juce::Synthesiser synth;
    synth.setCurrentPlaybackSampleRate(44100.0);
    synth.addSound(new SynthSound());
    for (int i = 0; i < 16; ++i)
        synth.addVoice(new SynthVoice(proc.apvts, i));

    // Configure unison voices manually (as PluginProcessor would do)
    float detune = 0.5f, spread = 1.0f;
    int N = 4;
    float maxDetuneCents = detune * 50.0f;
    auto* sound = synth.getSound(0).get();

    int assigned = 0;
    for (int i = 0; i < synth.getNumVoices() && assigned < N; ++i)
    {
        auto* v = dynamic_cast<SynthVoice*>(synth.getVoice(i));
        if (v == nullptr) continue;

        if (N > 1)
            v->unisonDetuneOffsetCents = maxDetuneCents *
                ((assigned - (N - 1) / 2.0f) / std::max((N - 1) / 2.0f, 1.0f));
        else
            v->unisonDetuneOffsetCents = 0.0f;

        if (N > 1)
        {
            float pan = spread * ((2.0f * assigned / (N - 1)) - 1.0f);
            v->unisonPanLeft  = std::sqrt(0.5f * (1.0f - pan));
            v->unisonPanRight = std::sqrt(0.5f * (1.0f + pan));
        }
        v->startNote(60, 1.0f, sound, 0);
        ++assigned;
    }

    juce::AudioBuffer<float> buf(2, 2048);
    buf.clear();
    juce::MidiBuffer empty;
    synth.renderNextBlock(buf, empty, 0, 2048);

    float rmsL = getRMS(buf, 0);
    float rmsR = getRMS(buf, 1);
    REQUIRE(rmsL > 0.0f);
    REQUIRE(rmsR > 0.0f);

    // L and R should differ due to pan spread
    float diffEnergy = 0.0f;
    const float* l = buf.getReadPointer(0);
    const float* r = buf.getReadPointer(1);
    for (int i = 0; i < 2048; ++i)
    {
        float d = l[i] - r[i];
        diffEnergy += d * d;
    }
    REQUIRE(diffEnergy > 0.0f);
}

TEST_CASE("Drift produces variation between voices", "[SynthVoice][drift]")
{
    // Create two separate synthesisers with one voice each, same note
    TestProcessor proc;

    // Set drift to max
    if (auto* param = proc.apvts.getParameter(ParamIDs::VOICE_DRIFT))
        param->setValueNotifyingHost(1.0f);

    juce::Synthesiser synth0, synth1;
    synth0.setCurrentPlaybackSampleRate(44100.0);
    synth1.setCurrentPlaybackSampleRate(44100.0);
    synth0.addSound(new SynthSound());
    synth1.addSound(new SynthSound());
    synth0.addVoice(new SynthVoice(proc.apvts, 0));
    synth1.addVoice(new SynthVoice(proc.apvts, 1));

    juce::MidiBuffer noteOn;
    noteOn.addEvent(juce::MidiMessage::noteOn(1, 60, (uint8_t)100), 0);

    // Render note-on block
    juce::AudioBuffer<float> b0(2, 512), b1(2, 512);
    b0.clear(); b1.clear();
    synth0.renderNextBlock(b0, noteOn, 0, 512);
    synth1.renderNextBlock(b1, noteOn, 0, 512);

    // Render 100 more blocks (let drift accumulate)
    juce::MidiBuffer empty;
    float sumDiff = 0.0f;
    for (int block = 0; block < 100; ++block)
    {
        b0.clear(); b1.clear();
        synth0.renderNextBlock(b0, empty, 0, 512);
        synth1.renderNextBlock(b1, empty, 0, 512);

        const float* d0 = b0.getReadPointer(0);
        const float* d1 = b1.getReadPointer(0);
        for (int i = 0; i < 512; ++i)
        {
            float diff = d0[i] - d1[i];
            sumDiff += diff * diff;
        }
    }

    // Voices with different seeds should produce measurably different output
    REQUIRE(sumDiff > 0.0f);
}

// ===== Phase 3 integration tests =====

TEST_CASE("SynthVoice MOD-04: per-voice LFO phase offset produces distinct render output",
          "[SynthVoice][MOD-04]")
{
    // Two voices, same note, same params, same mod matrix — but different voiceIdx
    // means different LFO phase offsets, which means different cutoff trajectories.
    TestProcessor proc;
    auto* sound = new SynthSound();

    // Configure LFO1 to be a slow audible sweep
    *proc.apvts.getRawParameterValue(ParamIDs::MOD_LFO1_RATE)     = 0.5f;
    *proc.apvts.getRawParameterValue(ParamIDs::MOD_LFO1_WAVEFORM) = 0.0f; // Sine
    *proc.apvts.getRawParameterValue(ParamIDs::MOD_LFO1_DEPTH)    = 1.0f;
    *proc.apvts.getRawParameterValue(ParamIDs::FILTER_CUTOFF)     = 1000.0f;
    *proc.apvts.getRawParameterValue(ParamIDs::FILTER_RESONANCE)  = 0.3f;
    *proc.apvts.getRawParameterValue(ParamIDs::FILTER_ENV_AMOUNT) = 0.0f;
    *proc.apvts.getRawParameterValue(ParamIDs::AMP_ENV_ATTACK)    = 0.001f;
    *proc.apvts.getRawParameterValue(ParamIDs::AMP_ENV_SUSTAIN)   = 0.9f;

    ModMatrixTable table;
    table.numRoutes = 1;
    table.routes[0] = { ModSource::LFO1, ModDest::FilterCutoff, 1.0f };

    SynthVoice v0(proc.apvts, 0), v1(proc.apvts, 1);
    v0.setCurrentPlaybackSampleRate(44100.0);
    v1.setCurrentPlaybackSampleRate(44100.0);
    v0.setModMatrix(&table);
    v1.setModMatrix(&table);

    v0.startNote(60, 1.0f, sound, 0);
    v1.startNote(60, 1.0f, sound, 0);

    juce::AudioBuffer<float> buf0(2, 4410), buf1(2, 4410);
    buf0.clear(); buf1.clear();
    v0.renderNextBlock(buf0, 0, 4410);
    v1.renderNextBlock(buf1, 0, 4410);

    float rms0 = getRMS(buf0, 0);
    float rms1 = getRMS(buf1, 0);
    REQUIRE(rms0 > 0.0f);
    REQUIRE(rms1 > 0.0f);

    // Compare per-sample difference between the two voices. Phase offsets ~ 0.0 and 0.618
    // mean the two voices see different cutoff values throughout the buffer.
    float diffSumSq = 0.0f;
    for (int i = 0; i < buf0.getNumSamples(); ++i)
    {
        float d = buf0.getSample(0, i) - buf1.getSample(0, i);
        diffSumSq += d * d;
    }
    float diffRMS = std::sqrt(diffSumSq / (float)buf0.getNumSamples());
    REQUIRE(diffRMS > 0.05f * rms0);
    delete sound;
}

TEST_CASE("SynthVoice FLT-04: LFO->FilterCutoff route alters audio output",
          "[SynthVoice][FLT-04]")
{
    TestProcessor proc;
    auto* sound = new SynthSound();

    // Use a 5Hz LFO and very low base cutoff (200Hz) so the LFO sweep produces a large
    // RMS difference vs the no-route baseline.
    // At 5Hz, 4410 samples (100ms) = half a period: sine completes 0→1→0 (positive half-cycle).
    // kCutoffModScale = 10000Hz: base 200Hz sweeps to 10200Hz at peak, average ~6566Hz.
    // NoRoute: constant 200Hz (heavily attenuated). WithRoute: mostly wide open.
    // Voice 0 LFO phase offset = fmod(0*0.618+0*0.25,1.0) = 0.0 -> sine starts at 0, rises.
    *proc.apvts.getRawParameterValue(ParamIDs::MOD_LFO1_RATE)     = 5.0f;   // half-cycle fits in 100ms
    *proc.apvts.getRawParameterValue(ParamIDs::MOD_LFO1_WAVEFORM) = 0.0f;   // Sine
    *proc.apvts.getRawParameterValue(ParamIDs::MOD_LFO1_DEPTH)    = 1.0f;
    *proc.apvts.getRawParameterValue(ParamIDs::FILTER_CUTOFF)     = 200.0f;  // very low base: LFO opens filter dramatically
    *proc.apvts.getRawParameterValue(ParamIDs::FILTER_RESONANCE)  = 0.3f;
    *proc.apvts.getRawParameterValue(ParamIDs::FILTER_ENV_AMOUNT) = 0.0f;    // isolate mod-matrix path
    *proc.apvts.getRawParameterValue(ParamIDs::AMP_ENV_ATTACK)    = 0.001f;
    *proc.apvts.getRawParameterValue(ParamIDs::AMP_ENV_SUSTAIN)   = 0.9f;

    // Render WITHOUT route
    SynthVoice noRoute(proc.apvts, 0);
    noRoute.setCurrentPlaybackSampleRate(44100.0);
    noRoute.setModMatrix(nullptr);
    noRoute.startNote(60, 1.0f, sound, 0);
    juce::AudioBuffer<float> bufNoRoute(2, 4410); bufNoRoute.clear();
    noRoute.renderNextBlock(bufNoRoute, 0, 4410);
    float rmsNoRoute = getRMS(bufNoRoute, 0);

    // Render WITH route on a fresh voice instance
    ModMatrixTable table;
    table.numRoutes = 1;
    table.routes[0] = { ModSource::LFO1, ModDest::FilterCutoff, 1.0f };

    SynthVoice withRoute(proc.apvts, 0);
    withRoute.setCurrentPlaybackSampleRate(44100.0);
    withRoute.setModMatrix(&table);
    withRoute.startNote(60, 1.0f, sound, 0);
    juce::AudioBuffer<float> bufWithRoute(2, 4410); bufWithRoute.clear();
    withRoute.renderNextBlock(bufWithRoute, 0, 4410);
    float rmsWithRoute = getRMS(bufWithRoute, 0);

    REQUIRE(rmsNoRoute > 0.0f);
    REQUIRE(rmsWithRoute > 0.0f);
    // The LFO sweeping the cutoff should produce a measurably different RMS.
    REQUIRE(std::abs(rmsNoRoute - rmsWithRoute) > 0.05f * rmsNoRoute);
    delete sound;
}

TEST_CASE("SynthVoice FLT-03 preserved: FilterEnvelope still drives cutoff when mod matrix is empty",
          "[SynthVoice][FLT-03]")
{
    TestProcessor proc;
    auto* sound = new SynthSound();

    // Strong filter envelope, no mod matrix routes.
    // fast attack + short decay: bufEarly (0-50ms) includes the open-filter period;
    // bufLate (50-100ms) sees the closed filter (envelope fully decayed by 50ms).
    // With attack=0.001s, decay=0.05s, sustain=0: by 50ms the envelope is ~fully decayed.
    // base 300Hz * (2^48/12) = 300*16 = 4800Hz at peak -> lots of high-freq content.
    // bufEarly has higher average cutoff than bufLate, so rmsEarly > rmsLate meaningfully.
    *proc.apvts.getRawParameterValue(ParamIDs::FILTER_CUTOFF)     = 300.0f;
    *proc.apvts.getRawParameterValue(ParamIDs::FILTER_RESONANCE)  = 0.5f;
    *proc.apvts.getRawParameterValue(ParamIDs::FILTER_ENV_AMOUNT) = 1.0f;   // full positive sweep: 300→4800Hz
    *proc.apvts.getRawParameterValue(ParamIDs::FILTER_ENV_ATTACK) = 0.001f; // very fast attack (~44 samples)
    *proc.apvts.getRawParameterValue(ParamIDs::FILTER_ENV_DECAY)  = 0.04f;  // ~1764 samples — decays within bufEarly
    *proc.apvts.getRawParameterValue(ParamIDs::FILTER_ENV_SUSTAIN)= 0.0f;
    *proc.apvts.getRawParameterValue(ParamIDs::AMP_ENV_ATTACK)    = 0.001f;
    *proc.apvts.getRawParameterValue(ParamIDs::AMP_ENV_SUSTAIN)   = 0.9f;

    SynthVoice voice(proc.apvts, 0);
    voice.setCurrentPlaybackSampleRate(44100.0);
    voice.setModMatrix(nullptr);  // no mod matrix
    voice.startNote(60, 1.0f, sound, 0);

    juce::AudioBuffer<float> bufEarly(2, 2205);  bufEarly.clear();  // first 50 ms
    juce::AudioBuffer<float> bufLate (2, 2205);  bufLate.clear();   // next 50 ms

    voice.renderNextBlock(bufEarly, 0, 2205);
    voice.renderNextBlock(bufLate,  0, 2205);

    float rmsEarly = getRMS(bufEarly, 0);
    float rmsLate  = getRMS(bufLate, 0);

    REQUIRE(rmsEarly > 0.0f);
    REQUIRE(rmsLate  > 0.0f);
    // With fast attack + short decay (both complete within bufEarly), bufLate sees the
    // filter closed at 300Hz base. bufEarly sees the filter briefly wide open (4800Hz peak)
    // before closing. rmsEarly should be noticeably higher than rmsLate.
    REQUIRE(rmsEarly > rmsLate);
    REQUIRE((rmsEarly - rmsLate) > 0.05f * rmsEarly);
    delete sound;
}

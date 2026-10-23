#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Parameters.h"
#include "Engine/SynthVoice.h"
#include "Engine/SynthSound.h"
#include "Engine/SampleBank.h"
#include "Engine/ParamCache.h"
#include "Modulation/ModMatrixTable.h"
#include "Preset/PresetManager.h"
#include <atomic>
#include <functional>

// Synthesiser subclass that prefers stealing releasing voices over attack/sustain
// voices, so long-release pads don't cause note drops in high-density playing.
class V0idSynthesiser : public juce::Synthesiser
{
protected:
    juce::SynthesiserVoice* findFreeVoice (juce::SynthesiserSound* sound,
                                            int midiChannel, int midiNoteNumber,
                                            bool stealIfNecessary) const override;
private:
    juce::SynthesiserVoice* findVoiceToStart (juce::SynthesiserSound* sound,
                                              bool stealIfNecessary) const;
};

class V0idSynthAudioProcessor : public juce::AudioProcessor,
                                private juce::AsyncUpdater
{
public:
    V0idSynthAudioProcessor();
    ~V0idSynthAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // MESSAGE-THREAD-ONLY: publish the bank's current buffer to the audio thread.
    void notifySampleLoaded();

    // Thread-safe: load a sample file in the background. Completion is delivered
    // on the message thread (publish to voices, then onSampleChanged).
    void loadSampleAsync(const juce::File& file);

    // Display name of the loaded sample ("" when none).
    juce::String getLoadedSampleName() const;

    // ===== Modulation matrix infrastructure (Phase 3 / MOD-03) =====
    // Lock-free pattern: message thread compiles + atomically swaps liveModMatrix.
    // Audio thread does a single load(acquire) per processBlock.
    // Stale table stored for deferred deletion on next recompile.
    void recompileModMatrix();
    void addModRoute(ModSource source, ModDest dest, float depth);
    void clearModRoutes();
    const ModMatrixTable* getLiveModMatrix() const noexcept
    { return liveModMatrix.load(std::memory_order_acquire); }

    // ===== Preset system (Phase 4 / PRE-01, PRE-02, PRE-03) =====
    // MESSAGE-THREAD-ONLY. Synchronous recall per D-14. Returns false if index OOR.
    bool loadPresetByIndex(int index);

    // Read-only accessor for editor/UI. Lifetime tied to the processor.
    PresetManager& getPresetManager() noexcept { return presetManager; }
    const PresetManager& getPresetManager() const noexcept { return presetManager; }

    SampleBank& getSampleBank() noexcept { return sampleBank; }

    // Called by editor after preset load to signal listeners (currently unused, but
    // reserved for explicit Editor refresh hooks if APVTS::Listener callbacks miss).
    std::function<void()> onPresetLoaded; // assigned by editor; processor calls after replaceState (preset or host restore)
    std::function<void()> onSampleChanged; // assigned by editor; message thread, after a sample is published

    juce::AudioProcessorValueTreeState apvts;

private:
    void handleAsyncUpdate() override;

    // Adds a PARAM child at its default value for every parameter missing from
    // `state`. APVTS::replaceState() otherwise keeps the CURRENT value of an
    // absent parameter, so a tweak to a newer param (osc2.*, mod.envN.depth)
    // would leak into every older preset / project loaded afterwards.
    void fillMissingParamsWithDefaults(juce::ValueTree& state) const;

    ParamCache params { apvts }; // declared after apvts — resolves pointers once

    int currentProgram = 0;
    V0idSynthesiser synthesiser;
    SampleBank sampleBank;

    // Sample buffer handoff — same epoch scheme as the mod matrix. The audio thread
    // only ever sees a raw pointer (no refcount traffic); the message thread keeps
    // every published buffer alive until a full block has completed without it.
    std::atomic<ReferenceCountedBuffer*> liveSample { nullptr };
    ReferenceCountedBuffer::Ptr          activeSampleBuffer;
    struct RetiredSample { ReferenceCountedBuffer::Ptr buffer; juce::uint64 epoch; };
    std::vector<RetiredSample>           retiredSamples; // message thread only

    // Async notification flags serviced in handleAsyncUpdate()
    std::atomic<bool> sampleLoadPending { false };
    std::atomic<bool> stateRestorePending { false };

    // Message thread -> audio thread: hard-stop every voice at the next block start
    // (preset load). Touching voice DSP state from the message thread races render.
    std::atomic<bool> voiceResetPending { false };
    void resetAllVoicesNow();

    // Voice mode dispatch
    void handleNoteOn(int midiChannel, int midiNote, float velocity);
    void handleNoteOff(int midiChannel, int midiNote, float velocity);
    void handleMonoMidi(const juce::MidiMessage& msg);
    int  lastVoiceMode = -1;
    juce::MidiBuffer emptyMidi; // reused — never allocates (always empty)
    void configureUnisonVoice(SynthVoice* voice, int unisonIdx, int totalUnisonVoices,
                              float detune, float spread);

    // Mono mode state — capacity reserved up front; never grows on the audio thread
    std::vector<int> monoNoteStack;
    float monoVelocity[128] = {};
    int currentMonoNote = -1;
    float monoPitchBendSemitones = 0.0f;

    // Cached voice pointers — populated in prepareToPlay(); avoids repeated dynamic_cast
    // in the audio-thread path. Valid until releaseResources() clears voices.
    std::vector<SynthVoice*> voiceCache;

    // Per-block smoothed master gain prevents clicks when the parameter is automated.
    juce::SmoothedValue<float> masterGainSmoothed;

    // Mod matrix lifetime — owned on message thread, read on audio thread.
    // Retired tables are freed only after the audio thread has completed at least
    // one block since retirement (audioBlockEpoch increments at end of processBlock),
    // which guarantees no in-flight block can still hold the pointer. A plain
    // one-swap deferral is not enough: two recompiles inside a single audio block
    // would free a table the block is still reading.
    std::atomic<ModMatrixTable*> liveModMatrix { nullptr };
    std::atomic<juce::uint64>    audioBlockEpoch { 0 };
    juce::uint32                 nextTableGeneration = 1;
    struct RetiredTable { ModMatrixTable* table; juce::uint64 epoch; };
    std::vector<RetiredTable>    retiredModMatrices; // message thread only

    PresetManager presetManager;
    void loadFactoryPresetsFromBinaryData(); // ctor helper

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(V0idSynthAudioProcessor)
};

#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../DSP/AnalogOscillator.h"
#include "../DSP/SampleOscillator.h"
#include "../DSP/OscMixer.h"
#include "../DSP/DriveStage.h"
#include "../DSP/FilterEnvelope.h"
#include "../DSP/MultiModeFilter.h"
#include "SampleBank.h"
#include "ParamCache.h"
#include "../Modulation/ModulationEngine.h"
#include "../Modulation/ModMatrixTable.h"

class SynthVoice : public juce::SynthesiserVoice
{
public:
    SynthVoice(juce::AudioProcessorValueTreeState& apvts, int voiceIndex);

    bool canPlaySound(juce::SynthesiserSound* sound) override;
    void startNote(int midiNoteNumber, float velocity,
                   juce::SynthesiserSound* sound,
                   int currentPitchWheelPosition) override;
    void stopNote(float velocity, bool allowTailOff) override;
    void pitchWheelMoved(int newPitchWheelValue) override;
    void controllerMoved(int controllerNumber, int newControllerValue) override;
    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                         int startSample, int numSamples) override;
    bool isVoiceActive() const override;
    void setCurrentPlaybackSampleRate(double newRate) override;

    void setSampleBuffer(const ReferenceCountedBuffer* buf);

    // Mod matrix wiring — called per processBlock by PluginProcessor (audio thread).
    void setModMatrix(const ModMatrixTable* table) noexcept { currentModMatrix = table; }

    // Pitch bend in semitones, pushed by the processor in Mono/Unison mode (in Poly
    // mode the Synthesiser routes pitchWheelMoved() per voice instead).
    void setPitchBendSemitones(float st) noexcept { pitchBendSemitones = st; }
    static float pitchWheelToSemitones(int wheel) noexcept
    {
        return (static_cast<float>(wheel) - 8192.0f) / 8192.0f * kPitchBendRange;
    }
    static constexpr float kPitchBendRange = 2.0f; // +/- semitones

    // Unison support — set by voice mode manager
    int unisonIndex = 0;
    float unisonDetuneOffsetCents = 0.0f;
    float unisonPanLeft = 1.0f;
    float unisonPanRight = 1.0f;

private:
    static float midiNoteToHz(int note);
    void updateDrift();

    ParamCache params; // pointers resolved once — no string lookups on the audio thread

    AnalogOscillator analogOsc;
    SampleOscillator sampleOsc;
    OscMixer oscMixer;
    DriveStage driveStage;
    FilterEnvelope filterEnv;
    MultiModeFilter filter;
    juce::ADSR ampEnv;
    ModulationEngine modEngine;
    const ModMatrixTable* currentModMatrix = nullptr;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> frequencySmoothed;
    juce::Random driftRng;
    float driftOffsetCents = 0.0f;
    float pitchBendSemitones = 0.0f;

    float voiceVelocity = 0.0f;
    float baseHz = 440.0f;
    int voiceIdx = 0;
    bool prepared = false;
    bool inRelease = false; // true after stopNote(allowTailOff=true), cleared on startNote/stopNote(false)

public:
    bool isInRelease() const noexcept { return inRelease; }
};

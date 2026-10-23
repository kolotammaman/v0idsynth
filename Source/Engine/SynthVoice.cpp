#include "SynthVoice.h"
#include "SynthSound.h"
#include "../Parameters.h"
#include <cmath>

SynthVoice::SynthVoice(juce::AudioProcessorValueTreeState& apvtsRef, int index)
    : params(apvtsRef), voiceIdx(index)
{
    driftRng.setSeed(static_cast<juce::int64>(index * 31337 + 42));
}

bool SynthVoice::canPlaySound(juce::SynthesiserSound* sound)
{
    return dynamic_cast<SynthSound*>(sound) != nullptr;
}

void SynthVoice::setCurrentPlaybackSampleRate(double newRate)
{
    SynthesiserVoice::setCurrentPlaybackSampleRate(newRate);
    if (newRate > 0.0)
    {
        analogOsc.prepare(newRate);
        sampleOsc.prepare(newRate);
        filter.prepare(newRate, 512);
        filterEnv.prepare(newRate);
        ampEnv.setSampleRate(newRate);
        frequencySmoothed.reset(newRate, 0.0);
        modEngine.prepare(newRate, voiceIdx);  // NEW
        prepared = true;
    }
}

void SynthVoice::startNote(int midiNoteNumber, float velocity,
                            juce::SynthesiserSound*, int currentPitchWheelPosition)
{
    // Poly mode passes the channel's live wheel position; Mono/Unison pass 0 and
    // push the bend through setPitchBendSemitones() instead.
    if (currentPitchWheelPosition > 0)
        pitchBendSemitones = pitchWheelToSemitones(currentPitchWheelPosition);

    inRelease = false;
    voiceVelocity = velocity;
    baseHz = midiNoteToHz(midiNoteNumber);

    const float glideParam = ParamCache::get(params.voiceGlide);
    const int   voiceMode  = (int)ParamCache::get(params.voiceMode);

    // Apply unison detune to target frequency
    float targetHz = baseHz * std::exp2f(unisonDetuneOffsetCents / 1200.0f);

    if (voiceMode == 1 && glideParam > 0.001f)
    {
        // Mono mode portamento: quadratic mapping 0-1 -> ~0-3s
        float glideTimeSec = 0.001f + glideParam * glideParam * 2.999f;
        frequencySmoothed.reset(getSampleRate(), glideTimeSec);
    }
    else
    {
        // 8 ms micro-ramp even without portamento. A hard frequency jump feeds a
        // transient into the resonant filter which rings audibly as a click on
        // mono retrigger. 8 ms is below pitch-glide perception threshold.
        frequencySmoothed.reset(getSampleRate(), 0.008);
    }

    // A freshly-constructed voice sits at 0 Hz — ramping from there would sweep
    // the whole spectrum through the resonant filter on the very first note.
    if (frequencySmoothed.getCurrentValue() <= 0.0f)
        frequencySmoothed.setCurrentAndTargetValue(targetHz);
    else
        frequencySmoothed.setTargetValue(targetHz);

    analogOsc.setWaveform(static_cast<AnalogOscillator::Waveform>(
        (int)ParamCache::get(params.osc1Waveform)));
    // Oscillator and filter are NOT reset here — they continue from current state to
    // avoid a block-boundary discontinuity. stopNote(false) resets them explicitly
    // for hard-stolen voices before the new note starts.
    sampleOsc.resetPosition();

    juce::ADSR::Parameters ampParams;
    ampParams.attack  = ParamCache::get(params.ampA);
    ampParams.decay   = ParamCache::get(params.ampD);
    ampParams.sustain = ParamCache::get(params.ampS);
    ampParams.release = ParamCache::get(params.ampR);
    ampEnv.setParameters(ampParams);
    ampEnv.noteOn();

    filterEnv.setParameters(ParamCache::get(params.filterEnvA),
                            ParamCache::get(params.filterEnvD),
                            ParamCache::get(params.filterEnvS),
                            ParamCache::get(params.filterEnvR));
    filterEnv.noteOn();
    modEngine.startNote();
}

void SynthVoice::stopNote(float, bool allowTailOff)
{
    if (allowTailOff)
    {
        inRelease = true;
        ampEnv.noteOff();
        filterEnv.noteOff();
        modEngine.stopNote();
        // Do NOT call clearCurrentNote() — voice continues rendering through release
    }
    else
    {
        inRelease = false;
        // Reset oscillator and filter to a clean state so the next startNote()
        // inherits silence, not a mid-cycle waveform or charged filter.
        analogOsc.reset();
        filter.reset();
        ampEnv.reset();
        filterEnv.reset();
        clearCurrentNote();
    }
}

bool SynthVoice::isVoiceActive() const
{
    return ampEnv.isActive();
}

void SynthVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                                  int startSample, int numSamples)
{
    if (!isVoiceActive())
        return;

    // --- All parameter reads happen once per block via cached atomics ---
    const float oscMixParam     = ParamCache::get(params.oscMix);
    const float oscDrive        = ParamCache::get(params.osc1Drive);
    const float filterCutoff    = ParamCache::get(params.filterCutoff);
    const float filterRes       = ParamCache::get(params.filterResonance);
    const float filterEnvAmt    = ParamCache::get(params.filterEnvAmount);

    // Sample oscillator tuning + loop — setters early-out when unchanged, so the
    // per-sample setFrequency() is a single multiply.
    sampleOsc.setRootNote((int)ParamCache::get(params.osc2RootNote));
    sampleOsc.setTuneCents(ParamCache::get(params.osc2Tune));
    sampleOsc.setLooping(ParamCache::get(params.osc2Loop) > 0.5f);
    sampleOsc.setSampleGainDb(ParamCache::get(params.osc2Gain));
    sampleOsc.setHPFCutoff(ParamCache::get(params.osc2Hpf));
    sampleOsc.setLPFCutoff(ParamCache::get(params.osc2Lpf));

    for (int i = 0; i < 4; ++i)
    {
        modEngine.setLFORate (i, ParamCache::get(params.lfoRate[i]));
        modEngine.setLFOShape(i, static_cast<VoiceLFO::Shape>((int)ParamCache::get(params.lfoShape[i])));
        modEngine.setLFODepth(i, ParamCache::get(params.lfoDepth[i]));
        modEngine.setEnvDepth(i, ParamCache::get(params.modEnvDepth[i]));
        modEngine.setEnvParameters(i, ParamCache::get(params.modEnvA[i]),
                                      ParamCache::get(params.modEnvD[i]),
                                      ParamCache::get(params.modEnvS[i]),
                                      ParamCache::get(params.modEnvR[i]));
    }

    updateDrift();

    // Filter mode/drive set once per block; cutoff + resonance are set per-sample
    // because they receive per-sample modulation (the filter caches coefficients
    // and only recomputes when they actually move).
    filter.setModeBlend(ParamCache::get(params.filterModeBlend));
    filter.setDrive(ParamCache::get(params.filterDrive));

    // Drift and pitch bend are fixed for the whole block: fold both into one
    // multiplier. Drive normalisation tanh(gain) is likewise per-block.
    modEngine.setDriftSource(driftOffsetCents / 50.0f);
    const float staticPitchMul = std::exp2f(driftOffsetCents / 1200.0f + pitchBendSemitones / 12.0f);
    const DriveStage::Coeffs driveCoeffs = DriveStage::makeCoeffs(oscDrive);

    // Filter envelope: 4 octaves of bipolar sweep (see FilterEnvelope::modulateCutoff)
    const float envOctaves = filterEnvAmt * 4.0f;
    const bool  hasFilterEnv = std::abs(envOctaves) > 1.0e-4f;

    float* outL = outputBuffer.getWritePointer(0, startSample);
    float* outR = outputBuffer.getNumChannels() > 1 ? outputBuffer.getWritePointer(1, startSample) : nullptr;

    for (int i = 0; i < numSamples; ++i)
    {
        const ModOutputs mods = modEngine.process(currentModMatrix);

        float finalHz = frequencySmoothed.getNextValue() * staticPitchMul;
        if (! juce::exactlyEqual(mods.pitchCentsOffset, 0.0f))
            finalHz *= std::exp2f(mods.pitchCentsOffset / 1200.0f);

        analogOsc.setFrequency(finalHz);
        sampleOsc.setFrequency(finalHz);

        const float analogSamp  = analogOsc.processSample();
        const float sampleSamp  = sampleOsc.processSample();
        const float finalOscMix = juce::jlimit(0.0f, 1.0f, oscMixParam + mods.oscMixOffset);
        const float mixed       = oscMixer.mix(analogSamp, sampleSamp, finalOscMix);
        const float driven      = driveStage.processSample(mixed, driveCoeffs);

        // FLT-03 preserved: FilterEnvelope produces its cutoff, mod-matrix ADDS on top.
        const float envVal = filterEnv.getNextSample();
        float fromEnvCutoff = filterCutoff;
        if (hasFilterEnv)
            fromEnvCutoff = juce::jlimit(20.0f, 20000.0f, filterCutoff * std::exp2f(envOctaves * envVal));
        const float finalCutoff = juce::jlimit(20.0f, 20000.0f, fromEnvCutoff + mods.filterCutoffOffset);
        const float finalRes    = juce::jlimit(0.0f, 1.0f, filterRes + mods.filterResOffset);
        filter.setCutoffFrequency(finalCutoff);
        filter.setResonance(finalRes);
        const float filtered = filter.processSample(driven);

        // Amp mod is multiplicative: (1 + offset). Clamped non-negative.
        const float ampMul  = std::max(0.0f, 1.0f + mods.ampGainOffset);
        const float ampGain = ampEnv.getNextSample() * voiceVelocity * ampMul;
        const float output  = filtered * ampGain;

        outL[i] += output * unisonPanLeft;
        if (outR != nullptr)
            outR[i] += output * unisonPanRight;
    }

    if (!ampEnv.isActive())
    {
        clearCurrentNote();
        // An idle voice parks at silence so its next note starts clean.
        filter.reset();
    }
}

void SynthVoice::updateDrift()
{
    float maxCents = ParamCache::get(params.voiceDrift) * 2.0f;
    float step = (driftRng.nextFloat() * 2.0f - 1.0f) * (maxCents * 0.05f);
    driftOffsetCents += step;
    driftOffsetCents = juce::jlimit(-maxCents, maxCents, driftOffsetCents);
}

void SynthVoice::setSampleBuffer(const ReferenceCountedBuffer* buf)
{
    sampleOsc.setBuffer(buf);
}

float SynthVoice::midiNoteToHz(int note)
{
    return 440.0f * std::exp2f(static_cast<float>(note - 69) / 12.0f);
}

void SynthVoice::pitchWheelMoved(int newPitchWheelValue)
{
    pitchBendSemitones = pitchWheelToSemitones(newPitchWheelValue);
}

void SynthVoice::controllerMoved(int, int) {}

#pragma once
#include <juce_audio_basics/juce_audio_basics.h>

// Mod-matrix envelope source for v0idsynth. Distinct from FilterEnvelope (FLT-03)
// which is dedicated to the filter cutoff path with a modulateCutoff() helper.
// ModEnvelope is a generic ADSR that produces values in [0, 1] for use as a
// modulation source routed through the mod matrix by ModulationEngine.
// Four instances live per SynthVoice: ModEnvelope modEnv[4].
class ModEnvelope
{
public:
    void prepare(double sampleRate)
    {
        adsr.setSampleRate(sampleRate);
    }

    void setParameters(float attack, float decay, float sustain, float release)
    {
        adsr.setParameters({ attack, decay, sustain, release });
    }

    void noteOn()  { adsr.noteOn(); }
    void noteOff() { adsr.noteOff(); }
    void reset()   { adsr.reset(); }
    bool isActive() const { return adsr.isActive(); }

    float getNextSample() { return adsr.getNextSample(); }

private:
    juce::ADSR adsr;
};

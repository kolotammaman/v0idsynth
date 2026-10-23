#pragma once
#include <juce_audio_basics/juce_audio_basics.h>

class FilterEnvelope
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

    // Compute modulated cutoff frequency.
    // envAmount: bipolar -1 to +1, envValue: 0 to 1 from ADSR
    // Uses exponential scaling: +/- 4 octaves max modulation
    static float modulateCutoff(float baseCutoffHz, float envAmount, float envValue)
    {
        constexpr float octaveRange = 4.0f;
        float ratio = std::exp2f(envAmount * envValue * octaveRange);
        float modulated = baseCutoffHz * ratio;
        return juce::jlimit(20.0f, 20000.0f, modulated);
    }

private:
    juce::ADSR adsr;
};

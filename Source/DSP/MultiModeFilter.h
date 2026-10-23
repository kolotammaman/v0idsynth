#pragma once
#include <juce_dsp/juce_dsp.h>

class MultiModeFilter
{
public:
    void prepare(double sampleRate, int samplesPerBlock);
    void reset();

    void setCutoffFrequency(float hz);
    void setResonance(float res);   // 0-1
    void setModeBlend(float blend); // 0=LP, 1=HP, 2=BP, 3=Notch (continuous)
    void setDrive(float drive);     // 0-1 (LadderFilter saturation)

    float processSample(float input);

private:
    // Exposes LadderFilter's per-sample path so we can skip the AudioBlock /
    // ProcessContext construction that the public block API needs per call.
    struct Ladder : juce::dsp::LadderFilter<float>
    {
        using juce::dsp::LadderFilter<float>::processSample;
        using juce::dsp::LadderFilter<float>::updateSmoothers;
    };

    void updateCoefficients() noexcept;

    // Single TPT state-variable filter (Zavalishin topology, same maths as
    // juce::dsp::StateVariableTPTFilter). One tick yields LP, BP and HP together,
    // so all four modes cost one filter instead of three.
    float s1 = 0.0f, s2 = 0.0f;
    float g = 0.0f, k = 2.0f, h = 1.0f;
    bool  coeffsDirty = true;

    Ladder ladderFilter;
    bool   ladderDirty = true;

    float modeBlend = 0.0f;
    float driveAmount = 0.0f;
    float currentCutoff = 20000.0f;
    float currentResonance = 0.0f;
    double sampleRate = 44100.0;
};

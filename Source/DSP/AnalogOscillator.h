#pragma once
#include <juce_core/juce_core.h>
#include <cmath>

class AnalogOscillator
{
public:
    enum class Waveform { Saw = 0, Square, Triangle, Sine };

    void prepare(double sampleRate);
    void setFrequency(float hz);
    void setWaveform(Waveform w);
    float processSample();
    void reset();

private:
    float polyBlep(float t) const;

    float phase = 0.0f;
    float phaseIncrement = 0.0f;
    double sampleRate = 44100.0;
    Waveform waveform = Waveform::Saw;
    float triangleAccumulator = 0.0f;
};

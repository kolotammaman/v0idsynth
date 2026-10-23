#include "AnalogOscillator.h"

void AnalogOscillator::prepare(double sr)
{
    sampleRate = sr;
    reset();
}

void AnalogOscillator::setFrequency(float hz)
{
    // Clamp below Nyquist: heavy pitch modulation on a high note could otherwise
    // push the increment past 1.0 and the single-step phase wrap would escape [0,1).
    phaseIncrement = juce::jlimit(0.0f, 0.5f, hz / static_cast<float>(sampleRate));
}

void AnalogOscillator::setWaveform(Waveform w)
{
    waveform = w;
}

void AnalogOscillator::reset()
{
    phase = 0.0f;
    triangleAccumulator = 0.0f;
}

float AnalogOscillator::processSample()
{
    float output = 0.0f;

    switch (waveform)
    {
        case Waveform::Saw:
        {
            output = 2.0f * phase - 1.0f;
            output -= polyBlep(phase);
            break;
        }

        case Waveform::Square:
        {
            output = (phase < 0.5f) ? 1.0f : -1.0f;
            output += polyBlep(phase);
            output -= polyBlep(phase < 0.5f ? phase + 0.5f : phase - 0.5f);
            break;
        }

        case Waveform::Triangle:
        {
            // Bandlimited triangle via leaky integration of PolyBLEP square
            float sq = (phase < 0.5f) ? 1.0f : -1.0f;
            sq += polyBlep(phase);
            sq -= polyBlep(phase < 0.5f ? phase + 0.5f : phase - 0.5f);
            triangleAccumulator = triangleAccumulator * 0.9999f + 4.0f * phaseIncrement * sq;
            output = juce::jlimit(-1.0f, 1.0f, triangleAccumulator);
            break;
        }

        case Waveform::Sine:
        {
            output = std::sin(juce::MathConstants<float>::twoPi * phase);
            break;
        }
    }

    phase += phaseIncrement;
    if (phase >= 1.0f)
        phase -= 1.0f;

    return output;
}

float AnalogOscillator::polyBlep(float t) const
{
    float dt = phaseIncrement;
    if (t < dt)
    {
        t /= dt;
        return t + t - t * t - 1.0f;
    }
    else if (t > 1.0f - dt)
    {
        t = (t - 1.0f) / dt;
        return t * t + t + t + 1.0f;
    }
    return 0.0f;
}

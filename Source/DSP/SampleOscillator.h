#pragma once
#include "../Engine/SampleBank.h"
#include <cmath>

class SampleOscillator
{
public:
    void prepare(double sr)
    {
        sampleRate = sr;
        hpfHz = lpfHz = -1.0f; // force coefficient refresh at the new rate
        updateRateScale();
    }

    // Raw pointer: the audio thread never touches the refcount. Lifetime is
    // guaranteed by the processor, which retires buffers only after the audio
    // thread has finished a block without them.
    void setBuffer(const ReferenceCountedBuffer* buf)
    {
        if (buf == currentBuffer)
            return;
        currentBuffer = buf;
        samplePosition = 0.0;
        playbackRate = 1.0;
        hpfState = 0.0f;
        lpfState = 0.0f;
        updateRateScale();
    }

    void setRootNote(int midiNote)
    {
        if (midiNote != rootNote) { rootNote = midiNote; updateRateScale(); }
    }
    void setTuneCents(float cents)
    {
        if (! juce::exactlyEqual(cents, tuneCents)) { tuneCents = cents; updateRateScale(); }
    }
    void setLooping(bool shouldLoop) { looping = shouldLoop; }

    void setSampleGainDb(float db)
    {
        if (! juce::exactlyEqual(db, sampleGainDb))
        {
            sampleGainDb = db;
            sampleGain = juce::Decibels::decibelsToGain(db);
        }
    }

    void setHPFCutoff(float hz)
    {
        if (! juce::exactlyEqual(hz, hpfHz))
        {
            hpfHz = hz;
            hpfCoeff = 1.0f - std::exp(-juce::MathConstants<float>::twoPi * hz / (float)sampleRate);
        }
    }

    void setLPFCutoff(float hz)
    {
        if (! juce::exactlyEqual(hz, lpfHz))
        {
            lpfHz = hz;
            lpfCoeff = 1.0f - std::exp(-juce::MathConstants<float>::twoPi * hz / (float)sampleRate);
        }
    }

    // Per-sample: one multiply. Root/tune/sample-rate maths is folded into
    // rateScale whenever one of those changes.
    void setFrequency(float hz)
    {
        playbackRate = (currentBuffer != nullptr) ? (double)hz * rateScale : 1.0;
    }

    float processSample()
    {
        if (currentBuffer == nullptr)
            return 0.0f;

        const int numSamples = currentBuffer->buffer.getNumSamples();
        if (numSamples <= 0)
            return 0.0f;

        if (looping)
        {
            if (samplePosition >= (double)numSamples)
                samplePosition = std::fmod(samplePosition, (double)numSamples);
        }
        else if (samplePosition >= (double)numSamples)
        {
            return 0.0f;
        }

        const float* data = currentBuffer->buffer.getReadPointer(0);
        const int i0 = (int)samplePosition;
        // Looping interpolates across the seam instead of holding the last sample.
        const int i1 = (i0 + 1 < numSamples) ? i0 + 1 : (looping ? 0 : numSamples - 1);
        const float frac = (float)(samplePosition - (double)i0);
        float out = data[i0] + frac * (data[i1] - data[i0]);

        // 1-pole HPF: state tracks the LP component; subtract it to get HP
        hpfState += hpfCoeff * (out - hpfState);
        out -= hpfState;

        // 1-pole LPF
        lpfState += lpfCoeff * (out - lpfState);
        out = lpfState;

        samplePosition += playbackRate;
        return out * sampleGain;
    }

    void resetPosition()
    {
        samplePosition = 0.0;
        hpfState = 0.0f;
        lpfState = 0.0f;
    }

    void reset()
    {
        samplePosition = 0.0;
        currentBuffer = nullptr;
        playbackRate = 1.0;
        hpfState = 0.0f;
        lpfState = 0.0f;
    }

    bool isFinished() const
    {
        if (currentBuffer == nullptr) return true;
        if (looping) return false;
        return samplePosition >= (double)currentBuffer->buffer.getNumSamples();
    }

    static float midiNoteToHz(int note)
    {
        return 440.0f * std::exp2f(static_cast<float>(note - 69) / 12.0f);
    }

private:
    void updateRateScale()
    {
        // Root note sets coarse pitch; tune adjusts by cents (1200 cents = 1 octave).
        const double rootHz = (double)midiNoteToHz(rootNote) * std::exp2((double)tuneCents / 1200.0);
        // A file recorded at 48 kHz must step faster than 1:1 in a 44.1 kHz session
        // to keep its pitch; buffers with no recorded rate are assumed to match.
        const double srcRate = (currentBuffer != nullptr && currentBuffer->sourceSampleRate > 0.0)
                                   ? currentBuffer->sourceSampleRate : sampleRate;
        rateScale = (rootHz > 0.0 && sampleRate > 0.0) ? (srcRate / sampleRate) / rootHz : 0.0;
    }

    const ReferenceCountedBuffer* currentBuffer = nullptr;
    double sampleRate     = 44100.0;
    // Double precision: a float position loses sub-sample resolution after a few
    // seconds (ulp 1/64 at 2^17), which quantises the step and detunes playback.
    double samplePosition = 0.0;
    double playbackRate   = 1.0;
    double rateScale      = 0.0;
    int    rootNote       = 60;
    float  tuneCents      = 0.0f;
    bool   looping        = false;
    float  sampleGainDb   = 0.0f;
    float  sampleGain     = 1.0f;
    float  hpfHz          = -1.0f;
    float  hpfState       = 0.0f;
    float  hpfCoeff       = 0.0f;   // 0 = DC-only removal (near bypass at 20 Hz default)
    float  lpfHz          = -1.0f;
    float  lpfState       = 0.0f;
    float  lpfCoeff       = 1.0f;   // 1 = pass-through (computed once sampleRate is set)
};

#include "MultiModeFilter.h"

void MultiModeFilter::prepare(double sr, int samplesPerBlock)
{
    sampleRate = sr;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sr;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 1;

    ladderFilter.prepare(spec);
    ladderFilter.setMode(juce::dsp::LadderFilterMode::LPF24);

    coeffsDirty = true;
    ladderDirty = true;
    reset();
}

void MultiModeFilter::reset()
{
    s1 = s2 = 0.0f;
    ladderFilter.reset();
}

void MultiModeFilter::setCutoffFrequency(float hz)
{
    // Keep below Nyquist so tan() stays finite at low host sample rates.
    const float limited = juce::jlimit(20.0f, juce::jmin(20000.0f, (float)sampleRate * 0.49f), hz);
    if (! juce::exactlyEqual(limited, currentCutoff))
    {
        currentCutoff = limited;
        coeffsDirty = ladderDirty = true;
    }
}

void MultiModeFilter::setResonance(float res)
{
    if (! juce::exactlyEqual(res, currentResonance))
    {
        currentResonance = res;
        coeffsDirty = ladderDirty = true;
    }
}

void MultiModeFilter::setModeBlend(float blend)
{
    modeBlend = juce::jlimit(0.0f, 3.0f, blend);
}

void MultiModeFilter::setDrive(float drive)
{
    if (! juce::exactlyEqual(drive, driveAmount))
    {
        driveAmount = drive;
        // Map 0-1 to 1-10x drive range for the LadderFilter
        ladderFilter.setDrive(1.0f + drive * 9.0f);
    }
}

void MultiModeFilter::updateCoefficients() noexcept
{
    // Map 0-1 to Q range — Q=0.5 is flat, Q=10 is very resonant
    const float q = 1.0f / (2.0f * (1.0f - currentResonance * 0.95f));
    g = std::tan(juce::MathConstants<float>::pi * currentCutoff / (float)sampleRate);
    k = 1.0f / q;
    h = 1.0f / (1.0f + k * g + g * g);
    coeffsDirty = false;
}

float MultiModeFilter::processSample(float input)
{
    // Coefficients only recompute when cutoff/resonance actually moved — static
    // patches pay for one tan() per change instead of three per sample.
    if (coeffsDirty)
        updateCoefficients();

    const float hpOut = h * (input - s1 * (g + k) - s2);
    const float bpOut = hpOut * g + s1;
    s1 = hpOut * g + bpOut;
    const float lpOut = bpOut * g + s2;
    s2 = bpOut * g + lpOut;

    // True notch: input minus the unity-gain band (== LP + HP), so the null at
    // the cutoff is complete regardless of resonance.
    const float notchOut = input - k * bpOut;

    // Crossfade between adjacent modes: 0=LP, 1=HP, 2=BP, 3=Notch
    const float blendClamped = juce::jlimit(0.0f, 2.999f, modeBlend);
    const int   modeA = static_cast<int>(blendClamped);
    const float frac  = blendClamped - static_cast<float>(modeA);
    const float outputs[4] = { lpOut, hpOut, bpOut, notchOut };
    const float svfOutput = outputs[modeA] + (outputs[modeA + 1] - outputs[modeA]) * frac;

    if (driveAmount > 0.001f)
    {
        // Ladder is only fed new cutoff/resonance while it is audible — its
        // setCutoffFrequencyHz() costs an exp() per call.
        if (ladderDirty)
        {
            ladderFilter.setCutoffFrequencyHz(currentCutoff);
            // Ladder resonance: 0-1, cap at 0.7 to avoid self-oscillation runaway
            ladderFilter.setResonance(currentResonance * 0.7f);
            ladderDirty = false;
        }
        ladderFilter.updateSmoothers();
        const float ladderOut = ladderFilter.processSample(input, 0);

        return svfOutput * (1.0f - driveAmount * 0.5f) + ladderOut * (driveAmount * 0.5f);
    }

    return svfOutput;
}

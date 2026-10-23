#pragma once
#include <cmath>

class DriveStage
{
public:
    // Per-block constants — tanh(gain) depends only on the drive knob, so it is
    // computed once per block instead of once per sample.
    struct Coeffs
    {
        float gain = 1.0f;
        float norm = 1.0f;   // 1 / tanh(gain): output level compensation
        bool  bypass = true;
    };

    // driveAmount: 0.0 = clean, 1.0 = full saturation
    static Coeffs makeCoeffs(float driveAmount)
    {
        Coeffs c;
        if (driveAmount < 0.001f)
            return c;
        // Scale input by drive gain (1x to 5x), then soft-clip with tanh
        c.gain = 1.0f + driveAmount * 4.0f;
        c.norm = 1.0f / std::tanh(c.gain);
        c.bypass = false;
        return c;
    }

    float processSample(float input, const Coeffs& c) const
    {
        if (c.bypass)
            return input;
        return std::tanh(input * c.gain) * c.norm;
    }

    // Convenience overload (tests / one-off use) — computes coefficients inline.
    float processSample(float input, float driveAmount) const
    {
        return processSample(input, makeCoeffs(driveAmount));
    }
};

#pragma once

class OscMixer
{
public:
    float mix(float analogSample, float sampleSample, float mixAmount) const
    {
        return (1.0f - mixAmount) * analogSample + mixAmount * sampleSample;
    }
};

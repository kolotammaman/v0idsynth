#pragma once
#include <juce_audio_formats/juce_audio_formats.h>
#include <functional>
#include "SampleBank.h"

class SampleLoader : public juce::Thread
{
public:
    SampleLoader(SampleBank& bank, const juce::File& fileToLoad, std::function<void()> onComplete = {})
        : juce::Thread("SampleLoader"), sampleBank(bank), file(fileToLoad),
          completionCallback(std::move(onComplete)) {}

    void run() override;

private:
    SampleBank& sampleBank;
    juce::File file;
    std::function<void()> completionCallback;
};

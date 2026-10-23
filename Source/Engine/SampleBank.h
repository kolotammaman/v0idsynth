#pragma once
#include <juce_audio_formats/juce_audio_formats.h>
#include <functional>
#include <memory>

class SampleLoader; // forward declaration

class ReferenceCountedBuffer : public juce::ReferenceCountedObject
{
public:
    using Ptr = juce::ReferenceCountedObjectPtr<ReferenceCountedBuffer>;

    ReferenceCountedBuffer(int numChannels, int numSamples)
        : buffer(numChannels, numSamples)
    {
        buffer.clear();
    }

    juce::AudioBuffer<float> buffer;
    int rootMidiNote = 60;
    double sourceSampleRate = 0.0; // rate the file was recorded at; 0 = unknown (treat as host rate)
    juce::File sourceFile;         // where it came from — persisted in plugin state
};

class SampleBank
{
public:
    SampleBank();
    ~SampleBank();

    ReferenceCountedBuffer::Ptr getCurrentBuffer() const;
    void setBuffer(ReferenceCountedBuffer::Ptr newBuffer);
    bool loadFromFile(const juce::File& file);
    void loadAsync(const juce::File& file, std::function<void()> onComplete = {});

    juce::AudioFormatManager& getFormatManager() { return formatManager; }

private:
    mutable juce::SpinLock mutex;
    ReferenceCountedBuffer::Ptr currentBuffer;
    juce::AudioFormatManager formatManager;
    std::unique_ptr<SampleLoader> activeLoader;
};

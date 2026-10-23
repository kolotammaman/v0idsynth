#include "SampleBank.h"
#include "SampleLoader.h"

SampleBank::SampleBank()
{
    // registerBasicFormats() already includes CoreAudioFormat on macOS (mp3/m4a/
    // aac/caf); registering it a second time tripped a debug assertion.
    formatManager.registerBasicFormats();
}

SampleBank::~SampleBank()
{
    if (activeLoader != nullptr)
        activeLoader->stopThread(2000);
}

ReferenceCountedBuffer::Ptr SampleBank::getCurrentBuffer() const
{
    // Never called from the audio thread, so a blocking lock is fine — the old
    // try-lock returned nullptr under contention and silently dropped the sample.
    juce::SpinLock::ScopedLockType lock(mutex);
    return currentBuffer;
}

void SampleBank::setBuffer(ReferenceCountedBuffer::Ptr newBuffer)
{
    juce::SpinLock::ScopedLockType lock(mutex);
    currentBuffer = newBuffer;
}

bool SampleBank::loadFromFile(const juce::File& file)
{
    auto* reader = formatManager.createReaderFor(file);
    if (reader == nullptr)
        return false;

    std::unique_ptr<juce::AudioFormatReader> readerOwner(reader);

    // Cap at ~10 minutes @ 48 kHz so a mis-dropped long file can't exhaust memory.
    constexpr juce::int64 kMaxSamples = 48000 * 600;
    if (reader->lengthInSamples <= 0 || reader->lengthInSamples > kMaxSamples)
        return false;

    int numSamples = (int)reader->lengthInSamples;
    ReferenceCountedBuffer::Ptr newBuf(new ReferenceCountedBuffer(1, numSamples));

    if (reader->numChannels == 1)
    {
        reader->read(&newBuf->buffer, 0, numSamples, 0, true, false);
    }
    else
    {
        juce::AudioBuffer<float> tempBuf(2, numSamples);
        reader->read(&tempBuf, 0, numSamples, 0, true, true);

        auto* mono = newBuf->buffer.getWritePointer(0);
        const auto* left = tempBuf.getReadPointer(0);
        const auto* right = tempBuf.getReadPointer(1);
        for (int i = 0; i < numSamples; ++i)
            mono[i] = (left[i] + right[i]) * 0.5f;
    }

    newBuf->rootMidiNote = 60;
    newBuf->sourceSampleRate = reader->sampleRate;
    newBuf->sourceFile = file;
    setBuffer(newBuf);
    return true;
}

void SampleBank::loadAsync(const juce::File& file, std::function<void()> onComplete)
{
    if (activeLoader != nullptr)
        activeLoader->stopThread(2000);

    activeLoader = std::make_unique<SampleLoader>(*this, file, std::move(onComplete));
    activeLoader->startThread();
}

#include "SampleLoader.h"

void SampleLoader::run()
{
    if (threadShouldExit())
        return;

    sampleBank.loadFromFile(file);

    // Fire on failure too, so the UI can drop its "loading" state; the receiver
    // compares against the bank's current buffer to see whether anything changed.
    if (completionCallback)
        completionCallback();
}

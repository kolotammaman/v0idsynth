#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Engine/SynthVoice.h"
#include "Engine/SynthSound.h"
#include "Engine/SampleBank.h"
#include <BinaryData.h>

juce::SynthesiserVoice* V0idSynthesiser::findFreeVoice (juce::SynthesiserSound* sound,
                                                         int ch, int note,
                                                         bool stealIfNecessary) const
{
    // Poly mode only calls this right before starting the returned voice. A voice
    // last used by Unison still carries its detune/pan offsets — clear them here,
    // since Synthesiser::startVoice() is not virtual.
    auto* voice = findVoiceToStart (sound, stealIfNecessary);
    if (auto* sv = static_cast<SynthVoice*> (voice))
    {
        sv->unisonDetuneOffsetCents = 0.0f;
        sv->unisonPanLeft  = 1.0f;
        sv->unisonPanRight = 1.0f;
    }
    juce::ignoreUnused (ch, note);
    return voice;
}

juce::SynthesiserVoice* V0idSynthesiser::findVoiceToStart (juce::SynthesiserSound* sound,
                                                            bool stealIfNecessary) const
{
    // Pass 1: idle voices (amp envelope fully finished — no audible interruption)
    for (auto* voice : voices)
        if (!voice->isVoiceActive() && voice->canPlaySound(sound))
            return voice;

    if (!stealIfNecessary) return nullptr;

    // Pass 2: steal the oldest releasing voice — already fading to silence.
    // Preferred over sustaining voices because the user perceives less disruption.
    juce::SynthesiserVoice* releasing = nullptr;
    for (auto* voice : voices)
    {
        if (voice->canPlaySound(sound) && static_cast<SynthVoice*>(voice)->isInRelease())
        {
            if (releasing == nullptr || voice->wasStartedBefore(*releasing))
                releasing = voice;
        }
    }
    if (releasing != nullptr) return releasing;

    // Pass 3: fall back to oldest active voice
    juce::SynthesiserVoice* oldest = nullptr;
    for (auto* voice : voices)
    {
        if (voice->canPlaySound(sound))
        {
            if (oldest == nullptr || voice->wasStartedBefore(*oldest))
                oldest = voice;
        }
    }
    return oldest;
}

V0idSynthAudioProcessor::V0idSynthAudioProcessor()
    : AudioProcessor(BusesProperties()
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, juce::Identifier("v0idSynthParams"), createParameterLayout())
{
    monoNoteStack.reserve(128);

    // Ensure liveModMatrix is non-null from construction onward.
    recompileModMatrix();

    loadFactoryPresetsFromBinaryData();
}

V0idSynthAudioProcessor::~V0idSynthAudioProcessor()
{
    cancelPendingUpdate();

    // Delete the currently-live table and any retired ones still awaiting an epoch.
    // Safe here because audio thread is guaranteed stopped by host before destruction.
    delete liveModMatrix.load(std::memory_order_acquire);
    for (auto& retired : retiredModMatrices)
        delete retired.table;
}

const juce::String V0idSynthAudioProcessor::getName() const
{
#ifdef JucePlugin_Name
    return JucePlugin_Name;
#else
    return "v0idsynth";
#endif
}
bool V0idSynthAudioProcessor::acceptsMidi() const { return true; }
bool V0idSynthAudioProcessor::producesMidi() const { return false; }
bool V0idSynthAudioProcessor::isMidiEffect() const { return false; }
double V0idSynthAudioProcessor::getTailLengthSeconds() const { return 2.0; }

int V0idSynthAudioProcessor::getNumPrograms() { return 1; }
int V0idSynthAudioProcessor::getCurrentProgram() { return currentProgram; }
void V0idSynthAudioProcessor::setCurrentProgram(int index) { currentProgram = index; }
const juce::String V0idSynthAudioProcessor::getProgramName(int) { return {}; }
void V0idSynthAudioProcessor::changeProgramName(int, const juce::String&) {}

void V0idSynthAudioProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    synthesiser.setCurrentPlaybackSampleRate(sampleRate);
    synthesiser.clearVoices();
    synthesiser.clearSounds();
    synthesiser.addSound(new SynthSound());
    for (int i = 0; i < 16; ++i)
        synthesiser.addVoice(new SynthVoice(apvts, i));

    voiceCache.clear();
    for (int i = 0; i < synthesiser.getNumVoices(); ++i)
        voiceCache.push_back(static_cast<SynthVoice*>(synthesiser.getVoice(i)));

    masterGainSmoothed.reset(sampleRate, 0.02);
    const float initGainDb = ParamCache::get(params.masterGain);
    masterGainSmoothed.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(initGainDb));

    // Audio is stopped here, so voices can take the live sample directly.
    auto* sample = liveSample.load();
    for (auto* voice : voiceCache)
        voice->setSampleBuffer(sample);

    monoNoteStack.clear();
    currentMonoNote = -1;
    monoPitchBendSemitones = 0.0f;
    lastVoiceMode = -1;
    voiceResetPending.store(false);
}

void V0idSynthAudioProcessor::releaseResources()
{
    synthesiser.clearVoices();
    synthesiser.clearSounds();
    voiceCache.clear();
}

void V0idSynthAudioProcessor::notifySampleLoaded()
{
    ReferenceCountedBuffer::Ptr buf = sampleBank.getCurrentBuffer();
    if (buf == activeSampleBuffer)
        return;

    // Publish; voices pick the pointer up at the start of the next block.
    liveSample.store(buf.get());

    // Free retired buffers once a full block has completed since retirement —
    // no in-flight block can still be reading them (see recompileModMatrix).
    const auto now = audioBlockEpoch.load();
    retiredSamples.erase(std::remove_if(retiredSamples.begin(), retiredSamples.end(),
                                        [now](const RetiredSample& r) { return now > r.epoch; }),
                         retiredSamples.end());
    if (activeSampleBuffer != nullptr)
        retiredSamples.push_back({ activeSampleBuffer, now });
    activeSampleBuffer = buf;
}

void V0idSynthAudioProcessor::loadSampleAsync(const juce::File& file)
{
    // Completion runs on the loader thread; AsyncUpdater hops to the message
    // thread and is cancelled automatically if the processor is destroyed first
    // (a captured-reference callAsync would dangle).
    sampleBank.loadAsync(file, [this]()
    {
        sampleLoadPending.store(true);
        triggerAsyncUpdate();
    });
}

juce::String V0idSynthAudioProcessor::getLoadedSampleName() const
{
    return activeSampleBuffer != nullptr ? activeSampleBuffer->sourceFile.getFileNameWithoutExtension()
                                         : juce::String();
}

void V0idSynthAudioProcessor::handleAsyncUpdate()
{
    if (sampleLoadPending.exchange(false))
    {
        notifySampleLoaded();
        if (onSampleChanged)
            onSampleChanged();
    }
    if (stateRestorePending.exchange(false) && onPresetLoaded)
        onPresetLoaded();
}

void V0idSynthAudioProcessor::resetAllVoicesNow()
{
    // AUDIO THREAD (or with audio stopped).
    for (auto* v : voiceCache)
    {
        v->stopNote(0.0f, false);
        v->unisonDetuneOffsetCents = 0.0f;
        v->unisonPanLeft  = 1.0f;
        v->unisonPanRight = 1.0f;
    }
    monoNoteStack.clear();
    currentMonoNote = -1;
}

bool V0idSynthAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // This is a synth — no audio input, stereo output only
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::disabled())
        return false;

    return true;
}

void V0idSynthAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                            juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    const int numSamples = buffer.getNumSamples();

    // === Mod matrix + sample handoff: load once, push to all voices before rendering. ===
    // Both are lock-free pointer loads; the message thread cannot free either
    // until this block has bumped audioBlockEpoch below.
    const ModMatrixTable* matrix = liveModMatrix.load();
    const ReferenceCountedBuffer* sample = liveSample.load();
    for (auto* v : voiceCache)
    {
        v->setModMatrix(matrix);
        v->setSampleBuffer(sample); // no-op unless the pointer changed
    }

    const int voiceMode = (int)ParamCache::get(params.voiceMode);

    // Preset load or a voice-mode switch: voices started by the Mono/Unison
    // dispatcher are invisible to juce::Synthesiser's note tracking (and carry
    // unison detune/pan), so they would hang or play detuned in another mode.
    const bool modeChanged = (lastVoiceMode >= 0 && voiceMode != lastVoiceMode);
    if (voiceResetPending.exchange(false))
    {
        resetAllVoicesNow();
    }
    else if (modeChanged)
    {
        // Tail off rather than hard-cut; the next mode assigns fresh detune/pan
        // to any voice it starts (findFreeVoice clears them for Poly).
        for (auto* v : voiceCache)
            if (v->isVoiceActive() && ! v->isInRelease())
                v->stopNote(0.0f, true);
        monoNoteStack.clear();
        currentMonoNote = -1;
    }
    lastVoiceMode = voiceMode;

    if (voiceMode == 0) // Poly — standard Synthesiser behavior
    {
        synthesiser.renderNextBlock(buffer, midiMessages, 0, numSamples);
    }
    else // Mono (1) or Unison (2) — render in segments so notes land sample-accurately
    {
        int pos = 0;
        for (const auto metadata : midiMessages)
        {
            const int eventPos = juce::jlimit(0, numSamples, metadata.samplePosition);
            if (eventPos > pos)
            {
                synthesiser.renderNextBlock(buffer, emptyMidi, pos, eventPos - pos);
                pos = eventPos;
            }
            handleMonoMidi(metadata.getMessage());
        }
        if (pos < numSamples)
            synthesiser.renderNextBlock(buffer, emptyMidi, pos, numSamples - pos);
    }

    masterGainSmoothed.setTargetValue(juce::Decibels::decibelsToGain(ParamCache::get(params.masterGain)));
    if (masterGainSmoothed.isSmoothing())
    {
        for (int i = 0; i < numSamples; ++i)
        {
            float g = masterGainSmoothed.getNextValue();
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                buffer.getWritePointer(ch)[i] *= g;
        }
    }
    else
    {
        buffer.applyGain(masterGainSmoothed.getCurrentValue());
    }

    // Block complete — the pointers loaded above are no longer in use.
    audioBlockEpoch.fetch_add(1);
}

void V0idSynthAudioProcessor::handleMonoMidi(const juce::MidiMessage& msg)
{
    if (msg.isNoteOn())
        handleNoteOn(msg.getChannel(), msg.getNoteNumber(), msg.getFloatVelocity());
    else if (msg.isNoteOff())
        handleNoteOff(msg.getChannel(), msg.getNoteNumber(), msg.getFloatVelocity());
    else if (msg.isPitchWheel())
    {
        monoPitchBendSemitones = SynthVoice::pitchWheelToSemitones(msg.getPitchWheelValue());
        for (auto* v : voiceCache)
            v->setPitchBendSemitones(monoPitchBendSemitones);
    }
    else if (msg.isAllNotesOff() || msg.isAllSoundOff())
    {
        // Host panic / transport stop — previously ignored outside Poly mode.
        monoNoteStack.clear();
        currentMonoNote = -1;
        for (auto* v : voiceCache)
            if (v->isVoiceActive())
                v->stopNote(0.0f, ! msg.isAllSoundOff());
    }
}

void V0idSynthAudioProcessor::handleNoteOn(int /*midiChannel*/, int midiNote, float velocity)
{
    const int voiceMode = (int)ParamCache::get(params.voiceMode);
    auto* sound = synthesiser.getSound(0).get();

    if (voiceMode == 1) // Mono — last-note priority
    {
        // Re-pressing a held note moves it to the top instead of duplicating it,
        // and the stack never exceeds its reserved capacity (no audio-thread alloc).
        monoNoteStack.erase(std::remove(monoNoteStack.begin(), monoNoteStack.end(), midiNote),
                            monoNoteStack.end());
        if (monoNoteStack.size() < monoNoteStack.capacity())
            monoNoteStack.push_back(midiNote);
        monoVelocity[midiNote & 127] = velocity;

        SynthVoice* monoVoice = nullptr;
        for (auto* v : voiceCache)
        {
            if (v->isVoiceActive())
            {
                monoVoice = v;
                break;
            }
        }
        if (monoVoice == nullptr && !voiceCache.empty())
            monoVoice = voiceCache[0];

        if (monoVoice != nullptr)
        {
            monoVoice->unisonDetuneOffsetCents = 0.0f;
            monoVoice->unisonPanLeft = 1.0f;
            monoVoice->unisonPanRight = 1.0f;
            monoVoice->startNote(midiNote, velocity, sound, 0);
        }
        currentMonoNote = midiNote;
    }
    else if (voiceMode == 2) // Unison
    {
        const int   unisonVoices = (int)ParamCache::get(params.unisonVoices);
        const float detune       = ParamCache::get(params.unisonDetune);
        const float spread       = ParamCache::get(params.unisonSpread);

        // Trigger gentle release on any voices that are currently sounding (not already
        // in release). This avoids a hard block-boundary cut — they fade out naturally
        // while the new note starts on fresh voices.
        for (auto* v : voiceCache)
            if (v->isVoiceActive() && !v->isInRelease())
                v->stopNote(0.0f, true); // tail-off release — no hard click

        int voicesAssigned = 0;
        SynthVoice* assignedVoices[16] = {};

        // Pass 1: idle voices (amp envelope fully done — cleanest, no click)
        for (auto* v : voiceCache)
        {
            if (voicesAssigned >= unisonVoices) break;
            if (!v->isVoiceActive())
            {
                configureUnisonVoice(v, voicesAssigned, unisonVoices, detune, spread);
                v->startNote(midiNote, velocity, sound, 0);
                assignedVoices[voicesAssigned++] = v;
            }
        }

        // Pass 2: voices in release (fading to silence — retrigger from current level,
        // no oscillator phase reset so there is no output discontinuity)
        for (auto* v : voiceCache)
        {
            if (voicesAssigned >= unisonVoices) break;
            if (!v->isVoiceActive() || !v->isInRelease()) continue;
            bool alreadyAssigned = false;
            for (int j = 0; j < voicesAssigned; ++j)
                if (assignedVoices[j] == v) { alreadyAssigned = true; break; }
            if (alreadyAssigned) continue;

            // No stopNote first — startNote() retriggers the envelope from its current
            // release level, and the oscillator continues from its current phase.
            configureUnisonVoice(v, voicesAssigned, unisonVoices, detune, spread);
            v->startNote(midiNote, velocity, sound, 0);
            assignedVoices[voicesAssigned++] = v;
        }

        // Pass 3: hard steal any remaining active voice (last resort — rapid playing)
        for (auto* v : voiceCache)
        {
            if (voicesAssigned >= unisonVoices) break;
            bool alreadyAssigned = false;
            for (int j = 0; j < voicesAssigned; ++j)
                if (assignedVoices[j] == v) { alreadyAssigned = true; break; }
            if (alreadyAssigned) continue;

            v->stopNote(0.0f, false); // hard reset — osc and filter cleared in stopNote
            configureUnisonVoice(v, voicesAssigned, unisonVoices, detune, spread);
            v->startNote(midiNote, velocity, sound, 0);
            voicesAssigned++;
        }
    }
}

void V0idSynthAudioProcessor::handleNoteOff(int /*midiChannel*/, int midiNote, float /*velocity*/)
{
    const int voiceMode = (int)ParamCache::get(params.voiceMode);
    auto* sound = synthesiser.getSound(0).get();

    if (voiceMode == 1) // Mono
    {
        auto it = std::find(monoNoteStack.begin(), monoNoteStack.end(), midiNote);
        if (it == monoNoteStack.end())
            return; // not held (e.g. stack cleared by a mode switch) — nothing to do
        const bool wasSounding = (midiNote == monoNoteStack.back());
        monoNoteStack.erase(it);
        if (! wasSounding)
            return; // releasing a buried note must not retrigger the sounding one

        if (monoNoteStack.empty())
        {
            for (auto* v : voiceCache)
                if (v->isVoiceActive())
                    v->stopNote(0.0f, true);
            currentMonoNote = -1;
        }
        else
        {
            int lastNote = monoNoteStack.back();
            for (auto* v : voiceCache)
            {
                if (v->isVoiceActive())
                {
                    v->startNote(lastNote, monoVelocity[lastNote & 127], sound, 0);
                    break;
                }
            }
            currentMonoNote = lastNote;
        }
    }
    else if (voiceMode == 2) // Unison — stop all active voices
    {
        // Unison voices are started via startNote() directly (bypassing Synthesiser::startVoice),
        // so currentlyPlayingNote is never set. Checking it would always fail. Unison is
        // monophonic by design — one MIDI note across all detuned voices — so stopping all
        // active voices on any note-off is correct.
        for (auto* v : voiceCache)
            if (v->isVoiceActive())
                v->stopNote(0.0f, true);
    }
}

void V0idSynthAudioProcessor::configureUnisonVoice(SynthVoice* voice, int unisonIdx,
                                                    int totalUnisonVoices, float detune, float spread)
{
    int N = totalUnisonVoices;
    float maxDetuneCents = detune * 50.0f;

    if (N > 1)
        voice->unisonDetuneOffsetCents = maxDetuneCents *
            ((static_cast<float>(unisonIdx) - static_cast<float>(N - 1) / 2.0f) / std::max(static_cast<float>(N - 1) / 2.0f, 1.0f));
    else
        voice->unisonDetuneOffsetCents = 0.0f;

    if (N > 1)
    {
        float panPosition = spread * ((2.0f * static_cast<float>(unisonIdx) / static_cast<float>(N - 1)) - 1.0f);
        voice->unisonPanLeft  = std::sqrt(0.5f * (1.0f - panPosition));
        voice->unisonPanRight = std::sqrt(0.5f * (1.0f + panPosition));
    }
    else
    {
        voice->unisonPanLeft  = 1.0f;
        voice->unisonPanRight = 1.0f;
    }

    voice->unisonIndex = unisonIdx;
}

bool V0idSynthAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* V0idSynthAudioProcessor::createEditor()
{
    return new V0idSynthAudioProcessorEditor(*this);
}

void V0idSynthAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    // The sample itself is not embedded — store its path so the project reloads it.
    if (activeSampleBuffer != nullptr && activeSampleBuffer->sourceFile != juce::File())
        state.setProperty("samplePath", activeSampleBuffer->sourceFile.getFullPathName(), nullptr);
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void V0idSynthAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
    {
        auto newState = juce::ValueTree::fromXml(*xmlState);
        const juce::String samplePath = newState.getProperty("samplePath").toString();
        newState.removeProperty("samplePath", nullptr);

        fillMissingParamsWithDefaults(newState);
        apvts.replaceState(newState);
        recompileModMatrix(); // After replaceState completes — see 03-RESEARCH.md Q3

        if (samplePath.isNotEmpty())
        {
            const juce::File f(samplePath);
            const bool alreadyLoaded = activeSampleBuffer != nullptr && activeSampleBuffer->sourceFile == f;
            if (f.existsAsFile() && ! alreadyLoaded)
                loadSampleAsync(f);
        }

        // Refresh any open editor (dest combos, preset name) on the message thread.
        stateRestorePending.store(true);
        triggerAsyncUpdate();
    }
}

void V0idSynthAudioProcessor::recompileModMatrix()
{
    // MESSAGE-THREAD-ONLY: allocates a fresh table, atomically publishes it,
    // and defers deletion of the previous one.
    auto* next = new ModMatrixTable();

    auto matrixNode = apvts.state.getOrCreateChildWithName("ModMatrix", nullptr);
    for (int i = 0; i < matrixNode.getNumChildren(); ++i)
    {
        if (next->numRoutes >= ModMatrixTable::kMaxRoutes) break;
        auto routeNode = matrixNode.getChild(i);
        const int   src   = (int)routeNode.getProperty("source", -1);
        const int   dst   = (int)routeNode.getProperty("dest",   -1);
        const float depth = (float)routeNode.getProperty("depth", 0.0);
        // Out-of-range indices (corrupt / future preset) would index past the
        // engine's source array; zero-depth routes are dead weight per sample.
        if (src < 0 || src >= (int)ModSource::Count || dst < 0 || dst >= (int)ModDest::Count
            || juce::exactlyEqual(depth, 0.0f))
            continue;
        ModRoute& r = next->routes[next->numRoutes];
        r.source = static_cast<ModSource>(src);
        r.dest   = static_cast<ModDest>(dst);
        r.depth  = depth;
        ++next->numRoutes;
    }
    next->generation = nextTableGeneration++;

    // seq_cst (default) on the swap and the epoch read: the publish must not be
    // reordered after the epoch sample, or a block could still load `old` after
    // we've recorded its retirement epoch.
    ModMatrixTable* old = liveModMatrix.exchange(next);

    // Free retired tables the audio thread can no longer be reading: once at least
    // one block has completed AFTER a table was retired, any in-flight block must
    // have loaded a newer pointer.
    const auto now = audioBlockEpoch.load();
    for (auto it = retiredModMatrices.begin(); it != retiredModMatrices.end();)
    {
        if (now > it->epoch) { delete it->table; it = retiredModMatrices.erase(it); }
        else                 ++it;
    }
    if (old != nullptr)
        retiredModMatrices.push_back({ old, now });
}

void V0idSynthAudioProcessor::addModRoute(ModSource source, ModDest dest, float depth)
{
    // MESSAGE-THREAD-ONLY. For testing API today; UI consumer in Phase 5.
    auto matrixNode = apvts.state.getOrCreateChildWithName("ModMatrix", nullptr);
    juce::ValueTree routeNode("Route");
    routeNode.setProperty("source", (int)source, nullptr);
    routeNode.setProperty("dest",   (int)dest,   nullptr);
    routeNode.setProperty("depth",  depth,        nullptr);
    matrixNode.appendChild(routeNode, nullptr);
    recompileModMatrix();
}

void V0idSynthAudioProcessor::clearModRoutes()
{
    auto matrixNode = apvts.state.getOrCreateChildWithName("ModMatrix", nullptr);
    matrixNode.removeAllChildren(nullptr);
    recompileModMatrix();
}

void V0idSynthAudioProcessor::loadFactoryPresetsFromBinaryData()
{
    // Iterate the JUCE-generated namedResourceList from v0idsynth_PresetData.
    // Each entry is a C-string resource name (e.g. "_01_pad_fluorescent_hum_xml").
    // We extract every .xml resource as a String and feed PresetManager.
    juce::StringArray xmls;
    for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
    {
        const char* resourceName = BinaryData::namedResourceList[i];
        int dataSize = 0;
        const char* data = BinaryData::getNamedResource(resourceName, dataSize);
        if (data != nullptr && dataSize > 0)
        {
            // Filter to .xml resources only (defensive -- v0idsynth_PresetData should
            // only contain XML, but namedResourceList is a flat name registry).
            const juce::String name(resourceName);
            if (name.endsWithIgnoreCase("_xml"))
                // Preset files are UTF-8 (em-dashes etc. in descriptions); the
                // (const char*, size) constructor assumes ASCII and mangles them.
                xmls.add(juce::String::fromUTF8(data, dataSize));
        }
    }
    presetManager.loadFromXmlStrings(xmls);
    DBG("PresetManager loaded " << presetManager.getNumPresets() << " factory presets from BinaryData");
}

void V0idSynthAudioProcessor::fillMissingParamsWithDefaults(juce::ValueTree& state) const
{
    for (auto* p : getParameters())
    {
        auto* withId = dynamic_cast<juce::RangedAudioParameter*>(p);
        if (withId == nullptr)
            continue;
        if (state.getChildWithProperty("id", withId->paramID).isValid())
            continue;

        juce::ValueTree param("PARAM");
        param.setProperty("id", withId->paramID, nullptr);
        param.setProperty("value", withId->convertFrom0to1(withId->getDefaultValue()), nullptr);
        state.appendChild(param, nullptr);
    }
}

bool V0idSynthAudioProcessor::loadPresetByIndex(int index)
{
    // MESSAGE-THREAD-ONLY. Mirrors setStateInformation but skips MemoryBlock decode.
    // D-14: synchronous; D-03: recompileModMatrix immediately after replaceState.
    auto xmlState = presetManager.getApvtsStateXmlForIndex(index);
    if (xmlState == nullptr)
        return false;
    if (!xmlState->hasTagName(apvts.state.getType()))
        return false;

    // Clear all active voices before loading new state — prevents unison/mono voices
    // (started via direct startNote(), currentlyPlayingNote==-1) from leaking into
    // poly mode where JUCE's noteOff() can never find and stop them.
    // Deferred to the audio thread: resetting voice DSP state here would race
    // a block that is rendering those same voices.
    voiceResetPending.store(true);

    auto newState = juce::ValueTree::fromXml(*xmlState);
    fillMissingParamsWithDefaults(newState);
    apvts.replaceState(newState);
    recompileModMatrix();

    presetManager.setCurrentIndex(index);

    if (onPresetLoaded)
        onPresetLoaded();
    return true;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new V0idSynthAudioProcessor();
}

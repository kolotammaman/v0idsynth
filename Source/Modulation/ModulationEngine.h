#pragma once
#include "VoiceLFO.h"
#include "ModEnvelope.h"
#include "ModMatrixTable.h"
#include <juce_core/juce_core.h>
#include <cmath>

class ModulationEngine
{
public:
    // Per-destination scaling constants — see 03-RESEARCH.md Pitfall 5
    static constexpr float kCutoffModScale = 10000.0f; // depth=1 -> +/-10 kHz cutoff swing
    static constexpr float kResModScale    = 0.5f;     // depth=1 -> +/-0.5 resonance
    static constexpr float kPitchModScale  = 1200.0f;  // depth=1 -> +/-1 octave (cents)
    static constexpr float kOscMixModScale = 1.0f;
    static constexpr float kAmpModScale    = 1.0f;

    void prepare(double sampleRate, int voiceIdx)
    {
        voiceIndex = voiceIdx;
        for (int i = 0; i < 4; ++i)
        {
            lfos[i].prepare(sampleRate);
            lfos[i].seedRNG(static_cast<juce::int64>(voiceIdx) * 1000 + i);
            lfos[i].setPhaseOffset(phaseOffset(voiceIdx, i));
            modEnvs[i].prepare(sampleRate);
        }
        driftValue = 0.0f;
    }

    // Called at startNote — trigger mod envs; LFOs run free (no retrigger to avoid click)
    void startNote()
    {
        for (int i = 0; i < 4; ++i)
        {
            modEnvs[i].reset();
            modEnvs[i].noteOn();
        }
    }

    void stopNote()
    {
        for (auto& e : modEnvs) e.noteOff();
    }

    void setLFORate(int idx, float hz)              { jassert(idx>=0 && idx<4); lfos[idx].setRate(hz); }
    void setLFOShape(int idx, VoiceLFO::Shape s)    { jassert(idx>=0 && idx<4); lfos[idx].setShape(s); }
    void setLFODepth(int idx, float depth01)        { jassert(idx>=0 && idx<4); lfoDepth[idx] = depth01; }
    void setEnvDepth(int idx, float depthBipolar)   { jassert(idx>=0 && idx<4); envDepth[idx] = depthBipolar; }
    void setEnvParameters(int idx, float a, float d, float s, float r)
    { jassert(idx>=0 && idx<4); modEnvs[idx].setParameters(a, d, s, r); }

    // SynthVoice injects its drift value (cents normalized to ~[-1,+1] range) each sample.
    void setDriftSource(float normalizedDrift) { driftValue = normalizedDrift; }

    // Per-sample: step every source, iterate table, accumulate into ModOutputs.
    ModOutputs process(const ModMatrixTable* table)
    {
        // Step all sources every sample so phase/envelope progress is continuous
        // even when no route is wired to them.
        // Unrouted LFOs only advance their phase — the waveform is skipped.
        // Published tables are immutable, so the mask is rebuilt only when a
        // different table arrives (at most once per block). The generation check
        // catches a freed table's address being reused by its successor.
        const uint32_t gen = (table != nullptr) ? table->generation : 0u;
        if (table != maskTable || gen != maskGeneration)
        {
            maskTable = table;
            maskGeneration = gen;
            sourceMask = 0;
            if (table != nullptr)
                for (int r = 0; r < table->numRoutes; ++r)
                    sourceMask |= 1u << (int)table->routes[r].source;
        }
        const uint32_t mask = sourceMask;
        float srcVal[(int)ModSource::Count] = { 0.0f };
        for (int i = 0; i < 4; ++i)
        {
            if (mask & (1u << ((int)ModSource::LFO1 + i)))
                srcVal[(int)ModSource::LFO1 + i] = lfos[i].processSample() * lfoDepth[i];
            else
                lfos[i].advance();
            srcVal[(int)ModSource::Env1 + i] = modEnvs[i].getNextSample() * envDepth[i];
        }
        srcVal[(int)ModSource::Drift] = driftValue;

        ModOutputs out;
        if (table == nullptr) return out;

        const int n = table->numRoutes;
        for (int r = 0; r < n; ++r)
        {
            const ModRoute& route = table->routes[r];
            const float contribution = srcVal[(int)route.source] * route.depth;
            switch (route.dest)
            {
                case ModDest::FilterCutoff:    out.filterCutoffOffset += contribution * kCutoffModScale; break;
                case ModDest::FilterResonance: out.filterResOffset    += contribution * kResModScale;    break;
                case ModDest::OscPitch:        out.pitchCentsOffset   += contribution * kPitchModScale;  break;
                case ModDest::OscMix:          out.oscMixOffset       += contribution * kOscMixModScale; break;
                case ModDest::AmpGain:         out.ampGainOffset      += contribution * kAmpModScale;    break;
                case ModDest::Count:           break;  // sentinel — not a routable destination
            }
        }
        return out;
    }

private:
    // Golden-ratio phase spread — 03-RESEARCH.md Q4 + 03-CONTEXT.md decision
    static float phaseOffset(int voiceIdx, int lfoIdx)
    {
        static constexpr float kGolden = 0.6180339887f;
        return std::fmod(static_cast<float>(voiceIdx) * kGolden + static_cast<float>(lfoIdx) * 0.25f, 1.0f);
    }

    VoiceLFO     lfos[4];
    ModEnvelope  modEnvs[4];
    float        lfoDepth[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    float        envDepth[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    float        driftValue  = 0.0f;
    int          voiceIndex  = 0;
    const ModMatrixTable* maskTable = nullptr;
    uint32_t     maskGeneration = 0;
    uint32_t     sourceMask  = 0;
};

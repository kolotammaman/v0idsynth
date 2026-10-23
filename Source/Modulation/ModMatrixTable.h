// ModMatrixTable.h — modulation matrix type contract for v0idsynth
// Plain POD types: no heap allocation, no JUCE, no methods.
// Consumed by VoiceLFO, ModulationEngine, SynthVoice, PluginProcessor.
#pragma once
#include <cstdint>

enum class ModSource : uint8_t {
    LFO1 = 0, LFO2, LFO3, LFO4,
    Env1, Env2, Env3, Env4,
    Drift,   // per-voice drift (existing in SynthVoice::driftRng)
    Count
};

enum class ModDest : uint8_t {
    FilterCutoff = 0, FilterResonance,
    OscPitch, OscMix, AmpGain,
    Count
};

struct ModRoute {
    ModSource source;
    ModDest   dest;
    float     depth;   // signed -1..+1; 0 == disabled, never stored in active table
};

struct ModMatrixTable {
    static constexpr int kMaxRoutes = 32;
    ModRoute routes[kMaxRoutes];
    int      numRoutes = 0;
    uint32_t generation = 0;  // unique per compiled table; guards pointer-reuse (ABA) in caches
};

struct ModOutputs {
    float filterCutoffOffset = 0.0f;   // Hz (additive to base cutoff)
    float filterResOffset    = 0.0f;   // 0..1 delta
    float pitchCentsOffset   = 0.0f;   // cents
    float oscMixOffset       = 0.0f;   // 0..1 delta
    float ampGainOffset      = 0.0f;   // 0..1 delta
};

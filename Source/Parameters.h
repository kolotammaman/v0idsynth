#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

// =============================================================================
// v0idsynth Parameter ID Registry — All 5 Phases
//
// VERSIONING RULE: All parameters in the initial release use version hint 1.
// When adding parameters in future releases, use the next incremented hint
// (hint 2, hint 3, etc.) so new parameters sort AFTER existing ones in AU hosts.
// NEVER change an existing parameter's ID string or version hint — doing so
// corrupts any Logic project that has saved automation against this plugin.
// See: PARAMETER_STRATEGY.md for the full scheme and warning.
//
// NAMING CONVENTION: hierarchical dot notation — section.parameter or
// section.subsection.parameter (e.g. osc1.waveform, filter.env.attack).
// =============================================================================

namespace ParamIDs
{
    // -------------------------------------------------------------------------
    // Oscillator section (Phase 1 + Phase 2)
    // -------------------------------------------------------------------------

    // Osc1: analog waveform oscillator
    inline constexpr const char* OSC1_WAVEFORM     = "osc1.waveform";       // Choice: Saw/Square/Triangle/Sine
    inline constexpr const char* OSC1_DRIVE        = "osc1.drive";          // Float 0-1: pre-filter soft-clip drive

    // Osc2: sample oscillator
    inline constexpr const char* OSC2_SAMPLE_ID    = "osc2.sample_id";      // Int 0-127: sample bank slot index
    inline constexpr const char* OSC2_ROOT_NOTE    = "osc2.root_note";      // Int 0-127: MIDI note at which sample plays at natural pitch
    inline constexpr const char* OSC2_TUNE         = "osc2.tune";           // Float -100 to +100 cents: fine-tune offset
    inline constexpr const char* OSC2_LOOP         = "osc2.loop";           // Bool: loop sample when note is held
    inline constexpr const char* OSC2_GAIN         = "osc2.gain";           // Float -24 to +24 dB: sample output gain
    inline constexpr const char* OSC2_HPF          = "osc2.hpf";            // Float 20-2000 Hz: sample high-pass filter cutoff
    inline constexpr const char* OSC2_LPF          = "osc2.lpf";            // Float 500-20000 Hz: sample low-pass filter cutoff

    // Oscillator mixer
    inline constexpr const char* OSC_MIX           = "osc.mix";             // Float 0-1: 0=osc1 only, 1=osc2 only

    // -------------------------------------------------------------------------
    // Filter section (Phase 1)
    // -------------------------------------------------------------------------

    inline constexpr const char* FILTER_CUTOFF     = "filter.cutoff";       // Float 20-20000 Hz (log-skewed)
    inline constexpr const char* FILTER_RESONANCE  = "filter.resonance";    // Float 0-1
    inline constexpr const char* FILTER_MODE_BLEND = "filter.mode_blend";   // Float 0-3: 0=LP, 1=HP, 2=BP, 3=Notch
    inline constexpr const char* FILTER_DRIVE      = "filter.drive";        // Float 0-1: LadderFilter saturation amount
    inline constexpr const char* FILTER_ENV_AMOUNT = "filter.env_amount";   // Float -1 to +1: filter envelope depth (bipolar)

    // Filter envelope (ADSR)
    inline constexpr const char* FILTER_ENV_ATTACK  = "filter.env.attack";  // Float 0.001-10 s (log-skewed)
    inline constexpr const char* FILTER_ENV_DECAY   = "filter.env.decay";   // Float 0.001-10 s (log-skewed)
    inline constexpr const char* FILTER_ENV_SUSTAIN = "filter.env.sustain"; // Float 0-1
    inline constexpr const char* FILTER_ENV_RELEASE = "filter.env.release"; // Float 0.001-10 s (log-skewed)

    // -------------------------------------------------------------------------
    // Amplitude envelope (Phase 1)
    // -------------------------------------------------------------------------

    inline constexpr const char* AMP_ENV_ATTACK    = "amp.env.attack";      // Float 0.001-10 s (log-skewed)
    inline constexpr const char* AMP_ENV_DECAY     = "amp.env.decay";       // Float 0.001-10 s (log-skewed)
    inline constexpr const char* AMP_ENV_SUSTAIN   = "amp.env.sustain";     // Float 0-1
    inline constexpr const char* AMP_ENV_RELEASE   = "amp.env.release";     // Float 0.001-10 s (log-skewed)

    // -------------------------------------------------------------------------
    // Voice section (Phase 2)
    // -------------------------------------------------------------------------

    inline constexpr const char* VOICE_MODE          = "voice.mode";          // Choice: Poly/Mono/Unison
    inline constexpr const char* VOICE_GLIDE         = "voice.glide";         // Float 0-1: portamento time
    inline constexpr const char* VOICE_UNISON_VOICES = "voice.unison_voices"; // Int 1-8: unison voice count
    inline constexpr const char* VOICE_UNISON_DETUNE = "voice.unison_detune"; // Float 0-1: unison detune spread
    inline constexpr const char* VOICE_UNISON_SPREAD = "voice.unison_spread"; // Float 0-1: unison stereo spread
    inline constexpr const char* VOICE_DRIFT        = "voice.drift";          // Float 0-1: per-voice pitch drift intensity

    // -------------------------------------------------------------------------
    // LFOs — 4 independent LFOs (Phase 3)
    // -------------------------------------------------------------------------

    inline constexpr const char* MOD_LFO1_RATE     = "mod.lfo1.rate";       // Float 0.01-20 Hz (log-skewed)
    inline constexpr const char* MOD_LFO1_WAVEFORM = "mod.lfo1.waveform";   // Choice: Sine/Triangle/Saw/Square/S&H
    inline constexpr const char* MOD_LFO1_DEPTH    = "mod.lfo1.depth";      // Float 0-1

    inline constexpr const char* MOD_LFO2_RATE     = "mod.lfo2.rate";
    inline constexpr const char* MOD_LFO2_WAVEFORM = "mod.lfo2.waveform";
    inline constexpr const char* MOD_LFO2_DEPTH    = "mod.lfo2.depth";

    inline constexpr const char* MOD_LFO3_RATE     = "mod.lfo3.rate";
    inline constexpr const char* MOD_LFO3_WAVEFORM = "mod.lfo3.waveform";
    inline constexpr const char* MOD_LFO3_DEPTH    = "mod.lfo3.depth";

    inline constexpr const char* MOD_LFO4_RATE     = "mod.lfo4.rate";
    inline constexpr const char* MOD_LFO4_WAVEFORM = "mod.lfo4.waveform";
    inline constexpr const char* MOD_LFO4_DEPTH    = "mod.lfo4.depth";

    // -------------------------------------------------------------------------
    // Modulation envelopes — 4 independent envelopes (Phase 3)
    // -------------------------------------------------------------------------

    inline constexpr const char* MOD_ENV1_ATTACK   = "mod.env1.attack";
    inline constexpr const char* MOD_ENV1_DECAY    = "mod.env1.decay";
    inline constexpr const char* MOD_ENV1_SUSTAIN  = "mod.env1.sustain";
    inline constexpr const char* MOD_ENV1_RELEASE  = "mod.env1.release";

    inline constexpr const char* MOD_ENV2_ATTACK   = "mod.env2.attack";
    inline constexpr const char* MOD_ENV2_DECAY    = "mod.env2.decay";
    inline constexpr const char* MOD_ENV2_SUSTAIN  = "mod.env2.sustain";
    inline constexpr const char* MOD_ENV2_RELEASE  = "mod.env2.release";

    inline constexpr const char* MOD_ENV3_ATTACK   = "mod.env3.attack";
    inline constexpr const char* MOD_ENV3_DECAY    = "mod.env3.decay";
    inline constexpr const char* MOD_ENV3_SUSTAIN  = "mod.env3.sustain";
    inline constexpr const char* MOD_ENV3_RELEASE  = "mod.env3.release";

    inline constexpr const char* MOD_ENV4_ATTACK   = "mod.env4.attack";
    inline constexpr const char* MOD_ENV4_DECAY    = "mod.env4.decay";
    inline constexpr const char* MOD_ENV4_SUSTAIN  = "mod.env4.sustain";
    inline constexpr const char* MOD_ENV4_RELEASE  = "mod.env4.release";

    // Mod envelope output depth (version hint 2). Bipolar -1..+1: scales the
    // envelope before it hits the mod matrix; negative values invert it.
    // Default 1.0 reproduces the pre-depth behaviour (routes ran at full scale).
    inline constexpr const char* MOD_ENV1_DEPTH    = "mod.env1.depth";
    inline constexpr const char* MOD_ENV2_DEPTH    = "mod.env2.depth";
    inline constexpr const char* MOD_ENV3_DEPTH    = "mod.env3.depth";
    inline constexpr const char* MOD_ENV4_DEPTH    = "mod.env4.depth";

    // -------------------------------------------------------------------------
    // Master (Phase 1)
    // -------------------------------------------------------------------------

    inline constexpr const char* MASTER_GAIN       = "master.gain";         // Float -60 to +12 dB

} // namespace ParamIDs


// =============================================================================
// createParameterLayout()
//
// Returns the full APVTS parameter layout for all 5 phases.
// Every ParameterID uses version hint 1 — these are the "founding" parameters.
// Future additions must use version hint >= 2. See PARAMETER_STRATEGY.md.
// =============================================================================

inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // =========================================================================
    // Oscillator section
    // =========================================================================

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { ParamIDs::OSC1_WAVEFORM, 1 },
        "Osc1 Waveform",
        juce::StringArray { "Saw", "Square", "Triangle", "Sine" },
        0)); // default: Saw

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::OSC1_DRIVE, 1 },
        "Osc1 Drive",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f));

    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID { ParamIDs::OSC2_SAMPLE_ID, 1 },
        "Osc2 Sample ID",
        0, 127, 0));

    // Version hint 2: added after initial release — sort after all hint-1 params in AU hosts.
    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID { ParamIDs::OSC2_ROOT_NOTE, 2 },
        "Osc2 Root Note",
        0, 127, 60));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::OSC2_TUNE, 2 },
        "Osc2 Tune",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.01f),
        0.0f));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { ParamIDs::OSC2_LOOP, 2 },
        "Osc2 Loop",
        false));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::OSC2_GAIN, 2 },
        "Osc2 Sample Gain",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f),
        0.0f));

    {
        auto hpfRange = juce::NormalisableRange<float>(20.0f, 2000.0f, 0.1f);
        hpfRange.setSkewForCentre(200.0f);
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { ParamIDs::OSC2_HPF, 2 },
            "Osc2 HPF Cutoff",
            hpfRange,
            20.0f));
    }

    {
        auto lpfRange = juce::NormalisableRange<float>(500.0f, 20000.0f, 0.1f);
        lpfRange.setSkewForCentre(5000.0f);
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { ParamIDs::OSC2_LPF, 2 },
            "Osc2 LPF Cutoff",
            lpfRange,
            20000.0f));
    }

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::OSC_MIX, 1 },
        "Osc Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f));

    // =========================================================================
    // Filter section
    // =========================================================================

    {
        auto cutoffRange = juce::NormalisableRange<float>(20.0f, 20000.0f, 0.1f);
        cutoffRange.setSkewForCentre(1000.0f);
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { ParamIDs::FILTER_CUTOFF, 1 },
            "Filter Cutoff",
            cutoffRange,
            20000.0f));
    }

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::FILTER_RESONANCE, 1 },
        "Filter Resonance",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::FILTER_MODE_BLEND, 1 },
        "Filter Mode Blend",
        juce::NormalisableRange<float>(0.0f, 3.0f, 0.01f),
        0.0f)); // 0=LP, 1=HP, 2=BP, 3=Notch — crossfadeable

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::FILTER_DRIVE, 1 },
        "Filter Drive",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::FILTER_ENV_AMOUNT, 1 },
        "Filter Env Amount",
        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f),
        0.0f));

    // Filter envelope
    {
        auto attackRange = juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f);
        attackRange.setSkewForCentre(0.3f);
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { ParamIDs::FILTER_ENV_ATTACK, 1 },
            "Filter Env Attack",
            attackRange,
            0.01f));
    }

    {
        auto decayRange = juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f);
        decayRange.setSkewForCentre(0.3f);
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { ParamIDs::FILTER_ENV_DECAY, 1 },
            "Filter Env Decay",
            decayRange,
            0.3f));
    }

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::FILTER_ENV_SUSTAIN, 1 },
        "Filter Env Sustain",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.7f));

    {
        auto releaseRange = juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f);
        releaseRange.setSkewForCentre(0.5f);
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { ParamIDs::FILTER_ENV_RELEASE, 1 },
            "Filter Env Release",
            releaseRange,
            0.5f));
    }

    // =========================================================================
    // Amplitude envelope
    // =========================================================================

    {
        auto attackRange = juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f);
        attackRange.setSkewForCentre(0.3f);
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { ParamIDs::AMP_ENV_ATTACK, 1 },
            "Amp Env Attack",
            attackRange,
            0.01f));
    }

    {
        auto decayRange = juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f);
        decayRange.setSkewForCentre(0.3f);
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { ParamIDs::AMP_ENV_DECAY, 1 },
            "Amp Env Decay",
            decayRange,
            0.3f));
    }

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::AMP_ENV_SUSTAIN, 1 },
        "Amp Env Sustain",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.8f));

    {
        auto releaseRange = juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f);
        releaseRange.setSkewForCentre(0.5f);
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { ParamIDs::AMP_ENV_RELEASE, 1 },
            "Amp Env Release",
            releaseRange,
            0.5f));
    }

    // =========================================================================
    // Voice section
    // =========================================================================

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { ParamIDs::VOICE_MODE, 1 },
        "Voice Mode",
        juce::StringArray { "Poly", "Mono", "Unison" },
        0));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::VOICE_GLIDE, 1 },
        "Voice Glide",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f));

    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID { ParamIDs::VOICE_UNISON_VOICES, 1 },
        "Unison Voices",
        1, 8, 4));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::VOICE_UNISON_DETUNE, 1 },
        "Unison Detune",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.3f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::VOICE_UNISON_SPREAD, 1 },
        "Unison Spread",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::VOICE_DRIFT, 1 },
        "Voice Drift",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.1f));

    // =========================================================================
    // LFOs — 4 independent LFOs
    // =========================================================================

    // LFO1
    { auto r = juce::NormalisableRange<float>(0.01f, 20.0f, 0.01f); r.setSkewForCentre(2.0f);
      layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { ParamIDs::MOD_LFO1_RATE, 1 }, "LFO1 Rate", r, 1.0f)); }
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID { ParamIDs::MOD_LFO1_WAVEFORM, 1 }, "LFO1 Waveform", juce::StringArray { "Sine", "Triangle", "Saw", "Square", "S&H" }, 0));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { ParamIDs::MOD_LFO1_DEPTH, 1 }, "LFO1 Depth", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));

    // LFO2
    { auto r = juce::NormalisableRange<float>(0.01f, 20.0f, 0.01f); r.setSkewForCentre(2.0f);
      layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { ParamIDs::MOD_LFO2_RATE, 1 }, "LFO2 Rate", r, 1.0f)); }
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID { ParamIDs::MOD_LFO2_WAVEFORM, 1 }, "LFO2 Waveform", juce::StringArray { "Sine", "Triangle", "Saw", "Square", "S&H" }, 0));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { ParamIDs::MOD_LFO2_DEPTH, 1 }, "LFO2 Depth", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));

    // LFO3
    { auto r = juce::NormalisableRange<float>(0.01f, 20.0f, 0.01f); r.setSkewForCentre(2.0f);
      layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { ParamIDs::MOD_LFO3_RATE, 1 }, "LFO3 Rate", r, 1.0f)); }
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID { ParamIDs::MOD_LFO3_WAVEFORM, 1 }, "LFO3 Waveform", juce::StringArray { "Sine", "Triangle", "Saw", "Square", "S&H" }, 0));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { ParamIDs::MOD_LFO3_DEPTH, 1 }, "LFO3 Depth", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));

    // LFO4
    { auto r = juce::NormalisableRange<float>(0.01f, 20.0f, 0.01f); r.setSkewForCentre(2.0f);
      layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { ParamIDs::MOD_LFO4_RATE, 1 }, "LFO4 Rate", r, 1.0f)); }
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID { ParamIDs::MOD_LFO4_WAVEFORM, 1 }, "LFO4 Waveform", juce::StringArray { "Sine", "Triangle", "Saw", "Square", "S&H" }, 0));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { ParamIDs::MOD_LFO4_DEPTH, 1 }, "LFO4 Depth", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));

    // =========================================================================
    // Modulation envelopes — 4 independent envelopes
    // =========================================================================

    // Mod Env1
    { auto r = juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f); r.setSkewForCentre(0.3f);
      layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { ParamIDs::MOD_ENV1_ATTACK, 1 }, "Mod Env1 Attack", r, 0.01f)); }
    { auto r = juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f); r.setSkewForCentre(0.3f);
      layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { ParamIDs::MOD_ENV1_DECAY, 1 }, "Mod Env1 Decay", r, 0.3f)); }
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { ParamIDs::MOD_ENV1_SUSTAIN, 1 }, "Mod Env1 Sustain", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));
    { auto r = juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f); r.setSkewForCentre(0.5f);
      layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { ParamIDs::MOD_ENV1_RELEASE, 1 }, "Mod Env1 Release", r, 0.5f)); }

    // Mod Env2
    { auto r = juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f); r.setSkewForCentre(0.3f);
      layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { ParamIDs::MOD_ENV2_ATTACK, 1 }, "Mod Env2 Attack", r, 0.01f)); }
    { auto r = juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f); r.setSkewForCentre(0.3f);
      layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { ParamIDs::MOD_ENV2_DECAY, 1 }, "Mod Env2 Decay", r, 0.3f)); }
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { ParamIDs::MOD_ENV2_SUSTAIN, 1 }, "Mod Env2 Sustain", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));
    { auto r = juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f); r.setSkewForCentre(0.5f);
      layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { ParamIDs::MOD_ENV2_RELEASE, 1 }, "Mod Env2 Release", r, 0.5f)); }

    // Mod Env3
    { auto r = juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f); r.setSkewForCentre(0.3f);
      layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { ParamIDs::MOD_ENV3_ATTACK, 1 }, "Mod Env3 Attack", r, 0.01f)); }
    { auto r = juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f); r.setSkewForCentre(0.3f);
      layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { ParamIDs::MOD_ENV3_DECAY, 1 }, "Mod Env3 Decay", r, 0.3f)); }
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { ParamIDs::MOD_ENV3_SUSTAIN, 1 }, "Mod Env3 Sustain", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));
    { auto r = juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f); r.setSkewForCentre(0.5f);
      layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { ParamIDs::MOD_ENV3_RELEASE, 1 }, "Mod Env3 Release", r, 0.5f)); }

    // Mod Env4
    { auto r = juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f); r.setSkewForCentre(0.3f);
      layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { ParamIDs::MOD_ENV4_ATTACK, 1 }, "Mod Env4 Attack", r, 0.01f)); }
    { auto r = juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f); r.setSkewForCentre(0.3f);
      layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { ParamIDs::MOD_ENV4_DECAY, 1 }, "Mod Env4 Decay", r, 0.3f)); }
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { ParamIDs::MOD_ENV4_SUSTAIN, 1 }, "Mod Env4 Sustain", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));
    { auto r = juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f); r.setSkewForCentre(0.5f);
      layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { ParamIDs::MOD_ENV4_RELEASE, 1 }, "Mod Env4 Release", r, 0.5f)); }

    // Mod env depths — version hint 2 (added after the founding release)
    {
        const char* ids[4]   = { ParamIDs::MOD_ENV1_DEPTH, ParamIDs::MOD_ENV2_DEPTH,
                                 ParamIDs::MOD_ENV3_DEPTH, ParamIDs::MOD_ENV4_DEPTH };
        const char* names[4] = { "Mod Env1 Depth", "Mod Env2 Depth", "Mod Env3 Depth", "Mod Env4 Depth" };
        for (int i = 0; i < 4; ++i)
            layout.add(std::make_unique<juce::AudioParameterFloat>(
                juce::ParameterID { ids[i], 2 }, names[i],
                juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), 1.0f));
    }

    // =========================================================================
    // Master
    // =========================================================================

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::MASTER_GAIN, 1 },
        "Master Gain",
        juce::NormalisableRange<float>(-60.0f, 12.0f, 0.1f),
        0.0f));

    return layout;
}

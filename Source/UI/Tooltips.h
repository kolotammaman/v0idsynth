#pragma once
// =============================================================================
// Phase 5 / UI-03 — Tooltip string lookup for all 52 APVTS parameters.
// Per .planning/phases/05-ui-and-aesthetic/05-UI-SPEC.md section "Tooltip Strings"
// (D-19). Locked copywriting. Do NOT edit strings without updating UI-SPEC.
//
// Usage: Tooltips::forParamId(ParamIDs::FOO) returns a const char* tooltip.
// Plan 05-03 wires this to every Slider/ComboBox/Button via setTooltip().
// =============================================================================

#include <cstring>
#include "../Parameters.h"

namespace Tooltips
{
    // Returns the human-readable tooltip string for the given APVTS parameter ID,
    // or an empty const char* if the paramId is not recognised. Caller must NOT
    // free the returned pointer.
    inline const char* forParamId(const char* paramId)
    {
        // ------------------------------------------------------------------
        // Oscillator (4)
        // ------------------------------------------------------------------
        if (std::strcmp(paramId, ParamIDs::OSC1_WAVEFORM) == 0)
            return "Shape of the analog oscillator. Saw cuts. Sine drifts.";
        if (std::strcmp(paramId, ParamIDs::OSC1_DRIVE) == 0)
            return "Pre-filter saturation. Push it until it smears.";
        if (std::strcmp(paramId, ParamIDs::OSC2_SAMPLE_ID) == 0)
            return "Sample slot index loaded into the second oscillator.";
        if (std::strcmp(paramId, ParamIDs::OSC_MIX) == 0)
            return "Blend between analog and sample oscillator. 0 = pure analog, 1 = pure sample.";

        // ------------------------------------------------------------------
        // Filter (5)
        // ------------------------------------------------------------------
        if (std::strcmp(paramId, ParamIDs::FILTER_CUTOFF) == 0)
            return "The throat of the sound. Opens and closes the frequency spectrum.";
        if (std::strcmp(paramId, ParamIDs::FILTER_RESONANCE) == 0)
            return "Self-resonance at the cutoff. High values will scream.";
        if (std::strcmp(paramId, ParamIDs::FILTER_MODE_BLEND) == 0)
            return "Crossfades LP -> HP -> BP -> Notch. Fractional values blend modes.";
        if (std::strcmp(paramId, ParamIDs::FILTER_DRIVE) == 0)
            return "Ladder-style harmonic saturation after the filter. Warms or destroys.";
        if (std::strcmp(paramId, ParamIDs::FILTER_ENV_AMOUNT) == 0)
            return "How far the filter envelope moves the cutoff. Negative inverts the sweep.";

        // ------------------------------------------------------------------
        // Filter envelope (4)
        // ------------------------------------------------------------------
        if (std::strcmp(paramId, ParamIDs::FILTER_ENV_ATTACK) == 0)
            return "Time for the filter envelope to reach its peak.";
        if (std::strcmp(paramId, ParamIDs::FILTER_ENV_DECAY) == 0)
            return "Time for the filter envelope to fall to sustain.";
        if (std::strcmp(paramId, ParamIDs::FILTER_ENV_SUSTAIN) == 0)
            return "Level the filter envelope holds while a key is pressed.";
        if (std::strcmp(paramId, ParamIDs::FILTER_ENV_RELEASE) == 0)
            return "Time for the filter envelope to close after key release.";

        // ------------------------------------------------------------------
        // Amp envelope (4)
        // ------------------------------------------------------------------
        if (std::strcmp(paramId, ParamIDs::AMP_ENV_ATTACK) == 0)
            return "How fast the voice fades in from silence.";
        if (std::strcmp(paramId, ParamIDs::AMP_ENV_DECAY) == 0)
            return "How fast the voice falls from its peak to the sustain level.";
        if (std::strcmp(paramId, ParamIDs::AMP_ENV_SUSTAIN) == 0)
            return "Volume the voice holds while a key is pressed.";
        if (std::strcmp(paramId, ParamIDs::AMP_ENV_RELEASE) == 0)
            return "How long the voice fades out after you let go.";

        // ------------------------------------------------------------------
        // Voice (6)
        // ------------------------------------------------------------------
        if (std::strcmp(paramId, ParamIDs::VOICE_MODE) == 0)
            return "Poly: 16 voices. Mono: one voice, glide-enabled. Unison: stacked and wide.";
        if (std::strcmp(paramId, ParamIDs::VOICE_GLIDE) == 0)
            return "Portamento time in mono/unison mode. Zero is instant.";
        if (std::strcmp(paramId, ParamIDs::VOICE_UNISON_VOICES) == 0)
            return "Number of stacked voices in unison mode. More = thicker and slower.";
        if (std::strcmp(paramId, ParamIDs::VOICE_UNISON_DETUNE) == 0)
            return "Spread of pitch between unison voices. Zero = phase-cancel risk.";
        if (std::strcmp(paramId, ParamIDs::VOICE_UNISON_SPREAD) == 0)
            return "Stereo width of unison detuning. Full right = hard pan separation.";
        if (std::strcmp(paramId, ParamIDs::VOICE_DRIFT) == 0)
            return "Per-voice pitch instability. The feeling that it is alive and slightly wrong.";

        // ------------------------------------------------------------------
        // LFOs (12 = 4 LFOs x 3 params)
        // ------------------------------------------------------------------
        if (std::strcmp(paramId, ParamIDs::MOD_LFO1_RATE) == 0)
            return "Speed of LFO 1 in Hz. Low values breathe. High values chatter.";
        if (std::strcmp(paramId, ParamIDs::MOD_LFO1_WAVEFORM) == 0)
            return "Shape of LFO 1's modulation wave.";
        if (std::strcmp(paramId, ParamIDs::MOD_LFO1_DEPTH) == 0)
            return "Overall output gain of LFO 1 before routing to destinations.";

        if (std::strcmp(paramId, ParamIDs::MOD_LFO2_RATE) == 0)
            return "Speed of LFO 2 in Hz.";
        if (std::strcmp(paramId, ParamIDs::MOD_LFO2_WAVEFORM) == 0)
            return "Shape of LFO 2's modulation wave.";
        if (std::strcmp(paramId, ParamIDs::MOD_LFO2_DEPTH) == 0)
            return "Overall output gain of LFO 2.";

        if (std::strcmp(paramId, ParamIDs::MOD_LFO3_RATE) == 0)
            return "Speed of LFO 3 in Hz.";
        if (std::strcmp(paramId, ParamIDs::MOD_LFO3_WAVEFORM) == 0)
            return "Shape of LFO 3's modulation wave.";
        if (std::strcmp(paramId, ParamIDs::MOD_LFO3_DEPTH) == 0)
            return "Overall output gain of LFO 3.";

        if (std::strcmp(paramId, ParamIDs::MOD_LFO4_RATE) == 0)
            return "Speed of LFO 4 in Hz.";
        if (std::strcmp(paramId, ParamIDs::MOD_LFO4_WAVEFORM) == 0)
            return "Shape of LFO 4's modulation wave.";
        if (std::strcmp(paramId, ParamIDs::MOD_LFO4_DEPTH) == 0)
            return "Overall output gain of LFO 4.";

        // ------------------------------------------------------------------
        // Mod envelopes (20 = 4 envelopes x ADSR + depth)
        // ------------------------------------------------------------------
        if (std::strcmp(paramId, ParamIDs::MOD_ENV1_ATTACK) == 0)
            return "Attack time of modulation envelope 1.";
        if (std::strcmp(paramId, ParamIDs::MOD_ENV1_DECAY) == 0)
            return "Decay time of modulation envelope 1.";
        if (std::strcmp(paramId, ParamIDs::MOD_ENV1_SUSTAIN) == 0)
            return "Sustain level of modulation envelope 1.";
        if (std::strcmp(paramId, ParamIDs::MOD_ENV1_RELEASE) == 0)
            return "Release time of modulation envelope 1.";
        if (std::strcmp(paramId, ParamIDs::MOD_ENV1_DEPTH) == 0)
            return "How hard envelope 1 pushes its destination. Centre is nothing. Left turns it inside out.";

        if (std::strcmp(paramId, ParamIDs::MOD_ENV2_ATTACK) == 0)
            return "Attack time of modulation envelope 2.";
        if (std::strcmp(paramId, ParamIDs::MOD_ENV2_DECAY) == 0)
            return "Decay time of modulation envelope 2.";
        if (std::strcmp(paramId, ParamIDs::MOD_ENV2_SUSTAIN) == 0)
            return "Sustain level of modulation envelope 2.";
        if (std::strcmp(paramId, ParamIDs::MOD_ENV2_RELEASE) == 0)
            return "Release time of modulation envelope 2.";
        if (std::strcmp(paramId, ParamIDs::MOD_ENV2_DEPTH) == 0)
            return "Output depth of modulation envelope 2. Negative values invert it.";

        if (std::strcmp(paramId, ParamIDs::MOD_ENV3_ATTACK) == 0)
            return "Attack time of modulation envelope 3.";
        if (std::strcmp(paramId, ParamIDs::MOD_ENV3_DECAY) == 0)
            return "Decay time of modulation envelope 3.";
        if (std::strcmp(paramId, ParamIDs::MOD_ENV3_SUSTAIN) == 0)
            return "Sustain level of modulation envelope 3.";
        if (std::strcmp(paramId, ParamIDs::MOD_ENV3_RELEASE) == 0)
            return "Release time of modulation envelope 3.";
        if (std::strcmp(paramId, ParamIDs::MOD_ENV3_DEPTH) == 0)
            return "Output depth of modulation envelope 3. Negative values invert it.";

        if (std::strcmp(paramId, ParamIDs::MOD_ENV4_ATTACK) == 0)
            return "Attack time of modulation envelope 4.";
        if (std::strcmp(paramId, ParamIDs::MOD_ENV4_DECAY) == 0)
            return "Decay time of modulation envelope 4.";
        if (std::strcmp(paramId, ParamIDs::MOD_ENV4_SUSTAIN) == 0)
            return "Sustain level of modulation envelope 4.";
        if (std::strcmp(paramId, ParamIDs::MOD_ENV4_RELEASE) == 0)
            return "Release time of modulation envelope 4.";
        if (std::strcmp(paramId, ParamIDs::MOD_ENV4_DEPTH) == 0)
            return "Output depth of modulation envelope 4. Negative values invert it.";

        // ------------------------------------------------------------------
        // Master (1)
        // ------------------------------------------------------------------
        if (std::strcmp(paramId, ParamIDs::MASTER_GAIN) == 0)
            return "Output gain in dB. 0 dB is unity. Clip at +12 if you need to.";

        // Unknown param ID — return empty string, never nullptr (safe for juce::String())
        return "";
    }

} // namespace Tooltips

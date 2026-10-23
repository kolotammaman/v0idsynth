#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../Parameters.h"

// Raw atomic parameter pointers resolved ONCE at construction.
// APVTS::getRawParameterValue() is a string-keyed lookup; calling it ~45 times
// per voice per block (x16 voices) was the single largest non-DSP cost on the
// audio thread. Every audio-thread read now goes through these cached pointers.
struct ParamCache
{
    explicit ParamCache (juce::AudioProcessorValueTreeState& apvts)
    {
        auto p = [&apvts] (const char* id)
        {
            auto* ptr = apvts.getRawParameterValue (id);
            jassert (ptr != nullptr);
            return ptr;
        };

        osc1Waveform = p (ParamIDs::OSC1_WAVEFORM);
        osc1Drive    = p (ParamIDs::OSC1_DRIVE);
        osc2RootNote = p (ParamIDs::OSC2_ROOT_NOTE);
        osc2Tune     = p (ParamIDs::OSC2_TUNE);
        osc2Loop     = p (ParamIDs::OSC2_LOOP);
        osc2Gain     = p (ParamIDs::OSC2_GAIN);
        osc2Hpf      = p (ParamIDs::OSC2_HPF);
        osc2Lpf      = p (ParamIDs::OSC2_LPF);
        oscMix       = p (ParamIDs::OSC_MIX);

        filterCutoff    = p (ParamIDs::FILTER_CUTOFF);
        filterResonance = p (ParamIDs::FILTER_RESONANCE);
        filterModeBlend = p (ParamIDs::FILTER_MODE_BLEND);
        filterDrive     = p (ParamIDs::FILTER_DRIVE);
        filterEnvAmount = p (ParamIDs::FILTER_ENV_AMOUNT);
        filterEnvA      = p (ParamIDs::FILTER_ENV_ATTACK);
        filterEnvD      = p (ParamIDs::FILTER_ENV_DECAY);
        filterEnvS      = p (ParamIDs::FILTER_ENV_SUSTAIN);
        filterEnvR      = p (ParamIDs::FILTER_ENV_RELEASE);

        ampA = p (ParamIDs::AMP_ENV_ATTACK);
        ampD = p (ParamIDs::AMP_ENV_DECAY);
        ampS = p (ParamIDs::AMP_ENV_SUSTAIN);
        ampR = p (ParamIDs::AMP_ENV_RELEASE);

        voiceMode     = p (ParamIDs::VOICE_MODE);
        voiceGlide    = p (ParamIDs::VOICE_GLIDE);
        unisonVoices  = p (ParamIDs::VOICE_UNISON_VOICES);
        unisonDetune  = p (ParamIDs::VOICE_UNISON_DETUNE);
        unisonSpread  = p (ParamIDs::VOICE_UNISON_SPREAD);
        voiceDrift    = p (ParamIDs::VOICE_DRIFT);

        const char* lfoR[4] = { ParamIDs::MOD_LFO1_RATE,     ParamIDs::MOD_LFO2_RATE,
                                ParamIDs::MOD_LFO3_RATE,     ParamIDs::MOD_LFO4_RATE };
        const char* lfoW[4] = { ParamIDs::MOD_LFO1_WAVEFORM, ParamIDs::MOD_LFO2_WAVEFORM,
                                ParamIDs::MOD_LFO3_WAVEFORM, ParamIDs::MOD_LFO4_WAVEFORM };
        const char* lfoD[4] = { ParamIDs::MOD_LFO1_DEPTH,    ParamIDs::MOD_LFO2_DEPTH,
                                ParamIDs::MOD_LFO3_DEPTH,    ParamIDs::MOD_LFO4_DEPTH };
        const char* envA[4] = { ParamIDs::MOD_ENV1_ATTACK,   ParamIDs::MOD_ENV2_ATTACK,
                                ParamIDs::MOD_ENV3_ATTACK,   ParamIDs::MOD_ENV4_ATTACK };
        const char* envD[4] = { ParamIDs::MOD_ENV1_DECAY,    ParamIDs::MOD_ENV2_DECAY,
                                ParamIDs::MOD_ENV3_DECAY,    ParamIDs::MOD_ENV4_DECAY };
        const char* envS[4] = { ParamIDs::MOD_ENV1_SUSTAIN,  ParamIDs::MOD_ENV2_SUSTAIN,
                                ParamIDs::MOD_ENV3_SUSTAIN,  ParamIDs::MOD_ENV4_SUSTAIN };
        const char* envR[4] = { ParamIDs::MOD_ENV1_RELEASE,  ParamIDs::MOD_ENV2_RELEASE,
                                ParamIDs::MOD_ENV3_RELEASE,  ParamIDs::MOD_ENV4_RELEASE };
        const char* envDp[4] = { ParamIDs::MOD_ENV1_DEPTH,   ParamIDs::MOD_ENV2_DEPTH,
                                 ParamIDs::MOD_ENV3_DEPTH,   ParamIDs::MOD_ENV4_DEPTH };
        for (int i = 0; i < 4; ++i)
        {
            lfoRate[i]    = p (lfoR[i]);
            lfoShape[i]   = p (lfoW[i]);
            lfoDepth[i]   = p (lfoD[i]);
            modEnvA[i]    = p (envA[i]);
            modEnvD[i]    = p (envD[i]);
            modEnvS[i]    = p (envS[i]);
            modEnvR[i]    = p (envR[i]);
            modEnvDepth[i] = p (envDp[i]);
        }

        masterGain = p (ParamIDs::MASTER_GAIN);
    }

    static float get (const std::atomic<float>* ptr) noexcept
    {
        return ptr->load (std::memory_order_relaxed);
    }

    std::atomic<float>* osc1Waveform; std::atomic<float>* osc1Drive;
    std::atomic<float>* osc2RootNote; std::atomic<float>* osc2Tune;  std::atomic<float>* osc2Loop;
    std::atomic<float>* osc2Gain;     std::atomic<float>* osc2Hpf;   std::atomic<float>* osc2Lpf;
    std::atomic<float>* oscMix;

    std::atomic<float>* filterCutoff; std::atomic<float>* filterResonance; std::atomic<float>* filterModeBlend;
    std::atomic<float>* filterDrive;  std::atomic<float>* filterEnvAmount;
    std::atomic<float>* filterEnvA;   std::atomic<float>* filterEnvD;
    std::atomic<float>* filterEnvS;   std::atomic<float>* filterEnvR;

    std::atomic<float>* ampA; std::atomic<float>* ampD; std::atomic<float>* ampS; std::atomic<float>* ampR;

    std::atomic<float>* voiceMode;    std::atomic<float>* voiceGlide;
    std::atomic<float>* unisonVoices; std::atomic<float>* unisonDetune; std::atomic<float>* unisonSpread;
    std::atomic<float>* voiceDrift;

    std::atomic<float>* lfoRate[4];  std::atomic<float>* lfoShape[4]; std::atomic<float>* lfoDepth[4];
    std::atomic<float>* modEnvA[4];  std::atomic<float>* modEnvD[4];
    std::atomic<float>* modEnvS[4];  std::atomic<float>* modEnvR[4];
    std::atomic<float>* modEnvDepth[4];

    std::atomic<float>* masterGain;

    JUCE_DECLARE_NON_COPYABLE (ParamCache)
};

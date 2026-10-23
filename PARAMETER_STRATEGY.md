# v0idsynth Parameter Strategy

**Status:** Locked — do not change existing parameter IDs or version hints after the first user release.

---

## Version Hint Scheme

Every APVTS parameter carries a `juce::ParameterID { "id.string", N }` where `N` is the version hint.

**Rule:** All parameters shipped in v1.0 use version hint `1`.

When new parameters must be added in a future release (v1.x or v2.x), they MUST use version hint `2` or higher — one hint increment per release that adds parameters. This ensures new parameters sort AFTER all existing parameters in AU hosts that use parameter order rather than ID strings.

| Plugin Release | Version Hint to Use | Notes |
|----------------|---------------------|-------|
| v1.0 (initial) | `1` | All 56 parameters in this table |
| v1.x additions | `2` | Any new parameter added post-ship |
| v2.x additions | `3` | Any parameter added in v2 beyond v1.x |

**Never assign a lower hint to a new parameter than the current maximum hint in use.** The AU parameter sorter uses hint as the primary sort key; a lower hint inserts the new parameter before existing ones, corrupting automation.

---

## WARNING: Parameter IDs and Ordering Are Permanent

> **CRITICAL:** Once a user has opened v0idsynth in Logic Pro X and recorded automation, that automation data is indexed by **parameter order** as sorted by (version hint, parameter ID string hash). Any change that alters this order silently remaps automation to the wrong parameters in every saved project — with no error message and no recovery.

**The following actions are FORBIDDEN after the initial release:**

1. Changing the string value of any existing `ParamIDs::*` constant
2. Changing the version hint of any existing parameter
3. Removing an existing parameter (remove the DSP wiring; keep the parameter registered as a no-op)
4. Adding a new parameter with a version hint equal to or lower than the current maximum

**The following actions ARE safe:**

- Adding new parameters with version hint `N+1` where `N` is the current max hint
- Changing the human-readable parameter label (the second string arg to `AudioParameterFloat`, etc.)
- Changing the parameter range — with caution (it does not break automation but may sound different)
- Changing the default value — with caution (only affects new instances, not saved presets)

---

## Naming Convention

All parameter IDs use **hierarchical dot notation**: `section.parameter` or `section.subsection.parameter`.

| Prefix | Section | Phase |
|--------|---------|-------|
| `osc1.` | Analog oscillator (Osc1) | Phase 1 |
| `osc2.` | Sample oscillator (Osc2) | Phase 2 |
| `osc.` | Oscillator mixer | Phase 1/2 |
| `filter.` | Filter controls | Phase 1 |
| `filter.env.` | Filter envelope (ADSR) | Phase 1 |
| `amp.env.` | Amplitude envelope (ADSR) | Phase 1 |
| `voice.` | Voice / polyphony settings | Phase 2 |
| `mod.lfo1.`–`mod.lfo4.` | LFO 1–4 | Phase 3 |
| `mod.env1.`–`mod.env4.` | Modulation envelopes 1–4 | Phase 3 |
| `master.` | Master output controls | Phase 1 |

---

## Complete Parameter Table

All parameters use version hint `1`.

| # | Parameter ID | Type | Range | Default | Phase | Notes |
|---|-------------|------|-------|---------|-------|-------|
| 1 | `osc1.waveform` | Choice | Saw/Square/Triangle/Sine | Saw | 1 | Osc1 waveform selector |
| 2 | `osc1.drive` | Float | 0–1 (step 0.01) | 0.0 | 1 | Pre-filter soft-clip drive on Osc1 output |
| 3 | `osc2.sample_id` | Int | 0–127 | 0 | 2 | Sample bank slot index for Osc2 |
| 4 | `osc.mix` | Float | 0–1 (step 0.01) | 0.0 | 2 | 0=Osc1 only, 1=Osc2 only |
| 5 | `filter.cutoff` | Float | 20–20000 Hz (log, centre 1000) | 20000 | 1 | Filter cutoff frequency |
| 6 | `filter.resonance` | Float | 0–1 (step 0.01) | 0.0 | 1 | Filter resonance / Q |
| 7 | `filter.mode_blend` | Float | 0–3 (step 0.01) | 0.0 | 1 | 0=LP, 1=HP, 2=BP, 3=Notch; crossfadeable |
| 8 | `filter.drive` | Float | 0–1 (step 0.01) | 0.0 | 1 | LadderFilter saturation amount |
| 9 | `filter.env_amount` | Float | -1–+1 (step 0.01) | 0.0 | 1 | Filter envelope depth (bipolar) |
| 10 | `filter.env.attack` | Float | 0.001–10 s (log, centre 0.3) | 0.01 | 1 | Filter envelope attack time |
| 11 | `filter.env.decay` | Float | 0.001–10 s (log, centre 0.3) | 0.3 | 1 | Filter envelope decay time |
| 12 | `filter.env.sustain` | Float | 0–1 (step 0.01) | 0.7 | 1 | Filter envelope sustain level |
| 13 | `filter.env.release` | Float | 0.001–10 s (log, centre 0.5) | 0.5 | 1 | Filter envelope release time |
| 14 | `amp.env.attack` | Float | 0.001–10 s (log, centre 0.3) | 0.01 | 1 | Amplitude envelope attack time |
| 15 | `amp.env.decay` | Float | 0.001–10 s (log, centre 0.3) | 0.3 | 1 | Amplitude envelope decay time |
| 16 | `amp.env.sustain` | Float | 0–1 (step 0.01) | 0.8 | 1 | Amplitude envelope sustain level |
| 17 | `amp.env.release` | Float | 0.001–10 s (log, centre 0.5) | 0.5 | 1 | Amplitude envelope release time |
| 18 | `voice.mode` | Choice | Poly/Mono/Unison | Poly | 2 | Polyphony mode selector |
| 19 | `voice.glide` | Float | 0–1 (step 0.01) | 0.0 | 2 | Portamento time |
| 20 | `voice.unison_voices` | Int | 1–8 | 4 | 2 | Number of unison voices |
| 21 | `voice.unison_detune` | Float | 0–1 (step 0.01) | 0.3 | 2 | Unison detune spread |
| 22 | `voice.unison_spread` | Float | 0–1 (step 0.01) | 0.5 | 2 | Unison stereo spread |
| 23 | `voice.drift` | Float | 0–1 (step 0.01) | 0.1 | 2 | Per-voice pitch drift intensity |
| 24 | `mod.lfo1.rate` | Float | 0.01–20 Hz (log, centre 2.0) | 1.0 | 3 | LFO1 rate |
| 25 | `mod.lfo1.waveform` | Choice | Sine/Triangle/Saw/Square/S&H | Sine | 3 | LFO1 waveform |
| 26 | `mod.lfo1.depth` | Float | 0–1 (step 0.01) | 0.5 | 3 | LFO1 depth |
| 27 | `mod.lfo2.rate` | Float | 0.01–20 Hz (log, centre 2.0) | 1.0 | 3 | LFO2 rate |
| 28 | `mod.lfo2.waveform` | Choice | Sine/Triangle/Saw/Square/S&H | Sine | 3 | LFO2 waveform |
| 29 | `mod.lfo2.depth` | Float | 0–1 (step 0.01) | 0.5 | 3 | LFO2 depth |
| 30 | `mod.lfo3.rate` | Float | 0.01–20 Hz (log, centre 2.0) | 1.0 | 3 | LFO3 rate |
| 31 | `mod.lfo3.waveform` | Choice | Sine/Triangle/Saw/Square/S&H | Sine | 3 | LFO3 waveform |
| 32 | `mod.lfo3.depth` | Float | 0–1 (step 0.01) | 0.5 | 3 | LFO3 depth |
| 33 | `mod.lfo4.rate` | Float | 0.01–20 Hz (log, centre 2.0) | 1.0 | 3 | LFO4 rate |
| 34 | `mod.lfo4.waveform` | Choice | Sine/Triangle/Saw/Square/S&H | Sine | 3 | LFO4 waveform |
| 35 | `mod.lfo4.depth` | Float | 0–1 (step 0.01) | 0.5 | 3 | LFO4 depth |
| 36 | `mod.env1.attack` | Float | 0.001–10 s (log, centre 0.3) | 0.01 | 3 | Mod Env1 attack |
| 37 | `mod.env1.decay` | Float | 0.001–10 s (log, centre 0.3) | 0.3 | 3 | Mod Env1 decay |
| 38 | `mod.env1.sustain` | Float | 0–1 (step 0.01) | 0.5 | 3 | Mod Env1 sustain |
| 39 | `mod.env1.release` | Float | 0.001–10 s (log, centre 0.5) | 0.5 | 3 | Mod Env1 release |
| 40 | `mod.env2.attack` | Float | 0.001–10 s (log, centre 0.3) | 0.01 | 3 | Mod Env2 attack |
| 41 | `mod.env2.decay` | Float | 0.001–10 s (log, centre 0.3) | 0.3 | 3 | Mod Env2 decay |
| 42 | `mod.env2.sustain` | Float | 0–1 (step 0.01) | 0.5 | 3 | Mod Env2 sustain |
| 43 | `mod.env2.release` | Float | 0.001–10 s (log, centre 0.5) | 0.5 | 3 | Mod Env2 release |
| 44 | `mod.env3.attack` | Float | 0.001–10 s (log, centre 0.3) | 0.01 | 3 | Mod Env3 attack |
| 45 | `mod.env3.decay` | Float | 0.001–10 s (log, centre 0.3) | 0.3 | 3 | Mod Env3 decay |
| 46 | `mod.env3.sustain` | Float | 0–1 (step 0.01) | 0.5 | 3 | Mod Env3 sustain |
| 47 | `mod.env3.release` | Float | 0.001–10 s (log, centre 0.5) | 0.5 | 3 | Mod Env3 release |
| 48 | `mod.env4.attack` | Float | 0.001–10 s (log, centre 0.3) | 0.01 | 3 | Mod Env4 attack |
| 49 | `mod.env4.decay` | Float | 0.001–10 s (log, centre 0.3) | 0.3 | 3 | Mod Env4 decay |
| 50 | `mod.env4.sustain` | Float | 0–1 (step 0.01) | 0.5 | 3 | Mod Env4 sustain |
| 51 | `mod.env4.release` | Float | 0.001–10 s (log, centre 0.5) | 0.5 | 3 | Mod Env4 release |
| 52 | `master.gain` | Float | -60–+12 dB (step 0.1) | 0.0 | 1 | Master output gain |

**Total: 52 parameter declarations, all version hint 1.**

Note: Parameters labeled Phase 2 and Phase 3 above are declared in the layout immediately (Phase 1) but their DSP wiring is implemented in those later phases. This upfront declaration locks the parameter order before any Logic automation is saved.

---

## Instructions for Adding Parameters in Future Versions

1. **Choose the next version hint.** If current max is 1, new parameters use hint 2. If current max is 2, use 3.

2. **Append new ParamIDs constants** to the `ParamIDs` namespace at the bottom — never insert between existing entries.

3. **Append new parameters** to `createParameterLayout()` at the end of the function — never insert between existing parameters.

4. **Update this document** — add rows to the table above with the new hint value and release version.

5. **Bump the plugin version number** in CMakeLists.txt and in any release notes.

6. **Never remove a parameter.** If a parameter is no longer needed, keep it registered but disconnect it from DSP. Removing a parameter corrupts existing projects.

---

*Locked: 2026-03-31 — initial v1.0 release parameter set*
*Document owner: Implementation team*
*Source of truth: Source/Parameters.h*

cmake_minimum_required(VERSION 3.22)
project(v0idsynth VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "Minimum macOS version")

add_subdirectory(JUCE)

juce_add_plugin(v0idsynth
    COMPANY_NAME                "madebymolo"
    PLUGIN_MANUFACTURER_CODE    Lmos
    PLUGIN_CODE                 V0sy
    FORMATS                     AU VST3
    PRODUCT_NAME                "v0idsynth"
    IS_SYNTH                    TRUE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    AU_MAIN_TYPE                kAudioUnitType_MusicDevice
    COPY_PLUGIN_AFTER_BUILD     TRUE
)

target_sources(v0idsynth PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/DSP/AnalogOscillator.cpp
    Source/DSP/MultiModeFilter.cpp
    Source/Engine/SampleBank.cpp
    Source/Engine/SampleLoader.cpp
    Source/Engine/SynthVoice.cpp
    Source/Preset/PresetManager.cpp
    Source/Preset/PresetBrowserComponent.cpp
    Source/UI/FilterSectionComponent.cpp
    Source/UI/MasterSectionComponent.cpp
    Source/UI/ModRoutingDisplayComponent.cpp
    Source/UI/ModSourceColumnComponent.cpp
    Source/UI/OscillatorSectionComponent.cpp
    Source/UI/V0idSynthLookAndFeel.cpp
    Source/UI/VoiceAmpEnvSectionComponent.cpp
)

target_compile_definitions(v0idsynth PUBLIC
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(v0idsynth
    PRIVATE
        juce::juce_audio_plugin_client
        juce::juce_audio_processors
        juce::juce_audio_basics
        juce::juce_audio_formats
        juce::juce_dsp
        juce::juce_gui_basics
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# =============================================================================
# Factory preset library -- embedded as BinaryData (Phase 4 / PRE-01)
# =============================================================================
juce_add_binary_data(v0idsynth_PresetData
    SOURCES
        Resources/Presets/01_pad_fluorescent_hum.xml
        Resources/Presets/02_pad_wrong_room.xml
        Resources/Presets/03_pad_exit_sign_flicker.xml
        Resources/Presets/04_bass_parking_structure_b.xml
        Resources/Presets/05_bass_basement_pulse.xml
        Resources/Presets/06_bass_low_voltage.xml
        Resources/Presets/07_lead_static_remembers.xml
        Resources/Presets/08_lead_call_box_drone.xml
        Resources/Presets/09_lead_dial_tone_ghost.xml
        Resources/Presets/10_fx_something_followed_me_home.xml
        Resources/Presets/11_fx_pneumatic_door.xml
        Resources/Presets/12_fx_security_camera_buzz.xml
        Resources/Presets/13_organic_breath_in_walls.xml
        Resources/Presets/14_organic_carpet_static.xml
        Resources/Presets/15_organic_old_intercom.xml
        Resources/Presets/16_sample_template_drone_bed.xml
        Resources/Presets/17_sample_template_string_haunt.xml
        Resources/Presets/18_sample_template_field_recording.xml
        Resources/Presets/19_pad_lobby_at_3am.xml
        Resources/Presets/20_pad_the_painting_is_watching.xml
        Resources/Presets/21_bass_weight_of_the_ceiling.xml
        Resources/Presets/22_bass_server_room_hum.xml
        Resources/Presets/23_lead_unanswered.xml
        Resources/Presets/24_lead_the_long_corridor.xml
        Resources/Presets/25_fx_cassette_that_wont_rewind.xml
        Resources/Presets/26_fx_maintenance_required.xml
        Resources/Presets/27_organic_water_finding_its_level.xml
        Resources/Presets/28_organic_flies_in_the_fixture.xml
        Resources/Presets/29_sample_template_choir_of_ghosts.xml
        Resources/Presets/30_sample_template_mechanical_clock.xml
        Resources/Presets/31_pad_fever_architecture.xml
        Resources/Presets/32_bass_maintenance_shaft.xml
        Resources/Presets/33_lead_frequency_sickness.xml
        Resources/Presets/34_fx_signal_decay.xml
        Resources/Presets/35_organic_mechanical_lung.xml
        Resources/Presets/36_sample_template_borrowed_tape.xml
        Resources/Presets/37_fx_wrong_number.xml
        Resources/Presets/38_fx_body_in_the_walls.xml
        Resources/Presets/39_fx_teeth_on_concrete.xml
)

target_link_libraries(v0idsynth PRIVATE v0idsynth_PresetData)

# =============================================================================
# Unit tests (Catch2)
# =============================================================================

include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.2
)
FetchContent_MakeAvailable(Catch2)

add_executable(v0idsynth_tests
    Tests/AnalogOscillatorTest.cpp
    Tests/MultiModeFilterTest.cpp
    Tests/SampleOscillatorTest.cpp
    Tests/SampleBankTest.cpp
    Tests/SynthVoiceTest.cpp
    Tests/VoiceLFOTest.cpp
    Tests/ModulationEngineTest.cpp
    Tests/ModMatrixTableTest.cpp
    Tests/PresetManagerTest.cpp
    Tests/PresetIntegrationTest.cpp
    Source/DSP/AnalogOscillator.cpp
    Source/DSP/MultiModeFilter.cpp
    Source/Engine/SampleBank.cpp
    Source/Engine/SampleLoader.cpp
    Source/Engine/SynthVoice.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Preset/PresetManager.cpp
    Source/Preset/PresetBrowserComponent.cpp
    Source/UI/FilterSectionComponent.cpp
    Source/UI/MasterSectionComponent.cpp
    Source/UI/ModRoutingDisplayComponent.cpp
    Source/UI/ModSourceColumnComponent.cpp
    Source/UI/OscillatorSectionComponent.cpp
    Source/UI/V0idSynthLookAndFeel.cpp
    Source/UI/VoiceAmpEnvSectionComponent.cpp
)

target_link_libraries(v0idsynth_tests PRIVATE
    Catch2::Catch2WithMain
    juce::juce_core
    juce::juce_events
    juce::juce_dsp
    juce::juce_audio_basics
    juce::juce_audio_formats
    juce::juce_audio_processors
)

target_link_libraries(v0idsynth_tests PRIVATE v0idsynth_PresetData juce::juce_gui_basics)

target_include_directories(v0idsynth_tests PRIVATE Source)

set_target_properties(v0idsynth_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Release"
)

# NOTE: Post-build auval validation is intentionally omitted from CMake.
# auval requires the system AU cache to be refreshed after plugin installation,
# which cannot reliably happen in the same build step. Run manually:
#   auval -v aumu V0sy Lmos
# after a fresh build + macOS AU cache refresh (or reboot).

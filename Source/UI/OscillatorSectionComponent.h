#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include "SectionLabel.h"

class V0idSynthAudioProcessor;

class OscillatorSectionComponent : public juce::Component
{
public:
    explicit OscillatorSectionComponent (V0idSynthAudioProcessor& processor);
    ~OscillatorSectionComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void setTooltips();
    void refreshSampleName(); // re-read the processor's loaded sample (message thread)

private:
    V0idSynthAudioProcessor& processorRef;

    class SampleDropButton : public juce::Component,
                             public juce::FileDragAndDropTarget
    {
    public:
        std::function<void(const juce::File&)> onFile;

        void setLoadedName (const juce::String& name) { loadedName = name; repaint(); }

        void paint         (juce::Graphics&) override;
        void mouseDown     (const juce::MouseEvent&) override;

        bool isInterestedInFileDrag (const juce::StringArray& files) override;
        void fileDragEnter (const juce::StringArray&, int, int) override { dragOver = true;  repaint(); }
        void fileDragExit  (const juce::StringArray&)           override { dragOver = false; repaint(); }
        void filesDropped  (const juce::StringArray& files, int, int) override;

    private:
        juce::String loadedName { "drag / click" };
        bool dragOver = false;
        std::unique_ptr<juce::FileChooser> chooser;
    };

    SectionLabel     header { "OSC" };
    juce::ComboBox   osc1WaveformCombo;
    juce::Slider     osc1DriveKnob, oscMixKnob;
    SampleDropButton sampleBtn;

    // Sample tuning + loop strip
    juce::Slider     rootNoteSlider;   // 0–127 coarse tuning; shows note name
    juce::Slider     tuneSlider;       // ±100 cents fine tuning
    juce::TextButton loopButton;       // toggles sample loop

    // Sample gain + filter strip
    juce::Slider     gainSlider;       // -24 to +24 dB sample gain
    juce::Slider     hpfSlider;        // 20–2000 Hz high-pass cutoff
    juce::Slider     lpfSlider;        // 500–20000 Hz low-pass cutoff

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> osc1WaveformAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   osc1DriveAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   oscMixAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   rootNoteAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   tuneAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>   loopAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   gainAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   hpfAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   lpfAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OscillatorSectionComponent)
};

#include "OscillatorSectionComponent.h"
#include "V0idSynthLookAndFeel.h"
#include "Tooltips.h"
#include "../Parameters.h"
#include "../PluginProcessor.h"

// ─── SampleDropButton ────────────────────────────────────────────────────────

void OscillatorSectionComponent::SampleDropButton::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced (1.0f);

    // Sharp corners per UI-SPEC (corner radius 0px — hardware instrument aesthetic)
    g.setColour (V0idColors::surface);
    g.fillRect (bounds);

    g.setColour (dragOver ? V0idColors::accentMid() : V0idColors::accentDim());
    g.drawRect (bounds, 1.0f);

    g.setFont (juce::Font (juce::FontOptions ("Courier New", 8.0f, juce::Font::plain)));
    g.setColour (V0idColors::accentLabel());
    g.drawText ("SAMPLE", bounds.withHeight (12.0f).translated (0.0f, 2.0f),
                juce::Justification::centred);

    g.setFont (juce::Font (juce::FontOptions ("Courier New", 9.0f, juce::Font::plain)));
    g.setColour (dragOver ? V0idColors::accent : V0idColors::accentLabel());
    g.drawText (loadedName, bounds.withTrimmedTop (16.0f),
                juce::Justification::centred, true);
}

void OscillatorSectionComponent::SampleDropButton::mouseDown (const juce::MouseEvent&)
{
    chooser = std::make_unique<juce::FileChooser> (
        "Load sample", juce::File(), "*.wav,*.aiff,*.aif,*.mp3,*.flac,*.ogg,*.m4a,*.aac,*.caf");

    chooser->launchAsync (
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [safeThis = juce::Component::SafePointer<SampleDropButton> (this)] (const juce::FileChooser& fc)
        {
            // Editor may have closed while the dialog was open.
            if (safeThis == nullptr) return;
            auto f = fc.getResult();
            if (f.existsAsFile() && safeThis->onFile)
                safeThis->onFile (f);
        });
}

bool OscillatorSectionComponent::SampleDropButton::isInterestedInFileDrag (
    const juce::StringArray& files)
{
    static const juce::StringArray kExts { ".wav", ".aiff", ".aif", ".mp3", ".flac", ".ogg", ".m4a", ".aac", ".caf" };
    for (auto& path : files)
        if (kExts.contains (juce::File (path).getFileExtension().toLowerCase()))
            return true;
    return false;
}

void OscillatorSectionComponent::SampleDropButton::filesDropped (
    const juce::StringArray& files, int, int)
{
    dragOver = false;
    repaint();
    // Take the first file we can actually decode, not blindly files[0].
    for (auto& path : files)
    {
        juce::File f (path);
        if (f.existsAsFile() && isInterestedInFileDrag (juce::StringArray (path)))
        {
            if (onFile) onFile (f);
            return;
        }
    }
}

// ─── OscillatorSectionComponent ──────────────────────────────────────────────

OscillatorSectionComponent::OscillatorSectionComponent (V0idSynthAudioProcessor& processor)
    : processorRef (processor)
{
    addAndMakeVisible (header);

    osc1WaveformCombo.setName ("WAVE");
    addAndMakeVisible (osc1WaveformCombo);

    osc1DriveKnob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    osc1DriveKnob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    osc1DriveKnob.setName ("DRIVE");
    addAndMakeVisible (osc1DriveKnob);

    addAndMakeVisible (sampleBtn);

    oscMixKnob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    oscMixKnob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    oscMixKnob.setName ("MIX");
    addAndMakeVisible (oscMixKnob);

    osc1WaveformCombo.addItem ("Saw",      1);
    osc1WaveformCombo.addItem ("Square",   2);
    osc1WaveformCombo.addItem ("Triangle", 3);
    osc1WaveformCombo.addItem ("Sine",     4);

    osc1WaveformAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        processor.apvts, ParamIDs::OSC1_WAVEFORM, osc1WaveformCombo);
    osc1DriveAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.apvts, ParamIDs::OSC1_DRIVE, osc1DriveKnob);
    oscMixAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.apvts, ParamIDs::OSC_MIX, oscMixKnob);

    // Root note slider — linear bar, 0–127, displays note name (Logic convention: 60 = C3)
    rootNoteSlider.setSliderStyle (juce::Slider::LinearBar);
    rootNoteSlider.setRange (0.0, 127.0, 1.0);
    rootNoteSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    rootNoteSlider.setName ("ROOT");
    rootNoteSlider.textFromValueFunction = [](double v) -> juce::String
    {
        static const char* names[] = { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" };
        int note   = juce::roundToInt (v);
        int octave = note / 12 - 2;  // Logic: MIDI 60 = C3
        return juce::String (names[note % 12]) + juce::String (octave);
    };
    addAndMakeVisible (rootNoteSlider);

    // Tune slider — linear bar, ±100 cents fine tuning
    tuneSlider.setSliderStyle (juce::Slider::LinearBar);
    tuneSlider.setRange (-100.0, 100.0, 0.01);
    tuneSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    tuneSlider.setName ("TUNE");
    tuneSlider.textFromValueFunction = [](double v) -> juce::String
    {
        return (v >= 0.0 ? "+" : "") + juce::String (v, 1) + "c";
    };
    addAndMakeVisible (tuneSlider);

    // Loop toggle button
    loopButton.setButtonText ("LOOP");
    loopButton.setClickingTogglesState (true);
    loopButton.setName ("LOOP");
    addAndMakeVisible (loopButton);

    rootNoteAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.apvts, ParamIDs::OSC2_ROOT_NOTE, rootNoteSlider);
    tuneAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.apvts, ParamIDs::OSC2_TUNE, tuneSlider);
    loopAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        processor.apvts, ParamIDs::OSC2_LOOP, loopButton);

    // Gain slider — linear bar, -24 to +24 dB
    gainSlider.setSliderStyle (juce::Slider::LinearBar);
    gainSlider.setRange (-24.0, 24.0, 0.1);
    gainSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    gainSlider.setName ("GAIN");
    gainSlider.textFromValueFunction = [](double v) -> juce::String
    {
        return (v >= 0.0 ? "+" : "") + juce::String (v, 1) + "dB";
    };
    addAndMakeVisible (gainSlider);

    // HPF slider — log-skewed, 20–2000 Hz
    hpfSlider.setSliderStyle (juce::Slider::LinearBar);
    hpfSlider.setRange (20.0, 2000.0, 0.1);
    hpfSlider.setSkewFactorFromMidPoint (200.0);
    hpfSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    hpfSlider.setName ("HPF");
    hpfSlider.textFromValueFunction = [](double v) -> juce::String
    {
        return juce::String (juce::roundToInt (v)) + "Hz";
    };
    addAndMakeVisible (hpfSlider);

    // LPF slider — log-skewed, 500–20000 Hz
    lpfSlider.setSliderStyle (juce::Slider::LinearBar);
    lpfSlider.setRange (500.0, 20000.0, 0.1);
    lpfSlider.setSkewFactorFromMidPoint (5000.0);
    lpfSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    lpfSlider.setName ("LPF");
    lpfSlider.textFromValueFunction = [](double v) -> juce::String
    {
        return (v >= 1000.0) ? (juce::String (v / 1000.0, 1) + "kHz")
                             : (juce::String (juce::roundToInt (v)) + "Hz");
    };
    addAndMakeVisible (lpfSlider);

    gainAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.apvts, ParamIDs::OSC2_GAIN, gainSlider);
    hpfAtt  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.apvts, ParamIDs::OSC2_HPF, hpfSlider);
    lpfAtt  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.apvts, ParamIDs::OSC2_LPF, lpfSlider);

    // Wire the sample button: load on a background thread; the processor publishes
    // the buffer and fires onSampleChanged (-> refreshSampleName) on the message thread.
    sampleBtn.onFile = [this, &proc = processor] (const juce::File& f)
    {
        sampleBtn.setLoadedName ("loading" + juce::String (juce::CharPointer_UTF8 ("\xe2\x80\xa6")));
        proc.loadSampleAsync (f);
    };
    refreshSampleName();
}

void OscillatorSectionComponent::refreshSampleName()
{
    const auto name = processorRef.getLoadedSampleName();
    sampleBtn.setLoadedName (name.isNotEmpty() ? name : juce::String ("drag / click"));
}

OscillatorSectionComponent::~OscillatorSectionComponent()
{
    osc1WaveformAtt.reset();
    osc1DriveAtt.reset();
    oscMixAtt.reset();
    rootNoteAtt.reset();
    tuneAtt.reset();
    loopAtt.reset();
    gainAtt.reset();
    hpfAtt.reset();
    lpfAtt.reset();
}

void OscillatorSectionComponent::setTooltips()
{
    osc1WaveformCombo.setTooltip (juce::String (Tooltips::forParamId (ParamIDs::OSC1_WAVEFORM)));
    osc1DriveKnob    .setTooltip (juce::String (Tooltips::forParamId (ParamIDs::OSC1_DRIVE)));
    oscMixKnob       .setTooltip (juce::String (Tooltips::forParamId (ParamIDs::OSC_MIX)));
}

void OscillatorSectionComponent::paint (juce::Graphics& g)
{
    g.setColour (V0idColors::surface);
    g.fillRect (getLocalBounds());
}

void OscillatorSectionComponent::resized()
{
    auto bounds = getLocalBounds();
    header.setBounds (bounds.removeFromTop (16));

    const int stripH = 22;
    const int gap    = 4;

    // Bottom strip: GAIN | HPF | LPF
    auto gainStrip = bounds.removeFromBottom (stripH);
    bounds.removeFromBottom (gap);

    const int gainW = 60;
    const int hpfW  = 74;
    gainSlider.setBounds (gainStrip.removeFromLeft (gainW));
    gainStrip.removeFromLeft (gap);
    hpfSlider .setBounds (gainStrip.removeFromLeft (hpfW));
    gainStrip.removeFromLeft (gap);
    lpfSlider .setBounds (gainStrip);   // remaining width

    // Second strip: ROOT | TUNE | LOOP
    auto strip = bounds.removeFromBottom (stripH);
    bounds.removeFromBottom (gap);

    const int loopW = 40;
    const int tuneW = 58;
    const int rootW = strip.getWidth() - loopW - tuneW - gap * 2;

    rootNoteSlider.setBounds (strip.removeFromLeft (rootW));
    strip.removeFromLeft (gap);
    tuneSlider    .setBounds (strip.removeFromLeft (tuneW));
    strip.removeFromLeft (gap);
    loopButton    .setBounds (strip);

    // Main 2×2 grid (above both strips)
    bounds.removeFromBottom (gap);

    const int halfW = (bounds.getWidth()  - gap) / 2;
    const int halfH = (bounds.getHeight() - gap) / 2;
    const int startX = bounds.getX();
    const int startY = bounds.getY();

    osc1WaveformCombo.setBounds (startX,               startY,               halfW, halfH);
    osc1DriveKnob    .setBounds (startX + halfW + gap,  startY,               halfW, halfH);
    sampleBtn        .setBounds (startX,               startY + halfH + gap,  halfW, halfH);
    oscMixKnob       .setBounds (startX + halfW + gap,  startY + halfH + gap, halfW, halfH);
}

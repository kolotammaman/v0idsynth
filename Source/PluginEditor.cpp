#include "PluginEditor.h"
#include "UI/Tooltips.h"

V0idSynthAudioProcessorEditor::V0idSynthAudioProcessorEditor(V0idSynthAudioProcessor& p)
    : AudioProcessorEditor(&p),
      processorRef(p),
      browser(p),
      oscSection(p),
      filterSection(p.apvts),
      voiceAmpSection(p.apvts),
      masterSection(p.apvts),
      routingDisplay(p.apvts),
      lfo1Column(p.apvts, ModSourceColumnComponent::Kind::LFO, 1, [&p]() { p.recompileModMatrix(); }),
      lfo2Column(p.apvts, ModSourceColumnComponent::Kind::LFO, 2, [&p]() { p.recompileModMatrix(); }),
      lfo3Column(p.apvts, ModSourceColumnComponent::Kind::LFO, 3, [&p]() { p.recompileModMatrix(); }),
      lfo4Column(p.apvts, ModSourceColumnComponent::Kind::LFO, 4, [&p]() { p.recompileModMatrix(); }),
      env1Column(p.apvts, ModSourceColumnComponent::Kind::Env, 1, [&p]() { p.recompileModMatrix(); }),
      env2Column(p.apvts, ModSourceColumnComponent::Kind::Env, 2, [&p]() { p.recompileModMatrix(); }),
      env3Column(p.apvts, ModSourceColumnComponent::Kind::Env, 3, [&p]() { p.recompileModMatrix(); }),
      env4Column(p.apvts, ModSourceColumnComponent::Kind::Env, 4, [&p]() { p.recompileModMatrix(); })
{
    // CRITICAL: setLookAndFeel BEFORE any addAndMakeVisible so children inherit on first paint.
    setLookAndFeel(&lookAndFeel);
    setOpaque(true); // paint() fills every pixel — lets JUCE skip whatever is behind us

    addAndMakeVisible(browser);
    addAndMakeVisible(oscSection);
    addAndMakeVisible(filterSection);
    addAndMakeVisible(voiceAmpSection);
    addAndMakeVisible(masterSection);
    addAndMakeVisible(routingDisplay);
    addAndMakeVisible(lfo1Column);
    addAndMakeVisible(lfo2Column);
    addAndMakeVisible(lfo3Column);
    addAndMakeVisible(lfo4Column);
    addAndMakeVisible(env1Column);
    addAndMakeVisible(env2Column);
    addAndMakeVisible(env3Column);
    addAndMakeVisible(env4Column);

    // Wire tooltips on every parameter control (UI-03).
    oscSection      .setTooltips();
    filterSection   .setTooltips();
    voiceAmpSection .setTooltips();
    masterSection   .setTooltips();
    lfo1Column.setTooltips();
    lfo2Column.setTooltips();
    lfo3Column.setTooltips();
    lfo4Column.setTooltips();
    env1Column.setTooltips();
    env2Column.setTooltips();
    env3Column.setTooltips();
    env4Column.setTooltips();

    processorRef.onPresetLoaded = [this]()
    {
        browser.refresh();
        lfo1Column.refreshDestCombo();
        lfo2Column.refreshDestCombo();
        lfo3Column.refreshDestCombo();
        lfo4Column.refreshDestCombo();
        env1Column.refreshDestCombo();
        env2Column.refreshDestCombo();
        env3Column.refreshDestCombo();
        env4Column.refreshDestCombo();
    };

    processorRef.onSampleChanged = [this]() { oscSection.refreshSampleName(); };

    setSize(1000, 700);
}

V0idSynthAudioProcessorEditor::~V0idSynthAudioProcessorEditor()
{
    processorRef.onPresetLoaded = nullptr;
    processorRef.onSampleChanged = nullptr;
    setLookAndFeel(nullptr); // CRITICAL: detach before lookAndFeel goes out of scope
}

void V0idSynthAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(V0idColors::background);

    // Horizontal section dividers between strips (UI-SPEC "Section Dividers")
    g.setColour(V0idColors::accentDivider());
    g.fillRect(0,  36, getWidth(), 1); // below preset strip
    g.fillRect(0, 256, getWidth(), 1); // below synth engine strip
    g.fillRect(0, 336, getWidth(), 1); // below routing display
}

void V0idSynthAudioProcessorEditor::paintOverChildren(juce::Graphics& g)
{
    // Drawn over the children because the section panels fill opaque surface
    // rects that would otherwise hide the texture. The overlay is static, so it
    // is rasterised once per size/scale and blitted — every knob repaint used to
    // re-stroke 233 scan lines and a full-window radial gradient.
    const float scale = g.getInternalContext().getPhysicalPixelScaleFactor();
    if (crtOverlay.isNull() || ! juce::approximatelyEqual(crtOverlayScale, scale))
    {
        crtOverlayScale = scale;
        renderCrtOverlay();
    }
    g.drawImage(crtOverlay, getLocalBounds().toFloat());
}

void V0idSynthAudioProcessorEditor::renderCrtOverlay()
{
    const int pw = juce::roundToInt((float)getWidth()  * crtOverlayScale);
    const int ph = juce::roundToInt((float)getHeight() * crtOverlayScale);
    crtOverlay = juce::Image(juce::Image::ARGB, juce::jmax(1, pw), juce::jmax(1, ph), true);

    juce::Graphics g(crtOverlay);
    g.addTransform(juce::AffineTransform::scale(crtOverlayScale));

    // CRT scan lines — horizontal 1px lines every 3px at 7% black (UI-SPEC:
    // "cover the full 1000x700 editor bounds").
    g.setColour(V0idColors::scanLineAlpha());
    for (int y = 0; y < getHeight(); y += 3)
        g.fillRect(0, y, getWidth(), 1);

    // Vignette — corners fall off into black like a curved CRT face. Flat until
    // ~72% of the radius so the centre of the panel stays untouched.
    const auto bounds = getLocalBounds().toFloat();
    const auto centre = bounds.getCentre();
    juce::ColourGradient vignette(juce::Colours::transparentBlack, centre.x, centre.y,
                                  juce::Colours::black.withAlpha(0.30f),
                                  bounds.getRight(), bounds.getBottom(), true);
    vignette.addColour(0.72, juce::Colours::transparentBlack);
    g.setGradientFill(vignette);
    g.fillRect(bounds);

    // Faint phosphor wash along the top edge — the glass catching the tube's glow.
    juce::ColourGradient glare(V0idColors::accent.withAlpha(0.035f), centre.x, 0.0f,
                               juce::Colours::transparentBlack, centre.x, 120.0f, false);
    g.setGradientFill(glare);
    g.fillRect(bounds.withHeight(120.0f));
}

void V0idSynthAudioProcessorEditor::resized()
{
    crtOverlay = {}; // rebuilt lazily at the new size
    auto bounds = getLocalBounds();

    // Row 1: preset browser
    browser.setBounds(bounds.removeFromTop(PresetBrowserComponent::kPreferredHeight));

    // Row 2: synth engine strip (220px)
    auto engineStrip = bounds.removeFromTop(220);
    oscSection      .setBounds(engineStrip.removeFromLeft(220));
    filterSection   .setBounds(engineStrip.removeFromLeft(280));
    voiceAmpSection .setBounds(engineStrip.removeFromLeft(420));
    masterSection   .setBounds(engineStrip);                       // remaining 80px

    // Row 3: routing display (80px)
    routingDisplay.setBounds(bounds.removeFromTop(80));

    // Row 4: 8 equal-width mod source columns (125px each, 364px tall)
    auto modStrip = bounds; // 364px tall, full 1000px width
    lfo1Column.setBounds(modStrip.removeFromLeft(125));
    lfo2Column.setBounds(modStrip.removeFromLeft(125));
    lfo3Column.setBounds(modStrip.removeFromLeft(125));
    lfo4Column.setBounds(modStrip.removeFromLeft(125));
    env1Column.setBounds(modStrip.removeFromLeft(125));
    env2Column.setBounds(modStrip.removeFromLeft(125));
    env3Column.setBounds(modStrip.removeFromLeft(125));
    env4Column.setBounds(modStrip);                                // remaining 125px
}

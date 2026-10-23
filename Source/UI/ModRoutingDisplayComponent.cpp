#include "ModRoutingDisplayComponent.h"
#include "V0idSynthLookAndFeel.h"

ModRoutingDisplayComponent::ModRoutingDisplayComponent (juce::AudioProcessorValueTreeState& apvts)
    : apvtsRef (apvts)
{
    apvts.state.addListener (this);
}

ModRoutingDisplayComponent::~ModRoutingDisplayComponent()
{
    apvtsRef.state.removeListener (this);
}

void ModRoutingDisplayComponent::paint (juce::Graphics& g)
{
    // Background — full-width seam; use background so the row merges visually.
    g.fillAll (V0idColors::background);

    // UI-SPEC dot matrix spec:
    //   dot diameter 7px, cell 12x12, gap 3px, left margin 72px (row labels), top margin 8px
    constexpr int dotDiameter = 7;
    constexpr int cellSize    = 12;
    constexpr int gap         = 3;
    constexpr int leftMargin  = 72; // row label area width
    constexpr int topMargin   = 8;
    constexpr int matrixWidth  = kNumSources * (cellSize + gap) - gap; // 132
    constexpr int matrixHeight = kNumDests   * (cellSize + gap) - gap; // 72
    constexpr int blockWidth   = leftMargin + matrixWidth;             // 204

    const int blockX  = (getWidth() - blockWidth) / 2; // centered horizontally
    const int matrixX = blockX + leftMargin;
    const int matrixY = topMargin;

    // Read active routes from apvts.state ModMatrix child
    bool active[kNumSources][kNumDests] = {};
    auto matrixNode = apvtsRef.state.getChildWithName ("ModMatrix");
    if (matrixNode.isValid())
    {
        for (int i = 0; i < matrixNode.getNumChildren(); ++i)
        {
            auto routeNode = matrixNode.getChild (i);
            const int src = static_cast<int> (routeNode.getProperty ("source", -1));
            const int dst = static_cast<int> (routeNode.getProperty ("dest",   -1));
            if (src >= 0 && src < kNumSources && dst >= 0 && dst < kNumDests)
                active[src][dst] = true;
        }
    }

    // Column labels (top, all-caps, 9pt mono, accentLabel)
    const char* srcLabels[kNumSources] = {
        "LFO1", "LFO2", "LFO3", "LFO4", "ENV1", "ENV2", "ENV3", "ENV4", "DRFT" };
    g.setColour (V0idColors::accentLabel());
    g.setFont (juce::Font (juce::FontOptions ("Courier New", 9.0f, juce::Font::plain)));
    for (int c = 0; c < kNumSources; ++c)
    {
        const int x = matrixX + c * (cellSize + gap);
        g.drawText (srcLabels[c], x, 0, cellSize, topMargin,
                    juce::Justification::centred, false);
    }

    // Row labels (left of matrix)
    const char* destLabels[kNumDests] = { "CUTOFF", "RESON", "PITCH", "MIX", "AMP" };
    for (int r = 0; r < kNumDests; ++r)
    {
        const int y = matrixY + r * (cellSize + gap);
        g.drawText (destLabels[r], blockX, y, leftMargin - 4, cellSize,
                    juce::Justification::centredRight, false);
    }

    // Dots — live routes get a phosphor halo so they read as "lit" from across
    // the panel, matching the knob bloom.
    for (int r = 0; r < kNumDests; ++r)
    {
        for (int c = 0; c < kNumSources; ++c)
        {
            const float cx = static_cast<float> (matrixX + c * (cellSize + gap) + (cellSize - dotDiameter) / 2);
            const float cy = static_cast<float> (matrixY + r * (cellSize + gap) + (cellSize - dotDiameter) / 2);
            if (active[c][r])
            {
                g.setColour (V0idColors::accent.withAlpha (0.20f));
                g.fillEllipse (cx - 2.5f, cy - 2.5f, dotDiameter + 5.0f, dotDiameter + 5.0f);
            }
            g.setColour (active[c][r] ? V0idColors::accent : V0idColors::accentDim());
            g.fillEllipse (cx, cy, static_cast<float> (dotDiameter), static_cast<float> (dotDiameter));
        }
    }

    juce::ignoreUnused (matrixHeight);

    // -------------------------------------------------------------------------
    // Synth title — rendered in the right black space next to the matrix.
    // The blurred glow is expensive, so it lives in a cached image that is only
    // rebuilt on resize / display-scale change.
    // -------------------------------------------------------------------------
    const int titleX = matrixX + matrixWidth + 20;
    const int titleW = getWidth() - titleX - 8;
    if (titleW <= 0)
        return;

    const float scale = g.getInternalContext().getPhysicalPixelScaleFactor();
    if (titleCache.isNull() || ! juce::approximatelyEqual (titleCacheScale, scale))
    {
        titleCacheScale = scale;
        renderTitle (titleW, getHeight());
    }
    g.drawImage (titleCache, juce::Rectangle<int> (titleX, 0, titleW, getHeight()).toFloat());
}

void ModRoutingDisplayComponent::renderTitle (int width, int height)
{
    const float scale = titleCacheScale;
    const int pw = juce::roundToInt ((float) width  * scale);
    const int ph = juce::roundToInt ((float) height * scale);

    const juce::String title ("v0idsynth");
    const juce::Font titleFont (juce::FontOptions ("Courier New", 34.0f * scale, juce::Font::bold));
    constexpr float pad = 10.0f; // room on the left for the bloom to spread
    const auto textArea = juce::Rectangle<float> (pad * scale, 0.0f, (float) pw - pad * scale, (float) ph);

    auto drawText = [&] (juce::Image& img, juce::Colour c, float dx)
    {
        juce::Graphics tg (img);
        tg.setColour (c);
        tg.setFont (titleFont);
        tg.drawText (title, textArea.translated (dx * scale, 0.0f), juce::Justification::centredLeft, false);
    };

    // Wide phosphor bloom + tighter inner glow, both gaussian-blurred.
    juce::Image wide (juce::Image::ARGB, pw, ph, true);
    drawText (wide, V0idColors::accent, 0.0f);
    juce::ImageConvolutionKernel wideKernel (juce::roundToInt (13.0f * scale) | 1);
    wideKernel.createGaussianBlur (5.0f * scale);
    wideKernel.applyToImage (wide, wide, wide.getBounds());

    juce::Image tight (juce::Image::ARGB, pw, ph, true);
    drawText (tight, V0idColors::accent, 0.0f);
    juce::ImageConvolutionKernel tightKernel (juce::roundToInt (5.0f * scale) | 1);
    tightKernel.createGaussianBlur (1.5f * scale);
    tightKernel.applyToImage (tight, tight, tight.getBounds());

    titleCache = juce::Image (juce::Image::ARGB, pw, ph, true);
    {
        juce::Graphics g (titleCache);
        g.setOpacity (0.55f);
        g.drawImageAt (wide, 0, 0);
        g.setOpacity (0.70f);
        g.drawImageAt (tight, 0, 0);
    }
    // Faint horizontal ghost — the trailing echo of a mistuned CRT.
    drawText (titleCache, V0idColors::accent.withAlpha (0.10f), 3.0f);
    drawText (titleCache, V0idColors::accent, 0.0f);
}

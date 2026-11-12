#include "PluginEditor.h"

namespace pal
{
    const juce::Colour bgTop    { 0xff17122b };
    const juce::Colour bgBottom { 0xff2a1838 };
    const juce::Colour panel    { 0x22ffffff };
    const juce::Colour gold     { 0xffe9b44c };
    const juce::Colour saffron  { 0xffe67e36 };
    const juce::Colour rose     { 0xffd0688a };
    const juce::Colour jade     { 0xff86c9a8 };
    const juce::Colour cream    { 0xfff3e8d2 };
    const juce::Colour dim      { 0xffa597ba };
    const juce::Colour shadow   { 0xff0c0918 };
}

//==============================================================================
// Use the Mac's own Georgia / Avenir Next; fall back safely if a font is ever missing
static juce::String pickFont (std::initializer_list<const char*> wanted, const juce::String& fallback)
{
    static const juce::StringArray installed = juce::Font::findAllTypefaceNames();
    for (auto* w : wanted)
        if (installed.contains (w)) return w;
    return fallback;
}

juce::Font SadhanaLookAndFeel::serif (float size, bool bold)
{
    static const juce::String name = pickFont ({ "Georgia", "Baskerville", "DejaVu Serif" }, juce::Font::getDefaultSerifFontName());
    return juce::Font (juce::FontOptions (name, size, bold ? juce::Font::bold : juce::Font::plain));
}
juce::Font SadhanaLookAndFeel::sans (float size, bool bold)
{
    static const juce::String name = pickFont ({ "Avenir Next", "Avenir", "Helvetica Neue", "DejaVu Sans" }, juce::Font::getDefaultSansSerifFontName());
    return juce::Font (juce::FontOptions (name, size, bold ? juce::Font::bold : juce::Font::plain));
}

SadhanaLookAndFeel::SadhanaLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, pal::cream);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Label::textColourId, pal::dim);
    setColour (juce::ComboBox::textColourId, pal::cream);
    setColour (juce::ComboBox::backgroundColourId, pal::shadow.withAlpha (0.6f));
    setColour (juce::ComboBox::arrowColourId, pal::gold);
    setColour (juce::ComboBox::outlineColourId, pal::gold.withAlpha (0.35f));
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff1d1633));
    setColour (juce::PopupMenu::textColourId, pal::cream);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, pal::saffron.withAlpha (0.35f));
    setColour (juce::PopupMenu::highlightedTextColourId, pal::cream);
}

void SadhanaLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                           float start, float end, juce::Slider& s)
{
    auto bounds = juce::Rectangle<int> (x, y, w, h).toFloat().reduced (6.0f);
    const float r = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto c = bounds.getCentre();
    const float angle = start + pos * (end - start);
    const bool hover = s.isMouseOverOrDragging();

    // soft halo
    g.setGradientFill (juce::ColourGradient (pal::saffron.withAlpha (hover ? 0.22f : 0.12f), c,
                                             juce::Colours::transparentBlack, c.translated (r * 1.1f, 0), true));
    g.fillEllipse (juce::Rectangle<float> (r * 2.2f, r * 2.2f).withCentre (c));

    // disc
    g.setColour (pal::shadow.withAlpha (0.85f));
    g.fillEllipse (juce::Rectangle<float> (r * 1.5f, r * 1.5f).withCentre (c));
    g.setColour (pal::gold.withAlpha (0.25f));
    g.drawEllipse (juce::Rectangle<float> (r * 1.5f, r * 1.5f).withCentre (c), 1.0f);

    // track
    juce::Path track;
    track.addCentredArc (c.x, c.y, r * 0.95f, r * 0.95f, 0.0f, start, end, true);
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.strokePath (track, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // value arc
    juce::Path val;
    val.addCentredArc (c.x, c.y, r * 0.95f, r * 0.95f, 0.0f, start, angle, true);
    g.setGradientFill (juce::ColourGradient (pal::rose, c.translated (-r, r), pal::gold, c.translated (r, -r), false));
    g.strokePath (val, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // glowing bead
    const juce::Point<float> bead (c.x + r * 0.95f * std::sin (angle), c.y - r * 0.95f * std::cos (angle));
    g.setColour (pal::gold.withAlpha (0.35f));
    g.fillEllipse (juce::Rectangle<float> (12.0f, 12.0f).withCentre (bead));
    g.setColour (pal::cream);
    g.fillEllipse (juce::Rectangle<float> (5.0f, 5.0f).withCentre (bead));

    // tiny lotus dot in the middle
    g.setColour (pal::gold.withAlpha (0.5f + 0.5f * pos));
    g.fillEllipse (juce::Rectangle<float> (4.0f, 4.0f).withCentre (c));
}

void SadhanaLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.5f);
    const bool on = b.getToggleState();
    if (on)
    {
        g.setGradientFill (juce::ColourGradient (pal::saffron.withAlpha (0.55f), r.getTopLeft(), pal::rose.withAlpha (0.45f), r.getBottomRight(), false));
        g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
    }
    else
    {
        g.setColour (pal::shadow.withAlpha (over ? 0.8f : 0.55f));
        g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
    }
    g.setColour (pal::gold.withAlpha (on ? 0.9f : (over ? 0.55f : 0.3f)));
    g.drawRoundedRectangle (r, r.getHeight() * 0.5f, 1.0f);
}

void SadhanaLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool)
{
    g.setFont (sans (13.0f, b.getToggleState()));
    g.setColour (b.getToggleState() ? pal::cream : pal::dim.brighter (0.2f));
    g.drawFittedText (b.getButtonText(), b.getLocalBounds().reduced (6, 0), juce::Justification::centred, 1);
}

void SadhanaLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (1.0f);
    g.setColour (pal::shadow.withAlpha (box.isMouseOver() ? 0.85f : 0.6f));
    g.fillRoundedRectangle (r, 8.0f);
    g.setColour (pal::gold.withAlpha (box.isMouseOver() ? 0.6f : 0.3f));
    g.drawRoundedRectangle (r, 8.0f, 1.0f);
    juce::Path arrow;
    const float ax = (float) w - 16.0f, ay = (float) h * 0.5f;
    arrow.addTriangle (ax - 4, ay - 2, ax + 4, ay - 2, ax, ay + 3);
    g.setColour (pal::gold);
    g.fillPath (arrow);
}

juce::Font SadhanaLookAndFeel::getComboBoxFont (juce::ComboBox&) { return sans (14.0f); }
juce::Font SadhanaLookAndFeel::getPopupMenuFont() { return sans (15.0f); }
void SadhanaLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& l)
{
    l.setBounds (10, 1, box.getWidth() - 30, box.getHeight() - 2);
    l.setFont (getComboBoxFont (box));
    l.setJustificationType (juce::Justification::centredLeft);
}

//==============================================================================
void Mandala::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const auto c = b.getCentre();
    const float R = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f - 6.0f;
    const float glow = juce::jlimit (0.0f, 1.0f, 0.35f + droneGlow * 0.8f + inputGlow * 0.6f);

    // aura
    g.setGradientFill (juce::ColourGradient (pal::saffron.withAlpha (0.18f * glow), c, juce::Colours::transparentBlack,
                                             c.translated (R, 0), true));
    g.fillEllipse (juce::Rectangle<float> (R * 2.0f, R * 2.0f).withCentre (c));

    // rotating lotus petals: two counter-rotating rings
    auto petals = [&] (int count, float radius, float len, float width, float rot, juce::Colour col)
    {
        for (int i = 0; i < count; ++i)
        {
            const float a = rot + juce::MathConstants<float>::twoPi * (float) i / (float) count;
            juce::Path p;
            p.addEllipse (-width * 0.5f, -len, width, len);
            p.applyTransform (juce::AffineTransform::translation (0, -radius + len * 0.5f)
                                  .rotated (a).translated (c.x, c.y));
            g.setColour (col);
            g.strokePath (p, juce::PathStrokeType (1.0f));
        }
    };
    petals (16, R * 0.86f, R * 0.30f, R * 0.11f, phase * 0.25f, pal::gold.withAlpha (0.22f + 0.25f * glow));
    petals (8,  R * 0.60f, R * 0.30f, R * 0.16f, -phase * 0.4f, pal::rose.withAlpha (0.25f + 0.3f * glow));

    // flower of life core
    const float fr = R * 0.17f;
    g.setColour (pal::cream.withAlpha (0.10f + 0.15f * glow));
    g.drawEllipse (juce::Rectangle<float> (fr * 2, fr * 2).withCentre (c), 1.0f);
    for (int i = 0; i < 6; ++i)
    {
        const float a = phase * 0.15f + juce::MathConstants<float>::twoPi * (float) i / 6.0f;
        g.drawEllipse (juce::Rectangle<float> (fr * 2, fr * 2).withCentre (c.translated (fr * std::cos (a), fr * std::sin (a))), 1.0f);
    }

    // scale wheel: 12 positions around the outside, tonic at the top
    const float wheelR = R * 0.98f;
    g.setColour (pal::gold.withAlpha (0.25f));
    g.drawEllipse (juce::Rectangle<float> (wheelR * 2, wheelR * 2).withCentre (c), 1.0f);
    for (int st = 0; st < 12; ++st)
    {
        const bool inScale = std::find (scale.begin(), scale.end(), st) != scale.end();
        const float a = juce::MathConstants<float>::twoPi * (float) st / 12.0f;
        const juce::Point<float> p (c.x + wheelR * std::sin (a), c.y - wheelR * std::cos (a));
        const float dotR = st == 0 ? 9.0f : (inScale ? 6.5f : 3.0f);
        if (inScale)
        {
            g.setColour ((st == 0 ? pal::saffron : pal::gold).withAlpha (0.3f));
            g.fillEllipse (juce::Rectangle<float> (dotR * 3.2f, dotR * 3.2f).withCentre (p));
        }
        g.setColour (st == 0 ? pal::saffron : (inScale ? pal::gold : pal::dim.withAlpha (0.4f)));
        g.fillEllipse (juce::Rectangle<float> (dotR * 2, dotR * 2).withCentre (p));

        if (inScale)
        {
            const juce::Point<float> lp (c.x + (wheelR - 24.0f) * std::sin (a), c.y - (wheelR - 24.0f) * std::cos (a));
            g.setColour (pal::cream.withAlpha (0.85f));
            g.setFont (SadhanaLookAndFeel::sans (12.0f, st == 0));
            g.drawText (sadhana::noteName (key + st + nameOffset), juce::Rectangle<float> (30, 16).withCentre (lp),
                        juce::Justification::centred);
        }
    }

    // tuner bead: where the note being played sits on the wheel
    if (detectedMidi > 0.0f)
    {
        float rel = std::fmod (detectedMidi - (float) key, 12.0f);
        if (rel < 0) rel += 12.0f;
        const float a = juce::MathConstants<float>::twoPi * rel / 12.0f;
        const juce::Point<float> p (c.x + wheelR * std::sin (a), c.y - wheelR * std::cos (a));
        const auto col = onScale && std::abs (cents) < 12.0f ? pal::jade : (onScale ? pal::gold : pal::rose);
        g.setColour (col.withAlpha (0.35f));
        g.fillEllipse (juce::Rectangle<float> (30.0f, 30.0f).withCentre (p));
        g.setColour (col);
        g.drawEllipse (juce::Rectangle<float> (18.0f, 18.0f).withCentre (p), 2.0f);

        // big note in the centre
        const int midi = (int) std::lround (detectedMidi);
        g.setColour (pal::cream);
        g.setFont (SadhanaLookAndFeel::serif (40.0f));
        g.drawText (sadhana::noteName (midi + nameOffset), juce::Rectangle<float> (120, 46).withCentre (c.translated (0, -8)),
                    juce::Justification::centred);
        g.setFont (SadhanaLookAndFeel::sans (13.0f));
        g.setColour (col);
        const int ci = juce::roundToInt (cents);
        g.drawText (std::abs (ci) <= 3 ? juce::String ("in tune") : (ci > 0 ? "+" : "") + juce::String (ci) + " cents",
                    juce::Rectangle<float> (140, 18).withCentre (c.translated (0, 22)), juce::Justification::centred);
    }
    else
    {
        g.setColour (pal::cream.withAlpha (droneOn ? 0.95f : 0.5f));
        g.setFont (SadhanaLookAndFeel::serif (40.0f));
        g.drawText (sadhana::noteName (key + nameOffset), juce::Rectangle<float> (120, 46).withCentre (c.translated (0, -10)),
                    juce::Justification::centred);
        g.setFont (SadhanaLookAndFeel::sans (12.0f));
        g.setColour (pal::dim);
        g.drawFittedText (scaleName, juce::Rectangle<int> (150, 32).withCentre (c.translated (0, 22).toInt()),
                          juce::Justification::centred, 2);
    }
}

//==============================================================================
SadhanaEditor::Knob& SadhanaEditor::makeKnob (const char* id, const juce::String& hint)
{
    auto k = std::make_unique<Knob>();
    k->slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k->slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 18);
    k->slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    k->slider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    k->slider.setColour (juce::Slider::textBoxTextColourId, pal::cream.withAlpha (0.85f));
    k->slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    k->slider.setDoubleClickReturnValue (true, (double) proc.apvts.getParameter (id)->convertFrom0to1 (
                                                 proc.apvts.getParameter (id)->getDefaultValue()));
    k->label.setText (proc.apvts.getParameter (id)->getName (32), juce::dontSendNotification);
    k->label.setJustificationType (juce::Justification::centred);
    k->label.setFont (SadhanaLookAndFeel::serif (15.0f));
    k->label.setColour (juce::Label::textColourId, pal::cream);
    k->attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, k->slider);
    addAndMakeVisible (k->slider);
    addAndMakeVisible (k->label);
    setHint (&k->slider, hint);
    knobs.push_back (std::move (k));
    return *knobs.back();
}

SadhanaEditor::Choice& SadhanaEditor::makeChoice (const char* id, const juce::String& title, const juce::String& hint)
{
    auto ch = std::make_unique<Choice>();
    if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter (id)))
        ch->box.addItemList (p->choices, 1);
    ch->label.setText (title, juce::dontSendNotification);
    ch->label.setFont (SadhanaLookAndFeel::sans (11.0f, true));
    ch->label.setColour (juce::Label::textColourId, pal::dim);
    ch->attach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, id, ch->box);
    addAndMakeVisible (ch->box);
    addAndMakeVisible (ch->label);
    setHint (&ch->box, hint);
    choices.push_back (std::move (ch));
    return *choices.back();
}

void SadhanaEditor::setHint (juce::Component* c, const juce::String& text)
{
    hints[c] = text;
    c->addMouseListener (this, false);
}

SadhanaEditor::SadhanaEditor (SadhanaProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);

    // presets: "offerings"
    const auto& ps = SadhanaProcessor::presets();
    for (int i = 0; i < (int) ps.size(); ++i)
    {
        auto* b = presetButtons.add (new juce::TextButton (ps[(size_t) i].name));
        b->setClickingTogglesState (false);
        b->onClick = [this, i] { proc.applyPreset (i); hintText = SadhanaProcessor::presets()[(size_t) i].blurb; refreshFromParams(); };
        setHint (b, juce::String (ps[(size_t) i].name) + ": " + ps[(size_t) i].blurb);
        addAndMakeVisible (b);
    }

    // voice
    for (auto* b : { &clarinetBtn, &bassBtn })
    {
        addAndMakeVisible (b);
        b->onClick = [this, b]
        {
            auto* prm = proc.apvts.getParameter ("instrument");
            prm->setValueNotifyingHost (prm->convertTo0to1 (b == &bassBtn ? 1.0f : 0.0f));
        };
    }
    setHint (&clarinetBtn, "Clarinet: shapes the low cut and body for the Bb clarinet.");
    setHint (&bassBtn, "Bass Clarinet: keeps the deep low notes and moves the body lower.");

    makeKnob ("warmth", "Warmth: gentle analogue-style glow that rounds off harsh edges. 30-50% is lovely.");
    makeKnob ("body",   "Body: fullness of the wood. Up for a rounder tone, down if it sounds boomy.");
    makeKnob ("air",    "Air: a soft lift of the breath and sparkle on top. Go easy, a little goes a long way.");
    makeKnob ("smooth", "Smooth: evens out loud and soft notes so every phrase sits nicely over the drone.");

    // drone
    droneBtn.setClickingTogglesState (true);
    addAndMakeVisible (droneBtn);
    droneBtn.onClick = [this]
    {
        auto* prm = proc.apvts.getParameter ("droneOn");
        prm->setValueNotifyingHost (droneBtn.getToggleState() ? 1.0f : 0.0f);
    };
    setHint (&droneBtn, "Drone: switch the living drone on or off. It fades in and out gently.");
    for (auto* b : { &lowBtn, &midBtn })
    {
        addAndMakeVisible (b);
        b->onClick = [this, b]
        {
            auto* prm = proc.apvts.getParameter ("octave");
            prm->setValueNotifyingHost (prm->convertTo0to1 (b == &midBtn ? 1.0f : 0.0f));
        };
    }
    setHint (&lowBtn, "Low: the drone sits deep, like a tanpura's lowest string.");
    setHint (&midBtn, "Mid: the drone sits closer to the clarinet's own range.");

    makeKnob ("droneLevel", "Drone Level: how loud the drone is under your playing.");
    makeKnob ("overtones",  "Overtones: the natural harmonics of the drone note that belong to your scale. They shimmer and sing slowly.");
    makeKnob ("texture",    "Texture: left is pure singing-bowl tones, right is a reedy shruti box / harmonium.");

    // centre choices
    makeChoice ("key",    "DRONE KEY",   "Key: the note of the drone (concert pitch). The wheel and the drone follow it.");
    makeChoice ("scale",  "SCALE",       "Scale: lights up the notes on the wheel, and chooses which overtones sing in the drone.");
    makeChoice ("tuning", "TUNING",      "Tuning: concert A = 440, or A = 432 which many sound healers love.");
    makeChoice ("names",  "NOTE NAMES",  "Note names: show notes as concert pitch, or as written for a Bb clarinet / bass clarinet.");

    // space
    makeKnob ("space",  "Space: a warm temple reverb around the clarinet and drone.");
    makeKnob ("echo",   "Echo: soft repeats in time with your Ableton tempo. Keep it subtle.");
    makeKnob ("output", "Output: overall volume. Double-click any knob to reset it.");

    addAndMakeVisible (mandala);
    setHint (&mandala, "The wheel shows your scale. Play a note and the bead shows where it lands: green when it's in tune.");

    defaultHint = "Welcome. Pick an offering above, then play. Hover over anything to learn what it does.";
    hintText = defaultHint;

    proc.apvts.addParameterListener ("instrument", this);
    proc.apvts.addParameterListener ("droneOn", this);
    proc.apvts.addParameterListener ("octave", this);
    proc.apvts.addParameterListener ("key", this);
    proc.apvts.addParameterListener ("scale", this);
    proc.apvts.addParameterListener ("names", this);

    refreshFromParams();
    setSize (940, 760);
    startTimerHz (30);
}

SadhanaEditor::~SadhanaEditor()
{
    for (auto* id : { "instrument", "droneOn", "octave", "key", "scale", "names" })
        proc.apvts.removeParameterListener (id, this);
    setLookAndFeel (nullptr);
}

void SadhanaEditor::parameterChanged (const juce::String&, float)
{
    juce::Component::SafePointer<SadhanaEditor> safe (this);
    juce::MessageManager::callAsync ([safe] { if (safe != nullptr) safe->refreshFromParams(); });
}

void SadhanaEditor::refreshFromParams()
{
    auto v = [this] (const char* id) { return proc.apvts.getRawParameterValue (id)->load(); };
    const int inst = (int) v ("instrument");
    clarinetBtn.setToggleState (inst == 0, juce::dontSendNotification);
    bassBtn.setToggleState (inst == 1, juce::dontSendNotification);
    droneBtn.setToggleState (v ("droneOn") > 0.5f, juce::dontSendNotification);
    droneBtn.setButtonText (v ("droneOn") > 0.5f ? "Drone On" : "Drone Off");
    const int oct = (int) v ("octave");
    lowBtn.setToggleState (oct == 0, juce::dontSendNotification);
    midBtn.setToggleState (oct == 1, juce::dontSendNotification);

    const int key = (int) v ("key");
    const int sc = (int) v ("scale");
    const bool written = v ("names") > 0.5f;
    mandala.key = key;
    mandala.scale = sadhana::scales()[(size_t) sc].degrees;
    mandala.scaleName = sadhana::scales()[(size_t) sc].name;
    mandala.nameOffset = written ? 2 : 0;
    mandala.droneOn = v ("droneOn") > 0.5f;

    readsText = juce::String ("Drone in ") + sadhana::noteName (key) + " concert  =  "
                + sadhana::noteName (key + 2) + " on your clarinet";

    const int cp = proc.getCurrentProgram();
    for (int i = 0; i < presetButtons.size(); ++i)
        presetButtons[i]->setToggleState (i == cp, juce::dontSendNotification);
    repaint();
}

void SadhanaEditor::runTuner()
{
    proc.getEngine().copyTunerSnapshot (tunerSnap);
    float clarity = 0.0f;
    const float hz = sadhana::detectPitch (tunerSnap, proc.getSampleRateSafe(), clarity);

    if (hz > 0.0f && clarity > 0.8f)
    {
        silentFrames = 0;
        smoothedHz = (smoothedHz > 0.0f && std::abs (std::log2 (hz / smoothedHz)) < 0.04f) ? smoothedHz * 0.6f + hz * 0.4f : hz;
        const float aRef = proc.apvts.getRawParameterValue ("tuning")->load() > 0.5f ? 432.0f : 440.0f;
        const float midi = 69.0f + 12.0f * std::log2 (smoothedHz / aRef);
        mandala.detectedMidi = midi;
        const int nearest = (int) std::lround (midi);
        mandala.cents = (midi - (float) nearest) * 100.0f;
        const int rel = ((nearest - mandala.key) % 12 + 12) % 12;
        mandala.onScale = std::find (mandala.scale.begin(), mandala.scale.end(), rel) != mandala.scale.end();
    }
    else if (++silentFrames > 6)
    {
        mandala.detectedMidi = -1.0f;
        smoothedHz = 0.0f;
    }
}

void SadhanaEditor::timerCallback()
{
    mandala.phase += 0.012f;
    mandala.droneGlow = mandala.droneGlow * 0.9f + 0.1f * juce::jlimit (0.0f, 1.0f, proc.getEngine().getDroneLevel() * 4.0f);
    mandala.inputGlow = mandala.inputGlow * 0.85f + 0.15f * juce::jlimit (0.0f, 1.0f, proc.getEngine().getInputLevel() * 3.0f);

    static int tick = 0;
    if (++tick % 2 == 0)
        runTuner();
    mandala.repaint();
}

void SadhanaEditor::mouseEnter (const juce::MouseEvent& e)
{
    for (auto* c = e.eventComponent; c != nullptr; c = c->getParentComponent())
        if (auto it = hints.find (c); it != hints.end()) { hintText = it->second; repaint (hintArea); return; }
}

void SadhanaEditor::mouseExit (const juce::MouseEvent&)
{
    hintText = defaultHint;
    repaint (hintArea);
}

//==============================================================================
void SadhanaEditor::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    g.setGradientFill (juce::ColourGradient (pal::bgTop, 0, 0, pal::bgBottom, 0, b.getHeight(), false));
    g.fillAll();

    // faint stars / incense specks
    juce::Random rnd (108);
    for (int i = 0; i < 90; ++i)
    {
        g.setColour (pal::cream.withAlpha (0.03f + 0.07f * rnd.nextFloat()));
        const float s = 1.0f + rnd.nextFloat() * 1.6f;
        g.fillEllipse (rnd.nextFloat() * b.getWidth(), rnd.nextFloat() * b.getHeight(), s, s);
    }

    // title
    g.setColour (pal::gold);
    g.setFont (SadhanaLookAndFeel::serif (30.0f));
    g.drawText ("S A D H A N A", 0, 14, getWidth(), 36, juce::Justification::centred);
    g.setColour (pal::dim);
    g.setFont (SadhanaLookAndFeel::sans (12.0f));
    g.drawText (juce::CharPointer_UTF8 ("clarinet offering  \xc2\xb7  living drone"), 0, 48, getWidth(), 18, juce::Justification::centred);

    // panels
    auto panel = [&] (juce::Rectangle<int> r, const juce::String& title)
    {
        g.setColour (pal::panel);
        g.fillRoundedRectangle (r.toFloat(), 14.0f);
        g.setColour (pal::gold.withAlpha (0.18f));
        g.drawRoundedRectangle (r.toFloat(), 14.0f, 1.0f);
        g.setColour (pal::saffron);
        g.setFont (SadhanaLookAndFeel::sans (11.0f, true));
        g.drawText (title, r.reduced (16, 10).removeFromTop (14), juce::Justification::centredLeft);
    };
    panel (voiceArea, "YOUR VOICE");
    panel (droneArea, "THE DRONE");
    panel (spaceArea, "THE SPACE");

    // what the clarinettist reads
    if (proc.apvts.getRawParameterValue ("names")->load() > 0.5f)
    {
        g.setColour (pal::gold.withAlpha (0.85f));
        g.setFont (SadhanaLookAndFeel::sans (12.0f));
        g.drawText (readsText, mandala.getX() - 40, mandala.getBottom() + 2, mandala.getWidth() + 80, 16, juce::Justification::centred);
    }

    // hint line
    g.setColour (pal::shadow.withAlpha (0.5f));
    g.fillRoundedRectangle (hintArea.toFloat(), 10.0f);
    g.setColour (pal::cream.withAlpha (0.9f));
    g.setFont (SadhanaLookAndFeel::sans (14.0f));
    g.drawFittedText (hintText, hintArea.reduced (16, 4), juce::Justification::centred, 2);
}

void SadhanaEditor::resized()
{
    auto area = getLocalBounds().reduced (18);
    area.removeFromTop (56);

    // presets row
    auto row = area.removeFromTop (34);
    const int pw = row.getWidth() / juce::jmax (1, presetButtons.size());
    for (auto* pb : presetButtons)
        pb->setBounds (row.removeFromLeft (pw).reduced (4, 2));
    area.removeFromTop (14);

    hintArea = area.removeFromBottom (44);
    area.removeFromBottom (12);

    spaceArea = area.removeFromBottom (158);
    area.removeFromBottom (12);

    voiceArea = area.removeFromLeft (250);
    droneArea = area.removeFromRight (250);
    auto centre = area.reduced (12, 0);

    auto knobCell = [] (Knob& k, juce::Rectangle<int> cell)
    {
        k.label.setBounds (cell.removeFromTop (18));
        k.slider.setBounds (cell);
    };

    // voice panel: instrument toggle + 2x2 knobs
    {
        auto r = voiceArea.reduced (14);
        r.removeFromTop (20);
        auto t = r.removeFromTop (30);
        clarinetBtn.setBounds (t.removeFromLeft (t.getWidth() / 2).reduced (2, 0));
        bassBtn.setBounds (t.reduced (2, 0));
        r.removeFromTop (8);
        const int cw = r.getWidth() / 2, ch = r.getHeight() / 2;
        for (int i = 0; i < 4; ++i)
            knobCell (*knobs[(size_t) i], { r.getX() + (i % 2) * cw, r.getY() + (i / 2) * ch, cw, ch });
    }

    // drone panel: on/off + octave, then 3 knobs
    {
        auto r = droneArea.reduced (14);
        r.removeFromTop (20);
        auto t = r.removeFromTop (30);
        droneBtn.setBounds (t.removeFromLeft (t.getWidth() / 2).reduced (2, 0));
        lowBtn.setBounds (t.removeFromLeft (t.getWidth() / 2).reduced (2, 0));
        midBtn.setBounds (t.reduced (2, 0));
        r.removeFromTop (8);
        const int cw = r.getWidth() / 2, ch = r.getHeight() / 2;
        knobCell (*knobs[4], { r.getX(), r.getY(), cw, ch });
        knobCell (*knobs[5], { r.getX() + cw, r.getY(), cw, ch });
        knobCell (*knobs[6], { r.getX() + cw / 2, r.getY() + ch, cw, ch });
    }

    // centre: mandala + choices
    {
        auto r = centre;
        auto choiceArea = r.removeFromBottom (96);
        r.removeFromBottom (18);
        const int m = juce::jmin (r.getWidth(), r.getHeight());
        mandala.setBounds (r.withSizeKeepingCentre (m, m));

        const int cw = choiceArea.getWidth() / 2;
        for (int i = 0; i < 4; ++i)
        {
            juce::Rectangle<int> cell (choiceArea.getX() + (i % 2) * cw, choiceArea.getY() + (i / 2) * 48, cw, 48);
            cell = cell.reduced (4, 2);
            choices[(size_t) i]->label.setBounds (cell.removeFromTop (14));
            choices[(size_t) i]->box.setBounds (cell.removeFromTop (28));
        }
    }

    // space panel: 3 knobs in a row
    {
        auto r = spaceArea.reduced (14);
        r.removeFromTop (16);
        auto knobsRow = r.withSizeKeepingCentre (juce::jmin (r.getWidth(), 560), r.getHeight());
        const int cw = knobsRow.getWidth() / 3;
        for (int i = 0; i < 3; ++i)
            knobCell (*knobs[(size_t) (7 + i)], knobsRow.removeFromLeft (cw));
    }
}

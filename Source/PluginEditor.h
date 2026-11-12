#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

//==============================================================================
class SadhanaLookAndFeel : public juce::LookAndFeel_V4
{
public:
    SadhanaLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float start, float end, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool over, bool down) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getPopupMenuFont() override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;

    static juce::Font serif (float size, bool bold = false);
    static juce::Font sans (float size, bool bold = false);
};

//==============================================================================
class Mandala : public juce::Component
{
public:
    void paint (juce::Graphics&) override;

    float phase = 0.0f;            // slow rotation
    float droneGlow = 0.0f;        // 0..1
    float inputGlow = 0.0f;        // 0..1
    int key = 2;
    std::vector<int> scale;        // semitones above tonic
    int nameOffset = 0;            // +2 when showing Bb clarinet names
    bool droneOn = true;

    // tuner
    float detectedMidi = -1.0f;    // continuous MIDI pitch (concert), <0 if none
    bool onScale = false;
    float cents = 0.0f;
    juce::String scaleName;
};

//==============================================================================
class SadhanaEditor : public juce::AudioProcessorEditor,
                      private juce::Timer,
                      private juce::AudioProcessorValueTreeState::Listener
{
public:
    explicit SadhanaEditor (SadhanaProcessor&);
    ~SadhanaEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseEnter (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    void parameterChanged (const juce::String&, float) override;
    void refreshFromParams();
    void runTuner();

    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attach;
    };
    Knob& makeKnob (const char* id, const juce::String& hint);

    struct Choice
    {
        juce::ComboBox box;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attach;
    };
    Choice& makeChoice (const char* id, const juce::String& title, const juce::String& hint);

    void setHint (juce::Component* c, const juce::String& text);

    SadhanaProcessor& proc;
    SadhanaLookAndFeel lnf;

    std::vector<std::unique_ptr<Knob>> knobs;
    std::vector<std::unique_ptr<Choice>> choices;
    std::map<juce::Component*, juce::String> hints;

    juce::OwnedArray<juce::TextButton> presetButtons;
    juce::TextButton clarinetBtn { "Clarinet" }, bassBtn { "Bass Clarinet" };
    juce::TextButton droneBtn { "Drone On" };
    juce::TextButton lowBtn { "Low" }, midBtn { "Mid" };

    Mandala mandala;
    juce::String hintText, defaultHint, readsText;

    juce::Rectangle<int> voiceArea, droneArea, spaceArea, hintArea;

    std::vector<float> tunerSnap;
    float smoothedHz = 0.0f;
    int silentFrames = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SadhanaEditor)
};

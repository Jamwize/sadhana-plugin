#include "PluginProcessor.h"
#include "PluginEditor.h"

using APVTS = juce::AudioProcessorValueTreeState;

APVTS::ParameterLayout SadhanaProcessor::createLayout()
{
    using P = juce::AudioParameterFloat;
    using C = juce::AudioParameterChoice;
    using B = juce::AudioParameterBool;
    using ID = juce::ParameterID;
    auto unit = [] { return juce::AudioParameterFloatAttributes().withStringFromValueFunction (
                         [] (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + "%"; }); };
    const juce::NormalisableRange<float> r01 (0.0f, 1.0f);

    juce::StringArray keys;
    for (int i = 0; i < 12; ++i) keys.add (sadhana::noteName (i));
    juce::StringArray scaleNames;
    for (auto& s : sadhana::scales()) scaleNames.add (s.name);

    APVTS::ParameterLayout l;
    l.add (std::make_unique<C> (ID { "instrument", 1 }, "Instrument", juce::StringArray { "Clarinet", "Bass Clarinet" }, 0));
    l.add (std::make_unique<P> (ID { "warmth", 1 },   "Warmth",  r01, 0.35f, unit()));
    l.add (std::make_unique<P> (ID { "body", 1 },     "Body",    r01, 0.5f,  unit()));
    l.add (std::make_unique<P> (ID { "air", 1 },      "Air",     r01, 0.4f,  unit()));
    l.add (std::make_unique<P> (ID { "smooth", 1 },   "Smooth",  r01, 0.4f,  unit()));

    l.add (std::make_unique<C> (ID { "key", 1 },      "Key",     keys, 2));
    l.add (std::make_unique<C> (ID { "scale", 1 },    "Scale",   scaleNames, 0));
    l.add (std::make_unique<C> (ID { "tuning", 1 },   "Tuning",  juce::StringArray { "A = 440", "A = 432" }, 0));
    l.add (std::make_unique<C> (ID { "names", 1 },    "Note Names", juce::StringArray { "Concert", "Bb Clarinet" }, 1));

    l.add (std::make_unique<B> (ID { "droneOn", 1 },  "Drone", true));
    l.add (std::make_unique<P> (ID { "droneLevel", 1 }, "Drone Level", r01, 0.45f, unit()));
    l.add (std::make_unique<P> (ID { "overtones", 1 }, "Overtones", r01, 0.5f, unit()));
    l.add (std::make_unique<P> (ID { "texture", 1 },  "Texture", r01, 0.35f, unit()));
    l.add (std::make_unique<C> (ID { "octave", 1 },   "Drone Octave", juce::StringArray { "Low", "Mid" }, 0));

    l.add (std::make_unique<P> (ID { "space", 1 },    "Space",   r01, 0.35f, unit()));
    l.add (std::make_unique<P> (ID { "echo", 1 },     "Echo",    r01, 0.12f, unit()));
    l.add (std::make_unique<P> (ID { "output", 1 },   "Output",  juce::NormalisableRange<float> (-24.0f, 6.0f, 0.1f), 0.0f,
                                juce::AudioParameterFloatAttributes().withLabel ("dB")));
    return l;
}

//==============================================================================
const std::vector<SadhanaProcessor::Preset>& SadhanaProcessor::presets()
{
    static const std::vector<Preset> p = {
        { "Morning Sadhana", "Bright and open. D major drone, a gentle hall.",
          { {"instrument",0},{"warmth",0.35f},{"body",0.5f},{"air",0.45f},{"smooth",0.4f},{"key",2},{"scale",0},
            {"droneOn",1},{"droneLevel",0.45f},{"overtones",0.5f},{"texture",0.3f},{"octave",0},{"space",0.35f},{"echo",0.12f} } },
        { "Temple Hall", "Long stone reverb, reedy shruti drone. C Bhairav.",
          { {"instrument",0},{"warmth",0.4f},{"body",0.55f},{"air",0.35f},{"smooth",0.5f},{"key",0},{"scale",6},
            {"droneOn",1},{"droneLevel",0.5f},{"overtones",0.55f},{"texture",0.65f},{"octave",0},{"space",0.75f},{"echo",0.18f} } },
        { "Desert Hijaz", "Arabic colour. D Hijaz, singing overtones, echoes.",
          { {"instrument",0},{"warmth",0.45f},{"body",0.5f},{"air",0.4f},{"smooth",0.45f},{"key",2},{"scale",7},
            {"droneOn",1},{"droneLevel",0.5f},{"overtones",0.75f},{"texture",0.8f},{"octave",0},{"space",0.5f},{"echo",0.25f} } },
        { "Warm Room", "Close and intimate. Soft pure drone, very little reverb.",
          { {"instrument",0},{"warmth",0.6f},{"body",0.6f},{"air",0.25f},{"smooth",0.35f},{"key",7},{"scale",3},
            {"droneOn",1},{"droneLevel",0.3f},{"overtones",0.35f},{"texture",0.1f},{"octave",1},{"space",0.15f},{"echo",0.0f} } },
        { "Earth Offering", "For bass clarinet. Deep A drone, minor and grounded.",
          { {"instrument",1},{"warmth",0.5f},{"body",0.55f},{"air",0.3f},{"smooth",0.45f},{"key",9},{"scale",4},
            {"droneOn",1},{"droneLevel",0.5f},{"overtones",0.45f},{"texture",0.5f},{"octave",0},{"space",0.45f},{"echo",0.1f} } },
        { "Sound Bath", "Pure singing overtones at A = 432. Floaty and wide.",
          { {"instrument",0},{"warmth",0.3f},{"body",0.45f},{"air",0.5f},{"smooth",0.4f},{"key",2},{"scale",10},{"tuning",1},
            {"droneOn",1},{"droneLevel",0.6f},{"overtones",0.9f},{"texture",0.0f},{"octave",0},{"space",0.65f},{"echo",0.2f} } },
    };
    return p;
}

int SadhanaProcessor::getNumPrograms() { return (int) presets().size(); }
const juce::String SadhanaProcessor::getProgramName (int i) { return juce::isPositiveAndBelow (i, getNumPrograms()) ? presets()[(size_t) i].name : ""; }
void SadhanaProcessor::setCurrentProgram (int i) { if (juce::isPositiveAndBelow (i, getNumPrograms())) applyPreset (i); }

void SadhanaProcessor::applyPreset (int index)
{
    currentPreset = index;
    // tuning defaults to 440 unless the preset says otherwise
    if (auto* t = apvts.getParameter ("tuning")) t->setValueNotifyingHost (0.0f);
    for (auto& [id, value] : presets()[(size_t) index].values)
        if (auto* prm = apvts.getParameter (id))
            prm->setValueNotifyingHost (prm->convertTo0to1 (value));
}

//==============================================================================
SadhanaProcessor::SadhanaProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
}

bool SadhanaProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet(), out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::stereo()) return false;
    return in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
}

void SadhanaProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    engine.prepare (sampleRate, samplesPerBlock);
}

sadhana::Params SadhanaProcessor::readParams()
{
    auto v = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    sadhana::Params p;
    p.instrument  = (int) v ("instrument");
    p.warmth      = v ("warmth");
    p.body        = v ("body");
    p.air         = v ("air");
    p.smooth      = v ("smooth");
    p.key         = (int) v ("key");
    p.scale       = (int) v ("scale");
    p.aRef        = v ("tuning") > 0.5f ? 432.0f : 440.0f;
    p.droneOn     = v ("droneOn") > 0.5f;
    p.droneLevel  = v ("droneLevel");
    p.overtones   = v ("overtones");
    p.texture     = v ("texture");
    p.droneOctave = (int) v ("octave");
    p.space       = v ("space");
    p.echo        = v ("echo");
    p.outputDb    = v ("output");

    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto bpm = pos->getBpm())
                p.bpm = *bpm;
    return p;
}

void SadhanaProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    // a mono input bus: make sure the right channel exists as a copy
    if (getTotalNumInputChannels() == 1 && buffer.getNumChannels() > 1)
        buffer.copyFrom (1, 0, buffer, 0, 0, buffer.getNumSamples());

    engine.setParams (readParams());
    engine.process (buffer);
}

juce::AudioProcessorEditor* SadhanaProcessor::createEditor() { return new SadhanaEditor (*this); }

void SadhanaProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("preset", currentPreset, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void SadhanaProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        auto state = juce::ValueTree::fromXml (*xml);
        if (state.isValid())
        {
            currentPreset = state.getProperty ("preset", 0);
            apvts.replaceState (state);
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new SadhanaProcessor(); }

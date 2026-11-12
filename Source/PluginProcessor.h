#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "SadhanaEngine.h"

class SadhanaProcessor : public juce::AudioProcessor
{
public:
    SadhanaProcessor();
    ~SadhanaProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Sadhana"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 6.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentPreset; }
    void setCurrentProgram (int) override;
    const juce::String getProgramName (int) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    struct Preset
    {
        const char* name;
        const char* blurb;
        std::vector<std::pair<const char*, float>> values; // param id -> real value (choices as index)
    };
    static const std::vector<Preset>& presets();
    void applyPreset (int index);

    sadhana::Engine& getEngine() { return engine; }
    double getSampleRateSafe() const { return sr.load(); }

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

private:
    sadhana::Params readParams();
    sadhana::Engine engine;
    std::atomic<double> sr { 48000.0 };
    int currentPreset = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SadhanaProcessor)
};

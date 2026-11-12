// Renders the editor to PNG so the UI can be checked without a DAW.
#include <juce_gui_basics/juce_gui_basics.h>
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"

class App : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override { return "UISnapshot"; }
    const juce::String getApplicationVersion() override { return "1"; }

    void initialise (const juce::String& cmd) override
    {
        outDir = juce::File (cmd.trim().isNotEmpty() ? cmd.trim() : ".");
        proc = std::make_unique<SadhanaProcessor>();
        proc->prepareToPlay (48000.0, 512);
        editor.reset (dynamic_cast<SadhanaEditor*> (proc->createEditor()));
        editor->setVisible (true);

        // feed a few blocks of drone + a tone at concert E4 (in D major) for the tuner
        juce::Timer::callAfterDelay (200, [this] { pump (330.0f); });
        juce::Timer::callAfterDelay (1500, [this] { snap ("ui_playing_E.png"); proc->applyPreset (2); });
        juce::Timer::callAfterDelay (2200, [this] { pump (0.0f); });
        juce::Timer::callAfterDelay (3500, [this] { snap ("ui_desert_hijaz.png"); quit(); });
    }

    void pump (float hz)
    {
        juce::AudioBuffer<float> b (2, 512);
        double ph = 0;
        for (int blk = 0; blk < 40; ++blk)
        {
            for (int i = 0; i < 512; ++i)
            {
                float s = 0;
                if (hz > 0) { ph += 2 * M_PI * hz / 48000.0; s = 0.2f * (float) (std::sin (ph) + 0.3 * std::sin (3 * ph)); }
                b.setSample (0, i, s); b.setSample (1, i, s);
            }
            juce::MidiBuffer m;
            proc->processBlock (b, m);
        }
        if (hz > 0)
            juce::Timer::callAfterDelay (100, [this, hz] { pumpCount++ < 8 ? pump (hz) : void(); });
    }

    void snap (const char* name)
    {
        auto img = editor->createComponentSnapshot (editor->getLocalBounds(), true, 1.0f);
        juce::PNGImageFormat png;
        juce::FileOutputStream os (outDir.getChildFile (name));
        os.setPosition (0); os.truncate();
        png.writeImageToStream (img, os);
    }

    void shutdown() override { editor = nullptr; proc = nullptr; }

private:
    juce::File outDir;
    std::unique_ptr<SadhanaProcessor> proc;
    std::unique_ptr<SadhanaEditor> editor;
    int pumpCount = 0;
};

START_JUCE_APPLICATION (App)

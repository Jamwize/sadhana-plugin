#include <juce_audio_formats/juce_audio_formats.h>
#include "../Source/SadhanaEngine.h"
#include <chrono>

static void writeWav (const juce::File& f, const juce::AudioBuffer<float>& b, double sr)
{
    f.deleteFile();
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (new juce::FileOutputStream (f), sr,
                                                                    (unsigned) b.getNumChannels(), 24, {}, 0));
    w->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
}

// A fake clarinet: odd-harmonic heavy tone, breathy attack, slow vibrato
static std::vector<float> fakeClarinet (double sr, const std::vector<std::pair<int, double>>& notes, double aRef)
{
    std::vector<float> out;
    std::mt19937 rng (5);
    std::normal_distribution<float> nd (0.0f, 1.0f);
    for (auto [midi, dur] : notes)
    {
        const double f = aRef * std::pow (2.0, (midi - 69) / 12.0);
        const int len = (int) (dur * sr);
        double ph = 0; float breathLp = 0;
        for (int i = 0; i < len; ++i)
        {
            const double t = i / sr;
            const float env = (float) (std::min (1.0, t / 0.08) * std::min (1.0, (dur - t) / 0.12));
            const double vib = 1.0 + 0.003 * std::sin (2 * M_PI * 5.0 * t) * std::min (1.0, t / 0.6);
            ph += 2 * M_PI * f * vib / sr;
            float s = 0;
            for (int h = 1; h <= 15; ++h)
            {
                const float a = (h % 2 == 1 ? 1.0f : 0.12f) / (float) h;
                s += a * (float) std::sin (ph * h);
            }
            breathLp += 0.2f * (nd (rng) - breathLp);
            out.push_back (0.25f * env * (s + 0.05f * breathLp));
        }
    }
    return out;
}

int main (int argc, char** argv)
{
    const juce::File outDir (argc > 1 ? argv[1] : ".");
    const double sr = 48000.0;
    const int block = 256;

    // print the overtone sets per scale (key of D)
    for (int sc = 0; sc < (int) sadhana::scales().size(); ++sc)
    {
        sadhana::Drone d; d.prepare (sr); d.ensureCapacity (block);
        sadhana::Params p; p.key = 2; p.scale = sc;
        float l[block], r[block]; d.render (l, r, block, p);
        juce::String s;
        for (int h : d.activeHarmonics()) s << h << "(" << sadhana::noteName (2 + (int) std::lround (12.0 * std::log2 ((double) h))) << ") ";
        std::printf ("%-32s tonic %.1f Hz  overtones: %s\n", sadhana::scales()[(size_t) sc].name, d.tonicHz(), s.toRawUTF8());
    }

    // D major phrase (concert pitch), quiet then drone
    auto mel = fakeClarinet (sr, { {62, 1.5}, {64, 0.7}, {66, 0.7}, {69, 2.0}, {67, 0.8}, {66, 0.8}, {64, 0.8}, {62, 3.0} }, 440.0);
    mel.resize (mel.size() + (size_t) (sr * 3.0), 0.0f);

    auto render = [&] (const juce::String& name, sadhana::Params p, bool clarinet)
    {
        sadhana::Engine eng;
        eng.prepare (sr, block);
        eng.setParams (p);
        const int total = (int) mel.size();
        juce::AudioBuffer<float> out (2, total);
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < total; ++i) out.setSample (ch, i, clarinet ? mel[(size_t) i] : 0.0f);

        const auto t0 = std::chrono::steady_clock::now();
        for (int s = 0; s < total; s += block)
        {
            juce::AudioBuffer<float> view (out.getArrayOfWritePointers(), 2, s, std::min (block, total - s));
            eng.process (view);
        }
        const double secs = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
        bool finite = true;
        for (int i = 0; i < total; ++i) if (! std::isfinite (out.getSample (0, i)) || ! std::isfinite (out.getSample (1, i))) finite = false;
        std::printf ("%-26s peak %.3f rms %.3f finite %s realtime x%.0f\n", name.toRawUTF8(), out.getMagnitude (0, total),
                     out.getRMSLevel (0, 0, total), finite ? "yes" : "NO", (total / sr) / secs);
        writeWav (outDir.getChildFile (name + ".wav"), out, sr);
    };

    {
        juce::AudioBuffer<float> dry (2, (int) mel.size());
        for (int ch = 0; ch < 2; ++ch) for (int i = 0; i < (int) mel.size(); ++i) dry.setSample (ch, i, mel[(size_t) i]);
        writeWav (outDir.getChildFile ("00_dry_fake_clarinet.wav"), dry, sr);
    }

    sadhana::Params morning; morning.key = 2; morning.scale = 0;
    render ("01_morning_sadhana_D_major", morning, true);

    sadhana::Params drone; drone.key = 2; drone.scale = 7; drone.texture = 0.8f; drone.overtones = 0.75f; drone.space = 0.5f;
    render ("02_drone_only_D_hijaz", drone, false);

    sadhana::Params pure; pure.key = 2; pure.scale = 0; pure.texture = 0.0f; pure.overtones = 0.8f; pure.aRef = 432.0f; pure.droneLevel = 0.6f;
    render ("03_pure_overtones_432", pure, false);

    sadhana::Params dryish; dryish.droneOn = false; dryish.space = 0.0f; dryish.echo = 0.0f; dryish.warmth = 0.6f;
    render ("04_clarinet_voice_only", dryish, true);
    return 0;
}

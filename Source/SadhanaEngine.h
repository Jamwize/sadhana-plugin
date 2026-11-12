// Sadhana — clarinet voice + scale drone
// Header-only so the plug-in and the offline test renderer share exactly the same DSP.
#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include <cmath>
#include <random>
#include <vector>

namespace sadhana
{

//==============================================================================
// Scales: Western name + raga/maqam name, as semitone sets above the tonic
struct Scale { const char* name; std::vector<int> degrees; };

inline const std::vector<Scale>& scales()
{
    static const std::vector<Scale> s = {
        { "Major  /  Bilawal",              { 0, 2, 4, 5, 7, 9, 11 } },
        { "Lydian  /  Yaman",               { 0, 2, 4, 6, 7, 9, 11 } },
        { "Mixolydian  /  Khamaj",          { 0, 2, 4, 5, 7, 9, 10 } },
        { "Dorian  /  Kafi",                { 0, 2, 3, 5, 7, 9, 10 } },
        { "Minor  /  Asavari",              { 0, 2, 3, 5, 7, 8, 10 } },
        { "Phrygian  /  Bhairavi",          { 0, 1, 3, 5, 7, 8, 10 } },
        { "Double Harmonic  /  Bhairav",    { 0, 1, 4, 5, 7, 8, 11 } },
        { "Hijaz  (Arabic)",                { 0, 1, 4, 5, 7, 8, 10 } },
        { "Major Pentatonic  /  Bhupali",   { 0, 2, 4, 7, 9 } },
        { "Minor Pentatonic",               { 0, 3, 5, 7, 10 } },
        { "Sa + Pa only  (root & fifth)",   { 0, 7 } },
    };
    return s;
}

inline const char* noteName (int pc)
{
    static const char* n[12] = { "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" };
    return n[((pc % 12) + 12) % 12];
}

//==============================================================================
struct Params
{
    int   instrument  = 0;      // 0 = clarinet, 1 = bass clarinet
    float warmth      = 0.35f;  // 0..1
    float body        = 0.5f;   // 0..1, 0.5 = flat
    float air         = 0.4f;   // 0..1
    float smooth      = 0.4f;   // 0..1 compression amount

    int   key         = 2;      // pitch class, concert
    int   scale       = 0;
    float aRef        = 440.0f;
    bool  droneOn     = true;
    float droneLevel  = 0.45f;
    float overtones   = 0.5f;
    float texture     = 0.35f;  // 0 = pure tones, 1 = reedy shruti box
    int   droneOctave = 0;      // 0 = low, 1 = mid

    float space       = 0.35f;
    float echo        = 0.12f;
    float outputDb    = 0.0f;
    double bpm        = 90.0;
};

//==============================================================================
// Drone: tanpura-style root + fifth, plus the tonic's natural harmonics that belong to the scale.
class Drone
{
public:
    struct Partial
    {
        double freq = 0, phase = 0;
        float amp = 0, pan = 0.5f;
        double lfoPhase = 0, lfoRate = 0.1;
        float lfoDepth = 0.3f;
    };

    void prepare (double sr)
    {
        sampleRate = sr;
        reedLpf.prepare ({ sr, 512, 2 });
        reedLpf.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
        reedLpf.setCutoffFrequency (1500.0f);
        reedLpf.setResonance (0.5f);
        level.reset (sr, 1.5);      // gentle fade in/out
        retuneGain.reset (sr, 0.12);
        retuneGain.setCurrentAndTargetValue (1.0f);
        built = false;
    }

    // Returns which harmonic numbers were used, for the GUI/test
    const std::vector<int>& activeHarmonics() const { return harmonicsUsed; }
    double tonicHz() const { return tonic; }

    void render (float* L, float* R, int n, const Params& p)
    {
        const int sig = p.key * 1000 + p.scale * 10 + p.droneOctave + (p.aRef < 436.0f ? 5000000 : 0);
        if (! built) { build (p); signature = sig; built = true; }
        else if (sig != signature && ! retuning)
        {
            retuning = true;            // fade out, rebuild, fade in
            retuneGain.setTargetValue (0.0f);
        }
        if (retuning && ! retuneGain.isSmoothing() && retuneGain.getCurrentValue() <= 0.0001f)
        {
            build (p);
            signature = sig;
            retuning = false;
            retuneGain.setTargetValue (1.0f);
        }

        level.setTargetValue (p.droneOn ? p.droneLevel : 0.0f);
        reedLpf.setCutoffFrequency ((float) juce::jlimit (300.0, 4000.0, tonic * 10.0));

        const float overt = p.overtones;
        const float tex = p.texture;
        const float pureGain = std::sqrt (1.0f - tex);
        const float reedGain = std::sqrt (tex);
        const double twoPi = juce::MathConstants<double>::twoPi;

        for (int i = 0; i < n; ++i)
        {
            float l = 0.0f, r = 0.0f;

            for (size_t k = 0; k < partials.size(); ++k)
            {
                auto& pt = partials[k];
                const bool core = k < numCore;
                const float lfo = 1.0f - pt.lfoDepth * (0.5f + 0.5f * (float) std::sin (pt.lfoPhase));
                pt.lfoPhase += twoPi * pt.lfoRate / sampleRate;
                if (pt.lfoPhase > twoPi) pt.lfoPhase -= twoPi;

                float a = pt.amp * lfo * (core ? pureGain : overt);
                const float s = (float) std::sin (pt.phase) * a;
                pt.phase += twoPi * pt.freq / sampleRate;
                if (pt.phase > twoPi) pt.phase -= twoPi;

                l += s * std::cos (pt.pan * juce::MathConstants<float>::halfPi);
                r += s * std::sin (pt.pan * juce::MathConstants<float>::halfPi);
            }

            // reed voices (shruti box / harmonium feel): detuned band-limited saws on the core notes
            float rl = 0.0f, rr = 0.0f;
            if (reedGain > 0.001f)
            {
                for (size_t k = 0; k < reeds.size(); ++k)
                {
                    auto& rd = reeds[k];
                    const double dt = rd.freq / sampleRate;
                    float v = (float) (2.0 * rd.phase - 1.0) - polyBlep (rd.phase, dt);
                    rd.phase += dt; if (rd.phase >= 1.0) rd.phase -= 1.0;
                    v *= rd.amp;
                    rl += v * (1.0f - rd.pan);
                    rr += v * rd.pan;
                }
            }
            reedL[(size_t) i] = rl * reedGain;
            reedR[(size_t) i] = rr * reedGain;

            L[i] = l;
            R[i] = r;
        }

        // filter reeds and sum
        float* ch[2] = { reedL.data(), reedR.data() };
        juce::dsp::AudioBlock<float> rb (ch, 2, (size_t) n);
        juce::dsp::ProcessContextReplacing<float> rc (rb);
        reedLpf.process (rc);

        for (int i = 0; i < n; ++i)
        {
            const float g = level.getNextValue() * retuneGain.getNextValue() * 0.4f;
            L[i] = (L[i] + reedL[(size_t) i]) * g;
            R[i] = (R[i] + reedR[(size_t) i]) * g;
        }
    }

    void ensureCapacity (int n)
    {
        if ((int) reedL.size() < n) { reedL.resize ((size_t) n); reedR.resize ((size_t) n); }
    }

private:
    struct Reed { double freq = 0, phase = 0; float amp = 0, pan = 0.5f; };

    static float polyBlep (double t, double dt)
    {
        if (t < dt) { t /= dt; return (float) (t + t - t * t - 1.0); }
        if (t > 1.0 - dt) { t = (t - 1.0) / dt; return (float) (t * t + t + t + 1.0); }
        return 0.0f;
    }

    void build (const Params& p)
    {
        const auto& sc = scales()[(size_t) juce::jlimit (0, (int) scales().size() - 1, p.scale)].degrees;
        auto inScale = [&] (int pc) { return std::find (sc.begin(), sc.end(), ((pc % 12) + 12) % 12) != sc.end(); };

        // tonic in octave 2 (low) or 3 (mid); MIDI note of C2 is 36
        const int midi = 36 + 12 * p.droneOctave + p.key;
        tonic = p.aRef * std::pow (2.0, (midi - 69) / 12.0);

        std::mt19937 rng ((unsigned) (p.key * 31 + p.scale * 7 + 11));
        std::uniform_real_distribution<double> u (0.0, 1.0);

        partials.clear();
        reeds.clear();
        harmonicsUsed.clear();

        const bool hasFifth = inScale (7);

        // ---- core: root, octave, (fifth) — the tanpura's Sa and Pa
        auto addCore = [&] (double ratio, float amp, float pan)
        {
            Partial pt; pt.freq = tonic * ratio; pt.amp = amp; pt.pan = pan;
            pt.phase = u (rng) * 6.28; pt.lfoPhase = u (rng) * 6.28; pt.lfoRate = 0.03 + 0.05 * u (rng); pt.lfoDepth = 0.12f;
            partials.push_back (pt);
            for (int d = -1; d <= 1; d += 2)
            {
                Reed rd; rd.freq = tonic * ratio * std::pow (2.0, d * 3.5 / 1200.0); rd.amp = amp * 0.35f;
                rd.pan = 0.5f + 0.25f * (float) d; rd.phase = u (rng);
                reeds.push_back (rd);
            }
        };
        addCore (1.0, 1.0f, 0.5f);
        addCore (2.0, 0.55f, 0.4f);
        if (hasFifth) addCore (3.0, 0.45f, 0.62f);
        numCore = partials.size();

        // ---- overtones: harmonics 4..24 of the tonic whose pitch lands on a scale degree
        for (int h = 4; h <= 24; ++h)
        {
            const double cents = 1200.0 * std::log2 ((double) h);
            const double semis = cents / 100.0;
            const int nearest = (int) std::lround (semis);
            const double dev = (semis - nearest) * 100.0;
            if (std::abs (dev) > 35.0) continue;            // too far from any scale step (e.g. 11th, 13th)
            if (! inScale (nearest)) continue;               // only notes that belong to the scale
            if (tonic * h > 5000.0) continue;                // keep it warm

            Partial pt;
            pt.freq = tonic * h;
            pt.amp = 0.5f / std::pow ((float) h, 0.75f);
            pt.pan = (h % 2 == 0) ? 0.25f + 0.1f * (float) u (rng) : 0.75f - 0.1f * (float) u (rng);
            pt.phase = u (rng) * 6.28;
            pt.lfoPhase = u (rng) * 6.28;
            pt.lfoRate = 0.04 + 0.12 * u (rng);              // each overtone "sings" at its own slow pace
            pt.lfoDepth = 0.65f;
            partials.push_back (pt);
            harmonicsUsed.push_back (h);
        }
    }

    double sampleRate = 48000.0, tonic = 110.0;
    std::vector<Partial> partials;
    std::vector<Reed> reeds;
    size_t numCore = 0;
    std::vector<int> harmonicsUsed;
    std::vector<float> reedL, reedR;
    juce::dsp::StateVariableTPTFilter<float> reedLpf;
    juce::SmoothedValue<float> level, retuneGain;
    int signature = -1;
    bool built = false, retuning = false;
};

//==============================================================================
// Simple tape-ish echo, synced to tempo (dotted eighth), filtered in the feedback loop
class Echo
{
public:
    void prepare (double sr)
    {
        sampleRate = sr;
        const int len = (int) (sr * 2.5);
        bufL.assign ((size_t) len, 0.0f); bufR.assign ((size_t) len, 0.0f);
        write = 0;
        time.reset (sr, 0.3);
        lpL = lpR = hpL = hpR = 0.0f;
    }

    void process (const float* in, float* outL, float* outR, int n, float mix, double bpm)
    {
        const double beat = 60.0 / juce::jlimit (40.0, 220.0, bpm);
        time.setTargetValue ((float) (beat * 0.75 * sampleRate));   // dotted 1/8
        const int len = (int) bufL.size();
        const float fb = 0.38f;
        const float lpC = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 3200.0f / (float) sampleRate);
        const float hpC = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 280.0f / (float) sampleRate);

        for (int i = 0; i < n; ++i)
        {
            const float d = time.getNextValue();
            auto read = [&] (std::vector<float>& b, float delay)
            {
                float pos = (float) write - delay;
                while (pos < 0) pos += (float) len;
                const int i0 = (int) pos; const float f = pos - (float) i0;
                return b[(size_t) i0] * (1.0f - f) + b[(size_t) ((i0 + 1) % len)] * f;
            };
            float l = read (bufL, d), r = read (bufR, d * 1.5f);   // right repeats a little later: wide but calm

            lpL += lpC * (l - lpL); lpR += lpC * (r - lpR);
            hpL += hpC * (lpL - hpL); hpR += hpC * (lpR - hpR);
            const float fl = lpL - hpL, fr = lpR - hpR;

            bufL[(size_t) write] = std::tanh (in[i] + fl * fb);
            bufR[(size_t) write] = std::tanh (in[i] * 0.7f + fr * fb);
            write = (write + 1) % len;

            outL[i] += fl * mix;
            outR[i] += fr * mix;
        }
    }

private:
    double sampleRate = 48000.0;
    std::vector<float> bufL, bufR;
    int write = 0;
    juce::SmoothedValue<float> time;
    float lpL = 0, lpR = 0, hpL = 0, hpR = 0;
};

//==============================================================================
class Engine
{
public:
    static constexpr int tunerSize = 4096;

    void prepare (double sr, int maxBlock)
    {
        sampleRate = sr;
        blockMax = juce::jmax (1, maxBlock);
        juce::dsp::ProcessSpec mono { sr, (juce::uint32) blockMax, 1 };
        juce::dsp::ProcessSpec stereo { sr, (juce::uint32) blockMax, 2 };

        hpf.prepare (mono); hpf.setType (juce::dsp::StateVariableTPTFilterType::highpass);
        comp.prepare (mono); comp.setAttack (18.0f); comp.setRelease (180.0f); comp.setRatio (2.5f);
        for (auto* f : { &body, &warmShelf, &airShelf })
        {
            f->coefficients = new juce::dsp::IIR::Coefficients<float> (1.0f, 0.0f, 1.0f, 0.0f);
            f->prepare (mono);
        }
        oversampler.initProcessing ((size_t) blockMax);
        oversampler.reset();

        echo.prepare (sr);
        drone.prepare (sr);
        drone.ensureCapacity (blockMax);
        reverb.prepare (stereo);
        predelay.prepare (stereo);
        predelay.setMaximumDelayInSamples ((int) (sr * 0.1));
        predelay.setDelay ((float) (sr * 0.035));

        clar.setSize (1, blockMax);
        wetIn.setSize (2, blockMax);
        droneBuf.setSize (2, blockMax);
        smoothOut.reset (sr, 0.05);
    }

    void setParams (const Params& np) { p = np; }

    // tuner tap (read by the GUI)
    void copyTunerSnapshot (std::vector<float>& dest) const
    {
        dest.resize (tunerSize);
        const int w = tunerWrite.load();
        for (int i = 0; i < tunerSize; ++i)
            dest[(size_t) i] = tunerBuf[(size_t) ((w + i) % tunerSize)];
    }
    float getInputLevel() const { return inLevel.load(); }
    float getDroneLevel() const { return droneLevelMeter.load(); }
    double getTonicHz() const { return drone.tonicHz(); }
    const std::vector<int>& droneHarmonics() const { return drone.activeHarmonics(); }

    void process (juce::AudioBuffer<float>& buffer)
    {
        const int total = buffer.getNumSamples();
        for (int s = 0; s < total; s += blockMax)
            processChunk (buffer, s, juce::jmin (blockMax, total - s));
    }

private:
    void updateFilters()
    {
        using AC = juce::dsp::IIR::ArrayCoefficients<float>;
        const bool bass = p.instrument == 1;
        hpf.setCutoffFrequency (bass ? 38.0f : 110.0f);
        *body.coefficients      = AC::makePeakFilter (sampleRate, bass ? 170.0f : 320.0f, 0.9f,
                                                      juce::Decibels::decibelsToGain ((p.body - 0.5f) * 9.0f));
        *warmShelf.coefficients = AC::makeHighShelf (sampleRate, 6500.0f, 0.6f,
                                                     juce::Decibels::decibelsToGain (-p.warmth * 4.5f));
        *airShelf.coefficients  = AC::makeHighShelf (sampleRate, 10000.0f, 0.7f,
                                                     juce::Decibels::decibelsToGain (p.air * 5.0f));
        // Smooth: lower threshold = more evening-out; gentle makeup
        comp.setThreshold (-8.0f - p.smooth * 22.0f);
    }

    void processChunk (juce::AudioBuffer<float>& buffer, int start, int n)
    {
        updateFilters();

        // ---- clarinet in (left channel; a mono mic input arrives on both sides)
        float* x = clar.getWritePointer (0);
        juce::FloatVectorOperations::copy (x, buffer.getReadPointer (0, start), n);

        // meter + tuner tap (pre-processing, cleaner pitch)
        float pk = 0.0f;
        int w = tunerWrite.load();
        for (int i = 0; i < n; ++i)
        {
            pk = juce::jmax (pk, std::abs (x[i]));
            tunerBuf[(size_t) w] = x[i];
            w = (w + 1) % tunerSize;
        }
        tunerWrite.store (w);
        inLevel.store (pk);

        juce::dsp::AudioBlock<float> cb (clar.getArrayOfWritePointers(), 1, (size_t) n);
        juce::dsp::ProcessContextReplacing<float> cc (cb);
        hpf.process (cc);
        comp.process (cc);
        if (p.smooth > 0.01f)
            juce::FloatVectorOperations::multiply (x, juce::Decibels::decibelsToGain (p.smooth * 6.0f), n);
        body.process (cc);

        // ---- warmth: oversampled soft saturation (analogue-style even/odd glow), level matched
        if (p.warmth > 0.01f)
        {
            auto up = oversampler.processSamplesUp (cb);
            // tanh(k x)/k matches level at low volume and rounds off peaks like tape/valve
            const float k = 1.0f + p.warmth * 3.5f;
            auto* d = up.getChannelPointer (0);
            for (size_t i = 0; i < up.getNumSamples(); ++i)
            {
                const float sat = std::tanh (k * d[i]) / k;
                d[i] += p.warmth * 0.8f * (sat - d[i]);
            }
            oversampler.processSamplesDown (cb);
        }
        warmShelf.process (cc);
        airShelf.process (cc);

        // ---- drone
        float* dl = droneBuf.getWritePointer (0);
        float* dr = droneBuf.getWritePointer (1);
        drone.render (dl, dr, n, p);
        droneLevelMeter.store (droneBuf.getMagnitude (0, n));

        // ---- dry mix
        auto* outL = buffer.getWritePointer (0, start);
        auto* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1, start) : nullptr;
        for (int i = 0; i < n; ++i)
        {
            outL[i] = x[i] + dl[i];
            if (outR) outR[i] = x[i] + dr[i];
        }

        // ---- echo (clarinet only)
        if (p.echo > 0.005f && outR != nullptr)
            echo.process (x, outL, outR, n, p.echo * 0.55f, p.bpm);

        // ---- temple reverb: clarinet + some drone
        if (p.space > 0.005f)
        {
            auto* wl = wetIn.getWritePointer (0);
            auto* wr = wetIn.getWritePointer (1);
            for (int i = 0; i < n; ++i)
            {
                wl[i] = x[i] + 0.35f * dl[i];
                wr[i] = x[i] + 0.35f * dr[i];
            }
            juce::dsp::AudioBlock<float> wb (wetIn.getArrayOfWritePointers(), 2, (size_t) n);
            juce::dsp::ProcessContextReplacing<float> wc (wb);
            predelay.process (wc);

            juce::dsp::Reverb::Parameters rp;
            rp.roomSize = 0.86f + 0.1f * p.space;
            rp.damping  = 0.55f;
            rp.width    = 1.0f;
            rp.wetLevel = 1.0f;
            rp.dryLevel = 0.0f;
            reverb.setParameters (rp);
            reverb.process (wc);

            const float wet = p.space * 0.45f;
            for (int i = 0; i < n; ++i)
            {
                outL[i] += wl[i] * wet;
                if (outR) outR[i] += wr[i] * wet;
            }
        }

        // ---- output: gentle soft clip so nothing ever bites
        smoothOut.setTargetValue (juce::Decibels::decibelsToGain (p.outputDb));
        for (int i = 0; i < n; ++i)
        {
            const float g = smoothOut.getNextValue();
            outL[i] = softClip (outL[i] * g);
            if (outR) outR[i] = softClip (outR[i] * g);
        }
        for (int ch = 2; ch < buffer.getNumChannels(); ++ch)
            buffer.copyFrom (ch, start, buffer, 0, start, n);
    }

    static float softClip (float v)
    {
        const float a = std::abs (v);
        if (a < 0.8f) return v;
        return std::copysign (0.8f + 0.2f * std::tanh ((a - 0.8f) / 0.2f), v);
    }

    Params p;
    double sampleRate = 48000.0;
    int blockMax = 512;

    juce::dsp::StateVariableTPTFilter<float> hpf;
    juce::dsp::Compressor<float> comp;
    juce::dsp::IIR::Filter<float> body, warmShelf, airShelf;
    juce::dsp::Oversampling<float> oversampler { 1, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true };

    Drone drone;
    Echo echo;
    juce::dsp::Reverb reverb;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> predelay;

    juce::AudioBuffer<float> clar, wetIn, droneBuf;
    juce::SmoothedValue<float> smoothOut;

    std::array<float, tunerSize> tunerBuf {};
    std::atomic<int> tunerWrite { 0 };
    std::atomic<float> inLevel { 0.0f }, droneLevelMeter { 0.0f };
};

//==============================================================================
// YIN pitch detector for the tuner (runs on the GUI thread)
inline float detectPitch (const std::vector<float>& x, double sr, float& clarity)
{
    clarity = 0.0f;
    const int N = (int) x.size();
    const int maxLag = juce::jmin (N / 2, (int) (sr / 55.0));
    const int minLag = (int) (sr / 1800.0);
    float rms = 0; for (auto v : x) rms += v * v; rms = std::sqrt (rms / (float) N);
    if (rms < 0.004f) return 0.0f;

    const int W = N - maxLag;
    std::vector<float> d ((size_t) maxLag + 1, 0.0f);
    for (int tau = 1; tau <= maxLag; ++tau)
    {
        float s = 0;
        for (int i = 0; i < W; i += 2) { const float diff = x[(size_t) i] - x[(size_t) (i + tau)]; s += diff * diff; }
        d[(size_t) tau] = s;
    }
    // cumulative mean normalised difference
    float running = 0; d[0] = 1.0f;
    for (int tau = 1; tau <= maxLag; ++tau) { running += d[(size_t) tau]; d[(size_t) tau] = d[(size_t) tau] * (float) tau / juce::jmax (1e-9f, running); }

    int best = -1;
    for (int tau = minLag; tau < maxLag; ++tau)
        if (d[(size_t) tau] < 0.15f) { while (tau + 1 < maxLag && d[(size_t) tau + 1] < d[(size_t) tau]) ++tau; best = tau; break; }
    if (best < 0) return 0.0f;

    // parabolic interpolation
    const float a = d[(size_t) best - 1], b = d[(size_t) best], c = d[(size_t) best + 1];
    const float denom = a - 2 * b + c;
    const float shift = std::abs (denom) > 1e-9f ? 0.5f * (a - c) / denom : 0.0f;
    clarity = 1.0f - b;
    return (float) (sr / ((double) best + shift));
}

} // namespace sadhana

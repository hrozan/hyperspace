#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "PreAmp.h"
#include "PresetManager.h"
#include "ReverbEngine.h"

class HyperspaceProcessor final : public juce::AudioProcessor
{
public:
    HyperspaceProcessor();
    ~HyperspaceProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override;

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    juce::AudioProcessorParameter* getBypassParameter() const override { return bypassParam; }

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getValueTreeState() { return parameters; }
    PresetManager& getPresetManager() { return presetManager; }

    // ---- display taps for the editor ----
    float getModPhase() const noexcept { return engine.getModPhase(); }
    float getEffectiveRateHz() const noexcept { return effectiveRate.load (std::memory_order_relaxed); }
    float getEffectivePreDelayMs() const noexcept { return effectivePreDelay.load (std::memory_order_relaxed); }
    float getDrive() const noexcept { return preAmp.getDrive(); }
    float getOutputPeak (int channel) const noexcept
    {
        return outputPeak[static_cast<size_t> (juce::jlimit (0, 1, channel))].load (std::memory_order_relaxed);
    }

private:
    ReverbEngine::Settings gatherSettings() const;
    PreAmp::Settings gatherPreAmpSettings() const;

    /** Choice parameters come back as floats; rounding avoids truncating 2.9999 to 2. */
    int choiceIndex (const std::atomic<float>* value, int numChoices) const noexcept
    {
        return value != nullptr ? juce::jlimit (0, numChoices - 1, juce::roundToInt (value->load())) : 0;
    }

    juce::AudioProcessorValueTreeState parameters;
    PresetManager presetManager { parameters };
    PreAmp preAmp;
    ReverbEngine engine;

    // Cached raw pointers: looking parameters up by string on the audio thread is wasteful.
    struct Raw
    {
        std::atomic<float>* reverbType   = nullptr;
        std::atomic<float>* decay        = nullptr;
        std::atomic<float>* size         = nullptr;
        std::atomic<float>* preDelay     = nullptr;
        std::atomic<float>* preDelaySync = nullptr;
        std::atomic<float>* diffusion    = nullptr;
        std::atomic<float>* earlyLate    = nullptr;
        std::atomic<float>* lowCut       = nullptr;
        std::atomic<float>* highCut      = nullptr;
        std::atomic<float>* damping      = nullptr;
        std::atomic<float>* tone         = nullptr;
        std::atomic<float>* modOn        = nullptr;
        std::atomic<float>* modMode      = nullptr;
        std::atomic<float>* modRate      = nullptr;
        std::atomic<float>* modSync      = nullptr;
        std::atomic<float>* modDepth     = nullptr;
        std::atomic<float>* modSpread    = nullptr;
        std::atomic<float>* modShape     = nullptr;
        std::atomic<float>* modTarget    = nullptr;
        std::atomic<float>* modMix       = nullptr;
        std::atomic<float>* preAmpOn     = nullptr;
        std::atomic<float>* preGain      = nullptr;
        std::atomic<float>* preTone      = nullptr;
        std::atomic<float>* preOutput    = nullptr;
        std::atomic<float>* freeze       = nullptr;
        std::atomic<float>* duckOn       = nullptr;
        std::atomic<float>* duck         = nullptr;
        std::atomic<float>* mix          = nullptr;
        std::atomic<float>* width        = nullptr;
        std::atomic<float>* level        = nullptr;
        std::atomic<float>* bypass       = nullptr;
    };

    Raw raw;
    juce::AudioProcessorParameter* bypassParam = nullptr;

    double hostBpm = 120.0;

    std::array<std::atomic<float>, 2> outputPeak { };
    std::atomic<float> effectiveRate { 0.62f };
    std::atomic<float> effectivePreDelay { 75.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HyperspaceProcessor)
};

#include "PluginProcessor.h"

#include "Parameters.h"
#include "PluginEditor.h"

namespace
{
constexpr auto stateIdentifier = "HyperspaceState";
}

HyperspaceProcessor::HyperspaceProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, juce::Identifier (stateIdentifier), Params::createLayout())
{
    const auto get = [this] (const char* id) { return parameters.getRawParameterValue (id); };

    raw.reverbType   = get (ParamID::reverbType);
    raw.decay        = get (ParamID::decay);
    raw.size         = get (ParamID::size);
    raw.preDelay     = get (ParamID::preDelay);
    raw.preDelaySync = get (ParamID::preDelaySync);
    raw.diffusion    = get (ParamID::diffusion);
    raw.earlyLate    = get (ParamID::earlyLate);
    raw.lowCut       = get (ParamID::lowCut);
    raw.highCut      = get (ParamID::highCut);
    raw.damping      = get (ParamID::damping);
    raw.tone         = get (ParamID::tone);
    raw.modOn        = get (ParamID::modOn);
    raw.modMode      = get (ParamID::modMode);
    raw.modRate      = get (ParamID::modRate);
    raw.modSync      = get (ParamID::modSync);
    raw.modDepth     = get (ParamID::modDepth);
    raw.modSpread    = get (ParamID::modSpread);
    raw.modShape     = get (ParamID::modShape);
    raw.modTarget    = get (ParamID::modTarget);
    raw.modMix       = get (ParamID::modMix);
    raw.preAmpOn     = get (ParamID::preAmpOn);
    raw.preGain      = get (ParamID::preGain);
    raw.preTone      = get (ParamID::preTone);
    raw.preOutput    = get (ParamID::preOutput);
    raw.freeze       = get (ParamID::freeze);
    raw.duckOn       = get (ParamID::duckOn);
    raw.duck         = get (ParamID::duck);
    raw.mix          = get (ParamID::mix);
    raw.width        = get (ParamID::width);
    raw.level        = get (ParamID::level);
    raw.bypass       = get (ParamID::bypass);

    bypassParam = parameters.getParameter (ParamID::bypass);
}

ReverbEngine::Settings HyperspaceProcessor::gatherSettings() const
{
    ReverbEngine::Settings s;

    s.type         = static_cast<ReverbEngine::Type> (choiceIndex (raw.reverbType, Params::typeChoices.size()));
    s.decaySeconds = raw.decay->load();
    s.size         = raw.size->load() * 0.01f;
    s.preDelayMs   = SyncMath::preDelayMs (choiceIndex (raw.preDelaySync, Params::preSyncChoices.size()),
                                           raw.preDelay->load(), hostBpm);
    s.diffusion    = raw.diffusion->load() * 0.01f;
    s.earlyLate    = raw.earlyLate->load() * 0.01f;

    s.lowCutHz     = raw.lowCut->load();
    s.highCutHz    = raw.highCut->load();
    s.damping      = raw.damping->load() * 0.01f;
    s.toneDb       = raw.tone->load();

    s.modOn        = raw.modOn->load() > 0.5f;
    s.modMode      = static_cast<ReverbEngine::ModMode> (choiceIndex (raw.modMode, Params::modModeChoices.size()));
    s.modShape     = static_cast<ReverbEngine::ModShape> (choiceIndex (raw.modShape, Params::modShapeChoices.size()));
    s.modTarget    = static_cast<ReverbEngine::ModTarget> (choiceIndex (raw.modTarget, Params::modTargetChoices.size()));
    s.modRateHz    = SyncMath::lfoRateHz (choiceIndex (raw.modSync, Params::rateSyncChoices.size()),
                                          raw.modRate->load(), hostBpm);
    s.modDepth     = raw.modDepth->load() * 0.01f;
    s.modSpread    = raw.modSpread->load() * 0.01f;
    s.modMix       = raw.modMix->load() * 0.01f;

    s.freeze       = raw.freeze->load() > 0.5f;
    s.duckOn       = raw.duckOn->load() > 0.5f;
    s.duckDb       = raw.duck->load();

    s.mix          = raw.mix->load() * 0.01f;
    s.width        = raw.width->load() * 0.01f;
    s.levelGain    = juce::Decibels::decibelsToGain (raw.level->load());

    return s;
}

PreAmp::Settings HyperspaceProcessor::gatherPreAmpSettings() const
{
    PreAmp::Settings s;

    s.enabled  = raw.preAmpOn->load() > 0.5f;
    s.gainDb   = raw.preGain->load();
    s.toneDb   = raw.preTone->load();
    s.outputDb = raw.preOutput->load();

    return s;
}

double HyperspaceProcessor::getTailLengthSeconds() const
{
    if (raw.freeze != nullptr && raw.freeze->load() > 0.5f)
        return 30.0;

    const auto decay = raw.decay != nullptr ? raw.decay->load() : 4.8f;
    const auto pre = raw.preDelay != nullptr ? raw.preDelay->load() : 0.0f;

    return static_cast<double> (decay + pre * 0.001f);
}

void HyperspaceProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);

    const auto channels = juce::jmax (getTotalNumInputChannels(), getTotalNumOutputChannels());

    preAmp.setSettings (gatherPreAmpSettings());
    preAmp.prepare (sampleRate, channels);

    engine.setSettings (gatherSettings());
    engine.prepare (sampleRate, channels);

    for (auto& p : outputPeak)
        p.store (0.0f, std::memory_order_relaxed);
}

void HyperspaceProcessor::releaseResources()
{
    preAmp.reset();
    engine.reset();
}

bool HyperspaceProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& mainOutput = layouts.getMainOutputChannelSet();
    const auto& mainInput  = layouts.getMainInputChannelSet();

    if (mainOutput != juce::AudioChannelSet::mono() && mainOutput != juce::AudioChannelSet::stereo())
        return false;

    return mainInput == mainOutput;
}

void HyperspaceProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

    juce::ScopedNoDenormals noDenormals;

    for (auto channel = getTotalNumInputChannels(); channel < getTotalNumOutputChannels(); ++channel)
        buffer.clear (channel, 0, buffer.getNumSamples());

    if (auto* playHead = getPlayHead())
        if (const auto position = playHead->getPosition())
            if (const auto bpm = position->getBpm())
                hostBpm = *bpm;

    const auto bypassed = raw.bypass != nullptr && raw.bypass->load() > 0.5f;

    if (bypassed)
    {
        preAmp.reset();
        engine.reset();
    }
    else
    {
        const auto settings = gatherSettings();
        effectiveRate.store (settings.modRateHz, std::memory_order_relaxed);
        effectivePreDelay.store (settings.preDelayMs, std::memory_order_relaxed);

        // The pre amp is an input stage, so it runs before the split: the dry side of
        // Mix is the pre-amped signal, not the untouched input.
        preAmp.setSettings (gatherPreAmpSettings());
        preAmp.process (buffer.getArrayOfWritePointers(),
                        juce::jmin (buffer.getNumChannels(), PreAmp::maxChannels),
                        buffer.getNumSamples());

        engine.setSettings (settings);
        engine.process (buffer.getArrayOfWritePointers(),
                        juce::jmin (buffer.getNumChannels(), ReverbEngine::maxChannels),
                        buffer.getNumSamples());
    }

    for (size_t c = 0; c < outputPeak.size(); ++c)
    {
        const auto source = juce::jmin (static_cast<int> (c), buffer.getNumChannels() - 1);
        const auto peak = source >= 0 ? buffer.getMagnitude (source, 0, buffer.getNumSamples()) : 0.0f;
        outputPeak[c].store (peak, std::memory_order_relaxed);
    }
}

juce::AudioProcessorEditor* HyperspaceProcessor::createEditor()
{
    return new HyperspaceEditor (*this);
}

void HyperspaceProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void HyperspaceProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (parameters.state.getType()))
            parameters.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HyperspaceProcessor();
}

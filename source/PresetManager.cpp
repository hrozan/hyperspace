#include "PresetManager.h"

#include "Presets.h"

PresetManager::PresetManager (juce::AudioProcessorValueTreeState& s)
    : state (s)
{ refresh(); }

juce::File PresetManager::directory()
{
    // On macOS userApplicationDataDirectory is ~/Library, not ~/Library/Application Support.
    auto base = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory);

#if JUCE_MAC
    base = base.getChildFile ("Application Support");
#endif

    return base.getChildFile ("Rozan Audio").getChildFile ("Hyperspace").getChildFile ("Presets");
}

void PresetManager::refresh()
{
    const auto previous = getCurrentName();

    entries.clear();

    for (const auto& p : Presets::factory())
        entries.push_back ({ p.name, true, {} });

    auto dir = directory();

    if (dir.isDirectory())
    {
        auto files = dir.findChildFiles (juce::File::findFiles, false, "*" + fileExtension());
        files.sort();

        for (const auto& f : files)
            entries.push_back ({ f.getFileNameWithoutExtension(), false, f });
    }

    // keep pointing at the same preset where possible
    current = 0;

    for (size_t i = 0; i < entries.size(); ++i)
        if (entries[i].name == previous)
            current = static_cast<int> (i);
}

juce::String PresetManager::getCurrentName() const
{
    if (juce::isPositiveAndBelow (current, static_cast<int> (entries.size())))
        return entries[static_cast<size_t> (current)].name;

    return {};
}

bool PresetManager::currentIsFactory() const
{
    if (juce::isPositiveAndBelow (current, static_cast<int> (entries.size())))
        return entries[static_cast<size_t> (current)].isFactory;

    return true;
}

void PresetManager::load (int index)
{
    if (!juce::isPositiveAndBelow (index, static_cast<int> (entries.size())))
        return;

    current = index;
    const auto& entry = entries[static_cast<size_t> (index)];

    if (entry.isFactory)
    {
        Presets::apply (state, index);
    }
    else if (auto xml = juce::parseXML (entry.file))
    {
        if (xml->hasTagName (state.state.getType()))
            state.replaceState (juce::ValueTree::fromXml (*xml));
    }

    notify();
}

void PresetManager::step (int delta)
{
    const auto total = static_cast<int> (entries.size());

    if (total == 0)
        return;

    load (((current + delta) % total + total) % total);
}

bool PresetManager::saveAs (const juce::String& name)
{
    const auto trimmed = name.trim();

    if (trimmed.isEmpty())
        return false;

    auto dir = directory();

    if (!dir.isDirectory() && !dir.createDirectory().wasOk())
        return false;

    auto file = dir.getChildFile (juce::File::createLegalFileName (trimmed) + fileExtension());

    const auto xml = state.copyState().createXml();

    if (xml == nullptr || !xml->writeTo (file))
        return false;

    refresh();

    for (size_t i = 0; i < entries.size(); ++i)
        if (!entries[i].isFactory && entries[i].file == file)
            current = static_cast<int> (i);

    notify();
    return true;
}

bool PresetManager::overwriteCurrent()
{
    if (currentIsFactory())
        return false;

    return saveAs (getCurrentName());
}

bool PresetManager::deleteCurrent()
{
    if (currentIsFactory())
        return false;

    const auto file = entries[static_cast<size_t> (current)].file;

    if (!file.moveToTrash())
        return false;

    const auto previous = juce::jmax (0, current - 1);
    refresh();
    load (juce::jmin (previous, static_cast<int> (entries.size()) - 1));
    return true;
}

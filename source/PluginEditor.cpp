#include "PluginEditor.h"

#include "Parameters.h"
#include "PresetManager.h"
#include "gui/DriveMeter.h"
#include "gui/IconButton.h"
#include "gui/Knob.h"
#include "gui/LevelMeter.h"
#include "gui/ModDisplay.h"
#include "gui/PanelSection.h"
#include "gui/ResponseDisplay.h"
#include "gui/SegmentedControl.h"
#include "gui/TailDisplay.h"
#include "gui/Theme.h"
#include "gui/ToggleSwitch.h"

namespace
{
// The jump mark, shared with design/hyperspace-layout.html.
constexpr auto markSvg = R"SVG(
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 64 64">
  <defs>
    <linearGradient id="hm" x1="0" y1="0" x2="0" y2="1">
      <stop offset="0" stop-color="#4a4a54"/><stop offset="38%" stop-color="#1b1b21"/>
      <stop offset="1" stop-color="#050507"/>
    </linearGradient>
    <radialGradient id="core" cx="50%" cy="50%" r="50%">
      <stop offset="0" stop-color="#eafffb"/><stop offset="35%" stop-color="#63ead9"/>
      <stop offset="1" stop-color="#0c7367" stop-opacity="0"/>
    </radialGradient>
    <linearGradient id="str" x1="0" y1="0" x2="1" y2="0">
      <stop offset="0" stop-color="#15c6b2" stop-opacity="0"/>
      <stop offset="1" stop-color="#9ff6e9"/>
    </linearGradient>
  </defs>
  <circle cx="32" cy="32" r="28" fill="url(#hm)" stroke="#15c6b2" stroke-width="1.1" stroke-opacity=".45"/>
  <circle cx="32" cy="32" r="22" fill="none" stroke="#000" stroke-width="1.6"/>
  <circle cx="32" cy="32" r="22" fill="none" stroke="#5b5b66" stroke-width=".7" stroke-opacity=".6"/>
  <g stroke="url(#str)" stroke-width="1.5" stroke-linecap="round" fill="none">
    <path d="M32 32 L32 8"/><path d="M32 32 L52 15"/><path d="M32 32 L56 32"/>
    <path d="M32 32 L52 49"/><path d="M32 32 L32 56"/><path d="M32 32 L12 49"/>
    <path d="M32 32 L8 32"/><path d="M32 32 L12 15"/>
  </g>
  <g stroke="url(#str)" stroke-width=".9" stroke-opacity=".6" stroke-linecap="round" fill="none">
    <path d="M32 32 L44 11"/><path d="M32 32 L55 22"/><path d="M32 32 L55 43"/>
    <path d="M32 32 L44 54"/><path d="M32 32 L20 54"/><path d="M32 32 L9 43"/>
    <path d="M32 32 L9 22"/><path d="M32 32 L20 11"/>
  </g>
  <circle cx="32" cy="32" r="13" fill="url(#core)"/>
  <circle cx="32" cy="32" r="3.4" fill="#f2fffc"/>
</svg>)SVG";

/** Adds every control to its panel without naming juce::Component* lists by hand. */
template <typename... Components>
void addAll (juce::Component& parent, Components&... items)
{ (parent.addAndMakeVisible (items), ...); }

template <typename... Components>
void fadeAll (float alpha, Components&... items)
{ (items.setAlpha (alpha), ...); }

juce::RangedAudioParameter& paramRef (juce::AudioProcessorValueTreeState& state, const char* id)
{
    auto* p = state.getParameter (id);
    jassert (p != nullptr);
    return *p;
}

/** Caption above a segmented control. */
class Caption final : public juce::Component
{
public:
    explicit Caption (juce::String t)
        : text (std::move (t))
    { setInterceptsMouseClicks (false, false); }

    void paint (juce::Graphics& g) override
    {
        g.setColour (Theme::textDim);
        g.setFont (Theme::label (9.0f, true, 0.2f));
        g.drawText (text, getLocalBounds(), juce::Justification::centredLeft, false);
    }

private:
    juce::String text;
};

/** The power LED in the header; lit means not bypassed. */
class PowerLed final : public juce::Component
{
public:
    explicit PowerLed (juce::RangedAudioParameter& p)
        : attachment (
              p,
              [this] (float v) {
                  on = v < 0.5f;
                  repaint();
              },
              nullptr)
    {
        attachment.sendInitialUpdate();
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }

    void mouseDown (const juce::MouseEvent&) override { attachment.setValueAsCompleteGesture (on ? 1.0f : 0.0f); }

    void paint (juce::Graphics& g) override
    {
        auto area = getLocalBounds();
        auto ledArea = area.removeFromTop (area.getHeight() - 12);
        const auto led = juce::Rectangle<float> (11.0f, 11.0f).withCentre (ledArea.toFloat().getCentre());

        if (on)
        {
            g.setColour (Theme::saber.withAlpha (0.35f));
            g.fillEllipse (led.expanded (9.0f));
            g.setColour (Theme::saber.withAlpha (0.55f));
            g.fillEllipse (led.expanded (4.0f));
        }

        g.setGradientFill ({ on ? juce::Colour (0xffff8a92) : juce::Colour (0xff4a1a1e),
                             led.getX() + led.getWidth() * 0.36f, led.getY() + led.getHeight() * 0.32f,
                             on ? juce::Colour (0xff7d0a13) : juce::Colour (0xff1a0608), led.getCentreX(),
                             led.getBottom(), true });
        g.fillEllipse (led);

        g.setColour (Theme::textDim);
        g.setFont (Theme::label (8.0f, true, 0.2f));
        g.drawText ("POWER", area, juce::Justification::centredTop, false);
    }

private:
    juce::ParameterAttachment attachment;
    bool on = true;
};

/** Preset name display with prev/next arrows, backed by the PresetManager. */
class PresetBar final : public juce::Component
{
public:
    explicit PresetBar (PresetManager& m)
        : manager (m)
    { setMouseCursor (juce::MouseCursor::PointingHandCursor); }

    void mouseDown (const juce::MouseEvent& e) override
    {
        constexpr int arrow = 28;

        if (e.x < arrow)
            manager.step (-1);
        else if (e.x > getWidth() - arrow)
            manager.step (1);
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        constexpr float arrow = 28.0f;

        g.setColour (juce::Colour (0xff0b0b0e));
        g.fillRoundedRectangle (bounds, 4.0f);
        g.setColour (juce::Colours::black);
        g.drawRoundedRectangle (bounds.reduced (0.5f), 4.0f, 1.0f);

        const auto display = bounds.reduced (arrow, 1.0f);
        g.setColour (Theme::displayBg);
        g.fillRect (display);

        g.setColour (Theme::tealBright.withAlpha (0.25f));
        g.drawVerticalLine (static_cast<int> (display.getX()), display.getY(), display.getBottom());
        g.drawVerticalLine (static_cast<int> (display.getRight()), display.getY(), display.getBottom());

        g.setColour (Theme::tealBright);
        g.setFont (Theme::label (11.0f, true, 0.2f));
        g.drawText (manager.getCurrentName().toUpperCase(), display.toNearestInt(), juce::Justification::centred,
                    false);

        g.setColour (Theme::textDim);
        g.setFont (juce::FontOptions (13.0f));
        g.drawText (juce::String::fromUTF8 ("\xe2\x97\x82"), bounds.removeFromLeft (arrow).toNearestInt(),
                    juce::Justification::centred, false);
        g.drawText (juce::String::fromUTF8 ("\xe2\x96\xb8"), bounds.removeFromRight (arrow).toNearestInt(),
                    juce::Justification::centred, false);
    }

private:
    PresetManager& manager;
};
} // namespace

//==============================================================================
class HyperspaceEditor::Content final : public juce::Component, private juce::Timer
{
public:
    explicit Content (HyperspaceProcessor& p)
        : proc (p)
        , state (p.getValueTreeState())
        , enginePanel ("REVERB ENGINE", 32, false)
        , tonePanel ("TONE & DAMPING")
        , modPanel ("MODULATION", 32, false)
        , preAmpPanel ("PRE AMP", 29, false)
        , tailPanel ("REVERB TAIL", 29, false)
        , outputPanel ("OUTPUT")
        , decay (paramRef (state, ParamID::decay), "DECAY", Theme::teal, true)
        , size (paramRef (state, ParamID::size), "SIZE", Theme::teal)
        , preDelay (paramRef (state, ParamID::preDelay), "PRE-DLY", Theme::teal)
        , diffusion (paramRef (state, ParamID::diffusion), "DIFFUSE", Theme::teal)
        , earlyLate (paramRef (state, ParamID::earlyLate), "EARLY/LATE", Theme::teal)
        , lowCut (paramRef (state, ParamID::lowCut), "LOW CUT", Theme::teal)
        , highCut (paramRef (state, ParamID::highCut), "HIGH CUT", Theme::teal)
        , damping (paramRef (state, ParamID::damping), "DAMP", Theme::teal)
        , tone (paramRef (state, ParamID::tone), "TONE", Theme::teal)
        , modRate (paramRef (state, ParamID::modRate), "RATE", Theme::teal)
        , modDepth (paramRef (state, ParamID::modDepth), "DEPTH", Theme::teal)
        , modSpread (paramRef (state, ParamID::modSpread), "SPREAD", Theme::saber)
        , modMix (paramRef (state, ParamID::modMix), "MOD MIX", Theme::teal)
        , preGain (paramRef (state, ParamID::preGain), "GAIN", Theme::teal)
        , preTone (paramRef (state, ParamID::preTone), "TONE", Theme::teal)
        , preOutput (paramRef (state, ParamID::preOutput), "OUTPUT", Theme::teal)
        , duck (paramRef (state, ParamID::duck), "DUCK", Theme::saber)
        , mix (paramRef (state, ParamID::mix), "MIX", Theme::teal)
        , width (paramRef (state, ParamID::width), "WIDTH", Theme::teal)
        , level (paramRef (state, ParamID::level), "LEVEL", Theme::teal)
        , typeSegment (paramRef (state, ParamID::reverbType), { "ROOM", "HALL", "PLATE", "SPRING", "AMB" })
        , preSyncSegment (paramRef (state, ParamID::preDelaySync), Params::preSyncChoices)
        , modeSegment (paramRef (state, ParamID::modMode), { "CHORUS", "VIBRATO", "PHASER" })
        , shapeSegment (paramRef (state, ParamID::modShape), { "SINE", "TRI", "RND" })
        , targetSegment (paramRef (state, ParamID::modTarget), { "DELAY", "FILTER", "BOTH" })
        , rateSyncSegment (paramRef (state, ParamID::modSync), Params::rateSyncChoices)
        , preSyncCaption ("PRE-DELAY SYNC")
        , shapeCaption ("SHAPE")
        , targetCaption ("TARGET")
        , rateSyncCaption ("RATE SYNC")
        , driveCaption ("DRIVE")
        , modeCaption ("MODE")
        , response (state)
        , modDisplay (p)
        , tailDisplay (p)
        , meter ([&p] (int ch) { return p.getOutputPeak (ch); })
        , drive ([&p] { return p.getDrive(); })
        , modSwitch (paramRef (state, ParamID::modOn), "ON", Theme::teal)
        , preAmpSwitch (paramRef (state, ParamID::preAmpOn), "ON", Theme::teal)
        , freezeSwitch (paramRef (state, ParamID::freeze), "FREEZE", Theme::teal)
        , duckSwitch (paramRef (state, ParamID::duckOn), "DUCK", Theme::teal)
        , presets (p.getPresetManager())
        , newPreset (IconButton::Icon::plus, "New preset", Theme::teal)
        , savePreset (IconButton::Icon::save, "Save preset", Theme::text.withAlpha (0.75f))
        , deletePreset (IconButton::Icon::trash, "Delete preset", Theme::text.withAlpha (0.75f))
        , power (paramRef (state, ParamID::bypass))
    {
        if (auto xml = juce::parseXML (markSvg))
            mark = juce::Drawable::createFromSVG (*xml);

        for (auto* panel : { &enginePanel, &tonePanel, &modPanel, &preAmpPanel, &tailPanel, &outputPanel })
            addAndMakeVisible (panel);

        addAndMakeVisible (presets);
        addAndMakeVisible (newPreset);
        addAndMakeVisible (savePreset);
        addAndMakeVisible (deletePreset);
        addAndMakeVisible (power);

        addAll (enginePanel, typeSegment, decay, size, preDelay, diffusion, earlyLate, preSyncCaption, preSyncSegment);
        addAll (tonePanel, lowCut, highCut, damping, tone, response);
        addAll (modPanel, modSwitch, modeCaption, modeSegment, modRate, modDepth, modSpread, modMix, shapeCaption,
                shapeSegment, targetCaption, targetSegment, rateSyncCaption, rateSyncSegment, modDisplay);
        addAll (preAmpPanel, preAmpSwitch, preGain, preTone, preOutput, driveCaption, drive);
        addAll (tailPanel, freezeSwitch, duckSwitch, duck, tailDisplay);
        addAll (outputPanel, mix, width, level, meter);

        modSwitch.onStateChange = [this] (bool on) { setModActive (on); };
        preAmpSwitch.onStateChange = [this] (bool on) { setPreAmpActive (on); };

        setModActive (modSwitch.isOn());
        setPreAmpActive (preAmpSwitch.isOn());
        applyMode (currentMode(), true);

        newPreset.onClick = [this] { promptForName(); };
        savePreset.onClick = [this] {
            if (!proc.getPresetManager().overwriteCurrent())
                reportFailure ("Could not save the preset.");
        };
        deletePreset.onClick = [this] { confirmDelete(); };

        proc.getPresetManager().onChange = [this] { presetChanged(); };
        presetChanged();

        startTimerHz (10);

        setSize (HyperspaceEditor::designWidth, HyperspaceEditor::designHeight);
    }

    ~Content() override { proc.getPresetManager().onChange = nullptr; }

    void resized() override
    {
        // Panel and control rectangles come straight from design/hyperspace-layout.html.
        header = { 13, 13, 1014, 70 };
        footer = { 13, 622, 1014, 13 };

        enginePanel.setBounds (13, 92, 500, 186);
        tonePanel.setBounds (522, 92, 505, 186);
        modPanel.setBounds (13, 287, 737, 186);
        preAmpPanel.setBounds (759, 287, 268, 186);
        tailPanel.setBounds (13, 482, 675, 131);
        outputPanel.setBounds (697, 482, 330, 131);

        // ---- header ----
        markArea = { 29, 24, 48, 48 };
        presets.setBounds (609, 33, 235, 30);
        newPreset.setBounds (867, 35, 26, 26);
        savePreset.setBounds (898, 35, 26, 26);
        deletePreset.setBounds (929, 35, 26, 26);
        power.setBounds (981, 36, 30, 36);

        // ---- reverb engine ----
        typeSegment.setBounds (237, 6, 252, 22);
        decay.setBounds (13, 49, 84, 118);
        size.setBounds (111, 42, 56, 85);
        preDelay.setBounds (218, 42, 56, 85);
        diffusion.setBounds (324, 42, 56, 85);
        earlyLate.setBounds (431, 42, 56, 85);
        preSyncCaption.setBounds (111, 134, 376, 12);
        preSyncSegment.setBounds (111, 151, 376, 22);

        // ---- tone and damping ----
        lowCut.setBounds (13, 48, 56, 85);
        highCut.setBounds (80, 48, 56, 85);
        damping.setBounds (147, 48, 56, 85);
        tone.setBounds (214, 48, 56, 85);
        response.setBounds (284, 30, 208, 120);

        // ---- modulation ----
        modSwitch.setBounds (406, 8, 46, 19);
        modeCaption.setBounds (460, 9, 34, 14);
        modeSegment.setBounds (498, 6, 228, 22);
        modRate.setBounds (13, 42, 56, 85);
        modDepth.setBounds (122, 42, 56, 85);
        modSpread.setBounds (230, 42, 56, 85);
        modMix.setBounds (339, 42, 56, 85);
        shapeCaption.setBounds (13, 134, 104, 12);
        shapeSegment.setBounds (13, 151, 104, 22);
        targetCaption.setBounds (127, 134, 124, 12);
        targetSegment.setBounds (127, 151, 124, 22);
        rateSyncCaption.setBounds (261, 134, 134, 12);
        rateSyncSegment.setBounds (261, 151, 134, 22);
        modDisplay.setBounds (409, 45, 315, 126);

        // ---- pre amp ----
        preAmpSwitch.setBounds (211, 6, 46, 19);
        preGain.setBounds (13, 39, 56, 85);
        preTone.setBounds (106, 39, 56, 85);
        preOutput.setBounds (199, 39, 56, 85);
        driveCaption.setBounds (13, 131, 242, 12);
        drive.setBounds (13, 148, 242, 13);

        // ---- reverb tail ----
        freezeSwitch.setBounds (531, 6, 67, 19);
        duckSwitch.setBounds (605, 6, 59, 19);
        duck.setBounds (13, 41, 50, 79);
        tailDisplay.setBounds (75, 38, 587, 84);

        // ---- output ----
        mix.setBounds (15, 27, 50, 79);
        width.setBounds (77, 27, 50, 79);
        level.setBounds (139, 27, 50, 79);
        meter.setBounds (205, 31, 22, 58);
        meterLabel = { 205 + outputPanel.getX(), 94 + outputPanel.getY(), 22, 10 };
    }

    void paint (juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds().toFloat();

        g.setColour (Theme::background);
        g.fillAll();

        // crimson rim light above, teal glow below, as in the mockup
        g.setGradientFill (juce::ColourGradient (Theme::saber.withAlpha (0.12f), bounds.getCentreX(),
                                                 -bounds.getHeight() * 0.25f, juce::Colours::transparentBlack,
                                                 bounds.getCentreX(), bounds.getHeight() * 0.62f, true));
        g.fillRect (bounds);

        g.setGradientFill (juce::ColourGradient (Theme::teal.withAlpha (0.10f), bounds.getCentreX(), bounds.getBottom(),
                                                 juce::Colours::transparentBlack, bounds.getCentreX(),
                                                 bounds.getHeight() * 0.55f, true));
        g.fillRect (bounds);

        // ---- header plate ----
        const auto plate = header.toFloat();
        g.setGradientFill ({ Theme::panelTop, plate.getX(), plate.getY(), Theme::panelBottom, plate.getX(),
                             plate.getBottom(), false });
        g.fillRoundedRectangle (plate, 5.0f);
        g.setColour (Theme::panelEdge);
        g.drawRoundedRectangle (plate.reduced (0.5f), 5.0f, 1.0f);
        g.setColour (Theme::panelHilite);
        g.drawHorizontalLine (static_cast<int> (plate.getY() + 1.0f), plate.getX() + 3.0f, plate.getRight() - 3.0f);

        if (mark != nullptr)
            mark->drawWithin (g, markArea.toFloat(), juce::RectanglePlacement::centred, 1.0f);

        g.setColour (Theme::text);
        g.setFont (Theme::displayFont (30.0f, 0.17f));
        g.drawText ("HYPERSPACE", juce::Rectangle<int> { 91, 24, 420, 34 }, juce::Justification::bottomLeft, false);

        g.setColour (Theme::textDim);
        g.setFont (Theme::label (9.0f, true, 0.42f));
        g.drawText ("MODULATED REVERB", juce::Rectangle<int> { 91, 58, 420, 12 }, juce::Justification::topLeft, false);

        // ---- meter caption + footer ----
        g.setColour (Theme::textDim);
        g.setFont (Theme::label (8.0f, true, 0.18f));
        g.drawText ("L   R", meterLabel, juce::Justification::centred, false);

        g.setColour (juce::Colour (0xff4e4c48));
        g.setFont (Theme::label (8.0f, true, 0.3f));
        g.drawText ("ROZAN AUDIO", footer, juce::Justification::centredLeft, false);

        auto right = footer;
        auto versionArea = right.removeFromRight (52);
        right.removeFromRight (8);

        // juce::String (const char*) decodes as ASCII, so the middle dot needs the UTF-8 pointer
        g.drawText (juce::String (juce::CharPointer_UTF8 ("VST3  \xc2\xb7  STANDALONE")), right.removeFromRight (160),
                    juce::Justification::centredRight, false);
        g.setColour (Theme::teal.withAlpha (0.85f));
        g.drawText ("v" JucePlugin_VersionString, versionArea, juce::Justification::centredRight, false);
    }

private:
    /** A switched-off section keeps its settings readable but stops drawing attention. */
    void setModActive (bool on)
    {
        modPanel.setActive (on);

        fadeAll (on ? 1.0f : Theme::disabledAlpha, modeCaption, modeSegment, modRate, modDepth, modSpread, modMix,
                 shapeCaption, shapeSegment, targetCaption, targetSegment, rateSyncCaption, rateSyncSegment,
                 modDisplay);
    }

    void setPreAmpActive (bool on)
    {
        preAmpPanel.setActive (on);

        fadeAll (on ? 1.0f : Theme::disabledAlpha, preGain, preTone, preOutput, driveCaption, drive);
    }

    int currentMode() const
    {
        auto* v = state.getRawParameterValue (ParamID::modMode);
        return v != nullptr ? juce::jlimit (0, 2, juce::roundToInt (v->load())) : 0;
    }

    /**
        The mode's hue runs through the selector, the SPREAD knob and the display, so
        the control and the picture are visibly the same thing. Vibrato is the
        modulated path alone, so MOD MIX is pinned at 100 % and greys out, the same
        way a knob does when the host tempo takes it over.
    */
    void applyMode (int mode, bool force)
    {
        if (!force && mode == shownMode)
            return;

        shownMode = mode;

        const auto palette = Theme::modePalette (mode);
        modeSegment.setAccentColour (palette.colour);
        modSpread.setAccentColour (palette.colour);

        const auto vibrato = mode == 1;
        modMix.setDimmed (vibrato);
        modMix.setOverrideText (vibrato ? juce::String ("100 %") : juce::String());
    }

    /** While a rate or pre-delay follows the host, its knob is no longer in charge. */
    void timerCallback() override
    {
        const auto choice = [this] (const char* id) {
            auto* v = state.getRawParameterValue (id);
            return v != nullptr ? juce::roundToInt (v->load()) : 0;
        };

        applyMode (currentMode(), false);

        const auto rateSynced = SyncMath::isSynced (choice (ParamID::modSync));
        const auto preSynced = SyncMath::isSynced (choice (ParamID::preDelaySync));

        if (rateSynced != wasRateSynced)
        {
            modRate.setDimmed (rateSynced);
            wasRateSynced = rateSynced;
        }

        if (preSynced != wasPreSynced)
        {
            preDelay.setDimmed (preSynced);
            wasPreSynced = preSynced;
        }

        modRate.setOverrideText (rateSynced ? juce::String (proc.getEffectiveRateHz(), 2) + " Hz" : juce::String());
        preDelay.setOverrideText (preSynced ? juce::String (juce::roundToInt (proc.getEffectivePreDelayMs())) + " ms"
                                            : juce::String());
    }

    static void reportFailure (const juce::String& message)
    { juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Hyperspace", message); }

    void presetChanged()
    {
        const auto factory = proc.getPresetManager().currentIsFactory();
        savePreset.setEnabled (!factory);
        deletePreset.setEnabled (!factory);
        presets.repaint();
    }

    void promptForName()
    {
        auto* window = new juce::AlertWindow ("New Preset", "Name this preset", juce::MessageBoxIconType::NoIcon);
        window->addTextEditor ("name", proc.getPresetManager().getCurrentName());
        window->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
        window->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

        juce::Component::SafePointer<Content> safeThis { this };
        juce::Component::SafePointer<juce::AlertWindow> safeWindow { window };

        window->enterModalState (
            true, juce::ModalCallbackFunction::create ([safeThis, safeWindow] (int result) {
                if (result != 1 || safeThis == nullptr || safeWindow == nullptr)
                    return;

                if (!safeThis->proc.getPresetManager().saveAs (safeWindow->getTextEditorContents ("name")))
                    safeThis->reportFailure ("Could not save the preset. Check that " +
                                             PresetManager::directory().getFullPathName() + " is writable.");
            }),
            true);
    }

    void confirmDelete()
    {
        auto& manager = proc.getPresetManager();

        if (manager.currentIsFactory())
            return;

        juce::Component::SafePointer<Content> safeThis { this };

        juce::AlertWindow::showOkCancelBox (juce::MessageBoxIconType::WarningIcon, "Delete Preset",
                                            "Move \"" + manager.getCurrentName() + "\" to the trash?", "Delete",
                                            "Cancel", nullptr,
                                            juce::ModalCallbackFunction::create ([safeThis] (int result) {
                                                if (result != 1 || safeThis == nullptr)
                                                    return;

                                                if (!safeThis->proc.getPresetManager().deleteCurrent())
                                                    safeThis->reportFailure ("Could not move the preset to the trash.");
                                            }));
    }

    HyperspaceProcessor& proc;
    juce::AudioProcessorValueTreeState& state;

    PanelSection enginePanel, tonePanel, modPanel, preAmpPanel, tailPanel, outputPanel;
    Knob decay, size, preDelay, diffusion, earlyLate;
    Knob lowCut, highCut, damping, tone;
    Knob modRate, modDepth, modSpread, modMix;
    Knob preGain, preTone, preOutput, duck, mix, width, level;
    ParameterSegment typeSegment, preSyncSegment, modeSegment, shapeSegment, targetSegment, rateSyncSegment;
    Caption preSyncCaption, shapeCaption, targetCaption, rateSyncCaption, driveCaption, modeCaption;
    ResponseDisplay response;
    ModDisplay modDisplay;
    TailDisplay tailDisplay;
    LevelMeter meter;
    DriveMeter drive;
    ToggleSwitch modSwitch, preAmpSwitch, freezeSwitch, duckSwitch;
    PresetBar presets;
    IconButton newPreset, savePreset, deletePreset;
    PowerLed power;

    std::unique_ptr<juce::Drawable> mark;
    juce::Rectangle<int> header, footer, markArea, meterLabel;
    bool wasRateSynced = false, wasPreSynced = false;
    int shownMode = -1;
};

//==============================================================================
HyperspaceEditor::HyperspaceEditor (HyperspaceProcessor& p)
    : AudioProcessorEditor (&p)
{
    setLookAndFeel (&lookAndFeel);

    content = std::make_unique<Content> (p);
    addAndMakeVisible (*content);

    constrainer.setFixedAspectRatio (static_cast<double> (designWidth) / static_cast<double> (designHeight));
    constrainer.setSizeLimits (designWidth / 2, designHeight / 2, designWidth * 2, designHeight * 2);
    setConstrainer (&constrainer);

    setResizable (true, true);
    setSize (designWidth, designHeight);
}

HyperspaceEditor::~HyperspaceEditor()
{
    setConstrainer (nullptr);
    setLookAndFeel (nullptr);
}

void HyperspaceEditor::paint (juce::Graphics& g) { g.fillAll (Theme::background); }

void HyperspaceEditor::resized()
{
    const auto scale = static_cast<float> (getWidth()) / static_cast<float> (designWidth);

    content->setBounds (0, 0, designWidth, designHeight);
    content->setTransform (juce::AffineTransform::scale (scale));
}

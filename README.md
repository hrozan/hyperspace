# Hyperspace

[![CI](https://github.com/hrozan/hyperspace/actions/workflows/ci.yml/badge.svg)](https://github.com/hrozan/hyperspace/actions/workflows/ci.yml)

A modulated reverb plugin built with [JUCE](https://juce.com) and CMake. Sibling to
[Darth Phaser](../darth-phaser), whose chassis, knobs and panels it reuses in teal.

> Status: DSP and GUI both implemented. Next up: factory preset polish and plugin validation.

## How it works

A console-style pre amp in front, then pre-delay, input diffusion and an eight-line
feedback delay network. Each line is a
fractional delay followed by a one-pole damping filter and a decay gain; the lines are mixed
every sample through a normalised 8x8 Hadamard matrix, which is orthogonal, so the tank
redistributes energy without creating any. Decay time sets the per-line gain directly:
`g = 10^(-3 L / RT60)` for a line of length `L` seconds, so one pass round a line loses exactly
as much as that line's share of 60 dB. Early reflections come from a separate tapped delay line
and are balanced against the tank by Early/Late.

`source/ReverbEngine.{h,cpp}` and `source/PreAmp.{h,cpp}` hold the DSP and deliberately depend on
no JUCE types, so they can be compiled and tested on their own — `tests/DspTests.cpp` does
exactly that.
`source/Parameters.{h,cpp}` declares the parameters and `PluginProcessor` maps them onto the
engine each block.

### The modulation stage

The point of the plugin. One three-way `modMode` parameter picks what the LFO drives, and the
three modes differ in how the modulated path is blended back:

| Mode | Stage | Blend |
| --- | --- | --- |
| **Chorus** | Modulated delay, 12 ms +/- 6 ms | Against the unmodulated tail, by Mod Mix |
| **Vibrato** | The same delay, 12 ms +/- 4 ms | Modulated path only; Mod Mix is forced to 100 % |
| **Phaser** | Six swept first-order all-passes | Against the unmodulated tail, by Mod Mix |

That is the real difference between the three effects, not three separate engines: chorus and
phaser need the dry path alongside the modulated one for the comb or the notches to appear, and
vibrato is what you get when you take it away. Phaser mode is Darth Phaser's all-pass cascade,
moved inside the reverb.

Mod Target decides what the LFO reaches: `Delay` drives the stage above plus the tank's own line
lengths, `Filter` drives the damping cutoff instead so the tail breathes darker and brighter, and
`Both` does the two together. Under `Filter` the delay stage is bypassed rather than left sitting
at a fixed offset, which would be a static comb.

Spread offsets the right channel's LFO by up to half a cycle, which is what decorrelates the two
sides.

### The pre amp

Gain into a `tanh` saturation, a tilt around 700 Hz, then an output trim. It is an input stage,
so it runs **before the dry/wet split**: the dry side of Mix is the pre-amped signal, not the
untouched input, which is what makes it behave like a console channel rather than a wet-only
colour. `tanh` has unity slope around zero, so clean settings stay clean and the curve only
rounds off as the gain pushes it.

Switching it off fades the whole stage — gain, tilt and trim ride the same fade — and with the
fade at zero it is a straight wire, which the tests assert sample for sample.

The DRIVE meter reads the peak of what the saturation sees, after the gain and before the curve,
not the output level: that is the number that tells you how hard the stage is working, and it is
the one the output trim cannot hide.

### The rest

- **Freeze** sets the line gains to unity, bypasses the damping filter and mutes the input into
  the tank and the early reflections, so what is already in there is all you hear.
- **Ducking** follows the dry input with a 5 ms attack and a 250 ms release and pulls the wet
  down by up to the Duck setting.
- **Switching Modulation off** crossfades over about 15 ms instead of switching. It sits around
  the feedback path, and an abrupt change there clicks. The pre amp fades the same way.
- Coefficients are refreshed every 32 samples rather than per sample; anything that would zipper
  at that rate — the modulated delay lengths, the phaser coefficients — is ramped across the
  block instead.

## Parameters

### Reverb engine
| Control | Range | Notes |
| --- | --- | --- |
| Type | Room, Hall, Plate, Spring, Ambient | Scales the tank and biases diffusion and damping |
| Decay | 0.1 - 20 s | RT60 of the tail |
| Size | 0 - 100 % | Scales the delay-line lengths, 0.5x to 1.5x |
| Pre-Delay | 0 - 250 ms | Tempo-syncable: MS, 1/16, 1/8, 1/4 |
| Diffusion | 0 - 100 % | All-pass gain in the input diffusers, smooth versus grainy |
| Early/Late | 0 - 100 % | Early reflections against the tail |

### Tone and damping
| Control | Range | Notes |
| --- | --- | --- |
| Low Cut | 20 Hz - 1 kHz | High-pass into the tank |
| High Cut | 1 - 20 kHz | Low-pass into the tank |
| Damp | 0 - 100 % | Loop cutoff, 20 kHz wide open down to 1 kHz |
| Tone | -12 to +12 dB | Tilt around 700 Hz on the wet signal |

### Modulation
| Control | Range | Notes |
| --- | --- | --- |
| Modulation | on / off | Panel switch; crossfades |
| Mode | Chorus, Vibrato, Phaser | See above |
| Rate | 0.01 - 10 Hz | Tempo-syncable: FREE, 1/2, 1/4, 1/8 |
| Depth | 0 - 100 % | How far the tail moves |
| Spread | 0 - 100 % | L/R LFO offset, 0 to half a cycle |
| Shape | Sine, Triangle, Random | Random is smoothed value noise |
| Target | Delay, Filter, Both | What the LFO drives |
| Mod Mix | 0 - 100 % | Modulated against unmodulated; forced to 100 % in Vibrato |

### Pre amp, tail and output
| Control | Range | Notes |
| --- | --- | --- |
| Pre Amp | on / off | Panel switch; off is a straight wire |
| Gain | -12 to +24 dB | Drive into the saturation |
| Tone | -12 to +12 dB | Tilt around 700 Hz |
| Output | -24 to +12 dB | Trim after the stage |
| Freeze | on / off | Holds the tail |
| Duck | on / off, 0 to -24 dB | Lowers the wet while the dry is playing |
| Mix | 0 - 100 % | Dry / wet |
| Width | 0 - 200 % | Mid/side width of the wet |
| Level | -24 to +12 dB | Output gain |
| Bypass | on / off | The power LED, exposed to the host through `getBypassParameter()` |

## Formats

- VST3
- Standalone application

Audio Unit is not built yet because it needs a full Xcode install (the Command Line Tools alone
are not enough). Add `AU` to `FORMATS` in `CMakeLists.txt` once Xcode is available.

## Requirements

- CMake 3.22+
- A C++20 compiler (Xcode Command Line Tools on macOS, MSVC on Windows, GCC/Clang on Linux)
- Git (JUCE is fetched automatically via `FetchContent`)

## Build

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The first configure clones JUCE, so it takes a few minutes. Subsequent builds are fast.

To work on the DSP alone, turn the plugin off and skip JUCE altogether — this configures and
builds in a couple of seconds and is exactly what CI runs per commit:

```sh
cmake -B build-dsp -DHYPERSPACE_BUILD_PLUGIN=OFF
cmake --build build-dsp && ctest --test-dir build-dsp --output-on-failure
```

For a faster local iteration loop, build a single architecture and use Debug:

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_OSX_ARCHITECTURES=arm64
cmake --build build
```

`COPY_PLUGIN_AFTER_BUILD` is enabled, so a successful build installs the VST3 into the user
plugin folder (`~/Library/Audio/Plug-Ins/VST3` on macOS). Pass
`-DHYPERSPACE_INSTALL_AFTER_BUILD=OFF` to opt out.

### Run the standalone

```sh
open "build/Hyperspace_artefacts/Release/Standalone/Hyperspace.app"
```

### Tests

```sh
cmake --build build --target HyperspaceDspTests
ctest --test-dir build --output-on-failure
```

### CI

Two workflows, split by cost. Every commit gets the cheap half; the expensive plugin build only
runs when there is something to ship.

**`.github/workflows/ci.yml` — every push and pull request.**

- *Lint.* `clang-format --dry-run --Werror` over every source file, plus a whitespace pass: no
  tabs, no trailing spaces, newline at end of file.
- *Test.* macOS, Linux and Windows, each configuring with `-DHYPERSPACE_BUILD_PLUGIN=OFF`. That
  drops JUCE entirely — the two DSP stages are plain C++20 — so there is no clone, no GUI
  development headers and no cache to warm, and the job finishes in seconds.

**`.github/workflows/release.yml` — tags matching `v*`, or run by hand.**

Builds the real plugin on all three platforms with `-DHYPERSPACE_INSTALL_AFTER_BUILD=OFF`, runs
`ctest` again against the full build, and packages the VST3 and standalone per platform —
`tar.gz` on macOS and Linux, because the `.vst3` and `.app` are bundles whose symlinks and
executable bits `zip` does not keep, and `.zip` on Windows. On a tag it then opens a GitHub
release and attaches the three archives. The JUCE checkout is cached against the hash of
`CMakeLists.txt`, so bumping the pinned tag refetches it and nothing else does.

Compiling JUCE is most of the Linux job, so that one also runs through `ccache`, which makes
every run after the first a cache lookup. macOS is left out because the universal build passes
two `-arch` flags in a single invocation and ccache will not cache that; MSVC is not a ccache
target at all.

Three Linux packages are there for the configure step rather than the link: `JUCEUtils.cmake`
pkg-configs `libcurl`, `webkit2gtk` and `gtk+-x11-3.0` when it is included, before any
`NEEDS_WEB_BROWSER` or `JUCE_USE_CURL` setting is read, so they are required even though this
plugin uses none of them. JUCE probes `webkit2gtk-4.1` before falling back to `4.0`, which is
what lets the job run on `ubuntu-24.04` — `4.0` is gone there.

```sh
git tag v0.1.0 && git push origin v0.1.0
```

### Formatting

`.clang-format` is tuned to the style already in the tree: Allman braces, four spaces, a space
before non-empty argument lists but not `foo()`, a 120-column limit, and `AfterEnum: false`,
without which clang-format refuses to leave a short `enum class` on one line. Consecutive
alignment is off — it cannot tell a struct's members from a function's locals, and padding out
locals is noisier than the hand-aligned columns it replaces.

The whole tree is formatted, so run it over anything you touch before pushing:

```sh
clang-format -i $(git ls-files 'source/*.cpp' 'source/*.h' 'tests/*.cpp')
```

The pinned version is in `ci.yml` (`CLANG_FORMAT_VERSION`); different clang-format releases
disagree on details, so match it or CI will flag lines your copy is happy with.

One thing the formatter costs: a handful of two-statement accessors that were written inline,
like `float lowpass (float x, float c) { z += c * (x - z); return z; }`, are now spread over
five lines. clang-format has no option to keep a multi-statement body on one line.

`tests/DspTests.cpp` drives both DSP stages directly — no JUCE, no host, no audio device. Each
reverb case feeds an impulse or a sine and then silence, and asserts the output stays finite and
bounded and has decayed by the end: every reverb type, modulation mode, shape and target, the
extremes of decay and size, and the sample rates from 44.1 to 96 kHz. Freeze is checked the other
way round, that the tail is still there ten seconds later. The pre amp cases check that bypass is
a straight wire, that the saturation holds the level however hard it is driven, and that the
drive reading rises with signal and falls back to nothing.

## GUI

The editor is built from `design/hyperspace-layout.png` at a logical 1040x650 and scales to
whatever the host gives it, with the aspect ratio locked. Panel and control rectangles in
`PluginEditor::Content::resized()` are the measured rectangles from the mockup, so the two stay
in sync.

```
source/gui/
  Theme.h               # palette and type
  HyperLookAndFeel.*    # knurled MXR-style knob with a glowing value arc
  Knob.*                # knob + caption + formatted readout, bound to a parameter
  SegmentedControl.*    # row of exclusive buttons, plus a parameter-bound wrapper
  ToggleSwitch.*        # lit-dot switch: the ON, FREEZE and DUCK toggles
  PanelSection.*        # bevelled panel with the teal title bar
  ResponseDisplay.*     # cuts, tail damping and tone tilt as one curve
  ModDisplay.*          # the LFO drawn twice, separated by Spread, coloured by mode
  TailDisplay.*         # pre-delay gap, early reflection taps, decay envelope
  LevelMeter.*          # segmented L/R output meters
  DriveMeter.*          # horizontal bar reading how hard the pre amp is driven
  IconButton.*          # new / save / delete preset icons
```

The three displays are driven by the live parameters rather than being decoration.
`ResponseDisplay` shares `ReverbEngine::dampingCutoffHz()` with the engine, so the curve cannot
drift away from what the tank is doing. `TailDisplay` runs its time axis as a square root:
pre-delay and early reflections are tens of milliseconds against seconds of tail, and on a linear
axis they collapse into the first few pixels.

Each modulation mode owns a hue — blue for Chorus, violet for Vibrato, orange for Phaser — and it
runs through the traces, the playhead, the SPREAD knob and the selected MODE button, so the
selector and the picture are visibly the same thing. Colour alone is easy to miss, so each mode
also draws its own signature: Chorus puts both traces over a solid DRY line, Phaser adds notch
markers riding the sweep, and Vibrato drops the dry line entirely, fills the band between the
tail and its resting pitch, and greys MOD MIX out at 100 % because the engine pins it there. A
named badge in the corner says it in words as well.

Switching a panel off dims its controls to 40 % rather than hiding or disabling them, so the
settings stay readable. The title, the hairline and the panel's own selectors fade with it; the
switch itself does not.

While pre-delay or the modulation rate follows the host, the matching knob greys out and its
readout shows what the tempo is producing rather than the now-unused knob value.

### Presets

`source/PresetManager.cpp` lists six read-only factory presets from `source/Presets.cpp`
followed by any user presets found in:

```
~/Library/Application Support/Rozan Audio/Hyperspace/Presets/*.hyperpreset
```

A user preset is the plugin's full parameter state as XML. The header icons create a new one
(prompting for a name), overwrite the current one, and move it to the trash; save and delete are
disabled while a factory preset is selected, and a failure is reported rather than swallowed.
Deleting only ever moves to the trash, never hard-deletes.

Fonts are Futura for the wordmark and Avenir Next Condensed for labels, both macOS system faces;
other platforms fall back.

## Design

`design/hyperspace-layout.png` is the GUI mockup the editor is built from, and
`design/hyperspace-layout.html` is the source it renders from:

```sh
"/Applications/Google Chrome.app/Contents/MacOS/Google Chrome" --headless --disable-gpu \
  --screenshot=design/hyperspace-layout.png --window-size=1040,650 \
  --force-device-scale-factor=2 --hide-scrollbars \
  "file://$PWD/design/hyperspace-layout.html"
```

Theme: Darth Phaser's chassis, type and knobs, with teal as the primary accent in place of amber
and crimson kept as the secondary (Duck and the meter peaks). Panel is 1040x650 rather
than Darth Phaser's 960x600, because the parameter count is roughly double.

Panels: REVERB ENGINE, TONE & DAMPING, MODULATION, PRE AMP, REVERB TAIL (impulse display with
Freeze and Duck) and OUTPUT.

The modulation display carries the mode rather than just the LFO. Each mode owns a hue — blue
for Chorus, violet for Vibrato, orange for Phaser — and the hue runs through the traces, the
playhead, the SPREAD knob and the selected MODE button, so the selector and the picture are
visibly the same thing. On top of the colour each mode draws its own signature, because colour
alone is easy to miss:

| Mode | The display shows |
| --- | --- |
| Chorus | Both LFO traces over a solid **DRY** line: the unmodulated path the tail beats against |
| Vibrato | The band between the tail and its resting pitch filled in, no dry line, and MOD MIX pinned at 100 % and greyed out |
| Phaser | Both traces over the **DRY** line, with notch markers riding the sweep |

A named badge in the corner — `CHORUS · DRY + DETUNED TAIL` — says it in words as well.

The variant renders come from the same file: `?mode=vibrato` and `?mode=phaser` for
`design/hyperspace-layout-vibrato.png` and `-phaser.png`, and `?off` for
`design/hyperspace-layout-off.png`, which shows both switchable panels turned off.

The editor matches the mockup, mode colours included.

## Layout

```
CMakeLists.txt            # build definition, JUCE dependency, plugin metadata
.clangd                   # points clangd at the generated compile database
.clang-format             # formatting rules, enforced tree-wide by CI
.github/workflows/
  ci.yml                  # lint and DSP tests, every commit
  release.yml             # plugin build and packaging, on a v* tag
source/
  ReverbEngine.h/.cpp     # the reverb DSP, free of JUCE
  PreAmp.h/.cpp           # the input stage, also free of JUCE
  Parameters.h/.cpp       # parameter layout and the choice lists
  SyncMath.h              # tempo-sync arithmetic for pre-delay and mod rate
  PluginProcessor.h/.cpp  # AudioProcessor, parameter state, audio callback
  PluginEditor.h/.cpp     # plugin UI
  PresetManager.h/.cpp    # factory and user presets
  Presets.h/.cpp          # the six factory presets
tests/
  DspTests.cpp            # engine and pre amp checks, built as HyperspaceDspTests
```

## Editor setup (clangd / Neovim)

`CMAKE_EXPORT_COMPILE_COMMANDS` is on, so configuring the project writes
`build/compile_commands.json` and symlinks it to the project root. Run the configure step at
least once before opening the project, then restart the language server.

## Plugin metadata

| Field | Value |
| --- | --- |
| Product name | Hyperspace |
| Company | Rozan Audio |
| Bundle ID | com.rozanaudio.hyperspace |
| Manufacturer code | `Rzan` |
| Plugin code | `Hypr` |

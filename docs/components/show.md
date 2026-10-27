# Show module

[日本語](ショーモジュール.md) · [Documentation index](../README.md)

`imkit::show` is an optional target for media servers, VJ tools and show control. Link `imkit::show` (it also pulls in `imkit::editor_core` for timecode) and include `imkit/show.h`; it is not part of `imkit/imkit.h`. Every component draws and reports input only. The host owns values, clocks, devices, files and persistence. The library never reads the system clock or a time-zone database, never opens MIDI/OSC/DMX, and never parses or runs a script.

## Conventions

| Rule | Detail |
|---|---|
| Draw and request | A control draws, then returns what the user asked for (`ValueEdit`, a request struct or an edit buffer). The host applies it. |
| Continuous values | `ValueEdit{started, changed, finished, cancelled, original, value}`. A click is started, changed and finished in one frame; Escape during a drag restores the original and sets `cancelled`. Use it to group Undo steps, thin OSC output or route MIDI Learn. |
| Bounded edits | `RuleEditBuffer` and `StepEditBuffer` are fixed-capacity and report overflow. |
| Time | UTC seconds as `std::int64_t`. `TimeZone` is a callback returning seconds east of UTC, so daylight saving is the host's data. |
| Colours | Derived from the theme's semantic tokens (`MakeShowColors`), so all 13 presets, high contrast and every density work. |
| State | Status is never colour alone: pads, lights and cue rows add a glyph or text. |
| Accessibility | Faders, knobs, XY pads and range handles are `Slider` nodes with a range and Increment/Decrement; meters are `Meter` nodes. |

## Components

| Area | Components |
|---|---|
| Controls (`show/controls.h`) | `Fader` (linear or dB taper, ticks, detent, fine drag, double-click reset, typed entry), `Knob`, `XYPad`, `PadGrid`, `MeterBar` + `PeakHold`, `TimecodeDisplay`, `BindingBadge`, `HoldButton`, `ArmSwitch`, `StatusLed` |
| Live (`show/live.h`) | `CueList` (virtualised), `GoBar`, `ClipLauncher`, `Mixer` with `CrossfadeLaw`, `TransitionBar`, `EffectRack`, `ModulatorEditor` with `ModulatorValue`, `MappingTable`, `MessageMonitor`, `DeviceHealth` |
| Rules (`show/rules.h`) | `RuleBuilder`, `VariableTable`, `RuleTrace`; pure `EvaluateRule`, `FormatRuleSentence`, `ValidateRule`; optional `RuleDocument` |
| Schedule (`show/schedule.h`) | `Calendar` (month, week, day, agenda), `ResourceTimeline`, `RecurrenceEditor`, `CronEditor`, `SolarTriggerEditor`, `DatePicker`, `TimePicker`, `DateTimeField`, `WeekPainter`, `NextRuns`, `ConflictPanel`; pure `NextOccurrences`, `FormatRRule`, `ParseRRule`, `ParseCron`, `CronNext`, `DescribeCron`, `SolarEvents`, `FindConflicts` |
| Beat (`show/beat.h`) | `BeatClock` + `TapTempo`, `Deck`, `ScrollingWaveform`, `OverviewWaveform`, `HotCuePads`, `DjChannelStrip`, `PitchFader`, `StepSequencer`, `Spectrum` |
| Script (`show/script.h`) | `ScriptPastePanel`, `DiagnosticList`, `ParameterPanel`, `PasteImportWizard`, `LogView`; pure `DetectDelimiter`, `ParseDelimited` |

## Examples

```cpp
float gain = -6;
imkit::show::FaderOptions fo;
fo.scale = imkit::show::FaderScale::Decibel;
fo.minimum = -60;
fo.maximum = 6;
const auto edit = imkit::show::Fader("Layer 1", &gain, fo, {&theme, &animation});
if (edit.changed) osc.Send("/layer1/gain", gain);   // Throttle while !edit.finished.
if (edit.finished) undo.Commit();                   // One Undo step per gesture.
```

```cpp
imkit::show::RuleDocument rule;                      // Or keep your own model and apply RuleEdit values.
std::array<imkit::show::RuleEdit, 32> storage{};
imkit::show::RuleEditBuffer edits(storage);
imkit::show::RuleBuilder("rule", rule.View(), context, builderState, edits, {}, options);
rule.ApplyAll(edits.Edits(), context);
const auto outcome = imkit::show::EvaluateRule(rule.View(), context, lookup);   // Dry run; nothing executes.
```

```cpp
const auto r = imkit::show::Calendar("calendar", events, calendarState, {.zone = zone, .now = now}, options);
if (r.move) host.MoveEvent(r.move, r.start);         // Duration is unchanged.
if (r.create) host.CreateEvent(r.start, r.end);
```

```cpp
const auto r = imkit::show::ScriptPastePanel("script", text, sizeof text, analysis, pasteState, {.now = now}, options);
if (r.analyze) worker.Analyse(text);                 // Return diagnostics and detected parameters next frame.
if (r.import) host.Import(text);
```

## Behaviour notes

- **Gallery.** Pages 25-30 show every component against simulated data (cues, a launcher, a mixer, a rule, a schedule with conflicts, decks and a pasted shader). The Gallery is the host there; it reads the clock and applies the requests.
- **Rules.** Empty `All` and `None` groups are true; an empty `Any` group is false. A missing variable counts as false and sets `missingVariable`. Text `Contains` and `StartsWith` ignore ASCII case; `Equals` is exact. A time range whose start is after its end wraps past midnight.
- **Recurrence.** Months without the requested day (the 31st) and non-leap February 29 are skipped. In a spring-forward gap a time moves forward by the gap; in a fall-back overlap the first occurrence is used. Excluded days still count towards `COUNT`.
- **Cron.** When both day-of-month and weekday are restricted, either may match (classic cron).
- **Solar.** `SolarEvents` follows the NOAA/Meeus approximation and is accurate to a few minutes; polar day and night return `valid == false`.
- **Script paste.** The panel uses Dear ImGui's own multi-line editing and IME. It grows to its text and scrolls through its parent, so the caret is not auto-scrolled into view in very long texts. With errors it offers "Import anyway" (a warning, not a block) unless `allowImportWithErrors` is false.
- **Not covered.** Real MIDI/OSC/DMX/NDI devices, a system time-zone database, media playback and audio analysis stay with the host.

## Verification scope

Verified: compile and link, unit tests (`imkit_show_*_test`) for the pure functions, interaction through synthetic input and the semantic tree, and Gallery captures in light and dark. Not verified: real devices, daylight-saving data from an OS, screen readers, touch hardware, macOS, and IME composition in the paste panel.

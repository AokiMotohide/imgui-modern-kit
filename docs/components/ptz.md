# PTZ camera control and range slider

[日本語](PTZ操作と範囲スライダー.md) · [Documentation index](../README.md)

Include `imkit/ptz.h`, `imkit/range_slider.h` or `imkit/imkit.h`. Both components draw and report input only. The host owns every value and any camera; nothing is retained between frames except the host-owned `PtzState`. Existing APIs are unchanged.

## PtzControl

```cpp
imkit::PtzView view;
view.label = "Camera 1";
view.position = {pan, tilt, zoom, /*valid*/ true};   // Reported by the device.
view.presets = presets;                              // span<const PtzPreset>
view.connected = device.connected;
const imkit::PtzRequest r = imkit::PtzControl("cam1", view, state, {}, {&theme, &animation});
if (r.moving) device.Move(r.velocity.x, r.velocity.y, r.zoom);   // -1..1, speed scale applied.
if (r.stop) device.Stop();                                       // One frame after a move ends.
if (r.home) device.Home();
if (r.recallPreset >= 0) device.Recall(r.recallPreset);
if (r.storePreset >= 0) device.Store(r.storePreset);
```

| Part | Behaviour |
|---|---|
| Pad | Eight arrows move at full speed while held. Pressing elsewhere on the disc is an analog joystick with a dead zone (`PtzPadVector`). |
| Home | The centre well returns `home`. |
| Zoom | The rocker zooms in or out while held; an optional bar shows `zoomFraction`. |
| Speed | A fill slider edits `PtzState::speed`; the scale applies to pad and zoom. |
| Presets | Click recalls a stored slot, Shift+click or a 0.6 s hold stores, and an empty slot stores on click. |
| Disconnected | Every control is disabled and a move in progress ends with `stop`. |

Arrows, Home, zoom halves and presets are focusable; hold Space or Enter on a focused arrow to move. Each is an accessibility Button node with a localisable name (`ptz.up`, `ptz.home`, `ptz.zoomIn`, `ptz.preset…` through `LocaleContext`). `PtzOptions::compact` draws the pad, zoom and Home only. The control never talks to a camera and was verified only against the Gallery's simulated camera, not a real device.

## RangeSlider

`RangeSlider` and `RangeSliderInt` edit a low and a high value in place and return true on the frame they change. Drag a handle, drag the filled span to move both, or click the rail to move the nearest handle. Values stay in the range with `low <= high`; `minGap` and `step` are options. A focused handle answers `+` and `-` (Shift = x10); the arrow keys stay with Dear ImGui navigation. Handles are two accessibility Group nodes with numeric ranges and Increment/Decrement actions.

## Fill sliders and combos

`imkit::SliderFloat`, `SliderInt`, `VSliderFloat` and the two-to-four component variants now show the value as a tinted bar inside the frame instead of a handle that overlaps the text; logarithmic sliders keep the native grab. `imkit::BeginCombo` and `imkit::Combo` replace the arrow box with a chevron and shorten long previews with an ellipsis. Input behaviour is Dear ImGui's own, except that `imkit::Combo` does not mark the item as edited, so `IsItemEdited()` is false after a combo change; use the return value.

# Charts

[日本語](チャート.md) · [Documentation index](../README.md)

Include `imkit/charts.h` or `imkit/imkit.h`. The chart components draw values the host already owns. They take borrowed views, retain nothing between frames, and never allocate. No global registry, worker or persistence is introduced, and existing APIs are unchanged.

| Component | Purpose | Returns |
|---|---|---|
| `DonutChart` | Share of a whole, with a centre value and an optional legend | Clicked segment index, or `-1` |
| `BarChart` | Compare a few values; vertical or horizontal | Clicked bar index, or `-1` |
| `Gauge` | One bounded value on a 240° ring, with colour zones | Nothing |
| `Sparkline` | Trend of one series, with a read-out on hover | Nothing |
| `CircularProgress` | Progress ring (see [workflow components](workflow-components.md)); shares the donut geometry | Nothing |

```cpp
const std::array<imkit::ChartDatum, 3> usage{{{"Render", 42}, {"Cache", 24}, {"Other", 6}}};
imkit::DonutChartView view;
view.segments = usage;
view.value = "72 GB";   // Borrowed for the frame.
view.caption = "Used";
const int clicked = imkit::DonutChart("storage", view, {}, {&theme, &animation});
if (clicked >= 0) { /* The host decides what a click means. */ }
```

## Design rules

- **Colour is never the only cue.** Segments and bars carry labels; the legend and tooltip repeat them. `ChartColor(theme, index)` returns a colour-blind-safe sequence whose first slot is the theme accent. Set `ChartDatum::color` (alpha above zero) to override it.
- **Few categories.** Donuts read best with up to five segments. Merge the tail into an "Other" segment in the host.
- **Honest scales.** Bars start at zero. `BarChartOptions::maximum` fixes the axis; zero derives a rounded maximum from the data.
- **Unavailable is not zero.** `Gauge` and `CircularProgress` render a negative or non-finite fraction as an unavailable ring with a dash.
- **Motion respects the theme.** Value changes tween when `ComponentOptions::animation` is set and `Theme::motion` allows it. Reduced motion draws the final state immediately.

## Accessibility and limits

Each chart publishes one semantic node (`Group`, or `Progress` for `Gauge`) when `ComponentOptions::accessibility` is set. Per-segment navigation is not published; provide the underlying numbers in a table or text when users need exact values. Negative and non-finite values are ignored. `Sparkline` draws up to 256 points. Chart text uses the host font and is not shaped for right-to-left layouts.

Gallery page **Charts** shows every component with animated sample data. It is a catalog capture, not proof of screen-reader behaviour or native OS integration.

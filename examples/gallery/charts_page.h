#pragma once
#include <imkit/charts.h>
#include <array>

namespace imkit::gallery {
struct GalleryState;
// Gallery-only data for the Charts page. The chart components never retain these.
struct ChartsPage {
    float progress = .62f, gauge = .72f;
    int donutSelected = -1, barSelected = -1, period = 1;
    bool livePlay = true;
    std::array<float, 24> series{};
    float clock = 0;
    bool initialized = false;
    void Show(GalleryState& host);
};
}

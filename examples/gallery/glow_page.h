#pragma once
#include <imkit/decor.h>
#include <imkit/shell.h>
namespace imkit::gallery {
struct GalleryState;
// Glow frame and external control indicator. The preview frames a mock application; the indicator frames the
// Gallery window itself, driven by hand or by a scripted remote session.
struct GlowPage {
    // Glow frame preview.
    GlowFrameStyle style;
    GlowFrameState preview;
    int preset = 0;
    bool active = true, animate = true, initialized = false;
    // External control indicator.
    ExternalControlStyle remote;
    ExternalControlState remoteState;
    int phase = 0, editPhase = 2, pillPosition = 2;
    bool simulate = false;
    double simulateStart = -1;
    int simulateStep = -1;
    int stops = 0;
    char client[64] = "automation-client";
    char activity[96] = "Moving the camera to preset 3";
    void Show(GalleryState& host);
    // Applies one of the named looks to the preview style.
    void ApplyPreset(int index, const Theme& theme);
};
}

#pragma once
#include <imkit/ptz.h>
#include <array>

namespace imkit::gallery {
struct GalleryState;
// Gallery-only simulation of a PTZ camera. The control never retains or drives this; the page applies the
// returned requests to a fake camera and feeds its position back, exactly as a host would with a device.
struct PtzPage {
    struct Pose {
        float pan = 0, tilt = 0, zoom = 1;
    };
    PtzState state, compactState, offlineState;
    Pose pose{-18, 6, 1.4f}, target{};
    bool moveToTarget = false;
    std::array<Pose, 8> stored{};
    std::array<bool, 8> storedSlot{true, true, true, false, false, false, false, false};
    std::array<PtzPreset, 8> presets{};
    bool connected = true, japanese = false, initialized = false;
    int stopCount = 0, storeCount = 0, recallCount = 0, homeCount = 0;
    PtzRequest last{};
    void Show(GalleryState& host);
};
} // namespace imkit::gallery

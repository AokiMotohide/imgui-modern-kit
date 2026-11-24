#pragma once
#include <imkit/video.h>
#include <cstdint>

namespace imkit::video::impl {
Tick AvailableTicks(std::uint64_t sourceTicks, double speed);
} // namespace imkit::video::impl

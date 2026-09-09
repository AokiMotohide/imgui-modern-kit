#pragma once
#include <cstddef>
namespace imkit::gallery {
void CountAllocations(bool enabled);
std::size_t AllocationCount();
} // namespace imkit::gallery

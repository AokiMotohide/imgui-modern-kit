#include "allocation_probe.h"
#include <cstdlib>
#include <new>
#include <malloc.h>
namespace {
thread_local bool counting = false;
thread_local std::size_t allocations = 0;
} // namespace
namespace imkit::gallery {
void CountAllocations(bool enabled) {
    counting = enabled;
    if (enabled)
        allocations = 0;
}
std::size_t AllocationCount() {
    return allocations;
}
} // namespace imkit::gallery
void *operator new(std::size_t size) {
    if (counting)
        ++allocations;
    if (auto *p = std::malloc(size ? size : 1))
        return p;
    throw std::bad_alloc();
}
void *operator new[](std::size_t size) {
    return ::operator new(size);
}
void operator delete(void *p) noexcept {
    std::free(p);
}
void operator delete[](void *p) noexcept {
    std::free(p);
}
void operator delete(void *p, std::size_t) noexcept {
    std::free(p);
}
void operator delete[](void *p, std::size_t) noexcept {
    std::free(p);
}
void *operator new(std::size_t size, std::align_val_t alignment) {
    if (counting)
        ++allocations;
    if (auto *p = _aligned_malloc(size ? size : 1, static_cast<std::size_t>(alignment)))
        return p;
    throw std::bad_alloc();
}
void *operator new[](std::size_t size, std::align_val_t alignment) {
    return ::operator new(size, alignment);
}
void operator delete(void *p, std::align_val_t) noexcept {
    _aligned_free(p);
}
void operator delete[](void *p, std::align_val_t) noexcept {
    _aligned_free(p);
}
void operator delete(void *p, std::size_t, std::align_val_t) noexcept {
    _aligned_free(p);
}
void operator delete[](void *p, std::size_t, std::align_val_t) noexcept {
    _aligned_free(p);
}

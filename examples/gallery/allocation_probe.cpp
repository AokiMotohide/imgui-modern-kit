#include "allocation_probe.h"
#include <cstdlib>
#include <new>
#if defined(_WIN32)
#include <malloc.h>
#endif
namespace {
thread_local bool counting = false;
thread_local std::size_t allocations = 0;

void *AllocateAligned(std::size_t size, std::size_t alignment) {
#if defined(_WIN32)
    return _aligned_malloc(size ? size : 1, alignment);
#else
    void *memory = nullptr;
    if (posix_memalign(&memory, alignment, size ? size : 1) != 0)
        return nullptr;
    return memory;
#endif
}

void FreeAligned(void *memory) noexcept {
#if defined(_WIN32)
    _aligned_free(memory);
#else
    std::free(memory);
#endif
}
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
    if (auto *p = AllocateAligned(size, static_cast<std::size_t>(alignment)))
        return p;
    throw std::bad_alloc();
}
void *operator new[](std::size_t size, std::align_val_t alignment) {
    return ::operator new(size, alignment);
}
void operator delete(void *p, std::align_val_t) noexcept {
    FreeAligned(p);
}
void operator delete[](void *p, std::align_val_t) noexcept {
    FreeAligned(p);
}
void operator delete(void *p, std::size_t, std::align_val_t) noexcept {
    FreeAligned(p);
}
void operator delete[](void *p, std::size_t, std::align_val_t) noexcept {
    FreeAligned(p);
}

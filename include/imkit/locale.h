#pragma once
#include <cstdint>
#include <span>
#include <string_view>

namespace imkit {
enum class TextDirection { LTR, RTL };
// Callbacks return host-owned UTF-8, valid for the current frame. No global locale.
struct LocaleContext {
    std::string_view languageTag="en";
    TextDirection direction=TextDirection::LTR;
    void* user=nullptr;
    const char* (*lookup)(void*, std::string_view key, const char* fallback)=nullptr;
    std::string_view (*formatNumber)(void*, double, std::span<char>)=nullptr;
    std::string_view (*formatDateTime)(void*, std::int64_t unixSeconds, std::span<char>)=nullptr;
    const char* Text(std::string_view key, const char* fallback) const {
        const auto* result=lookup ? lookup(user,key,fallback) : fallback;
        return result ? result : fallback;
    }
    int VisualIndex(int index,int count) const { return direction==TextDirection::RTL ? count-1-index : index; }
};
} // namespace imkit

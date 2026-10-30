#include <imkit/theme.h>
#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <cstdio>
#include <vector>

// Text form of a Theme: `key = value` lines. One field table drives both directions, so a token that can be
// written can always be read back, and adding a token means adding one row here.
namespace imkit {
namespace {
enum class Kind { Color, Float, Bool, Vec2, Scheme, Contrast, Density, Easing };

struct Field {
    std::string_view key;
    Kind kind;
    void *(*access)(Theme &);
    float low = 0, high = 1000; // Float and Vec2 limits; values outside are rejected, not clamped.
};
// FIELD: a value without a numeric range. RANGED: a number (or pair) that must lie in [low, high].
#define IMKIT_FIELD(key, kind, member) \
    Field { key, Kind::kind, [](Theme &t) -> void * { return &t.member; } }
#define IMKIT_RANGED(key, kind, member, low, high) \
    Field { key, Kind::kind, [](Theme &t) -> void * { return &t.member; }, low, high }

const Field kFields[] = {
    IMKIT_FIELD("scheme", Scheme, scheme),
    IMKIT_FIELD("contrast", Contrast, contrast),
    IMKIT_FIELD("density", Density, density),
    IMKIT_FIELD("semantic.canvas", Color, semantic.canvas),
    IMKIT_FIELD("semantic.surface", Color, semantic.surface),
    IMKIT_FIELD("semantic.surface-raised", Color, semantic.surfaceRaised),
    IMKIT_FIELD("semantic.overlay", Color, semantic.overlay),
    IMKIT_FIELD("semantic.text", Color, semantic.text),
    IMKIT_FIELD("semantic.text-secondary", Color, semantic.textSecondary),
    IMKIT_FIELD("semantic.text-disabled", Color, semantic.textDisabled),
    IMKIT_FIELD("semantic.border", Color, semantic.border),
    IMKIT_FIELD("semantic.accent", Color, semantic.accent),
    IMKIT_FIELD("semantic.on-accent", Color, semantic.onAccent),
    IMKIT_FIELD("semantic.success", Color, semantic.success),
    IMKIT_FIELD("semantic.warning", Color, semantic.warning),
    IMKIT_FIELD("semantic.error", Color, semantic.error),
    IMKIT_FIELD("semantic.shadow", Color, semantic.shadow),
    IMKIT_FIELD("semantic.control.rest", Color, semantic.control.rest),
    IMKIT_FIELD("semantic.control.hover", Color, semantic.control.hover),
    IMKIT_FIELD("semantic.control.pressed", Color, semantic.control.pressed),
    IMKIT_FIELD("semantic.control.focused", Color, semantic.control.focused),
    IMKIT_FIELD("semantic.control.selected", Color, semantic.control.selected),
    IMKIT_FIELD("semantic.control.disabled", Color, semantic.control.disabled),
    IMKIT_RANGED("typography.caption", Float, typography.caption, 1, 200),
    IMKIT_RANGED("typography.body", Float, typography.body, 1, 200),
    IMKIT_RANGED("typography.label", Float, typography.label, 1, 200),
    IMKIT_RANGED("typography.heading", Float, typography.heading, 1, 200),
    IMKIT_RANGED("typography.title", Float, typography.title, 1, 200),
    IMKIT_RANGED("typography.monospace", Float, typography.monospace, 1, 200),
    IMKIT_RANGED("radius.subtle", Float, radius.subtle, 0, 1000),
    IMKIT_RANGED("radius.control", Float, radius.control, 0, 1000),
    IMKIT_RANGED("radius.overlay", Float, radius.overlay, 0, 1000),
    IMKIT_RANGED("stroke.border", Float, stroke.border, 0, 32),
    IMKIT_RANGED("stroke.focus", Float, stroke.focus, 0, 32),
    IMKIT_RANGED("elevation.surface", Float, elevation.surface, 0, 128),
    IMKIT_RANGED("elevation.raised", Float, elevation.raised, 0, 128),
    IMKIT_RANGED("elevation.overlay", Float, elevation.overlay, 0, 128),
    IMKIT_RANGED("elevation.offset", Vec2, elevation.offset, -64, 64),
    IMKIT_FIELD("elevation.hard", Bool, elevation.hard),
    IMKIT_RANGED("opacity.disabled", Float, opacity.disabled, 0, 1),
    IMKIT_RANGED("opacity.scrim", Float, opacity.scrim, 0, 1),
    IMKIT_RANGED("motion.control-seconds", Float, motion.controlSeconds, 0, 10),
    IMKIT_RANGED("motion.overlay-seconds", Float, motion.overlaySeconds, 0, 10),
    IMKIT_FIELD("motion.enabled", Bool, motion.enabled),
    IMKIT_FIELD("motion.reduced", Bool, motion.reducedMotion),
    IMKIT_FIELD("motion.easing", Easing, motion.easing),
};
#undef IMKIT_FIELD
#undef IMKIT_RANGED

template <class E> struct Name { std::string_view text; E value; };
constexpr Name<ColorScheme> kSchemes[] = {{"light", ColorScheme::Light}, {"dark", ColorScheme::Dark}};
constexpr Name<ContrastMode> kContrasts[] = {{"standard", ContrastMode::Standard}, {"high-contrast", ContrastMode::HighContrast}};
constexpr Name<Density> kDensities[] = {{"compact", Density::Compact}, {"comfortable", Density::Comfortable}, {"touch", Density::Touch}};
constexpr Name<Easing> kEasings[] = {{"linear", Easing::Linear}, {"ease-out", Easing::EaseOut}, {"ease-in-out", Easing::EaseInOut}};
constexpr Name<CornerStyle> kCorners[] = {{"sharp", CornerStyle::Sharp}, {"soft", CornerStyle::Soft},
                                          {"round", CornerStyle::Round}, {"pill", CornerStyle::Pill}};

template <class E, std::size_t N> bool ParseName(const Name<E> (&names)[N], std::string_view text, E &out) {
    for (const auto &n : names)
        if (n.text == text) {
            out = n.value;
            return true;
        }
    return false;
}
template <class E, std::size_t N> std::string_view NameOf(const Name<E> (&names)[N], E value) {
    for (const auto &n : names)
        if (n.value == value)
            return n.text;
    return names[0].text;
}

std::string_view Trim(std::string_view s) {
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t' || s.front() == '\r'))
        s.remove_prefix(1);
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r'))
        s.remove_suffix(1);
    return s;
}
bool ParseFloat(std::string_view text, float low, float high, float &out) {
    text = Trim(text);
    float value = 0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (error != std::errc{} || end != text.data() + text.size() || !std::isfinite(value) || value < low || value > high)
        return false;
    out = value;
    return true;
}
bool ParseColor(std::string_view text, ImVec4 &out) {
    if ((text.size() != 7 && text.size() != 9) || text[0] != '#')
        return false;
    unsigned value = 0;
    for (const char c : text.substr(1)) {
        const int digit = c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1;
        if (digit < 0)
            return false;
        value = value << 4 | static_cast<unsigned>(digit);
    }
    if (text.size() == 7)
        value = value << 8 | 0xffu;
    out = {static_cast<float>(value >> 24 & 255) / 255.f, static_cast<float>(value >> 16 & 255) / 255.f,
           static_cast<float>(value >> 8 & 255) / 255.f, static_cast<float>(value & 255) / 255.f};
    return true;
}
bool ParseBool(std::string_view text, bool &out) {
    if (text == "true" || text == "on" || text == "yes" || text == "1") {
        out = true;
        return true;
    }
    if (text == "false" || text == "off" || text == "no" || text == "0") {
        out = false;
        return true;
    }
    return false;
}
bool ParseVec2(std::string_view text, float low, float high, ImVec2 &out) {
    const auto split = text.find_first_of(" \t,");
    if (split == std::string_view::npos)
        return false;
    float x = 0, y = 0;
    auto rest = Trim(text.substr(split + 1));
    if (!rest.empty() && rest.front() == ',')
        rest = Trim(rest.substr(1));
    if (!ParseFloat(text.substr(0, split), low, high, x) || !ParseFloat(rest, low, high, y))
        return false;
    out = {x, y};
    return true;
}
bool ParseValue(const Field &field, Theme &theme, std::string_view value) {
    void *target = field.access(theme);
    switch (field.kind) {
    case Kind::Color: return ParseColor(value, *static_cast<ImVec4 *>(target));
    case Kind::Float: return ParseFloat(value, field.low, field.high, *static_cast<float *>(target));
    case Kind::Bool: return ParseBool(value, *static_cast<bool *>(target));
    case Kind::Vec2: return ParseVec2(value, field.low, field.high, *static_cast<ImVec2 *>(target));
    case Kind::Scheme: return ParseName(kSchemes, value, *static_cast<ColorScheme *>(target));
    case Kind::Contrast: return ParseName(kContrasts, value, *static_cast<ContrastMode *>(target));
    case Kind::Density: return ParseName(kDensities, value, *static_cast<Density *>(target));
    case Kind::Easing: return ParseName(kEasings, value, *static_cast<Easing *>(target));
    }
    return false;
}

void AppendFloat(std::string &out, float value) {
    char buffer[32];
    const auto [end, error] = std::to_chars(buffer, buffer + sizeof buffer, value);
    out.append(buffer, error == std::errc{} ? end : buffer);
}
void AppendColor(std::string &out, ImVec4 color) {
    const auto byte = [](float v) { return static_cast<unsigned>(std::lround(std::clamp(v, 0.f, 1.f) * 255.f)); };
    char buffer[16];
    const unsigned alpha = byte(color.w);
    if (alpha == 255)
        std::snprintf(buffer, sizeof buffer, "#%02x%02x%02x", byte(color.x), byte(color.y), byte(color.z));
    else
        std::snprintf(buffer, sizeof buffer, "#%02x%02x%02x%02x", byte(color.x), byte(color.y), byte(color.z), alpha);
    out += buffer;
}
void AppendValue(std::string &out, const Field &field, Theme &theme) {
    void *source = field.access(theme);
    switch (field.kind) {
    case Kind::Color: AppendColor(out, *static_cast<ImVec4 *>(source)); break;
    case Kind::Float: AppendFloat(out, *static_cast<float *>(source)); break;
    case Kind::Bool: out += *static_cast<bool *>(source) ? "true" : "false"; break;
    case Kind::Vec2:
        AppendFloat(out, static_cast<ImVec2 *>(source)->x);
        out += ' ';
        AppendFloat(out, static_cast<ImVec2 *>(source)->y);
        break;
    case Kind::Scheme: out += NameOf(kSchemes, *static_cast<ColorScheme *>(source)); break;
    case Kind::Contrast: out += NameOf(kContrasts, *static_cast<ContrastMode *>(source)); break;
    case Kind::Density: out += NameOf(kDensities, *static_cast<Density *>(source)); break;
    case Kind::Easing: out += NameOf(kEasings, *static_cast<Easing *>(source)); break;
    }
}

constexpr int kMaxWarnings = 16;
void Warn(ThemeParseResult &result, int line, std::string_view message) {
    ++result.ignored;
    if (result.ignored <= kMaxWarnings) {
        if (line > 0)
            result.warnings += "line " + std::to_string(line) + ": ";
        result.warnings += message;
        result.warnings += '\n';
    } else if (result.ignored == kMaxWarnings + 1) {
        result.warnings += "(further warnings omitted)\n";
    }
}

struct Entry {
    int line;
    std::string_view key, value;
};
} // namespace

std::string SerializeTheme(const Theme &theme) {
    std::string out = "# ImKit theme. Keys and values: docs/architecture/make-your-own-look.md\n";
    Theme copy = theme; // The field table reads through mutable accessors.
    for (const auto &field : kFields) {
        out += field.key;
        out += " = ";
        AppendValue(out, field, copy);
        out += '\n';
    }
    return out;
}

ThemeParseResult ParseTheme(std::string_view text, Theme &theme) {
    ThemeParseResult result;
    if (text.starts_with("\xEF\xBB\xBF")) // Editors on Windows may add a byte order mark.
        text.remove_prefix(3);
    std::vector<Entry> entries;
    for (int number = 1; !text.empty(); ++number) {
        const auto end = text.find('\n');
        const auto line = Trim(text.substr(0, end));
        text = end == std::string_view::npos ? std::string_view{} : text.substr(end + 1);
        if (line.empty() || line.front() == '#')
            continue;
        const auto equals = line.find('=');
        if (equals == std::string_view::npos) {
            Warn(result, number, "expected `key = value`");
            continue;
        }
        entries.push_back({number, Trim(line.substr(0, equals)), Trim(line.substr(equals + 1))});
    }
    // Step 1: the preset replaces the base. Fonts and the skin belong to the host and survive it.
    for (const auto &e : entries) {
        if (e.key != "preset")
            continue;
        const auto preset = ThemePresetFromId(e.value);
        if (!preset) {
            Warn(result, e.line, "unknown preset `" + std::string(e.value) + "`");
            continue;
        }
        Theme next = MakeTheme(*preset);
        next.fonts = theme.fonts;
        theme = next;
        ++result.applied;
    }
    // Step 2: brand colours rebuild the colour tokens only; shape, type and motion stay as they are.
    BrandPalette brand;
    bool required[4] = {};
    int brandLines = 0;
    struct BrandKey { std::string_view key; ImVec4 BrandPalette::*member; int required; };
    static const BrandKey brandKeys[] = {
        {"brand.canvas", &BrandPalette::canvas, 0}, {"brand.surface", &BrandPalette::surface, 1},
        {"brand.text", &BrandPalette::text, 2},     {"brand.accent", &BrandPalette::accent, 3},
        {"brand.raised", &BrandPalette::raised, -1}, {"brand.border", &BrandPalette::border, -1},
        {"brand.text-secondary", &BrandPalette::textSecondary, -1}, {"brand.success", &BrandPalette::success, -1},
        {"brand.warning", &BrandPalette::warning, -1}, {"brand.error", &BrandPalette::error, -1}};
    for (const auto &e : entries)
        for (const auto &b : brandKeys) {
            if (e.key != b.key)
                continue;
            if (!ParseColor(e.value, brand.*b.member)) {
                Warn(result, e.line, "invalid colour `" + std::string(e.value) + "`, expected #RRGGBB or #RRGGBBAA");
                continue;
            }
            ++brandLines;
            if (b.required >= 0)
                required[b.required] = true;
        }
    if (brandLines > 0) {
        if (required[0] && required[1] && required[2] && required[3]) {
            result.applied += brandLines;
            const Theme derived = MakeBrandTheme(brand);
            const ImVec4 shadow = theme.semantic.shadow;
            theme.scheme = derived.scheme;
            theme.semantic = derived.semantic;
            theme.semantic.shadow = shadow;
            theme.colors = derived.colors;
            theme.editor = derived.editor;
        } else {
            Warn(result, 0, "brand.* needs brand.canvas, brand.surface, brand.text and brand.accent; the brand colours were not applied");
        }
    }
    // Step 3: every other key, top to bottom.
    for (const auto &e : entries) {
        if (e.key == "preset" || e.key.starts_with("brand."))
            continue;
        if (e.key == "corner") {
            CornerStyle style{};
            if (ParseName(kCorners, e.value, style)) {
                SetCornerStyle(theme, style);
                ++result.applied;
            } else {
                Warn(result, e.line, "unknown corner style `" + std::string(e.value) + "`, expected sharp, soft, round or pill");
            }
            continue;
        }
        const auto field = std::find_if(std::begin(kFields), std::end(kFields), [&](const Field &f) { return f.key == e.key; });
        if (field == std::end(kFields)) {
            Warn(result, e.line, "unknown key `" + std::string(e.key) + "`");
            continue;
        }
        if (ParseValue(*field, theme, e.value))
            ++result.applied;
        else
            Warn(result, e.line, "invalid value `" + std::string(e.value) + "` for `" + std::string(e.key) + "`");
    }
    ResolveTheme(theme);
    return result;
}
} // namespace imkit

#include <imkit/show/script.h>
#include "internal.h"
#include <imkit/components.h>
#include <imkit/patterns.h>
#include <imkit/widgets.h>
#include <imkit/workflow.h>
#include <algorithm>
#include <cctype>
#include <cfloat>
#include <cstring>

namespace imkit::show {
using namespace impl;

namespace {
ImVec4 SeverityTone(const ShowColors& c, Severity s) { return s == Severity::Error ? c.error : s == Severity::Warning ? c.warning : c.accent; }
void SeverityGlyph(ImDrawList* d, const ShowColors& c, Severity s, ImVec2 at, float size) {
    const ImU32 col = U32(SeverityTone(c, s));
    if (s == Severity::Error) {
        d->AddCircleFilled(at, size * .45f, col, 14);
        detail::DrawCross(d, at, size * .42f, U32(c.surface), 1.6f);
    } else if (s == Severity::Warning) {
        GlyphWarning(d, at, size, col);
    } else {
        d->AddCircle(at, size * .4f, col, 14, 1.6f);
        d->AddLine({at.x, at.y - size * .08f}, {at.x, at.y + size * .2f}, col, 1.6f);
    }
}
const char* Label(const ParamDecl& d) { return d.label && d.label[0] ? d.label : d.name; }
} // namespace

// ---- ParameterPanel --------------------------------------------------------------------------------------------
ParamEdit ParameterPanel(const char* id, std::span<const ParamDecl> decls, std::span<ParamValue> values, ComponentOptions options) {
    ParamEdit out;
    const ShowColors c = MakeShowColors(options);
    const float u = Unit();
    const float labelW = u * 7.f;
    ImGui::PushID(id);
    for (int i = 0; i < static_cast<int>(decls.size()) && i < static_cast<int>(values.size()); ++i) {
        const ParamDecl& d = decls[static_cast<std::size_t>(i)];
        ParamValue& v = values[static_cast<std::size_t>(i)];
        ImGui::PushID(i);
        const char* label = Label(d);
        if (d.type != ParamType::Bool && d.type != ParamType::Event) {
            ImGui::AlignTextToFramePadding();
            ImGui::TextColored(c.muted, "%s", label);
            ImGui::SameLine(labelW);
        }
        switch (d.type) {
        case ParamType::Float:
        case ParamType::Int: {
            FaderOptions fo;
            fo.vertical = false;
            fo.minimum = d.minimum;
            fo.maximum = d.maximum > d.minimum ? d.maximum : d.minimum + 1;
            fo.defaultValue = d.defaultValue;
            fo.format = d.type == ParamType::Int ? "%.0f" : "%.2f";
            fo.step = d.type == ParamType::Int ? 1.f : 0.f;
            fo.size = {std::max(u * 8.f, ImGui::GetContentRegionAvail().x - u), u * 1.3f};
            const ValueEdit e = Fader(label, &v.v[0], fo, options);
            if (e) {
                out.index = i;
                out.edit = e;
            }
            break;
        }
        case ParamType::Bool: {
            bool on = v.v[0] > .5f;
            if (Toggle(label, &on, options)) {
                const float before = v.v[0];
                v.v[0] = on ? 1.f : 0.f;
                out.index = i;
                out.edit = {true, true, true, false, before, v.v[0]};
            }
            break;
        }
        case ParamType::Color: {
            if (ImGui::ColorEdit4("##color", v.v, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar)) {
                out.index = i;
                out.edit = {true, true, true, false, 0, v.v[0]};
            }
            break;
        }
        case ParamType::Point2D: {
            XYPadOptions xo;
            xo.size = {u * 6.f, u * 6.f};
            xo.minimumX = xo.minimumY = d.minimum;
            xo.maximumX = xo.maximumY = d.maximum > d.minimum ? d.maximum : d.minimum + 1;
            xo.defaultX = xo.defaultY = d.defaultValue;
            const ValueEdit e = XYPad(label, &v.v[0], &v.v[1], xo, options);
            if (e) {
                out.index = i;
                out.edit = e;
            }
            break;
        }
        case ParamType::Choice: {
            int index = std::clamp(static_cast<int>(v.v[0]), 0, std::max(0, static_cast<int>(d.choices.size()) - 1));
            ImGui::SetNextItemWidth(u * 9.f);
            if (!d.choices.empty() && Combo("##choice", &index, d.choices.data(), static_cast<int>(d.choices.size()))) {
                const float before = v.v[0];
                v.v[0] = static_cast<float>(index);
                out.index = i;
                out.edit = {true, true, true, false, before, v.v[0]};
            }
            break;
        }
        default:
            if (ActionButton(label, ActionVariant::Secondary, {}, options)) {
                out.index = i;
                out.triggered = true;
            }
            break;
        }
        ImGui::PopID();
    }
    ImGui::PopID();
    return out;
}

// ---- Diagnostics -----------------------------------------------------------------------------------------------
DiagnosticListRequest DiagnosticList(const char* id, std::span<const Diagnostic> diagnostics, ImVec2 size, ComponentOptions options) {
    DiagnosticListRequest request;
    const ShowColors c = MakeShowColors(options);
    auto* outer = ImGui::GetWindowDrawList();
    (void)outer;
    const float u = Unit();
    ImGui::PushID(id);
    ImGuiStorage* st = ImGui::GetStateStorage();
    const ImGuiID filterKey = ImGui::GetID("##filter");
    int mask = st->GetInt(filterKey, 7);
    int counts[3] = {0, 0, 0};
    for (const Diagnostic& d : diagnostics)
        ++counts[static_cast<int>(d.severity)];
    const char* names[3] = {"Info", "Warnings", "Errors"};
    const int order[3] = {2, 1, 0};
    for (int k : order) {
        const bool on = mask & (1 << k);
        if (FilterChip(FrameText("sev%d", k), FrameText("%s %d", names[k], counts[k]), on, options))
            mask ^= 1 << k;
        ImGui::SameLine();
    }
    ImGui::NewLine();
    st->SetInt(filterKey, mask);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, c.surface);
    if (size.y <= 0)
        size.y = u * 1.5f * static_cast<float>(std::clamp<int>(static_cast<int>(diagnostics.size()), 2, 7)) + 6.f;
    if (ImGui::BeginChild("##diagnostics", size, ImGuiChildFlags_Borders)) {
        auto* draw = ImGui::GetWindowDrawList();
        bool any = false;
        for (int i = 0; i < static_cast<int>(diagnostics.size()); ++i) {
            const Diagnostic& d = diagnostics[static_cast<std::size_t>(i)];
            if (!(mask & (1 << static_cast<int>(d.severity))))
                continue;
            any = true;
            ImGui::PushID(i);
            const ImVec2 a = ImGui::GetCursorScreenPos();
            const float rowH = u * 1.4f, w = ImGui::GetContentRegionAvail().x;
            ImGui::SetNextItemAllowOverlap(); // The suggestion chip sits on top of the row.
            ImGui::Selectable("##row", false, 0, {w, rowH});
            if (ImGui::IsItemClicked())
                request.jumpToLine = d.line;
            SemanticItem(options, FrameText("Line %d: %s", d.line, Safe(d.message)), accessibility::SemanticRole::Row, accessibility::SemanticAction::Select);
            SeverityGlyph(draw, c, d.severity, {a.x + u * .6f, a.y + rowH * .5f}, u * .7f);
            const char* where = d.column > 0 ? FrameText("%d:%d", d.line, d.column) : FrameText("%d", d.line);
            draw->AddText({a.x + u * 1.2f, a.y + (rowH - ImGui::GetTextLineHeight()) * .5f}, U32(c.muted), where);
            draw->AddText({a.x + u * 3.8f, a.y + (rowH - ImGui::GetTextLineHeight()) * .5f}, U32(c.text), Safe(d.message));
            if (d.suggestion && d.suggestion[0]) {
                const ImVec2 ts = ImGui::CalcTextSize(d.suggestion);
                const ImVec2 sa{a.x + w - ts.x - u * 1.2f, a.y + u * .18f}, sb{a.x + w - u * .3f, a.y + rowH - u * .18f};
                ImGui::SetCursorScreenPos(sa);
                ImGui::InvisibleButton("##fix", {sb.x - sa.x, sb.y - sa.y});
                if (ImGui::IsItemClicked())
                    request.applySuggestion = i;
                SemanticItem(options, FrameText("Apply: %s", d.suggestion), accessibility::SemanticRole::Button, accessibility::SemanticAction::Press);
                draw->AddRectFilled(sa, sb, U32(Mix(c.input, c.accent, ImGui::IsItemHovered() ? .35f : .2f)), (sb.y - sa.y) * .5f);
                draw->AddText({sa.x + u * .4f, sa.y + (sb.y - sa.y - ImGui::GetTextLineHeight()) * .5f}, U32(c.accent), d.suggestion);
                ImGui::SetCursorScreenPos({a.x, a.y + rowH});
            }
            ImGui::PopID();
        }
        if (!any)
            ImGui::TextColored(c.muted, "%s", diagnostics.empty() ? Str(options, "show.noDiagnostics", "No problems found.") : Str(options, "show.filteredOut", "All problems are hidden by the filter."));
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::PopID();
    return request;
}

// ---- Paste panel -----------------------------------------------------------------------------------------------
ScriptPasteRequest ScriptPastePanel(const char* id, char* text, std::size_t capacity, const ScriptAnalysis& analysis, ScriptPasteState& state,
                                    ScriptPasteOptions o, ComponentOptions options) {
    ScriptPasteRequest request;
    if (!text || capacity < 2)
        return request;
    const ShowColors c = MakeShowColors(options);
    const float u = Unit();
    const auto& style = ImGui::GetStyle();
    ImGui::PushID(id);
    ImVec2 size = o.size;
    if (size.x <= 0)
        size.x = std::max(u * 16.f, ImGui::GetContentRegionAvail().x);
    if (size.y <= 0)
        size.y = u * 14.f;
    const float statusH = u * 2.2f;
    const float gutterW = u * 3.2f;
    const float lineH = ImGui::GetTextLineHeight();

    // Line starts for the gutter and the diagnostic underlines. The vector is reused between frames.
    static thread_local std::vector<const char*> lineStart;
    lineStart.clear();
    lineStart.push_back(text);
    for (const char* p = text; *p; ++p)
        if (*p == '\n')
            lineStart.push_back(p + 1);
    const int lines = static_cast<int>(lineStart.size());
    const float inputH = std::max(size.y - statusH - style.WindowPadding.y * 2.f, static_cast<float>(lines) * lineH + style.FramePadding.y * 2.f + style.ScrollbarSize + 2.f);

    ImGui::PushStyleColor(ImGuiCol_ChildBg, c.surface);
    const bool open = ImGui::BeginChild("##editor", {size.x, size.y - statusH}, ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding);
    if (open) {
        auto* draw = ImGui::GetWindowDrawList();
        const ImVec2 origin = ImGui::GetCursorScreenPos();
        ImGui::SetCursorScreenPos({origin.x + gutterW, origin.y});
        ImGui::PushStyleColor(ImGuiCol_FrameBg, {0, 0, 0, 0});
        const bool edited = ImGui::InputTextMultiline("##text", text, capacity, {size.x - gutterW - style.WindowPadding.x * 2.f - style.ScrollbarSize, inputH}, ImGuiInputTextFlags_AllowTabInput);
        ImGui::PopStyleColor();
        const ImVec2 inputMin = ImGui::GetItemRectMin();
        const bool inputFocused = ImGui::IsItemActive();
        const int length = static_cast<int>(std::strlen(text));
        if (edited) {
            request.textChanged = true;
            ++state.revision;
            state.lastEdit = o.now;
            state.pending = true;
            if (length - state.lastLength >= 16 || (state.lastLength == 0 && length >= 16))
                request.pasted = true;
        }
        state.lastLength = length;
        const float textTop = inputMin.y + style.FramePadding.y, textLeft = inputMin.x + style.FramePadding.x;
        // Gutter: line numbers, with marks and tinted rows for diagnostics.
        for (int i = 0; i < lines; ++i) {
            const float y = textTop + static_cast<float>(i) * lineH;
            if (!ImGui::IsRectVisible({origin.x, y}, {origin.x + 10.f, y + lineH}))
                continue;
            const char* number = FrameText("%d", i + 1);
            draw->AddText({origin.x + gutterW - u * .6f - ImGui::CalcTextSize(number).x, y}, U32(c.muted, .8f), number);
        }
        draw->AddLine({origin.x + gutterW - u * .3f, origin.y}, {origin.x + gutterW - u * .3f, origin.y + inputH}, U32(c.border, .7f), 1.f);
        for (const Diagnostic& d : analysis.diagnostics) {
            if (d.line < 1 || d.line > lines)
                continue;
            const float y = textTop + static_cast<float>(d.line - 1) * lineH;
            const ImVec4 tone = SeverityTone(c, d.severity);
            draw->AddRectFilled({origin.x + gutterW - u * .3f, y}, {origin.x + size.x - style.WindowPadding.x * 2.f - style.ScrollbarSize, y + lineH}, U32(tone, .1f));
            SeverityGlyph(draw, c, d.severity, {origin.x + u * .45f, y + lineH * .5f}, u * .62f);
            // Wavy underline under the offending range.
            const char* start = lineStart[static_cast<std::size_t>(d.line - 1)];
            const char* lineEnd = d.line < lines ? lineStart[static_cast<std::size_t>(d.line)] - 1 : start + std::strlen(start);
            const int column = std::max(1, d.column);
            if (start + column - 1 <= lineEnd) {
                const char* from = start + column - 1;
                const char* to = d.length > 0 ? std::min(lineEnd, from + d.length) : from;
                if (d.length <= 0)
                    while (to < lineEnd && !std::isspace(static_cast<unsigned char>(*to)))
                        ++to;
                const float x0 = textLeft + ImGui::CalcTextSize(start, from).x, x1 = textLeft + ImGui::CalcTextSize(start, to).x;
                const float base = y + lineH - 2.f;
                for (float x = x0; x < std::max(x1, x0 + 6.f); x += 3.f)
                    draw->AddLine({x, base + (static_cast<int>((x - x0) / 3.f) % 2 ? 1.5f : -.5f)}, {x + 3.f, base + (static_cast<int>((x - x0) / 3.f) % 2 ? -.5f : 1.5f)}, U32(tone), 1.4f);
            }
            // Gutter tooltip and click-to-jump.
            const ImVec2 mouse = ImGui::GetIO().MousePos;
            if (ImGui::IsWindowHovered() && mouse.x >= origin.x && mouse.x <= origin.x + gutterW - u * .3f && mouse.y >= y && mouse.y < y + lineH) {
                ImGui::SetTooltip("%s", Safe(d.message));
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                    request.jumpToLine = d.line;
            }
        }
        if (length == 0 && !inputFocused && o.placeholder && o.placeholder[0])
            draw->AddText({textLeft, textTop}, U32(c.muted, .7f), o.placeholder);
        AnnotateText(options, id, FrameText("%d lines, %d problems", lines, static_cast<int>(analysis.diagnostics.size())), accessibility::SemanticRole::TextField);
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();

    // Debounced analysis request.
    if (state.pending && (o.now <= 0 || o.now - state.lastEdit >= o.debounceSeconds)) {
        request.analyze = true;
        state.pending = false;
    }

    // Status row: format, summary, problem counts, then the actions on the right.
    int errors = 0, warnings = 0, infos = 0;
    for (const Diagnostic& d : analysis.diagnostics)
        (d.severity == Severity::Error ? errors : d.severity == Severity::Warning ? warnings : infos) += 1;
    auto* draw = ImGui::GetWindowDrawList();
    ImGui::AlignTextToFramePadding();
    if (analysis.format && analysis.format[0]) {
        const ImVec2 ts = ImGui::CalcTextSize(analysis.format);
        const ImVec2 a = ImGui::GetCursorScreenPos(), b{a.x + ts.x + u * .9f, a.y + u * 1.05f};
        ImGui::Dummy({b.x - a.x, u * 1.05f});
        draw->AddRectFilled(a, b, U32(Mix(c.input, c.accent, .2f)), u * .5f);
        draw->AddText({a.x + u * .45f, a.y + (u * 1.05f - ts.y) * .5f}, U32(c.accent), analysis.format);
        ImGui::SameLine();
    } else if (!text[0]) {
        ImGui::TextColored(c.muted, "%s", Str(options, "show.pasteHint", "Paste a script, or drop text here."));
        ImGui::SameLine();
    }
    if (analysis.analyzing) {
        Spinner("analysing", options);
        ImGui::SameLine();
        ImGui::TextColored(c.muted, "%s", Str(options, "show.analysing", "Checking..."));
        ImGui::SameLine();
    } else if (analysis.summary && analysis.summary[0]) {
        ImGui::TextColored(c.text, "%s", analysis.summary);
        ImGui::SameLine();
    }
    const auto count = [&](Severity s, int n, const char* word) {
        if (n == 0)
            return;
        const ImVec2 a = ImGui::GetCursorScreenPos();
        ImGui::Dummy({u * 1.0f, u});
        SeverityGlyph(draw, c, s, {a.x + u * .5f, a.y + u * .5f}, u * .7f);
        ImGui::SameLine(0, 2.f);
        ImGui::TextColored(SeverityTone(c, s), "%d %s", n, word);
        ImGui::SameLine();
    };
    count(Severity::Error, errors, errors == 1 ? "error" : "errors");
    count(Severity::Warning, warnings, warnings == 1 ? "warning" : "warnings");
    count(Severity::Info, infos, "notes");
    const bool empty = !text[0];
    const bool blocked = empty || !analysis.importable || analysis.analyzing || (errors > 0 && !o.allowImportWithErrors);
    const char* importLabel = errors > 0 && o.allowImportWithErrors ? Str(options, "show.importAnyway", "Import anyway") : Str(options, "show.import", o.importLabel);
    const float clearW = ImGui::CalcTextSize(Str(options, "show.clear", "Clear")).x + u * 1.6f;
    const float importW = ImGui::CalcTextSize(importLabel).x + u * 2.f;
    ImGui::SameLine(std::max(ImGui::GetCursorPosX(), size.x - clearW - importW - u * .8f));
    ImGui::BeginDisabled(empty);
    if (ActionButton(Str(options, "show.clear", "Clear"), ActionVariant::Ghost, {0, 0}, options))
        request.clear = true;
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(blocked);
    ComponentOptions importOptions = options;
    importOptions.disabledReason = empty ? "Nothing to import." : !analysis.importable ? "This text was not recognised." : analysis.analyzing ? "Still checking." : "Fix the errors first.";
    if (ActionButton(importLabel, errors > 0 || warnings > 0 ? ActionVariant::Secondary : ActionVariant::Primary, {0, 0}, importOptions))
        request.import = true;
    ImGui::EndDisabled();
    ImGui::PopID();
    return request;
}

// ---- Table paste -----------------------------------------------------------------------------------------------
namespace {
// Counts `delimiter` outside quotes in one line.
int CountOutsideQuotes(std::string_view line, char delimiter) {
    int count = 0;
    bool quoted = false;
    for (char ch : line) {
        if (ch == '"')
            quoted = !quoted;
        else if (ch == delimiter && !quoted)
            ++count;
    }
    return count;
}
std::string Lower(std::string_view s) {
    std::string out(s);
    for (char& ch : out)
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    return out;
}
std::string Trim(std::string s) {
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front())))
        s.erase(s.begin());
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back())))
        s.pop_back();
    return s;
}
} // namespace

char DetectDelimiter(std::string_view text) {
    std::vector<std::string_view> sample;
    while (!text.empty() && sample.size() < 5) {
        const std::size_t end = text.find('\n');
        std::string_view line = text.substr(0, end);
        if (!line.empty() && line.back() == '\r')
            line.remove_suffix(1);
        if (!line.empty())
            sample.push_back(line);
        text = end == std::string_view::npos ? std::string_view{} : text.substr(end + 1);
    }
    if (sample.empty())
        return 0;
    for (char d : {'\t', ',', ';', '|'}) {
        const int first = CountOutsideQuotes(sample[0], d);
        if (first == 0)
            continue;
        bool same = true;
        for (std::string_view line : sample)
            same = same && CountOutsideQuotes(line, d) == first;
        if (same)
            return d;
    }
    return 0;
}

void ParseDelimited(std::string_view text, char delimiter, std::vector<std::vector<std::string>>& rows) {
    rows.clear();
    std::vector<std::string> row;
    std::string cell;
    bool quoted = false, any = false;
    const auto endCell = [&] {
        row.push_back(Trim(cell));
        cell.clear();
    };
    const auto endRow = [&] {
        endCell();
        const bool blank = row.size() == 1 && row[0].empty();
        if (!blank)
            rows.push_back(std::move(row));
        row.clear();
        any = false;
    };
    for (std::size_t i = 0; i < text.size(); ++i) {
        const char ch = text[i];
        if (quoted) {
            if (ch == '"') {
                if (i + 1 < text.size() && text[i + 1] == '"') {
                    cell += '"';
                    ++i;
                } else {
                    quoted = false;
                }
            } else {
                cell += ch;
            }
            continue;
        }
        if (ch == '"' && cell.find_first_not_of(" \t") == std::string::npos) {
            quoted = true;
            any = true;
        } else if (ch == delimiter && delimiter != 0) {
            endCell();
            any = true;
        } else if (ch == '\n') {
            endRow();
        } else if (ch != '\r') {
            cell += ch;
            any = true;
        }
    }
    if (any || !cell.empty() || !row.empty())
        endRow();
}

int ImportState::Columns() const {
    std::size_t n = 0;
    for (const auto& r : rows)
        n = std::max(n, r.size());
    return static_cast<int>(n);
}
int ImportState::DataRows() const { return std::max(0, static_cast<int>(rows.size()) - (header ? 1 : 0)); }
const std::string& ImportState::Cell(int dataRow, int column) const {
    static const std::string empty;
    const std::size_t r = static_cast<std::size_t>(dataRow + (header ? 1 : 0));
    if (dataRow < 0 || r >= rows.size() || column < 0 || static_cast<std::size_t>(column) >= rows[r].size())
        return empty;
    return rows[r][static_cast<std::size_t>(column)];
}
void ImportState::Reparse(std::span<const ImportColumn> targets) {
    const std::string_view text(buffer.data());
    const char d = delimiter ? delimiter : DetectDelimiter(text);
    ParseDelimited(text, d, rows);
    mapping.assign(static_cast<std::size_t>(Columns()), -1);
    if (header && !rows.empty()) {
        std::vector<bool> taken(targets.size(), false);
        for (std::size_t col = 0; col < rows[0].size() && col < mapping.size(); ++col) {
            const std::string h = Lower(rows[0][col]);
            for (std::size_t t = 0; t < targets.size(); ++t) {
                if (taken[t])
                    continue;
                bool match = h == Lower(targets[t].name);
                std::string_view aliases = targets[t].aliases ? targets[t].aliases : "";
                while (!match && !aliases.empty()) {
                    const std::size_t comma = aliases.find(',');
                    match = h == Lower(Trim(std::string(aliases.substr(0, comma))));
                    aliases = comma == std::string_view::npos ? std::string_view{} : aliases.substr(comma + 1);
                }
                if (match) {
                    mapping[col] = static_cast<int>(t);
                    taken[t] = true;
                    break;
                }
            }
        }
    }
    mappedRevision = revision;
}

ImportRequest PasteImportWizard(const char* id, ImportState& state, std::span<const ImportColumn> targets, ComponentOptions options) {
    ImportRequest request;
    const ShowColors c = MakeShowColors(options);
    auto* draw = ImGui::GetWindowDrawList();
    const float u = Unit();
    ImGui::PushID(id);
    bool reparse = state.mappedRevision < 0;
    // 1. Paste.
    ImGui::TextColored(c.muted, "%s", Str(options, "show.pasteTable", "Paste cells from a spreadsheet, or CSV / TSV text."));
    if (ImGui::InputTextMultiline("##paste", state.buffer.data(), state.buffer.size(), {ImGui::GetContentRegionAvail().x, u * 6.f}, ImGuiInputTextFlags_AllowTabInput)) {
        ++state.revision;
        reparse = true;
    }
    // 2. Separator and header.
    const char* seps[] = {"Auto", "Tab", "Comma", "Semicolon", "Pipe"};
    const char values[] = {0, '\t', ',', ';', '|'};
    int sep = 0;
    for (int i = 0; i < 5; ++i)
        if (values[i] == state.delimiter)
            sep = i;
    if (Segmented("separator", &sep, seps, options)) {
        state.delimiter = values[sep];
        reparse = true;
    }
    ImGui::SameLine(0, u);
    if (Toggle(Str(options, "show.firstRowHeader", "First row is a header"), &state.header, options))
        reparse = true;
    if (reparse)
        state.Reparse(targets);

    const int columns = std::min(state.Columns(), 12);
    // 3. Mapping and preview.
    if (columns > 0) {
        ImGui::Spacing();
        if (ImGui::BeginTable("##preview", columns, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY,
                              {0, u * 1.2f * 9.f})) {
            for (int col = 0; col < columns; ++col)
                ImGui::TableSetupColumn(FrameText("col%d", col), ImGuiTableColumnFlags_WidthFixed, u * 9.f);
            ImGui::TableNextRow();
            for (int col = 0; col < columns; ++col) {
                ImGui::TableSetColumnIndex(col);
                ImGui::PushID(col);
                const int mapped = state.mapping[static_cast<std::size_t>(col)];
                const char* preview = mapped >= 0 && mapped < static_cast<int>(targets.size()) ? targets[static_cast<std::size_t>(mapped)].name : Str(options, "show.ignore", "Ignore");
                ImGui::SetNextItemWidth(-FLT_MIN);
                if (ImGui::BeginCombo("##target", preview)) {
                    if (ImGui::Selectable(Str(options, "show.ignore", "Ignore"), mapped < 0))
                        state.mapping[static_cast<std::size_t>(col)] = -1;
                    for (int t = 0; t < static_cast<int>(targets.size()); ++t)
                        if (ImGui::Selectable(targets[static_cast<std::size_t>(t)].name, mapped == t)) {
                            for (int& m : state.mapping)
                                if (m == t)
                                    m = -1;
                            state.mapping[static_cast<std::size_t>(col)] = t;
                        }
                    ImGui::EndCombo();
                }
                ImGui::PopID();
            }
            if (state.header && !state.rows.empty()) {
                ImGui::TableNextRow();
                for (int col = 0; col < columns; ++col) {
                    ImGui::TableSetColumnIndex(col);
                    ImGui::TextColored(c.muted, "%s", static_cast<std::size_t>(col) < state.rows[0].size() ? state.rows[0][static_cast<std::size_t>(col)].c_str() : "");
                }
            }
            const int shown = std::min(state.DataRows(), 8);
            for (int r = 0; r < shown; ++r) {
                ImGui::TableNextRow();
                for (int col = 0; col < columns; ++col) {
                    ImGui::TableSetColumnIndex(col);
                    const bool used = state.mapping[static_cast<std::size_t>(col)] >= 0;
                    ImGui::TextColored(used ? c.text : c.muted, "%s", state.Cell(r, col).c_str());
                }
            }
            ImGui::EndTable();
        }
    }
    // 4. Status and actions.
    std::string missing;
    for (std::size_t t = 0; t < targets.size(); ++t) {
        if (!targets[t].required)
            continue;
        if (std::find(state.mapping.begin(), state.mapping.end(), static_cast<int>(t)) == state.mapping.end())
            missing += (missing.empty() ? "" : ", ") + std::string(targets[t].name);
    }
    ImGui::Spacing();
    ImGui::AlignTextToFramePadding();
    if (columns == 0) {
        ImGui::TextColored(c.muted, "%s", Str(options, "show.nothingPasted", "Nothing pasted yet."));
    } else if (!missing.empty()) {
        const ImVec2 a = ImGui::GetCursorScreenPos();
        ImGui::Dummy({u, u});
        GlyphWarning(draw, {a.x + u * .5f, a.y + u * .5f}, u * .7f, U32(c.warning));
        ImGui::SameLine(0, 2.f);
        ImGui::TextColored(c.warning, "%s %s", Str(options, "show.mapRequired", "Map these columns:"), missing.c_str());
    } else {
        const ImVec2 a = ImGui::GetCursorScreenPos();
        ImGui::Dummy({u, u});
        detail::DrawCheck(draw, {a.x + u * .5f, a.y + u * .5f}, u * .45f, U32(c.success), 1.8f);
        ImGui::SameLine(0, 2.f);
        ImGui::TextColored(c.text, "%d %s", state.DataRows(), state.DataRows() == 1 ? "row ready" : "rows ready");
    }
    ImGui::SameLine(std::max(ImGui::GetCursorPosX() + u, ImGui::GetContentRegionMax().x - u * 12.f));
    if (ActionButton(Str(options, "show.cancel", "Cancel"), ActionVariant::Ghost, {0, 0}, options))
        request.cancel = true;
    ImGui::SameLine();
    ImGui::BeginDisabled(columns == 0 || !missing.empty() || state.DataRows() == 0);
    if (ActionButton(Str(options, "show.import", "Import"), ActionVariant::Primary, {0, 0}, options))
        request.import = true;
    ImGui::EndDisabled();
    ImGui::PopID();
    return request;
}

// ---- Log view --------------------------------------------------------------------------------------------------
LogViewRequest LogView(const char* id, std::span<const LogEntry> entries, LogViewState& state, ImVec2 size, ComponentOptions options) {
    LogViewRequest request;
    const ShowColors c = MakeShowColors(options);
    const float u = Unit();
    int counts[3] = {0, 0, 0};
    for (const LogEntry& e : entries)
        ++counts[static_cast<int>(e.severity)];
    ImGui::PushID(id);
    bool* flags[3] = {&state.info, &state.warning, &state.error};
    const char* names[3] = {"Info", "Warnings", "Errors"};
    for (int k = 2; k >= 0; --k) {
        if (FilterChip(FrameText("lv%d", k), FrameText("%s %d", names[k], counts[k]), *flags[k], options))
            *flags[k] = !*flags[k];
        ImGui::SameLine();
    }
    ImGui::SetNextItemWidth(u * 8.f);
    ImGui::InputTextWithHint("##filter", Str(options, "show.filter", "Filter"), state.filter, sizeof(state.filter));
    ImGui::SameLine();
    if (ActionButton(Str(options, "show.clear", "Clear"), ActionVariant::Ghost, {0, 0}, options))
        request.clear = true;
    static thread_local std::vector<int> shown;
    shown.clear();
    for (int i = 0; i < static_cast<int>(entries.size()); ++i) {
        const LogEntry& e = entries[static_cast<std::size_t>(i)];
        if (!*flags[static_cast<int>(e.severity)])
            continue;
        if (state.filter[0] && !std::strstr(Safe(e.text), state.filter))
            continue;
        shown.push_back(i);
    }
    ImGui::PushStyleColor(ImGuiCol_ChildBg, c.surface);
    if (ImGui::BeginChild("##log", size, ImGuiChildFlags_Borders)) {
        auto* draw = ImGui::GetWindowDrawList();
        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(shown.size()), u * 1.2f);
        while (clipper.Step())
            for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row) {
                const int index = shown[static_cast<std::size_t>(row)];
                const LogEntry& e = entries[static_cast<std::size_t>(index)];
                ImGui::PushID(index);
                const ImVec2 a = ImGui::GetCursorScreenPos();
                const float h = u * 1.2f;
                ImGui::Selectable("##row", false, ImGuiSelectableFlags_AllowDoubleClick, {0, h});
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                    request.copyIndex = index;
                SeverityGlyph(draw, c, e.severity, {a.x + u * .5f, a.y + h * .5f}, u * .65f);
                const int t = static_cast<int>(e.seconds);
                draw->AddText({a.x + u * 1.1f, a.y + (h - ImGui::GetTextLineHeight()) * .5f}, U32(c.muted), FrameText("%02d:%02d.%03d", t / 60, t % 60, static_cast<int>((e.seconds - std::floor(e.seconds)) * 1000.0)));
                draw->AddText({a.x + u * 5.6f, a.y + (h - ImGui::GetTextLineHeight()) * .5f}, U32(e.severity == Severity::Info ? c.text : SeverityTone(c, e.severity)), Safe(e.text));
                ImGui::PopID();
            }
        if (state.follow && ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 2.f)
            ImGui::SetScrollHereY(1.f);
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::PopID();
    return request;
}
} // namespace imkit::show

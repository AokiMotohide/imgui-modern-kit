#pragma once
#include <imkit/show/controls.h>
#include <array>
#include <span>
#include <string>
#include <string_view>
#include <vector>

// Paste, check, import. The library never parses, compiles or runs a script: the host analyses the text (usually on a
// worker thread) and hands back diagnostics and the parameters it found. These components show that result next to
// the text, and turn a pasted table into columns the host can map.
namespace imkit::show {
enum class Severity { Info, Warning, Error };
struct Diagnostic {
    int line = 1;                   // 1-based.
    int column = 0;                 // 1-based; zero marks the whole line.
    int length = 0;                 // Characters to underline; zero extends to the end of the word.
    Severity severity = Severity::Error;
    const char* message = "";
    const char* suggestion = "";    // "Did you mean light?" style hint; empty for none.
};

enum class ParamType { Float, Int, Bool, Color, Point2D, Choice, Event };
struct ParamDecl {
    const char* name = "";          // Identifier, used as the accessible name when `label` is empty.
    const char* label = "";
    ParamType type = ParamType::Float;
    float minimum = 0, maximum = 1, defaultValue = 0;
    std::span<const char* const> choices{};
};
struct ParamValue {
    float v[4]{};                   // Float/Int/Bool/Choice use v[0], Point2D v[0..1], Color v[0..3].
};
struct ParamEdit {
    int index = -1;
    ValueEdit edit{};               // Edit of v[0] (Point2D: x; Color: reported as changed with v[0] = red).
    bool triggered = false;         // An Event parameter's button was pressed.
};
// Builds one control per declaration: Fader, Toggle, colour swatch, XY pad, combo or trigger button. Values are
// host-owned; `values` must hold at least as many entries as `decls`.
ParamEdit ParameterPanel(const char* id, std::span<const ParamDecl> decls, std::span<ParamValue> values, ComponentOptions options = {});

// ---- Paste panel --------------------------------------------------------------------------------------------
struct ScriptAnalysis {
    std::span<const Diagnostic> diagnostics{};
    const char* format = "";        // Detected kind, for example "ISF shader" or "cue script".
    const char* summary = "";       // For example "4 cues found".
    bool analyzing = false;         // The host is still checking the latest text.
    bool importable = true;         // False when the host cannot import this text at all (nothing recognised).
};
struct ScriptPasteState {
    int lastLength = 0;
    double lastEdit = 0;
    bool pending = false;
    int revision = 0;               // Increments on every text change.
};
struct ScriptPasteOptions {
    ImVec2 size{0, 0};              // Zero fills the width and uses about 14 frame heights.
    const char* placeholder = "";
    double now = 0;                 // Host clock in seconds; used only to debounce analysis.
    double debounceSeconds = .35;
    bool allowImportWithErrors = true; // Shows "Import anyway" instead of blocking, so a quality problem warns and continues.
    const char* importLabel = "Import";
};
struct ScriptPasteRequest {
    bool textChanged = false;
    bool pasted = false;            // A large insertion arrived in one frame (a paste or a drop).
    bool analyze = false;           // The text settled: run the host's analysis now.
    bool import = false;
    bool clear = false;
    int jumpToLine = 0;             // A gutter mark or a diagnostic was clicked.
};
// `text` is the host's editable buffer (multi-line, standard Dear ImGui editing and IME). Line numbers and
// diagnostic marks sit in a gutter and the status row summarises the analysis.
ScriptPasteRequest ScriptPastePanel(const char* id, char* text, std::size_t capacity, const ScriptAnalysis& analysis, ScriptPasteState& state,
                                    ScriptPasteOptions layout = {}, ComponentOptions options = {});
struct DiagnosticListRequest {
    int jumpToLine = -1;            // A row was clicked.
    int applySuggestion = -1;       // Index of the diagnostic whose suggestion chip was clicked.
};
DiagnosticListRequest DiagnosticList(const char* id, std::span<const Diagnostic> diagnostics, ImVec2 size = {0, 0}, ComponentOptions options = {});

// ---- Table paste ------------------------------------------------------------------------------------------
// Picks the separator that splits the first lines into the same number of cells: tab, comma, semicolon or pipe.
// Returns '\0' when none fits.
char DetectDelimiter(std::string_view text);
// Splits delimited text (quotes may hold separators and doubled quotes) into rows of cells.
void ParseDelimited(std::string_view text, char delimiter, std::vector<std::vector<std::string>>& rows);
struct ImportColumn {
    const char* name = "";          // Target field, for example "Number".
    const char* aliases = "";       // Comma-separated header names that map automatically, for example "no,cue,#".
    bool required = false;
};
struct ImportState {
    std::array<char, 32768> buffer{};
    char delimiter = 0;             // Zero detects it.
    bool header = true;
    int revision = 0;
    std::vector<std::vector<std::string>> rows;
    std::vector<int> mapping;       // Target column per source column, -1 = ignore.
    int mappedRevision = -1;
    // Parses the buffer and proposes a mapping from header names. The wizard calls this when the text changes.
    void Reparse(std::span<const ImportColumn> targets);
    int Columns() const;
    // Cell of a data row (excluding the header row when `header`).
    const std::string& Cell(int dataRow, int column) const;
    int DataRows() const;
};
struct ImportRequest {
    bool import = false;
    bool cancel = false;
};
// Paste area, separator choice, header toggle, a mapping combo per column and a preview of the first rows. Import is
// enabled once every required target is mapped.
ImportRequest PasteImportWizard(const char* id, ImportState& state, std::span<const ImportColumn> targets, ComponentOptions options = {});

// ---- Log view -----------------------------------------------------------------------------------------------
struct LogEntry {
    double seconds = 0;
    Severity severity = Severity::Info;
    const char* text = "";
};
struct LogViewState {
    bool info = true, warning = true, error = true, follow = true;
    char filter[64]{};
};
struct LogViewRequest {
    bool clear = false;
    int copyIndex = -1;             // Double-click on a row.
};
LogViewRequest LogView(const char* id, std::span<const LogEntry> entries, LogViewState& state, ImVec2 size = {0, 0}, ComponentOptions options = {});
} // namespace imkit::show

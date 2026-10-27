#include <imkit/imkit.h>
#include <imkit/show.h>
#include <imkit/show/script.h>
#include <array>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

namespace {
int failures = 0;
void Check(bool ok, const char* label) {
    if (!ok) {
        std::fprintf(stderr, "FAIL: %s\n", label);
        ++failures;
    }
}
} // namespace

using namespace imkit;
using namespace imkit::show;

int main() {
    // Delimiters and parsing.
    Check(DetectDelimiter("a\tb\tc\n1\t2\t3\n") == '\t' && DetectDelimiter("a,b\n1,2\n") == ',' && DetectDelimiter("a;b;c\n1;2;3") == ';' && DetectDelimiter("a|b\n1|2") == '|',
          "delimiters are detected");
    Check(DetectDelimiter("just text\nmore text") == 0 && DetectDelimiter("") == 0, "plain text has no delimiter");
    Check(DetectDelimiter("a,b\n1,2,3\n") == 0, "inconsistent columns do not count");
    Check(DetectDelimiter("\"a,b\",c\n1,2\n") == ',', "separators inside quotes are ignored");
    {
        std::vector<std::vector<std::string>> rows;
        ParseDelimited("No,Name\r\n10,\"Intro, loop\"\r\n\r\n20,\"He said \"\"hi\"\"\"\n", ',', rows);
        Check(rows.size() == 3 && rows[1][1] == "Intro, loop" && rows[2][1] == "He said \"hi\"" && rows[0][0] == "No", "quotes, doubled quotes, CRLF and blank lines");
        ParseDelimited("  a \t b ", '\t', rows);
        Check(rows.size() == 1 && rows[0][0] == "a" && rows[0][1] == "b", "cells are trimmed");
    }

    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.DisplaySize = {1500, 3200};
    io.DeltaTime = 1.f / 60;
    io.Fonts->AddFontDefault();
    unsigned char* pixels;
    int w, h;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &w, &h);
    auto theme = MakeTheme(ThemePreset::PrecisionDark);
    ApplyTheme(theme);
    std::array<accessibility::SemanticNode, 1024> nodes{};
    accessibility::AccessibilityFrame semantics{nodes};
    AnimationState animation;
    ComponentOptions options{&theme, &animation, &semantics};

    // Import wizard state.
    ImportState importState;
    std::snprintf(importState.buffer.data(), importState.buffer.size(), "No,Name,Duration\n10,Intro,0:42\n20,Walk-in,0:08\n");
    const ImportColumn targets[] = {{"Number", "no,#", false}, {"Name", "title", true}, {"Duration", "length,time", false}};
    importState.Reparse(targets);
    Check(importState.Columns() == 3 && importState.DataRows() == 2 && importState.mapping[0] == 0 && importState.mapping[1] == 1 && importState.mapping[2] == 2, "headers map to targets by name and alias");
    Check(importState.Cell(0, 1) == "Intro" && importState.Cell(1, 2) == "0:08" && importState.Cell(5, 0).empty(), "data cells skip the header");
    importState.header = false;
    importState.Reparse(targets);
    Check(importState.DataRows() == 3 && importState.mapping[0] == -1, "without a header nothing maps automatically");
    importState.header = true;
    importState.Reparse(targets);
    importState.mappedRevision = -1;

    // Paste panel state.
    std::array<char, 2048> script{};
    ScriptPasteState pasteState;
    std::array<Diagnostic, 2> diagnostics{};
    diagnostics[0] = {3, 8, 0, Severity::Error, "Unknown command lights-up", "light"};
    diagnostics[1] = {1, 1, 3, Severity::Warning, "Deprecated keyword", ""};
    ScriptAnalysis analysis;
    analysis.diagnostics = diagnostics;
    analysis.format = "cue script";
    analysis.summary = "4 cues found";
    ScriptPasteRequest pasteRequest;
    ScriptPasteOptions pasteOptions;
    pasteOptions.size = {900, 360};
    DiagnosticListRequest listRequest;
    std::array<ParamDecl, 5> decls{};
    const char* choices[] = {"Normal", "Add", "Screen"};
    decls[0] = {"speed", "Speed", ParamType::Float, 0, 4, 1, {}};
    decls[1] = {"mirror", "Mirror", ParamType::Bool, 0, 1, 0, {}};
    decls[2] = {"mode", "Blend", ParamType::Choice, 0, 2, 0, choices};
    decls[3] = {"flash", "Flash", ParamType::Event, 0, 1, 0, {}};
    decls[4] = {"centre", "Centre", ParamType::Point2D, 0, 1, .5f, {}};
    std::array<ParamValue, 5> values{};
    values[0].v[0] = 1;
    ParamEdit paramEdit;
    std::array<LogEntry, 3> logEntries{};
    logEntries[0] = {1.0, Severity::Info, "Compiled in 12 ms"};
    logEntries[1] = {2.0, Severity::Warning, "Uniform unused: gain"};
    logEntries[2] = {3.0, Severity::Error, "Line 3: syntax error"};
    LogViewState logState;
    ImportRequest importRequest;
    auto frame = [&](double now) {
        semantics.Begin(static_cast<std::uint64_t>(ImGui::GetFrameCount()) + 1);
        ImGui::NewFrame();
        ImGui::SetNextWindowPos({0, 0});
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("Host");
        pasteOptions.now = now;
        pasteRequest = ScriptPastePanel("script", script.data(), script.size(), analysis, pasteState, pasteOptions, options);
        listRequest = DiagnosticList("problems", diagnostics, {800, 0}, options);
        paramEdit = ParameterPanel("params", decls, values, options);
        LogView("log", logEntries, logState, {800, 140}, options);
        importRequest = PasteImportWizard("import", importState, targets, options);
        ImGui::End();
        ImGui::Render();
    };
    double clock = 0;
    auto step = [&] {
        clock += 1.0 / 60;
        frame(clock);
    };
    auto find = [&](std::string_view name) -> const accessibility::SemanticNode* {
        for (const auto& n : semantics.Tree().nodes)
            if (n.name == name)
                return &n;
        return nullptr;
    };
    auto centre = [](const accessibility::SemanticNode& n) { return ImVec2{(n.minimum.x + n.maximum.x) * .5f, (n.minimum.y + n.maximum.y) * .5f}; };
    auto click = [&](ImVec2 p) {
        io.AddMousePosEvent(p.x, p.y);
        step();
        io.AddMouseButtonEvent(0, true);
        step();
    };
    // Buttons and toggles fire on release, so press, release and check in the release frame.
    auto tap = [&](ImVec2 p) {
        click(p);
        io.AddMouseButtonEvent(0, false);
        step();
    };
    auto release = [&] {
        io.AddMouseButtonEvent(0, false);
        for (int i = 0; i < 32; ++i)
            step();
    };
    for (int i = 0; i < 3; ++i)
        step();
    Check(semantics.Tree().Validate(), "the script components publish a valid semantic tree");
    Check(find("script") && find("Import anyway") && find("Clear"), "the paste panel names its controls");

    // Typing and pasting.
    if (const auto* field = find("script")) {
        click(centre(*field));
        release();
        io.AddInputCharactersUTF8("light");
        step();
        Check(pasteRequest.textChanged && !pasteRequest.pasted, "typing a few characters is not a paste");
        step();
        step();
        Check(std::strcmp(script.data(), "light") == 0, "typed text lands in the host buffer");
        io.AddInputCharactersUTF8("cue 10 \"Intro\" play intro.mov\ncue 20");
        step();
        Check(pasteRequest.textChanged && pasteRequest.pasted, "a large insertion is reported as a paste");
        bool analyzed = false;
        for (int i = 0; i < 40 && !analyzed; ++i) {
            step();
            analyzed = pasteRequest.analyze;
        }
        Check(analyzed, "analysis is requested once the text settles");
    } else
        Check(false, "script field");

    // Import is allowed with errors (warn and continue) and is reported.
    if (const auto* import = find("Import anyway")) {
        tap(centre(*import));
        Check(pasteRequest.import, "importing despite errors is possible and reported");
        release();
    } else
        Check(false, "import button");
    // Diagnostics list.
    if (const auto* row = find("Line 3: Unknown command lights-up")) {
        click(centre(*row));
        Check(listRequest.jumpToLine == 3, "a diagnostic row jumps to its line");
        release();
    } else
        Check(false, "diagnostic row");
    if (const auto* fix = find("Apply: light")) {
        click(centre(*fix));
        Check(listRequest.applySuggestion == 0, "the suggestion chip is reported");
        release();
    } else
        Check(false, "suggestion chip");
    // Parameter panel.
    if (const auto* flash = find("Flash")) {
        tap(centre(*flash));
        Check(paramEdit.index == 3 && paramEdit.triggered, "an event parameter reports its trigger");
        release();
    } else
        Check(false, "event parameter");
    if (const auto* mirror = find("Mirror")) {
        tap(centre(*mirror));
        Check(paramEdit.index == 1 && values[1].v[0] > .5f, "a bool parameter toggles");
        release();
    } else
        Check(false, "bool parameter");
    Check(find("Speed") && find("Speed")->role == accessibility::SemanticRole::Slider, "a float parameter is a slider");
    // Import wizard.
    if (const auto* button = find("Import")) {
        tap(centre(*button));
        Check(importRequest.import, "a fully mapped table can be imported");
        release();
    } else
        Check(false, "wizard import button");
    ImGui::DestroyContext();
    std::printf("Show script failures: %d\n", failures);
    return failures == 0 ? 0 : 1;
}

#include "show_pages.h"
#include "gallery.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string_view>

namespace imkit::gallery {
using namespace show;

namespace {
const char* SampleScript =
    "/*{\n"
    "  \"DESCRIPTION\": \"Plasma\",\n"
    "  \"ISFVSN\": \"2.0\",\n"
    "  \"INPUTS\": [\n"
    "    {\"NAME\": \"speed\", \"TYPE\": \"float\", \"MIN\": 0.0, \"MAX\": 4.0, \"DEFAULT\": 1.0},\n"
    "    {\"NAME\": \"mirror\", \"TYPE\": \"bool\", \"DEFAULT\": false},\n"
    "    {\"NAME\": \"tint\", \"TYPE\": \"color\"},\n"
    "    {\"NAME\": \"centre\", \"TYPE\": \"point2D\", \"MIN\": 0.0, \"MAX\": 1.0, \"DEFAULT\": [0.5, 0.5]},\n"
    "    {\"NAME\": \"flash\", \"TYPE\": \"event\"}\n"
    "  ]\n"
    "}*/\n"
    "void main() {\n"
    "  vec2 uv = isf_FragNormCoord;\n"
    "  float t = TIME * speed\n"
    "  gl_FragColor = vec4(uv, sin(t), 1.0);\n"
    "}\n";

int Distance(std::string_view a, std::string_view b) {
    std::vector<int> row(b.size() + 1);
    for (std::size_t j = 0; j <= b.size(); ++j)
        row[j] = static_cast<int>(j);
    for (std::size_t i = 1; i <= a.size(); ++i) {
        int diagonal = row[0];
        row[0] = static_cast<int>(i);
        for (std::size_t j = 1; j <= b.size(); ++j) {
            const int keep = row[j];
            row[j] = std::min({row[j] + 1, row[j - 1] + 1, diagonal + (a[i - 1] == b[j - 1] ? 0 : 1)});
            diagonal = keep;
        }
    }
    return row[b.size()];
}
// Finds `"KEY"` inside [begin, end) and returns the text after the colon, trimmed, up to a comma or the end.
std::string JsonValue(std::string_view object, const char* key) {
    const std::string needle = std::string("\"") + key + "\"";
    const std::size_t at = object.find(needle);
    if (at == std::string_view::npos)
        return {};
    std::size_t i = object.find(':', at + needle.size());
    if (i == std::string_view::npos)
        return {};
    ++i;
    while (i < object.size() && std::isspace(static_cast<unsigned char>(object[i])))
        ++i;
    std::size_t end = i;
    int depth = 0;
    bool quoted = false;
    while (end < object.size()) {
        const char c = object[end];
        if (c == '"')
            quoted = !quoted;
        else if (!quoted && (c == '[' || c == '{'))
            ++depth;
        else if (!quoted && (c == ']' || c == '}')) {
            if (depth == 0)
                break;
            --depth;
        } else if (!quoted && c == ',' && depth == 0)
            break;
        ++end;
    }
    std::string v(object.substr(i, end - i));
    while (!v.empty() && std::isspace(static_cast<unsigned char>(v.back())))
        v.pop_back();
    if (v.size() >= 2 && v.front() == '"' && v.back() == '"')
        v = v.substr(1, v.size() - 2);
    return v;
}
} // namespace

void ShowPages::Script(GalleryState& host) {
    ComponentOptions o{&host.theme, &host.animation};
    ScriptSim& s = script;
    const double now = ImGui::GetTime();
    const float u = ImGui::GetFrameHeight();

    if (!s.initialized) {
        s.initialized = true;
        std::snprintf(s.text.data(), s.text.size(), "%s", SampleScript);
        s.logText.reserve(256);
        s.import.header = true;
        std::snprintf(s.import.buffer.data(), s.import.buffer.size(), "No\tTitle\tLength\tNotes\n10\tHouse open\t0:30\tLights half\n20\tIntro loop\t0:42\t\n30\tSpeaker walk-in\t0:08\tWalk-on music\n");
    }
    auto log = [&](Severity severity, std::string text) {
        if (s.logText.size() >= 200) {
            s.logText.erase(s.logText.begin(), s.logText.begin() + 50);
            s.log.erase(s.log.begin(), s.log.begin() + 50);
        }
        s.logText.push_back(std::move(text));
        s.log.push_back({now, severity, ""});
        for (std::size_t i = 0; i < s.log.size(); ++i)
            s.log[i].text = s.logText[i].c_str();
    };

    // The host analyses the text. Here it is a small synchronous checker made to show the panel states: a short
    // "checking" period first, then the diagnostics, the detected format and the parameters.
    auto analyze = [&]() {
        const std::string_view text(s.text.data());
        s.diagnostics.clear();
        s.messages.clear();
        s.suggestions.clear();
        s.decls.clear();
        s.names.clear();
        s.values.clear();
        s.format.clear();
        s.summary.clear();
        s.importable = !text.empty();
        if (text.empty())
            return;
        std::vector<Diagnostic> found;
        std::vector<std::pair<std::string, std::string>> notes; // message, suggestion
        const bool isf = text.find("\"INPUTS\"") != std::string_view::npos;
        const bool cue = text.rfind("cue ", 0) == 0 || text.find("\ncue ") != std::string_view::npos;
        if (isf || text.find("void main") != std::string_view::npos || text.find("mainImage") != std::string_view::npos) {
            s.format = isf ? "ISF shader" : text.find("mainImage") != std::string_view::npos ? "Shadertoy" : "GLSL";
            // Parameters from the ISF header.
            std::size_t pos = 0;
            while (isf && (pos = text.find("\"NAME\"", pos)) != std::string_view::npos) {
                const std::size_t open = text.rfind('{', pos), close = text.find('}', pos);
                if (open == std::string_view::npos || close == std::string_view::npos)
                    break;
                const std::string_view object = text.substr(open, close - open + 1);
                const std::string name = JsonValue(object, "NAME"), type = JsonValue(object, "TYPE");
                s.names.push_back(name);
                ParamDecl d;
                d.type = type == "bool" ? ParamType::Bool : type == "color" ? ParamType::Color : type == "point2D" ? ParamType::Point2D : type == "event" ? ParamType::Event
                         : type == "long" ? ParamType::Choice : ParamType::Float;
                const std::string mn = JsonValue(object, "MIN"), mx = JsonValue(object, "MAX"), def = JsonValue(object, "DEFAULT");
                d.minimum = mn.empty() ? 0.f : static_cast<float>(std::atof(mn.c_str()));
                d.maximum = mx.empty() ? 1.f : static_cast<float>(std::atof(mx.c_str()));
                d.defaultValue = def.empty() || def[0] == '[' ? .5f : static_cast<float>(std::atof(def.c_str()));
                s.decls.push_back(d);
                ParamValue v;
                v.v[0] = v.v[1] = d.defaultValue;
                v.v[3] = 1.f;
                s.values.push_back(v);
                pos = close;
            }
            for (std::size_t i = 0; i < s.decls.size(); ++i)
                s.decls[i].name = s.names[i].c_str();
            // Brace balance and missing semicolons inside main().
            int depth = 0, line = 1;
            bool inMain = false;
            std::size_t lineStart = 0;
            for (std::size_t i = 0; i <= text.size(); ++i) {
                if (i == text.size() || text[i] == '\n') {
                    std::string_view l = text.substr(lineStart, i - lineStart);
                    while (!l.empty() && std::isspace(static_cast<unsigned char>(l.back())))
                        l.remove_suffix(1);
                    if (l.find("void main") != std::string_view::npos)
                        inMain = true;
                    if (inMain && !l.empty() && l.find_first_not_of(" \t") != std::string_view::npos) {
                        const char last = l.back();
                        const bool statement = l.find('=') != std::string_view::npos || l.find('(') != std::string_view::npos;
                        if (statement && last != ';' && last != '{' && last != '}' && last != ')' && l.find("void main") == std::string_view::npos) {
                            found.push_back({line, static_cast<int>(l.size()), 1, Severity::Error, "", ""});
                            notes.push_back({"Expected ';' at the end of the statement", ""});
                        }
                    }
                    ++line;
                    lineStart = i + 1;
                    continue;
                }
                depth += text[i] == '{' ? 1 : text[i] == '}' ? -1 : 0;
            }
            if (depth != 0) {
                found.push_back({line - 1, 1, 0, Severity::Error, "", ""});
                notes.push_back({"Braces are not balanced", ""});
            }
            if (text.find("TIME") != std::string_view::npos && !isf) {
                found.push_back({1, 1, 0, Severity::Info, "", ""});
                notes.push_back({"TIME is an ISF built-in; define it as a uniform for plain GLSL", ""});
            }
            s.summary = std::to_string(s.decls.size()) + " parameters found";
        } else if (cue) {
            s.format = "cue script";
            static const char* commands[] = {"cue", "wait", "light", "fade", "go", "stop"};
            int cues = 0, line = 1;
            std::size_t lineStart = 0;
            for (std::size_t i = 0; i <= text.size(); ++i) {
                if (i == text.size() || text[i] == '\n') {
                    std::string_view l = text.substr(lineStart, i - lineStart);
                    const std::size_t first = l.find_first_not_of(" \t\r");
                    if (first != std::string_view::npos) {
                        const std::size_t end = l.find_first_of(" \t\r", first);
                        const std::string_view word = l.substr(first, end == std::string_view::npos ? std::string_view::npos : end - first);
                        bool known = false;
                        for (const char* c : commands)
                            known = known || word == c;
                        if (word == "cue")
                            ++cues;
                        if (!known) {
                            const char* best = commands[0];
                            int bestDistance = 99;
                            for (const char* c : commands)
                                if (const int d = Distance(word, c); d < bestDistance) {
                                    bestDistance = d;
                                    best = c;
                                }
                            found.push_back({line, static_cast<int>(first) + 1, static_cast<int>(word.size()), Severity::Error, "", ""});
                            notes.push_back({"Unknown command '" + std::string(word) + "'", bestDistance <= 3 ? best : ""});
                        }
                    }
                    ++line;
                    lineStart = i + 1;
                }
            }
            s.summary = std::to_string(cues) + " cues found";
        } else {
            s.format = "text";
            s.importable = false;
            found.push_back({1, 1, 0, Severity::Warning, "", ""});
            notes.push_back({"This text was not recognised as a shader or a cue script.", ""});
        }
        for (std::size_t i = 0; i < found.size(); ++i) {
            s.messages.push_back(notes[i].first);
            s.suggestions.push_back(notes[i].second);
        }
        for (std::size_t i = 0; i < found.size(); ++i) {
            found[i].message = s.messages[i].c_str();
            found[i].suggestion = s.suggestions[i].c_str();
        }
        s.diagnostics = std::move(found);
        s.values.resize(s.decls.size());
        char summary[96];
        int errors = 0;
        for (const Diagnostic& d : s.diagnostics)
            errors += d.severity == Severity::Error ? 1 : 0;
        std::snprintf(summary, sizeof summary, "%s, %d error(s)", s.summary.c_str(), errors);
        log(errors ? Severity::Error : Severity::Info, std::string("Checked ") + s.format + ": " + summary);
    };

    if (s.lastRevision < 0) {
        s.lastRevision = 0;
        analyze(); // The first look already shows the checked state.
    }
    if (BeginGrid(host, "show-script-grid", 2)) {
        const bool s1 = BeginSection(host, "Paste a script", "Paste a shader (ISF, GLSL, Shadertoy) or a cue script. Problems show in the gutter, line by line.");
        if (s1) {
            ScriptAnalysis analysis;
            analysis.diagnostics = s.diagnostics;
            analysis.format = s.format.c_str();
            analysis.summary = s.summary.c_str();
            analysis.analyzing = s.analyzing;
            analysis.importable = s.importable;
            ScriptPasteOptions po;
            po.size = {0, u * 17.f};
            po.now = now;
            po.placeholder = "Paste a script here";
            const ScriptPasteRequest r = ScriptPastePanel("script", s.text.data(), s.text.size(), analysis, s.state, po, o);
            Record(host, "show-script");
            if (r.textChanged)
                log(Severity::Info, r.pasted ? "Pasted text" : "Edited text");
            if (r.analyze) {
                s.analyzing = true;
                s.finishAt = now + .4;
            }
            if (s.analyzing && now >= s.finishAt) {
                s.analyzing = false;
                analyze();
            }
            if (r.clear) {
                s.text[0] = 0;
                s.diagnostics.clear();
                s.decls.clear();
                s.values.clear();
                s.format.clear();
                s.summary.clear();
                s.importable = false;
            }
            if (r.import) {
                ++s.importedScripts;
                log(Severity::Info, "Imported " + (s.format.empty() ? std::string("script") : s.format));
            }
            ImGui::Spacing();
            const DiagnosticListRequest dr = DiagnosticList("problems", s.diagnostics, {0, 0}, o);
            Record(host, "show-diagnostics");
            if (dr.applySuggestion >= 0 && static_cast<std::size_t>(dr.applySuggestion) < s.diagnostics.size()) {
                const Diagnostic& d = s.diagnostics[static_cast<std::size_t>(dr.applySuggestion)];
                // Replace the word at the diagnostic with the suggestion.
                std::string text(s.text.data());
                std::size_t at = 0;
                for (int line = 1; line < d.line && at != std::string::npos; ++line)
                    at = text.find('\n', at) == std::string::npos ? std::string::npos : text.find('\n', at) + 1;
                if (at != std::string::npos) {
                    const std::size_t from = at + static_cast<std::size_t>(std::max(0, d.column - 1));
                    text.replace(from, static_cast<std::size_t>(std::max(0, d.length)), d.suggestion);
                    std::snprintf(s.text.data(), s.text.size(), "%s", text.c_str());
                    s.state.pending = true;
                    s.state.lastEdit = now;
                }
            }
            ImGui::TextDisabled("Imported scripts: %d", s.importedScripts);
        }
        EndSection(s1);

        const bool s2 = BeginSection(host, "Detected parameters", "Controls generated from the parameters the host found in the text.");
        if (s2) {
            if (s.decls.empty()) {
                ImGui::TextDisabled("Paste a shader with INPUTS to get controls.");
            } else {
                const ParamEdit e = ParameterPanel("params", s.decls, s.values, o);
                Record(host, "show-params");
                if (e.triggered)
                    log(Severity::Info, "Event: " + std::string(s.decls[static_cast<std::size_t>(e.index)].name));
            }
        }
        EndSection(s2);

        const bool s3 = BeginSection(host, "Import a table", "Paste cells from a spreadsheet. Columns map by header name; the rest is yours to map.");
        if (s3) {
            static const ImportColumn targets[] = {{"Number", "no,#,cue", true}, {"Name", "title,label", true}, {"Duration", "length,time", false}, {"Notes", "note,comment", false}};
            const ImportRequest r = PasteImportWizard("import", s.import, targets, o);
            Record(host, "show-import");
            if (r.import) {
                s.imported += s.import.DataRows();
                log(Severity::Info, "Imported " + std::to_string(s.import.DataRows()) + " cues from the table");
            }
            ImGui::TextDisabled("Cues imported from tables: %d", s.imported);
        }
        EndSection(s3);

        const bool s4 = BeginSection(host, "Log", "Host messages with severity filters and search.");
        if (s4) {
            LogView("log", s.log, s.logState, {0, u * 1.2f * 8.f}, o);
            Record(host, "show-log");
        }
        EndSection(s4);
        EndGrid();
    }
}
} // namespace imkit::gallery

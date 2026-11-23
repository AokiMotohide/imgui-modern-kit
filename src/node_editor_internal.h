#pragma once
#include <imkit/node_editor.h>
#include <algorithm>
#include <array>
#include <string_view>

namespace imkit::node_editor::detail {
inline constexpr float kPi = 3.14159265358979f;
inline constexpr float kKnotRing = 8;

inline ImVec2 Add(ImVec2 a, ImVec2 b) { return {a.x + b.x, a.y + b.y}; }
inline ImVec2 Sub(ImVec2 a, ImVec2 b) { return {a.x - b.x, a.y - b.y}; }
inline ImVec2 Mul(ImVec2 a, float n) { return {a.x * n, a.y * n}; }
inline float Dot(ImVec2 a, ImVec2 b) { return a.x * b.x + a.y * b.y; }
inline ImU32 Color(ImVec4 c) { return ImGui::GetColorU32(c); }
inline void Text(std::string_view s) { ImGui::TextUnformatted(s.data(), s.data() + s.size()); }
inline bool Inside(ImVec2 p, ImVec2 a, ImVec2 b) { return p.x >= a.x && p.y >= a.y && p.x <= b.x && p.y <= b.y; }
inline ImVec4 WithAlpha(ImVec4 c, float alpha) {
    c.w *= alpha;
    return c;
}
inline float KnotRadius(double zoom) { return std::max(4.f, 6.5f * float(zoom)); }
inline ImVec2 Direction(Side side) {
    switch (side) {
    case Side::Left:  return {-1, 0};
    case Side::Right: return {1, 0};
    case Side::Top:   return {0, -1};
    default:          return {0, 1};
    }
}

const NodeView *Find(GraphView g, NodeId id);
const PinView *FindPin(GraphView g, PinId id);
const NodeView *Find(const EditorFrame &f, NodeId id);
const PinView *FindPin(const EditorFrame &f, PinId id);
const NodeStyle &Style(const EditorFrame &f, const NodeView &n);
Point Position(const EditorFrame &f, const NodeView &n);
Point Size(const EditorFrame &f, const NodeView &n);
ImVec2 Screen(const EditorFrame &f, Point p);
Point World(const EditorFrame &f, ImVec2 p);
bool Hidden(const EditorFrame &f, const NodeView &n);
bool Locked(const EditorFrame &f, const NodeView &n);
bool Visible(const EditorFrame &f, const NodeView &n);
const char *Tr(const EditorFrame &f, std::string_view key, const char *fallback);
ImVec4 PinColor(const EditorFrame &f, const PinView &pin);
bool Concealed(const EditorFrame &f, const PinView &pin);
ImVec2 PinPosition(const EditorFrame &f, const PinView &pin);
float SegmentDistance(ImVec2 p, ImVec2 a, ImVec2 b);
Side PinSide(const EditorFrame &f, const PinView &pin);
std::array<ImVec2, 25> CurvePoints(LinkStyle style, ImVec2 p, ImVec2 dp, ImVec2 q, ImVec2 dq);
bool IsMultiInput(const PinView &pin);
ConnectionVerdict DragVerdict(const EditorFrame &f, PinId target);
std::array<ImVec2, 25> LinkCurve(const EditorFrame &f, const LinkView &link, const PinView &a, const PinView &b);
void DashedPolyline(ImDrawList *dl, const std::array<ImVec2, 25> &points, ImU32 color, float width, int on, int off);
EditRequest Request(EditorFrame &f, EditKind kind, NodeId node = {});
bool Chord(ImGuiKeyChord chord);
PreviewState &GetPreview(EditorState &s, NodeId node);
void DrawOutput(const PreviewOutput &o, ImVec2 size, bool interactive);
void ApplyLayout(EditorFrame &f, int action);
void ReorderTarget(EditorFrame &f, const PinView &pin, int index);
void PinActions(EditorFrame &f, const PinView &pin);
} // namespace imkit::node_editor::detail

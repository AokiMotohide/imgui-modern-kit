#pragma once
#include <imkit/editor_core.h>
namespace imkit::cg {
using editor::StableId;
struct Vec3 {
    double x = 0, y = 0, z = 0;
};
struct Transform {
    Vec3 translation{}, rotation{}, scale{1, 1, 1};
};
enum class Projection { Perspective, Orthographic, Camera };
enum class Shading { Wireframe, Solid, Material, Rendered };
enum class TransformTool { Select, Translate, Rotate, Scale, Unified };
enum class Orientation { World, Local, View, Parent, Custom };
enum class Pivot { Individual, Median, Bounds, Cursor };
enum class Axis { None, X, Y, Z, XY, YZ, ZX, Screen };
struct Basis {
    Vec3 x{1, 0, 0}, y{0, 1, 0}, z{0, 0, 1};
};
struct Camera {
    Vec3 target{};
    double yaw = .65, pitch = .4, distance = 6, verticalFov = .8, orthographicHeight = 5;
    Projection projection = Projection::Perspective;
};
Basis OrientationBasis(Orientation orientation, const Transform &object, const Camera &camera,
                       const Basis &parent = {}, const Basis &custom = {});
struct ProjectionResult {
    ImVec2 screen{};
    double depth = 0;
    bool visible = false;
};
ProjectionResult Project(Vec3 world, const Camera &camera, ImVec2 origin, ImVec2 size);
// Orbit/pan are screen pixel deltas. Orthographic zoom changes the visible height.
void NavigateCamera(Camera &camera, editor::Point orbitPixels, editor::Point panPixels,
                    double wheel, double viewportHeight);
void AlignCamera(Camera &camera, Axis axis, bool negative = false);
Transform TransformDelta(const Transform &original, TransformTool tool, Axis axis, Vec3 delta,
                         const Basis &basis, double snap = 0, bool fine = false);
// Applies the same constrained delta to an object's transform and its offset from a shared pivot.
// Basis must be orthonormal. Transform stores TRS; it cannot represent affine shear.
Transform TransformAroundPivot(const Transform &original, TransformTool tool, Axis axis, Vec3 delta,
                               const Basis &basis, Vec3 pivot, double snap = 0, bool fine = false);
struct ObjectView {
    StableId id = 0, parent = 0;
    const char *label = "";
    Transform transform{};
    bool selected = false, visible = true, selectable = true, renderable = true, locked = false;
    int depth = 0;
    bool expanded = true, hasChildren = false;
    StableId geometry=0; // Optional host-owned geometry data block; zero means none.
};
struct SceneProvider {
    void *user = nullptr;
    std::uint64_t revision = 0;
    int visibleCount = 0;
    std::span<const ObjectView> (*query)(void *, int first, int count, std::string_view filter) = nullptr;
};
struct TransformCompanion {
    Transform original{};
    editor::Transaction transform, position;
};
struct ViewportState {
    Camera camera{};
    TransformTool tool = TransformTool::Translate;
    Orientation orientation = Orientation::World;
    Pivot pivot = Pivot::Median;
    Shading shading = Shading::Solid;
    bool grid = true, axes = true, origins = true, gizmo = true, safeFrame = false, wireframe = false,
         normals = false, snap = false;
    editor::Transaction drag;
    Axis activeAxis = Axis::None;
    editor::Point mouseStart{};
    Transform original{};
    Basis customBasis{};
    Basis parentBasis{};
    Vec3 pivotPosition{}; // Host-computed median/bounds/cursor for non-individual pivots.
    const Camera *cameraView = nullptr; // Optional non-owning host camera; valid throughout the call.
    bool navigationGizmo = true;
    editor::Transaction pivotDrag; // Position companion for rotation/scale around an external pivot.
    std::span<const ObjectView> selectedObjects; // Complete host selection, including offscreen objects.
    std::span<TransformCompanion> companions; // Host storage; keep stable throughout a gesture.
    std::size_t companionCount=0;
    editor::CanvasState selectionCanvas;
    std::span<editor::SelectablePoint> selectionPoints;
    bool lassoSelection=false;
    const IconAtlas *icons=nullptr;
    std::uint64_t selectionRevision=0;
    editor::Point rotationMouse{}; // Previous sample for continuous ring rotation.
    double rotationAngle=0; // Unwrapped gesture angle in radians.
};
struct ViewportView {
    ImVec2 min{}, size{};
    bool hovered = false;
};
ViewportView BeginViewport(const char *id, ViewportState &state, ImTextureRef hostTexture, ImVec2 size,
                           const Theme &theme);
void ViewportObjects(const ViewportView &view, std::span<const ObjectView> visible, ViewportState &state,
                     editor::Selection &selection, std::uint64_t revision, editor::EventBuffer &events,
                     const Theme &theme);
void TransformGizmo(const ViewportView &view, const ObjectView &object, ViewportState &state,
                    std::uint64_t revision, editor::EventBuffer &events, const Theme &theme);
void EndViewport();
struct OutlinerState {
    char search[128]{}, rename[256]{};
    StableId renaming = 0;
    editor::Transaction renameTransaction;
    bool renameFocus=false;
};
void Outliner(const char *id, const SceneProvider &provider, OutlinerState &state,
              editor::Selection &selection, editor::EventBuffer &events);
struct ComponentView {
    StableId id=0, owner=0;
    const char *label="", *description="";
    bool enabled=true, expanded=false, locked=false;
};
struct ComponentTypeView {StableId id=0;const char *label="";};
struct ComponentStackOptions {
    StableId owner=0;
    std::span<const ComponentTypeView> availableTypes;
    bool locked=false;
};
// Toggle fields: 0 enabled, 1 expanded, 2 locked. Reorder carries owner and offset -1/+1.
// ComponentAdd targets options.owner and carries the chosen type ID in proposed.parent.
void ComponentStack(const char *id, std::span<const ComponentView> components,
                    std::uint64_t revision, editor::EventBuffer &events,
                    const ComponentStackOptions &options = {});
struct UVVertex {
    StableId id = 0, island = 0;
    editor::Point uv{};
    bool selected = false, pinned = false, seam = false, overlap = false;
};
struct UVEdge {
    StableId id = 0;
    editor::Point a{}, b{};
    bool selected = false, seam = false;
    StableId aVertex=0,bVertex=0;
};
struct UVFace {
    StableId id=0,island=0;
    std::span<const UVVertex> vertices;
    bool overlap=false;
};
enum class UVSelection { Vertex, Edge, Face, Island };
enum class UVCoordinates { Normalized, Pixel, Tiles };
struct UVProvider {
    void *user = nullptr;
    std::uint64_t revision = 0;
    std::span<const UVVertex> (*vertices)(void *, editor::Rect) = nullptr;
    std::span<const UVEdge> (*edges)(void *, editor::Rect) = nullptr;
    std::span<const UVFace> (*faces)(void *, editor::Rect) = nullptr;
    std::span<const StableId> (*all)(void *, UVSelection) = nullptr;
    std::span<const UVVertex> (*selected)(void *, std::span<const StableId>, UVSelection) = nullptr;
    std::span<const editor::SelectablePoint> (*selectionQuery)(void *, editor::Rect, UVSelection) = nullptr;
};
struct UVState {
    editor::CanvasState canvas{{-.1, -.1}, {300, 300}};
    editor::Transaction drag;
    editor::Point mouseStart{};
    std::span<editor::Transaction> companionDrags;
    std::size_t companionCount=0;
    UVSelection selection = UVSelection::Vertex;
    bool lassoSelect = false;
    UVCoordinates coordinates = UVCoordinates::Normalized;
    bool checker=true,showTexture=true,grid=true;
    editor::Point imageSize{1,1}; // Host image dimensions for pixel coordinate display.
    std::span<const editor::Binding> bindings;
    TransformTool tool = TransformTool::Translate;
    double snap = 0;
    editor::Point pivot{.5, .5};
    editor::CanvasView view{};
};
editor::Point TransformUV(editor::Point uv, editor::Point pivot, editor::Point translation,
                          double rotationRadians, editor::Point scale, double snap = 0);
void UVEditor(const char *id, const UVProvider &provider, ImTextureRef texture, UVState &state,
              editor::Selection &selection, editor::EventBuffer &events, const Theme &theme,
              ImVec2 size = {0, 240});
struct StripView {
    StableId id = 0, channel = 0;
    const char *label = "";
    editor::Range range{};
    double scale = 1, repeat = 1, blend = 1;
    bool muted = false, locked = false;
};
void AnimationStrips(const char *id, std::span<const StripView> visible, std::uint64_t revision,
                     editor::CanvasState &canvas, editor::Transaction &drag, editor::EventBuffer &events,
                     const Theme &theme, ImVec2 size = {0, 180});
void DopeSheet(const char *id, const editor::CurveProvider &provider, editor::CurveState &state,
               editor::Selection &selection, editor::EventBuffer &events, const Theme &theme,
               ImVec2 size = {0, 200});
} // namespace imkit::cg

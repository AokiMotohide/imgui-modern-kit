#pragma once
#include <imkit/editor_core.h>
#include <imkit/workflow.h>

namespace imkit::editor {
enum class ImageScaleMode { Fit, Fill, ActualSize, Manual };
enum class ImagePlacementMode { Fit, Fill, Stretch };
struct ImagePlacement {
    Rect display{};
    ImVec2 uv0{0,0},uv1{1,1};
    bool valid=false;
};
struct ImageView { ImTextureRef texture{}; Point pixels{}; ImVec2 uv0{0,0},uv1{1,1}; };
struct ImageViewportState {
    CanvasState canvas;
    ImageScaleMode mode=ImageScaleMode::Fit;
    Point previousSize{}, previousPixels{};
    bool reset=true;
    ToolbarState toolbar;
};
struct ImageViewportOptions {
    bool checkerboard=true, clamp=true, disabled=false;
    double minimumZoom=.01, maximumZoom=256;
};
bool ImageGeometryValid(Point pixels,Point viewport);
// Pure local-space layout. Invalid or non-drawable geometry returns valid=false.
ImagePlacement ResolveImagePlacement(Point sourcePixels,Point available,ImagePlacementMode mode);
void FitImage(CanvasState& state,Point pixels,Point viewport,ImageScaleMode mode);
void ClampImage(CanvasState& state,Point pixels,Point viewport);
Point PixelToNormalized(Point pixel,Point pixels);
Point NormalizedToPixel(Point normalized,Point pixels);
// Same always-paired Begin/End contract as BeginCanvas/EndCanvas.
CanvasView BeginImageViewport(const char* id,const ImageView& image,ImageViewportState& state,
                              ImVec2 size,const Theme& theme,ImageViewportOptions options={},ComponentOptions components={});
void EndImageViewport();
void ZoomToolbar(const char* id,ImageViewportState& state,ComponentOptions options={});
enum class OverlayShape { Point, Polyline, Rectangle, Circle, Label };
struct OverlayView {
    OverlayShape shape=OverlayShape::Point;
    std::span<const Point> points{};
    const char* label="";
    double radius=1;
    bool selected=false, hovered=false, closed=false;
};
void DrawOverlay(const CanvasView& view,const CanvasState& state,const OverlayView& overlay,const Theme& theme);
enum class PreviewState { Ready, Loading, Empty, Offline, Error };
struct PreviewTileView {
    StableId id=0;
    ImageView image;
    const char* title="";
    const char* detail="";
    PreviewState status=PreviewState::Ready;
    std::span<const imkit::Command> actions{};
    bool disabled=false;
};
struct TileSize { StableId id=0; float extent=0; };
enum class TileAction { Select, Open, Context, Command, ResizeBegin, ResizeUpdate, ResizeCommit, ResizeCancel };
struct TileEvent { TileAction action{}; StableId tile=0,command=0; float first=0,second=0; };
struct TileEventBuffer {
    std::span<TileEvent> storage{};std::size_t count=0;bool overflow=false;
    bool Push(TileEvent event);
};
struct TileStripState {
    StableId resizing=0, neighbor=0;
    float originalFirst=0,originalSecond=0;
    bool blockUntilRelease=false;
    bool cancelPending=false;
    ToolbarState toolbar;
};
struct TileStripOptions { Orientation orientation=Orientation::Horizontal; float minimum=0; ImVec2 size{}; };
// Host applies selection/actions and owns the ID-keyed dimension storage.
void PreviewTile(const char* id,const PreviewTileView& tile,bool selected,ImVec2 size,
                 TileEventBuffer& events,ComponentOptions options={});
void ResizableTileStrip(const char* id,std::span<const PreviewTileView> tiles,std::span<TileSize> sizes,
                         StableId selected,TileStripState& state,TileEventBuffer& events,
                         TileStripOptions layout={},ComponentOptions options={});
} // namespace imkit::editor

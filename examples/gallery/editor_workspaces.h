#pragma once
#include <imkit/video.h>
#include <imkit/cg.h>
#include <imkit/preview.h>
#include <vector>
#include <array>
#include <map>
#include <string>
namespace imkit::gallery {
struct EditorWorkspaces {
    const IconAtlas *icons = nullptr; // Host-owned renderer resources.
    std::vector<video::TrackView> tracks;
    std::vector<double> trackOffsets;
    std::vector<video::AudioStripView> audioStrips;
    editor::StableId mixerTrack = 0;
    editor::PropertyState mixerState;
    std::vector<video::ClipView> clips;
    std::vector<editor::Keyframe> keys;
    std::array<cg::ObjectView, 4> objects{};
    std::array<editor::AssetView, 8> assets{}, filteredAssets{};
    std::size_t filteredAssetCount=0;
    std::array<editor::StableId,2> assetPathIds{880001,880002};
    int assetPathDepth=2;
    std::array<cg::UVVertex, 4> uv{};
    std::array<cg::UVEdge, 4> edges{};
    std::array<editor::StableId, 1024> selectionStorage{}, objectSelectionStorage{}, keySelectionStorage{},
        uvSelectionStorage{};
    editor::Selection selection{selectionStorage}, objectSelection{objectSelectionStorage},
        keySelection{keySelectionStorage}, uvSelection{uvSelectionStorage};
    std::array<editor::Event, 128> eventStorage{};
    std::array<double, 4> clipPropertyValues{1, 1, 0, 1};
    std::array<editor::Marker, 64> markers{};
    std::size_t markerCount = 0;
    std::array<editor::SnapCandidate, 256> snapCandidates{};
    std::map<editor::StableId, std::string> renamedLabels;
    std::map<editor::StableId, unsigned> propertyFlags;
    std::map<editor::StableId, std::vector<editor::Keyframe>> propertyKeys;
    editor::EventBuffer events{eventStorage};
    std::array<editor::Binding, 32> bindings{};
    std::size_t bindingCount = 0;
    video::TimelineState timeline;
    std::array<video::TimelineState::MemberDrag, 32> clipDrags{};
    std::array<video::ClipView, 64> selectedClips{};
    editor::CurveState curve;
    editor::PropertyState videoProperties, objectProperties;
    editor::AssetState assetState;
    cg::ViewportState viewport;
    cg::Camera sceneCamera{{0,0,0},0,.15,10,.65,5,cg::Projection::Perspective};
    cg::OutlinerState outliner;
    cg::UVState uvState;
    std::array<preview::Vertex, 24> cubeVertices{};
    std::array<std::uint32_t, 36> cubeIndices{};
    std::array<preview::Triangle, 256> scratch{};
    preview::OpenGL3Renderer previewRenderer;
    std::array<video::AudioBucket, 128> audio{};
    std::array<float, 512> pcm{};
    video::MeterState meter;
    video::MeterState rightMeter;
    std::array<std::uint32_t, 256> red{}, green{}, blue{}, luma{};
    std::array<std::uint32_t, 16384> scopeWave{};
    std::array<std::array<std::uint32_t, 16384>,3> scopeRGB{};
    std::array<std::uint32_t, 65536> scopeVector{};
    std::array<video::Rgba, 4096> pixels{};
    video::ColorValues colors{};
    video::ColorState colorState;
    video::ColorPropertyIds colorIds{910011, 910029, 910047, 910063, 910081, 910097};
    int videoPanel = -1, activeVideoPanel = 0;
    std::uint64_t revision = 1;
    std::size_t queryCount = 0, queriedClips = 0, commits = 0;
    bool initialized = false, large = false, japanese = false, narrow = false, useGL = true;
    int animationPage = -1;
    int clipsPerTrack = 12;
    editor::StableId nextId = 2000000;
    ImVec2 timelineOrigin{}, viewportOrigin{}, viewportSize{};
    void Initialize();
    void Dataset(bool largeData);
    void RenderPreview();
    void ApplyEvents();
    void RebuildTrackLayout();
};
void VideoWorkspace(EditorWorkspaces &state, const Theme &theme, ImTextureRef hostTexture);
void CGWorkspace(EditorWorkspaces &state, const Theme &theme, ImTextureRef hostTexture);
void CoreWorkspace(EditorWorkspaces &state, const Theme &theme);
} // namespace imkit::gallery

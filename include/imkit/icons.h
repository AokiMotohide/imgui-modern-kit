#pragma once

#include <imkit/version.h>
#include <array>
#include <cstdint>
#include <optional>
#include <span>

namespace imkit {
// Stable catalogue order. Add new entries before Count; never reorder existing IDs.
enum class IconId : std::uint16_t {
    Add,
    Delete,
    Close,
    Check,
    Edit,
    Search,
    Settings,
    More,
    Refresh,
    Reset,
    ArrowUp,
    ArrowDown,
    ArrowLeft,
    ArrowRight,
    Back,
    Forward,
    Home,
    Menu,
    ExternalLink,
    Logout,
    FileNew,
    File,
    Folder,
    FolderOpen,
    Save,
    SaveAs,
    Import,
    Export,
    Download,
    Upload,
    Undo,
    Redo,
    Cut,
    Copy,
    Paste,
    Duplicate,
    SelectAll,
    Deselect,
    Lock,
    Unlock,
    Eye,
    EyeOff,
    ZoomIn,
    ZoomOut,
    FitView,
    ActualSize,
    Fullscreen,
    ExitFullscreen,
    Grid,
    List,
    SidebarLeft,
    SidebarRight,
    PanelBottom,
    SplitHorizontal,
    SplitVertical,
    AlignLeft,
    AlignCenter,
    AlignRight,
    AlignTop,
    AlignBottom,
    Play,
    Pause,
    Stop,
    Record,
    PreviousTrack,
    NextTrack,
    Repeat,
    Shuffle,
    Volume,
    Mute,
    Image,
    Video,
    Audio,
    Camera,
    Microphone,
    Text,
    Code,
    Link,
    Attachment,
    Palette,
    Info,
    Help,
    Warning,
    Error,
    Success,
    Loading,
    Bell,
    BellOff,
    Connected,
    Disconnected,
    Filter,
    SortAscending,
    SortDescending,
    Tag,
    Pin,
    PinOff,
    Star,
    Heart,
    History,
    Calendar,
    User,
    Users,
    UserAdd,
    Share,
    Comment,
    Send,
    Inbox,
    Cloud,
    Sync,
    Key,
    Layers,
    LayerAdd,
    Cube,
    Move,
    Rotate,
    Scale,
    Monitor,
    Projector,
    Ruler,
    Target,
    Keyframe,
    PreviousFrame,
    NextFrame,
    Magnet,
    SelectPointer,
    BoxSelect,
    LassoSelect,
    HandPan,
    Razor,
    RippleEdit,
    SlipEdit,
    RollingEdit,
    SlideEdit,
    Solo,
    SourcePatch,
    EffectTrack,
    AdjustmentTrack,
    VertexNormals,
    FaceNormals,
    ObjectOrigin,
    SelectionOutline,
    RenderRegion,
    Passepartout,
    UnifiedTransform,
    SelectVertex,
    SelectEdge,
    SelectFace,
    SelectIsland,
    UVEditor,
    UVSeam,
    UVOverlap,
    UDIMTiles,
    Transition,
    Dissolve,
    FadeIn,
    FadeOut,
    Crossfade,
    TransitionDuration,
    Count
};
struct IconInfo {
    IconId id;
    const char *name;
    const char *category;
};
inline constexpr std::array<int, 6> IconPixelSizes{16, 20, 24, 32, 48, 64};
std::span<const IconInfo> GetIconCatalog();
const IconInfo *GetIconInfo(IconId id); // nullptr for invalid IDs.
struct IconAtlasPixels {
    int width = 0, height = 0, iconPixels = 0;
    std::span<const unsigned char> rgba; // Straight alpha; process lifetime; read-only.
};
// Exact supported pixel size required. Unsupported size returns an empty view.
IconAtlasPixels GetIconAtlasPixels(int iconPixels);
struct IconRegion {
    ImVec2 uv0{}, uv1{};
};
IconRegion GetIconRegion(IconId id, int iconPixels);

// Host-owned, one instance per renderer/resource lifetime. No global texture binding.
// Upload all six atlases as RGBA8, linear filtering, clamp-to-edge, no mipmaps.
struct IconAtlas {
    std::array<ImTextureRef, 6> textures{};
    bool SetTexture(int iconPixels, ImTextureRef texture);
    void Clear();
};
struct IconOptions {
    float size = 20; // Logical pixels. Raster level follows DisplayFramebufferScale.
    std::optional<ImVec4> color{}; // Absent: current ImGuiCol_Text. Style alpha applies once.
};
// Missing texture/invalid ID: Icon reserves space; buttons are disabled.
void Icon(const IconAtlas &atlas, IconId icon, IconOptions options = {});
bool IconButton(const char *id, const IconAtlas &atlas, IconId icon,
                const char *accessibleLabel, IconOptions options = {});
bool IconLabelButton(const char *id, const IconAtlas &atlas, IconId icon,
                     const char *label, IconOptions options = {});
} // namespace imkit

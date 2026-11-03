#include <imkit/node_editor.h>
#include <type_traits>
namespace ne = imkit::node_editor;
static_assert(!std::is_convertible_v<ne::NodeId, ne::PinId>);
static_assert(!std::is_convertible_v<ne::PinId, ne::LinkId>);
static_assert(std::is_copy_constructible_v<ne::ViewState>);
// Separate public consumer: explicit signatures also force link resolution.
ne::LayoutResult (*volatile align)(ne::GraphView, std::span<const ne::NodeId>, std::span<ne::PositionChange>,
                                   ne::AlignOptions) = &ne::AlignNodes;
ne::EditorFrame (*volatile begin)(const char *, ne::GraphView, ne::EditorState &, ne::RequestBuffer &,
                                  const ne::NodeStyle &, ne::EditorOptions) = &ne::BeginEditor;
void (*volatile preview)(ne::EditorFrame &, std::span<const ne::PreviewOutput>,
                         void (*)(void *, const ne::PreviewDemand &), void *) = &ne::Preview;
bool (*volatile pinEdits)(ne::GraphView, std::span<const ne::EditRequest>, ne::RequestBuffer &,
                          std::uint64_t) = &ne::QueuePinEdits;
void (*volatile pinRow)(ne::EditorFrame &, ne::PinId, ne::PinRowOptions) = &ne::PinRow;
ne::ConnectionVerdict (*volatile validate)(ne::GraphView, const ne::EditRequest &) = &ne::ValidatePinEdit;
// Appearance types share their enums with the theme file.
static_assert(std::is_same_v<ne::HeaderFill, imkit::NodeHeaderFill>);
static_assert(std::is_same_v<ne::LinkColorSource, imkit::NodeLinkColor>);
static_assert(std::is_same_v<ne::LinkArrow, imkit::NodeLinkArrow>);
int main() {
    static const ne::NodeCategoryStyle categories[] = {{7, {.4f, .6f, 1, 1}, "M"}};
    ne::NodeStyle look;
    look.headerFill = ne::HeaderFill::Tinted;
    look.linkColor = ne::LinkColorSource::Gradient;
    look.linkArrow = ne::LinkArrow::None;
    look.status = ne::StatusDisplay::Never;
    look.categories = categories;
    ne::EditorOptions options;
    options.locale = nullptr;
    ne::EditorState state;
    state.Reserve(16);
    ne::NodeId ids[] = {{1}};
    ne::NodeView nodes[] = {{ids[0], {0, 0}, {240, 240}, "Example"}};
    nodes[0].category = 7;
    nodes[0].subtitle = "Math";
    (void)look;
    (void)options;
    ne::GraphView graph;
    graph.nodes = nodes;
    ne::PositionChange changes[1];
    auto r = ne::SnapNodesToGrid(graph, ids, changes);
    return r ? 0 : 1;
}

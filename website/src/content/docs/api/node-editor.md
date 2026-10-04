---
title: "Node Editor API一覧"
---

この生成一覧は[`docs/node-editor-api.json`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/docs/node-editor-api.json)にある`<imkit/node_editor.h>`の**47宣言**を掲載します。型と正確な契約はheaderを基準にしてください。v3.1.0の開発中APIであり、グラフデータ、検証、取り消し、保存はアプリ側が管理します。

以下は宣言一覧で、単独で使うコード例ではありません。`BeginEditor`の呼び出し、所有権、戻り値、対応する終了呼び出しは[BeginEditor詳細](begin-editor/)を参照してください。[Node Editorガイド](../../features/node-editor/)に描画順を示しています。

## スタイルと接続

```cpp
NodeStyle MakeNodeStyle(const Theme &theme);
ConnectionVerdict CanConnect(GraphView graph, PinId a, PinId b, LinkId replacing = {});
ConnectionVerdict ValidatePinEdit(GraphView graph, const EditRequest &request);
bool IsPinConnected(GraphView graph, PinId pin);
```

## グラフの配置

```cpp
LayoutResult AlignNodes(GraphView graph, std::span<const NodeId> selection, std::span<PositionChange> output, AlignOptions options = {});
LayoutResult DistributeNodes(GraphView graph, std::span<const NodeId> selection, std::span<PositionChange> output, Axis axis = Axis::Horizontal, Distribution distribution = Distribution::Gaps);
LayoutResult ArrangeNodes(GraphView graph, std::span<const NodeId> selection, std::span<PositionChange> output, ArrangeOptions options = {});
LayoutResult SnapNodesToGrid(GraphView graph, std::span<const NodeId> selection, std::span<PositionChange> output, double spacing = 24);
bool FitGroupToContents(GraphView graph, NodeId group, Rect &bounds, double padding = 24);
bool QueueLayout(GraphView graph, std::span<const PositionChange> changes, RequestBuffer &output, std::uint64_t operation);
LayoutResult TraceNodes(GraphView graph, NodeId start, bool upstream, std::span<NodeId> output);
```

## 表示と選択状態

```cpp
ViewState CaptureView(const EditorState &state);
void RestoreView(EditorState &state, const ViewState &view);
void RememberView(EditorState &state);
bool NavigateHistory(EditorState &state, int direction);
bool AddBookmark(EditorState &state, std::string_view label);
void SetZoom(EditorState &state, double zoom, Point anchorInViewport = {});
bool FrameNodes(EditorState &state, GraphView graph, std::span<const NodeId> nodes, Point viewportSize, double padding = 40);
bool IsSelected(const EditorState &state, NodeId node);
void Select(EditorState &state, NodeId node, bool additive = false, bool toggle = false);
```

## 編集領域とnode描画

```cpp
EditorFrame BeginEditor(const char *id, GraphView graph, EditorState &state, RequestBuffer &requests, const NodeStyle &style, EditorOptions options = {});
void EndEditor(EditorFrame &frame);
bool BeginNode(EditorFrame &frame, NodeId node);
void EndNode(EditorFrame &frame);
void BeginPin(EditorFrame &frame, PinId pin);
void EndPin(EditorFrame &frame);
void PinRow(EditorFrame &frame, PinId pin, PinRowOptions options = {});
void PinAddRow(EditorFrame &frame, NodeId node, PinKind kind, std::uint64_t group = 0);
bool GetPinPosition(const EditorFrame &frame, PinId pin, ImVec2 &screenPosition);
void Link(EditorFrame &frame, const LinkView &link);
void DrawLinks(EditorFrame &frame);
void DrawNodes(EditorFrame &frame, void (*body)(void *, EditorFrame &, const NodeView &) = nullptr, void *user = nullptr);
bool QueueCommand(EditorFrame &frame, EditKind kind, NodeId node = {});
bool QueuePinEdits(GraphView graph, std::span<const EditRequest> edits, RequestBuffer &output, std::uint64_t operation);
bool InsertNode(EditorFrame &frame, NodeId node, LinkId link);
```

## パネルとプレビュー

```cpp
void NodePalette(EditorFrame &frame, std::span<const PaletteEntry> entries);
void MiniMap(EditorFrame &frame, ImVec2 size = {180, 120});
void Diagnostics(EditorFrame &frame);
void LayoutToolbar(EditorFrame &frame);
void NodeSearch(EditorFrame &frame);
void NodeInspector(EditorFrame &frame, NodeId node);
void ExposedProperties(EditorFrame &frame, NodeId node, std::span<const PropertyView> properties);
void BackgroundImage(EditorFrame &frame, ImTextureRef texture, Rect bounds, ImVec4 tint = {1, 1, 1, 1});
void Annotation(EditorFrame &frame, std::span<const Point> points, ImVec4 color, float width = 2);
void Breadcrumbs(EditorFrame &frame, std::span<const PathEntry> path);
void Preview(EditorFrame &frame, std::span<const PreviewOutput> outputs, void (*demand)(void *, const PreviewDemand &) = nullptr, void *user = nullptr);
void DrawDetachedPreviews(EditorFrame &frame, NodeId node, std::span<const PreviewOutput> outputs, void (*demand)(void *, const PreviewDemand &) = nullptr, void *user = nullptr);
```

## 関連ページ

- [Node Editorの機能ガイド](../../features/node-editor/)
- [導入ガイド](../../getting-started/)
- [公開header](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/include/imkit/node_editor.h)

---
title: "Build a node editor"
description: "The Node Editor is ImKit's graph canvas. It is an optional target, imkit::nodeeditor."
---

The Node Editor is ImKit's graph canvas. It is an optional target, `imkit::node_editor`. Each frame you give it a snapshot of your graph, and it returns the edits the user made as requests.

The library has no evaluation engine, renderer, file access or global "current editor". You own the graph and apply the edits.

This tutorial builds a graph model, draws it, and applies move, rename, create, delete, link and Undo/Redo requests.

![The Node Editor Gallery: nodes with dynamic sockets and links](/imgui-modern-kit/docs-images/v4-node-editor-poster.png)

[Open native GIF](https://raw.githubusercontent.com/AokiMotohide/imgui-modern-kit/main/docs/images/v4-node-editor.gif)

## The pattern

Each frame has three steps:

1. **Snapshot.** Build an `ne::GraphView` from your data and stamp it with the current `revision`.
2. **Draw.** Call `BeginEditor`, then `DrawLinks` and `DrawNodes`, then `EndEditor`.
3. **Apply.** Pass the returned `EditRequest` values to your model's `Apply`.

ImKit never changes your graph. A request is a value you inspect, and only a request built from the current snapshot may be applied.

## Before you start

- A Dear ImGui context and a window (your application or the Gallery).
- `imkit::node_editor` linked.
- C++20, because the code uses `std::span` and `std::erase_if`.

## Step 1: Keep the graph in your own model

The model owns the nodes, pins, links and the Undo history. Titles are `std::string` in the model, and the snapshot borrows them.

```cpp
#include <imgui.h>
#include <imkit/imkit.h>
#include <imkit/node_editor.h>
#include <algorithm>
#include <array>
#include <span>
#include <string>
#include <vector>

namespace ne = imkit::node_editor;

// Everything in this struct belongs to your application.
struct GraphModel {
    struct Node {
        ne::NodeId id;
        std::string title;
        ne::Point position;
        ne::Point size{240, 160};
    };
    struct Snapshot {
        std::vector<Node> nodes;
        std::vector<ne::PinView> pins;
        std::vector<ne::LinkView> links;
    };

    Snapshot data;
    std::vector<Snapshot> undo, redo;
    std::vector<ne::NodeView> nodeViews;   // rebuilt every frame
    ne::GraphId graph{1};
    std::uint64_t revision = 1;            // increase it whenever the data changes
    std::uint64_t nextId = 100;

    ne::NodeId AddNode(ne::Point position, std::string title) {
        const ne::NodeId id{nextId++};
        data.nodes.push_back({id, std::move(title), position});

        ne::PinView in;
        in.id = {nextId++};
        in.node = id;
        in.label = "In";
        in.kind = ne::PinKind::Input;
        in.side = ne::Side::Left;

        ne::PinView out;
        out.id = {nextId++};
        out.node = id;
        out.label = "Out";
        out.kind = ne::PinKind::Output;
        out.side = ne::Side::Right;

        data.pins.push_back(in);
        data.pins.push_back(out);
        return id;
    }

    Node* Find(ne::NodeId id) {
        for (Node& n : data.nodes)
            if (n.id == id) return &n;
        return nullptr;
    }

    // The snapshot borrows from this model and is valid for one frame.
    ne::GraphView View() {
        nodeViews.clear();
        for (const Node& n : data.nodes) {
            ne::NodeView v;
            v.id = n.id;
            v.position = n.position;
            v.size = n.size;
            v.title = n.title;
            nodeViews.push_back(v);
        }
        return {graph, revision, nodeViews, data.pins, data.links};
    }
    // RemoveNode() and Apply() are added in step 4.
};
```

`AddNode` gives each node an input and an output pin. A node's `Node` record is yours: add whatever fields your application needs.

## Step 2: Build the snapshot

`View()` returns a `GraphView` that borrows the model's vectors. It is valid for one frame. The `revision` you stamp on it is what makes a request applicable: the editor copies it into every request, and `Apply` drops any request that does not match.

The `View()` member in the code above builds the snapshot. Change `revision` only when the data changes.

## Step 3: Draw the editor

Keep one `GraphEditor` across frames, and call `DrawGraph` between `NewFrame()` and `Render()`.

```cpp
struct GraphEditor {
    ne::EditorState state;                          // pan, zoom and selection
    std::array<ne::EditRequest, 256> storage;       // bounded request storage
};

void DrawGraph(GraphModel& model, GraphEditor& editor, const imkit::Theme& theme) {
    ne::RequestBuffer requests{editor.storage};

    const ne::GraphView graph = model.View();
    const ne::NodeStyle style = ne::MakeNodeStyle(theme);
    ne::EditorOptions options;
    options.size = ImGui::GetContentRegionAvail();

    ne::EditorFrame frame = ne::BeginEditor("graph", graph, editor.state, requests, style, options);
    ne::DrawLinks(frame);
    ne::DrawNodes(frame);

    const std::array<ne::PaletteEntry, 1> palette{{{1, "Processing", "Nodes", "A generic node"}}};
    ne::NodePalette(frame, palette);
    ne::EndEditor(frame);                           // always, even when frame.visible is false

    model.Apply(requests.Requests());
}
```

- `EditorState` holds pan, zoom and selection. It is UI state, not graph data.
- `RequestBuffer` is bounded. Give it storage that outlives the frame.
- `DrawNodes(frame)` draws standard nodes. To put your own widgets inside a node, pass a callback as the second argument, and call `ne::PinRow` for each pin. [Node Editor](../../features/node-editor/) shows this.
- `NodePalette` lists the node types the user can create with Tab. A create request carries the entry's `type`.
- Call `EndEditor` always, even when `frame.visible` is false.

## Step 4: Apply the requests

Add these two members to `GraphModel`. `Apply` is the only place that changes the graph.

```cpp
    void RemoveNode(ne::NodeId id) {
        std::vector<ne::PinId> pins;
        for (const ne::PinView& p : data.pins)
            if (p.node == id) pins.push_back(p.id);
        auto touches = [&](ne::PinId pin) { return std::find(pins.begin(), pins.end(), pin) != pins.end(); };
        std::erase_if(data.links, [&](const ne::LinkView& l) { return touches(l.from) || touches(l.to); });
        std::erase_if(data.pins, [&](const ne::PinView& p) { return p.node == id; });
        std::erase_if(data.nodes, [&](const Node& n) { return n.id == id; });
    }

    // Applies the requests of one frame. Returns without effect for stale input.
    void Apply(std::span<const ne::EditRequest> all) {
        const std::uint64_t frameRevision = revision;
        bool modified = false;
        for (std::size_t i = 0; i < all.size();) {
            const std::size_t count = all[i].operationSize;       // one gesture = one batch
            if (count == 0 || i + count > all.size()) break;      // incomplete batch
            const auto batch = all.subspan(i, count);
            i += count;

            const ne::EditRequest& first = batch.front();
            if (first.phase != ne::Phase::Commit || first.graph != graph || first.revision != frameRevision)
                continue;                                         // preview, or built from an older snapshot

            if (first.kind == ne::EditKind::Undo) {
                if (!undo.empty()) {
                    redo.push_back(std::move(data));
                    data = std::move(undo.back());
                    undo.pop_back();
                    modified = true;
                }
                continue;
            }
            if (first.kind == ne::EditKind::Redo) {
                if (!redo.empty()) {
                    undo.push_back(std::move(data));
                    data = std::move(redo.back());
                    redo.pop_back();
                    modified = true;
                }
                continue;
            }

            Snapshot before = data;
            bool changed = false;
            for (const ne::EditRequest& r : batch) {
                switch (r.kind) {
                case ne::EditKind::Move:
                    if (Node* n = Find(r.node)) { n->position = r.after; changed = true; }
                    break;
                case ne::EditKind::Rename:
                    if (Node* n = Find(r.node)) { n->title = r.text.data(); changed = true; }
                    break;
                case ne::EditKind::CreateNode:
                    AddNode(r.after, "New node");
                    changed = true;
                    break;
                case ne::EditKind::DeleteNode:
                    RemoveNode(r.node);
                    changed = true;
                    break;
                case ne::EditKind::CreateLink: {
                    ne::LinkView link;
                    link.id = {nextId++};
                    link.from = r.from;
                    link.to = r.to;
                    data.links.push_back(link);
                    changed = true;
                    break;
                }
                case ne::EditKind::DeleteLink:
                    std::erase_if(data.links, [&](const ne::LinkView& l) { return l.id == r.link; });
                    changed = true;
                    break;
                default:
                    break;                                        // other kinds: handle the ones you support
                }
            }
            if (changed) {
                undo.push_back(std::move(before));
                redo.clear();
                modified = true;
            }
        }
        if (modified) ++revision;
    }
```

Rules `Apply` follows:

| Rule | Reason |
| --- | --- |
| Skip requests whose `phase` is not `Commit` | `Begin` and `Update` are previews of a drag in progress |
| Skip requests whose `graph` or `revision` differs from the snapshot | They were built from older data |
| Walk the requests in batches of `operationSize` | A gesture, such as moving several nodes, is one batch and is one Undo step |
| Save the previous state before changing it | That state is what Undo restores |
| Increase `revision` after any change | The next snapshot then differs from the one the user saw |

## Step 5: Check the logic without a window

`Apply` is plain C++ and needs no Dear ImGui. Feed it requests directly:

```cpp
ne::EditRequest move;
move.kind = ne::EditKind::Move;
move.graph = model.graph;
move.revision = model.revision;   // built for the current snapshot
move.node = a;
move.after = {90, 100};
model.Apply({&move, 1});        // the node moves, revision becomes 2

model.Apply({&move, 1});        // stale: ignored, nothing changes
```

The Node Editor Gallery (`examples/node_editor/main.cpp`) is a complete application with a palette, previews, groups and subgraphs. Build the `imkit_node_editor_gallery` target to run it.

## Rules

- Revision-check every request, and drop stale ones.
- Borrow, do not store. `GraphView` fields, titles and pin labels are valid for the frame only.
- Always call `EndEditor`. `BeginEditor` and `EndEditor` are asserted to be balanced.
- The editor never evaluates, saves or runs your graph.

## Next

- [Node Editor](../../features/node-editor/): the full integration reference.
- [Editor API](../../api/editor-suite/)
- [Build a settings screen](../settings-screen/)

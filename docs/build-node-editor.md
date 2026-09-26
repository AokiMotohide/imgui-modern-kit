# Build a node editor

[日本語](build-node-editor.ja.md) · [Documentation index](README.md) · [How ImKit works](how-it-works.md)

The Node Editor is ImKit's independent graph UI. It is an optional target (`imkit::node_editor`) that renders the *snapshot* you supply each frame and collects *edit requests* you choose to apply. There is no evaluation engine, renderer, filesystem, clipboard, or "current editor" singleton inside the library. You own the graph; ImKit draws it and hands back intent.

![Node editor with dynamic sockets and links](images/v3-node-editor.gif)

## The core pattern

Each frame the relationship between you and ImKit is exactly three steps:

1. **Snapshot in.** Build a `ne::GraphView` from your host-owned data (nodes, pins, links), stamping the current `revision`.
2. **Draw.** Call `BeginEditor`, then `DrawLinks` / `DrawNodes` / `NodePalette`, then `EndEditor`.
3. **Apply out.** After `ImGui::Render()`, call your model's `Apply` on the returned `EditRequest`s.

ImKit never mutates your graph and never acts on input by itself. An `EditRequest` is a value you inspect; only requests that match the frame's snapshot are committed.

## What you need

1. A Dear ImGui context and a window (your own GLFW/Metal app, or the Gallery).
2. The `imkit::node_editor` target linked into your app.
3. C++20 — the API uses `std::span`, `std::vector`, and `std::erase_if`.

## What you'll build

- A host-owned graph model: nodes, pins, links, and an undo/redo history.
- A per-frame `GraphView` snapshot built from that model.
- The editor surface: canvas, links, node bodies, a palette.
- An `Apply` step that commits moves, resizes, renames, node creation/deletion, and Undo/Redo.

## The model (host-owned data)

Everything you display lives here. ImKit only reads what you put into `GraphView`; it writes nothing back to these vectors.

```cpp
namespace ne = imkit::node_editor;

// Host-owned data. ImKit reads View() and never writes here.
struct Snap {
    std::vector<ne::NodeView> nodes;
    std::vector<ne::PinView> pins;
    std::vector<ne::LinkView> links;
};

struct Model {
    Snap data;
    std::vector<Snap> undo, redo;
    ne::GraphId graph{1};
    std::uint64_t revision = 1, next = 100;

    ne::NodeId Add(ne::Point pos, const char *title) {
        ne::NodeId id{next++};
        ne::NodeView v;
        v.id = id;
        v.position = pos;
        v.size = {240, 240};
        v.title = title;
        v.kind = ne::NodeKind::Node;
        data.nodes.push_back(v);
        ne::PinView in{}, out{};
        in.id  = {next++}; in.node  = id; in.label = "In";  in.kind = ne::PinKind::Input;  in.side = ne::Side::Left;
        out.id = {next++}; out.node = id; out.label = "Out"; out.kind = ne::PinKind::Output; out.side = ne::Side::Right;
        data.pins.push_back(in);
        data.pins.push_back(out);
        return id;
    }

    ne::NodeView *Find(ne::NodeId id) {
        for (auto &v : data.nodes)
            if (v.id == id)
                return &v;
        return nullptr;
    }

    ne::GraphView View() const {
        // Frame-local buffers must outlive BeginEditor...EndEditor.
        // In a real app own these as model members.
        static thread_local std::array<ne::NodeView, 64> nodes{};
        static thread_local std::array<ne::PinView, 256> pins{};
        static thread_local std::array<ne::LinkView, 256> links{};
        std::size_t n = 0, p = 0, l = 0;
        for (auto &v : data.nodes) nodes[n++] = v;
        for (auto &pv : data.pins)  pins[p++] = pv;
        for (auto &l : data.links)  links[l++] = l;
        ne::GraphView g;
        g.id = graph;
        g.revision = revision;   // stamped once per frame
        g.nodes = {nodes.data(), n};
        g.pins = {pins.data(), p};
        g.links = {links.data(), l};
        return g;
    }
};
```

`GraphView` borrows data for the duration of one frame. The `revision` you stamp is what makes a request applicable: the editor tags each queued request with it, and `Apply` below drops anything that does not match the current frame's snapshot.

## The per-frame draw

Inside your existing Dear ImGui frame loop (after `NewFrame`):

```cpp
auto graph = model.View();

ne::EditorOptions opts;
opts.size = canvasSize;      // ImVec2 from your layout
opts.grid = true;

ne::EditorState state;       // pan/zoom/selection; call state.Reserve(n) once
ne::RequestBuffer req{requests};   // requests = std::array<ne::EditRequest, 4096>
auto style = ne::MakeNodeStyle(theme);

auto f = ne::BeginEditor("graph", graph, state, req, style, opts);
ne::DrawLinks(f);
ne::DrawNodes(
    f,
    [](void *user, ne::EditorFrame &f, const ne::NodeView &node) {
        auto *model = static_cast<Model *>(user);
        auto *m = model->Find(node.id);   // your host-side record
        if (!m) return;
        ImGui::SliderFloat("##value", &m->progress, 0.f, 1.f);   // node content
        for (auto &pin : f.graph.pins)
            if (pin.node == node.id)
                ne::PinRow(f, pin.id);     // per-socket row
    },
    &model);
ne::NodePalette(f, palette);   // std::vector<ne::PaletteEntry> you build
ne::EndEditor(f);              // always, even when f.visible is false

ImGui::Render();
model.Apply(req.Requests());    // commit this frame's requests, after Render()
```

A `PaletteEntry` is `ne::PaletteEntry{type, title, category, description, user, compatible}`. For richer per-node content (groups, frames, previews), the callback can call `BeginNode`/`EndNode`, `BeginPin`/`EndPin`, and `Preview`; the full reference is [node-editor.md](node-editor.md).

## Applying the requests

`Apply` must:

- Ignore any request whose `phase != ne::Phase::Commit`, whose `graph` differs, or whose `revision` is stale (a newer snapshot superseded it).
- Apply each committed request to your data (move, resize, rename, create, delete).
- Push the pre-change state onto `undo`, clear `redo`, and handle `Undo`/`Redo` by moving snapshots between the two lists.
- Increment `revision` when anything changed, so the next frame stamps a new value.

A minimal correct `Apply`:

```cpp
void Model::Apply(std::span<const ne::EditRequest> requests) {
    const auto frameRevision = revision;   // what View() stamped this frame
    bool modified = false;
    for (auto &r : requests) {
        if (r.phase != ne::Phase::Commit || r.graph != graph || r.revision != frameRevision)
            continue;                      // stale request: drop, do not apply
        switch (r.kind) {
        case ne::EditKind::Move: {
            if (auto *n = Find(r.node); n && !n->locked) { n->position = r.after; modified = true; }
            break; }
        case ne::EditKind::Resize: {
            if (auto *n = Find(r.node)) { n->size = r.after; modified = true; }
            break; }
        case ne::EditKind::Rename: {
            if (auto *n = Find(r.node)) { n->title = r.text.data(); modified = true; }
            break; }
        case ne::EditKind::CreateNode: {
            Add(r.after, "New node");
            modified = true;
            break; }
        case ne::EditKind::DeleteNode: {
            std::vector<ne::PinId> gone;
            for (auto &p : data.pins)
                if (p.node == r.node) gone.push_back(p.id);
            std::erase_if(data.links, [&](const ne::LinkView &l) {
                return std::find(gone.begin(), gone.end(), l.from) != gone.end() ||
                       std::find(gone.begin(), gone.end(), l.to) != gone.end(); });
            std::erase_if(data.pins,  [&](const ne::PinView &p) {
                return std::find(gone.begin(), gone.end(), p.id) != gone.end(); });
            std::erase_if(data.nodes, [&](const ne::NodeView &v) { return v.id == r.node; });
            modified = true;
            break; }
        case ne::EditKind::Undo: {
            if (!undo.empty()) { redo.push_back(std::move(data)); data = std::move(undo.back()); undo.pop_back(); modified = true; }
            break; }
        case ne::EditKind::Redo: {
            if (!redo.empty()) { undo.push_back(std::move(data)); data = std::move(redo.back()); redo.pop_back(); modified = true; }
            break; }
        default:
            break;
        }
    }
    if (modified) ++revision;
}
```

## How to run it

- The full companion app is `examples/node_editor/main.cpp` — a self-contained GLFW/OpenGL (or Metal) window that owns a real model, palette, and previews. Add it as a CMake executable linking `imkit::node_editor`.
- The main Gallery also hosts a Node Editor page; build the `imkit_gallery` target and choose that route to see the same pattern at scale.

## What just happened

- Your `Model` owned the graph and the undo history; ImKit never touched it directly.
- Each frame stamped a `GraphView` with the current `revision`.
- The editor returned requests; `Apply` committed only the ones matching the frame's revision.
- `revision` incremented, so the next snapshot reflects the change.

## The rules that keep it safe

- **Revision-gate every request.** A stale request from an older snapshot must be dropped, not applied.
- **Borrow, don't store.** `GraphView` fields, node titles, and pin labels are valid only for the frame.
- **Apply after `Render()`.** The editor queues requests across frames; read them only after `ImGui::Render()`.
- **`EndEditor` is unconditional.** Even when the editor is hidden or collapsed, call it.
- **Keep `BeginEditor`/`EndEditor` balanced.** ImKit asserts on mismatch.

## Next

- [Node Editor integration (full reference)](node-editor.md)
- [Editor API reference](editor-api.md)
- [Build a settings screen](build-settings-screen.md)

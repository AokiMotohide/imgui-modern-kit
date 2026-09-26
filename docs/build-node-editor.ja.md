# ノードエディターの作り方

[English](build-node-editor.md) · [文書索引](README.md) · [ImKit の仕組み](how-it-works.md)

ノードエディターは ImKit の独立的な graph UI です。これは任意のターゲット（`imkit::node_editor`）で、フレーム毎にあなたが渡す**スナップショット**をレンダリングし、あなたが適用する判断する**編集リクエスト**を集めます。ライブラリ内には評価エンジン、レンダラー、ファイルシステム、クリップボード、「現在エディター」シングルトンは一切ありません。グラフはあなたが所有し、ImKit は描画し、意図を返し渡すだけです。

![動的ソケットとリンクを持つノードエディター](images/v3-node-editor.gif)

## コアパターン

フレーム毎の、あなたと ImKit の関係はちょうど 3 ステップです：

1. **スナップショット投入。** あなたのホスト所有データ（ノード、ピン、リンク）から `ne::GraphView` を作り、現在の `revision` をスタンプする。
2. **描画。** `BeginEditor` を呼び、`DrawLinks` / `DrawNodes` / `NodePalette` を呼び、`EndEditor` を呼ぶ。
3. **リクエスト適用。** `ImGui::Render()` の後、返された `EditRequest` に対し、あなたのモデルの `Apply` を呼ぶ。

ImKit はあなたのグラフを何としても変えず、入力に自ら動くこともありません。`EditRequest` はあなたが検討する値であり、現在のフレームのスナップショットに一致したリクエストだけがコミットされます。

## 必要なもの

1. Dear ImGui コンテキストとウィンドウ（あなたの GLFW/Metal アプリ、または Gallery）。
2. アプリに `imkit::node_editor` ターゲットをリンクする。
3. C++20 — API は `std::span`、`std::vector`、`std::erase_if` を使います。

## 作るもの

- ホスト所有のグラフモデル：ノード、ピン、リンク、Undo/Redo 履歴。
- そのモデルから毎フレーム構築される `GraphView` スナップショット。
- エディター表面：キャンバス、リンク、ノード本体、パレット。
- 移動・リサイズ・リネーム・ノード生成/削除・Undo/Redo をコミットする `Apply` 処理。

## モデル（ホスト所有データ）

表示するものはすべてここで持たれます。ImKit は `View()` にあなたが置いたものを読み、これらベクターに書き戻すことはありません。

```cpp
namespace ne = imkit::node_editor;

// ホスト所有データ。ImKit は View() を読むだけで、こちらには書き戻さない。
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
        // フレーム局所バッファは BeginEditor...EndEditor まで生存しなければならない。
        // 実アプリではモデルメンバーとして所有するのがよい。
        static thread_local std::array<ne::NodeView, 64> nodes{};
        static thread_local std::array<ne::PinView, 256> pins{};
        static thread_local std::array<ne::LinkView, 256> links{};
        std::size_t n = 0, p = 0, l = 0;
        for (auto &v : data.nodes) nodes[n++] = v;
        for (auto &pv : data.pins)  pins[p++] = pv;
        for (auto &l : data.links)  links[l++] = l;
        ne::GraphView g;
        g.id = graph;
        g.revision = revision;   // フレーム毎に一度スタンプ
        g.nodes = {nodes.data(), n};
        g.pins = {pins.data(), p};
        g.links = {links.data(), l};
        return g;
    }
};
```

`GraphView` は 1 フレーム分のデータを借りている。スタンプした `revision` はリクエストが適用される条件になります：エディターはキューする各リクエストにその値を付ke、`Apply` は現在のフレームのスナップショットに一致しないものは破棄します。

## フレーム毎の描画

既存の Dear ImGui フレームループ（`NewFrame` の後）で：

```cpp
auto graph = model.View();

ne::EditorOptions opts;
opts.size = canvasSize;      // レイアウト由来の ImVec2
opts.grid = true;

ne::EditorState state;       // パン/ズーム/選択。一度だけ state.Reserve(n)
ne::RequestBuffer req{requests};   // requests = std::array<ne::EditRequest, 4096>
auto style = ne::MakeNodeStyle(theme);

auto f = ne::BeginEditor("graph", graph, state, req, style, opts);
ne::DrawLinks(f);
ne::DrawNodes(
    f,
    [](void *user, ne::EditorFrame &f, const ne::NodeView &node) {
        auto *model = static_cast<Model *>(user);
        auto *m = model->Find(node.id);   // あなたのホスト側レコード
        if (!m) return;
        ImGui::SliderFloat("##value", &m->progress, 0.f, 1.f);   // ノード内容
        for (auto &pin : f.graph.pins)
            if (pin.node == node.id)
                ne::PinRow(f, pin.id);     // ソケット毎の行
    },
    &model);
ne::NodePalette(f, palette);   // あなたが構築する std::vector<ne::PaletteEntry>
ne::EndEditor(f);              // f.visible が false でも必ず呼ぶ

ImGui::Render();
model.Apply(req.Requests());    // Render() の後、このフレームのリクエストをコミット
```

`PaletteEntry` は `ne::PaletteEntry{type, title, category, description, user, compatible}` です。より詳細なノード内容（グループ、フレーム、プレビュー）にはコールバック内で `BeginNode`/`EndNode`、`BeginPin`/`EndPin`、`Preview` を使えます。完全な参照は [node-editor.md](node-editor.md) を参照。

## リクエストの適用

`Apply` は以下をします：

- `phase != ne::Phase::Commit`、`graph` が異なる、または `revision` が古い（新しいスナップショットに上書きされた）リクエストは無視する。
- コミットされた各リクエストをデータに適用する（移動、リサイズ、リネーム、生成、削除）。
- 変更前に `undo` に状態を push し `redo` をクリアし、`Undo`/`Redo` は 2 リスト間のスナップショット移動で処理する。
- 何か変化した場合 `revision` を増やし、次のフレームに新しい値をスタンプさせる。

最小かつ正しい `Apply`：

```cpp
void Model::Apply(std::span<const ne::EditRequest> requests) {
    const auto frameRevision = revision;   // 今フレームで View() がスタンプした値
    bool modified = false;
    for (auto &r : requests) {
        if (r.phase != ne::Phase::Commit || r.graph != graph || r.revision != frameRevision)
            continue;                      // 古いリクエスト：破棄して適用しない
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

## 実行する

- 完全な companion アプリは `examples/node_editor/main.cpp` — 実際のモデル、パレット、プレビューを所有する独立の GLFW/OpenGL（または Metal）ウィンドウアプリです。`imkit::node_editor` をリンクする CMake 実行可能として追加します。
- メインの Gallery もノードエディターページを収めています。`imkit_gallery` ターゲットをビルドしてそのルートを選ぶと、同パターンが大規模に確認できます。

## 何が起きているか

- あなたの `Model` がグラフと Undo 履歴を所有し、ImKit は直接触っていません。
- 毎フレームが現在の `revision` を持つ `GraphView` をスタンプした。
- エディターがリクエストを返し、`Apply` はフレームの revision に一致したものだけコミットした。
- `revision` が増え、次のスナップショットが変更を反映する。

## 安全性を保証するルール

- **すべてのリクエストを revision でゲートする。** 古いスナップショット由来のリクエストは破棄して、適用しない。
- **借りるだけ、保持しない。** `GraphView` のフィールド、ノードタイトル、ピンラベルは 1 フレーム分のみ有効。
- **`Render()` の後に適用する。** エディターはフレームまたぎでリクエストをキューする。`ImGui::Render()` の後で読む。
- **`EndEditor` は無条件。** エディターが隠れていようが何とも呼ぶ。
- **`BeginEditor`/`EndEditor` を対応させる。** ミスマッチは ImKit が assert する。

## 次に読む

- [ノードエディター連携（完全な参照）](node-editor.md)
- [エディター API 参照](editor-api.md)
- [設定画面の作り方](build-settings-screen.md)

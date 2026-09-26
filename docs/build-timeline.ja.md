# タイムラインエディターの作り方

[English](build-timeline.md) · [文書索引](README.md) · [ImKit の仕組み](how-it-works.md)

ビデオタイムラインは、複数トラック編集、フェード、トランジション、リップル/ロール/スライド/スリップ移動、トラック管理、エンベロップキーフレーム、プレビューモニターを持つ、プロフェッショナルな NLE 表面です。これはホスト所有の即時モードウィジェットです。あなたは `TimelineProvider`（フレーム毎にあなたのトラックとクリップ、借りられる view と、スタンプする `revision`）、persistent な `TimelineState`（パン/ズーム/ジェスチャー）、イベントバッファを渡します。ライブラリはメディアをデコードせず、ファイルも所有せず、あなたのクリップリストも保持しません — 描画と報告だけです。

![クリップ、トラック、オーバービューを持つマルチトラックタイムライン](images/v3-timeline.gif)

## コアパターン

フレーム毎の、あなたと ImKit の関係は 3 ステップです：

1. **ビューを提供する。** `TimelineProvider` は、あなたの `TrackView`s と `ClipView`s（ホスト所有）と、あなたがスタンプする `revision` を返す。
2. **描画。** `video::Timeline(id, provider, state, selection, events, theme, size)` を呼ぶ。
3. **適用。** `ImGui::Render()` の後、`events.Events()` を反復し、現在のフレームと一致する `revision` のイベントだけコミットする。

すべてのクリップ、トラック、Undo エントリはあなたが所有します。タイムラインは `editor::Event` を発火し、あなたのデータを直接変えません。

## 必要なもの

1. Dear ImGui コンテキストとウィンドウ（あなたのアプリ、または Gallery）。
2. アプリに `imkit::video` ターゲットをリンクする。
3. ホストモデル：トラック、クリップ、Undo 履歴。

## 作るもの

- ホスト所有のモデル：`TrackView`s と `ClipView`s（あなたのデータで満たされる ImKit の view 型）。
- `tracks`/`clips` Callbacks がそれらを返す `TimelineProvider`。
- Persistent な `TimelineState` とクリップの `Selection`。
- トリム、スプリット、フェード、トランジション、トラック編集、リップル削除をコミットする `Apply` 処理。

## モデル（ホスト所有データ）

ImKit の `TrackView` と `ClipView` は普通の、ホスト所有の値型です。メディアからそれらを満たし、データを变えるごとにインクリメントする `revision` を保持します。

```cpp
namespace editor = imkit::editor;
namespace video = imkit::video;

struct Model {
    std::vector<video::TrackView> tracks;
    std::vector<video::ClipView> clips;        // あるトラックのクリップは連続に
    std::uint64_t revision = 1;
    std::uint32_t nextTrack = 400, nextClip = 800;

    // persistent: フレーム間ジェスチャー/ドラッグ状態を保持する。
    video::TimelineState state{};
    // ホスト所有のクリップ選択。Timeline が書き、あなたは読み返す。
    std::vector<editor::StableId> selectedClipIds;
    editor::Selection selection;

    void Init() {
        tracks.push_back(video::TrackView{ nextTrack, "Video A",
                              video::TrackKind::Video, 64, true });
        clips.push_back(MakeClip(nextClip++, 400, "Intro"));
        selection.storage = selectedClipIds.data();
        selection.count = 0;
    }

    video::ClipView MakeClip(std::uint32_t id, std::uint32_t track, const char *label) {
        video::ClipView c;
        c.id = id; c.track = track; c.label = label;
        c.start = editor::FromSeconds(0);
        c.duration = editor::FromSeconds(3);
        c.sourceIn = 0;
        c.speed = 1;
        return c;
    }

    // メディアハンドルの長さ、クリップ最短長さ。ライブラリはこの基準で検証する。
    video::ClipConstraints Constraints(const video::ClipView &c) const {
        video::ClipConstraints b;
        b.mediaFirst = 0;
        b.mediaLast = editor::FromSeconds(3600);   // あなたのメディアから
        b.minimumDuration = editor::FromSeconds(0.083);
        return b;
    }

    video::TimelineProvider Provider() {
        video::TimelineProvider p;
        p.user = this;
        p.revision = revision;
        p.trackCount = (int)tracks.size();
        p.tracks = [](void *u, int, int) {
            auto *m = static_cast<Model *>(u);
            return std::span<const video::TrackView>(m->tracks.data(), m->tracks.size());
        };
        // あるトラックのクリップ、表示範囲内の順序通り。
        p.clips = [](void *u, editor::StableId track, editor::Range) {
            auto *m = static_cast<Model *>(u);
            int i = 0, n = (int)m->clips.size();
            while (i < n && m->clips[i].track != track) ++i;
            int j = i;
            while (j < n && m->clips[j].track == track) ++j;
            return std::span<const video::ClipView>(m->clips.data() + i, j - i);
        };
        return p;
    }
};
```

Provider は 1 つの `Timeline` 呼び出し分、あなたのデータを借りる。スタンプした `revision` はイベントが適用される条件になります：タイムラインはコミットされた各イベントにその値を付け、以下の `Apply` は現在のフレームに一致しないものは破棄します。

## フレーム毎の描画

既存の Dear ImGui フレームループ（`NewFrame` の後）で：

```cpp
auto provider = model.Provider();
// model.state はフレーム間で持続。パン/ズームと進行中のジェスチャーを保持する。
// model.selection は persistent。Timeline が in-place で書き、あなたは読み返す。

std::array<editor::Event, 16> eventStorage{};
editor::EventBuffer events{eventStorage};

video::Timeline("timeline", provider, model.state, model.selection,
                events, theme, ImVec2(0, 300));

ImGui::Render();
model.Apply(events.Events());   // このフレームのイベントをコミット
```

`ImVec2(0, 300)` はウィジェットサイズ（高さ 300 px）。`Timeline` はツールバー、ズームコントロール、オーバービューストリップを描画し、`provider.waveform`/`provider.externalDrops`/`provider.drawClipOverlay` を提供すれば、ウェーブフォーム、外部アセットドロップ、クリップ毎のオーバーレイも描画します。

## イベントの適用

`Apply` は以下をします：

- `phase != editor::Phase::Commit`、または `revision` が古いイベントは無視する。
- 各イベントの `Value` フィールドをモデル変更へマッピングする。ヘッダーは種別毎のフィールドを文書化。公開ヘルパー（`EditClip`, `SplitClip`, `RippleDeletePosition`, `RollClips`, `SlideClip`）はあなたが適用する前に、あなたのメディアに対して変更を検証する。
- 何か変更があれば `revision` を増やし、次のフレームに新しい値をスタンプさせる。

代表的な `Apply`（他の種別も同じ形）：

```cpp
void Model::Apply(std::span<const editor::Event> events) {
    const auto frameRevision = revision;
    bool changed = false;
    auto *clip = [this](editor::StableId id) {
        for (auto &c : clips) if (c.id == id) return &c;
        return nullptr;
    };
    for (auto &e : events) {
        if (e.phase != editor::Phase::Commit || e.revision != frameRevision)
            continue;                          // 古いイベント：破棄
        switch (e.kind) {
        // first/last = start / start+duration。delta = proposed - original。
        case editor::EditKind::TrimStart:
        case editor::EditKind::TrimEnd:
        case editor::EditKind::Move: {
            auto *c = clip(e.target);
            if (!c) break;
            const Tick delta = e.proposed.first - e.original.first;
            auto edit = video::EditClip(*c, e.kind, delta, Constraints(*c));
            if (!edit.valid) break;            // メディア外の編集は破棄
            c->start = edit.start; c->duration = edit.duration;
            c->sourceIn = edit.sourceIn;
            if (edit.rippleDelta != 0) RippleFollowing(c->track, c, edit.rippleDelta); // ホストヘルパー
            changed = true;
            break;
        }
        case editor::EditKind::Split: {        //  Razor
            auto *c = clip(e.target);
            if (!c) break;
            auto sr = video::SplitClip(*c, e.proposed.first, Constraints(*c));
            if (!sr.valid) break;
            ReplaceClip(*c, sr.left, sr.right); // ホストヘルパー：2 クリップが元のスロットを取る
            changed = true;
            break;
        }
        case editor::EditKind::ClipFades: {    // first/last = in/out 長さ。x/y = 曲線
            auto *c = clip(e.target);
            if (!c) break;
            c->fades.inDuration  = e.original.first;
            c->fades.outDuration = e.original.last;
            c->fades.inCurve  = static_cast<video::FadeCurve>(e.original.x);
            c->fades.outCurve = static_cast<video::FadeCurve>(e.original.y);
            changed = true;
            break;
        }
        case editor::EditKind::CutTransition: { // target=左クリップ, parent=右クリップ。first=長さ, x=種別
            auto *c = clip(e.target);
            if (!c) break;
            c->outgoingTransition = video::CutTransitionView{
                e.target, e.parent, e.original.first, 0,
                static_cast<video::TransitionKind>(e.original.x), false, ""};
            changed = true;
            break;
        }
        case editor::EditKind::RippleDelete: { // 関連クリップは 1 バッチで届く
            // Host helper: 生存クリップからトラックを再構築し、隙間を閉じる。
            // RebuildTrack(e.target, e.operationSize);
            changed = true;
            break;
        }
        case editor::EditKind::TrackEdit: {    // offset=TrackAction, parent=插入前のトラック, x=種別
            auto action = static_cast<video::TrackAction>(e.offset);
            if (action == video::TrackAction::Add) {
                auto t = video::TrackView{ nextTrack, "", video::TrackKind::Video, 64 };
                auto before = e.parent == 0 ? 0 :
                    (int)(std::find_if(tracks.begin(), tracks.end(),
                        [e](const auto &tr){ return tr.id == e.parent; }) - tracks.begin());
                tracks.insert(tracks.begin() + before, t);
            } else if (action == video::TrackAction::Remove) {
                std::erase_if(tracks, [e](const auto &tr){ return tr.id == e.parent; });
            } else { /* Reorder / Duplicate: 同形。 */ }
            changed = true;
            break;
        }
        default:
            break;   // Roll/Slide/Slip: video::RollClips / SlideClip / EditClip；
                     // CaptionInsert / AudioEnvelope / Clipboard: ホスト意味。
        }
    }
    if (changed) ++revision;
}
```

選択イベント（`Select`、`BoxSelect`、`LassoSelect`、トラック行編集）は渡した `Selection` に反映済みなので、`Timeline()` の後に `selection.count` / `selection.Contains(id)` を読み返すだけでよい。

## 実行する

- メイン Gallery はタイムライン（`imkit_gallery` ターゲット）を収めている。ビルドしてタイムラインページを開くと、フルサイズの表面を確認できる。
- `tests/video.cpp` はヘッドレス検証。同じ `Timeline()` API を合成モデルで駆動し、トリム/スプリット/トランジション/リップル動作を検証する。テストとして実行すれば、イベント契約を確認できる。

## 何が起きているか

- あなたの `Model` がトラック、クリップ、revision を所有。ImKit は直接変えていない。
- 毎フレームが現在の `revision` を持つ provider をスタンプし、タイムラインを描画した。
- タイムラインがコミットされたイベントを返し、`Apply` はフレームの revision に一致したものだけ受け入れた。
- `revision` が増え、次のスナップショットが編集を反映する。

## 安全性を保証するルール

- **すべてのイベントを revision でゲートする。** 古いフレーム由来のイベントは破棄して、適用しない。
- **メディアに対して検証する。** `EditClip`/`SplitClip`/`RollClips`/`SlideClip` を適用前に使う。ライブラリのヘルパーはメディアハンドルと最短長を強制する。
- **state を所有する。** `TimelineState` はフレーム間で持続しなければならない。再作成すると進行中のジェスチャーがリセットされる。
- **借りるだけ、保持しない。** Provider のビューは現在の `Timeline` 呼び出し分のみ有効。
- **Undo はホスト所有。** タイムラインから Undo イベントは届かない。独自の Undo 履歴を保持し、モデルに再適用する。

## 次に読む

- [タイムライン編集（完全な参照）](timeline-editing.md)
- [エディター API 参照](editor-api.md)
- [カスタムコントロールの作成](custom-component.md)

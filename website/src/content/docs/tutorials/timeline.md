---
title: "タイムラインを作る"
description: "ビデオのタイムラインは、トラックとクリップを表示し、ユーザーがそれをトリム、移動、分割、フェードできるウィジェットです。即時モードで動きます。フレームごとに、トラックとクリップを返すプロバイダーを渡すと、ユーザーが行った編集が、イベントとして返ってきます。"
---

ビデオのタイムラインは、トラックとクリップを表示し、ユーザーがそれをトリム、移動、分割、フェードできるウィジェットです。即時モードで動きます。フレームごとに、トラックとクリップを返すプロバイダーを渡すと、ユーザーが行った編集が、イベントとして返ってきます。

ライブラリは、メディアのデコード、ファイルを開くこと、クリップの一覧の保持をしません。

このチュートリアルでは、1つのトラックと2つのクリップを持つモデルを作って描画し、移動、トリム、分割のイベントを適用します。

![Galleryのタイムライン。複数のトラックのクリップを、トリム、移動、分割で編集する](/imgui-modern-kit/docs-images/v4-timeline-poster.png)

[操作GIFを開く](https://raw.githubusercontent.com/AokiMotohide/imgui-modern-kit/main/docs/images/v4-timeline.gif)

## 基本の形

各フレームは、3つの手順です。

1. **提供。** `TimelineProvider`が、`TrackView`と`ClipView`、そして`revision`を返します。
2. **描画。** `video::Timeline(id, provider, state, selection, events, theme, size)`を呼びます。
3. **適用。** 返ったイベントをモデルに渡します。適用するのは、`revision`がこのフレームと一致するイベントだけです。

タイムラインは、アプリのデータを変更しません。`editor::Event`を返し、その扱いはアプリが決めます。

## 始める前に

- Dear ImGuiのContextとウィンドウ（アプリ、またはGallery）。
- リンクした`imkit::video`。
- トラック、クリップ、Undoの履歴を持つモデル。

## 手順1: データを、アプリ自身のモデルに持つ

`TrackView`と`ClipView`は、素材から値を詰める、ただの値の型です。`revision`を持ち、データが変わるたびに増やします。

```cpp
#include <imgui.h>
#include <imkit/imkit.h>
#include <imkit/video.h>
#include <algorithm>
#include <array>
#include <span>
#include <vector>

namespace editor = imkit::editor;
namespace video = imkit::video;

// この構造体の中身は、すべてアプリのものです。
struct TimelineModel {
    std::vector<video::TrackView> tracks;
    std::vector<video::ClipView> clips;                  // 1つのトラックのクリップは、時間順にまとめて並べる
    std::array<editor::StableId, 64> selectedStorage{};
    editor::Selection selection{selectedStorage};        // タイムラインが書き込み、アプリが読み戻す
    video::TimelineState state;                          // パン、ズーム、操作の途中状態。フレームをまたいで持つ
    std::uint64_t revision = 1;                          // データが変わるたびに増やす
    editor::StableId nextClip = 900;

    TimelineModel() {
        video::TrackView track;
        track.id = 400;
        track.label = "Video A";
        tracks.push_back(track);
        clips.push_back(MakeClip(800, 400, "Intro", 0, 3));
        clips.push_back(MakeClip(801, 400, "Main", 3, 5));
    }

    static video::ClipView MakeClip(editor::StableId id, editor::StableId track, const char* label,
                                    double startSeconds, double seconds) {
        video::ClipView clip;
        clip.id = id;
        clip.track = track;
        clip.label = label;
        clip.start = editor::FromSeconds(startSeconds);
        clip.duration = editor::FromSeconds(seconds);
        clip.sourceIn = editor::FromSeconds(startSeconds);   // このチュートリアルでは、長い素材を1つだけ使う
        return clip;
    }

    // クリップの元になる素材の範囲。EditClipとSplitClipが、これに照らして検査する。
    static video::ClipConstraints Constraints() {
        video::ClipConstraints limits;
        limits.mediaFirst = 0;
        limits.mediaLast = editor::FromSeconds(3600);
        limits.minimumDuration = editor::FromSeconds(1.0 / 12);
        return limits;
    }

    video::ClipView* Find(editor::StableId id) {
        for (video::ClipView& clip : clips)
            if (clip.id == id) return &clip;
        return nullptr;
    }

    video::TimelineProvider Provider() {
        video::TimelineProvider provider;
        provider.user = this;
        provider.revision = revision;
        provider.trackCount = static_cast<int>(tracks.size());
        provider.tracks = [](void* user, int, int) {
            auto& self = *static_cast<TimelineModel*>(user);
            return std::span<const video::TrackView>(self.tracks);
        };
        provider.clips = [](void* user, editor::StableId track, editor::Range) {
            auto& self = *static_cast<TimelineModel*>(user);
            auto first = std::find_if(self.clips.begin(), self.clips.end(),
                                      [&](const video::ClipView& c) { return c.track == track; });
            auto last = std::find_if_not(first, self.clips.end(),
                                         [&](const video::ClipView& c) { return c.track == track; });
            return std::span<const video::ClipView>(&*first, static_cast<std::size_t>(last - first));
        };
        provider.constraints = [](void*, editor::StableId) { return Constraints(); };
        return provider;
    }
    // Split()、MoveOrTrim()、Apply() は、手順4で加えます。
};
```

`TimelineState`は、パン、ズーム、操作の途中状態を持ちます。タイムラインが生きている間は、1つを持ち続けてください。毎フレーム作り直すと、操作の途中状態が失われます。`Selection`もアプリが持ちます。タイムラインが書き込み、アプリが読み戻します。

## 手順2: プロバイダーを作る

`Provider()`は、タイムラインに、トラックと、1つのトラックのクリップを渡すコールバックを返します。コールバックは`Timeline()`の中で呼ばれ、アプリのデータは、その呼び出しの間だけ借用されます。`revision`が、イベントを適用してよいかを決めます。タイムラインは、それをすべてのイベントにコピーし、`Apply`は、一致しないイベントを捨てます。

## 手順3: タイムラインを描く

`NewFrame()`と`Render()`の間で呼びます。

```cpp
void DrawTimeline(TimelineModel& model, const imkit::Theme& theme) {
    const video::TimelineProvider provider = model.Provider();

    std::array<editor::Event, 16> eventStorage{};
    editor::EventBuffer events{eventStorage};

    video::Timeline("timeline", provider, model.state, model.selection, events, theme, ImVec2(0, 300));

    model.Apply(events.Events());
}
```

`ImVec2(0, 300)`は、ウィジェットの大きさで、幅いっぱい、高さ300ピクセルです。タイムラインは、ツールバー、ズーム操作、概要も描きます。`provider.waveform`、`provider.externalDrops`、`provider.drawClipOverlay`を設定すると、波形の表示、外部からのドロップ、クリップごとのオーバーレイを使えます。

## 手順4: イベントを適用する

次のメンバーを、`TimelineModel`に加えます。

```cpp
    // イベントのtickで、クリップを2つに切る。
    bool Split(const editor::Event& e) {
        video::ClipView* clip = Find(e.target);
        if (!clip || clip->locked) return false;
        const video::SplitResult split = video::SplitClip(*clip, e.proposed.first, Constraints());
        if (!split.valid) return false;

        video::ClipView second = *clip;                      // 新しくできる後ろ半分
        second.id = nextClip++;
        second.start = split.right.start;
        second.duration = split.right.duration;
        second.sourceIn = split.right.sourceIn;
        clip->start = split.left.start;
        clip->duration = split.left.duration;
        clip->sourceIn = split.left.sourceIn;
        clips.insert(clips.begin() + (clip - clips.data()) + 1, second);
        return true;
    }

    // 1つの操作に含まれるすべてのクリップを移動またはトリムする。すべて適用するか、何も適用しない。
    bool MoveOrTrim(std::span<const editor::Event> batch) {
        struct Planned { editor::StableId id; video::ClipEdit edit; };
        std::vector<Planned> planned;
        for (const editor::Event& e : batch) {
            video::ClipView* clip = Find(e.target);
            if (!clip || clip->locked) return false;
            const editor::Tick delta = e.kind == editor::EditKind::TrimEnd
                ? e.proposed.last - e.original.last          // 終端が動いた
                : e.proposed.first - e.original.first;       // 始端が動いた
            const video::ClipEdit edit = video::EditClip(*clip, e.kind, delta, Constraints());
            if (!edit.valid) return false;                   // 素材の範囲外、または短すぎる
            planned.push_back({clip->id, edit});
        }
        for (const Planned& p : planned) {
            video::ClipView* clip = Find(p.id);
            clip->start = p.edit.start;
            clip->duration = p.edit.duration;
            clip->sourceIn = p.edit.sourceIn;
        }
        return !planned.empty();
    }

    // 1フレーム分のイベントを適用する。古いイベントは、何も変えない。
    void Apply(std::span<const editor::Event> events) {
        const std::uint64_t frameRevision = revision;
        bool changed = false;
        for (std::size_t i = 0; i < events.size();) {
            const std::size_t count = std::max<std::size_t>(1, events[i].operationSize);   // 1つの操作 = 1つのバッチ
            if (i + count > events.size()) break;                                           // 途中で切れたバッチ
            const auto batch = events.subspan(i, count);
            i += count;

            const editor::Event& first = batch.front();
            if (first.phase != editor::Phase::Commit || first.revision != frameRevision)
                continue;                                                                   // プレビュー、または古いフレームのイベント

            switch (first.kind) {
            case editor::EditKind::Move:
            case editor::EditKind::TrimStart:
            case editor::EditKind::TrimEnd:
                changed |= MoveOrTrim(batch);
                break;
            case editor::EditKind::Split:
                changed |= Split(first);
                break;
            default:
                break;                                                                      // 対応する種類だけ処理する
            }
        }
        if (changed) ++revision;                                                            // 次のフレームは、新しい値を付ける
    }
```

| 規則 | 理由 |
| --- | --- |
| `phase`が`Commit`でないイベントは、飛ばす | `Begin`と`Update`は、ドラッグ中のプレビューです |
| `revision`がフレームと違うイベントは、飛ばす | 古いデータから作られたイベントです |
| イベントは、`operationSize`個ずつのバッチで読む | 複数のクリップの移動は、1つの操作で、Undoでは1段階です |
| バッチは、すべて適用するか、何も適用しない | 途中までの適用は、タイムラインを不整合にします |
| 素材の検査は、`EditClip`と`SplitClip`に任せる | 素材の範囲と最小の長さを、これらが守ります |
| 何か変わったら、`revision`を増やす | 次のフレームが、ユーザーが見たものと異なるようになります |

選択のイベント（`Select`、`BoxSelect`、`LassoSelect`）は、渡した`Selection`にすでに反映されています。処理せず、`Timeline()`のあとで`selection.count`と`selection.Contains(id)`を読んでください。フェード、トランジション、リップル削除、トラックの編集、キーフレームなどのほかの種類も、同じ形で扱えます。各種類のフィールドは、[リファレンス](../../features/timeline/)にあります。

## 手順5: ウィンドウなしで動作を確かめる

`Apply`は、普通のC++です。イベントを直接渡して確かめられます。

```cpp
editor::Event move;
move.kind = editor::EditKind::Move;
move.target = 800;
move.revision = model.revision;                       // 現在のフレーム向けのイベント
move.original = {editor::FromSeconds(0), editor::FromSeconds(3)};
move.proposed = {editor::FromSeconds(1), editor::FromSeconds(4)};
model.Apply({&move, 1});                              // クリップは1秒から始まり、revisionは2になる

model.Apply({&move, 1});                              // 古いイベント: 無視され、何も変わらない
```

Galleryの「Timeline」画面で、全体の様子を確認できます。`tests/video.cpp`は、同じAPIを、画面なしで動かします。Undoは、タイムラインのイベントではありません。履歴はアプリが持ち、それをモデルに再生します。

## 守る規則

- すべてのイベントの`revision`を確かめ、古いイベントは捨てます。
- 適用の前に、`EditClip`、`SplitClip`、`RollClips`、`SlideClip`で、編集を素材に照らして検査します。
- `TimelineState`は、フレームをまたいで持ちます。作り直すと、操作の途中状態が失われます。
- プロバイダーのデータは、`Timeline`の1回の呼び出しの間だけ有効です。spanを保存しません。
- Undoは、アプリが持ちます。

## 次に読む

- [タイムライン編集](../../features/timeline/): 完全なリファレンスです。
- [エディタAPI](../../api/editor-suite/)
- [カスタムコンポーネントを作る](../custom-component/)

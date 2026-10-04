---
title: "Editor Suite"
---

## 用途

アプリが持つデータとresourceに、編集用のCanvas、Timeline、Video、CG部品を加えるときに使います。

## Galleryの画面

![ImKitのTimeline編集画面。](../../../assets/captures/v3-timeline-poster.png)

Editor Suite — Timelineと編集画面 · [Galleryの操作映像を開く](https://raw.githubusercontent.com/AokiMotohide/imgui-modern-kit/main/docs/images/v3-timeline.gif)

## 最小描画例

```cpp
#include <imkit/video.h>
#include <imkit/editor_core.h>

void DrawTimeline(const imkit::video::TimelineProvider& provider,
                  imkit::video::TimelineState& state,
                  imkit::editor::Selection& selection,
                  imkit::editor::EventBuffer& events,
                  const imkit::Theme& theme) {
    imkit::video::Timeline("timeline", provider, state, selection, events, theme);
}
```

**例の種別:** アプリの状態を引数に取る関数例です。provider、選択、event buffer、Themeはアプリ側で用意し、eventは検証してから適用します。

## アプリへ組み込む

`imkit::editor_suite`または`imkit::video` targetをlinkし、providerの参照先は呼び出し中に有効に保ちます。

## 範囲

編集画面はmediaのdecode・再生、任意の3D描画、Undo、project file読み込みを担当しません。

## 関連APIとガイド

- [Editor API契約](../../api/editor-suite/)
- [Timeline編集ガイド](../timeline/)

---

Editor Suiteは汎用的な編集UIです。media decoder/player、色管理engine、UV unwrapper、simulation、PBR renderer、file loaderではありません。

## moduleとtarget

| module | header | CMake target |
|---|---|---|
| Editor Core | `imkit/editor_core.h` | `imkit::editor_core` |
| Video | `imkit/video.h` | `imkit::video` |
| CG | `imkit/cg.h`, `imkit/preview.h` | `imkit::cg` |
| Editor Suite集約 | 上記module header | `imkit::editor_suite` |
| OpenGL3 preview | `imkit/preview.h` | `imkit::preview_opengl3`（任意） |

`imkit::editor_suite`はCore・Video・CGをまとめますが、OpenGLは必須ではありません。

## 所有権と時間

Context、backend、font、texture、元data、provider、UI状態、selection、Undo、保存、workerはホスト所有です。spanとUTF-8 labelは各呼出し期間だけ借用します。StableIdは一意かつ非zeroの64bit値です。Tickは毎秒705600000単位のint64値、FrameRateは有理数で、負のpre-rollとdrop-frame timecodeに対応します。

部品が返す編集eventをホストが検証・適用します。連続操作ではBegin/Update/Commit/Cancelとrevisionを扱い、buffer不足を明示的に処理します。selection、model、衝突policy、Undo、永続化はライブラリへ移りません。

## Editor controls

Coreはcanvas、ruler、selection、property、asset表示を提供します。Videoはtrack/clip編集UI、Monitor、transitionとscope表示を持ちます。CGは階層、gizmo、animation、UVとviewport操作を提供します。入力制約、provider query、request/event storageの条件は[Editor API](../../api/editor-suite/)と各公開headerに従います。

Timelineでは選択全体のoffscreen/linked対象もproviderで解決し、移動可能性とlockを一括判定します。衝突判定とUndo適用はホストが行います。[Timeline guide](../timeline/)に具体的な操作を記載しています。

## Previewの寿命

DrawList previewはホストのscratch bufferを使い、depth bufferを持たないため交差geometryの順序は近似です。OpenGL3 previewはFBO等の自身のGPU資源を所有します。関数表とcurrent Contextはホストから受け取り、Context破棄前に`Shutdown()`を呼びます。GL stateの復元もホスト責務です。

Video Monitorは呼出し時に渡されたtextureと状態を表示します。media再生・decodeは行いません。Previewはホストのdevice/contextを使う明示的な別契約です。OpenGL3 previewのresource寿命とstate副作用は[Editor API](../../api/editor-suite/)で確認してください。

## Galleryで試す

Galleryの **Editor Core**、**Video**、**CG** ページは公開module APIを使うsample hostです。[`editor_workspaces.cpp`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/examples/gallery/editor_workspaces.cpp)から描画の組立を、[`editor_workspaces.h`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/examples/gallery/editor_workspaces.h)からGallery側state/providerを辿れます。詳細なevent/query条件は[Editor API reference](../../api/editor-suite/)、Videoのgestureは[Timeline editing](../timeline/)を参照してください。

Galleryのsynthetic dataやUndo実装はsample hostの機能であり、実アプリのmedia engine・scene・保存contractにはなりません。自動IO/GPU確認とnative OS入力・実host統合の受入も区別してください。

## 対象範囲

ImKitはediting surfaceを提供し、media decoder/player、resampler、complete color-management engine、UV unwrapper、IK/simulation runtime、PBR renderer、file loaderは提供しません。Gallery/CIの合格をnative inputや実project統合の受入と解釈しないでください。

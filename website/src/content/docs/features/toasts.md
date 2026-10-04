---
title: "トースト通知"
---

## 色付きカード

`imkit/toast.h` または `imkit/imkit.h` をincludeします。`ToastViewport` はホストウィンドウが属するviewportに色付きカードを表示し、出現時にフォーカスを奪いません。既存の `ToastRegion` は維持します。状態、本文、context、font、renderer、実処理はホストが所有します。React依存、グローバル管理、worker、永続化は導入しません。

```cpp
// ホストがフレームをまたいで保持します。
imkit::ToastViewportState state;
std::vector<imkit::ToastView> queue;
queue.push_back({1, "読み込み完了", "12件のファイルを読み込みました", imkit::FeedbackKind::Success});
// ホストのウィンドウとテーマscope内で、毎フレーム1回呼びます。
std::array<imkit::ToastEvent, 16> storage{};
imkit::ToastEventBuffer events{storage};
imkit::ToastViewportOptions layout;
layout.position = imkit::ToastPosition::TopRight;
imkit::ToastViewport("notifications", queue, state, ImGui::GetTime(), events,
                     layout, {&theme});
for (std::size_t i = 0; i < events.count; ++i) {
    const auto event = storage[i];
    if (event.kind == imkit::ToastEventKind::Action) {
        // event.idからホストの実処理を実行します。
    } else {
        std::erase_if(queue, [&](const auto& item) { return item.id == event.id; });
    }
}
```

例では `<array>`、`<vector>`、`<algorithm>` が必要です。文字列はUTF-8で借用するため、描画中に記憶領域を有効に保ちます。IDは0以外の一意な値を使います。状態はviewport/contextごとに用意し、contextへ再利用する前に `Reset()` します。呼び出しはUIスレッドで行い、workerの結果はホスト側で受け渡します。

通知と重なるホストウィンドウには `ImGuiWindowFlags_NoBringToFrontOnFocus` を指定します。たとえば `ImGui::Begin("App", nullptr, ImGuiWindowFlags_NoBringToFrontOnFocus)` の後に通知を描画します。未指定だとホストのクリックで既存の通知より前面へ移動します。通知は公開APIの通常ウィンドウで構成し、内部APIによる強制的な描画順変更やフォーカス取得は行いません。新しく作られた重なるウィンドウとモーダルダイアログはDear ImGuiの描画順・入力規則に従います。

## 表示時間と待機

`duration` が負なら既定値を使います。成功・情報は5秒、警告・エラーは残ります。0なら持続、正なら秒数指定です。処理中は持続します。ホバー・キーボードフォーカス中は期限を停止し、待機中も時間を消費しません。毎フレーム1回描画します。非描画の期限処理には `UpdateToastViewport` を使えますが、同一フレームに同じ状態の描画と併用しません。

処理中は `phase=ToastPhase::Loading` にします。`progress` が負または非有限値ならスピナー、それ以外は0〜1へ制限します。完了時は同じIDを `phase=Message` と成功・エラーへ更新します。phase、kind、durationの変更で期限を開始し直しますが、挿入順は維持します。本文・進捗更新では期限を再開しません。

新しい通知を指定辺に近い位置へ置き、表示上限を超えた通知は待機します。ID重複時は最後の要素を採用します。状態はホストの削除待ちの期限終了IDを含め64件まで保持します。超過は `state.overflow` で通知し、空きができると追跡を開始します。期限終了IDは削除または明示的な更新まで再表示しません。要求バッファ不足は `events.overflow` で通知します。未送達の期限終了要求は次のフレーム以降に再送しますが、操作・閉じる要求が入らなかった場合は再操作が必要です。要求件数は毎フレーム初期化します。

## 配置とカスタマイズ

`ToastPosition` は上下×左・中央・右の6位置に対応します。既定は `TopRight`、`maximum=4` です。幅0ならフォント基準、余白・間隔が負ならテーマ適用後のstyleを使います。カード寸法と表示件数をviewportの作業領域に収め、単独で高さを超えるカードには縦スクロールを設けます。

色は意味tokenの面色・状態色から作り、背景に対する文字のコントラストを確認します。角丸、出現時のフェード、動きを減らす設定はテーマに従います。動きを減らす設定ではフェードとスピナーの回転を停止します。`ComponentOptions` でテーマ、ロケール、アクセシビリティを渡します。閉じるボタンの読み上げ名には `dismiss` キーを使い、本文・操作ラベルはホストが指定します。ボタンは標準のキーボードナビゲーションに対応します。グローバルなショートカットや自動フォーカス移動は設けません。

`action` でボタンを指定し、`actionDisabled` で無効化します。`dismissible` は閉じるボタンの有無です。`ToastEvent` は `Action`、`Dismiss`、`Expired` を区別します。操作後に削除・更新するかはホストが決め、処理のキャンセルもホスト側で実行します。

## Galleryと検証

**Toasts**（page 20）に6位置、期限指定、連続通知、処理完了への更新、進捗あり・なし、英日例を用意します。**Appearance** でテーマ、密度、コントラスト、動きを減らす設定を変更できます。

![実際のGalleryでLoadingからSuccessへ変わるトースト](/imgui-modern-kit/docs-images/v3-toasts-poster.png)

[操作GIFを開く](/imgui-modern-kit/docs-images/v3-toasts.gif)

`imkit.toast` は期限、停止、待機、更新、バッファ上限、重複ID、配置、公開マウス入力の要求を検証します。`imkit.toast_api_compile` と外部consumerは公開署名を確認します。GalleryのGPU captureとnative OS/IME、読み上げ、複数モニターDPI、macOSの受け入れは区別します。

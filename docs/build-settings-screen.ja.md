# 設定画面の作り方

[English](build-settings-screen.md) · [文書索引](README.md) · [ImKit の仕組み](how-it-works.md)

ImKit の設定画面は、ホスト所有の struct と、ラベル付きコントロールの列です。ImKit が与えるのは、あなたの状態を読み書きし結果を報告するテーマ付きコントロールで、変更の意味はあなたのコードが決めます。このページでは空ウィンドウから、完全な保存可能な設定パネルまで一歩ずつ組み上げます。各コントロールの詳細は [コンポーネント](components.md) を参照。

![テーマ付きパネルを持つ Gallery 概要](images/gallery-overview.gif)

## コアパターン

設定画面が動くには 3 つのルールがあります：

1. **状態はあなたの struct に。** 通常構造体が全設定を保持。コントロールは読み書きし、ImKit は永続化しない。
2. **1 行 1 コントロール。** `BeginSettingRow` は左にラベル、右にコントロールを描画。1 行 = 1 設定 = 1 コントロール。
3. **書き込まない。要求を返す。** ユーザーが Save を押すとパネルは「保存要求」を返す。あなたのコードが検証・永続化・通知表示を行う。

## 必要なもの

1. Dear ImGui コンテキストとウィンドウ（あなたのアプリ、または Gallery）。
2. パネルをスコープする `Theme`。
3. CMake で `imkit::imkit` ターゲットをリンク。

## ホスト状態

全設定を 1 構造体に集めます。これが単一の信頼源で、コントロールはそのビューです。

```cpp
#include <imkit/imkit.h>
#include <array>

struct DisplaySettings {
    bool enabled = true;          // トグル
    int quality = 1;              // 0..2、コンボ/セグメンテッドで選択
    char qualitySearch[64]{};     // 検索バッファ（ホスト所有）
    float exposure = 0.0f;        // スライダー、EV
    char name[128] = "Main display"; // テキスト入力
    bool showSavedNotice = false; // 保存通知を駆動
};
```

何も ImKit が所有していません。`qualitySearch` バフファはコンボの検索フィールドで、あなたはそれを生かす必要がある。

## 行パターン

定型的な部品は設定行です：左にラベル、右にコントロール、パネル全体の縦リズムに合わせて描画。

```cpp
// ブール値：行不要。Toggle は自己完結。
imkit::Toggle("Enabled", &settings.enabled, {&theme});

// ラベル付きコントロール：行で包む。
if (imkit::BeginSettingRow("exposure", "Exposure")) {
    imkit::DragFloatWithUnit("##exposure", &settings.exposure, "EV", 0.01f, -8.0f, 8.0f);
    imkit::EndSettingRow();   // BeginSettingRow が true を返した時だけ呼ぶ
}
```

- `BeginSettingRow(id, label, labelWidth)` はラベルと第 2 列をレイアウト。`id` は安定した ImGui ID。
- コントロールの id に `##` を入れると、行の中で安定した ImGui id を保つ。
- `EndSettingRow` が 2 列レイアウトを閉じる。`BeginSettingRow` が true を返した時だけ呼ぶ。
- ブール値なら行を省略可 — `Toggle`/`Switch`/`IndeterminateCheckbox` は自己完結。

## 完全な設定画面

上記の部品を組んだ完全パネル。既存の Dear ImGui フレーム内、`NewFrame()` の後に描画。

```cpp
bool DrawDisplaySettings(DisplaySettings& settings,
                         const imkit::Theme& theme, double now) {
    imkit::ThemeScope themeScope(theme);      // パネルはこれでテーマを使い、復元
    bool saveRequested = false;

    if (imkit::Begin("Display settings")) {
        imkit::Toggle("Enabled", &settings.enabled, {&theme});

        if (imkit::BeginSettingRow("quality", "Quality")) {
            constexpr std::array<const char*, 3> labels{"Draft", "Balanced", "High"};
            imkit::SearchableCombo("##quality", &settings.quality, labels,
                                   settings.qualitySearch, sizeof(settings.qualitySearch));
            imkit::EndSettingRow();
        }

        if (imkit::BeginSettingRow("exposure", "Exposure")) {
            imkit::DragFloatWithUnit("##exposure", &settings.exposure, "EV", 0.01f, -8.0f, 8.0f);
            imkit::EndSettingRow();
        }

        imkit::InputTextWithHint("Name", "Required", settings.name, sizeof(settings.name));
        const bool invalidName = settings.name[0] == '\0';
        imkit::ValidationMessage("Enter a name before saving.", invalidName, &theme);
        imkit::BeginDisabled(invalidName);
        saveRequested = imkit::ActionButton("Save", imkit::ActionVariant::Primary,
                                            {}, {&theme});
        imkit::EndDisabled();

        if (settings.showSavedNotice &&
            imkit::NotificationCard({"saved", "Settings saved", imkit::StatusKind::Success, 0},
                                    now, &theme)) {
            settings.showSavedNotice = false;   // 非表示要求 → ホストが非表示に
        }
    }
    imkit::End();                                // Begin と対応。false を返しても呼ぶ
    return saveRequested;                       // true ならホストが永続化
}
```

同様に拡張可能：`SearchableCombo` を `Segmented`（小さな固定選択肢）、数値は `InputScalarWithUnit`/`DragVector3WithUnit` に置き換え、`StatusBadge` を追加して現在状態を表示。各コントロールは [コンポーネント](components.md) を参照。

## 保存、検証、通知

パネルは**永続化しない**。報告する。`DrawDisplaySettings` が `true` を返すと、あなたのコードが 3 つのことをする：

```cpp
double now = ImGui::GetIO().Time;
if (DrawDisplaySettings(settings, theme, now)) {
    if (settings.name[0] == '\0') {
        // すでにボタンで拒否済み。他経路の編集に備えてガード。
        return;
    }
    PersistDisplaySettings(settings);      // あなたのファイル/DB/ネットワーク保存
    settings.showSavedNotice = true;       // 次のフレームで通知を再描画
}
```

- **先に検証。** `ValidationMessage` はフィードバックを描画、`BeginDisabled`/`EndDisabled` が同じ条件で Save ボタンをゲート。条件は 1 箇所にまとめる。
- **事後に通知。** `NotificationCard` は非表示要求を返す。true（ユーザーが非表示）なら `showSavedNotice` を消す。`expiresAt` が 0 ならあなたが削除するまで表示され続ける。
- **最後に永続化。** `PersistDisplaySettings` が返る*後に* `showSavedNotice` を設定し、失敗した保存で成功トーストが飛び立たないようにする。

## 実行する

- Gallery の設定エリアはこのパターンをそのまま組み立てている。`imkit_gallery` ターゲットをビルドし、設定ページを開くとテーマ付きで確認できる。
- 独自のアプリでは `NewFrame()` と `Render()` の間から `DrawDisplaySettings` を呼び、上記のように返った要求を扱う。

## 何が起きているか

- あなたの `DisplaySettings` struct が全値を所有。コントロールはそのビューだった。
- 各設定は、1 コントロールを 1 行に載せたラベル付き行だった。
- パネルはファイルを返すのではなく、保存要求を返した。
- あなたのコードが検証・永続化・非表示可能な通知を表示した。

## きれいに保つためのルール

- **1 値 1 所有者。** 各設定はちょうど 1 フィールドに存在。コントロールが書き込む。隠れた状態は持たない。
- **1 行 1 コントロール。** 1 行にラベルと 2 コントロールを詰めない。
- **検証でアクションをゲート。** 無効なら Save を無効化、該当行の横にメッセージを表示。
- **永続化はホストで。** コントロールにファイルを開いたりソケットを開いたりさせるな。パネルは返し、あなたが動く。
- **`Begin`/`End` と行のペアを対応させる。** `End` は `Begin` が false を返しても呼ぶ。`EndSettingRow` は `BeginSettingRow` が true を返した時だけ呼ぶ。

## 次に読む

- [コンポーネントとレシピ（各コントロール参照）](components.md)
- [ノードエディターの作り方](build-node-editor.md)
- [カスタムコントロールの作成](custom-component.md)

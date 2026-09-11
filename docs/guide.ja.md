# 利用ガイド

[English](guide.md)

## 所有権と拡張構造

`imkit::imkit`はC++20静的ターゲットです。Dear ImGui本体、Context、NewFrame/Render、backend、OS window、フォント、編集値はホストが所有します。現在の生存中ContextのGUIスレッドで呼び出してください。Begin/Endの契約と標準の入力仕様を維持します。

`theme.h`はコピー可能な意味別token、`native.h`は標準overload群、`widgets.h`は選択・タブ装飾を含む互換ラッパー、`components.h`は小さな合成部品です。隠れたregistry、current-theme singleton、worker、plugin managerはありません。拡張時も明示的な値・設定を渡し、データ編集はホスト側、入力処理は標準widget側に残します。

## Theme・フォント・倍率

`MakePrecisionTheme(Light/Dark)`が採用デザインを生成します。`ApplyTheme(theme, scale)`はNewFrame前に適用し、未拡大tokenからstyleを作るため倍率が累積しません。`io.FontDefault`は変更しません。フレーム内の一時適用は`ThemeScope`を使い、入れ子終了時にstyle/fontを復元します。Beginより前、または対象widgetより前に作成し、同じ生存中Contextで破棄してください。

```cpp
auto theme = imkit::MakePrecisionTheme(imkit::ColorScheme::Light);
theme.fonts = {regularFont, emphasisFont}; // ホストが準備した非所有参照
auto saved = theme;
{
    imkit::ThemeScope scope(theme, 1.25f);
    if (imkit::Begin("Display")) {
        imkit::TextUnformatted("Settings / 設定");
    }
    imkit::End();
}
theme = saved; // ホストが保持・復元。自動永続化は行わない
```

基本値は高さ28、間隔6、角丸4、本文14、見出し18 logical pixelです。標準部品の高さは現在の文字サイズとpaddingから決まり、大きな文字には高さを増やします。inline action、menu row、table、multilineを一律28pxにはしません。アプリ倍率は`scale`で適用し、外側で寸法を再拡大しないでください。monitor DPI対応はホストの責務です。

`SetAccent`はaccent、on-accent、focus、selectionだけを更新します。他の色は独立しています。canvas、surface、input、raised、text、muted、border、destructive/on-destructive、success、warningを編集できます。任意の配色に対する無条件のコントラスト保証はありません。

カタログはInter 4.1を先に読み、通常・強調それぞれへNoto Sans JP 2.004 Regularをmergeします。Latin U+0020–U+024Fをフォールバックから除外します。日本語は本文・見出しともRegularで、疑似太字は作りません。フォントは任意のホスト資産で、ライブラリに埋め込みません。文字列はUTF-8です。ホスト側で適切なglyphとplatform IME callbackを設定してください。OSフォント探索は行いません。

## 合成部品

| API | 契約 |
|---|---|
| `ActionButton` | primary/secondary/ghost/destructive。配色とmotionは`ComponentOptions{&theme, &animation}`で明示 |
| `IconActionButton` | 単独操作向けのアイコン＋短いラベル。説明、無効理由、操作種別を指定 |
| `IconButton` | 標準arrow buttonの操作と説明tooltip |
| `Toggle` / `Switch` | checkboxの入力・keyboard契約を維持し、switch trackを描画 |
| `IndeterminateCheckbox` | Mixedから操作するとCheckedへ。その後は通常の切替 |
| `Segmented` | label spanとホスト所有の選択index |
| `SearchableCombo` | ホスト所有のUTF-8検索buffer・選択値・disabled index。ASCIIのみ大文字小文字を無視 |
| `InputScalarWithUnit` | 標準scalarの解析・精度を維持し、単位を別表示 |
| `DragFloatWithUnit`, `InputVector3WithUnit` | 標準編集と単位、等幅の複数成分入力 |
| `BeginSettingRow` / `EndSettingRow` | 2列table。BeginがtrueのときだけEnd |
| `StatusBadge` | 色に加え、文字とマーク形状で状態を表示 |
| `NotificationCard` | 寿命・削除はホスト所有。dismiss要求を返す。expiry=0は期限なし |
| `BeginToolbar` / `EndToolbar` | ChildなのでEndは常に必要 |
| `ValidationMessage` | 入力直後に呼ぶ描画専用のinvalid枠とtooltip |
| `OverlayDecoration` | 現在のpopup内部にclipした控えめな奥行き |

範囲編集は`DragFloatRange2` / `DragIntRange2`を使います。scalar/vectorを含む全overloadは[API対応表](api-coverage.md)に記載しています。標準ラッパーは現在のImGuiStyleを使い、Theme引数を要求しません。DrawList装飾で別itemを追加しないため、`IsItem*`の対象を維持します。単位入力や分割選択は標準groupのstatusを公開します。

## アニメーション状態の寿命

`AnimationState`はホスト所有の固定256slotです。Contextごとに別instanceを使い、Context破棄・再利用時にホストの世代値でResetしてください。Contextの生pointerは保持しません。IDは標準ID stackに従います。`Prune(frame)`は120frame見えない項目を回収し、容量超過時は最古の項目を置換します。新しい項目は現在のtargetから始まり、不要な登場animationを起こしません。

標準ラッパーは即時応答です。明示的な状態を渡すaction outline、switch移動、overlay装飾は既定60/100msで遷移します。状態省略、motion無効、duration=0は即時表示です。編集値の変更を遅延させません。

## ソース導入

```cmake
add_library(host_imgui STATIC
    ${IMGUI_SOURCE_DIR}/imgui.cpp
    ${IMGUI_SOURCE_DIR}/imgui_draw.cpp
    ${IMGUI_SOURCE_DIR}/imgui_tables.cpp
    ${IMGUI_SOURCE_DIR}/imgui_widgets.cpp)
target_include_directories(host_imgui PUBLIC ${IMGUI_SOURCE_DIR})
set(IMKIT_IMGUI_TARGET host_imgui)
add_subdirectory(external/imgui-modern-kit)
target_link_libraries(your_app PRIVATE imkit::imkit)
```

固定版1.92.9b dockingを使用してください。1.91.9と1.89 WIP（18814）の構成も読み取り比較しましたが、現在のfont/style/API契約が異なるため本Releaseの対応外です。ホストのImGui更新は別途判断し、既存forkを暗黙に置換しないでください。他OS/toolchainは未検証です。

## インストール済みSDK

Windows x64、MSVC v145、Release `/MD`、Debug `/MDd`、Dear ImGui 1.92.9b dockingの既定ABI型が条件です。compile definitionと`imconfig.h`も全翻訳単位で揃えてください。archiveにImGui本体は含みません。別のImGui版はheader guardで拒否します。以下の確認flagはcompiler/CRT/architecture/型の一致を利用者が確認したことを表し、自動的なbinary解析ではありません。

```cmake
# 対応するhost_imguiを先に作成
set(IMKIT_IMGUI_TARGET host_imgui)
set(IMKIT_SDK_ABI_CONFIRMED ON) # manifest・構成の一致を確認してから
find_package(imkit 1.0 CONFIG REQUIRED)
target_link_libraries(your_app PRIVATE imkit::imkit)
```

`-DCMAKE_PREFIX_PATH=/path/to/sdk`でconfigureします。依存adapterからホストtargetへ接続し、元buildのパスを埋め込みません。`examples/consumer`はソースとinstalled packageの両方に対応します。binary条件が一致しない場合はソースからビルドしてください。

検証の範囲と制約は[検証記録](validation.md)、ライセンスは[第三者通知](../THIRD_PARTY_NOTICES.md)を参照してください。公開IOによる統合確認と、native OS/IME・実機の受け入れ確認は別です。

## Editor Suite 2.0 workspace

native GalleryのEditor Core／Video Editor／CG Editorを開きます。`--verify-editors --capture-editors --output out/editors`で公開IO検証と150%を含む実backbuffer画像を取得できます。中ボタンでpan/orbit、TimelineはCtrl+wheelでzoom、clip端でtrim、Razorでsplit、gizmo軸端でdragします。UV/Graphタブの点も編集できます。[Editor Suite 2.0の検証結果と範囲](editor-refresh.md)を参照してください。

## デザインシステム基盤

[API移行・provider所有権・今回の実装範囲](design-system.md)を参照してください。

## 汎用ワークフローと画像部品

[API・所有権・座標系・Gallery](workflow-components.md)を参照してください。
Generic Workspace、Feedback / States、Preview Tilesを使います。通知時刻・queue、
texture、選択、resize寸法はホストの状態として保持してください。

# Changelog / 変更履歴

> Published release entries present English first, followed by equivalent Japanese notes. Unreleased remains a work-in-progress list.

## Unreleased

### Added

| Header | Additions |
| --- | --- |
| `imkit/decor.h` | `DrawGlowFrame`, `GlowFrameLevel`, `GlowFrameStyle`, `GlowFrameState`, `GlowMotion`, `GlowPlacement` |
| `imkit/shell.h` | `ExternalControlIndicator`, `MakeExternalControlStyle`, `ExternalControlView`, `ExternalControlStyle`, `ExternalControlState`, `ExternalControlRequest`, `ExternalControlPhase`, `ExternalControlPillPosition` |

- `DrawGlowFrame` lights the edge of a rectangle: steady, breathing or orbiting, with ripples on `Pulse()` or at an interval, inside, outside or centred on the edge. Every value is a field of `GlowFrameStyle`.
- `ExternalControlIndicator` frames the viewport while an automation API, a remote operator or an agent drives the application, and shows a pill with the client, its activity and Stop. Each phase has its own frame style: connected, active and input locked.
- Gallery: a Glow frame page with six looks, an editor for every field, a copyable C++ snippet, and a scripted remote session that frames the Gallery window.

### 日本語

#### 追加

| ヘッダー | 追加 |
| --- | --- |
| `imkit/decor.h` | `DrawGlowFrame`、`GlowFrameLevel`、`GlowFrameStyle`、`GlowFrameState`、`GlowMotion`、`GlowPlacement` |
| `imkit/shell.h` | `ExternalControlIndicator`、`MakeExternalControlStyle`、`ExternalControlView`、`ExternalControlStyle`、`ExternalControlState`、`ExternalControlRequest`、`ExternalControlPhase`、`ExternalControlPillPosition` |

- `DrawGlowFrame` は矩形の縁を光らせます。一定、呼吸、周回の動きと、`Pulse()` または一定間隔で出る波紋を持ち、縁の内側、外側、中央に描けます。値はすべて `GlowFrameStyle` のフィールドです。
- `ExternalControlIndicator` は、自動化API、遠隔の操作者、エージェントがアプリを操作している間、ビューポートを枠で囲み、操作中の相手、その作業、停止ボタンを載せたピルを出します。枠の見た目は、接続中、操作中、入力ロック中の状態ごとに持ちます。
- Gallery：Glow frame ページを追加しました。6つの見た目、全フィールドの編集、コピーできる C++ のコード、Gallery のウィンドウを囲む遠隔操作の再現を含みます。

## 4.1.0 — 2026-11-06

**ImKit 4.1 redesigns the node editor, adds a tile map for tiled displays, and lets you tune control sizes, play sprite sheets and draw only the PTZ controls a camera has.** Rebuild your application and every linked ImKit library with the 4.1 headers: structures grew at their end. Only the node editor changes its default look. The Dear ImGui revision and the dependencies are unchanged. See the [migration guide](docs/reference/migration-v4.md#upgrading-from-40-to-41) and the [4.1 additions](docs/reference/v4.1-api.md).

### Highlights

- **Redesigned node editor.** Header fills, link colours that follow socket types, socket glyphs, node shapes and states, presets, draw hooks, wiring feedback, and Alt-click and Ctrl-drag on sockets.
- **Tile map.** `show::TileMap` draws the panels of an LED wall or video wall coloured by port, with the wiring order.
- **Adjustable control sizes.** `Theme::layout` sets how tall controls are; unset values keep the density.
- **Sprites and progress thumbs.** Play a sprite sheet, or let a picture ride a progress fill.
- **PTZ from capabilities.** The control draws only what the camera offers.

### Added

| Header | Additions |
| --- | --- |
| `imkit/show/led.h` | `TileMap`, `TileMapTile`, `TileMapView`, `TileMapOptions`, `TileMapRequest` |
| `imkit/theme.h` | `Theme::layout` (`LayoutTokens`), `Theme::node` (`NodeTokens`); `layout.*` and `node.*` theme keys; resolved values in `Metrics` |
| `imkit/decor.h` | `Sprite`, `SpriteSheet`, `SpriteState`, `SpriteFrameAt`, `SpriteFrameUV`, `DrawSprite` |
| `imkit/patterns.h` | `ProgressTrackView::thumb`, `thumbUser`, `thumbHeight` |
| `imkit/ptz.h` | `PtzView::capabilities` (`PtzCapabilities`), `PtzView::readoutText`; locale keys `ptz.panPad`, `ptz.tiltPad`, `ptz.presetRecallHint`, `ptz.unsupported` |
| `imkit/node_editor.h` | `NodeStyle` fills, link colours and arrows; `NodeView::category`, `tint`, `subtitle`, `glyph`, `shape`, `diagnostic`; `PinView` roles, containers and flags; `DrawPinGlyph`, `PinChromeView`; `LinkView::validity`; `MakeNodeStylePreset`, `NodeDrawHooks`; `BeginNodePanel`, `EndNodePanel`; `ConnectSelected`; `EditorOptions::locale`, `EditorFrame::hoveredNode` |

Gallery: LED gains a tile map, Skins gains a sprite and progress-thumb demo, and PTZ camera gains a Capabilities section.

#### Tile map for tiled displays

- `show::TileMap` draws one cell per panel of a tiled display, such as LED wall cabinets. Tiles are coloured by group (the data port or power circuit that feeds them), each group is joined in `order` like its cables, and a dot marks the first tile. `highlightGroup` dims the other groups. The host gets the hovered and clicked tile and writes its own tooltip.

#### Adjustable control sizes

- `Theme::layout` (`LayoutTokens`) overrides the sizes that decide how tall controls are: `controlHeight`, `spacingX` / `spacingY`, `horizontalPadding`, window and cell padding, `indent`, `scrollbar`, `grabMin`, `separatorTextPaddingY`, and the component ratios `checkHeightScale`, `toggleHeightScale`, `dialogPaddingScale`, `progressBarHeightScale`, `badgeExtraHeight`, `segmentedInset`. `ResolveTheme` applies them after the density; an unset value keeps the density, so existing themes draw as before.
- `Metrics` gains the resolved values (`spacingY`, `windowPaddingX/Y`, `cellPaddingY`, `indent`, `scrollbar`, `grabMin`, `separatorTextPaddingY`, `parts`). Components that have no `Theme` (Checkbox, Toggle, StatusBadge, Segmented, dialogs, progress bars) read `parts` from the theme last applied to the context; `ThemeScope` restores the previous one.
- The theme text format reads and writes `layout.*` keys; an unset token is not written.

#### Sprite playback and progress thumbs

- `Sprite`, `SpriteSheet` and `SpriteState` play a sheet of equally sized frames from one host-owned texture: `Loop`, `Once` (then the rest frame) or `HoldLast`. Reduced motion shows `restFrame`; a rectangle outside the visible area is not drawn. `SpriteFrameAt` and `SpriteFrameUV` give the frame and its texture coordinates without a Dear ImGui context, and `DrawSprite` draws one frame into any rectangle.
- `ProgressTrackView::thumb` (with `thumbUser` and `thumbHeight`) calls the host with the track rectangle, the head position and the shown fraction, so a picture can ride the fill. It is called for a determinate track and for a running indeterminate one. `thumbHeight` reserves layout height above the track.

#### PTZ control draws only what the camera offers

- `PtzView::capabilities` (`PtzCapabilities`: `pan`, `tilt`, `zoom`, `home`, `presets`, `presetStore`, `speed`) hides the controls a camera lacks. A pan-only head, a zoom-only lens or recall-only presets get a smaller control; with nothing to offer it draws one line of text. Every field defaults to true, so existing code is unchanged.
- `PtzView::readoutText` lets the host format the P / T / Z chips for devices that report normalised values.
- With the default diameter the pad shrinks (down to 3.2 frame heights) to fit a narrow host instead of being clipped at the right edge.

#### Node editor

- `NodeStyle` gains `headerFill` (`Strip`, `Tinted`, `Flat`, `Gradient`, `None`), `headerTint`, hover and selected border widths, `linkColor`, `linkArrow`, `highlightConnectedLinks`, `hollowUnconnectedPins`, `status` and a borrowed `categories` table. `NodeView` gains `category`, `tint`, `subtitle` and `glyph`; a glyph is drawn in a tile left of the title.
- `Theme::node` (`NodeTokens`) carries the same choices, and theme files read and write `node.*` keys. `MakeNodeStyle` applies them.
- Behaviour changes: links take the output socket type colour by default (accent without type colours); the status line of an Idle node with no time or progress is hidden; `NodeKind::Reroute` draws as a 16-unit knot with both sockets at its centre; an unconnected required input is marked; link labels sit in a pill; the resize grip scales with the zoom. Set `linkColor = Accent` and `status = Always` for the previous look.
- `EditorOptions::locale` translates the status line, connection verdicts and the full-buffer warning (`node_editor.*` keys). `EditorFrame::hoveredNode` reports the topmost hovered node.
- Wiring feedback: sockets that can take the dragged wire get a ring, the others fade (`incompatibleAlpha`) and nodes without a candidate fade (`incompatibleNodeAlpha`). The draft wire has the final link shape, snaps to the nearest candidate within `snapDistance`, and a pill by the cursor shows the verdict reason instead of a tooltip.
- `LinkView::validity` (`Valid`, `Converted`, `Invalid`, `Cycle`) and `LinkView::reason` draw the host's verdict on existing links: a marker in the middle, and for `Invalid` the error colour, long dashes and a ring on the receiving input.
- Socket glyphs: `PinView::role` (`Data`, `Flow`), `container` (`Single`, `Array`, `Set`, `Map`), `flags` (`Reference`, `Wildcard`, `Field`, `Optional`, `Unavailable`), `keyType`, `inspect` and `reason`. `DrawPinGlyph` and `PinChromeView` draw them outside the canvas too. Flow links use the text colour, `flowLinkWidth` and chevrons; inputs with `multiple=true` draw as a capsule with the links side by side; `pinSeat` docks sockets into the edge; active links move marks at `flowSpeed` and `flowSpacing`.
- `CanConnect` rejects an unavailable socket with its reason and a flow-to-data link, and lets a wildcard through without the type check.
- Node shapes and states: `NodeView::shape` (`Standard`, `Event`, `Compact`, `Variable`, `Boundary`) and `NodeView::diagnostic` (`Severity`, message). Frames and groups draw a label tab and member count, notes a folded card, subgraphs a stacked card with an open button that queues `EnterGraph`. Running, waiting, success, error, bypassed, disabled, cached, breakpoint and locked states each have their own look; `Status::Disabled` disables the body widgets.
- `MakeNodeStylePreset(theme, NodeStylePreset)` with `Classic`, `Studio` and `Logic`. `NodeStyle::hooks` (`NodeDrawHooks`) lets the host replace the drawing of node backgrounds, title rows, sockets and links; input stays with the editor.
- `BeginNodePanel` / `EndNodePanel` group rows; a closed panel gathers its sockets (`PinView::panel`) on its title row. `NodeView::hideUnusedPins` hides unconnected sockets.
- New gestures: Alt-click breaks a socket's links, Ctrl-drag moves them to another socket, double-clicking a link requests `EditKind::InsertReroute`, dropping a single dragged node onto a link inserts it, and `ConnectSelected` links two selected nodes. `Keymap` gains `bypass` (Ctrl+B) and `collapse` (Ctrl+H).

### Changed

- The node editor's default look changes: links take the output socket's type colour, an idle node's empty status line is hidden, `NodeKind::Reroute` is a small knot, an unconnected required input is marked, link labels sit in a pill and the resize grip scales with the zoom. Set `linkColor = LinkColorSource::Accent` and `status = StatusDisplay::Always` for the previous look.
- Fixed: a node whose body submits no item (for example a locked, empty node) no longer trips Dear ImGui's cursor-extension assertion.
- `IMKIT_VERSION` reads `4.1.0`.

### Compatibility

Public additions are at the end of existing structs (`Theme`, `ThemeScope`, `Metrics`, `PtzView`, `ProgressTrackView`, `NodeStyle`, `NodeView`, `PinView`, `LinkView`, `EditorOptions`, `EditorFrame`), so objects built with 4.0 must not be mixed with 4.1 structures. A request for `find_package(imkit 4.0)` no longer matches; ask for 4.1.

### 日本語

**ImKit 4.1は、ノードエディタを作り直し、タイル状の表示装置向けのタイルマップを加え、コントロールの寸法の調整、スプライトシートの再生、カメラが持つ機能だけのPTZ操作を可能にします。** 構造体は末尾に拡張されています。アプリと、リンクするすべてのImKitライブラリを、4.1のヘッダーで再ビルドしてください。既定の見た目が変わるのは、ノードエディタだけです。Dear ImGuiのrevisionと依存は変わりません。[移行ガイド](docs/reference/v4移行ガイド.md#40から41へ)と[4.1追加API](docs/reference/4.1追加API.md)を参照してください。

#### 主な変更

- **ノードエディタを作り直しました。** ヘッダーの塗り、ソケットの型に沿う配線の色、ソケットの記号、ノードの形と状態、プリセット、描画フック、配線中のフィードバック、ソケットのAltクリックとCtrlドラッグが入ります。
- **タイルマップ。** `show::TileMap` で、LEDウォールやビデオウォールのパネルを、ポートごとに色分けし、配線の順序とともに描けます。
- **調整できるコントロールの寸法。** `Theme::layout` でコントロールの縦幅を決められます。未指定の値は密度のままです。
- **スプライトと進捗のつまみ。** スプライトシートの再生と、進捗の塗りに乗る絵を描けます。
- **機能から決めるPTZ操作。** カメラが持つ機能だけを描きます。

#### 追加

| ヘッダー | 追加するもの |
| --- | --- |
| `imkit/show/led.h` | `TileMap`、`TileMapTile`、`TileMapView`、`TileMapOptions`、`TileMapRequest` |
| `imkit/theme.h` | `Theme::layout`（`LayoutTokens`）、`Theme::node`（`NodeTokens`）。`layout.*` と `node.*` のテーマキー。`Metrics` の解決済みの値 |
| `imkit/decor.h` | `Sprite`、`SpriteSheet`、`SpriteState`、`SpriteFrameAt`、`SpriteFrameUV`、`DrawSprite` |
| `imkit/patterns.h` | `ProgressTrackView::thumb`、`thumbUser`、`thumbHeight` |
| `imkit/ptz.h` | `PtzView::capabilities`（`PtzCapabilities`）、`PtzView::readoutText`。ロケールキー `ptz.panPad`、`ptz.tiltPad`、`ptz.presetRecallHint`、`ptz.unsupported` |
| `imkit/node_editor.h` | `NodeStyle` の塗り、配線の色と矢印。`NodeView::category`、`tint`、`subtitle`、`glyph`、`shape`、`diagnostic`。`PinView` の役割、入れ物、修飾。`DrawPinGlyph`、`PinChromeView`。`LinkView::validity`。`MakeNodeStylePreset`、`NodeDrawHooks`。`BeginNodePanel`、`EndNodePanel`。`ConnectSelected`。`EditorOptions::locale`、`EditorFrame::hoveredNode` |

Galleryには、LEDにタイルマップ、Skinsにスプライトと進捗のつまみのデモ、PTZ cameraにCapabilitiesの節を加えました。

##### タイル状の表示装置のタイルマップ

- `show::TileMap` で、LED ウォールのキャビネットのようなタイル状の表示装置を、パネル1枚を1マスとして描けます。マスはグループ（そのパネルへ給電・送信するデータポートや電源回路）ごとに色分けし、グループ内を `order` の順に線で結んでケーブルの順序を示します。先頭のマスには点を打ちます。`highlightGroup` を指定すると、ほかのグループを薄く表示します。ホストは、ポインタが乗ったマスとクリックしたマスを受け取り、ツールチップを自分で描きます。

##### 調整できるコントロールの寸法

- `Theme::layout`（`LayoutTokens`）で、コントロールの縦幅を決める寸法を上書きできます。`controlHeight`、`spacingX` / `spacingY`、`horizontalPadding`、ウィンドウとセルの余白、`indent`、`scrollbar`、`grabMin`、`separatorTextPaddingY`、および部品ごとの比率 `checkHeightScale`、`toggleHeightScale`、`dialogPaddingScale`、`progressBarHeightScale`、`badgeExtraHeight`、`segmentedInset` です。`ResolveTheme` が密度のあとに適用し、未指定の項目は密度の値のままなので、既存のテーマの見た目は変わりません。
- `Metrics` に解決済みの値（`spacingY`、`windowPaddingX/Y`、`cellPaddingY`、`indent`、`scrollbar`、`grabMin`、`separatorTextPaddingY`、`parts`）が加わります。`Theme` を持たない部品（Checkbox、Toggle、StatusBadge、Segmented、ダイアログ、進捗バー）は、そのコンテキストに最後に適用したテーマの `parts` を読みます。`ThemeScope` は前の値に戻します。
- テーマのテキスト形式で `layout.*` を読み書きできます。未指定のトークンは書き出しません。

##### スプライトの再生と進捗のつまみ

- `Sprite`、`SpriteSheet`、`SpriteState` で、ホストが所有する1枚のテクスチャに並べた、同じ大きさのコマのシートを再生できます。`Loop`、`Once`（終わったら静止コマ）、`HoldLast` に対応します。`reducedMotion` では `restFrame` を表示し、見えない領域は描きません。`SpriteFrameAt` と `SpriteFrameUV` は、Dear ImGui のコンテキストなしで、コマ番号とテクスチャ座標を返し、`DrawSprite` は、任意の矩形に1コマを描きます。
- `ProgressTrackView::thumb`（`thumbUser`、`thumbHeight` つき）で、トラックの矩形、先頭の位置、表示中の割合をホストへ渡し、塗りに乗る絵を描けます。確定した進捗と、動いている不確定の進捗で呼ばれます。`thumbHeight` は、トラックの上にレイアウトの高さを確保します。

##### カメラが持つ機能だけを描くPTZ操作

- `PtzView::capabilities`（`PtzCapabilities`: `pan`、`tilt`、`zoom`、`home`、`presets`、`presetStore`、`speed`）で、カメラが持たない操作を隠せます。パンだけの雲台、ズームだけのレンズ、呼び出し専用のプリセットでは小さな操作部になり、何もなければ1行の文だけを描きます。どのフィールドも既定値はtrueなので、既存のコードは変わりません。
- `PtzView::readoutText` で、正規化した値を報告する機器向けにP / T / Zのチップの文字をホストが整えられます。
- 直径が既定のときは、狭い配置先でも右端が切れないよう、パッドをフレーム高さの3.2倍まで縮めます。

##### ノードエディタ

- `NodeStyle` に、`headerFill`（`Strip`、`Tinted`、`Flat`、`Gradient`、`None`）、`headerTint`、hover 時と選択時の枠幅、`linkColor`、`linkArrow`、`highlightConnectedLinks`、`hollowUnconnectedPins`、`status`、借用の `categories` 表が加わります。`NodeView` に `category`、`tint`、`subtitle`、`glyph` が加わり、glyph はタイトル左のタイルに描きます。
- `Theme::node`（`NodeTokens`）で同じ設定を持てます。テーマファイルで `node.*` キーを読み書きでき、`MakeNodeStyle` が適用します。
- 動作の変更: 配線は既定で出力ソケットの型の色になります（型の色がなければ accent）。時間も進捗もない Idle ノードのステータス行は出ません。`NodeKind::Reroute` は 16 単位のノットとして描き、2つのソケットを中心に置きます。必須入力が未接続のとき印を付けます。配線のラベルはピルの中に描きます。リサイズのつまみはズームに合わせて大きさが変わります。以前の見た目に戻すには `linkColor = Accent` と `status = Always` を指定します。
- `EditorOptions::locale` で、ステータス行、接続の判定、バッファ不足の警告を翻訳できます（`node_editor.*` キー）。`EditorFrame::hoveredNode` は、一番手前の hover 中のノードを返します。
- 配線中のフィードバック: ドラッグ中の配線を受けられるソケットに輪を付け、ほかのソケット（`incompatibleAlpha`）と受け先のないノード（`incompatibleNodeAlpha`）を薄くします。ドラッグ中の配線は確定後と同じ形で描き、`snapDistance` 以内の最も近い候補に吸着します。判定の理由は tooltip ではなく、カーソル横のピルに出します。
- `LinkView::validity`（`Valid`、`Converted`、`Invalid`、`Cycle`）と `LinkView::reason` で、既存の配線に対するホストの判定を描きます。中央に印を付け、`Invalid` はエラーの色と長い破線で描き、受け側の入力に輪を付けます。
- ソケットの記号: `PinView::role`（`Data`、`Flow`）、`container`（`Single`、`Array`、`Set`、`Map`）、`flags`（`Reference`、`Wildcard`、`Field`、`Optional`、`Unavailable`）、`keyType`、`inspect`、`reason` が加わります。`DrawPinGlyph` と `PinChromeView` で、キャンバスの外でも同じ記号を描けます。Flow の配線は文字色、`flowLinkWidth`、山形の刻みで描きます。`multiple=true` の入力は長円で描き、配線を並べて受けます。`pinSeat` でソケットを枠に差し込んだように見せます。実行中の配線では、`flowSpeed` と `flowSpacing` で印が動きます。
- `CanConnect` は、使えないソケットを理由つきで拒否し、Flow とデータの接続も拒否します。ワイルドカードは型の判定をせずに通します。
- ノードの形と状態: `NodeView::shape`（`Standard`、`Event`、`Compact`、`Variable`、`Boundary`）と `NodeView::diagnostic`（`Severity` と文言）が加わります。Frame と Group はラベルタブと中のノード数を、Note は角を折ったカードを、サブグラフは重ねたカードと `EnterGraph` を要求する「開く」ボタンを描きます。実行中、待機、成功、エラー、Bypass、無効、キャッシュ、ブレークポイント、ロックの各状態に専用の見た目があります。`Status::Disabled` は本体のウィジェットを無効にします。
- `MakeNodeStylePreset(theme, NodeStylePreset)` で、`Classic`、`Studio`、`Logic` のプリセットを選べます。`NodeStyle::hooks`（`NodeDrawHooks`）で、ノードの背景、タイトル行、ソケット、配線の描画をホストが置き換えられます。入力はエディタ側に残ります。
- `BeginNodePanel` / `EndNodePanel` で行をまとめられます。閉じたパネルは、`PinView::panel` が一致するソケットを見出しの行に集めます。`NodeView::hideUnusedPins` は未接続のソケットを隠します。
- 新しい操作: Altクリックでソケットの配線をすべて外し、Ctrlドラッグで別のソケットへ移します。配線のダブルクリックで `EditKind::InsertReroute` を要求し、1つだけドラッグしたノードを配線の上で離すと挿入します。`ConnectSelected` は選んだ2つのノードをつなぎます。`Keymap` に `bypass`（Ctrl+B）と `collapse`（Ctrl+H）が加わります。

#### 変更

- ノードエディタの既定の見た目が変わります。配線は出力ソケットの型の色になり、Idle のノードの空のステータス行は隠れ、`NodeKind::Reroute` は小さなノットになり、未接続の必須入力に印が付き、配線のラベルはピルの中に描かれ、リサイズのつまみはズームに合わせて大きさが変わります。以前の見た目に戻すには `linkColor = LinkColorSource::Accent` と `status = StatusDisplay::Always` を指定します。
- 修正: 本体に項目を1つも置かないノード（ロックされた空のノードなど）で、Dear ImGui のカーソル拡張のアサートが出なくなりました。
- `IMKIT_VERSION` は `4.1.0` を返します。

#### 互換性

公開構造体への追加は、末尾だけです（`Theme`、`ThemeScope`、`Metrics`、`PtzView`、`ProgressTrackView`、`NodeStyle`、`NodeView`、`PinView`、`LinkView`、`EditorOptions`、`EditorFrame`）。それでも、4.0でビルドしたオブジェクトと4.1の構造体は混ぜないでください。`find_package(imkit 4.0)` の要求は4.1にマッチしなくなったので、4.1を指定してください。

## 4.0.0 — 2026-10-31

**ImKit 4.0 refreshes the default look, adds brand themes, and brings new modules for charts, camera control and live shows.** Rebuild your application and every linked ImKit library with the 4.0 headers: structures grew and default tokens changed. The Dear ImGui revision and the dependencies are unchanged. See the [migration guide](docs/reference/migration-v4.md) and the [4.0 additions](docs/reference/v4.0-api.md).

### Highlights

- **New default look.** Larger radii, real shadows, 0.14 s control motion, tonal buttons, pill segments, fill sliders, chevron combos, and redrawn progress, toasts, alerts, empty states, pagination and search.
- **Brand themes.** `MakeBrandTheme` builds a complete theme from four colours. Corners, outlines, hard shadows and text roles are tokens. Skins replace component frames, and the Gallery gains Theme Studio and Skins.
- **New modules.** Charts, PTZ control, range sliders, and the optional `imkit::show` module for media servers.
- **Redesigned Gallery** with a new Start page, searchable navigation, and a three-minute film in Japanese and English.

### Added

| Header | Additions |
| --- | --- |
| `imkit/charts.h` | `DonutChart`, `BarChart`, `Gauge`, `Sparkline`, `ChartColor` |
| `imkit/ptz.h` | `PtzControl`, `PtzPadVector` |
| `imkit/range_slider.h` | `RangeSlider`, `RangeSliderInt` |
| `imkit/theme.h` | `MakeBrandTheme`, `BrandPalette`, `SetCornerStyle`, `PushTextRole`, `SerializeTheme`, `ParseTheme`; outline strokes and hard shadows; `Theme::skin`, `FontSet::display` |
| `imkit/skin.h` | `Skin`: component frames, backdrop patterns, nine-slice pictures |
| `imkit/components.h` | `DrawShadow`, `IMKIT_DEPRECATED`, `AxisVector3Field`, `AxisVector3Interaction`, `CompactIconActionButton` |
| `imkit/workflow.h` | `BeginCollapsibleInspectorCard`, `EndCollapsibleInspectorCard`; `HierarchyRowView::accentColor`; hierarchy rows select across the full width |
| `imkit/icons.h` | 32 LED and live-surface icons and `IconId::PtzCamera`: 321 icons in 21 categories |
| `imkit/show.h` (`imkit::show`) | Controls: `Fader`, `Knob`, `XYPad`, `PadGrid`, `MeterBar`, `TimecodeDisplay`, `HoldButton`, `ConsoleButton`, `ScribbleStrip`, `MasterBar`. Live: `CueList`, `GoBar`, `ClipLauncher`, `Mixer`, `EffectRack`, `ModulatorEditor`, `MappingTable`, `DeviceHealth`. Rules: `RuleBuilder`, `EvaluateRule`, `FormatRuleSentence`, `ValidateRule`. Schedule: `Calendar`, `ResourceTimeline`, `RecurrenceEditor`, `CronEditor`, pickers, `WeekPainter`. Beat: `BeatClock`, `Deck`, `DjChannelStrip`, `StepSequencer`, `Spectrum`. Script panel. Pictures: `ImageButton`, `ImageCropEditor`, `FitImage`. Surface: `ControlSurface`, `SurfacePalette`. LED: `LedColorPicker`, `GradientEditor`, `PixelPreview`, `BudgetBar`, `ColorOrderPicker`, `TestPatternBar`, `PortStrip`. Address: `AddressMap`, `ChannelMonitor`, `FindOverlaps`, `FirstFreeRange`, `PixelsPerUniverse` |

Gallery pages: Start, Theme Studio, Skins, Charts, PTZ camera and nine Show pages. Continuous controls report `ValueEdit`.

### Changed

Default tokens (set them explicitly if your application relied on the old values):

| Token | 3.3.0 | 4.0.0 |
| --- | --- | --- |
| `Radius` subtle / control / overlay | 2 / 4 / 6 | 4 / 8 / 12 |
| `Elevation` raised / overlay | 2 / 5 | 4 / 14, plus `offset` and `hard` |
| `Opacity::disabled` | 1 | 0.55 |
| `Metrics::controlHeight` (Comfortable) | 28 | 30 |
| `Metrics::horizontalPadding` | 10 | 12 |
| `Motion` control / overlay seconds | 0.06 / 0.10 | 0.14 / 0.20 |

- `imkit::Combo`, `BeginCombo`, `EndCombo`, `RadioButton`, `SliderInt*`, `SliderFloat2-4` and `VSliderFloat` are now ImKit wrappers instead of forwards to `ImGui::`. The chevron combos do not mark the item as edited.
- `SemanticRole` gains `Slider` and `Meter`; `RangeSlider` publishes `Slider` nodes instead of `Group`. `PtzState` keeps only `speed` public.
- Timeline hit rectangles are unchanged apart from a narrower chevron on the track header.
- `IMKIT_VERSION` read `3.0.0` since 3.0; it now reads `4.0.0`.
- Inspector cards draw a tinted header band. The window frame fixes title-bar top-edge dragging and move / resize hit testing.

### Deprecated

- The `const Theme*` overloads of `StatusBadge`, `NotificationCard` and `ValidationMessage`: pass `ComponentOptions`.
- `EmptyState(message)`, `Progress(id, fraction, label)` and `ToastRegion(span<Notification>)`: use the newer spellings.

Define `IMKIT_DEPRECATED(message)` as nothing to silence the warnings.

### Compatibility

Public additions are at the end of existing structs (`HoldButtonOptions` gains `key` and `hint`, `GoBarView` gains `nextNumber` and `totalSeconds`, `CueListOptions` gains `showHeader`), so objects built with 3.x must not be mixed with 4.0 structures.

### 日本語

**ImKit 4.0は、既定の見た目を刷新し、ブランドテーマを加え、グラフ、カメラ操作、ライブ演出向けの新しいモジュールを追加します。** 構造体が拡張され、既定のトークンも変わっています。アプリと、リンクするすべてのImKitライブラリを、4.0のヘッダーで再ビルドしてください。Dear ImGuiのrevisionと依存は変わりません。[移行ガイド](docs/reference/v4移行ガイド.md)と[4.0追加API](docs/reference/4.0追加API.md)を参照してください。

#### 主な変更

- **新しい既定の見た目。** 角丸を大きく、影を実際に落とし、コントロールの動きを0.14秒に。トーン塗りのボタン、ピル形のセグメント、塗りつぶし表示のスライダー、シェブロン付きのコンボを備え、進捗・トースト・通知・空状態・ページ送り・検索欄を描き直しました。
- **ブランドテーマ。** `MakeBrandTheme` が4色から完成したテーマを作ります。角、枠線、硬い影、文字の役割はトークンになりました。スキンで部品の枠を差し替えられ、GalleryにTheme StudioとSkinsを追加しました。
- **新しいモジュール。** グラフ、PTZ操作、範囲スライダー、メディアサーバー向けの任意モジュール `imkit::show` です。
- **Galleryを作り直しました。** 新しいStart画面、検索できるナビゲーション、日英の約3分の紹介動画があります。

#### 追加

| ヘッダー | 追加するもの |
| --- | --- |
| `imkit/charts.h` | `DonutChart`、`BarChart`、`Gauge`、`Sparkline`、`ChartColor` |
| `imkit/ptz.h` | `PtzControl`、`PtzPadVector` |
| `imkit/range_slider.h` | `RangeSlider`、`RangeSliderInt` |
| `imkit/theme.h` | `MakeBrandTheme`、`BrandPalette`、`SetCornerStyle`、`PushTextRole`、`SerializeTheme`、`ParseTheme`。枠線（アウトライン）と硬い影。`Theme::skin`、`FontSet::display` |
| `imkit/skin.h` | `Skin`: 部品の枠、背景パターン、ナインスライス画像 |
| `imkit/components.h` | `DrawShadow`、`IMKIT_DEPRECATED`、`AxisVector3Field`、`AxisVector3Interaction`、`CompactIconActionButton`（インスペクター用の三軸入力と小型アイコンボタン） |
| `imkit/workflow.h` | `BeginCollapsibleInspectorCard`、`EndCollapsibleInspectorCard`（開閉式のインスペクターカード）、`HierarchyRowView::accentColor`（種類色）。階層行は全幅で選択できる |
| `imkit/icons.h` | LEDとライブ操作のアイコン32個と `IconId::PtzCamera`。合計321個、21カテゴリ |
| `imkit/show.h`（`imkit::show`） | 操作部品: `Fader`、`Knob`、`XYPad`、`PadGrid`、`MeterBar`、`TimecodeDisplay`、`HoldButton`、`ConsoleButton`、`ScribbleStrip`、`MasterBar`。ライブ: `CueList`、`GoBar`、`ClipLauncher`、`Mixer`、`EffectRack`、`ModulatorEditor`、`MappingTable`、`DeviceHealth`。ルール: `RuleBuilder`、`EvaluateRule`、`FormatRuleSentence`、`ValidateRule`。スケジュール: `Calendar`、`ResourceTimeline`、`RecurrenceEditor`、`CronEditor`、各ピッカー、`WeekPainter`。ビート: `BeatClock`、`Deck`、`DjChannelStrip`、`StepSequencer`、`Spectrum`。スクリプトパネル。画像: `ImageButton`、`ImageCropEditor`、`FitImage`。サーフェス: `ControlSurface`、`SurfacePalette`。LED: `LedColorPicker`、`GradientEditor`、`PixelPreview`、`BudgetBar`、`ColorOrderPicker`、`TestPatternBar`、`PortStrip`。アドレス: `AddressMap`、`ChannelMonitor`、`FindOverlaps`、`FirstFreeRange`、`PixelsPerUniverse` |

Galleryには、Start、Theme Studio、Skins、Charts、PTZ camera、Showの9画面を追加しました。連続値の部品は `ValueEdit` を返します。

#### 変更

既定のトークン（従来の値に頼るアプリは、明示的に指定してください）:

| トークン | 3.3.0 | 4.0.0 |
| --- | --- | --- |
| `Radius` 小・コントロール・オーバーレイ | 2 / 4 / 6 | 4 / 8 / 12 |
| `Elevation` raised / overlay | 2 / 5 | 4 / 14（`offset`と`hard`を追加） |
| `Opacity::disabled` | 1 | 0.55 |
| `Metrics::controlHeight`（Comfortable） | 28 | 30 |
| `Metrics::horizontalPadding` | 10 | 12 |
| `Motion` コントロール・オーバーレイの秒数 | 0.06 / 0.10 | 0.14 / 0.20 |

- `imkit::Combo`、`BeginCombo`、`EndCombo`、`RadioButton`、`SliderInt*`、`SliderFloat2〜4`、`VSliderFloat` は、`ImGui::` への転送をやめ、ImKitのラッパーになりました。シェブロン付きのコンボは、項目を編集済みとして記録しません。
- `SemanticRole` に `Slider` と `Meter` を追加し、`RangeSlider` は `Group` ではなく `Slider` ノードを公開します。`PtzState` の公開フィールドは `speed` だけです。
- タイムラインの当たり判定は、トラック見出しのシェブロンを狭めた点を除いて変わりません。
- `IMKIT_VERSION` は3.0以降 `3.0.0` のままでした。`4.0.0` に直しました。
- インスペクターカードに見出し帯を付けました。ウィンドウ枠は、タイトルバー上端のドラッグと、移動・サイズ変更の判定を修正しました。

#### 非推奨

- `StatusBadge`、`NotificationCard`、`ValidationMessage` の `const Theme*` 版: `ComponentOptions` を渡してください。
- `EmptyState(message)`、`Progress(id, fraction, label)`、`ToastRegion(span<Notification>)`: 新しい綴りを使ってください。

警告を消すには、`IMKIT_DEPRECATED(message)` を空に定義します。

#### 互換性

公開構造体への追加は、末尾だけです（`HoldButtonOptions` に `key` と `hint`、`GoBarView` に `nextNumber` と `totalSeconds`、`CueListOptions` に `showHeader`）。それでも、3.xでビルドしたオブジェクトと4.0の構造体は混ぜないでください。

## 3.3.0 — 2026-10-23

Shared progress tracks, stage lists, task progress cards and richer loading toasts make long-running work easier to follow. `ProgressTrack`, `StageProgress` and `TaskProgressCard` take borrowed views; the host owns values and handles operation IDs. `ToastView` adds `stage`, `progressText` and `status`. Recompile applications and static libraries with matching 3.3 headers; do not mix 3.2 objects with 3.3 structures. Dependencies and the Dear ImGui baseline are unchanged. See [the 3.3 guide](docs/reference/v3.3-api.md).

共通の進捗トラック、工程一覧、操作付きタスクカード、進捗情報を持つトーストを追加しました。`ProgressTrack`・`StageProgress`・`TaskProgressCard`は表示用の参照を受け取り、値の管理と操作IDの処理はホストが行います。`ToastView`に`stage`・`progressText`・`status`を追加したため、アプリと静的ライブラリを同じ3.3ヘッダーで再コンパイルしてください。3.2のobjectと3.3の構造体は混在させないでください。依存とDear ImGuiの基準revisionは変更していません。[3.3追加API](docs/reference/3.3追加API.md)を参照してください。

## Media revision 2 / 映像改訂2 — 2026-10-05

The 60-second English film now uses continuous native operation footage, a large synchronized cursor, kinetic typography, close-ups, connected transitions and an original 128 BPM electro-house score. All twelve principal GIFs were recaptured at 960×540, 20 fps, 6–8 seconds and under 2 MiB each. README and Pages point to the revised film; Pages uses a content hash in the video URL. The v3.2.0 library tag, source archive and five SDK archives retain their original identities.

英語60秒動画を連続したnative操作映像から作り直しました。大きなカーソル、動く文字、部品への寄り、場面をつなぐ切り替え、オリジナルの128 BPMエレクトロハウスを組み合わせています。主要GIF12本も撮り直し、960×540、20 fps、6〜8秒、各2 MiB以下にそろえました。READMEとPagesから改訂版を案内し、Pagesの動画URLには内容ハッシュを付けます。v3.2.0のライブラリタグ、sourceアーカイブ、5構成のSDKアーカイブの識別情報は維持します。

## 3.2.0 — 2026-10-04

**Added:** WorkspaceTabs, hierarchy visibility/lock/group action requests, inspector cards, SettingToggleRow, DragVector3WithUnit, titled responsive ChoiceGroup, CompactActionRow, ToastViewport, SubtleDestructive action style, Slate (13th theme), and USB/Network/Smartphone/IndustrialCamera icons (288 total).

**Gallery and documentation:** category navigation, independent Theme only / Components comparison, host-applied 3.2 workspace, refreshed native captures, bilingual public guides and declarations, and a 60-second English motion graphics film with original electronic music. Pages now includes toast documentation and the 3.2 API reference.

**Compatibility:** `WindowFrameContent::iconTexture` extends the structure. Recompile all consumers and ImKit static libraries against 3.2; do not mix 3.1 object files. The pinned Dear ImGui revision remains unchanged. No new library dependency is introduced.

**日本語**

作業タブ、階層の可視・ロック・見出し操作要求、設定カード、設定行、単位付き三軸入力、見出し付き選択、状態と操作の行、ToastViewport、SubtleDestructive、13番目のテーマSlateを追加しました。USB、Network、Smartphone、IndustrialCameraを含む288アイコンを利用できます。

Galleryのカテゴリ案内、独立した2モードの比較、要求をアプリが反映する3.2作業画面を整えました。実キャプチャGIF、英日ガイドとAPI宣言、オリジナルBGM付きの約60秒英語モーショングラフィックス動画を公開します。Pagesにもトーストと3.2 API参照を含めます。

`WindowFrameContent::iconTexture` の追加により構造体が拡張されています。アプリとImKit静的ライブラリを3.2のヘッダーで再コンパイルしてください。3.1のobjectは混在させないでください。Dear ImGuiの対応revisionは変更せず、新しいライブラリ依存も追加していません。

## 3.1.0 — 2026-09-26

### Added
- A bilingual documentation catalog maps each public guide to its audience, API contract, Gallery page, source file, and SDK package path.
- Module recipes cover Components, Themes, Workflow, Node Editor, Editor Suite, Timeline, and WindowFrame, including frame placement and host-owned responsibilities.
- Gallery Start adds task-focused routes and a direct path to the recipe map.

### Compatibility
- No public API or ABI changes. The Dear ImGui ABI baseline remains the one introduced in v3.0.0.

### 日本語
#### 追加
- 英日文書カタログを追加し、各ガイドの対象読者、API契約、Gallery画面、実装source、SDK内の配置先を対応付けました。
- Components、Theme、Workflow、Node Editor、Editor Suite、Timeline、WindowFrameの実装例を追加しました。フレーム内で呼ぶ位置と、ホスト側に残る責務も確認できます。
- GalleryのStart画面に目的別の案内を設け、実装例一覧へ直接進めるようにしました。

#### 互換性
- 公開API・ABIに変更はありません。Dear ImGuiのABI基準はv3.0.0から変わっていません。

## 3.0.0 — 2026-09-13

### Added
- A standalone imkit::node_editor target adds a host-controlled graph canvas, typed connections, dynamic sockets, inline values, minimap, search, previews, and revision-checked edit requests.
- Context-free helpers arrange, align, distribute, and snap nodes. The host retains the graph model, validation, evaluation, undo, and persistence.
- Workflow and Editor Suite additions include circular progress, a resizable Inspector, Timeline drop previews, and a host toolbar layout contract.
- Native Metal Gallery and preview support, macOS WindowFrame and accessibility bridges, and Windows/macOS platform packages.

### Fixed
- Invalid Timeline drop candidates are rejected before commit. Node Editor collapse and minimap interaction issues were corrected, and macOS WindowFrame coordinate and bundle-resource handling was fixed.

### Breaking changes and migration
- The supported ABI is pinned to Dear ImGui 1.93.0 WIP docking commit 367b2c24f399988ddafc0bb4628da0106bcc09be. Rebuild all ImKit-dependent translation units against that revision.
- OpenGL preview declarations moved to the separate preview_opengl3.h header. Win32ActionSink was replaced by NativeActionSink.
- Dear ImGui 1.88 compatibility targets and legacy WindowFrame ABI paths were removed. Follow the v3 migration guide before upgrading.

### 日本語
#### 追加
- 独立したimkit::node_editor targetを追加しました。ホストが管理するgraph canvas上で、型付き接続、動的socket、値の直接編集、minimap、検索、preview、revisionを検証する編集要求を扱えます。
- Dear ImGui Contextに依存しない、nodeの配置・整列・分配・snap機能を追加しました。graph model、検証、評価、Undo、保存はホスト側で管理します。
- WorkflowとEditor Suiteに、円形進捗、幅を変更できるInspector、Timelineのdrop候補preview、ホストtoolbarの配置契約を追加しました。
- Metal対応Gallery／preview、macOS WindowFrame／accessibility bridge、Windows／macOSの配布packageを追加しました。

#### 修正
- Timelineで無効なdrop候補を確定できないようにしました。Node Editorの折りたたみ時の異常終了とminimap操作を修正し、macOS WindowFrameの座標変換とbundle内の資産配置も修正しました。

#### 破壊的変更と移行
- 対応ABIをDear ImGui 1.93.0 WIP docking commit 367b2c24f399988ddafc0bb4628da0106bcc09beに固定しました。全ImKit依存translation unitをこのrevisionに合わせて再buildしてください。
- OpenGL previewの宣言をpreview_opengl3.hへ分離しました。Win32ActionSinkはNativeActionSinkへ置き換わります。
- Dear ImGui 1.88互換targetと旧WindowFrame ABI pathを削除しました。更新前にv3移行ガイドを確認してください。

## 2.2.0 — 2026-09-12

### Added
- Preview placement now supports Fit, Fill, and Stretch. Monitor presentation also supports crop and vertical flip.
- Five explicit preview states—Ready, Loading, Empty, Offline, and Error—make unavailable or incomplete media visible to the user.
- Gallery examples cover representative source and viewport aspect ratios alongside the live comparison, Workflow, and Timeline surfaces.

### 日本語
#### 追加
- Previewの配置方法にFit、Fill、Stretchを追加しました。Monitorではcropと上下反転も指定できます。
- Ready、Loading、Empty、Offline、Errorの5状態を追加し、素材の準備中や利用できない状態を画面上で示せるようにしました。
- 代表的なsource／viewportのアスペクト比を使ったGallery実例を追加し、ライブ比較、Workflow、Timelineの画面と合わせて確認できます。

## 2.1.0 — 2026-09-12

### Added
- Gallery Start guides new users to a movable, resizable comparison between Default Dear ImGui and ImKit. Both sides update the same host-owned values.
- Reproducible native capture routes cover the overview, comparison, theme palettes, Workflow feedback, and Timeline editing.
- A Windows x64 Gallery archive includes the required design assets, runtime dependency inventory, license, and third-party notices.

### 日本語
#### 追加
- GalleryのStart画面から、Default Dear ImGuiとImKitを並べた比較を開けます。windowは移動・resizeでき、両側から同じホスト所有値を更新します。
- 概要、比較、Theme palette、Workflowのfeedback、Timeline編集を、再生成できるnative capture routeとして追加しました。
- 必要なdesign asset、runtime dependency一覧、license、第三者noticeを含むWindows x64 Gallery archiveを用意しました。

## 2.0.0

### Added
- Host-owned application shell components: AppBar, WorkspaceHeader, InspectorSection, BottomActionBar, AdvancedSection, DiagnosticsDrawer, and ThemePicker.
- The Editor Suite expands with timeline overview and source placement, host-owned undo/redo, waveform and pixel-envelope providers, property curves, and batch reservations.
- Twelve named themes have stable IDs; ThemePresetFromId supports host-owned preset persistence. The native Gallery opens on Components and provides searchable examples.
- Versioned Inter and Noto Sans JP font assets include OFL notices, hashes, and a CMake copy helper. Font atlas and GPU lifetime remain host-owned.
- Source and Windows SDK packages add install/consumer verification for the expanded library.

### Fixed
- Multi-clip timeline edits validate the full selection before commit, preventing partial application when a clip or selection limit is reached.
- Clip and transition hit regions now follow Dear ImGui layout and scale. Editing a deselected keyframe is rejected, and Inspector reset behavior is corrected.

### 日本語
#### 追加
- AppBar、WorkspaceHeader、InspectorSection、BottomActionBar、AdvancedSection、DiagnosticsDrawer、ThemePickerなど、ホスト側で構成するアプリシェル部品を追加しました。
- Editor Suiteを拡張し、Timeline overview、素材配置、ホスト所有のUndo／Redo、waveformとpixel envelopeのprovider、property curve、batch reservationを追加しました。
- 安定ID付きの名前付きThemeを12種類用意しました。ThemePresetFromIdでホスト側に保存したpresetを復元できます。GalleryはComponents画面から開き、実例を検索できます。
- InterとNoto Sans JPのversion固定font資産に、OFL notice、hash、CMake配置helperを追加しました。font atlasとGPU資源の寿命はホスト側で管理します。
- 拡張したlibraryに合わせてsource／Windows SDK packageと、install後のconsumer確認を整えました。

#### 修正
- 複数clipのTimeline編集はcommit前に選択範囲全体を検証し、容量や選択queryの不足による部分適用を防ぐようにしました。
- clipとtransitionの入力領域をDear ImGuiのlayoutと倍率に合わせました。選択解除済みkeyframeの編集開始を拒否し、Inspectorのreset動作も修正しました。

## 1.0.0 — 2026-09-10

- Complete Editor Core, Video, CG and optional OpenGL3 preview with host-owned data and typed edits.
- Connect related timeline edits, transitions/captions/audio/color curves, hierarchy/Inspector, gizmos, animation and UV workflows in the native Gallery.
- Preserve 120 IconIds and add 86 originals: 206 icons, 1236 PNG variants, six atlases.
- Validate Debug/Release, public IO, native GPU and 100k interaction performance; document source/SDK consumption and GL lifetime.
- Core・Video・CG・任意OpenGL3 previewとホスト所有の型付き編集を完成。
- Timeline関連編集・transition/caption/audio/color curve、階層/Inspector・gizmo・animation・UVをnative Galleryへ接続。
- 既存120 IconIdを保持して86原画を追加。合計206アイコン・1236 PNG・6 atlas。
- Debug/Release・公開IO・実GPU・10万件操作性能を検証し、導入・所有権・GL寿命を文書化。

## 0.2.0 — Precision Layers

### Added
- Precision Layers applies light/dark semantic palettes, scale, and font scopes across the public GUI families.
- The release tracks 365 public GUI and helper overloads with signature and link coverage while preserving the original six wrappers.
- Composite controls include selection marks, action variants, switches, mixed selection, segments, searchable selection, units, settings rows, badges, notifications, and toolbars.
- An optional six-category live catalog includes an editable palette, Japanese glyph fallback, public-IO verification, and native GPU captures.

### SDK scope
- C++20 static library targeting Dear ImGui 1.92.9b docking commit b48d1afbe8ee8b238e2961dc363a949dd7304e23.
- The binary SDK is Windows x64 / MSVC v145, with Debug and Release CRT variants. ImGui core is not embedded; use a matching host target.
- The source archive is the recommended route for other platforms, compilers, or ABI configurations. Changing only the version guard does not port the ABI.

### 日本語
#### 追加
- 公開GUI部品群にPrecision Layersを適用し、light／darkの意味別palette、倍率、font scopeを追加しました。
- 公開GUI／helperの365 overloadをsignatureとlinkの両面から追跡し、従来の6 wrapperを維持しました。
- 選択mark、action variant、switch、mixed selection、segment、検索可能な選択欄、単位表示、設定行、badge、notification、toolbarを追加しました。
- 任意で起動できる6カテゴリのlive catalogに、編集可能なpalette、日本語glyph fallback、公開IOによる操作確認、native GPU captureを用意しました。

#### SDKの対象
- Dear ImGui 1.92.9b docking commit b48d1afbe8ee8b238e2961dc363a949dd7304e23を対象とするC++20静的libraryです。
- binary SDKはWindows x64／MSVC v145向けで、Debug／ReleaseのCRT構成を収録します。ImGui coreは含まず、ホスト側の対応targetを使用します。
- ほかのOS、compiler、ABI構成ではsource archiveから導入してください。version guardだけを変更してもABI移植にはなりません。

## 0.1.0

Initial six-wrapper development foundation / 6関数の開発基盤。

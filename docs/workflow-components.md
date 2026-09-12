# Generic workflow components / 汎用ワークフロー部品

Include `imkit/workflow.h` (also included by `imkit/imkit.h`) and link `imkit::imkit`.
For images/previews include `imkit/editor_canvas.h` and link `imkit::editor_core`.
These additive APIs reuse patterns, Canvas, Selection and Splitter. They do not
establish completion of the whole Editor Suite.

基本部品は`imkit/workflow.h`、画像・previewは`imkit/editor_canvas.h`をincludeし、
それぞれ上記targetへlinkします。既存patterns・Canvas・Selection・Splitterを利用する
追加APIで、Editor Suite全体の完成を意味しません。

## Ownership / 所有権

Labels, textures, candidate views and scratch buffers are borrowed. IDs must be
nonzero and unique per instance, except notifications: the last duplicate wins.
State and queues belong to the host. No texture upload, worker, persistence, registry,
undo engine, dependency or icon asset is added. End every scope before destroying
its context. Semantic text must remain valid through host frame publication.

ラベル・texture・候補view・scratchは借用です。IDはinstance内で非zeroかつ一意とし、
通知だけは同一IDの最後の要素が優先されます。状態・queueはホスト所有です。
upload・worker・永続化・registry・Undo・新規依存・icon資産は追加していません。
Context破棄前にscopeを閉じ、semantic文字列はframe公開完了まで保持します。

Reset event buffers each frame. Full buffers set `overflow`. Keyboard tile resizing
requires three slots for Begin/Update/Commit. Mouse cancellation restores both
adjacent sizes; terminal events are retried if the buffer is full. Tile sizes are
host-owned, ID-keyed entries. Notification sorting uses caller index scratch and
limits results to the smaller of scratch capacity and requested maximum.

イベントbufferはframeごとに初期化し、容量不足は`overflow`で確認します。
keyboard resizeはBegin/Update/Commitの3件、マウス取消は隣接寸法を復元し、
終了イベントはbufferが満杯なら次frameで再送します。Tile寸法はID付きホスト状態です。
通知の表示上限は指定数とindex scratch容量の小さい方です。

## API inventory / API一覧

`tests/workflow_api_compile.cpp` is the compile/link inventory, also built by source
and relocated SDK consumers. Existing signatures and native API inventory are retained.

上記fixtureがcompile/link一覧で、source・relocated SDK consumerも使用します。
既存署名とnative API inventoryは維持しています。

| API | Contract / 契約 |
|---|---|
| `StepNavigator`, `NavigationRail` | Borrowed StepItem span, current ID, explicit focus state; returns selection request / 選択要求のみ |
| `FilterChip`, `SectionHeader` | Explicit selection/open state, native activation / 明示状態とnative操作 |
| `SelectNotifications` | Last duplicate wins before expiry filtering; priority descending, stable ties / 重複更新・期限・優先順位 |
| `NotificationCard(FeedbackView)`, `InlineAlert`, `PersistentBanner` | Semantic status and dismiss request; banner persists / 意味色とdismiss要求 |
| `ToastRegion(id, ...)` | Host queue/time/scratch, work-area-clamped stack, no focus stealing / ホスト時刻とqueue |
| `EmptyState(id, StateView)`, `UnavailableState`, `RetryState` | Heading, description, optional icon/action; returns request / 見出し・説明・任意action |
| `Progress(ProgressView, ...)` | Inline, child overlay, modal; negative fraction means indeterminate; cancel request / 表示と取消要求のみ |
| `BeginCard`, `EndCard` | Always paired, even when Begin returns false / falseでもEnd必須 |
| `MultiSelectionBar`, `HelpCallout`, `ValidationSummary` | Host count, commands or issues; no validation engine / 検証処理を所有しない |
| `ResponsiveToolbar(..., ToolbarOptions)` | Overflow, icon labels, disabled reasons, native keyboard focus / overflowとaccessible label |
| `ResolveRightSidePanelLayout`, `RightSidePanelHandle` | Host-owned open/width state, compact edge toggle and mouse/keyboard resize; host scopes shortcut requests / 開閉・幅はホスト所有、右端toggleとmouse／keyboard resize、shortcut範囲はホストが決定 |
| `ImageGeometryValid`, `FitImage`, `ClampImage` | Numeric image layout independent of texture availability / textureと独立した数値layout |
| `ResolveImagePlacement` | Pure Fit/Fill/Stretch destination and UV crop resolver; invalid geometry is rejected / 純粋な配置・UV crop計算。無効geometryは拒否 |
| `PixelToNormalized`, `NormalizedToPixel` | Divide/multiply by image dimensions; invalid dimensions return zero / 無効寸法はzero |
| `BeginImageViewport`, `EndImageViewport` | Existing Canvas, borrowed texture, uniform zoom; always paired / 同一Canvasを使用 |
| `ZoomToolbar` | Fit/fill/1:1, zoom and pan; updates display state / 表示状態だけを更新 |
| `DrawOverlay` | Point/polyline/rectangle/circle/label, selected/hovered decoration, clipped / 描画補助のみ |
| `PreviewTile`, `ResizableTileStrip` | Borrowed previews/actions, horizontal/vertical scroll, minimum extents / 借用previewと寸法変更 |
| `RequestBuffer::Push`, `TileEventBuffer::Push` | Caller storage, explicit overflow / 固定容量と不足通知 |

Image coordinates start at top left, x right and y down; Canvas units are image
pixels. Use existing `ToScreen`/`FromScreen` for desktop-screen conversion. Fit shows
the whole image, Fill crops, 1:1 maps one image pixel to one ImGui coordinate unit
(not necessarily one physical display pixel). Clamp centers small images and limits
large images to their edges. Cursor-anchored zoom is preserved until edge clamping
is needed. Resizing refits non-manual modes. `CanvasSelection` handles marquee/lasso.

画像は左上原点・右下正方向、Canvas単位は画像pixelです。screen変換は既存APIを使用します。
Fitは全体、Fillは切り抜き、1:1は画像pixelとImGui座標単位を対応させます。
高DPIの物理pixelとは異なります。小さい画像は中央、大きい画像は端までpan可能です。
zoom中心は端clampが必要になるまで保持します。Manual以外はresizeで再fitし、
矩形・lasso選択は既存`CanvasSelection`を使用します。

`ResolveImagePlacement` uses local coordinates whose origin is the available region's
top-left. Fit returns a centered destination with full UVs, Fill returns the full
destination with centered crop UVs, and Stretch uses both complete regions. It owns
no texture or renderer and returns `valid=false` for non-positive, non-finite or
non-drawable geometry.

`ResolveImagePlacement`の原点は利用可能領域の左上です。Fitは中央配置と全UV、Fillは
全表示領域と中央crop UV、Stretchは表示領域・UVの全域を返します。texture／rendererは
所有せず、0以下・非有限・描画不能なgeometryでは`valid=false`を返します。

Pass `ComponentOptions` for theme, locale and semantic publication. Draw-only
overlays do not create interactive object nodes: the host describes edited objects.
Step arrows skip disabled items; native Tab/Shift-Tab and activation remain intact.
Splitter additionally supports arrow-key resizing. Reduced motion uses existing
static indicators; no new transitions are introduced.

Theme・locale・semantic公開は`ComponentOptions`で渡します。描画専用overlayの編集対象nodeは
ホストが定義します。Stepの矢印はdisabledを飛ばし、Tab／Shift-Tabと実行はnative操作です。
Splitterは矢印resizeにも対応します。Reduced Motionでは既存の静止表示を利用します。

`ResolveRightSidePanelLayout` returns widths for content, the always-visible edge
handle and the optional panel. Draw those regions in that order and pass the same
available extent to `RightSidePanelHandle`. The host decides whether an `N` key or
another shortcut becomes `toggleRequested`; this prevents the library from stealing
text input or shortcuts from unrelated editors. The handle remains visible while
collapsed, and the panel reduces content width instead of covering it.

`ResolveRightSidePanelLayout`は本文、常時表示する右端handle、任意panelの幅を返します。
その順で描画し、同じ利用可能寸法を`RightSidePanelHandle`へ渡します。`N`キー等を
`toggleRequested`へ変換する範囲はホストが決めるため、無関係なeditorや文字入力から
shortcutを奪いません。閉じた状態でもhandleは残り、panelは本文へ重ならず幅を縮めます。

## Gallery / Gallery

Open **Generic Workspace**, **Feedback / States**, or **Preview Tiles**. The workspace
combines navigation, toolbar, image, overlays, selection, feedback and status. It uses
the existing procedural texture. `--verify-workflow --output <directory>` runs public
IO checks and captures 36 page/theme combinations plus two narrow Japanese/disabled
examples. Native GPU captures are distinct from native OS/IME and screen-reader tests.

上記3ページで全追加部品を公開API経由で操作できます。workspaceではnavigation・toolbar・
画像・overlay・選択・通知・状態表示を組み合わせ、既存の手続き生成textureを使います。
上記引数で公開IOと3ページ×12設定の36枚、狭幅日本語・disabled例の2枚を取得できます。
GPU captureはnative OS/IME・実スクリーンリーダー試験とは別です。

# Design system refresh / デザインシステム刷新

This change adds cross-cutting foundations to Editor Suite 2.0. It is **not full acceptance of the comprehensive next-release specification**. The implementation and remaining work below are separate from previous Editor Suite verification.

Editor Suite 2.0に横断基盤を追加した変更です。**次期リリース総合仕様の全項目完了ではありません**。以下の実装・残作業は、以前のEditor Suite検証とは区別します。

## Design / 設計

| Reference / 比較元 | Decision / 採用方針 |
|---|---|
| [Fluent tokens](https://fluent2.microsoft.design/design-tokens) | Explicit semantic tokens; deterministic style projection / 明示tokenからstyleを導出 |
| [Qt Model/View](https://doc.qt.io/qt-6/model-view-programming.html) | Host provider owns data and visible index / データと可視indexをホストが所有 |
| [Radix accessibility](https://www.radix-ui.com/primitives/docs/overview/accessibility), [APG Dialog](https://www.w3.org/WAI/ARIA/apg/patterns/dialog-modal/) | Native input widgets plus explicit composite focus state / 標準入力と明示的focus状態 |
| [Qt internationalization](https://doc.qt.io/qt-6/internationalization.html) | Explicit host locale callbacks, no global locale / 明示callback、global localeなし |
| [UI Automation fragments](https://learn.microsoft.com/en-us/windows/win32/api/uiautomationcore/nn-uiautomationcore-irawelementproviderfragment) | Copied native snapshots, borrowed HWND and action callback / native snapshotはコピー、HWNDとcallbackは非所有 |

## API and migration / APIと移行

`MakeTheme(scheme, contrast, density)` provides 12 presets. `SetDensity` changes 24/28/44px control geometry without changing typography. Edit `semantic`, `typography`, `radius`, `stroke`, `elevation`, `opacity`, then call `ResolveTheme`. The legacy `colors` and `metrics` remain rendering projections for existing editor code; do not edit both independently. `MakePrecisionTheme` remains available. Host font pointers are never loaded or freed by ImKit.

`MakeTheme(scheme, contrast, density)`は12種の設定を生成します。`SetDensity`はTypographyを変えず24/28/44pxの寸法へ切り替えます。意味tokenを編集したら`ResolveTheme`を呼びます。既存`colors`と`metrics`はEditor描画用の導出値として残します。両方を別々に編集しないでください。fontの読込・解放はホスト責務です。

`motion.reducedMotion` disables component animation. `ValidateContrast` checks text at 4.5:1 and focus/borders at 3:1 against the declared surfaces/control states, including alpha compositing. It is not a proof for arbitrary custom colors, background images, or every composed widget state. `AnimationState` supports Linear, EaseOut and EaseInOut interpolation.

`motion.reducedMotion`は部品animationを停止します。`ValidateContrast`は宣言されたsurface・control状態に対してalpha合成後の文字4.5:1、focus・境界3:1を検査します。任意のcustom色、背景画像、全合成状態を保証するものではありません。`AnimationState`はLinear・EaseOut・EaseInOut補間に対応します。

Pass a host-owned `AccessibilityFrame` through `ComponentOptions::accessibility`. Call `Begin(generation)` before drawing and `Publish(sink)` after drawing. Node strings and child arrays are borrowed for that publication only. Storage overflow is explicit. `Validate()` checks IDs, parents, label references and cycles; it is an optional diagnostic, not a per-frame large-tree requirement. `ActionQueue` has fixed host-provided storage and must be serialized by the host. Clear stale requests at a host-defined frame boundary.

ホスト所有`AccessibilityFrame`を`ComponentOptions::accessibility`へ渡します。描画前に`Begin(generation)`、描画後に`Publish(sink)`を呼びます。文字列・子配列はpublication中だけ有効な非所有参照です。容量超過は明示されます。`Validate()`はID・親・label・循環の任意診断で、大規模treeの毎frame必須処理ではありません。`ActionQueue`の容量と同期、古い要求の破棄はホストが管理します。

Enable `IMKIT_BUILD_ACCESSIBILITY_WIN32` and link `imkit::accessibility_win32`. `CreateWin32Provider` copies the frame into a COM snapshot. The host initializes COM, handles `WM_GETOBJECT`, obtains `IRawElementProviderSimple` through `QueryInterface`, and passes it to `UiaReturnRawElementProvider`. Release caller-owned COM references. Native clients can retain snapshots; the action callback/user data must outlive them. Snapshot refresh, UIA events and application shutdown invalidation remain host integration work. Invoke, Toggle, Focus, fragment navigation and common properties are implemented; other UIA patterns are not.

`IMKIT_BUILD_ACCESSIBILITY_WIN32`を有効にし、`imkit::accessibility_win32`へlinkします。`CreateWin32Provider`はframeをCOM snapshotへコピーします。COM初期化、`WM_GETOBJECT`、`QueryInterface`で取得した`IRawElementProviderSimple`の`UiaReturnRawElementProvider`への受渡し、参照解放はホスト責務です。native clientが保持するsnapshotよりcallback/user dataを長生きさせてください。snapshot更新、UIA event、終了時の無効化はホスト統合作業です。Invoke・Toggle・Focus・fragment移動・基本propertyを実装し、他のUIA patternは未実装です。

`LocaleContext` accepts language tag, layout direction, translation, number and date/time callbacks. Breadcrumbs and data column ordering support pseudo RTL. This is not bidirectional text shaping or complete RTL layout. Coordinate-oriented editor canvases retain their direction.

`LocaleContext`はlanguage tag、配置方向、翻訳、数値・日時書式callbackを受け取ります。breadcrumbとデータ列順序は疑似RTLに対応します。双方向文字組版や全layoutのRTL対応ではありません。座標系を持つEditor canvasの方向は維持します。

## Components / 部品

`patterns.h` exposes CommandPalette, SearchField, Breadcrumbs, SplitButton, Toolbar/ResponsiveToolbar, Dialog/AlertDialog, Popover, Menu, FormField, Progress, Spinner, Skeleton, empty/loading/error states, ToastRegion, Pagination, AdaptiveSplitLayout, VirtualList, DataTable and TreeDataGrid. All edited data and queues remain host-owned. Draw functions return requests or synchronously invoke the provider's `apply`; they do not implement persistence or undo.

`patterns.h`は上記の汎用部品を公開します。編集データ・queueはホスト所有です。描画関数は要求を返すかproviderの`apply`を同期呼出しします。永続化やUndoは内包しません。

Providers supply a flattened visible index after sort/filter/tree expansion. `query` receives a clipped range; `id` and `cell` are called only in that range. Stable row IDs must be nonzero. Callbacks must keep returned text alive for the frame. Host operations can rebuild indexes on events; drawing itself must not scan all rows. Table selection events carry index ranges, so hosts can use interval selection rather than allocate per selected row. Column sizing/visibility uses Dear ImGui tables.

providerはsort・filter・展開後の平坦な可視indexを提供します。`query`はclipされた範囲を受け取り、`id`・`cell`もその範囲だけで呼ばれます。行IDは非zeroで、文字列はframe中有効にしてください。index再構築はホストのevent処理で行い、定常描画では全行走査しません。選択eventはindex範囲を持ちます。列幅・表示切替はDear ImGui tableが管理します。

## Remaining implementation / 残る実装

- Complete semantic emission for all existing wrappers, composite parent/label/action relationships, and all UIA patterns/events; full Gallery UIA integration.
- Exhaustive roving-focus verification including disabled items, Tabs close/focus restoration, complete Tree/DataGrid navigation and typeahead.
- Complete FormField label/validation semantics, indeterminate progress animation, responsive layout policies and menu/form RTL; host number/date formatting usage.
- Full component state matrices and distinct production pattern pages; complete long-label and narrow-width acceptance.
- Existing raster icon migration for optical issues, full per-size optical acceptance, installed/relocated SDK verification after this API change.

- 既存wrapper全体の意味送出、複合部品の親・label・action関係、UIA pattern/event、GalleryのUIA統合。
- disabledを含むfocus移動の網羅検証、Tabs終了後focus復元、Tree/DataGridの完全なkey操作とtypeahead。
- FormFieldのlabel/validation意味情報、不定進捗animation、responsive方針とmenu/form RTL、数値・日時書式の実使用。
- 全状態matrixと独立した製品品質patternページ、長文・狭幅の受入確認。
- 光学的問題のある既存raster iconの移行、全sizeの目視受入、このAPI変更後のinstalled/relocated SDK検証。

Native OS/IME, real screen readers and integration into another application are not verified. Public ImGui IO and native backbuffer captures do not establish those results.

native OS/IME、実スクリーンリーダー、他アプリ統合は未検証です。公開ImGui IOとnative backbuffer captureから合格を推定しません。

## Recorded verification / 今回の検証

| Check / 検証 | Result / 結果 |
|---|---|
| Debug API | Existing public API compile/link and new design API fixture passed / 既存・新規fixture合格 |
| Theme | 12 combinations, 24/28/44px density, contrast and easing/reduced-motion tests passed / 12設定・密度・contrast・補間検査合格 |
| Semantic tree | Stable IDs, duplicate detection, parents, Grid/Row/Cell relationships and action queue passed / ID・重複・親子・queue検査合格 |
| Win32 UIA | Optional target compiled; representative child/name/Invoke callback retrieval passed / compile・代表node取得とInvoke往復合格 |
| Public keyboard IO | Space opens Dialog; Escape closes it and restores launcher focus / Dialog起動・終了・focus復帰合格 |
| Data views | DataTable and TreeDataGrid at 100,000 rows: clipped query, steady ImGui and C++ allocations 0 / 可視query・定常allocation 0 |
| Icons | 280 IDs, seven raster levels, alpha/tint/atlas correspondence and C++ icon tests passed / 280 ID・7段階・画素対応合格 |
| External source consumer | Separate host ImGui target, MakeTheme and semantic publication compiled/ran / 別host targetでcompile・実行合格 |
| Release Gallery | Build passed; 100 native backbuffers in two resolutions / build合格・2解像度100枚 |

Capture directories are `out/design-system-final-1280` and `out/design-system-final-1920`. They contain 96 combinations of four pages and 12 themes across two resolutions, plus Japanese 150% and pseudo RTL 200% at each resolution. Representative frames were visually inspected. These are captures, not proof of every keyboard path, state matrix, or every icon's optical quality. Generated captures are not committed.

capture先は`out/design-system-final-1280`と`out/design-system-final-1920`です。4ページ×12設定×2解像度の96枚と、各解像度の日本語150%・疑似RTL200%を含みます。代表frameを目視確認しました。全keyboard経路、状態matrix、全iconの光学品質を保証する証拠ではありません。生成captureはコミットしていません。

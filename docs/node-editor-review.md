# Socket redesign review / ソケット再設計レビュー

Baseline: `Version3.0-addNodeEditor`, `27ba4d5`.
CodeGraph was used first to trace PinView, BeginPin, PinPosition, NodeInspector,
CanConnect and NodePalette. Baseline PinPosition read a fixed offset while
BeginPin emitted only the label; there was no shared row layout measurement.
The palette already carried a source pin, but compatibility filtering was optional.

起点の上記シンボルとデータフローをCodeGraphから確認した。
PinPositionは固定offset、BeginPinはラベルだけを描き、行との位置共有がなかった。
Paletteには接続元pinが渡ったが、適合callback未指定の候補も表示されていた。

| Priority | Baseline issue / 起点の問題 | Resolution / 対応 |
|---|---|---|
| P0 | No typed socket lifecycle / 動的編集要求不足 | Typed creation, deletion, naming, type, multiplicity, group limits; atomic request buffer / 型付き要求と一括バッファ |
| P0 | Offset and widget row diverge / 行と端点の不一致 | PinRow measurements feed socket and final link rendering / 実測行をソケット・配線で共有 |
| P0 | No value transaction / 既定値の操作契約不足 | Owned value drafts, Begin/Update/Commit/Cancel, connected-input gating / 所有コピーの値要求と接続時制御 |
| P0 | Binary compatibility and fragile reconnection / 二値判定・付け替え | Three verdicts, reasons, semantic feedback, filtered palette and host atomic creation / 三段階判定・理由・候補絞込み |
| P0 | Inspector cannot edit input/output schema / 入出力編集不足 | Separate sections, capabilities, drag ordering, linked-edit confirmation / 方向別編集・能力・並べ替え・影響確認 |
| P0 | No material graph integration example / 汎用APIの使用例不足 | Independent host mock with 12 templates, dynamic/variadic pins, snapshots and CPU previews / 独立ホストMock |
| P1 | Secondary navigation and toolbar density / 周辺操作の密度 | Compact Material toolbar and preview overflow; corrected minimap overlap. A full redesign of the legacy studio navigation remains outside this P0 pass / Material toolbar・preview menu・重なりを改善。従来Studioの周辺navigation全面刷新は未実施 |

The mock deliberately refuses connected pin deletion and type changes. The
library emits advisory affected-link counts and never mutates those links.
No external code, icon, image or dependency was introduced. The reference web
manuals could not be fetched in this run; implementation follows the requested
interaction contract and this repository's own primitives.

Mockは接続済みpinの削除・型変更を意図的に拒否する。ライブラリは影響リンク数を要求へ付け、
モデルを変更しない。外部コード・icon・画像・依存は追加していない。指定の参考web manualは
今回取得できず、依頼の操作契約と本リポジトリの部品から実装した。

## Verification scope / 検証範囲

Dedicated Debug build: `build/node-sockets-debug`. Focused CPU/public-ImGui-IO
checks cover typed input/output operations, shared operation/revision, capacity
failure, host rejection preserving wiring, three connection verdicts, variadic
limits/order, value visibility, Text Delete isolation and value phases including
terminal retry. Row centers are checked at several zoom and metric scales.
The mock checks atomic addition/autoconnection, Undo/Redo and CPU preview changes.

専用Debug buildで型付き入出力操作、共通operation/revision、バッファ不足、リンクを維持する
ホスト拒否、三段階接続判定、可変長個数・順序、値表示条件、Text入力中のDelete遮断、
値phaseと終端再送を確認する。Zoomと寸法scaleを変えて行中心と端点も確認する。
Mockでは追加・自動接続の一括適用、Undo／Redo、CPU preview変化を確認する。

Native Gallery capture exposed a clipped Principled preview and overlapping
minimap controls; these were corrected and visually rechecked. Captures stay in
`out/node-sockets/` and are not committed. The public API fixture is also built
as an independent consumer with its own ImGui target in `build/node-sockets-consumer`.

Native captureでPrincipled previewの切れとminimap上のcontrol重なりを検出し、修正後に
目視再確認した。captureはout/node-sockets/に置き、commitしない。公開API fixtureは
独自ImGui targetを持つ独立consumerとしてもbuild/node-sockets-consumerで確認する。

Native OS/IME, physical DPI, assistive technologies, performance, external-host
integration, Release and distribution acceptance are not established by these checks.

実機OS／IME、実機DPI、支援技術、性能、外部ホスト統合、Release・配布受入は未実施。

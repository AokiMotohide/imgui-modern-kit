# Validation / 検証

Design-system foundation verification and remaining implementation: [design-system.md](design-system.md#recorded-verification--今回の検証). This is separate from the Editor Suite 2.0 record below.

デザインシステム基盤の検証・残る実装は[刷新文書](design-system.md)を参照してください。以下のEditor Suite 2.0記録とは別の検証です。

Editor Suite 2.0 results and boundaries: [Editor refresh](editor-refresh.md#recorded-verification-2026-09-10--今回の検証結果).
Editor Suite 2.0の今回の結果と検証範囲は[刷新記録](editor-refresh.md)を参照してください。以下の従来記録とは区別します。

This record distinguishes compile/link, public-IO interaction, GPU appearance and distribution. Passing native-API signature coverage does not mean every native behavior was exhaustively retested.

compile/link、公開IO操作、GPU外観、配布を区別します。署名確認を、標準APIの全動作を個別に再検証した証拠としては扱いません。

| Gate | Evidence / 確認内容 |
|---|---|
| Public API | 365 included overloads; independent function-pointer signature references compile and link against the pinned header; excluded functions are enumerated |
| Debug / Release | Library and production catalog compile/link on Windows x64, MSVC 19.51.36256 / v145 |
| Ownership | Two independent contexts; non-cumulative scale; nested ThemeScope style/font restoration; surrounding disabled alpha retained |
| Theme presets | 12 stable unique IDs; complete finite palettes; Light/Dark metadata; legacy Precision values; normal/muted/semantic contrast thresholds |
| Motion | Intermediate value, endpoint, disabled immediate result, pruning and generation reset |
| External target | Separate consumer creates its own ImGui target; no core/backend sources in imkit; no GLFW/OpenGL target leakage |
| Actions | Pointer/model edits, disabled action, switch, mixed-to-checked, radio, same-label IDs, programmatic focus, Space, Tab/Shift-Tab |
| Numeric | Slider edit, Ctrl-click drag direct input, exact 64-bit value beyond float integer precision, range drag and ordering |
| Input / Media | UTF-8 editing, edit callback, resize callback, multiline, validation edit, image button and color picker input; real color/image/plot rendering |
| Hierarchy | Tree expansion, table row selection, independent inline action, descending sort, scrolling with frozen headers, tab switch/reorder/close |
| Overlay | Popup open/Escape, menu selection, modal blocking, nested combo, modal Escape/cancellation and launcher focus restoration |
| Composites | Segmented selection, disabled search result, filtered result selection |
| Visual | Product Home, searchable navigation, 12 presets, component categories and advanced examples; Japanese glyphs, representative 1.5 scale, modal; real OpenGL backbuffers |
| Installed SDK | Debug and Release consumer compile/link/run from a relocated prefix; normalized archives contain no Windows absolute paths |
| Distribution | Source/SDK manifests identify the source commit and ABI; release attachments and SHA256SUMS are the authoritative published artifacts |

The public-IO integration log is `out/catalog/interaction.txt`; renderer details are in `out/catalog/capture-info.txt`. These are generated outputs. Representative images are checked into `docs/images/` for documentation. Full captures are available as release evidence.

The README animation is generated from 120 native 960×540 backbuffer frames. `tools/build_readme_gif.py` enforces the frame count, dimensions and 8 MiB limit; it does not validate native OS input.

公開IOログは`out/catalog/interaction.txt`、renderer情報は`out/catalog/capture-info.txt`に生成します。文書用の代表画像は`docs/images/`、全カテゴリ画像はReleaseの検証用archiveに収録します。

README animationは960×540のnative backbuffer 120枚から生成します。`tools/build_readme_gif.py`はframe数、寸法、8MiB上限を確認しますが、native OS入力の検証ではありません。

## Limits / 制約

- Baseline: Dear ImGui **1.92.9b docking**, default ABI types, Windows x64/MSVC. Other versions and platforms are not verified or implicitly compatible.
- Native OS/IME input, physical devices and integration into another application are not tested by this runner.
- Reordering/resizing/drag-drop preserve exact native APIs; exhaustive combinations are not reimplemented or exhaustively retested.
- Native wrappers are immediate. Optional explicit animation affects custom decoration only; fonts/renderer/state ownership stays with the host.
- Input units are separate labels. Native numeric text layout and parsing are retained, including native centering for drags/sliders and native text editing behavior.
- Japanese coverage is limited to the supplied font. Japanese headings use Regular; arbitrary Unicode/emoji coverage is not claimed.
- Screenshot state after interaction can differ from initial values; no synthetic reference image is used as evidence.

対応版・環境以外、native OS/IME、実機、他アプリへの導入は未検証です。標準のreorder/resize/drag-dropの全組合せを再試験していません。日本語の字形範囲、通常weight、数値の標準レイアウトを明示し、未実施の受け入れ確認を合格扱いしません。

## Editor Suite 1.0

See [Editor validation](editor-validation.md) for CPU/public IO, source and installed
consumers, native GPU, captures and the six-operation Release performance result.
EditorのCPU・公開IO・source/SDK consumer・実GPU・capture・6操作のRelease性能は上記を参照してください。
Native OS/IME and real-project integration remain separate, unperformed categories.
native OS/IMEと実project統合は別区分で、未実施です。

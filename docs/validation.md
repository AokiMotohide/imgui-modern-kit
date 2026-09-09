# Validation / 検証

This record distinguishes compile/link, public-IO interaction, GPU appearance and distribution. Passing native-API signature coverage does not mean every native behavior was exhaustively retested.

compile/link、公開IO操作、GPU外観、配布を区別します。署名確認を、標準APIの全動作を個別に再検証した証拠としては扱いません。

| Gate | Evidence / 確認内容 |
|---|---|
| Public API | 365 included overloads; independent function-pointer signature references compile and link against the pinned header; excluded functions are enumerated |
| Debug / Release | Library and production catalog compile/link on Windows x64, MSVC 19.51.36256 / v145 |
| Ownership | Two independent contexts; non-cumulative scale; nested ThemeScope style/font restoration; surrounding disabled alpha retained |
| Motion | Intermediate value, endpoint, disabled immediate result, pruning and generation reset |
| External target | Separate consumer creates its own ImGui target; no core/backend sources in imkit; no GLFW/OpenGL target leakage |
| Actions | Pointer/model edits, disabled action, switch, mixed-to-checked, radio, same-label IDs, programmatic focus, Space, Tab/Shift-Tab |
| Numeric | Slider edit, Ctrl-click drag direct input, exact 64-bit value beyond float integer precision, range drag and ordering |
| Input / Media | UTF-8 editing, edit callback, resize callback, multiline, validation edit, image button and color picker input; real color/image/plot rendering |
| Hierarchy | Tree expansion, table row selection, independent inline action, descending sort, scrolling with frozen headers, tab switch/reorder/close |
| Overlay | Popup open/Escape, menu selection, modal blocking, nested combo, modal Escape/cancellation and launcher focus restoration |
| Composites | Segmented selection, disabled search result, filtered result selection |
| Visual | Six categories in light/dark, Japanese glyphs, representative 1.5 scale, modal; real 1920×1440 OpenGL backbuffer |
| Installed SDK | Debug and Release consumer compile/link/run from a relocated prefix; normalized archives contain no Windows absolute paths |
| Distribution | Source/SDK manifests identify the source commit and ABI; release attachments and SHA256SUMS are the authoritative published artifacts |

The public-IO integration log is `out/catalog/interaction.txt`; renderer details are in `out/catalog/capture-info.txt`. These are generated outputs. Representative images are checked into `docs/images/` for documentation. Full captures are available as release evidence.

公開IOログは`out/catalog/interaction.txt`、renderer情報は`out/catalog/capture-info.txt`に生成します。文書用の代表画像は`docs/images/`、全カテゴリ画像はReleaseの検証用archiveに収録します。

## Limits / 制約

- Baseline: Dear ImGui **1.92.9b docking**, default ABI types, Windows x64/MSVC. Other versions and platforms are not verified or implicitly compatible.
- Native OS/IME input, physical devices and integration into another application are not tested by this runner.
- Reordering/resizing/drag-drop preserve exact native APIs; exhaustive combinations are not reimplemented or exhaustively retested.
- Native wrappers are immediate. Optional explicit animation affects custom decoration only; fonts/renderer/state ownership stays with the host.
- Input units are separate labels. Native numeric text layout and parsing are retained, including native centering for drags/sliders and native text editing behavior.
- Japanese coverage is limited to the supplied font. Japanese headings use Regular; arbitrary Unicode/emoji coverage is not claimed.
- Screenshot state after interaction can differ from initial values; no synthetic reference image is used as evidence.

対応版・環境以外、native OS/IME、実機、他アプリへの導入は未検証です。標準のreorder/resize/drag-dropの全組合せを再試験していません。日本語の字形範囲、通常weight、数値の標準レイアウトを明示し、未実施の受け入れ確認を合格扱いしません。

## Editor extension

See [editor-validation.md](editor-validation.md) for focused CPU, external consumer, public IO, real GPU and performance evidence. The full Editor Suite 1.0 acceptance remains incomplete.
Editor拡張の検証結果は上記を参照してください。Editor Suite 1.0全体の受入は未完了です。

Focused Inspector host verification: `imkit_gallery.exe --verify-inspector-model` checks selected-object rotation/scale/reset, locked object/property rejection, selection-change rejection and property key add/remove/navigation without creating a graphics context. This is model/event evidence, not native input acceptance.

Inspectorのホスト処理確認は `imkit_gallery.exe --verify-inspector-model` で行います。複数objectの回転・scale・reset、object/property lockと選択変更による拒否、property keyの追加・削除・前後移動を確認します。graphics contextは作成せず、native入力の検証ではありません。

The no-capture Gallery frame path reuses an empty filesystem path. Reconstructing it per frame caused one C++ allocation under the Debug runtime. A focused 180-frame Debug pan run at 1920x1440 with 256 tracks, 100096 clips and 100000 keys measured P95 2.3645 ms, at most 7 queries/30 visible clips, and zero C++ new/ImGui allocations. This is a pan-only measurement; it does not satisfy the full Release interaction performance gate.

captureしないGalleryフレームでは空のfilesystem pathを再利用します。毎フレームの構築がDebug runtimeで1回のC++ allocationを発生させていました。1920x1440、256 track、100096 clip、100000 keyのDebug pan 180フレームではP95 2.3645ms、最大7 query・30可視clip、C++ new/ImGui allocationとも0でした。panのみの測定であり、Releaseで全代表操作を確認する性能gateの完了を意味しません。

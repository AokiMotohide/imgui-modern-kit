# Architecture / 設計

Precision Layers separates semantic design, native behavior and host state.

| Layer | Responsibility / 責務 |
|---|---|
| `version.h` | Explicit baseline guard; no silent cross-version ABI claim / 対応版を明示 |
| `theme.h`, `theme.cpp` | Copyable palette/metrics/fonts/motion, deterministic style derivation, nested RAII / 値型と適用・復元 |
| `native.h` | Exact overload sets imported from the host's public header / 標準overloadの透過公開 |
| `widgets.h`, `widgets.cpp` | Original six compatible functions, pointer Selectable, selection/tree/tab markers / 公開DrawListで装飾 |
| `components.h`, `components.cpp` | Small native compositions, explicitly passed optional theme/animation / 明示的な合成部品 |
| Catalog host | Context, fonts, GLFW/OpenGL, image capture and representative inputs / 所有と実行環境 |

`imkit` compiles only its own implementation. It does not compile Dear ImGui, link a backend, initialize a context, discover fonts, persist settings or spawn workers. Public wrappers preserve native Begin/End, focus, callback, disabled, clipping and ID contracts. Decoration submits no replacement item; compound controls use native groups.

ライブラリ本体は自身の実装だけをコンパイルします。Dear ImGui本体・backend・OS・font loaderへの依存を内包しません。新しい部品でも入力を再実装せず、意味別token、公開widget、公開DrawListの順で設計します。将来の追加は他の部品の所有権を変えない小さなAPIとして行います。

There is no global theme registry. `Theme` is a copyable host-owned preset, `FontSet` contains non-owning references, and `AnimationState` is fixed-capacity storage whose lifecycle is explicitly controlled by the host. A scope must end before its context is destroyed. The static SDK binds to the consumer's already-created ImGui target through an imported interface adapter.

グローバルなTheme registryはありません。Themeの保存はホスト、FontSetは非所有参照、AnimationStateはホストが寿命を管理する固定容量状態です。scopeより先にContextを破棄しないでください。SDKは利用者が先に作成したImGui targetへ依存adapterで接続します。

## Extension policy / 拡張方針

Add overloads only after comparing the pinned public signature, preserving defaults and return semantics. Regenerate the API inventory and compile/link fixture with `tools/generate_api.py`. An unsupported new Dear ImGui version requires deliberate adapter/style review, not just relaxing the version guard. Keep rendering dimensions derived from theme/font size, state bounded, and native editing intact. Do not introduce host-specific data or services into this library.

新しいDear ImGui版への対応では署名・style・font契約を比較し、対応表とcompile/link fixtureを更新します。version guardを緩めるだけでは対応完了にしません。寸法はThemeと文字サイズから導出し、状態容量を制限し、ホスト固有の情報・サービスを持ち込みません。

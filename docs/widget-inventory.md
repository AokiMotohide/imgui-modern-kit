# Dear ImGui widget inventory

This inventory is based on Dear ImGui `v1.92.9b-docking` at commit `b48d1afbe8ee8b238e2961dc363a949dd7304e23`. References name public declarations in `imgui.h` and the corresponding examples or sections in `imgui_demo.cpp`. Families include their scalar-type, vector-width, and overload variants unless the note narrows them.

| 分類 | 標準API／部品群 | 標準側の参照 | 今回の状態 | 将来の対応検討 | 補足 |
|---|---|---|---|---|---|
| テキスト・ラベル・リンクなどの表示 | `Text*`, `LabelText`, `BulletText`, `SeparatorText`, `TextLink*` | `imgui.h`: `ImGui::Text`, `TextUnformatted`, `LabelText`, `TextLink`; `imgui_demo.cpp`: Text, Tooltips sections | 未実装 | 共通スタイル・挙動維持 | formatted/unformatted、wrapped、colored、disabled、URL variantsを含む |
| ボタン・画像ボタン | `Button` | `imgui.h`: `ImGui::Button`; `imgui_demo.cpp`: Widgets/Basic | 標準委譲のみ | 外観変更・挙動維持 | 現在の`imkit::Button`は同一引数を委譲 |
| ボタン・画像ボタン | `SmallButton`, `InvisibleButton`, `ArrowButton`, `ImageButton` | `imgui.h`: 同名シンボル; `imgui_demo.cpp`: Widgets/Basic, Images | 未実装 | 外観変更・挙動維持 | ImageButtonはtexture referenceとUV等を持つ |
| チェックボックス・ラジオ・選択項目 | `Checkbox` | `imgui.h`: `ImGui::Checkbox`; `imgui_demo.cpp`: Widgets/Basic | 標準委譲のみ | 外観変更・挙動維持 | 現在の`imkit::Checkbox`は値を保持しない |
| チェックボックス・ラジオ・選択項目 | `CheckboxFlags*`, `RadioButton`, `Selectable` families | `imgui.h`: 同名シンボル; `imgui_demo.cpp`: Widgets/Basic, Selectables | 一部標準委譲のみ | 外観変更・挙動維持 | `Selectable`のみ実装。bool/pointer、size、flags variantsを含む |
| コンボ・リスト | `BeginCombo`/`EndCombo`, `Combo`, `BeginListBox`/`EndListBox`, `ListBox` | `imgui.h`: 同名シンボル; `imgui_demo.cpp`: Widgets/Combo, List Boxes | 未実装 | 外観変更・複合部品・挙動維持 | getter、zero-separated string等のCombo overloadを含む |
| ドラッグ数値編集 | `DragFloat*`, `DragInt*`, `DragScalar*`, `Drag*Range2` | `imgui.h`: 同名シンボル; `imgui_demo.cpp`: Widgets/Drag Sliders | 未実装 | 外観変更・共通スタイル・挙動維持 | 全数値型、2/3/4次元、range variantsを含む |
| スライダー | `SliderFloat*`, `SliderInt*`, `SliderScalar*`, `VSlider*` | `imgui.h`: 同名シンボル; `imgui_demo.cpp`: Widgets/Sliders | 一部標準委譲のみ | 外観変更・共通スタイル・挙動維持 | `SliderFloat`のみ実装。全数値型・次元・vertical variantsを含む |
| 数値入力 | `InputFloat*`, `InputInt*`, `InputDouble`, `InputScalar*` | `imgui.h`: 同名シンボル; `imgui_demo.cpp`: Widgets/Data Types, Inputs | 未実装 | 外観変更・共通スタイル・挙動維持 | 全数値型、2/3/4次元、step variantsを含む |
| 単行・複数行テキスト入力 | `InputText`, `InputTextMultiline`, `InputTextWithHint` | `imgui.h`: 同名シンボル; `imgui_demo.cpp`: Widgets/Text Input | 一部標準委譲のみ | 外観変更・挙動維持 | `InputText`のみ実装。callback、resize等のflagsは透過 |
| 色編集・カラーピッカー | `ColorEdit3/4`, `ColorPicker3/4`, `ColorButton`, drag-drop options | `imgui.h`: 同名シンボル; `imgui_demo.cpp`: Widgets/Color | 未実装 | 外観変更・複合部品・共通スタイル | picker、palette、alpha、HDR関連flagsを含む |
| ツリー・折り畳み | `TreeNode*`, `TreeNodeEx*`, `TreePush`/`TreePop`, `CollapsingHeader` | `imgui.h`: 同名シンボル; `imgui_demo.cpp`: Widgets/Trees | 未実装 | 外観変更・挙動維持 | label、string-id、pointer-id overloadを含む |
| タブ | `BeginTabBar`/`EndTabBar`, `BeginTabItem`/`EndTabItem`, `TabItemButton` | `imgui.h`: 同名シンボル; `imgui_demo.cpp`: Layout/Tabs | 未実装 | 外観変更・共通スタイル・挙動維持 | reorder、close、leading/trailing flagsを含む |
| メニュー | `BeginMenuBar`, `BeginMainMenuBar`, `BeginMenu`, `MenuItem` and matching end calls | `imgui.h`: 同名シンボル; `imgui_demo.cpp`: Examples/Main menu bar | 未実装 | 外観変更・複合部品・挙動維持 | shortcut label、selected pointer overloadを含む |
| ツールチップ | `BeginTooltip`/`EndTooltip`, `SetTooltip`, `BeginItemTooltip`, `SetItemTooltip` | `imgui.h`: 同名シンボル; `imgui_demo.cpp`: Widgets/Tooltips | 未実装 | 共通スタイル・挙動維持 | item helperとformatted variantsを含む |
| ポップアップ・モーダル | `OpenPopup*`, `BeginPopup*`, `BeginPopupModal`, `CloseCurrentPopup`, `EndPopup` | `imgui.h`: 同名シンボル; `imgui_demo.cpp`: Popups & Modal windows | 未実装 | 外観変更・複合部品・挙動維持 | item/window/void context helpersを含む |
| テーブル | `BeginTable`/`EndTable`, `TableSetup*`, `TableNext*`, `TableSet*`, sort/query APIs | `imgui.h`: `ImGui::BeginTable`ほか; `imgui_demo.cpp`: Tables & Columns | 未実装 | 共通スタイル・挙動維持 | Galleryは標準APIを直接使用しラッパー対象にはしていない |
| 画像・グラフ・進捗 | `Image`, `PlotLines`, `PlotHistogram` | `imgui.h`: 同名シンボル; `imgui_demo.cpp`: Widgets/Images, Plotting | 未実装 | 外観変更・複合部品・挙動維持 | plotはarray/getter overloadを含む |
| 画像・グラフ・進捗 | `ProgressBar` | `imgui.h`: `ImGui::ProgressBar`; `imgui_demo.cpp`: Widgets/Progress Bars | 標準委譲のみ | 外観変更・挙動維持 | fraction、size、overlayをそのまま委譲 |
| ウィンドウ・子ウィンドウ・スクロール | `Begin`/`End`, `BeginChild`/`EndChild`, next/current window and scrolling APIs | `imgui.h`: Windows, Child Windows, Scrolling sections; `imgui_demo.cpp`: Layout | 基盤対象外 | 挙動維持 | Context・フレーム・window所有はホスト責務 |
| Docking | `DockSpace`, `DockSpaceOverViewport`, next/current dock query APIs | `imgui.h`: Docking section; `imgui_demo.cpp`: Docking examples | 基盤対象外 | 挙動維持 | docking固定版を採用するが、Docking管理はホスト責務 |
| 無効化・フォーカス・キーボード操作などの横断機能 | `BeginDisabled`/`EndDisabled`, focus APIs, item status/query APIs, shortcut/navigation APIs | `imgui.h`: Parameters stacks, Item/Widgets Utilities, Inputs sections; `imgui_demo.cpp`: Inputs & Focus | 未実装 | 共通スタイル・挙動維持 | 個々のwidgetと組み合わせる横断機能 |

## 独自部品候補

以下はAPIとデザインを確定していない後工程の候補であり、すべて未実装である。

| 候補 | 状態 |
|---|---|
| トグル | 未実装・後工程 |
| セグメント選択 | 未実装・後工程 |
| 検索付き選択 | 未実装・後工程 |
| 単位付き数値編集 | 未実装・後工程 |
| 設定行 | 未実装・後工程 |
| 状態バッジ | 未実装・後工程 |
| 通知 | 未実装・後工程 |
| ツールバー | 未実装・後工程 |


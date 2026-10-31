# PTZカメラ操作と範囲スライダー

[English](ptz.md) · [文書一覧](../README.md)

`imkit/ptz.h`、`imkit/range_slider.h`、または`imkit/imkit.h`をincludeします。どちらも描画と入力の報告だけを行います。値とカメラはすべてホストが持ち、フレーム間で保持されるのはホストが持つ`PtzState`だけです。既存APIは変更していません。

## PtzControl

```cpp
imkit::PtzView view;
view.label = "Camera 1";
view.position = {pan, tilt, zoom, /*valid*/ true};   // 機器が報告した位置。
view.presets = presets;                              // span<const PtzPreset>
view.connected = device.connected;
const imkit::PtzRequest r = imkit::PtzControl("cam1", view, state, {}, {&theme, &animation});
if (r.moving) device.Move(r.velocity.x, r.velocity.y, r.zoom);   // -1〜1、速度スケール適用済み。
if (r.stop) device.Stop();                                       // 移動が終わった次の1フレーム。
if (r.home) device.Home();
if (r.recallPreset >= 0) device.Recall(r.recallPreset);
if (r.storePreset >= 0) device.Store(r.storePreset);
```

| 部品 | 動作 |
|---|---|
| パッド | 8方向の矢印は押している間、全速度で動きます。円盤の他の場所を押すとアナログのジョイスティックで、不感帯があります（`PtzPadVector`）。 |
| Home | 中央の円が`home`を返します。 |
| ズーム | ロッカーを押している間ズームイン・アウトします。`zoomFraction`を渡すと位置バーを表示します。 |
| 速度 | 塗りスライダーが`PtzState::speed`を編集し、パッドとズームに掛かります。 |
| プリセット | クリックで保存済みを呼び出し、Shift+クリックまたは0.6秒の長押しで保存します。空きスロットはクリックで保存します。 |
| 未接続 | すべて無効になり、移動中なら`stop`で終わります。 |
| 狭い配置先 | `PtzOptions::diameter`が既定の0のときは、パッドとズームの列が使える幅に収まるよう、パッドをフレーム高さの3.2倍まで縮めます。直径を指定した場合は、そのまま使います。 |

矢印、Home、ズーム、プリセットはフォーカスできます。フォーカスした矢印でSpaceまたはEnterを押し続けると動きます。各部品はアクセシビリティのButtonノードで、名前は`LocaleContext`で差し替えられます（`ptz.up`、`ptz.home`、`ptz.zoomIn`など）。`PtzOptions::compact`はパッド、ズーム、Homeだけを描きます。この部品はカメラと通信せず、Galleryの疑似カメラでだけ確認しています。実機は未検証です。

### 機能の有無

すべてのカメラがパン、チルト、ズーム、ホーム、プリセット保存に対応しているわけではありません。機器が報告した内容を`PtzView::capabilities`に入れると、対応する部品だけを描きます。対応しない部品は場所を取らず、要求も返さず、アクセシビリティのノードも出しません。

| フィールド | falseのとき |
|---|---|
| `pan`、`tilt` | 左右または上下の矢印が消え、パッドのアナログ操作も同じ軸だけになります。斜めは両方が必要です。どちらもなければパッドとHomeが消えます。 |
| `zoom` | ズームのロッカーとバーが消えます。ズームだけのレンズではロッカーだけを表示します。 |
| `home` | Homeの円が消えます。`pan`か`tilt`も必要です。 |
| `presets` | プリセットの枠が消えます。 |
| `presetStore` | 枠は呼び出し専用になります。空きスロットは押せず、Shift+クリックと0.6秒の長押しでも保存しません。 |
| `speed` | 速度スライダーが消え、要求は全速度になります。 |

パン、チルト、ズーム、プリセットのどれもなければ、1行の文（`ptz.unsupported`）だけを描きます。どのフィールドも既定値はtrueなので、既存のコードは変わりません。

`PtzView::readoutText`は、度や倍率ではなく正規化したパン・チルトや0〜1のズームを報告する機器向けに、P / T / Zのチップの文字を置き換えます。nullのフィールドは`position`の数値のままです。

追加したロケールキー: `ptz.panPad`、`ptz.tiltPad`、`ptz.presetRecallHint`、`ptz.unsupported`。

## RangeSlider

`RangeSlider`と`RangeSliderInt`は下限と上限をその場で編集し、変化したフレームでtrueを返します。つまみのドラッグ、塗り部分のドラッグ（両方を移動）、レールのクリック（近いつまみを移動）ができます。値は範囲内で`low <= high`を保ち、`minGap`と`step`を指定できます。フォーカス中のつまみは`+`と`-`（Shiftで10倍）に反応し、矢印キーはDear ImGuiのナビゲーションのままです。つまみは数値範囲とIncrement/Decrement操作を持つ2つのGroupノードです。

## 塗りスライダーとコンボ

`imkit::SliderFloat`・`SliderInt`・`VSliderFloat`と2〜4成分版は、値の文字に重なるつまみの代わりに、枠内の色付きバーで値を示します。対数スライダーは従来のつまみのままです。`imkit::BeginCombo`と`imkit::Combo`は矢印ボックスをシェブロンに替え、長いプレビューを省略記号で縮めます。入力の挙動はDear ImGui標準ですが、`imkit::Combo`は項目を編集済みとして記録しないため、変更後の`IsItemEdited()`はfalseです。戻り値を使ってください。

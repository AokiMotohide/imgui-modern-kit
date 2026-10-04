---
title: "アイコン"
---

## 用途

編集toolの操作に同梱icon catalogを使い、atlas uploadとGPU resourceはrenderer側で管理します。

## Galleryの画面

![ImKitのicon atlasとicon buttonを表示するGallery画面。](../../../assets/captures/gallery-icons-poster.png)

Icons — atlasとlabel付き操作 · [Galleryの操作映像を開く](https://raw.githubusercontent.com/AokiMotohide/imgui-modern-kit/main/docs/images/gallery-icons.gif)

## 最小描画例

```cpp
#include <imkit/icons.h>

bool DrawSaveAction(imkit::IconAtlas& icons) {
    return imkit::IconButton("save", icons, imkit::IconId::Save, "Save document");
}
```

**例の種別:** 完結した関数例です。描画前にatlas textureを登録し、戻り値の操作はアプリ側で処理します。

## アプリへ組み込む

7種類からatlas sizeを選んでアプリのrendererでuploadし、`IconAtlas`へ登録します。描画完了後にGPU resourceを解放します。

## 範囲

Iconsはrenderer、image decoder、file検索、OS accessibility bridgeを追加しません。

## 関連APIとガイド

- [WindowFrameガイド](../window-frame/)
- [公開icons header](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/include/imkit/icons.h)

---

ImKit は、280種類以上の Lucide ベクターアイコンを埋め込みアトラスとして提供しており、専用のヘッダー `<imkit/icons.h>` をインクルードすることで利用できます。

---

## アイコンの利用方法

アイコンアトラスのテクスチャ生成および GPU への転送はホスト側で行い、描画時に `imkit::IconAtlas` を渡します：

```cpp
#include <imkit/icons.h>
#include <imkit/imkit.h>

// 1. 初期化時: GPU テクスチャアトラスの準備
imkit::IconAtlas icons;
for (int size : imkit::IconPixelSizes) {
    const auto atlasPixels = imkit::GetIconAtlasPixels(size);
    // ホストのグラフィックスAPI（OpenGL / Metal / DirectX）でテクスチャを作成
    ImTextureID textureId = HostCreateTexture(atlasPixels.width, atlasPixels.height, atlasPixels.rgba.data());
    icons.SetTexture(size, textureId);
}

// 2. フレームループ内での描画
// アイコン単体の描画
imkit::Icon(icons, imkit::IconId::Search, 16);

// ツールバー向けのアイコンボタン（ツールチップ付き）
if (imkit::IconButton("play_btn", icons, imkit::IconId::Play, "再生")) {
    // 再生ボタンが押されたときの処理
}

// アイコン＋テキストラベル付きボタン
if (imkit::IconActionButton("設定を開く", icons, imkit::IconId::Settings)) {
    // 設定ボタンが押されたときの処理
}
```

---

## 主な機能と特徴

- **マルチスケール対応**: 12px、16px、20px、24px、32px、48px、64px の7サイズのアトラスがあらかじめ生成されており、表示スケールや DPI に合わせて鮮明に描画されます。
- **テクスチャ未設定時の安全性**: テクスチャがまだロードされていない場合や無効な ID が指定された場合でもクラッシュせず、安全に空白領域を確保（またはボタンを無効化）します。
- **テーマカラーへの追従**: アイコンの色は `theme.semantic.text` やボタンの前景カラーに自動的にティントされます。

---

## 資産と再現手順

`assets/icons/{12,16,20,24,32,48,64}` には 2,016 枚の透過 PNG が、`assets/icons/atlases` には 7 枚のアトラス画像が収録されています。
アイコンアセットを再構築する場合は、Python 環境で `python tools/build_icons.py` を実行します。アトラス領域やハッシュ検証は `python tools/build_icons.py --check` で確認できます（C++ 利用者がビルド時に Python や Pillow を導入する必要はありません）。

---

## Gallery での動作確認

Gallery アプリの **Icons** 画面（ページID: 6）では、全288種類のアイコンカタログをリアルタイム検索し、異なるサイズやテーマでの外観を確認できます。
実装コードは [`examples/gallery/gallery.cpp`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/examples/gallery/gallery.cpp) を参照してください。

ライセンス条件やアセットの出典情報については [THIRD_PARTY_NOTICES.md](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/THIRD_PARTY_NOTICES.md) をご覧ください。

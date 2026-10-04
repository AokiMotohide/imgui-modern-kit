---
title: "WindowFrame"
---

## 用途

WindowFrameはテーマ付きのtitle areaを描画し、型付きwindow操作を返します。native windowとevent loopはアプリ側が管理します。

## Galleryの画面

![アプリケーションshellとcontent領域を含むGallery画面。](../../../assets/captures/v3-overview-poster.png)

Galleryの全体画面 — application shellとcontent · [Galleryの操作映像を開く](https://raw.githubusercontent.com/AokiMotohide/imgui-modern-kit/main/docs/images/v3-overview.gif)

## 最小描画例

```cpp
#include <imkit/window_frame.h>

imkit::WindowFrameResult DrawTitleArea(const imkit::Theme& theme,
                                       const imkit::WindowFrameContent& content,
                                       float widthPixels) {
    const auto style = imkit::MakeWindowFrameStyle(
        imkit::WindowFramePreset::Workspace, theme);
    const auto layout = imkit::LayoutWindowFrame(widthPixels, style);
    return imkit::DrawWindowFrame(style, content, layout);
}
```

**例の種別:** 完結した関数例です。現在のTheme、借用content、window幅をアプリ側で渡し、戻り値の操作を適用します。

## アプリへ組み込む

任意のWin32/macOS targetでnative title barの操作を連携します。基本描画APIはnative windowを所有しません。

## 範囲

Icon atlasとGPU textureもアプリ側のresourceです。window生成やtexture uploadは行いません。

## 関連APIとガイド

- [WindowFrame API map](../../api/window-frame/)
- [Iconガイド](../icons/)

---

WindowFrame APIはtitle areaの配置と描画、型付きwindow操作要求を提供します。基本targetは`imkit::window_frame`、headerは`<imkit/window_frame.h>`です。`Style`、寸法、feature、content、stateは呼出し側が明示します。文字列/spanはdraw呼出し中だけ借用します。

## frame内の使い方

ホストが開始したDear ImGui frame内で描画関数を呼び、返されたoperationをnative windowへ適用します。ImKit coreはnative window、platform message loop、resize/drag動作を所有しません。OS adapter targetは対応platformに分けてlinkします。

## Gallery Frame Lab

**Frame Lab** page (`18`) でpreset、style、状態を操作します。実例は[`gallery.cpp`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/examples/gallery/gallery.cpp)です。Galleryのwindow処理は別途host側にあり、libraryの全利用方法を代表するものではありません。

## Windows・macOS adapter

Win32 adapterは借用`HWND`を使い、macOS adapterは借用Cocoa windowを使います。対応platform targetだけをlinkしてください。native button、traffic light、drag、full screen、minimize/zoomの担当範囲と復元動作は[英語版WindowFrame contract](../../en/features/window-frame/)に記載されています。

CI buildや合成操作は実OS上のIME、DPI、window manager入力の受け入れを証明しません。platform別の受け入れ状態は時点付きの[Validation](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/docs/validation.ja.md)を確認してください。

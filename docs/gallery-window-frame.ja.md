# Public window frame / 公開WindowFrame

[English](gallery-window-frame.md) · [実例recipe](examples-recipes.ja.md) · [文書カタログ](documentation-catalog.ja.md)

WindowFrame APIはtitle areaの配置と描画、型付きwindow操作要求を提供します。基本targetは`imkit::imkit`、headerは`<imkit/window_frame.h>`です。`Style`、寸法、feature、content、stateは呼出し側が明示します。文字列/spanはdraw呼出し中だけ借用します。

## frame内の使い方

ホストが開始したDear ImGui frame内で描画関数を呼び、返されたoperationをnative windowへ適用します。ImKit coreはnative window、platform message loop、resize/drag動作を所有しません。OS adapter targetは対応platformに分けてlinkします。

## Gallery Frame Lab

**Frame Lab** page (`18`) でpreset、style、状態を操作します。実例は[`gallery.cpp`](../examples/gallery/gallery.cpp)です。Galleryのwindow処理は別途host側にあり、libraryの全利用方法を代表するものではありません。

## Windows・macOS adapter

Win32 adapterは借用`HWND`を使い、macOS adapterは借用Cocoa windowを使います。対応platform targetだけをlinkしてください。native button、traffic light、drag、full screen、minimize/zoomの担当範囲と復元動作は[英語版WindowFrame contract](gallery-window-frame.md)に記載されています。

CI buildや合成操作は実OS上のIME、DPI、window manager入力の受け入れを証明しません。platform別の受け入れ状態は時点付きの[Validation](validation.ja.md)を確認してください。

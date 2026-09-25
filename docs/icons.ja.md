# アイコン / Icon reference

[English](icons.md) · [Theme](themes.ja.md) · [実例recipe](examples-recipes.ja.md)

Icon APIは公開header `<imkit/icons.h>`と`imkit::imkit` targetで利用できます。描画は現在のDear ImGui frameへ行い、font atlasまたはtextureの準備・upload・破棄はホストが担当します。

## Host統合

Icon IDと描画関数は[`include/imkit/icons.h`](../include/imkit/icons.h)を参照してください。Gallery sampleのtexture登録方法を利用アプリのrenderer契約と同一視しないでください。

frame内では、hostがuploadした`IconAtlas`と公開IDを使って`imkit::Icon(atlas, imkit::IconId::Search)`を呼びます。

正確な型と描画overloadは公開headerと[English API behavior](icons.md#api-behavior)を参照してください。

## API behavior

`Icon`は現在のframeにatlasのspriteを描画します。`IconButton`/`IconLabelButton`はIDとaccessibility labelを受け取り、押下結果を返します。texture未設定または無効IDでは描画せず、buttonを無効にします。正確なoverloadは公開headerを参照してください。

## Assets and reproduction

生成icon dataのsource・再生成手順・license記録は英語版の[Assets and reproduction](icons.md#assets-and-reproduction)と`THIRD_PARTY_NOTICES.md`にあります。Icon APIはhost font、GPU atlas、texture registry、保存を所有しません。配布時は同梱noticeに従ってください。

## Native Gallery

Gallery **Icons** pageでは検索、atlas表示、各IDのsymbolを操作できます。page登録と描画実装は[`gallery.cpp`](../examples/gallery/gallery.cpp)にあります。Themeとの併用は[Theme guide](themes.ja.md)を参照してください。

# Editor API reference / Editor API参照

[English signature reference](editor-api.md) · [Editor Suite](editor-suite.ja.md) · [文書カタログ](documentation-catalog.ja.md)

英語版には公開型、overload、event payload、provider callbackを示す詳細な署名表があります。ここでは統合契約を日本語で要約します。C++の型名・関数名・正確な宣言は共通の[英語版API reference](editor-api.md)を参照してください。

## Providerとevent

- Providerが返すdataとtextは非所有viewです。draw/callbackの必要期間だけ有効にしてください。
- 連続編集はBegin/Update/Commit/Cancelとrevision、元値、提案値で表現します。preview反映と確定を区別し、古いrevisionのgestureは破棄します。
- event、scratch、選択配列の容量は呼出し側が提供します。容量不足はAPIのoverflow結果として処理し、暗黙の動的確保を前提にしません。
- hostが受理した変更だけrevisionを更新します。model mutation、衝突policy、Undo、保存、workerはホスト責務です。
- visible-range queryで通常frameの処理量を制限し、編集時には必要なoffscreen・関連対象も問い合わせます。IDはsort/filterをまたいで安定させます。

## Graphics preview

OpenGL3 previewを使う場合、ホストは関数表とcurrent OpenGL 3.3 Contextを渡します。明示的preview objectがFBOやtextureを所有し、Context破棄より前に`Shutdown()`を呼びます。描画後のGL state復元はホスト側です。DrawList previewにはz-bufferがなく、交差面や隠れたoutlineは近似になることがあります。

## TimelineとMonitorの追加API

Timeline providerは選択、隣接clip、offscreen対象、external dropの候補をホスト側で解決します。Monitorはホストが各frameで渡すtextureと状態を表示します。media再生、decode、capture、collision policyはAPIに含まれません。詳細な署名と制約は[英語版API表](editor-api.md)を参照してください。

## Range、marker、property array

range/marker/property array callbackのdataとscratch storageは呼出し側が用意します。IDをsort/filter間で維持し、容量不足時は明示的に失敗を扱います。最終決定はホストが所有し、ImKitが編集dataやUndo履歴を保持しません。

[Editor Suite guide](editor-suite.ja.md)はmodule選択とGallery source、[Timeline recipe](timeline-editing.ja.md)は具体操作を説明します。[Public API coverage](api-coverage.ja.md)はDear ImGui基礎APIの対応範囲を示します。

[Editor Suite guide](editor-suite.ja.md)は用途・target・Gallery上の画面を説明し、[Timeline recipe](timeline-editing.ja.md)は編集操作のhost責任を示します。[公開API coverage](api-coverage.ja.md)は固定Dear ImGui APIの範囲を記載します。これらは異なるmoduleの契約なので、相互に同一の検証証拠とは扱いません。

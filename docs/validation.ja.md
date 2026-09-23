# Validation / 検証

[English evidence log](validation.md) · [Architecture](architecture.ja.md) · [文書カタログ](documentation-catalog.ja.md)

英語版は検証履歴の詳細正本です。以下は見出しごとに対象と限界を案内します。記録されたpassは対象日・revision・構成に限られ、後続checkoutや未記載platformの証明ではありません。

## v3.0 platform matrix / v3.0 platform構成

2026-09-13時点のv3.0 CI記録です。各jobのhead SHA、OS、architecture、結果は[英語記録](validation.md#v30-platform-matrix-v30-platform-matrix)で確認してください。

## Node editor access, appearance and collapse / Node Editorの導線・外観・折りたたみ（2026-09-12）

Node Editor companionのUI変更に対する当時の確認記録です。現行のhost integration受け入れとは区別します。

## V3 Gallery and documentation captures / v3 Galleryと文書capture（2026-09-13）

Galleryおよび文書用captureの記録です。backbuffer/GPU captureはnative OS入力、IME、外部rendererの証明ではありません。

## Timeline external-drop preview / Timeline外部drop preview（2026-09-12）

Timelineのdrop preview UIとevent経路を対象とした履歴です。利用hostのdrop source・collision policy・Undoまで保証しません。

## Preview placement extension / Preview配置拡張（2026-09-12）

Preview配置の実装・検証範囲を記録しています。対象revisionとrenderer契約は英語版の同項目を確認してください。

## Public window frame / 公開WindowFrame（2026-09-11）

公開WindowFrameの当時のbuild/操作証拠です。物理DPI、mixed-DPI、native window managerの受け入れは別範囲です。

## Generic workflow extension / 汎用Workflow拡張（2026-09-11）

Workflow部品のcaptureと合成操作履歴です。Galleryのsample hostであり、外部アプリ統合やOS入力の受け入れではありません。

## Limits / 制約

native OS入力・IME、screen reader、実DPI/mixed-DPI、外部host renderer、実media/device、署名・notarizationは、それぞれの証拠が記録されない限り未確認です。署名compile/link、CI build、public ImGui IO、GPU captureは別種の証拠です。

## Editor Suite 1.0

Editor Suite 1.0は前段revisionの履歴です。現在のEditor Suite 2.0契約や現行branchの合格として再利用しません。

## Dynamic node sockets / 動的Node socket（2026-09-12）

動的socketのGallery specimenとrequest処理に関する過去の確認です。graph model、socket policy、revision受理、評価はホスト責務です。

## Node navigation completion / Node周辺操作（2026-09-12）

Node navigation・操作の履歴確認です。現在のAPI署名、制約、検証範囲は[Node Editor guide](node-editor.ja.md)と公開headerで照合してください。

### 検証baseline

公開API基準はDear ImGui docking revision `367b2c24f399988ddafc0bb4628da0106bcc09be`、default ABI typesです。過去の検証値を後続revisionの証拠として扱わないでください。

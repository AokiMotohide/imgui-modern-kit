# Public API coverage / 公開API対応表

[English generated table](api-coverage.md) · [API inventory JSON](api-inventory.json) · [文書カタログ](documentation-catalog.ja.md)

このページは生成されたAPI対応表の読み方と適用範囲を説明します。overload単位の署名、include/exclude、実装方式、分類、証拠scopeは[英語版の生成表](api-coverage.md)を参照してください。表のC++宣言とsymbol名は言語間で共通です。

対応表はDear ImGui `1.93.0 WIP docking`、revision `367b2c24f399988ddafc0bb4628da0106bcc09be`に対して生成されています。含まれるoverloadは固定版を前提とし、native aliasはdefault引数、callback、flag、Begin/End契約を維持します。Precision LayersのTheme適用には`ApplyTheme`が必要です。

各行は署名compile/linkの対象を示します。全APIを個別の人手操作で確認したという意味ではありません。代表カテゴリの操作と別々に解釈してください。Selection/tab wrapperは同一frameの公開DrawListにmarkerを描き、追加のitemをsubmitしません。旧来6 wrapper signatureは維持されます。

Context、frame、renderer、platform、debug tool、logging、allocator、ini保存はホスト責務です。内部APIや現在のnamespace外の旧宣言は対象外です。合成componentは[利用ガイド](guide.ja.md)を参照してください。検証値は[Validation](validation.ja.md)に記録されたrevision・日付の範囲に限ります。

# コンポーネントとrecipe

[English](components.md)

ImKitのコンポーネントはDear ImGui公開動作を使った小さな合成部品です。編集値、寿命、保存はホストが所有します。

## 主要action

```cpp
if (imkit::ActionButton("Apply", imkit::ActionVariant::Primary, {}, {&theme, &animation}))
    SaveSettings();
```

primaryは1つの判断領域で1回だけ使います。secondary、ghost、destructiveもbuttonの標準操作を変えず、意図だけを表現します。

## Booleanと混在状態

```cpp
imkit::Toggle("Enabled", &enabled, {&theme, &animation});
imkit::IndeterminateCheckbox("Inherited", &state);
```

## 検索とvalidation

```cpp
imkit::SearchableCombo("Quality", &quality, labels, search, sizeof(search), disabled);
imkit::InputTextWithHint("Name", "Required", name, sizeof(name));
imkit::ValidationMessage("名前が必要です", name[0] == 0, &theme);
```

検索bufferと選択indexはホスト所有です。validationは表示だけを行い、値の拒否や保存は行いません。

## 設定行と単位

```cpp
if (imkit::BeginSettingRow("exposure", "Exposure")) {
    imkit::DragFloatWithUnit("##value", &exposure, "EV", .01f, -8.f, 8.f);
    imkit::EndSettingRow();
}
```

`BeginSettingRow`がtrueのときだけ`EndSettingRow`を呼びます。scalarの標準解析と精度を維持します。

## 状態と通知

```cpp
imkit::StatusBadge("Ready", imkit::StatusKind::Success, &theme);
if (imkit::NotificationCard({"saved", "保存しました", imkit::StatusKind::Success, 0}, now, &theme))
    dismissSavedNotice = true;
```

通知の期限と削除はホスト所有です。overload単位の詳細は[API対応表](api-coverage.md)を参照してください。

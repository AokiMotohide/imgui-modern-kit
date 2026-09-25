# コンポーネントとrecipe

[English](components.md)

ImKitのコンポーネントはDear ImGui公開動作を使った小さな合成部品です。編集値、寿命、保存はホストが所有します。以下に、既存frame内で設定画面全体を描画する例と、個別部品の短い例を示します。

## 設定画面をひとつ組み立てる

値はアプリケーションの状態に置き、ホストが既に開始したDear ImGui frame内で画面を描画します。この関数は保存要求を返すだけで、ファイルや通知queueは所有しません。

```cpp
#include <imkit/imkit.h>
#include <array>

struct DisplaySettings {
    bool enabled = true;
    int quality = 1;
    char qualitySearch[64]{};
    float exposure = 0.0f;
    char name[128] = "Main display";
    bool showSavedNotice = false;
};

bool DrawDisplaySettings(DisplaySettings& settings, const imkit::Theme& theme, double now) {
    imkit::ThemeScope themeScope(theme); // 同じ生存中Context上で使い、Context破棄前に終了する
    bool saveRequested = false;
    if (imkit::Begin("Display settings")) {
        imkit::Toggle("Enabled", &settings.enabled, {&theme});

        if (imkit::BeginSettingRow("quality", "Quality")) {
            constexpr std::array<const char*, 3> labels{"Draft", "Balanced", "High"};
            imkit::SearchableCombo("##quality", &settings.quality, labels,
                                   settings.qualitySearch, sizeof(settings.qualitySearch));
            imkit::EndSettingRow();
        }

        if (imkit::BeginSettingRow("exposure", "Exposure")) {
            imkit::DragFloatWithUnit("##exposure", &settings.exposure, "EV", 0.01f, -8.0f, 8.0f);
            imkit::EndSettingRow();
        }

        imkit::InputTextWithHint("Name", "Required", settings.name, sizeof(settings.name));
        const bool invalidName = settings.name[0] == '\0';
        imkit::ValidationMessage("Enter a name before saving.", invalidName, &theme);
        imkit::BeginDisabled(invalidName);
        saveRequested = imkit::ActionButton("Save", imkit::ActionVariant::Primary, {}, {&theme});
        imkit::EndDisabled();

        if (settings.showSavedNotice &&
            imkit::NotificationCard({"saved", "Settings saved", imkit::StatusKind::Success, 0}, now, &theme)) {
            settings.showSavedNotice = false; // dismiss要求を処理するのはホスト
        }
    }
    imkit::End(); // Beginの戻り値に関係なく対にする
    return saveRequested; // 設定の保存と通知を表示する時点はホストが決める
}
```

ホストの`NewFrame()`後、`Render()`前に呼びます。戻り値が`true`なら、ホスト側で`DisplaySettings`を検証・保存してから`showSavedNotice`を設定します。`Theme`、font pointer、Dear ImGui Context、画面状態もホストが保持します。

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

`Toggle`は値が変わったときに`true`を返します。`IndeterminateCheckbox`は操作すると`Mixed`から`Checked`へ移ります。enum値は他のmodelと同様にホスト側へ保存してください。

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

`NotificationCard`はdismiss要求を返します。`expiresAt`が0ならホストが削除するまで表示し続け、0以外ならホストから渡した`now`と比較します。

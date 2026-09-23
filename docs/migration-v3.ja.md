# ImKit 3.0 migration / ImKit 3.0移行

[English](migration-v3.md) · [導入ガイド](getting-started.ja.md) · [文書カタログ](documentation-catalog.ja.md)

v2 consumerをv3へ移行する際は、API変更だけでなくDear ImGui ABI、renderer固有header、platform target、host ownershipを確認します。

1. 利用側Dear ImGui、対応公式backend、Test Engineをdocking commit `367b2c24f399988ddafc0bb4628da0106bcc09be`に合わせます。
2. OpenGL型には`<imkit/preview_opengl3.h>`をincludeします。`<imkit/preview.h>`はrenderer非依存型です。macOSは`imkit::preview_metal`と借用`MTLDevice`/command bufferを使います。
3. `Win32ActionSink`を`NativeActionSink`へ移行します。callbackは`bool`を返し、任意のUTF-8値を受け取ります。対象OSのaccessibility targetをlinkします。
4. `IMKIT_BUILD_WINDOW_FRAME_LEGACY_IMGUI`とlegacy target参照を除き、通常のv3 targetを使います。
5. installed SDKの`IMKIT_SDK_ABI_CONFIRMED=ON`は、記録されたOS、architecture、compiler、Dear ImGui revision、`imconfig` ABIを照合してから設定します。
6. Node Editorを使う場合は`imkit::node_editor`を明示的にlinkします。graph data、revision検証、編集適用、Undo、評価、保存はホスト所有です。Material Graph companionをlibrary stateとして扱いません。

実際のtarget/header選択は[導入ガイド](getting-started.ja.md)と[Dependencies](dependencies.ja.md)を確認してください。正確なv3差分の基準は[英語migration reference](migration-v3.md)です。

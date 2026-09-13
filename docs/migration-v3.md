# ImKit 3.0 migration / ImKit 3.0移行

1. Update the host to Dear ImGui `docking` commit
   `367b2c24f399988ddafc0bb4628da0106bcc09be`, including matching official backends
   and Test Engine sources.
2. Include `<imkit/preview_opengl3.h>` for `GLFunctions` and `OpenGL3Renderer`.
   `<imkit/preview.h>` now contains renderer-neutral types only. On macOS link
   `imkit::preview_metal` and pass borrowed `MTLDevice`/command buffers.
3. Replace `Win32ActionSink` with `NativeActionSink`. The callback returns `bool` and
   receives an optional UTF-8 value. Link the matching `accessibility_win32` or
   `accessibility_macos` target.
4. Remove `IMKIT_BUILD_WINDOW_FRAME_LEGACY_IMGUI` and legacy target references.
   All consumers use the normal v3 targets and the exact pinned Dear ImGui ABI.
5. Installed consumers set `IMKIT_SDK_ABI_CONFIRMED=ON` only after matching the
   recorded OS, architecture, compiler, ImGui revision and `imconfig` ABI.
6. Link `imkit::node_editor` explicitly when adopting the new Node Editor. Move
   graph data, revision checks, edit application, Undo, evaluation and persistence
   into the host; do not treat the companion Material Graph model as library state.

1. ホストのDear ImGui、公式backend、Test Engineを上記`docking` commitへ揃えます。
2. OpenGL固有型は`preview_opengl3.h`をincludeします。macOSは`preview_metal`へ借用した
   `MTLDevice`／command bufferを渡します。
3. `Win32ActionSink`を、UTF-8値付きで`bool`を返す`NativeActionSink`へ移行します。
4. 1.88互換オプション／targetを削除し、通常のv3 targetを使用します。
5. install済みSDKは記録されたABI条件を照合した後だけ明示確認します。
6. Node Editorを採用する場合は`imkit::node_editor`を明示linkし、graph data、revision検証、
   request適用、Undo、評価、永続化をホストに置きます。Material Graph companionのmodelを
   library状態として使用しません。

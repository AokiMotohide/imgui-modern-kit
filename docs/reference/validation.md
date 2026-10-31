# Validation

[日本語](検証記録.md) · [Architecture](../architecture/architecture.md) · [Documentation catalog](documentation-catalog.md)

This page records what was checked, on which revision and configuration, and what was not. Compile/link, public-IO interaction, GPU appearance and distribution are separate kinds of evidence, and none substitutes for another. A result holds only for its stated date, revision and configuration.

## Standing gates

The Gallery and the test suite re-check these on every release. Passing native-API signature coverage does not mean every native behavior was retested.

| Gate | Evidence |
|---|---|
| Public API | 365 included overloads; independent function-pointer signature references compile and link against the pinned header; excluded functions are enumerated |
| Debug / Release | Library and production catalog compile and link on Windows x64, MSVC 19.51.36256 / v145 |
| Ownership | Two independent contexts; non-cumulative scale; nested ThemeScope style and font restoration; surrounding disabled alpha retained |
| Theme presets | 13 stable unique IDs; complete finite palettes; Light/Dark metadata; legacy Precision values; normal, muted and semantic contrast thresholds |
| Motion | Intermediate value, endpoint, disabled immediate result, pruning and generation reset |
| External target | A separate consumer creates its own ImGui target; no core or backend sources in imkit; no GLFW/OpenGL target leakage |
| Actions | Pointer and model edits, disabled action, switch, mixed-to-checked, radio, same-label IDs, programmatic focus, Space, Tab/Shift-Tab |
| Numeric | Slider edit, Ctrl-click drag direct input, exact 64-bit value beyond float integer precision, range drag and ordering |
| Input / Media | UTF-8 editing, edit callback, resize callback, multiline, validation edit, image button and color picker input; real color, image and plot rendering |
| Hierarchy | Tree expansion, table row selection, independent inline action, descending sort, scrolling with frozen headers, tab switch, reorder and close |
| Overlay | Popup open and Escape, menu selection, modal blocking, nested combo, modal Escape and cancellation, launcher focus restoration |
| Composites | Segmented selection, disabled search result, filtered result selection |
| Visual | Product Home, searchable navigation, presets, component categories and advanced examples; Japanese glyphs, representative 1.5 scale, modal; real OpenGL backbuffers |
| Installed SDK | Debug and Release consumer compile, link and run from a relocated prefix; normalized archives contain no Windows absolute paths |
| Distribution | Source and SDK manifests identify the source commit and ABI; release attachments and SHA256SUMS are the authoritative published artifacts |

Generated evidence and media:

- The public-IO log is `out/catalog/interaction.txt`; renderer details are in `out/catalog/capture-info.txt`. Both are generated and not committed.
- Representative images are checked into `docs/images/`. Full captures are release evidence.
- The README and Pages animations are cut from lossless 1920×1080, 60 Hz takes of the Gallery. `tools/media/capture.py` records them and checks each take's assertions. `tools/media/verify.py` enforces the GIF contract (960×540, at most 2 MiB, 6–8 s) and the film contract. Neither validates native OS input.

## Records, newest first

### 4.0.0 — 2026-10-31

**Verified**

- CI at merge commit `efa9d17` (push): Windows x64, Windows Arm64, macOS arm64, macOS x86_64, the macOS Universal 2 package and documentation, six of six succeeded. The platform jobs build, test, package and run the packaged SDK consumer.
- Windows x64 Debug Gallery rebuilt from the merged tree: `--verify` (public IO) 45 checks passed, `--verify-comparison` passed.
- Both films (`tools/media/verify.py`): 1920×1080, 60 fps, 180.0 s (10,800 frames), −14.64 LUFS, −2.19 dBTP, 28.3 MiB (ja) and 28.1 MiB (en). 13 GIFs at 960×540 and 8.0 s, the largest 1.67 MiB. Both languages have the same 24 caption keys.
- Audio encode: AAC at 192 kbit/s overshot the limited signal by 2.6 dB (−0.83 dBTP), above the −1.5 dBTP limit. At 256 kbit/s the same signal measures −2.19 dBTP.
- Documentation: `tools/check_docs.py` passed for 39 English/Japanese pairs and 80 public pages. The site build produced 67 pages and checked 4,900 local links, 365 API rows (48 excluded) and 47 Node declarations.
- Pages: published from `main`. Both films, the poster, the credit line and the migration guide answer HTTP 200, and the home page was checked at 375 px width.
- macOS build: `ParseTheme` no longer uses floating-point `std::from_chars`, which Apple's libc++ does not provide.

**Not verified**

- How the music sounds. Only loudness and true peak were measured.
- Physical OS input, native IME, screen readers, mixed-DPI and multi-monitor use.
- Third-party hosts and hardware: cameras, PTZ heads, media servers, LED controllers.
- macOS signing and notarization. The macOS packages are unsigned.

### 3.3.0 — 2026-10-23

**Verified**

- Windows x64 Debug Gallery and four targets built in `build/windows-debug` through the shared build lock. `imkit.workflow`, `imkit.workflow_api_compile`, `imkit.toast` and `imkit.toast_api_compile` passed (4/4). Workflow covers indeterminate, running and paused semantics and task operation IDs. Toast covers timing, focus, loading completion and existing API behavior.
- Native Gallery toast verifier: mouse action and dismiss, Space, same-ID loading completion, six positions in light and dark, Japanese indeterminate loading.
- The progress GIF has 80 native OpenGL backbuffer frames (960×540, 10 fps, 8 s). Six public-IO clicks advance the host-owned task to completion. It is a demonstration, not physical OS input. The 3.2 film remains labeled as a historical demonstration.
- Release gate: push CI at the exact release commit (Windows x64/Arm64, macOS arm64/x86_64/Universal 2: build, test, package, packaged SDK consumer). The manifest and evidence ZIP record the run, each SDK's source commit and the native evidence hashes. Older records below are not 3.3 acceptance.

**Not verified**

- Physical OS input, native IME, real screen readers, mixed-DPI and multi-monitor use, external-host and hardware acceptance.
- macOS signing and notarization depend on CI credentials. Universal 2 runs on arm64; Intel runs in its own job.

### Media revision 2 — 2026-10-05

**Verified**

- Native footage: Windows x64 Debug Gallery and Node Editor built in the shared output. Twelve continuous 1920×1080, 60 Hz public-IO takes hold 7,857 frames and 38 successful state checks.
- The checks cover shared comparison edits, workspace tabs and hierarchy, axis dragging, host-applied actions, typed icon search and selection, four theme selections, Loading to Success, timeline scrub, clip move, play and pause, a socket connection and a node-header drag. Middle-button pan and wheel zoom are recorded. The independent comparison regression also passed keyboard, disabled, style-restoration and host-workbench checks. Raw videos and event logs stay outside Git.
- Film: 25 cuts, kinetic type, focused native screens, masked transitions, an icon expansion and an original 128 BPM score. Review fixes: cropped headings, caption overlap, crowded icon tiles, offscreen theme targets, an invalid timeline move, a scrolled notification control, clipped and overlapping nodes after navigation. The corrected compositions were rechecked in 24 timecoded stills and full normal-speed browser playback, and a video-only copy ran 60 s for silent review. Source exhaustion raises an error instead of repeating the last frame.
- Measurements: 60.000 s, 1920×1080, 60 fps (3,600 frames), H.264/AAC stereo 48 kHz, 8,557,071 bytes, −16.06 LUFS, −1.84 dBTP. Two-pass normalization leaves headroom for AAC reconstruction. The 12 native GIFs are 960×540, 20 fps and 6–8 s, each under 2 MiB (largest 1,325,419 bytes).
- Pages generation, Astro type checking and the static build passed; the inspection covered 61 pages and 4,182 links. Browser checks: Japanese and English, light and dark, desktop and about 390 CSS px, video playback, GIF play and stop. README and Pages use the revised poster, and the Pages video URL carries the film's content hash.
- The v3.2.0 library tag, source archive, five SDK archives and original release CI identity are unchanged. The revised media has its own commit, timestamp and hashes in the release manifest.

**Not verified**

- Listening quality, buyer acceptance of commercial quality, physical OS and IME input, hardware.
- The footage is a Windows OpenGL development capture. It does not show native-operation acceptance on every packaged platform.

### 3.2 release preparation — 2026-10-04

**Verified**

- Windows x64 Debug Gallery built in the shared output. Passed: `imkit.workflow`, `workflow_api_compile`, `toast`, `toast_api_compile`, `theme`, `api_compile`, `window_frame_api_compile`.
- Native public-IO verification covered both comparison modes, shared edits in both directions, keyboard activation, disabled controls, complete style restoration, and host-applied workspace selection and toggle requests. `--verify-workflow` and `--verify-toasts` passed; toast evidence covers six positions in light and dark, Japanese loading, actions, dismissal and same-ID completion.
- 640×600 backbuffers verified compact navigation and scrollable workspace settings. 1280×720 captures supplied the GIFs.
- Three review passes covered the first ten seconds, timecoded compositions with full playback, and README, Pages, GIF and API consistency. Fixes: default font conditions, isolated comparison, label fit, compact toast controls, light icon tiles, Loading-to-Success timing, stale paths and anchors.
- Film: 60.000 s, 1920×1080, 60 fps (3,600 frames), H.264/AAC stereo, 6,023,496 bytes, −16.00 LUFS, −1.78 dBTP. The 12 `v3-*.gif` captures are 960×540, each under 2 MiB (largest 1,877,626 bytes). Motion graphics and music are original. The film is a composition; the documentation GIFs are native captures.
- Pages generation, Astro type checking (zero diagnostics), the static build and a 61-page, 4,182-link inspection passed. Chrome checks: Japanese and English, light and dark, desktop and about 384 CSS px, GIF play and stop, the full 60 s film, Pagefind queries for `DragVector3WithUnit` and `TableGetColumnFlags`.

**Not verified**

- The final-commit CI result and SDK package consumer evidence are stored in the release manifest and evidence archive once all five platform jobs succeed. Local checks alone do not meet the platform release gate.
- Physical OS input, native IME, real screen readers, multi-monitor DPI, external-product integration, hardware.
- Universal 2 tests run on the arm64 CI host, with a separate x86_64 job. macOS signing and notarization depend on release credentials.
- Consumers must recompile for `WindowFrameContent::iconTexture`. The pinned Dear ImGui revision is unchanged.

### Colored toasts — 2026-10-01

**Verified**

- Windows x64 Debug: the new toast and API compile tests, existing workflow regressions, and an external host-ImGui toast consumer.
- Native OpenGL Gallery `--verify-toasts --output out/toast-verification`: mouse action and dismiss, Space action, same-ID loading completion. Six positions in light and dark and Japanese indeterminate loading were captured.
- The animation was a 12-frame, 960×540, 96-color GIF encoded from the six-position light and dark backbuffers. The encoder restores the canvas between frames so moving cards stay clear.
- Host covering windows use `NoBringToFrontOnFocus`, so notifications stay visible after host clicks. See [Toasts](../components/toasts.md).

**Not verified**

- Native OS/IME, screen reader, cross-monitor DPI, macOS, Arm64, Release and installed-package acceptance for this addition.

### v3 Gallery and documentation captures — 2026-09-13

**Verified**

- The Debug native Gallery passed `--verify-comparison`. Default Dear ImGui and ImKit controls changed the same host-owned value, temporary comparison styles were restored after a frame, and closing and reopening removed then restored the submitted controls.
- Native 960×540 captures: 120 overview frames, 80 Node Editor, 120 workflow/progress, 80 timeline and 120 theme/comparison. The encoder accepted all five GIFs under its 8 MiB limit. The Node Editor sequence deterministically changes host-owned zoom and pan, adds and connects a visible socket, and shows inline values, previews and the minimap. The showcase MP4 came from the same 520 native frames.
- The released Gallery executable is built, staged and inspected for DLL imports separately. Its archive does not bundle the Microsoft Visual C++ Redistributable.

**Not verified**

- The runner sends public Dear ImGui IO and reads a real OpenGL backbuffer. It does not establish native OS/IME input, assistive-technology operation, physical DPI, performance or external-host acceptance.

### v3.0 platform matrix — 2026-09-13

| Configuration | Automated gate | Native acceptance |
|---|---|---|
| Windows 10/11 x64 | build, focused tests, Gallery, install consumer, ZIP | required on one Windows x64 machine |
| Windows 11 Arm64 | build, focused tests, Gallery, install consumer, ZIP | architecture-specific manual run not required |
| macOS 15+ Apple Silicon | build, tests, Metal Gallery, app bundle, ZIP | required on one Apple Silicon Mac |
| macOS 15+ Intel | build, tests, Metal Gallery, app bundle, ZIP | architecture-specific manual run not required |

**Verified**

- PR #8 head `326618c` passed all five GitHub Actions jobs on 2026-09-13: Windows x64 and Arm64 build, test and package; macOS arm64 and x86_64 build, test and package; the Universal 2 Gallery and package job. The arm64 job also launched the native Metal Gallery smoke path.

**Not verified**

- Signing and notarization were skipped because release credentials were unavailable.
- Physical Apple Silicon and Intel acceptance, Japanese IME, Retina or mixed-DPI behavior, traffic-light interaction, VoiceOver, native pointer and keyboard behavior, external-host integration.

### Node editor access, appearance and collapse — 2026-09-12

**Verified**

- The Debug Gallery and Node Editor companion built. `imkit.node_editor` passed host name and color Undo/Redo, borrowed header styles and repeated collapsed-node layout checks.
- The collapse regression reproduced Dear ImGui's cursor-boundary assertion before the fix.
- The native `--verify-node-actions` runner passed 60 public-IO clicks on Output monitor, producing 30 collapse and expand transitions.
- The v3 PR CI later built and packaged the companion on every supported architecture.

**Not verified**

- Native OS mouse and IME input, launcher focus behavior.

### Timeline external-drop preview — 2026-09-12

**Verified**

- The Debug Video test, the Editor API compile fixture and an independent source consumer passed. The checks cover the optional exact candidate-range callback, the preserved single delivery callback and source-compatible aggregate initialization.

**Not verified**

- Release, native OS input, external-host acceptance.

### Preview placement extension — 2026-09-12

**Verified**

- The affected Debug library, API-fixture and Gallery targets built. `imkit.workflow`, `imkit.workflow_api_compile`, `imkit.video`, `imkit.cg`, `imkit.editor_api_compile` and `imkit.consumer_build` passed.
- CPU checks: Fit/Fill/Stretch placement; invalid and extreme geometry; ImageViewport mode recalculation, pan and coordinates; legacy and additive Monitor rendering; UV cropping and `flipY`; all five preview states; action requests; steady-frame allocation; perspective and orthographic projection at 4:3, 16:9, 1:1 and 9:16.
- The Debug native Gallery verifier passed public-IO interaction and OpenGL backbuffer captures for the four ImageViewport modes, every combination of four source aspects, five Monitor states and three placement modes, and uniform CG projection at the four viewport aspects. The evidence in `out/preview-contract-debug/` is not committed. Representative ImageViewport, portrait Loading Monitor and portrait CG captures were inspected visually.
- v2.2.0: Debug and Release built the affected library, Gallery and API fixtures. The workflow, Video, CG, editor API, public API, Window Frame and accessibility checks passed. Source and staged-SDK consumers configured, built and ran in both configurations. The Windows Gallery and SDK were staged for packaging, with the DLL-import inventory inspected.

**Not verified**

- Real camera enumeration and capture, media decoding, physical monitor selection or output, native OS/IME input, external-host integration. Debug and Release builds and synthetic Gallery sources do not establish them.

### Dynamic node sockets — 2026-09-12

**Verified**

- The dedicated Debug node-editor library, API fixture and independent Gallery built. `imkit.node_editor` and `imkit.node_editor_api_compile` passed. The same public fixture built and ran against a separate consumer-owned ImGui target.
- A native Gallery capture was checked visually, and the clipped preview and minimap overlap were corrected. The [review](../archive/node-editor-review.md) records the P0/P1 findings, focused regressions and remaining boundaries. No capture is committed.

**Not verified**

- Physical OS/IME and DPI, assistive technology, performance, external-host integration, Release and distribution.

### Node navigation completion — 2026-09-12

**Verified**

- The focused Debug node test and the native Gallery build passed. A new public-IO regression verifies that minimap clicks navigate without editing the nodes underneath.
- Native captures cover both pages, Comfortable, Touch and Compact density, narrow width, Japanese labels and High Contrast.
- Touch preview clipping was corrected by sizing host views from control metrics, and narrow vector controls keep the exact value in a tooltip. An initial minimap cursor-restoration assertion was fixed before the final pass.

**Not verified**

- Physical IME and DPI, performance, external-host integration.

### Public window frame — 2026-09-11

**Verified**

- The isolated `build/window-frame-public-debug` tree passed the Debug public API unit test and compile fixture, the independent source consumer build, the minimal Gallery build and `--verify-window-frame`.
- The verifier covered all four presets; Theme-derived color regeneration versus complete preset reset; metric and feature layout effects; UTF-8 elision; Win32 caption, client and edge hit tests including `HTMAXBUTTON`; maximize and work-area containment, restore and minimize; and the GLFW close-request path. Captures and reports under `out/window-frame-public/` are not committed.
- GitHub-hosted macOS arm64 and x86_64 compilation and the arm64 automated Gallery smoke now pass.

**Not verified**

- Interactive physical-Mac acceptance: traffic-light controls; drag, full-screen, minimize and zoom; native input; 100% and 200% or mixed-DPI behavior; screen readers; signing; notarization. Automated launch does not establish them.

### Generic workflow extension — 2026-09-11

**Verified**

- Debug workflow tests: notification replacement, expiry, priority and capacity; Step mouse, keyboard and disabled IDs; dismissal; tile selection, resize, cancel and parent disabled semantics; overflow commands; palette actions; modal Begin/End; empty geometry; image fit, fill, clamp and zoom anchor; two context lifetimes.
- Warmed composed controls produced zero measured C++ and ImGui allocations.
- Native API, design-system and Editor Core direct regressions passed. The new API fixture passed in Debug and Release, and source and relocated-SDK consumers compiled, linked and ran.
- The Debug and Release Gallery built. Public IO exercised filter click and cursor zoom. `out/workflow-final/` holds 38 native GPU captures; representative workspace, feedback, tile and narrow Japanese frames were inspected visually.
- The Debug and Release install includes both new headers. The optional Win32 accessibility Release library was built because the existing install manifest requires it.

**Not verified**

- Every state combination, native OS/IME input, real screen-reader operation, external application integration, release publication. This result does not declare the earlier Editor Suite acceptance complete.

## Related records

- Design-system foundation and remaining work: [design-system.md](../architecture/design-system.md#recorded-verification). This is separate from the Editor Suite 2.0 record.
- Editor Suite 2.0 results and boundaries: [Editor refresh](../archive/editor-refresh.md).
- Editor Suite 1.0 (CPU and public IO, source and installed consumers, native GPU, captures, six-operation Release performance): [Editor validation](../archive/editor-validation.md). Native OS/IME and real-project integration remain separate, unperformed categories.

## Limits

- Baseline: Dear ImGui docking commit `367b2c24f399988ddafc0bb4628da0106bcc09be` with default ABI types. Supported artifacts are the release packages (Windows x64 and Arm64, macOS arm64 and x86_64, Universal 2); other combinations need source builds and their own validation.
- Native OS/IME input, physical devices and integration into another application are not tested by this runner.
- Reordering, resizing and drag-drop keep the exact native APIs. Their combinations are not reimplemented or exhaustively retested.
- Native wrappers are immediate. Optional explicit animation affects custom decoration only; fonts, renderer and state ownership stay with the host.
- Input units are separate labels. Native numeric text layout and parsing are retained, including native centering for drags and sliders and native text editing.
- Japanese coverage is limited to the supplied font. Japanese headings use Regular; arbitrary Unicode and emoji coverage is not claimed.
- Screenshot state after interaction can differ from initial values. No synthetic reference image is used as evidence.

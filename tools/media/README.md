# Media production / 動画の制作

**Maintainers only.** These scripts record the Gallery and cut the film and the README GIFs. They are not part of the library, its API or its packages. Intermediate files go to `out/media` (git-ignored); only the final films, poster, GIFs and `provenance.json` are committed.

**保守者向けです。** Gallery を録画し、紹介動画と README の GIF を作る道具です。ライブラリ、その API、パッケージには含まれません。途中のファイルは `out/media`（git 管理外）に置き、コミットするのは、完成した動画、ポスター、GIF、`provenance.json` だけです。

## How it fits together / 構成

| Step / 段階 | Tool / 道具 | Input / 入力 | Output / 出力 |
| --- | --- | --- | --- |
| Record / 録画 | `capture.py` | Scenes in `examples/gallery/main.cpp` (`kScenes`), Debug Gallery | `out/media/native/<take>.mkv` (lossless 1080p60), `docs/images/v4-*.gif` |
| Compose / 編集 | `film.py` | `film.json` (shots on bar lines), `captions.{ja,en}.json`, the takes, the music | `website/public/media/imkit-film-{ja,en}.mp4`, `imkit-poster.jpg`, `provenance.json` |
| Check / 検査 | `verify.py` | The files above | `out/media/media-validation.json` |
| Score / 音楽 | `music.py` | The licensed track named in `film.json`, or a synthesized fallback | Used by `film.py` |

The file names carry no version. The release a film belongs to is `edition` in `film.json`, copied to `provenance.json`; the Pages hero reads it from there.
ファイル名に版は含みません。動画がどのリリースのものかは `film.json` の `edition` で決まり、`provenance.json` に写されます。Pages の先頭の動画は、そこから版を読みます。

## Updating for a release / リリースごとの更新

Only what changed is recorded again, drawn again and committed. / 変わった部分だけを、録り直し、描き直し、コミットします。

```
# 1. Build the Debug Gallery once (through the shared launcher; see AGENTS.md).
# 2. Re-record every take; keep only those whose pixels changed.
python tools/media/capture.py --check

# 3. Edit film.json and the captions if shots or words change. Bump "edition" for a new release.
# 4. See which chunks are stale, then draw them.
python tools/media/film.py --status
python tools/media/film.py

# 5. Look at one still per shot, then check the contract.
python tools/media/film.py --review
python tools/media/verify.py
```

| Change / 変更 | What runs again / 再実行される範囲 |
| --- | --- |
| A caption word / キャプション1語 | The chunks of that shot, in that language / その言語の、そのシーンの塊だけ |
| A shot's length or camera / シーンの長さやカメラ | The chunks that overlap it / 重なる塊 |
| A take's pixels changed / take の画素が変わった | Its GIF, and the chunks that show it / その GIF と、それを映す塊 |
| Gallery change that does not alter a take / take に影響しない Gallery の変更 | Nothing: `capture.py --check` reports it unchanged / 何も。`--check` が「変更なし」と報告 |
| Drawing code in `film.py` / `film.py` の描画コード | Everything: raise `ENGINE_REV` / 全部。`ENGINE_REV` を上げる |
| Music / 音楽 | The audio only: `film.py --audio-only` / 音声だけ。`--audio-only` |

Chunks (6 s each) are cached in `out/media/film/cache/<lang>`. `--force` draws everything. The final concatenation and the audio mix always run; they are cheap next to drawing.
塊（1つ6秒）は `out/media/film/cache/<lang>` に残ります。`--force` で全部を描き直します。最後の結合と音声の合成は毎回行いますが、描画に比べて軽い処理です。

## Adding a take / take を足す

1. Add a scene to `kScenes` and a `Play…` function in `examples/gallery/main.cpp`. Name the rectangles you aim at with probes (`<take>.layout.json` lists them).
2. Add the take to `TAKES` in `capture.py`, with the GIF name or `None`.
3. Add a shot to `film.json` and its captions to both `captions.*.json`. `verify.py` fails when the two languages differ or a key is missing.
4. Add the GIF name to `website/scripts/extract-capture-posters.py` if the Pages should show it.

## Contract / 満たす条件

| Output | Contract |
| --- | --- |
| Films | 1920×1080, 60 fps, about 180 s (96 bars at 128 BPM), about −14 LUFS, true peak at most −1.5 dBTP, at most 40 MiB |
| GIFs | 960×540, at most 2 MiB, 6–8 s |
| Music | Kevin MacLeod ft. DarkNES "Laserpack", CC BY 4.0; the credit stays on the end card, in `THIRD_PARTY_NOTICES.md`, the README and the release notes |
| Evidence | A film shows the Gallery recorded through public Dear ImGui input. It is not validation evidence for native input, IME or hardware |

The audio is never listened to by the scripts: only loudness and true peak are measured. / 音声そのものは聞いて確かめられません。測るのは、ラウドネスとトゥルーピークだけです。

# Third-party notices

## Inter 4.1 (optional host-loaded font asset)

- Official source: https://github.com/rsms/inter/releases/tag/v4.1
- Release archive: https://github.com/rsms/inter/releases/download/v4.1/Inter-4.1.zip
- Unmodified files: `extras/ttf/Inter-Regular.ttf`, `extras/ttf/Inter-SemiBold.ttf`
- Copyright (c) 2016 The Inter Project Authors (https://github.com/rsms/inter)
- License: SIL Open Font License 1.1. Complete original text: [Inter-OFL.txt](assets/fonts/Inter-OFL.txt).
- Font files, manifest and complete license are installed to `share/imkit/fonts` and may be copied beside a host executable with `imkit_copy_font_assets`.
- These are optional assets. The library never loads them or owns the host font atlas. The repository code license does not replace the font license.

| File | SHA256 |
|---|---|
| Inter-Regular.ttf | 40d692fce188e4471e2b3cba937be967878f631ad3ebbbdcd587687c7ebe0c82 |
| Inter-SemiBold.ttf | 78a843fade9d4612a5567302fb595b56976eb5fcebf4fea5a5912d638bafcde3 |
| Inter-OFL.txt | b3195af0fb14368d1b3b10fb9d3fe503b7163ea083859d2ee553bc74da07c320 |

## Noto Sans JP 2.004 (optional host-loaded font asset)

- Official source: https://github.com/notofonts/noto-cjk/tree/Sans2.004
- Pinned commit: `523d033d6cb47f4a80c58a35753646f5c3608a78`.
- Unmodified Japanese subset font: `Sans/SubsetOTF/JP/NotoSansJP-Regular.otf`.
- Embedded font copyright: © 2014-2021 Adobe (http://www.adobe.com/).
- Original license: repository-root `LICENSE`, saved as [NotoSansJP-OFL.txt](assets/fonts/NotoSansJP-OFL.txt); SIL Open Font License 1.1.
- Font, manifest and full license are installed as optional host assets. They are not embedded in `imkit` or loaded by the library.
- Inter remains the primary Latin source. Noto Sans JP Regular supplies missing Japanese glyphs for both body and headings; Japanese headings are not synthesized bold.

| File | SHA256 |
|---|---|
| NotoSansJP-Regular.otf | dff723ba59d57d136764a04b9b2d03205544f7cd785a711442d6d2d085ac5073 |
| NotoSansJP-OFL.txt | 88f117575237307bdd86a17ef15e21790fc9a662fe4dfb103ca1ca077f0d9982 |

## Design benchmarks (reference only; no incorporated software or assets)

The Design Gallery references general principles from the official documentation of the following projects. Their source code, CSS, screenshots, logos, icons, font assets, and example data are not incorporated. Colors, dimensions, drawing code, and geometric icons in the Gallery were authored for these proposals. The projects are not dependencies and no affiliation or endorsement is implied.

Repository license files were checked on 2026-09-09. These repository licenses are not treated as a blanket license for website content, branding, trademarks, or separately licensed assets.

| Reference | Official repository license | Use in this project |
|---|---|---|
| shadcn/ui | [MIT](https://raw.githubusercontent.com/shadcn-ui/ui/main/LICENSE.md) | Semantic color roles and action hierarchy |
| Base UI | [MIT](https://raw.githubusercontent.com/mui/base-ui/master/LICENSE) | Separation of behavior and rendering; composition |
| Radix Themes | [MIT](https://raw.githubusercontent.com/radix-ui/themes/main/LICENSE) | Consistent variants, spacing, radius, and elevation tokens |
| React Aria | [Apache-2.0](https://raw.githubusercontent.com/adobe/react-spectrum/main/LICENSE) | Distinguishing hover, press, focus, and disabled states |
| Mantine | [MIT](https://raw.githubusercontent.com/mantinedev/mantine/master/LICENSE) | Shared theme values across input and overlay families |

For any future source or asset reuse, review the exact file's license and preserve required notices before incorporation. Current benchmark references do not add runtime or production dependencies.

## Dear ImGui

- Source: https://github.com/ocornut/imgui
- Release/tag: `v1.92.9b-docking`
- Commit: `b48d1afbe8ee8b238e2961dc363a949dd7304e23`
- License: MIT
- Used files: core sources, public headers, demo source, and the official GLFW/OpenGL3 backend

The MIT License (MIT)

Copyright (c) 2014-2026 Omar Cornut

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

## GLFW

- Source: https://github.com/glfw/glfw
- Release/tag: `3.5.1`
- Commit: `d9d6f0f1f967807ffade6598ea9a631ebaf37a56`
- License: zlib/libpng
- Used files: window, input, and OpenGL context support for the Gallery only

Copyright (c) 2002-2006 Marcus Geelnard

Copyright (c) 2006-2019 Camilla Löwy

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

3. This notice may not be removed or altered from any source
   distribution.

## Generated icon assets

The original 238 icons under `assets/icons` were individually generated with the built-in
OpenAI image generation tool for this project. They are not copied from the design
reference projects mentioned above. Their prompts and source hashes accompany the
assets. These project assets are distributed under the repository MIT license.

The 42 additional design-system icons are original deterministic code drawings in
`tools/design_icons.py`, distributed under the repository MIT license. No third-party
icon shapes were imported. Seven raster levels (12–64px) are generated from the recorded sources.

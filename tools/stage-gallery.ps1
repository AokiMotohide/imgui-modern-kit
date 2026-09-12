param(
    [string]$BuildDirectory = "build/windows-debug",
    [string]$OutputDirectory = "out/release"
)
$ErrorActionPreference = "Stop"
$root = (Resolve-Path (Split-Path $PSScriptRoot -Parent)).Path
$build = (Resolve-Path (Join-Path $root $BuildDirectory)).Path
$out = [IO.Path]::GetFullPath((Join-Path $root $OutputDirectory))
if (-not $out.StartsWith($root + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
    throw "Output must be inside the workspace."
}
$catalog = Join-Path $build "catalog/Release"
$exe = Join-Path $catalog "imkit_gallery.exe"
$assets = Join-Path $catalog "design-assets"
foreach ($required in @($exe, $assets, (Join-Path $root "LICENSE"), (Join-Path $root "THIRD_PARTY_NOTICES.md"))) {
    if (-not (Test-Path -LiteralPath $required)) { throw "Missing Gallery release input: $required" }
}
$stage = Join-Path $out "gallery"
$stageFull = [IO.Path]::GetFullPath($stage)
if (-not $stageFull.StartsWith($out + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
    throw "Gallery stage must be inside the requested output directory."
}
if (Test-Path -LiteralPath $stageFull) { Remove-Item -LiteralPath $stageFull -Recurse -Force }
New-Item -ItemType Directory -Force $stageFull | Out-Null
Copy-Item -LiteralPath $exe -Destination (Join-Path $stageFull "imkit_gallery.exe")
Copy-Item -LiteralPath $assets -Destination (Join-Path $stageFull "design-assets") -Recurse
Copy-Item -LiteralPath (Join-Path $root "LICENSE") -Destination (Join-Path $stageFull "LICENSE")
Copy-Item -LiteralPath (Join-Path $root "THIRD_PARTY_NOTICES.md") -Destination (Join-Path $stageFull "THIRD_PARTY_NOTICES.md")

$compilerFile = Get-ChildItem (Join-Path $build "CMakeFiles") -Filter CMakeCXXCompiler.cmake -Recurse | Select-Object -First 1
if (-not $compilerFile) { throw "Could not locate the configured C++ compiler." }
$compilerText = Get-Content $compilerFile.FullName -Raw
$compiler = [regex]::Match($compilerText, 'set\(CMAKE_CXX_COMPILER "([^"]+)"\)').Groups[1].Value
$linker = Join-Path (Split-Path $compiler -Parent) "link.exe"
if (-not (Test-Path -LiteralPath $linker)) { throw "Could not locate link.exe beside the configured C++ compiler." }
$dump = @(& $linker /dump /dependents $exe 2>&1)
if ($LASTEXITCODE -ne 0) { throw "Could not inspect Gallery runtime dependencies." }
$sanitizedDump = $dump -replace [regex]::Escape($exe), "imkit_gallery.exe"
$sanitizedDump | Set-Content -LiteralPath (Join-Path $stageFull "DEPENDENCIES.txt") -Encoding utf8
$dependencies = @($dump |
    ForEach-Object { if ($_ -match '^\s+([A-Za-z0-9_.-]+\.dll)\s*$') { $Matches[1].ToUpperInvariant() } } |
    Where-Object { $_ } |
    Sort-Object -Unique)
if ($dependencies.Count -eq 0) { throw "No Gallery runtime dependencies were detected." }
$allowed = @(
    "KERNEL32.DLL", "USER32.DLL", "GDI32.DLL", "ADVAPI32.DLL", "SHELL32.DLL",
    "OLE32.DLL", "OLEAUT32.DLL", "COMCTL32.DLL", "DWMAPI.DLL", "OPENGL32.DLL",
    "WINDOWSCODECS.DLL", "IMM32.DLL", "VCRUNTIME140.DLL", "VCRUNTIME140_1.DLL", "MSVCP140.DLL", "MSVCP140_2.DLL",
    "API-MS-WIN-CRT-RUNTIME-L1-1-0.DLL", "API-MS-WIN-CRT-CONVERT-L1-1-0.DLL",
    "API-MS-WIN-CRT-FILESYSTEM-L1-1-0.DLL", "API-MS-WIN-CRT-HEAP-L1-1-0.DLL",
    "API-MS-WIN-CRT-LOCALE-L1-1-0.DLL", "API-MS-WIN-CRT-MATH-L1-1-0.DLL",
    "API-MS-WIN-CRT-STDIO-L1-1-0.DLL", "API-MS-WIN-CRT-STRING-L1-1-0.DLL",
    "API-MS-WIN-CRT-TIME-L1-1-0.DLL", "API-MS-WIN-CRT-UTILITY-L1-1-0.DLL", "UCRTBASE.DLL"
)
$unexpected = @($dependencies | Where-Object { $_ -notin $allowed })
if ($unexpected.Count -gt 0) { throw "Unexpected Gallery runtime dependency: $($unexpected -join ', ')" }

@'
# ImKit Gallery / Gallery 実行方法

Run `imkit_gallery.exe` from this directory. Keep `design-assets` beside the executable.

実行ファイルと同じdirectoryで `imkit_gallery.exe` を実行してください。`design-assets` は移動・削除しないでください。

- Platform: Windows x64
- Runtime: the Microsoft Visual C++ Redistributable x64 may be required; it is not bundled in this archive.
- This archive installs no service and creates no application configuration.
- `DEPENDENCIES.txt` records the executable's inspected DLL imports.

ImKit is MIT licensed. Dear ImGui, GLFW and optional font assets retain their licenses; read `LICENSE` and `THIRD_PARTY_NOTICES.md` before redistribution. This archive contains no newly added third-party image, icon, font, code or media asset.
'@ | Set-Content -LiteralPath (Join-Path $stageFull "RUN-GALLERY.md") -Encoding utf8

Write-Output "Gallery stage: $stageFull"

param(
    [string]$BuildDirectory = "build/windows-debug",
    [string]$OutputDirectory = "out/release"
)
$ErrorActionPreference = "Stop"
$root = Split-Path $PSScriptRoot -Parent
$build = (Resolve-Path (Join-Path $root $BuildDirectory)).Path
$out = [IO.Path]::GetFullPath((Join-Path $root $OutputDirectory))
if (-not $out.StartsWith($root + [IO.Path]::DirectorySeparatorChar)) { throw "Output must be inside the workspace." }
New-Item -ItemType Directory -Force $out | Out-Null
$stage = Join-Path $out "sdk"
foreach ($config in @("Debug", "Release")) {
    & cmake --install $build --config $config --prefix $stage
    if ($LASTEXITCODE -ne 0) { throw "Install failed: $config" }
}
# Archive member names otherwise carry the absolute build directory. Rebuild
# the archive with relative member names and normalized debug filenames.
# Executable sections, relocations and symbol/record offsets remain unchanged.
$compilerFile = Get-ChildItem (Join-Path $build "CMakeFiles") -Filter CMakeCXXCompiler.cmake -Recurse | Select-Object -First 1
$compilerText = Get-Content $compilerFile.FullName -Raw
$compiler = [regex]::Match($compilerText, 'set\(CMAKE_CXX_COMPILER "([^"]+)"\)').Groups[1].Value
$librarian = Join-Path (Split-Path $compiler -Parent) "lib.exe"
foreach ($config in @("Debug", "Release")) {
    $objectDir = Join-Path $build "imkit.dir/$config"
    $name = if ($config -eq "Debug") { "imkitd.lib" } else { "imkit.lib" }
    $library = Join-Path $stage "lib/$name"
    $copyDir = Join-Path $out "objects/$config"
    New-Item -ItemType Directory -Force $copyDir | Out-Null
    foreach ($object in @("widgets.obj", "theme.obj", "components.obj")) {
        & python (Join-Path $PSScriptRoot "normalize-coff.py") (Join-Path $objectDir $object) (Join-Path $copyDir $object)
        if ($LASTEXITCODE -ne 0) { throw "Object normalization failed." }
    }
    Push-Location $copyDir
    try {
        & $librarian /NOLOGO /Brepro "/OUT:$library" widgets.obj theme.obj components.obj
        if ($LASTEXITCODE -ne 0) { throw "Library repack failed." }
    } finally { Pop-Location }
}
Write-Output "SDK stage: $stage"

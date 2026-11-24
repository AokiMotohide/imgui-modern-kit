[CmdletBinding()]
param(
    [ValidateSet('windows-debug', 'windows-arm64')]
    [string]$Preset = 'windows-debug',

    [string[]]$Target = @(),

    [string[]]$ChangedFiles = @(),

    [string[]]$CompileOnly = @(),

    [switch]$Configure,

    [switch]$Tests,

    [switch]$AllTests,

    [switch]$BuildAndTest,

    [switch]$VerboseBuild,

    [string]$TestRegex = ''
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$projectRoot = 'C:\aokiDev\imgui-modern-kit'
$buildRoot = Join-Path $projectRoot 'build'
$buildDir = Join-Path $buildRoot $Preset
$sharedHelper = 'C:\aokiDev\tools\build-coordination\Invoke-SharedBuild.ps1'
$invocationProjectRoot = [System.IO.Path]::GetFullPath((Split-Path -Parent $PSScriptRoot))
if (-not [string]::Equals(
        $invocationProjectRoot,
        $projectRoot,
        [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "共有ビルドは主チェックアウトからのみ実行できます: $invocationProjectRoot"
}
if ($Preset -notin @('windows-debug', 'windows-arm64') -or
    -not [System.IO.Path]::GetFullPath($buildDir).StartsWith(
        [System.IO.Path]::GetFullPath($buildRoot) + '\',
        [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "登録済みの主チェックアウトbuild profileではありません: $buildDir"
}

if (-not (Test-Path -LiteralPath $sharedHelper -PathType Leaf)) {
    throw "共有ビルドヘルパーが見つかりません: $sharedHelper"
}

function ConvertTo-PowerShellLiteral {
    param([Parameter(Mandatory = $true)][string]$Value)
    return "'" + $Value.Replace("'", "''") + "'"
}

function Invoke-SharedHelperProcess {
    param(
        [Parameter(Mandatory = $true)][string]$Executable,
        [Parameter(Mandatory = $true)][string[]]$Arguments
    )

    $helperCall = @(
        '&', (ConvertTo-PowerShellLiteral $sharedHelper),
        '-ProjectRoot', (ConvertTo-PowerShellLiteral $projectRoot),
        '-BuildRoot', (ConvertTo-PowerShellLiteral $buildRoot),
        '-SourceRoot', (ConvertTo-PowerShellLiteral $projectRoot),
        '-Executable', (ConvertTo-PowerShellLiteral $Executable),
        '-Arguments', ('@(' + (($Arguments | ForEach-Object { ConvertTo-PowerShellLiteral ([string]$_) }) -join ', ') + ')')
    ) -join ' '
    $encodedCommand = [Convert]::ToBase64String([System.Text.Encoding]::Unicode.GetBytes($helperCall))

    $powerShellExe = Join-Path $PSHOME 'powershell.exe'
    if (-not (Test-Path -LiteralPath $powerShellExe -PathType Leaf)) {
        $powerShellExe = Join-Path $PSHOME 'pwsh.exe'
    }
    if (-not (Test-Path -LiteralPath $powerShellExe -PathType Leaf)) {
        throw "PowerShell実行ファイルが見つかりません: $PSHOME"
    }

    $process = Start-Process -FilePath $powerShellExe `
        -ArgumentList @('-NoLogo', '-NoProfile', '-EncodedCommand', $encodedCommand) `
        -NoNewWindow -Wait -PassThru
    if ($process.ExitCode -ne 0) {
        throw "共有ビルドが失敗しました: exitCode=$($process.ExitCode)"
    }
}

function Expand-ListArgument {
    param([string[]]$Values)
    $out = @()
    foreach ($v in $Values) {
        if ([string]::IsNullOrWhiteSpace($v)) { continue }
        foreach ($part in ($v -split ',')) {
            $trimmed = $part.Trim()
            if (-not [string]::IsNullOrWhiteSpace($trimmed)) {
                $out += $trimmed
            }
        }
    }
    return $out
}

function Find-MSBuildExecutable {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (Test-Path -LiteralPath $vswhere -PathType Leaf) {
        $found = & $vswhere -latest -prerelease -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\amd64\MSBuild.exe' 2>$null | Select-Object -First 1
        if ($found -and (Test-Path -LiteralPath $found -PathType Leaf)) {
            return $found
        }
    }
    $cachePath = Join-Path $buildDir 'CMakeCache.txt'
    if (Test-Path -LiteralPath $cachePath -PathType Leaf) {
        $linkerLine = Get-Content -LiteralPath $cachePath -Encoding UTF8 | Where-Object { $_ -match '^CMAKE_LINKER:FILEPATH=(.+)$' } | Select-Object -First 1
        if ($linkerLine -match '^CMAKE_LINKER:FILEPATH=(.+)$') {
            $dir = Split-Path -Parent $Matches[1]
            for ($i = 0; $i -lt 8 -and $dir; $i++) {
                $cand = Join-Path $dir 'MSBuild\Current\Bin\amd64\MSBuild.exe'
                if (Test-Path -LiteralPath $cand -PathType Leaf) {
                    return $cand
                }
                $dir = Split-Path -Parent $dir
            }
        }
    }
    $cmd = Get-Command 'MSBuild.exe' -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    throw 'MSBuild.exe が見つかりませんでした。'
}

$Target = @(Expand-ListArgument $Target)
$ChangedFiles = @(Expand-ListArgument $ChangedFiles)
$CompileOnly = @(Expand-ListArgument $CompileOnly)

$configName = if ($Preset -eq 'windows-debug') { 'Debug' } else { 'Release' }
$platformName = if ($Preset -eq 'windows-debug') { 'x64' } else { 'ARM64' }

if ($CompileOnly.Count -gt 0) {
    if ($Configure -or $Tests) {
        throw '-CompileOnly は -Configure / -Tests と同時に指定できません。'
    }
    $msbuild = Find-MSBuildExecutable
    $projFiles = @(Get-ChildItem -LiteralPath $buildDir -Filter '*.vcxproj' -File | Where-Object {
        $_.Name -notin @('ALL_BUILD.vcxproj', 'ZERO_CHECK.vcxproj', 'RUN_TESTS.vcxproj', 'PACKAGE.vcxproj', 'INSTALL.vcxproj')
    })
    $grouped = @{}
    foreach ($rawPath in $CompileOnly) {
        $absPath = if ([System.IO.Path]::IsPathRooted($rawPath)) {
            [System.IO.Path]::GetFullPath($rawPath)
        } else {
            [System.IO.Path]::GetFullPath((Join-Path $projectRoot $rawPath))
        }
        if (-not (Test-Path -LiteralPath $absPath -PathType Leaf)) {
            throw "CompileOnly の対象ファイルが存在しません: $absPath"
        }
        $escaped = [regex]::Escape($absPath)
        $matches = @($projFiles | Where-Object {
            (Get-Content -LiteralPath $_.FullName -Raw -Encoding UTF8) -match ('Include="' + $escaped + '"')
        })
        if ($matches.Count -eq 0) {
            throw "対象ファイルを含む .vcxproj が $buildDir に見つかりません（必要なら -Configure を先に実行してください）: $absPath"
        }
        # Prefer imkit.vcxproj or imkit_gallery.vcxproj when a helper file is shared by tests.
        $chosen = ($matches | Where-Object { $_.BaseName -in @('imkit', 'imkit_gallery', 'imkit_show', 'imkit_video', 'imkit_node_editor') } | Select-Object -First 1)
        if ($null -eq $chosen) { $chosen = $matches[0] }
        if (-not $grouped.ContainsKey($chosen.FullName)) {
            $grouped[$chosen.FullName] = [System.Collections.Generic.List[string]]::new()
        }
        $grouped[$chosen.FullName].Add($absPath)
    }
    foreach ($projPath in $grouped.Keys) {
        $selected = ($grouped[$projPath] -join '%3B')
        $msbuildArgs = @(
            $projPath,
            '-t:ClCompile',
            "-p:SelectedFiles=$selected",
            '-p:BuildProjectReferences=false',
            "-p:Configuration=$configName",
            "-p:Platform=$platformName",
            '-nr:false',
            '-p:UseMultiToolTask=true',
            '-p:EnforceProcessCountAcrossBuilds=true',
            '-nologo',
            '-v:minimal'
        )
        Invoke-SharedHelperProcess -Executable $msbuild -Arguments $msbuildArgs
    }
    return
}

if ($ChangedFiles.Count -gt 0) {
    if ($Configure -or $Tests) {
        throw '-ChangedFiles は -Configure / -Tests と同時に指定できません。'
    }
    $affectedJson = & python (Join-Path $PSScriptRoot 'affected_targets.py') '--json' @ChangedFiles
    if ($LASTEXITCODE -ne 0) {
        throw 'affected_targets.py の実行に失敗しました。'
    }
    $plan = $affectedJson | ConvertFrom-Json
    if (-not $plan.build_required) {
        Write-Host '[SharedBuild] ビルド不要（文書・非コード変更のみ）'
        if ($plan.docs_check) {
            Write-Host '推奨確認: python tools/check_docs.py'
        }
        return
    }
    if ($plan.configure_required) {
        Write-Host '[SharedBuild] CMake 構成変更を検出したため先に -Configure を実行します。'
        Invoke-SharedHelperProcess -Executable 'cmake.exe' -Arguments @('--preset', $Preset)
    }
    if ($Target.Count -eq 0 -and $plan.targets.Count -gt 0) {
        $Target = @($plan.targets)
    }
    if ([string]::IsNullOrWhiteSpace($TestRegex) -and -not [string]::IsNullOrWhiteSpace([string]$plan.test_regex)) {
        $TestRegex = [string]$plan.test_regex
    }
}

if ($Tests) {
    if ($Configure) {
        throw '-Tests と -Configure は同時に指定できません。'
    }
    if ([string]::IsNullOrWhiteSpace($TestRegex) -and -not $AllTests) {
        throw '-Tests 実行時は -TestRegex を指定してください（全テストは明示的に -AllTests を指定）。'
    }
    $arguments = @('--test-dir', $buildDir, '-C', $configName)
    if (-not [string]::IsNullOrWhiteSpace($TestRegex)) {
        $arguments += @('-R', $TestRegex)
    }
    $arguments += '--output-on-failure'
    Invoke-SharedHelperProcess -Executable 'ctest.exe' -Arguments $arguments
    return
}

if ($Configure) {
    Invoke-SharedHelperProcess -Executable 'cmake.exe' -Arguments @('--preset', $Preset)
    return
}

if ($Target.Count -eq 0) {
    throw '-Target / -ChangedFiles / -CompileOnly / -Configure / -Tests のいずれかを指定してください（誤爆防止のため既定ターゲットはありません）。'
}

$slnPath = @((Join-Path $buildDir 'imgui-modern-kit.slnx'), (Join-Path $buildDir 'imgui-modern-kit.sln')) |
    Where-Object { Test-Path -LiteralPath $_ -PathType Leaf } |
    Select-Object -First 1

if ($Target.Count -gt 1 -and $slnPath) {
    $msbuild = Find-MSBuildExecutable
    $msbuildArgs = @(
        $slnPath,
        ('-t:' + ($Target -join ';')),
        '-m',
        '-nr:false',
        "-p:Configuration=$configName",
        "-p:Platform=$platformName",
        '-p:UseMultiToolTask=true',
        '-p:EnforceProcessCountAcrossBuilds=true'
    )
    if (-not $VerboseBuild) {
        $msbuildArgs += @('-nologo', '-v:minimal')
    }
    Invoke-SharedHelperProcess -Executable $msbuild -Arguments $msbuildArgs
} else {
    $buildArgs = @('--build', $buildDir, '--config', $configName, '--target') + $Target + @('--parallel', '--', '/m', '/nr:false', '/p:UseMultiToolTask=true', '/p:EnforceProcessCountAcrossBuilds=true')
    if (-not $VerboseBuild) {
        $buildArgs += @('/nologo', '/v:minimal')
    }
    Invoke-SharedHelperProcess -Executable 'cmake.exe' -Arguments $buildArgs
}

if ($BuildAndTest) {
    if ([string]::IsNullOrWhiteSpace($TestRegex) -and -not $AllTests) {
        Write-Host '[SharedBuild] -BuildAndTest: 対応する -TestRegex がないためテストをスキップしました。'
        return
    }
    $testArgs = @('--test-dir', $buildDir, '-C', $configName)
    if (-not [string]::IsNullOrWhiteSpace($TestRegex)) {
        $testArgs += @('-R', $TestRegex)
    }
    $testArgs += '--output-on-failure'
    Invoke-SharedHelperProcess -Executable 'ctest.exe' -Arguments $testArgs
}

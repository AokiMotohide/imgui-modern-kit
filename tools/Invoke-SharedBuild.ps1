[CmdletBinding()]
param(
    [ValidateSet('windows-debug', 'windows-arm64')]
    [string]$Preset = 'windows-debug',

    [string[]]$Target = @('imkit_gallery'),

    [switch]$Configure,

    [switch]$Tests,

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

if ($Tests) {
    if ($Configure) {
        throw '-Tests と -Configure は同時に指定できません。'
    }
    $executable = 'ctest.exe'
    $arguments = @('--test-dir', $buildDir, '-C', $(if ($Preset -eq 'windows-debug') { 'Debug' } else { 'Release' }))
    if (-not [string]::IsNullOrWhiteSpace($TestRegex)) {
        $arguments += @('-R', $TestRegex)
    }
    $arguments += '--output-on-failure'
} elseif ($Configure) {
    $executable = 'cmake.exe'
    $arguments = @('--preset', $Preset)
} else {
    $executable = 'cmake.exe'
    $arguments = @('--build', $buildDir, '--config', $(if ($Preset -eq 'windows-debug') { 'Debug' } else { 'Release' }))
    if ($Target.Count -gt 0) {
        $arguments += '--target'
        $arguments += $Target
    }
    $arguments += '--parallel'
}

function ConvertTo-PowerShellLiteral {
    param([Parameter(Mandatory = $true)][string]$Value)
    return "'" + $Value.Replace("'", "''") + "'"
}

$helperCall = @(
    '&', (ConvertTo-PowerShellLiteral $sharedHelper),
    '-ProjectRoot', (ConvertTo-PowerShellLiteral $projectRoot),
    '-BuildRoot', (ConvertTo-PowerShellLiteral $buildRoot),
    '-SourceRoot', (ConvertTo-PowerShellLiteral $projectRoot),
    '-Executable', (ConvertTo-PowerShellLiteral $executable),
    '-Arguments', ('@(' + (($arguments | ForEach-Object { ConvertTo-PowerShellLiteral ([string]$_) }) -join ', ') + ')')
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

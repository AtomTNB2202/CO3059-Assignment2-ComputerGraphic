param(
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Debug',
    [switch]$Run,
    [switch]$Test
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path -LiteralPath $vswhere)) { throw 'Install Visual Studio Build Tools with Desktop development with C++.' }
$installation = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -format json | ConvertFrom-Json
if (-not $installation) { throw 'No Visual Studio C++ toolchain found.' }
$vsRoot = $installation.installationPath
$cmake = Join-Path $vsRoot 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
if (-not (Test-Path -LiteralPath $cmake)) { throw 'Enable C++ CMake tools for Windows in Visual Studio Installer.' }
$version = [version]$installation.installationVersion
if ($version.Major -eq 18) { $generator = 'Visual Studio 18 2026' }
elseif ($version.Major -eq 17) { $generator = 'Visual Studio 17 2022' }
else { throw 'This script supports Visual Studio 2022 and 2026.' }
$buildDirectory = Join-Path $projectRoot "out\build\vs$($version.Major)"
& $cmake -S $projectRoot -B $buildDirectory -G $generator -A x64
if ($LASTEXITCODE -ne 0) { throw 'CMake configuration failed.' }
& $cmake --build $buildDirectory --config $Configuration --parallel
if ($LASTEXITCODE -ne 0) { throw 'Build failed.' }
if ($Test) {
    $ctest = Join-Path (Split-Path -Parent $cmake) 'ctest.exe'
    & $ctest --test-dir $buildDirectory -C $Configuration --output-on-failure
    if ($LASTEXITCODE -ne 0) { throw 'Tests failed.' }
}
$executable = Join-Path $buildDirectory "$Configuration\ComputerGraphicsAssignment1.exe"
Write-Host "Executable: $executable"
if ($Run) { & $executable }

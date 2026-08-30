param(
    [string]$QtVersion = "6.6.3",
    [string]$QtRoot = "",
    [string]$QtToolsRoot = "",
    [switch]$RunTests
)

$ErrorActionPreference = "Stop"
[Console]::OutputEncoding = [System.Text.UTF8Encoding]::new($false)
$OutputEncoding = [Console]::OutputEncoding
$ProjectRoot = Split-Path -Parent $PSScriptRoot

. (Join-Path $PSScriptRoot "qt-toolchain.ps1")
$toolchain = Get-QtToolchain -Version $QtVersion -QtRoot $QtRoot -QtToolsRoot $QtToolsRoot
$QtRoot      = $toolchain.QtRoot
$MingwBin    = $toolchain.MingwBin
$NinjaExe    = $toolchain.NinjaExe
$CmakeExe    = $toolchain.CmakeExe
$WinDeployQt = $toolchain.WinDeployQt

$env:PATH = "$MingwBin;$(Join-Path $QtRoot 'bin');$env:PATH"

$buildDir = Join-Path $ProjectRoot "build"
if (-not (Test-Path $buildDir)) {
    & $CmakeExe -S $ProjectRoot -B $buildDir -G Ninja `
        -DCMAKE_MAKE_PROGRAM="$NinjaExe" `
        -DCMAKE_PREFIX_PATH="$QtRoot" `
        -DCMAKE_BUILD_TYPE=Release
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

& $CmakeExe --build $buildDir
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

# 部署 Qt 运行时到 build 目录 exe 旁：安装进 Keil 工程的钩子直接调用本 exe，
# Keil 进程环境没有 Qt PATH，必须自包含。
& $WinDeployQt --release --qmldir (Join-Path $ProjectRoot "qml") `
    (Join-Path $buildDir "KeilPluginStudio.exe")
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

if ($RunTests) {
    & $CmakeExe --build $buildDir --target test
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

$DistExecutable = Join-Path $ProjectRoot "dist\KeilPluginStudio\KeilPluginStudio.exe"
if (-not (Test-Path -LiteralPath $DistExecutable)) {
    throw "构建完成但发布目录同步失败：$DistExecutable"
}
Write-Host "发布目录已同步：$DistExecutable"

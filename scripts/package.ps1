param(
    [string]$QtRoot = "E:\QT\6.6.3\mingw_64",
    [string]$QtToolsRoot = "E:\QT\Tools"
)

$ErrorActionPreference = "Stop"
[Console]::OutputEncoding = [System.Text.UTF8Encoding]::new($false)
$OutputEncoding = [Console]::OutputEncoding
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$Executable = Join-Path $ProjectRoot "build\KeilPluginStudio.exe"
$DeployDirectory = Join-Path $ProjectRoot "dist\KeilPluginStudio"
$DeployExecutable = Join-Path $DeployDirectory "KeilPluginStudio.exe"
$Readme = Join-Path $ProjectRoot "README.md"
$DeployReadme = Join-Path $DeployDirectory "README.md"
$WinDeployQt = Join-Path $QtRoot "bin\windeployqt.exe"

if (-not (Test-Path -LiteralPath $Executable)) {
    & (Join-Path $PSScriptRoot "build.ps1") -QtRoot $QtRoot -QtToolsRoot $QtToolsRoot
}
if (-not (Test-Path -LiteralPath $WinDeployQt)) {
    throw "未找到 windeployqt：$WinDeployQt"
}

New-Item -ItemType Directory -Force -Path $DeployDirectory | Out-Null
Copy-Item -LiteralPath $Executable -Destination $DeployExecutable -Force
if (Test-Path -LiteralPath $Readme) {
    Copy-Item -LiteralPath $Readme -Destination $DeployReadme -Force
}

$env:PATH = "$(Join-Path $QtToolsRoot 'mingw1120_64\bin');$(Join-Path $QtRoot 'bin');$env:PATH"
& $WinDeployQt --release --compiler-runtime --qmldir (Join-Path $ProjectRoot "qml") $DeployExecutable
if ($LASTEXITCODE -ne 0) {
    throw "Qt 依赖部署失败，退出码 $LASTEXITCODE"
}

Write-Host "部署完成：$DeployExecutable"
Write-Host "提示：dist 为自包含发布目录；如以 dist 作为日常使用位置，请在 dist 的 exe 上重新安装一次插件钩子（钩子记录的是安装时的 exe 路径）。"

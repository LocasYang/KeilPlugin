# Qt 工具链定位：不写死盘符，按版本号在各个固定盘的 QT\<版本>\mingw_64 自动
# 探测（Qt 在线安装器的标准布局，Tools 与版本目录同级）；也可用 QT_ROOT 环境
# 变量或 -QtRoot/-QtToolsRoot 显式指定。QtRoot 指 kit 目录（...\6.6.3\mingw_64）。

function Get-QtToolchain {
    param(
        [string]$Version = "6.6.3",
        [string]$QtRoot = "",
        [string]$QtToolsRoot = ""
    )

    if (-not $QtRoot) {
        if ($env:QT_ROOT -and (Test-Path (Join-Path $env:QT_ROOT "bin\windeployqt.exe"))) {
            $QtRoot = $env:QT_ROOT
        } else {
            foreach ($drive in (Get-PSDrive -PSProvider FileSystem).Root) {
                $candidate = Join-Path $drive "QT\$Version\mingw_64"
                if (Test-Path (Join-Path $candidate "bin\windeployqt.exe")) {
                    $QtRoot = $candidate
                    break
                }
            }
        }
        if (-not $QtRoot) {
            throw "未找到 Qt $Version (mingw_64)：请安装到任意盘的 QT\$Version\mingw_64，" +
                  "或设置 QT_ROOT / 传 -QtRoot 指定"
        }
    }
    $WinDeployQt = Join-Path $QtRoot "bin\windeployqt.exe"
    if (-not (Test-Path -LiteralPath $WinDeployQt)) {
        throw "windeployqt 不存在：$WinDeployQt（检查 -QtRoot / QT_ROOT 是否指向 mingw_64 目录）"
    }

    if (-not $QtToolsRoot) {
        $QtToolsRoot = Join-Path (Split-Path -Parent (Split-Path -Parent $QtRoot)) "Tools"
    }
    $mingwMatch = Get-Item (Join-Path $QtToolsRoot "mingw*_64\bin") -ErrorAction SilentlyContinue |
        Sort-Object FullName | Select-Object -First 1
    if (-not $mingwMatch) {
        throw "未找到 MinGW 工具链：$QtToolsRoot\mingw*_64\bin"
    }

    return @{
        QtRoot      = $QtRoot
        QtToolsRoot = $QtToolsRoot
        MingwBin    = $mingwMatch.FullName
        NinjaExe    = Join-Path $QtToolsRoot "Ninja\ninja.exe"
        CmakeExe    = Join-Path $QtToolsRoot "CMake_64\bin\cmake.exe"
        WinDeployQt = $WinDeployQt
    }
}

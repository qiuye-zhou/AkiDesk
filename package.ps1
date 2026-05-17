# AkiDesk 打包脚本
# 用法: 右键 → 使用 PowerShell 运行, 或在终端中执行: .\package.ps1

$ErrorActionPreference = "Stop"

# ============================================================
# 路径配置（根据实际安装位置修改）
# ============================================================
$ProjectDir   = $PSScriptRoot
$BuildDir     = "$ProjectDir\build\Desktop_Qt_6_11_0_MinGW_64_bit-Release"
$DistDir      = "$ProjectDir\dist"
$InstallerDir = "$ProjectDir\installer"
$CMakeBinDir  = "E:\QT\Tools\CMake_64\bin"
$QtBinDir     = "E:\QT\6.11.0\mingw_64\bin"
$MinGWBinDir  = "E:\QT\Tools\mingw1310_64\bin"

# Inno Setup 编译器路径（按优先级查找）
$ISCCPaths = @(
    "C:\Program Files (x86)\Inno Setup 6\ISCC.exe",
    "C:\Program Files\Inno Setup 6\ISCC.exe"
)

# 将 Qt 工具链加入临时 PATH（cmake / MinGW / windeployqt）
$env:PATH = "$CMakeBinDir;$QtBinDir;$MinGWBinDir;$env:PATH"

# ============================================================
# 颜色输出函数
# ============================================================
function Write-Step($msg) { Write-Host "`n>>> $msg" -ForegroundColor Cyan }
function Write-OK($msg)   { Write-Host "    $msg" -ForegroundColor Green }
function Write-Warn($msg) { Write-Host "    $msg" -ForegroundColor Yellow }
function Write-Fail($msg) { Write-Host "    $msg" -ForegroundColor Red }

# ============================================================
# 第 1 步：构建 Release
# ============================================================
Write-Step "第 1 步：构建 Release 版本"

$ExePath = "$BuildDir\AkiDesk.exe"
if (Test-Path $ExePath) {
    $exeTime = (Get-Item $ExePath).LastWriteTime
    Write-Warn "AkiDesk.exe 已存在 ($exeTime)"
    $rebuild = Read-Host "    是否重新构建? (y/N)"
    if ($rebuild -eq 'y' -or $rebuild -eq 'Y') {
        Write-Host "    正在构建..."
        cmake --build $BuildDir --config Release
        if ($LASTEXITCODE -ne 0) { Write-Fail "构建失败"; exit 1 }
        Write-OK "构建完成"
    } else {
        Write-OK "跳过构建，使用现有 AkiDesk.exe"
    }
} else {
    Write-Warn "未找到 AkiDesk.exe，开始构建..."
    cmake --build $BuildDir --config Release
    if ($LASTEXITCODE -ne 0) { Write-Fail "构建失败"; exit 1 }
    Write-OK "构建完成"
}

# ============================================================
# 第 2 步：windeployqt — 收集 Qt 依赖
# ============================================================
Write-Step "第 2 步：收集 Qt 运行时依赖"

# 清理旧的 dist 目录
if (Test-Path $DistDir) {
    Remove-Item $DistDir -Recurse -Force
}
New-Item -ItemType Directory -Path $DistDir | Out-Null

# 复制主程序
Copy-Item $ExePath $DistDir
Write-OK "已复制 AkiDesk.exe"

# 将 Qt/MinGW 的 bin 加入临时 PATH
$env:PATH = "$QtBinDir;$MinGWBinDir;$env:PATH"

# 运行 windeployqt
Write-Host "    正在运行 windeployqt..."
& "$QtBinDir\windeployqt.exe" "$DistDir\AkiDesk.exe" --release
if ($LASTEXITCODE -ne 0) {
    Write-Fail "windeployqt 失败"
    exit 1
}
Write-OK "Qt 依赖收集完成"

# 复制 MinGW 运行时 DLL（windeployqt 不会处理这些）
$mingwDlls = @("libgcc_s_seh-1.dll", "libstdc++-6.dll", "libwinpthread-1.dll")
foreach ($dll in $mingwDlls) {
    $src = "$MinGWBinDir\$dll"
    if (Test-Path $src) {
        Copy-Item $src $DistDir -Force
        Write-OK "已复制 $dll"
    }
}

# 统计文件
$dllCount = (Get-ChildItem $DistDir -Filter "*.dll" -Recurse).Count
$dirCount = (Get-ChildItem $DistDir -Directory).Count
Write-OK "共 $dllCount 个 DLL，$dirCount 个子目录"

# ============================================================
# 第 3 步：Inno Setup 生成安装包
# ============================================================
Write-Step "第 3 步：生成安装包"

# 查找 Inno Setup 编译器
$ISCC = $null
foreach ($p in $ISCCPaths) {
    if (Test-Path $p) {
        $ISCC = $p
        break
    }
}

# 尝试通过注册表查找
if (-not $ISCC) {
    $regPaths = @(
        "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Inno Setup 6_is1",
        "HKLM:\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\Inno Setup 6_is1"
    )
    foreach ($reg in $regPaths) {
        $installLoc = (Get-ItemProperty $reg -ErrorAction SilentlyContinue).InstallLocation
        if ($installLoc) {
            $candidate = Join-Path $installLoc "ISCC.exe"
            if (Test-Path $candidate) {
                $ISCC = $candidate
                break
            }
        }
    }
}

if (-not $ISCC) {
    Write-Fail "未找到 Inno Setup 6"
    Write-Warn "请安装 Inno Setup: https://jrsoftware.org/isdl.php"
    Write-Warn "安装完成后重新运行此脚本即可"
    Write-Warn ""
    Write-Warn "也可以手动编译: 用 Inno Setup 打开 installer\AkiDesk.iss → Build"
    exit 1
}

Write-OK "Inno Setup: $ISCC"

# 编译安装包
$issFile = "$InstallerDir\AkiDesk.iss"
& $ISCC $issFile
if ($LASTEXITCODE -ne 0) {
    Write-Fail "安装包编译失败"
    exit 1
}

# ============================================================
# 完成
# ============================================================
Write-Step "打包完成!"
$outputFile = "$InstallerDir\output\AkiDesk_Setup.exe"
if (Test-Path $outputFile) {
    $size = [math]::Round((Get-Item $outputFile).Length / 1MB, 1)
    Write-OK "安装包: $outputFile ($size MB)"
}

Write-Host ""

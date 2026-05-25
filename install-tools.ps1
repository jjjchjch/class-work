$ErrorActionPreference = "Stop"
$toolsDir = "C:\arm-tools"
New-Item -ItemType Directory -Force -Path $toolsDir | Out-Null

function Download {
    param($Name, $Url, $Dest)
    Write-Host "`n[$Name] 正在下载: $Url"
    try {
        Start-BitsTransfer -Source $Url -Destination $Dest -DisplayName $Name
    } catch {
        Write-Host "BitsTransfer 失败，改用 WebRequest..."
        $wc = New-Object System.Net.WebClient
        $wc.DownloadFile($Url, $Dest)
    }
    Write-Host "[$Name] 下载完成."
}

# ── 1. Ninja ──────────────────────────────────────────────────────────────────
$ninjaDir = "$toolsDir\ninja"
if (-not (Test-Path "$ninjaDir\ninja.exe")) {
    New-Item -ItemType Directory -Force -Path $ninjaDir | Out-Null
    Download "Ninja" "https://github.com/ninja-build/ninja/releases/download/v1.12.1/ninja-win.zip" "$toolsDir\ninja.zip"
    Expand-Archive -Path "$toolsDir\ninja.zip" -DestinationPath $ninjaDir -Force
    Remove-Item "$toolsDir\ninja.zip"
    Write-Host "[Ninja] 解压完成."
} else { Write-Host "[Ninja] 已存在，跳过." }

# ── 2. CMake ──────────────────────────────────────────────────────────────────
$cmakeVer = "3.31.6"
$cmakeDir = "$toolsDir\cmake-$cmakeVer-windows-x86_64"
if (-not (Test-Path "$cmakeDir\bin\cmake.exe")) {
    Download "CMake" "https://github.com/Kitware/CMake/releases/download/v$cmakeVer/cmake-$cmakeVer-windows-x86_64.zip" "$toolsDir\cmake.zip"
    Expand-Archive -Path "$toolsDir\cmake.zip" -DestinationPath $toolsDir -Force
    Remove-Item "$toolsDir\cmake.zip"
    Write-Host "[CMake] 解压完成."
} else { Write-Host "[CMake] 已存在，跳过." }

# ── 3. ARM GCC Toolchain ──────────────────────────────────────────────────────
$armVer  = "13.3.1-1.1"
$armName = "xpack-arm-none-eabi-gcc-$armVer"
$armDir  = "$toolsDir\$armName"
if (-not (Test-Path "$armDir\bin\arm-none-eabi-gcc.exe")) {
    Download "ARM-GCC" "https://github.com/xpack-dev-tools/arm-none-eabi-gcc-xpack/releases/download/v$armVer/$armName-win32-x64.zip" "$toolsDir\arm-gcc.zip"
    Write-Host "[ARM-GCC] 正在解压（约 300 MB，请稍候）..."
    Expand-Archive -Path "$toolsDir\arm-gcc.zip" -DestinationPath $toolsDir -Force
    Remove-Item "$toolsDir\arm-gcc.zip"
    Write-Host "[ARM-GCC] 解压完成."
} else { Write-Host "[ARM-GCC] 已存在，跳过." }

# ── 4. 写入用户 PATH ──────────────────────────────────────────────────────────
$addPaths = @(
    "$cmakeDir\bin",
    $ninjaDir,
    "$armDir\bin"
)
$userPath = [Environment]::GetEnvironmentVariable("Path", "User")
foreach ($p in $addPaths) {
    if ($userPath -notlike "*$p*") {
        $userPath = "$userPath;$p"
        Write-Host "已添加到 PATH: $p"
    } else {
        Write-Host "PATH 中已存在: $p"
    }
}
[Environment]::SetEnvironmentVariable("Path", $userPath, "User")

# ── 5. 验证 ───────────────────────────────────────────────────────────────────
Write-Host "`n===== 安装验证 ====="
$env:PATH = [System.Environment]::GetEnvironmentVariable("Path","User") + ";" + [System.Environment]::GetEnvironmentVariable("Path","Machine")
& "$cmakeDir\bin\cmake.exe" --version
& "$ninjaDir\ninja.exe" --version
& "$armDir\bin\arm-none-eabi-gcc.exe" --version

Write-Host "`n===== 完成！请重启 VS Code 终端以使 PATH 生效 ====="

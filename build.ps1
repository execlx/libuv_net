# 设置错误时停止执行
$ErrorActionPreference = "Stop"

# 检查 vcpkg 安装
$vcpkgRoot = "C:\Users\minxi\Documents\vcpkg"
$vcpkgExe = Join-Path $vcpkgRoot "vcpkg.exe"
$vcpkgToolchain = Join-Path $vcpkgRoot "scripts\buildsystems\vcpkg.cmake"

# 验证 vcpkg 安装
if (-not (Test-Path $vcpkgExe)) {
    Write-Host "vcpkg 未安装，正在安装..."
    
    # 克隆 vcpkg
    git clone https://github.com/Microsoft/vcpkg.git $vcpkgRoot
    if ($LASTEXITCODE -ne 0) {
        throw "克隆 vcpkg 失败"
    }
    
    # 运行引导脚本
    Push-Location $vcpkgRoot
    .\bootstrap-vcpkg.bat
    if ($LASTEXITCODE -ne 0) {
        throw "vcpkg 引导失败"
    }
    Pop-Location
}

# 安装依赖
Write-Host "正在安装依赖..."
& $vcpkgExe install fmt:x64-windows
& $vcpkgExe install spdlog:x64-windows
& $vcpkgExe install libuv:x64-windows

# 集成到 CMake
Write-Host "正在集成到 CMake..."
& $vcpkgExe integrate install

# 验证工具链文件
if (-not (Test-Path $vcpkgToolchain)) {
    throw "找不到 vcpkg 工具链文件: $vcpkgToolchain"
}

# 清理旧的构建目录
if (Test-Path "build") {
    Write-Host "正在清理旧的构建目录..."
    Remove-Item -Path "build" -Recurse -Force
}

# 创建新的构建目录
Write-Host "正在创建构建目录..."
New-Item -Path "build" -ItemType Directory -Force
Set-Location "build"

# 配置项目
Write-Host "正在配置项目..."
$cmakeArgs = @(
    "..",
    "-DCMAKE_TOOLCHAIN_FILE=$vcpkgToolchain",
    "-DCMAKE_BUILD_TYPE=Release"
)
cmake @cmakeArgs
if ($LASTEXITCODE -ne 0) {
    throw "CMake 配置失败"
}

# 构建项目
Write-Host "正在构建项目..."
cmake --build . --config Release
if ($LASTEXITCODE -ne 0) {
    throw "构建失败"
}

Write-Host "构建完成！" 
<#
build_engine.ps1

用途：在 Windows PowerShell 中配置、构建 FileRecover engine 并（可选）运行单元测试。

设计考虑：
- 避免一次性粘贴超长命令导致 PSReadLine 渲染异常（推荐逐行或执行此脚本）
- 可选地绕过 CMake 的 TLS 验证（仅用于受控/临时环境以解决 CI/网络受限时的 FetchContent 下载问题）

用法示例：
# 在项目根目录下运行（或从任意位置调用脚本的绝对路径）
# 1) 构建并运行测试（默认）
#   .\scripts\build_engine.ps1
# 2) 只构建，不运行测试
#   .\scripts\build_engine.ps1 -BuildTests:$false
# 3) 若网络环境导致 CMake 下载依赖失败，可临时绕过 TLS 校验（不推荐长期使用）
#   .\scripts\build_engine.ps1 -SkipTlsVerify:$true

参数说明：
 -BuildTests (bool)  : 是否构建并运行单元测试（默认 $true）
 -SkipTlsVerify (bool): 是否在本次会话中设置 CMAKE_TLS_VERIFY=0 来跳过 TLS 证书校验（临时、风险自负）

注意：建议在 CI 环境中使用系统已安装的 googletest 或把 googletest 作为子模块/仓库依赖以避免网络下载不稳定。
#>

[CmdletBinding()]
param(
    [switch]$BuildTests = $true,
    [switch]$SkipTlsVerify = $false
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

# 可选：临时绕过 CMake TLS 验证（仅本次 PowerShell 会话有效）
if ($SkipTlsVerify) {
    Write-Host "Warning: Disabling CMake TLS verification for this session (CMAKE_TLS_VERIFY=0). Do not use this in untrusted networks." -ForegroundColor Yellow
    $env:CMAKE_TLS_VERIFY = '0'
}

# 计算路径：脚本假定仓库根是脚本所在路径的上级目录
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
$repoRoot = Resolve-Path (Join-Path $scriptDir '..')
$engineDir = Join-Path $repoRoot 'src\engine'
$buildDir = Join-Path $engineDir 'build'

Write-Host "Repository root: $repoRoot"
Write-Host "Engine dir: $engineDir"
Write-Host "Build dir: $buildDir"

# 确保 build 目录存在
if (-Not (Test-Path $buildDir)) { New-Item -ItemType Directory -Path $buildDir | Out-Null }

Write-Host "Configuring CMake..."
# PowerShell 不支持 C 风格的三元运算符，使用 if/else 表达式来计算选项
$buildTestsFlag = if ($BuildTests) { 'ON' } else { 'OFF' }
cmake -S $engineDir -B $buildDir -DBUILD_ENGINE_TESTS=$buildTestsFlag

Write-Host "Building..."
cmake --build $buildDir --config Release

if ($BuildTests) {
    Write-Host "Running tests..."
    # 使用 ctest 运行测试并在失败时输出详细信息
    # 若使用 MinGW 动态运行时，需要确保运行时 DLL 在 PATH 中可见。
    # 尝试自动定位 MinGW 的 bin 目录并临时添加到 PATH。此操作不会修改全局环境变量，仅对本次脚本有效。
    $mingwBin = $null
    try {
        $gpp = Get-Command g++ -ErrorAction SilentlyContinue
        if ($gpp) { $mingwBin = Split-Path $gpp.Path }
    } catch { }
    if (-Not $mingwBin) {
        $candidates = @(
            'D:\\green\\mingw64\\bin',
            'C:\\msys64\\mingw64\\bin',
            'C:\\mingw-w64\\mingw64\\bin',
            'C:\\Program Files\\mingw-w64\\mingw64\\bin'
        )
        foreach ($p in $candidates) { if (Test-Path $p) { $mingwBin = $p; break } }
    }

    $origPath = $env:PATH
    if ($mingwBin) {
        Write-Host "Prepending MinGW bin to PATH for tests: $mingwBin"
        $env:PATH = "$mingwBin;$env:PATH"
    } else {
        Write-Host "MinGW bin not auto-detected; ensure MinGW runtime DLLs are available in PATH or next to test executables." -ForegroundColor Yellow
    }

    ctest -C Release --test-dir $buildDir --output-on-failure

    # 恢复 PATH
    $env:PATH = $origPath
}

Write-Host "Done." -ForegroundColor Green

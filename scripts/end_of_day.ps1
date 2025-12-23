<#
end_of_day.ps1

用途：在每天结束工作时提醒保存当天 chat 日志并（可选）将工程提交并推送到 GitHub。

行为（交互式）:
- 提示并打开 `docs/work_sessions/YYYY-MM-DD-chat-log.md`（如存在或新建）。
- 显示 git 状态并提示是否执行 `git add/commit/push`。

注意：此脚本不会在未经确认的情况下自动推送任何代码；如果你在受管环境中，请先确认网络/凭证可用。
#>

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Get-RepoRoot {
    $scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
    return Resolve-Path (Join-Path $scriptDir '..')
}

$repoRoot = (Get-RepoRoot).Path
$logDir = Join-Path $repoRoot 'docs\work_sessions'
if (-not (Test-Path $logDir)) { New-Item -ItemType Directory -Path $logDir | Out-Null }

$today = Get-Date -Format 'yyyy-MM-dd'
$logFile = Join-Path $logDir ($today + "-chat-log.md")

Write-Host "End-of-day reminder: Chat log file = $logFile" -ForegroundColor Cyan

if (-not (Test-Path $logFile)) {
    Write-Host "Creating new chat log file for today: $logFile"
    @("# Chat log ($today)", "", "- Notes:") | Out-File -FilePath $logFile -Encoding UTF8
}

# Offer to open in VS Code if available
function Open-InEditor($path) {
    $codeCmd = Get-Command code -ErrorAction SilentlyContinue
    if ($codeCmd) {
        & code --goto $path
    } else {
        Start-Process notepad.exe -ArgumentList $path
    }
}

$open = Read-Host "Open today's chat log now in editor? (Y/N)"
if ($open -match '^[Yy]') { Open-InEditor $logFile }

Push-Location $repoRoot
try {
    # Show git status summary
    if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
        Write-Host "git not found in PATH. Please ensure git is installed." -ForegroundColor Red
        return
    }

    Write-Host "Current git status:" -ForegroundColor Yellow
    git status --porcelain

    $ans = Read-Host "Do you want to add and commit local changes and push to origin (interactive)? (Y/N)"
    if ($ans -match '^[Yy]') {
        $commitMsg = Read-Host "Commit message (leave empty for default)"
        if (-not $commitMsg) { $commitMsg = "End of day: save chat log and changes ($today)" }

        git add -A
        $st = git status --porcelain
        if (-not $st) {
            Write-Host "No changes to commit." -ForegroundColor Green
        } else {
            git commit -m "$commitMsg"
            $pushAns = Read-Host "Push to remote now? (Y/N)"
            if ($pushAns -match '^[Yy]') {
                git push
                Write-Host "Pushed to remote." -ForegroundColor Green
            } else {
                Write-Host "Skipping push. Remember to push before ending your session." -ForegroundColor Yellow
            }
        }
    } else {
        Write-Host "Skipping commit/push as requested." -ForegroundColor Yellow
    }
} finally {
    Pop-Location
}

Write-Host "End-of-day tasks complete. Don't forget to back up any sensitive info out of logs." -ForegroundColor Cyan

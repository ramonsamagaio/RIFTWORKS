$ErrorActionPreference = 'Stop'

$ProjectDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectFile = Join-Path $ProjectDir 'RiftworksUE.uproject'
$LogDir = Join-Path $ProjectDir 'Saved\Logs'
$LogFile = Join-Path $LogDir 'RiftworksUE_ManualBuild.log'
New-Item -ItemType Directory -Force -Path $LogDir | Out-Null

function Find-Unreal58 {
    $Candidates = @()
    $LauncherManifest = Join-Path $env:ProgramData 'Epic\UnrealEngineLauncher\LauncherInstalled.dat'
    if (Test-Path $LauncherManifest) {
        try {
            $Data = Get-Content $LauncherManifest -Raw | ConvertFrom-Json
            foreach ($Entry in $Data.InstallationList) {
                if ($Entry.AppName -eq 'UE_5.8' -or $Entry.AppVersion -like '5.8*') {
                    $Candidates += $Entry.InstallLocation
                }
            }
        } catch {}
    }

    $Candidates += @(
        (Join-Path $env:ProgramFiles 'Epic Games\UE_5.8'),
        (Join-Path ${env:ProgramFiles(x86)} 'Epic Games\UE_5.8')
    )

    foreach ($Candidate in $Candidates | Select-Object -Unique) {
        if (-not [string]::IsNullOrWhiteSpace($Candidate)) {
            $BuildBat = Join-Path $Candidate 'Engine\Build\BatchFiles\Build.bat'
            if (Test-Path $BuildBat) { return $Candidate }
        }
    }
    return $null
}

$EngineDir = Find-Unreal58
if (-not $EngineDir) {
    $Message = @'
RIFTWORKS could not find Unreal Engine 5.8.
Open this script and set $EngineDir manually to your UE 5.8 install folder,
or verify UE 5.8 is registered in Epic Games Launcher.
'@
    $Message | Tee-Object -FilePath $LogFile
    Read-Host 'Press Enter to close'
    exit 2
}

$BuildBat = Join-Path $EngineDir 'Engine\Build\BatchFiles\Build.bat'
$Intermediate = Join-Path $ProjectDir 'Intermediate'
$Binaries = Join-Path $ProjectDir 'Binaries'

Write-Host "RIFTWORKS Unreal 5.8 clean rebuild" -ForegroundColor Cyan
Write-Host "Engine:  $EngineDir"
Write-Host "Project: $ProjectFile"
Write-Host "Log:     $LogFile"

if (Test-Path $Intermediate) { Remove-Item $Intermediate -Recurse -Force }
if (Test-Path $Binaries) { Remove-Item $Binaries -Recurse -Force }

$Args = @(
    'RiftworksUEEditor',
    'Win64',
    'Development',
    $ProjectFile,
    '-WaitMutex',
    '-NoHotReloadFromIDE'
)

& $BuildBat @Args 2>&1 | Tee-Object -FilePath $LogFile
$ExitCode = $LASTEXITCODE

Write-Host ''
if ($ExitCode -eq 0) {
    Write-Host 'BUILD SUCCEEDED. You can open RiftworksUE.uproject now.' -ForegroundColor Green
} else {
    Write-Host "BUILD FAILED (exit $ExitCode)." -ForegroundColor Red
    Write-Host "Send the LAST ~80 lines of this file: $LogFile" -ForegroundColor Yellow
    Write-Host ''
    Write-Host 'If the log says Visual Studio / MSVC / Windows SDK is missing, install Visual Studio 2022 -> Game development with C++ plus a Windows 11/10 SDK.' -ForegroundColor Yellow
}

Read-Host 'Press Enter to close'
exit $ExitCode

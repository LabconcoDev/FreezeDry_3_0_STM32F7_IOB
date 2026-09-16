param(
    [string]$Device = "STM32F723ZC",
    [string]$Interface = "SWD",
    [int]$Speed = 4000,
    [int]$Port = 2331,
    [int]$SwoPort = 2332,
    [int]$TelnetPort = 2333,
    [ValidateSet(0,1)]
    [int]$LocalhostOnly = 0,
    [string]$JLinkRoot = "C:\Program Files\SEGGER"
)

$ErrorActionPreference = "Stop"

function Resolve-JLinkExe {
    param([string]$Root)

    $direct = "C:\Program Files\SEGGER\JLink_V948\JLinkGDBServerCL.exe"
    if (Test-Path $direct) {
        return $direct
    }

    $candidates = Get-ChildItem -Path $Root -Directory -Filter "JLink*" -ErrorAction SilentlyContinue |
        Sort-Object Name -Descending

    foreach ($dir in $candidates) {
        $exe = Join-Path $dir.FullName "JLinkGDBServerCL.exe"
        if (Test-Path $exe) {
            return $exe
        }
    }

    throw "JLinkGDBServerCL.exe not found under $Root"
}

$exePath = Resolve-JLinkExe -Root $JLinkRoot
$workDir = Split-Path $exePath -Parent

Write-Host "Using J-Link server:" $exePath
Write-Host "Starting with device=$Device, if=$Interface, speed=${Speed}kHz, port=$Port"
Write-Host "localhostonly=$LocalhostOnly (set 0 for Dev Container access)"

Push-Location $workDir
try {
    & $exePath `
        -device $Device `
        -if $Interface `
        -speed $Speed `
        -port $Port `
        -swoport $SwoPort `
        -telnetport $TelnetPort `
        -localhostonly $LocalhostOnly
}
finally {
    Pop-Location
}

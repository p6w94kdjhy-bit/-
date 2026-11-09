# Jellibi AGV2 laptop setup check (SETUP_GUIDE steps 1-7). ASCII only: works in Windows PowerShell 5.1.
# Run: right-click -> "Run with PowerShell"
#   or:  powershell -ExecutionPolicy Bypass -File check_setup.ps1 [-Repo D:\path\-]
# Connect the robot by USB and power it ON first. Copy the whole output to Claude.
param([string]$Repo = "C:\robot\-")

$ok = 0; $bad = 0
function Pass($m) { Write-Host "[OK]   $m" -ForegroundColor Green; $script:ok++ }
function Fail($m) { Write-Host "[X]    $m" -ForegroundColor Red; $script:bad++ }
function Info($m) { Write-Host "       $m" }

Write-Host "=== Jellibi AGV2 setup check ===  $(Get-Date -Format 'yyyy-MM-dd HH:mm')"
Write-Host "Windows: $([Environment]::OSVersion.VersionString)"

# 1. Arduino IDE
Write-Host "`n[1] Arduino IDE"
$ide = @("$env:LOCALAPPDATA\Programs\Arduino IDE\Arduino IDE.exe",
         "$env:ProgramFiles\Arduino IDE\Arduino IDE.exe",
         "${env:ProgramFiles(x86)}\Arduino\arduino.exe",
         "$env:ProgramFiles\Arduino\arduino.exe") | Where-Object { Test-Path $_ } | Select-Object -First 1
if ($ide) {
    $v = (Get-Item $ide).VersionInfo.ProductVersion
    Pass "Arduino IDE: $ide (version $v)"
    if ($ide -like "*arduino.exe") { Info "Old IDE 1.x. IDE 2.x recommended (guide step 1)" }
} else { Fail "Arduino IDE not found -> guide step 1" }

# 2. Robot USB port
Write-Host "`n[2] Robot USB port (CH340/CH341)"
$dev = Get-CimInstance Win32_PnPEntity -ErrorAction SilentlyContinue | Where-Object { $_.Name -match "CH34" }
if ($dev) {
    foreach ($d in $dev) {
        if ($d.Status -eq "OK") { Pass "$($d.Name)" } else { Fail "$($d.Name) status=$($d.Status) -> reinstall CH341 driver" }
    }
} else {
    Fail "No CH340/CH341 device -> check USB cable + robot power ON, install driver (guide step 2)"
    $ports = Get-CimInstance Win32_PnPEntity -ErrorAction SilentlyContinue | Where-Object { $_.Name -match "\(COM\d+\)" }
    foreach ($p in $ports) { Info "visible port: $($p.Name)" }
}

# 3. GitHub Desktop
Write-Host "`n[3] GitHub Desktop"
if (Test-Path "$env:LOCALAPPDATA\GitHubDesktop\GitHubDesktop.exe") { Pass "GitHub Desktop installed" }
else { Fail "GitHub Desktop not found -> guide step 3" }

# 4. Repository and branch
Write-Host "`n[4] Code (repository $Repo)"
$want = "claude/jellibi-agv2-contest-rnqx70"
$sk = Join-Path $Repo "jellibi_agv2"
$headFile = Join-Path $Repo ".git\HEAD"
if (Test-Path $headFile) {
    $head = (Get-Content $headFile -Raw).Trim()
    if ($head -eq "ref: refs/heads/$want") { Pass "branch $want" }
    else { Fail "current: $head -> set Current branch to $want" }
    $refFile = Join-Path $Repo ".git\refs\heads\$want"
    if (Test-Path $refFile) { Info "commit: $((Get-Content $refFile -Raw).Trim().Substring(0,7))" }
} else { Fail "No repository at $Repo -> guide step 4 (other location: use -Repo)" }
foreach ($s in @("course1_drive", "course2_pallet", "calib_ir", "tune_drive")) {
    if (Test-Path (Join-Path $sk "$s\$s.ino")) { Pass "sketch $s" } else { Fail "sketch $s missing" }
}
$cfg = Join-Path $sk "libraries\AGV2\src\config.h"
if (Test-Path $cfg) {
    Pass "config.h: $cfg"
    $irLines = Select-String -Path $cfg -Pattern '^#define (IR_[LR]_(WHITE|BLACK)|FRONT_OBSTACLE_TH)\s+(-?\d+)'
    foreach ($l in $irLines) { Info ($l.Line -replace '\s+//.*$', '') }
} else { Fail "config.h missing" }

# 5. Arduino IDE settings
Write-Host "`n[5] Arduino IDE settings"
$userDir = $null
$cliYaml = "$env:USERPROFILE\.arduinoIDE\arduino-cli.yaml"          # IDE 2.x
if (Test-Path $cliYaml) {
    $m = Select-String -Path $cliYaml -Pattern '^\s*user:\s*(.+)$' | Select-Object -First 1
    if ($m) { $userDir = $m.Matches[0].Groups[1].Value.Trim().Trim('"').Trim("'") }
}
if (-not $userDir) {
    $pref = "$env:LOCALAPPDATA\Arduino15\preferences.txt"              # IDE 1.x
    if (Test-Path $pref) {
        $m = Select-String -Path $pref -Pattern '^sketchbook\.path=(.+)$' | Select-Object -First 1
        if ($m) { $userDir = $m.Matches[0].Groups[1].Value.Trim() }
    }
}
if ($userDir) {
    if ($userDir.TrimEnd('\', '/') -ieq $sk.TrimEnd('\', '/')) { Pass "Sketchbook location = $userDir" }
    else { Fail "Sketchbook location = $userDir -> change to $sk and restart IDE (guide 5-1)" }
} else { Fail "Sketchbook setting not found (IDE never started?) -> guide 5-1" }

$avr = "$env:LOCALAPPDATA\Arduino15\packages\arduino\hardware\avr"
if (Test-Path $avr) {
    $vers = (Get-ChildItem $avr -Directory | ForEach-Object { $_.Name }) -join ", "
    Pass "Arduino AVR Boards installed ($vers)"
} else { Fail "Arduino AVR Boards missing -> Boards Manager (guide 5-2)" }

$servo = @("$env:LOCALAPPDATA\Arduino15\libraries\Servo",
           "$sk\libraries\Servo",
           "$env:USERPROFILE\Documents\Arduino\libraries\Servo")
if ($ide) {
    $servo += (Join-Path (Split-Path $ide) "resources\app\lib\backend\resources\libraries\Servo")
    $servo += (Join-Path (Split-Path $ide) "libraries\Servo")
}
$sv = $servo | Where-Object { Test-Path $_ } | Select-Object -First 1
if ($sv) { Pass "Servo library: $sv" } else { Fail "Servo library not found -> Library Manager (guide 5-2)" }

# 6-7. Cannot be checked automatically
Write-Host "`n[6-7] Check by hand"
Info "- IDE: Board Arduino Nano / Processor ATmega328P (or Old Bootloader) / Port"
Info "- Verify all 4 sketches; upload calib_ir; Serial Monitor 115200 shows L= R= F="
Info "- Claude session: https://claude.ai/code/session_016MkBxvgMtn2TAvaizhZMSu"

Write-Host "`n=== RESULT: OK $ok / PROBLEMS $bad ==="
if ($Host.Name -eq "ConsoleHost") { Read-Host "Press Enter to close" | Out-Null }

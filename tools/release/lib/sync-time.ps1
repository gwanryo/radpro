# Rad Pro: set the clock and time zone of a Bosean FS-5000 from this PC over USB.
# Windows PowerShell 5.1 compatible. Never fails the caller: exits 0 always.
# Flashing uses only the ST-LINK, so the device's USB-C port may not be connected yet.
# 1) Probe the ports already present for about $probeSeconds (whole rounds; if the device
#    answers here, sync and warn: ST-LINK and USB-C must not power the board together).
# 2) Otherwise ask the user to disconnect the ST-LINK and connect the USB-C cable, and probe
#    all ports (USB serial ports first) every second for up to -WaitSeconds (Enter skips).
# Each port probe has its own time limit; a new probe only starts before the deadline.

param([int]$WaitSeconds = 60)

$probeSeconds = 3      # first rounds over the ports already present
$idWaitMs = 800        # GET deviceId reply time per port (the device answers in ~40 ms)
$setSeconds = 10       # time allowed for the SET requests once the device answered
if ($WaitSeconds -lt 0) { $WaitSeconds = 0 }

$deadline = (Get-Date).AddSeconds($probeSeconds)

function Remaining-Ms { return [int][Math]::Max(0, ($script:deadline - (Get-Date)).TotalMilliseconds) }

function Send-Request($port, $request, $waitMs) {
    if ($waitMs -le 0) { return $null }
    $port.DiscardInBuffer()
    $port.Write($request + "`r`n")
    $end = (Get-Date).AddMilliseconds($waitMs)
    while ((Get-Date) -lt $end) {
        $port.ReadTimeout = [int][Math]::Max(50, [Math]::Min(300, ($end - (Get-Date)).TotalMilliseconds))
        try {
            $line = $port.ReadLine().Trim()
        } catch [System.TimeoutException] {
            continue
        }
        if ($line.StartsWith("OK") -or $line.StartsWith("ERROR")) { return $line }
    }
    return $null
}

# Returns $true if the device answered on $name (even if it did not accept the settings).
function Try-Port($name) {
    $port = New-Object System.IO.Ports.SerialPort $name, 115200, ([System.IO.Ports.Parity]::None), 8, ([System.IO.Ports.StopBits]::One)
    $port.NewLine = "`r`n"
    $port.ReadTimeout = 300
    $port.WriteTimeout = 1000
    try {
        $port.Open()
        $id = Send-Request $port "GET deviceId" $idWaitMs
        if (-not $id -or $id -notmatch "Bosean FS-5000") { return $false }
        Write-Host "Found $id on $name"

        $setDeadline = (Get-Date).AddSeconds($setSeconds)
        $epoch = New-Object DateTime 1970, 1, 1, 0, 0, 0, ([DateTimeKind]::Utc)
        $r1 = $null; $r2 = $null
        # The device may still be booting and answer ERROR: retry until $setDeadline.
        while ($true) {
            $seconds = [int64][Math]::Floor(([DateTime]::UtcNow - $epoch).TotalSeconds)
            $offset = [Math]::Truncate([TimeZoneInfo]::Local.GetUtcOffset([DateTime]::UtcNow).TotalHours * 10) / 10
            $offsetText = $offset.ToString("0.#", [System.Globalization.CultureInfo]::InvariantCulture)
            $r1 = Send-Request $port ("SET deviceTime " + $seconds) 1500
            $r2 = Send-Request $port ("SET deviceTimeZone " + $offsetText) 1500
            if ($r1 -eq "OK" -and $r2 -eq "OK") {
                Write-Host "Device clock set (UTC $seconds, time zone UTC$(if ($offset -ge 0) {'+'})$offsetText)."
                return $true
            }
            if (($setDeadline - (Get-Date)).TotalMilliseconds -le 1000) { break }
            Start-Sleep -Milliseconds 1000
        }
        Write-Host "Device did not accept the clock settings (time: $r1, time zone: $r2)."
        return $true
    } catch {
        return $false
    } finally {
        if ($port.IsOpen) { $port.Close() }
        $port.Dispose()
    }
}

# Keyboard check only on a real console: [Console]::KeyAvailable throws when input is redirected.
$interactive = $false
try { $interactive = -not [Console]::IsInputRedirected } catch { }

function Enter-Pressed {
    if (-not $script:interactive) { return $false }
    try {
        while ([Console]::KeyAvailable) {
            if ([Console]::ReadKey($true).Key -eq [ConsoleKey]::Enter) { return $true }
        }
    } catch {
        $script:interactive = $false
    }
    return $false
}

# Wait up to $ms milliseconds (bounded by the deadline); returns $true if Enter was pressed.
function Wait-Or-Enter($ms) {
    $end = (Get-Date).AddMilliseconds([Math]::Min($ms, (Remaining-Ms)))
    while ((Get-Date) -lt $end) {
        if (Enter-Pressed) { return $true }
        Start-Sleep -Milliseconds 100
    }
    return (Enter-Pressed)
}

# COM ports, USB serial adapters first (PNPDeviceID USB\... or FTDIBUS\...).
$script:usbKey = ""
$script:usbPorts = @()
function Get-Ports {
    $names = @()
    try { $names = @([System.IO.Ports.SerialPort]::GetPortNames() | Sort-Object -Unique) } catch { return @() }
    $key = $names -join ","
    if ($key -ne $script:usbKey) {
        # Look up the USB ports only when the port list changes.
        $script:usbKey = $key
        $script:usbPorts = @()
        try {
            foreach ($dev in @(Get-CimInstance -ClassName Win32_PnPEntity -Filter "Name LIKE '%(COM%'" -ErrorAction Stop)) {
                if ($dev.Name -match '\((COM\d+)\)' -and $dev.PNPDeviceID -match '^(USB|FTDIBUS)\\') { $script:usbPorts += $Matches[1] }
            }
        } catch { }
    }
    $usb = @($names | Where-Object { $script:usbPorts -contains $_ })
    $other = @($names | Where-Object { $script:usbPorts -notcontains $_ })
    return @($usb + $other)
}

# One round over all current ports. Returns "found", "skip" (Enter) or "".
# $full: probe every port regardless of the deadline (the very first round); otherwise a
# probe only starts before the deadline. $keys: check Enter between probes.
function Probe-Round($full, $keys) {
    foreach ($name in (Get-Ports)) {
        if (-not $full -and (Remaining-Ms) -le 0) { break }
        if ($keys -and (Enter-Pressed)) { return "skip" }
        if (Try-Port $name) { return "found" }
    }
    return ""
}

# ---- main ----
$result = ""
try {
    # 1) Ports already present: one full round, then more rounds until $probeSeconds.
    $first = $true
    do {
        $result = Probe-Round $first $false
        $first = $false
        if ($result -eq "" -and (Remaining-Ms) -gt 0) { Start-Sleep -Milliseconds ([Math]::Min(1000, (Remaining-Ms))) }
    } while ($result -eq "" -and (Remaining-Ms) -gt 0)
    if ($result -eq "found") {
        # The install guide powers the board from the ST-LINK 3.3 V pin; USB-C powers it too.
        Write-Host "Warning: do not connect the USB-C cable while the ST-LINK is attached to the device."
        Write-Host "Next time, disconnect the ST-LINK first and connect USB-C only when asked."
    }

    # 2) Ask for the USB-C cable and probe all ports every second.
    if ($result -eq "" -and $WaitSeconds -gt 0) {
        # Discard keys typed earlier (e.g. at flashtool's "Press any key" prompt).
        if ($interactive) { try { while ([Console]::KeyAvailable) { [void][Console]::ReadKey($true) } } catch { $interactive = $false } }
        Write-Host ""
        Write-Host "Disconnect the ST-LINK from the device, then connect the device's USB-C port"
        Write-Host "to this computer with a data cable."
        if ($interactive) {
            Write-Host "Waiting up to $WaitSeconds seconds for the device. Press Enter to skip."
        } else {
            Write-Host "Waiting up to $WaitSeconds seconds for the device. Press Ctrl+C to skip."
        }
        $script:deadline = (Get-Date).AddSeconds($WaitSeconds)
        while ($result -eq "" -and (Remaining-Ms) -gt 0) {
            $result = Probe-Round $false $true
            if ($result -eq "" -and (Wait-Or-Enter 1000)) { $result = "skip" }
        }
    }
} catch {
}
if ($result -ne "found") {
    if ($result -eq "skip") {
        Write-Host "Clock sync skipped."
    } else {
        Write-Host "Could not find a Bosean FS-5000 on a USB serial port; clock not synchronized."
    }
    Write-Host "Connect the device with USB and set the clock later (any radpro-tool.py command syncs it) or in the settings."
}
exit 0

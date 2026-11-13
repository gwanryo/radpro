# Rad Pro: set the clock and time zone of a Bosean FS-5000 from this PC over USB.
# Windows PowerShell 5.1 compatible. Never fails the caller: exits 0 always.
# Flashing uses only the ST-Link, so the device's USB-C port may not be connected yet:
# probe the existing ports briefly (warn if the device is already there: ST-LINK and USB-C
# must not power the board at the same time), then ask the user to disconnect the ST-LINK
# and connect the USB-C cable, and wait up to -WaitSeconds for a new port (Enter skips).
# Every phase is bounded by $deadline,
# which is checked before every port probe and read.

param([int]$WaitSeconds = 60)

$probeSeconds = 3    # initial probe of the ports already present
$setSeconds = 10     # time allowed for the SET requests once the device answered
if ($WaitSeconds -lt 0) { $WaitSeconds = 0 }

$deadline = (Get-Date).AddSeconds($probeSeconds)

function Remaining-Ms { return [int][Math]::Max(0, ($script:deadline - (Get-Date)).TotalMilliseconds) }

function Send-Request($port, $request, $waitMs) {
    $waitMs = [Math]::Min($waitMs, (Remaining-Ms))
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

function Try-Port($name) {
    if ((Remaining-Ms) -le 0) { return $false }
    $port = New-Object System.IO.Ports.SerialPort $name, 115200, ([System.IO.Ports.Parity]::None), 8, ([System.IO.Ports.StopBits]::One)
    $port.NewLine = "`r`n"
    $port.ReadTimeout = 300
    $port.WriteTimeout = 1000
    try {
        $port.Open()
        $id = Send-Request $port "GET deviceId" 1500
        if (-not $id -or $id -notmatch "Bosean FS-5000") { return $false }
        Write-Host "Found $id on $name"
        # Device found: give the SET requests their own time budget.
        $script:deadline = (Get-Date).AddSeconds($setSeconds)

        $epoch = New-Object DateTime 1970, 1, 1, 0, 0, 0, ([DateTimeKind]::Utc)
        $r1 = $null; $r2 = $null
        # The device may still be booting and answer ERROR: retry until the deadline.
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
            if ((Remaining-Ms) -le 1000) { break }
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
    if (-not $interactive) { return $false }
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

function Get-Ports {
    try { return @([System.IO.Ports.SerialPort]::GetPortNames() | Sort-Object -Unique) } catch { return @() }
}

$done = $false
$skipped = $false
try {
    # 1) Ports already present (the cable may already be connected).
    while (-not $done -and (Remaining-Ms) -gt 0) {
        foreach ($name in (Get-Ports)) {
            if ((Remaining-Ms) -le 0) { break }
            if (Try-Port $name) { $done = $true; break }
        }
        if (-not $done -and (Remaining-Ms) -gt 0) { Start-Sleep -Milliseconds ([Math]::Min(1000, (Remaining-Ms))) }
    }
    if ($done) {
        # The installer guide powers the board from the ST-LINK 3.3 V pin; USB-C powers it too.
        Write-Host "Warning: do not connect the USB-C cable while the ST-LINK is attached to the device."
        Write-Host "Next time, disconnect the ST-LINK first and connect USB-C only when asked."
    }

    # 2) Ask for the USB-C cable and probe ports that appear afterwards.
    if (-not $done -and $WaitSeconds -gt 0) {
        $known = @(Get-Ports)
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
        while (-not $done -and (Remaining-Ms) -gt 0) {
            if (Enter-Pressed) { $skipped = $true; break }
            $current = Get-Ports
            # A port that disappears (device re-enumerating) counts as new when it returns.
            $known = @($known | Where-Object { $current -contains $_ })
            foreach ($name in $current) {
                if ($known -contains $name) { continue }
                if ((Remaining-Ms) -le 0) { break }
                if (Enter-Pressed) { $skipped = $true; break }
                # New ports stay unknown, so a port that is not ready yet is probed again next round.
                if (Try-Port $name) { $done = $true; break }
            }
            if ($done -or $skipped) { break }
            if (Wait-Or-Enter 1000) { $skipped = $true; break }
        }
    }
} catch {
}
if (-not $done) {
    if ($skipped) {
        Write-Host "Clock sync skipped."
    } else {
        Write-Host "Could not find a Bosean FS-5000 on a USB serial port; clock not synchronized."
    }
    Write-Host "Connect the device with USB and set the clock later (any radpro-tool.py command syncs it) or in the settings."
}
exit 0

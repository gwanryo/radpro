# Rad Pro: set the clock and time zone of a Bosean FS-5000 from this PC over USB.
# Windows PowerShell 5.1 compatible. Never fails the caller: exits 0 always.
# Total time is bounded by $maxSeconds (checked before every port probe and read).

$maxSeconds = 12
$deadline = (Get-Date).AddSeconds($maxSeconds)

function Remaining-Ms { return [int][Math]::Max(0, ($deadline - (Get-Date)).TotalMilliseconds) }

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
    $port = New-Object System.IO.Ports.SerialPort $name, 115200, ([System.IO.Ports.Parity]::None), 8, ([System.IO.Ports.StopBits]::One)
    $port.NewLine = "`r`n"
    $port.ReadTimeout = 300
    $port.WriteTimeout = 1000
    try {
        $port.Open()
        $id = Send-Request $port "GET deviceId" 1500
        if (-not $id -or $id -notmatch "Bosean FS-5000") { return $false }
        Write-Host "Found $id on $name"

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

$done = $false
try {
    while (-not $done -and (Remaining-Ms) -gt 0) {
        foreach ($name in [System.IO.Ports.SerialPort]::GetPortNames()) {
            if ((Remaining-Ms) -le 0) { break }
            if (Try-Port $name) { $done = $true; break }
        }
        if (-not $done -and (Remaining-Ms) -gt 1000) { Start-Sleep -Milliseconds 1000 }
    }
} catch {
}
if (-not $done) {
    Write-Host "Could not find a Bosean FS-5000 on a USB serial port; clock not synchronized."
    Write-Host "Connect the device with USB and set the clock later (any radpro-tool.py command syncs it) or in the settings."
}
exit 0

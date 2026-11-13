# Rad Pro: set the clock and time zone of a Bosean FS-5000 from this PC over USB.
# Windows PowerShell 5.1 compatible. Never fails the caller: exits 0 always.

$maxSeconds = 13
$deadline = (Get-Date).AddSeconds($maxSeconds)

function Send-Request($port, $request) {
    $port.DiscardInBuffer()
    $port.Write($request + "`r`n")
    $deadlineLine = (Get-Date).AddSeconds(1.5)
    while ((Get-Date) -lt $deadlineLine) {
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
        $id = Send-Request $port "GET deviceId"
        if (-not $id -or $id -notmatch "Bosean FS-5000") { return $false }
        Write-Host "Found $id on $name"

        $epoch = New-Object DateTime 1970, 1, 1, 0, 0, 0, ([DateTimeKind]::Utc)
        $seconds = [int64][Math]::Floor(([DateTime]::UtcNow - $epoch).TotalSeconds)
        $offset = [Math]::Truncate([TimeZoneInfo]::Local.GetUtcOffset([DateTime]::UtcNow).TotalHours * 10) / 10
        $offsetText = $offset.ToString("0.#", [System.Globalization.CultureInfo]::InvariantCulture)

        $r1 = Send-Request $port ("SET deviceTime " + $seconds)
        $r2 = Send-Request $port ("SET deviceTimeZone " + $offsetText)
        if ($r1 -eq "OK" -and $r2 -eq "OK") {
            Write-Host "Device clock set (UTC $seconds, time zone UTC$(if ($offset -ge 0) {'+'})$offsetText)."
        } else {
            Write-Host "Device did not accept the clock settings (time: $r1, time zone: $r2)."
        }
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
    while (-not $done -and (Get-Date) -lt $deadline) {
        foreach ($name in [System.IO.Ports.SerialPort]::GetPortNames()) {
            if (Try-Port $name) { $done = $true; break }
        }
        if (-not $done) { Start-Sleep -Milliseconds 1000 }
    }
} catch {
}
if (-not $done) {
    Write-Host "Could not find a Bosean FS-5000 on a USB serial port; clock not synchronized."
    Write-Host "Connect the device with USB and set the clock later (any radpro-tool.py command syncs it) or in the settings."
}
exit 0

#!/bin/bash
# Rad Pro: set the clock and time zone of a Bosean FS-5000 from this computer over USB.
# Linux/macOS, best effort. Never fails the caller: always exits 0.
# Flashing uses only the ST-Link, so the device's USB-C port may not be connected yet:
# probe the existing ports briefly (warn if the device is already there: ST-LINK and USB-C
# must not power the board at the same time), then ask the user to disconnect the ST-LINK
# and connect the USB-C cable, and wait up to WAIT_SECONDS for a new port (Enter skips
# when stdin is a terminal).
# Usage: sync-time.sh [WAIT_SECONDS]   (or SYNC_WAIT_SECONDS=N; default 60)
# Every phase is bounded by DEADLINE_EPOCH (checked before every port probe and read).
# Uses only integer read timeouts (macOS /bin/bash 3.2 compatible).

WAIT_SECONDS=${1:-${SYNC_WAIT_SECONDS:-60}}
case "$WAIT_SECONDS" in ''|*[!0-9]*) WAIT_SECONDS=60;; esac
WAIT_SECONDS=$((10#$WAIT_SECONDS))
PROBE_SECONDS=3    # initial probe of the ports already present
SET_SECONDS=10     # time allowed for the SET requests once the device answered
PORTS_GLOB="/dev/ttyUSB* /dev/ttyACM* /dev/cu.usbserial* /dev/cu.wchusbserial* /dev/cu.usbmodem*"
DEADLINE_EPOCH=$(( $(date +%s) + PROBE_SECONDS ))

remaining() {
    local r=$((DEADLINE_EPOCH - $(date +%s)))
    [ $r -lt 0 ] && r=0
    echo $r
}

# Time zone offset in hours with one decimal (e.g. 9, 5.5, -3.5), DST included.
tz_hours() {
    local z sign h m
    z=$(date +%z)
    sign=${z:0:1}; h=${z:1:2}; m=${z:3:2}
    h=$((10#$h)); m=$((10#$m))
    local tenths=$(( (h * 60 + m) * 10 / 60 ))
    local out="$((tenths / 10))"
    if [ $((tenths % 10)) -ne 0 ]; then out="$out.$((tenths % 10))"; fi
    if [ "$sign" = "-" ] && [ "$tenths" -ne 0 ]; then out="-$out"; fi
    echo "$out"
}

try_python() {  # $1 = port; returns 0 if device found (even if set failed)
    python3 - "$1" "$(tz_hours)" "$DEADLINE_EPOCH" "$SET_SECONDS" <<'PY' 2>/dev/null
import sys, time
try:
    import serial
except ImportError:
    sys.exit(2)
port, tz, deadline, set_seconds = sys.argv[1], sys.argv[2], float(sys.argv[3]), float(sys.argv[4])
def req(s, text, wait=1.5):
    wait = min(wait, deadline - time.time())
    if wait <= 0:
        return None
    s.reset_input_buffer()
    s.write((text + "\r\n").encode())
    end = time.time() + wait
    while time.time() < end:
        line = s.readline().decode(errors="replace").strip()
        if line.startswith("OK") or line.startswith("ERROR"):
            return line
    return None
if time.time() >= deadline:
    sys.exit(1)
try:
    s = serial.Serial(port, 115200, timeout=0.3, write_timeout=1)
except Exception:
    sys.exit(1)
with s:
    ident = req(s, "GET deviceId")
    if not ident or "Bosean FS-5000" not in ident:
        sys.exit(1)
    print("Found " + ident + " on " + port)
    deadline = time.time() + set_seconds    # own time budget for the SET requests
    while True:
        now = int(time.time())
        r1 = req(s, "SET deviceTime %d" % now)
        r2 = req(s, "SET deviceTimeZone " + tz)
        if r1 == "OK" and r2 == "OK":
            print("Device clock set (UTC %d, time zone UTC%s%s)." % (now, "" if tz.startswith("-") else "+", tz))
            break
        if deadline - time.time() <= 1:
            print("Device did not accept the clock settings (time: %s, time zone: %s)." % (r1, r2))
            break
        time.sleep(1)
sys.exit(0)
PY
}

if [ "$(uname)" = "Darwin" ]; then STTY_DEV="-f"; else STTY_DEV="-F"; fi

# request TEXT MAXWAIT: send a request on fd 3 and print the first OK/ERROR line
# (empty on timeout). Reads char by char so partial lines survive read timeouts.
request() {
    local wait=$2 rem c buf="" end
    rem=$(remaining)
    [ "$wait" -gt "$rem" ] && wait=$rem
    [ "$wait" -le 0 ] && return
    printf '%s\r\n' "$1" >&3
    end=$(( $(date +%s) + wait ))
    while [ "$(date +%s)" -lt "$end" ]; do
        if IFS= read -r -n 1 -t 1 c <&3; then
            if [ -z "$c" ] || [ "$c" = $'\n' ]; then      # end of line
                buf=${buf%$'\r'}
                case "$buf" in OK*|ERROR*) echo "$buf"; return;; esac
                buf=""
            else
                buf="$buf$c"
            fi
        fi
    done
}

try_stty() {  # $1 = port
    local dev=$1 id r1 r2 tz junk
    [ "$(remaining)" -le 0 ] && return 1
    stty $STTY_DEV "$dev" 115200 cs8 -cstopb -parenb raw -echo clocal cread 2>/dev/null || return 1
    exec 3<>"$dev" 2>/dev/null || return 1
    while [ "$(remaining)" -gt 0 ] && IFS= read -r -n 4096 -t 1 junk <&3; do :; done    # drain stale input until a read times out
    id=$(request "GET deviceId" 2)
    case "$id" in
        *"Bosean FS-5000"*) ;;
        *) exec 3>&-; return 1;;
    esac
    echo "Found $id on $dev"
    DEADLINE_EPOCH=$(( $(date +%s) + SET_SECONDS ))    # own time budget for the SET requests
    tz=$(tz_hours)
    while true; do
        r1=$(request "SET deviceTime $(date -u +%s)" 2)
        r2=$(request "SET deviceTimeZone $tz" 2)
        [ "$r1" = "OK" ] && [ "$r2" = "OK" ] && break
        [ "$(remaining)" -le 1 ] && break
        sleep 1    # device may still be booting: retry until the deadline
    done
    exec 3>&-
    if [ "$r1" = "OK" ] && [ "$r2" = "OK" ]; then
        echo "Device clock set (time zone UTC$tz)."
    else
        echo "Device did not accept the clock settings (time: $r1, time zone: $r2)."
    fi
    return 0
}

HAVE_PYSERIAL=0
if command -v python3 >/dev/null 2>&1 && python3 -c 'import serial' 2>/dev/null; then
    HAVE_PYSERIAL=1
fi

list_ports() {
    local dev
    for dev in $PORTS_GLOB; do [ -e "$dev" ] && echo "$dev"; done
}

probe() {  # $1 = port; returns 0 if the device answered
    if [ $HAVE_PYSERIAL -eq 1 ]; then try_python "$1"; else try_stty "$1"; fi
}

INTERACTIVE=0
[ -t 0 ] && INTERACTIVE=1

# Wait one second; returns 0 if the user pressed Enter (or Ctrl+D) on the terminal.
wait_or_enter() {
    local line rc
    if [ $INTERACTIVE -eq 1 ]; then
        IFS= read -r -t 1 line
        rc=$?
        [ $rc -gt 128 ] && return 1    # timeout
        return 0                       # a line (Enter) or end of input
    fi
    sleep 1
    return 1
}

found=0
skipped=0

# 1) Ports already present (the cable may already be connected).
while [ $found -eq 0 ] && [ "$(remaining)" -gt 0 ]; do
    for dev in $(list_ports); do
        [ "$(remaining)" -le 0 ] && break
        probe "$dev" && { found=1; break; }
    done
    [ $found -eq 0 ] && [ "$(remaining)" -gt 0 ] && sleep 1
done
if [ $found -eq 1 ]; then
    # The installer guide powers the board from the ST-LINK 3.3 V pin; USB-C powers it too.
    echo "Warning: do not connect the USB-C cable while the ST-LINK is attached to the device."
    echo "Next time, disconnect the ST-LINK first and connect USB-C only when asked."
fi

# 2) Ask for the USB-C cable and probe ports that appear afterwards.
if [ $found -eq 0 ] && [ "$WAIT_SECONDS" -gt 0 ]; then
    known=" $(echo $(list_ports)) "
    echo
    echo "Disconnect the ST-LINK from the device, then connect the device's USB-C port"
    echo "to this computer with a data cable."
    if [ $INTERACTIVE -eq 1 ]; then
        echo "Waiting up to $WAIT_SECONDS seconds for the device. Press Enter to skip."
    else
        echo "Waiting up to $WAIT_SECONDS seconds for the device. Press Ctrl+C to skip."
    fi
    DEADLINE_EPOCH=$(( $(date +%s) + WAIT_SECONDS ))
    while [ $found -eq 0 ] && [ "$(remaining)" -gt 0 ]; do
        current=" $(echo $(list_ports)) "
        # A port that disappears (device re-enumerating) counts as new when it returns.
        still=" "
        for dev in $known; do
            case "$current" in *" $dev "*) still="$still$dev ";; esac
        done
        known=$still
        for dev in $current; do
            case "$known" in *" $dev "*) continue;; esac
            [ "$(remaining)" -le 0 ] && break
            # New ports stay unknown, so a port that is not ready yet is probed again next round.
            probe "$dev" && { found=1; break; }
        done
        [ $found -eq 1 ] && break
        if wait_or_enter; then skipped=1; break; fi
    done
fi

if [ $found -eq 0 ]; then
    if [ $skipped -eq 1 ]; then
        echo "Clock sync skipped."
    else
        echo "Could not find a Bosean FS-5000 on a USB serial port; clock not synchronized."
    fi
    echo "Connect the device with USB and set the clock later (any radpro-tool.py command syncs it) or in the settings."
fi
exit 0

#!/bin/bash
# Rad Pro: set the clock and time zone of a Bosean FS-5000 from this computer over USB.
# Linux/macOS, best effort. Never fails the caller: always exits 0.
# Flashing uses only the ST-LINK, so the device's USB-C port may not be connected yet.
# 1) Probe the ports already present for about PROBE_SECONDS (whole rounds; if the device
#    answers here, sync and warn: ST-LINK and USB-C must not power the board together).
# 2) Otherwise ask the user to disconnect the ST-LINK and connect the USB-C cable, and probe
#    all USB serial ports every second for up to WAIT_SECONDS (Enter skips on a terminal).
# Usage: sync-time.sh [WAIT_SECONDS]   (or SYNC_WAIT_SECONDS=N; default 60, negative = 0)
# Each port probe has its own time limit (OP_DEADLINE); a new probe only starts before the
# wait deadline (DEADLINE_EPOCH). Uses only integer read timeouts (macOS bash 3.2).

WAIT_SECONDS=${1:-${SYNC_WAIT_SECONDS:-60}}
case "$WAIT_SECONDS" in
    -*[0-9]*) case "${WAIT_SECONDS#-}" in *[!0-9]*) WAIT_SECONDS=60;; *) WAIT_SECONDS=0;; esac;;
    ''|*[!0-9]*) WAIT_SECONDS=60;;
esac
WAIT_SECONDS=$((10#$WAIT_SECONDS))
PROBE_SECONDS=3    # first rounds over the ports already present
SET_SECONDS=10     # time allowed for the SET requests once the device answered
PORTS_GLOB="/dev/ttyUSB* /dev/ttyACM* /dev/cu.usbserial* /dev/cu.wchusbserial* /dev/cu.usbmodem*"
DEADLINE_EPOCH=$(( $(date +%s) + PROBE_SECONDS ))
OP_DEADLINE=0

remaining() {     # seconds left before DEADLINE_EPOCH (when new probes may start)
    local r=$((DEADLINE_EPOCH - $(date +%s)))
    [ $r -lt 0 ] && r=0
    echo $r
}

op_remaining() {  # seconds left for the current port operation
    local r=$((OP_DEADLINE - $(date +%s)))
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
    python3 - "$1" "$(tz_hours)" "$SET_SECONDS" <<'PY' 2>/dev/null
import sys, time
try:
    import serial
except ImportError:
    sys.exit(2)
port, tz, set_seconds = sys.argv[1], sys.argv[2], float(sys.argv[3])
def req(s, text, wait):
    s.reset_input_buffer()
    s.write((text + "\r\n").encode())
    end = time.time() + wait
    while time.time() < end:
        line = s.readline().decode(errors="replace").strip()
        if line.startswith("OK") or line.startswith("ERROR"):
            return line
    return None
try:
    s = serial.Serial(port, 115200, timeout=0.2, write_timeout=1)
except Exception:
    sys.exit(1)
with s:
    ident = req(s, "GET deviceId", 0.8)
    if not ident or "Bosean FS-5000" not in ident:
        sys.exit(1)
    print("Found " + ident + " on " + port)
    deadline = time.time() + set_seconds    # own time budget for the SET requests
    while True:
        now = int(time.time())
        r1 = req(s, "SET deviceTime %d" % now, 1.5)
        r2 = req(s, "SET deviceTimeZone " + tz, 1.5)
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
    rem=$(op_remaining)
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
    OP_DEADLINE=$(( $(date +%s) + 4 ))    # own limit for this probe: drain ~1 s + GET deviceId 2 s
    stty $STTY_DEV "$dev" 115200 cs8 -cstopb -parenb raw -echo clocal cread 2>/dev/null || return 1
    exec 3<>"$dev" 2>/dev/null || return 1
    while [ "$(op_remaining)" -gt 0 ] && IFS= read -r -n 4096 -t 1 junk <&3; do :; done    # drain stale input until a read times out
    id=$(request "GET deviceId" 2)
    case "$id" in
        *"Bosean FS-5000"*) ;;
        *) exec 3>&-; return 1;;
    esac
    echo "Found $id on $dev"
    OP_DEADLINE=$(( $(date +%s) + SET_SECONDS ))    # own time budget for the SET requests
    tz=$(tz_hours)
    while true; do
        r1=$(request "SET deviceTime $(date -u +%s)" 2)
        r2=$(request "SET deviceTimeZone $tz" 2)
        [ "$r1" = "OK" ] && [ "$r2" = "OK" ] && break
        [ "$(op_remaining)" -le 1 ] && break
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

list_ports() {    # USB serial ports only (PORTS_GLOB order)
    local dev
    for dev in $PORTS_GLOB; do [ -e "$dev" ] && echo "$dev"; done
}

probe() {  # $1 = port; returns 0 if the device answered
    if [ $HAVE_PYSERIAL -eq 1 ]; then try_python "$1"; else try_stty "$1"; fi
}

# Enter detection: a background reader waits for one line on the terminal; a line read
# (exit 0) means Enter, end of input (Ctrl+D) is ignored and the reader is restarted.
# Works with bash 3.2, where `read -t` cannot tell a timeout from end of input.
INTERACTIVE=0
[ -t 0 ] && INTERACTIVE=1
READER_PID=""

start_reader() {
    exec 4<&0
    ( IFS= read -r _line ) <&4 &
    READER_PID=$!
}

stop_reader() {
    [ -n "$READER_PID" ] && kill "$READER_PID" 2>/dev/null
    READER_PID=""
}
trap stop_reader EXIT

enter_pressed() {  # returns 0 if the user pressed Enter
    local rc
    [ -n "$READER_PID" ] || return 1
    kill -0 "$READER_PID" 2>/dev/null && return 1    # still waiting for a line
    wait "$READER_PID"; rc=$?
    READER_PID=""
    [ $rc -eq 0 ] && return 0
    start_reader    # end of input: keep waiting for Enter
    return 1
}

# Discard lines typed before the prompt (bounded; ~1 s when nothing is pending).
drain_stdin() {
    local i=0 _l
    while [ $i -lt 20 ] && IFS= read -r -t 1 _l; do i=$((i + 1)); done
}

# One round over all current ports; sets ROUND to found, skip (Enter) or empty.
# $1 = 1: probe every port regardless of the deadline (the very first round).
# $2 = 1: check Enter between probes.
probe_round() {
    local dev
    ROUND=""
    for dev in $(list_ports); do
        [ "$1" -eq 0 ] && [ "$(remaining)" -le 0 ] && break
        [ "$2" -eq 1 ] && enter_pressed && { ROUND=skip; return; }
        probe "$dev" && { ROUND=found; return; }
    done
}

# ---- main ----
ROUND=""

# 1) Ports already present: one full round, then more rounds until PROBE_SECONDS.
first=1
while true; do
    probe_round $first 0
    first=0
    [ -n "$ROUND" ] && break
    [ "$(remaining)" -le 0 ] && break
    sleep 1
done
if [ "$ROUND" = "found" ]; then
    # The install guide powers the board from the ST-LINK 3.3 V pin; USB-C powers it too.
    echo "Warning: do not connect the USB-C cable while the ST-LINK is attached to the device."
    echo "Next time, disconnect the ST-LINK first and connect USB-C only when asked."
fi

# 2) Ask for the USB-C cable and probe all ports every second.
if [ -z "$ROUND" ] && [ "$WAIT_SECONDS" -gt 0 ]; then
    if [ $INTERACTIVE -eq 1 ]; then
        drain_stdin    # e.g. an impatient Enter typed while flashing
        start_reader
    fi
    echo
    echo "Disconnect the ST-LINK from the device, then connect the device's USB-C port"
    echo "to this computer with a data cable."
    if [ $INTERACTIVE -eq 1 ]; then
        echo "Waiting up to $WAIT_SECONDS seconds for the device. Press Enter to skip."
    else
        echo "Waiting up to $WAIT_SECONDS seconds for the device. Press Ctrl+C to skip."
    fi
    DEADLINE_EPOCH=$(( $(date +%s) + WAIT_SECONDS ))
    while [ -z "$ROUND" ] && [ "$(remaining)" -gt 0 ]; do
        probe_round 0 1
        [ -n "$ROUND" ] && break
        sleep 1
        enter_pressed && ROUND=skip
    done
    stop_reader
fi

if [ "$ROUND" != "found" ]; then
    if [ "$ROUND" = "skip" ]; then
        echo "Clock sync skipped."
    else
        echo "Could not find a Bosean FS-5000 on a USB serial port; clock not synchronized."
    fi
    echo "Connect the device with USB and set the clock later (any radpro-tool.py command syncs it) or in the settings."
fi
exit 0

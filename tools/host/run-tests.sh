#!/bin/bash
set -u
HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/../.." && pwd)"
P="$ROOT/platform.io"
OUT="$HERE/out"

mkdir -p "$OUT"
shopt -s nullglob
tests=("$HERE"/test_*.c)
if [ ${#tests[@]} -eq 0 ]; then
    echo "no tests"
    exit 0
fi

status=0
for t in "${tests[@]}"; do
    name="$(basename "$t" .c)"
    sources=""
    first="$(head -n 1 "$t" | tr -d '\r')"
    case "$first" in
        "// SOURCES:"*) sources="${first#// SOURCES:}" ;;
    esac
    files=()
    for s in $sources; do files+=("$P/src/$s"); done
    if gcc -std=gnu11 -O1 -Wall -Werror -DFS5000 -DLANGUAGE='"ko"' -DSTRINGS='"strings/ko.h"' -I "$P/src" "$t" "${files[@]}" -lm -o "$OUT/$name.exe" && "$OUT/$name.exe"; then
        echo "PASS $name"
    else
        echo "FAIL $name"
        status=1
    fi
done
exit $status

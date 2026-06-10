#!/bin/bash

echo "=== TinyC Compiler Test Suite ==="
echo ""

cd "$(dirname "$0")/.."
BUILDDIR="build"

if [ ! -f "$BUILDDIR/tinyc" ]; then
    echo "Error: tinyc not found. Build first: mkdir build && cd build && cmake .. && make"
    exit 1
fi

PASS=0
FAIL=0

for testfile in tests/test*.tiny; do
    name=$(basename "$testfile" .tiny)
    printf "Running %-10s ... " "$name"

    output=$("$BUILDDIR/tinyc" "$testfile" 2>/dev/null)
    if [ $? -eq 0 ]; then
        exe="${testfile}.out"
        if [ -f "$exe" ]; then
            result=$(./$exe 2>/dev/null)
            echo "PASS  (output: $result)"
            rm -f "$exe" "${testfile}.o" "${testfile}.ll"
            PASS=$((PASS + 1))
        else
            echo "FAIL  (no executable)"
            FAIL=$((FAIL + 1))
        fi
    else
        echo "FAIL  (compilation error)"
        FAIL=$((FAIL + 1))
    fi
done

echo ""
echo "Results: $PASS passed, $FAIL failed out of $((PASS + FAIL)) tests"

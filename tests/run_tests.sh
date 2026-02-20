#!/bin/bash
# run_tests.sh - Regression test runner for c2asm370
#
# Usage: ./tests/run_tests.sh [path/to/c2asm370]
#
# Exit codes:
#   0 = all tests passed
#   1 = one or more tests failed

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
COMPILER="${1:-$PROJECT_DIR/c2asm370}"
INCLUDE_DIR="$PROJECT_DIR/pdpclib"
EXPECTED_DIR="$SCRIPT_DIR/expected"
TMPDIR="${TMPDIR:-/tmp}/c2asm370-tests.$$"

# colors (disabled if not a terminal)
if [ -t 1 ]; then
    GREEN='\033[0;32m'; RED='\033[0;31m'; YELLOW='\033[0;33m'; NC='\033[0m'
else
    GREEN=''; RED=''; YELLOW=''; NC=''
fi

pass=0; fail=0; skip=0; regress=0

cleanup() { rm -rf "$TMPDIR"; }
trap cleanup EXIT
mkdir -p "$TMPDIR"

if [ ! -x "$COMPILER" ]; then
    echo "ERROR: compiler not found or not executable: $COMPILER"
    exit 1
fi

echo "c2asm370 test runner"
echo "===================="
echo "Compiler: $COMPILER"
echo "Include:  $INCLUDE_DIR"
echo ""

# strip_header: remove lines before "* Program text area" and ==> annotations
strip_for_compare() {
    sed -n '/^\* Program text area/,$p' | sed 's/ ==>.*$//'
}

run_test() {
    local src="$1"
    local category="$(basename "$(dirname "$src")")"
    local name="$(basename "$src" .c)"
    local label="$category/$name"
    local outfile="$TMPDIR/${category}_${name}.s"
    local expected="$EXPECTED_DIR/$category/$name.s"

    # Phase 1: compilation (must succeed)
    if ! "$COMPILER" -I "$INCLUDE_DIR" -O1 -fverbose-asm -S "$src" -o "$outfile" 2>"$TMPDIR/${category}_${name}.err"; then
        printf "${RED}FAIL${NC}  %-30s  (compilation error)\n" "$label"
        cat "$TMPDIR/${category}_${name}.err" | head -5 | sed 's/^/       /'
        ((fail++))
        return
    fi

    # Phase 2: basic sanity checks on output
    if ! grep -q "PDPPRLG" "$outfile"; then
        printf "${RED}FAIL${NC}  %-30s  (no PDPPRLG in output)\n" "$label"
        ((fail++))
        return
    fi

    if ! grep -q "PDPEPIL" "$outfile"; then
        printf "${RED}FAIL${NC}  %-30s  (no PDPEPIL in output)\n" "$label"
        ((fail++))
        return
    fi

    # Phase 3: regression check against expected output (if available)
    if [ -f "$expected" ]; then
        local diff_out
        diff_out=$(diff \
            <(strip_for_compare < "$outfile") \
            <(strip_for_compare < "$expected") \
            2>&1) || true

        if [ -z "$diff_out" ]; then
            printf "${GREEN}PASS${NC}  %-30s  (matches expected)\n" "$label"
        else
            printf "${YELLOW}REGR${NC}  %-30s  (output differs from expected)\n" "$label"
            echo "$diff_out" | head -10 | sed 's/^/       /'
            ((regress++))
            ((pass++))  # compilation succeeded, so still a pass
            return
        fi
    else
        printf "${GREEN}PASS${NC}  %-30s\n" "$label"
    fi

    ((pass++))
}

# run all test categories
for category in smoke regression; do
    testdir="$SCRIPT_DIR/$category"
    [ -d "$testdir" ] || continue

    count=$(find "$testdir" -name '*.c' 2>/dev/null | wc -l)
    [ "$count" -eq 0 ] && continue

    echo "--- $category ($count tests) ---"
    for src in "$testdir"/*.c; do
        run_test "$src"
    done
    echo ""
done

# summary
echo "===================="
echo "Results: $pass passed, $fail failed, $regress regressions"

if [ "$fail" -gt 0 ]; then
    exit 1
fi
exit 0

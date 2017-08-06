#!/usr/bin/env bash
set -eu -o pipefail

# This script checks that lines in the given file(s) are at most 80 bytes,
# including newline characters. It prints violations to standard output.

# Usage:
#   ./check-line-lengths.sh file1 file2 ...

VIOLATIONS="$( \
  awk '{ if (length > 79) { print length, FILENAME, "@", FNR }}' "$@" \
)"

if test -n "$VIOLATIONS"; then
  echo "$VIOLATIONS"
  exit 1
fi

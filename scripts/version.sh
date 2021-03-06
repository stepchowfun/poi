#!/usr/bin/env bash
set -eu -o pipefail

# This script creates a C++ source file with information about the version.
# The output is printed to stdout; no actual files are written.

# Usage:
#   ./version.sh BUILD_TYPE

# The source of truth for the version number.
VERSION='0.0.1'

# Get the build type (release or debug).
if echo "$1" | grep -qi '^release$'; then
  BUILD_TYPE='release'
elif echo "$1" | grep -qi '^debug$'; then
  BUILD_TYPE='debug'
else
  echo "BUILD_TYPE must be 'release' or 'debug'" >&2
  exit 1
fi

# Fetch the git commit hash.
if git diff --quiet HEAD > /dev/null 2>&1; then
  COMMIT_HASH="$(git rev-parse --verify 'HEAD^{commit}')"
else
  COMMIT_HASH=''
fi

# Print the version information as a C++ source file.
cat <<-ENDOFMESSAGE
#include <poi/version.h>

const char * const Poi::VERSION = "$VERSION";
const char * const Poi::COMMIT_HASH = $(
  [ -z "$COMMIT_HASH" ] && echo 'nullptr' || echo "\"$COMMIT_HASH\""
);
const char * const Poi::BUILD_TYPE = "$BUILD_TYPE";
ENDOFMESSAGE

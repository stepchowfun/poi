#!/usr/bin/env bash
set -eu -o pipefail

# This script downloads, builds, and installs Poi.

# Usage:
#   ./install.sh

echo "Installing Poi..."

rm -rf /tmp/poi
git clone https://github.com/stepchowfun/poi.git /tmp/poi
cd /tmp/poi && make && (make install || sudo make install)
rm -rf /tmp/poi

echo "Installation successful."
poi --version

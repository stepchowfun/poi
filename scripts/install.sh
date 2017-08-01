#!/usr/bin/sh

# This script downloads, builds, and installs Poi. This is intended to be
# curled and piped into a shell. It should work with any POSIX-compatible
# shell.

# Usage:
#   ./install.sh

echo "Installing Poi..."

rm -rf /tmp/poi
git clone https://github.com/stepchowfun/poi.git /tmp/poi
cd /tmp/poi && make && (make install || sudo make install)
rm -rf /tmp/poi

echo "Installation successful."
poi --version

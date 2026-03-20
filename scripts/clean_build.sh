#!/bin/bash
#
# Clean build directories used by scripts/build.sh and scripts/build_macos.sh.
# For a clean reconfigure, run: CLEAN=yes make linux (or make windows / make macos)
#

set -e

source "$(dirname "${BASH_SOURCE[0]}")/functions.sh"
cd "$REPO_ROOT"

for dir in build_linux build_windows build_macos; do
  if [ -d "$dir" ]; then
    echo "Removing $dir/..."
    rm -rf "$dir"
  fi
done

echo "Done. Run make linux, make windows, or make macos to build."

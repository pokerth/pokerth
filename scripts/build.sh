#!/usr/bin/env bash

###
# Dispatch build to platform-specific scripts:
#   linux / windows → build_linux.sh
#   anything else → build_${TARGET_PLATFORM}.sh (e.g. macos, android)
# Prefer: make linux, make windows, etc.

set -euo pipefail

# shellcheck source=/dev/null
source "$(dirname "${BASH_SOURCE[0]}")/functions.sh"

# Sets TARGET_PLATFORM when unset (Darwin → macos, else linux); Makefile usually sets it explicitly.
default_target_platform_from_uname

case "${TARGET_PLATFORM}" in
  linux|windows)
    exec "${REPO_ROOT}/scripts/build_linux.sh" "$@"
    ;;
  *)
    exec "${REPO_ROOT}/scripts/build_${TARGET_PLATFORM}.sh" "$@"
    ;;
esac

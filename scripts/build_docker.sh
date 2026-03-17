#!/usr/bin/env bash
# Build a Docker image and run a make target (or custom command) in the container.
# Repo is always mounted at /workspaces/pokerth. Optional: --target, extra mounts, env, setup-if-missing.
#
# Usage:
#   build_docker.sh IMAGE DOCKERFILE CONTEXT MAKE_TARGET [OPTIONS]
#
# Positional: IMAGE, DOCKERFILE, CONTEXT (e.g. .), MAKE_TARGET (e.g. windows or android-in-docker)
# Options:
#   --target NAME         docker build --target NAME
#   --mount HOST:GUEST    add -v REPO_ROOT/HOST:GUEST (mkdir -p REPO_ROOT/HOST). Can repeat.
#   -e KEY=VAL            add -e to docker run. Can repeat.
#   --setup-if-missing    run setup when vcpkg not present, then make MAKE_TARGET (for Windows docker)
#
# Example (Windows):
#   build_docker.sh pokerth-windows-dev docker/windows/.devcontainer/Dockerfile . windows \
#     --target base --mount docker/windows/vcpkg:/opt/pokerth-windows \
#     -e VCPKG_DIR=/opt/pokerth-windows/vcpkg -e QT_OUTPUT_DIR=/opt/pokerth-windows/Qt \
#     --setup-if-missing
#
# Example (Android):
#   build_docker.sh pokerth-android-dev docker/android/.devcontainer/Dockerfile docker/android/.devcontainer android-in-docker \
#     -e ANDROID_BUILD_ARGS=""

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

if [ $# -lt 4 ]; then
  echo "Usage: $0 IMAGE DOCKERFILE CONTEXT MAKE_TARGET [--target NAME] [--mount HOST:GUEST] [-e KEY=VAL] [--setup-if-missing]" >&2
  exit 1
fi

IMAGE="$1"
DOCKERFILE="$2"
CONTEXT="$3"
MAKE_TARGET="$4"
shift 4

DOCKER_TARGET=""
MOUNTS=()
ENV_ARGS=()
SETUP_IF_MISSING=0

while [ $# -gt 0 ]; do
  case "$1" in
    --target)
      DOCKER_TARGET="$2"
      shift 2
      ;;
    --mount)
      MOUNTS+=("$2")
      shift 2
      ;;
    -e)
      ENV_ARGS+=(-e "$2")
      shift 2
      ;;
    --setup-if-missing)
      SETUP_IF_MISSING=1
      shift
      ;;
    *)
      echo "Unknown option: $1" >&2
      exit 1
      ;;
  esac
done

cd "$REPO_ROOT"

# Build
BUILD_CMD=(docker build -f "$DOCKERFILE" -t "$IMAGE" "$CONTEXT")
if [ -n "$DOCKER_TARGET" ]; then
  BUILD_CMD+=(--target "$DOCKER_TARGET")
fi
echo "Building image $IMAGE (this may take a while on first run)..."
"${BUILD_CMD[@]}"

# Ensure mount dirs exist and build docker run args
RUN_EXTRA=()
for m in "${MOUNTS[@]}"; do
  host_path="${m%%:*}"
  guest_path="${m#*:}"
  if [[ "$host_path" != /* ]]; then
    mkdir -p "$REPO_ROOT/$host_path"
    RUN_EXTRA+=(-v "$REPO_ROOT/$host_path:$guest_path")
  else
    RUN_EXTRA+=(-v "$m")
  fi
done

if [ ${#MOUNTS[@]} -gt 0 ]; then
  echo "Running build in container (repo + extra mounts)..."
else
  echo "Running build in container (repo mounted at /workspaces/pokerth)..."
fi

if [ ${#MOUNTS[@]} -gt 0 ]; then
  ENV_ARGS+=(-e "WINDOWS_BUILD_SUBDIR=docker/windows/build")
fi

# When using mounts, run as root and chown so host user can edit/delete; pass command via env to avoid duplicating it.
HOST_UID="$(id -u 2>/dev/null || true)"
HOST_GID="$(id -g 2>/dev/null || true)"
if [ "$SETUP_IF_MISSING" -eq 1 ]; then
  DOCKER_CMD="bash scripts/ensure_windows_deps.sh $MAKE_TARGET"
else
  DOCKER_CMD="make $MAKE_TARGET"
fi
if [ ${#MOUNTS[@]} -gt 0 ] && [ -n "$HOST_UID" ] && [ "$HOST_UID" != "0" ]; then
  CHOWN_DIRS="/workspaces/pokerth/docker/windows/build"
  for m in "${MOUNTS[@]}"; do CHOWN_DIRS="$CHOWN_DIRS ${m#*:}"; done
  ENV_ARGS+=(-e "HOST_UID=$HOST_UID" -e "HOST_GID=$HOST_GID")
  CMD=(bash -c "set -e; eval \"\$DOCKER_CMD\"; r=\$?; chown -R \${HOST_UID}:\${HOST_GID} ${CHOWN_DIRS} 2>/dev/null || true; exit \$r")
else
  CMD=(bash -c 'eval "$DOCKER_CMD"')
fi
ENV_ARGS+=(-e "DOCKER_CMD=$DOCKER_CMD")

docker run --rm \
  -v "$REPO_ROOT:/workspaces/pokerth:rw" \
  "${RUN_EXTRA[@]}" \
  "${ENV_ARGS[@]}" \
  -w /workspaces/pokerth \
  --user root \
  "$IMAGE" \
  "${CMD[@]}"

#!/usr/bin/env bash
# Build Docker image and run a make target. Windows: config from devcontainer.json (jq).
# Android: explicit DOCKERFILE/CONTEXT and -e ANDROID_BUILD_ARGS.
#
# Usage:
#   build_docker.sh IMAGE DOCKERFILE CONTEXT MAKE_TARGET [OPTIONS]
#
# Windows (make windows-docker / windows-installer-docker):
#   MAKE_TARGET=windows or windows-installer. DOCKERFILE/CONTEXT ignored; uses docker/windows/.devcontainer/devcontainer.json.
#   Requires jq. Runs: ensure_docker_deps.sh MAKE_TARGET.
#
# Android (make android-docker):
#   MAKE_TARGET=android-in-docker. Uses DOCKERFILE, CONTEXT. Options: -e KEY=VAL (e.g. ANDROID_BUILD_ARGS).

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

if [ $# -lt 4 ]; then
  echo "Usage: $0 IMAGE DOCKERFILE CONTEXT MAKE_TARGET [-e KEY=VAL] ..." >&2
  exit 1
fi

IMAGE="$1"
DOCKERFILE="$2"
CONTEXT="$3"
MAKE_TARGET="$4"
shift 4

ENV_ARGS=()
BUILD_TARGET_OPT=""
MOUNT_OPTS=()
while [ $# -gt 0 ]; do
  case "$1" in
    -e) ENV_ARGS+=(-e "$2"); shift 2 ;;
    --target) BUILD_TARGET_OPT="$2"; shift 2 ;;
    --mount) MOUNT_OPTS+=(-v "$2"); shift 2 ;;
    *) echo "Unknown option: $1" >&2; exit 1 ;;
  esac
done

cd "$REPO_ROOT"

# On macOS, enforce amd64 for Windows/Android Docker (arm64 has limited support for aqt and Android tooling).
PLATFORM_OPT=()
if [ "$(uname -s)" = "Darwin" ] && { [ "$MAKE_TARGET" = "windows" ] || [ "$MAKE_TARGET" = "windows-installer" ] || [ "$MAKE_TARGET" = "android-in-docker" ]; }; then
  PLATFORM_OPT=(--platform linux/amd64)
fi

# Windows: drive from devcontainer.json so make windows-docker matches devcontainer.
if [ "$MAKE_TARGET" = "windows" ] || [ "$MAKE_TARGET" = "windows-installer" ]; then
  command -v jq >/dev/null 2>&1 || { echo "jq required for Windows docker (e.g. brew install jq)." >&2; exit 1; }
  DEVCONTAINER_JSON="$REPO_ROOT/docker/windows/.devcontainer/devcontainer.json"
  DEVCONTAINER_DIR="$(cd "$(dirname "$DEVCONTAINER_JSON")" && pwd)"

  DOCKERFILE_ABS="$DEVCONTAINER_DIR/$(jq -r '.build.dockerfile' "$DEVCONTAINER_JSON")"
  BUILD_TARGET="$(jq -r '.build.target // empty' "$DEVCONTAINER_JSON")"
  BUILD_CONTEXT_ABS="$(cd "$DEVCONTAINER_DIR" && cd "$(jq -r '.build.context' "$DEVCONTAINER_JSON")" && pwd)"
  REMOTE_USER="$(jq -r '.remoteUser // empty' "$DEVCONTAINER_JSON")"
  LOCAL_WORKSPACE_FOLDER="$REPO_ROOT/docker/windows"

  WORKSPACE_MOUNT_STR="$(jq -r '.workspaceMount' "$DEVCONTAINER_JSON")"
  WORKSPACE_MOUNT_STR="${WORKSPACE_MOUNT_STR//\$\{localWorkspaceFolder\}/$LOCAL_WORKSPACE_FOLDER}"
  workspace_source=""; workspace_target=""; consistency=""
  IFS=',' read -ra workspace_parts <<< "$WORKSPACE_MOUNT_STR"
  for part in "${workspace_parts[@]}"; do
    key="${part%%=*}"; val="${part#*=}"
    case "$key" in source) workspace_source="$val" ;; target) workspace_target="$val" ;; consistency) consistency="$val" ;; esac
  done
  workspace_source="$(cd "$workspace_source" && pwd)"
  CONSISTENCY_SUFFIX=""; [ "$consistency" = "cached" ] && CONSISTENCY_SUFFIX=":cached"

  RUN_EXTRA=()
  RUN_EXTRA+=(-v "$workspace_source:$workspace_target${CONSISTENCY_SUFFIX}")
  while IFS= read -r mount_str; do
    mount_str="${mount_str//\$\{localWorkspaceFolder\}/$LOCAL_WORKSPACE_FOLDER}"
    mount_source="$(printf '%s' "$mount_str" | awk -F',' '{for(i=1;i<=NF;i++){if($i~/^source=/){print substr($i,8)}}}')"
    mount_target="$(printf '%s' "$mount_str" | awk -F',' '{for(i=1;i<=NF;i++){if($i~/^target=/){print substr($i,8)}}}')"
    if [ -n "$mount_source" ] && [ -n "$mount_target" ]; then
      mkdir -p "$mount_source"
      RUN_EXTRA+=(-v "$mount_source:$mount_target")
    fi
  done < <(jq -r '.mounts // [] | .[]' "$DEVCONTAINER_JSON")

  while IFS= read -r line; do
    key="${line%%=*}"; val="${line#*=}"
    [ -n "$key" ] && ENV_ARGS+=(-e "$key=$val")
  done < <(jq -r '.containerEnv // {} | to_entries[] | "\(.key)=\(.value)"' "$DEVCONTAINER_JSON")

  BUILD_CMD=(docker build -f "$DOCKERFILE_ABS" -t "$IMAGE" "${PLATFORM_OPT[@]}" "$BUILD_CONTEXT_ABS")
  [ -n "$BUILD_TARGET" ] && [ "$BUILD_TARGET" != "null" ] && BUILD_CMD+=(--target "$BUILD_TARGET")
  echo "Building $IMAGE (Windows devcontainer config)..."
  "${BUILD_CMD[@]}"

  # Run as root so ensure_docker_deps.sh can write to the cache, chown it (best-effort), then run make as vscode.
  docker run --rm \
    "${PLATFORM_OPT[@]}" \
    --user root \
    "${RUN_EXTRA[@]}" \
    "${ENV_ARGS[@]}" \
    -w "$workspace_target" \
    "$IMAGE" \
    ./scripts/ensure_docker_deps.sh "$MAKE_TARGET"
  exit 0
fi

# Android: bind-mount only vcpkg at /opt/pokerth-android/vcpkg so SDK/NDK/Qt from the image stay visible.
if [ "$MAKE_TARGET" = "android-in-docker" ]; then
  ANDROID_VCPKG_HOST="$REPO_ROOT/docker/android/build/vcpkg"
  mkdir -p "$ANDROID_VCPKG_HOST"
  ANDROID_VCPKG_HOST="$(cd "$ANDROID_VCPKG_HOST" && pwd)"
  # :cached (macOS Docker) can avoid bind-mount utime errors (e.g. tar "Cannot utime"); keeps downloads cached like windows-docker.
  MOUNT_OPTS+=( -v "${ANDROID_VCPKG_HOST}:/opt/pokerth-android/vcpkg:cached" )
fi

BUILD_CMD=(docker build -f "$DOCKERFILE" -t "$IMAGE" "${PLATFORM_OPT[@]}" "$CONTEXT")
[ -n "$BUILD_TARGET_OPT" ] && BUILD_CMD+=(--target "$BUILD_TARGET_OPT")
echo "Building $IMAGE..."
"${BUILD_CMD[@]}"

# Android: run as root (like Windows) so ensure_docker_deps.sh can run vcpkg (tar etc.) and chown the cache, then run make as vscode.
if [ "$MAKE_TARGET" = "android-in-docker" ]; then
  docker run --rm \
    "${PLATFORM_OPT[@]}" \
    --user root \
    -v "$REPO_ROOT:/workspaces/pokerth:rw" \
    "${MOUNT_OPTS[@]}" \
    "${ENV_ARGS[@]}" \
    -w /workspaces/pokerth \
    "$IMAGE" \
    ./scripts/ensure_docker_deps.sh android-in-docker
  exit 0
fi

docker run --rm \
  -v "$REPO_ROOT:/workspaces/pokerth:rw" \
  "${MOUNT_OPTS[@]}" \
  "${ENV_ARGS[@]}" \
  -w /workspaces/pokerth \
  "$IMAGE" \
  make "$MAKE_TARGET"

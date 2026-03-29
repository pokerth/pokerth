# =============================================================================
# PokerTH — Android (docker/android/Dockerfile)
# Base + final: SDK/NDK/Gradle toolchain in image; Qt + vcpkg on docker run (ensure -> setup.sh deps).
# Host: devcontainer binds docker/${TARGET_PLATFORM}/build/{vcpkg,Qt} -> /opt/pokerth-${TARGET_PLATFORM}/{vcpkg,Qt}.
#       SDK/NDK stay in image under /opt/pokerth-${TARGET_PLATFORM} (workspace bind mount does not cover /opt).
# Intended: make android-docker or devcontainer rebuild (scripts/run_devcontainer.py). No RUN --mount.
# Cache: apt layer tracks only scripts/${TARGET_PLATFORM}-apt-packages.txt (SKIP_SYSTEM_PACKAGES=yes skips repeat apt in setup).
# =============================================================================

FROM ubuntu:25.10 AS base

# --- Args ---
ARG USERNAME=vscode
ARG USER_UID=1000
ARG USER_GID=1000
ARG LANG=de_DE.UTF-8
ARG LANGUAGE=de_DE.UTF-8
ARG LC_ALL=de_DE.UTF-8

# So later stages (e.g. final) can USER ${USERNAME}; ARG alone is not inherited across FROM.
ENV USERNAME=${USERNAME}

ENV DEBIAN_FRONTEND=noninteractive
ENV TARGET=pokerth_client
# Skip repeat apt in setup.sh (base already installed packages). SKIP_SYSTEM_PACKAGES = apt only.
ENV SKIP_SYSTEM_PACKAGES=yes

# --- Platform: Android ---
# TARGET_PLATFORM, ROOT, and BUILD_DIR for setup.sh; ROOT used for mkdir/chown below.
ENV TARGET_PLATFORM=android
ENV ROOT=/opt/pokerth-${TARGET_PLATFORM}
ENV VCPKG_DIR=${ROOT}/vcpkg
# Align with Makefile REPO_BUILD_ROOT=docker/${TARGET_PLATFORM}/build when IN_DOCKER=1.
ENV BUILD_DIR=docker/${TARGET_PLATFORM}/build

RUN sed -i 's|http://archive.ubuntu.com/ubuntu/|http://de.archive.ubuntu.com/ubuntu/|' /etc/apt/sources.list.d/ubuntu.sources

# Locales first (so perl during apt does not warn)
RUN unset LANG LC_ALL LANGUAGE && apt-get update && apt-get install -y --no-install-recommends locales && rm -rf /var/lib/apt/lists/*
RUN _locale="${LANG}" && unset LANG LC_ALL LANGUAGE && sed -i '/'${_locale}'/s/^# *//' /etc/locale.gen && locale-gen
ENV LANG=${LANG}
ENV LANGUAGE=${LANGUAGE}
ENV LC_ALL=${LC_ALL}

# --- System packages (scripts/${TARGET_PLATFORM}-apt-packages.txt) ---
COPY scripts/${TARGET_PLATFORM}-apt-packages.txt /tmp/${TARGET_PLATFORM}-apt-packages.txt
RUN apt-get update && apt-get install -y --no-install-recommends \
    $(grep -v '^#' /tmp/${TARGET_PLATFORM}-apt-packages.txt | tr '\n' ' ') \
    && rm -rf /var/lib/apt/lists/*

# --- User + /opt (setup.sh populates caches) ---
RUN userdel ubuntu 2>/dev/null || true; rm -Rf /home/ubuntu
RUN groupadd -g ${USER_GID} ${USERNAME} && useradd -s /bin/bash -m -d "/home/${USERNAME}" -u ${USER_UID} -g ${USERNAME} ${USERNAME}
RUN mkdir -p /etc/sudoers.d && echo ${USERNAME} ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/${USERNAME} && chmod 0440 /etc/sudoers.d/${USERNAME}
RUN mkdir -p ${ROOT} && chown -R ${USERNAME}:${USERNAME} ${ROOT}

# --- Repo scripts (patches/, etc.); context = repo root ---
COPY scripts /workspaces/pokerth/scripts
RUN chown -R ${USERNAME}:${USERNAME} /workspaces/pokerth
WORKDIR /workspaces/pokerth

# Base runs as ${USERNAME} so make/setup use correct uid; matches remoteUser in devcontainer.
USER ${USERNAME}

# --- Final: IN_DOCKER + toolchain (Qt/vcpkg still from docker run / mounts) ---
FROM base AS final
# Makefile REPO_BUILD_ROOT=docker/<kind>/build when IN_DOCKER=1; bake for shells if containerEnv is missing.
ENV IN_DOCKER=1
LABEL org.opencontainers.image.description="PokerTH Android devcontainer (toolchain in image; deps on docker run)"

RUN ./scripts/setup.sh toolchain

# Runtime: workspace at /workspaces/pokerth when mounted; setup_android.sh writes ${ROOT}/.manifest.env under /opt/pokerth-${TARGET_PLATFORM}.

# =============================================================================
# PokerTH — Unified devcontainer + make *-docker image (canonical).
# Base: merged apt (shared + android + windows host tools). MinGW posix thread model: setup_linux.sh (windows). Linux native
#   libs + system Qt: same path as host — scripts/setup_linux.sh (USE_AQT=no) as vscode.
# Final: Android + Windows toolchain RUNs.
# Qt/vcpkg: workspace docker/<kind>/build on docker run (ensure -> setup.sh deps).
# Build:  docker build -f docker/Dockerfile -t pokerth-dev .
# Context: repo root (same as devcontainer build.context ..).
# =============================================================================

FROM ubuntu:25.10 AS base

ARG USERNAME=vscode
ARG USER_UID=1000
ARG USER_GID=1000
ARG LANG=de_DE.UTF-8
ARG LANGUAGE=de_DE.UTF-8
ARG LC_ALL=de_DE.UTF-8

ENV USERNAME=${USERNAME}
ENV DEBIAN_FRONTEND=noninteractive
ENV TARGET=pokerth_client
# Do not set SKIP_SYSTEM_PACKAGES — matches native setup_linux default (full OS installs).

RUN unset LANG LC_ALL LANGUAGE && apt-get update && apt-get install -y --no-install-recommends locales && rm -rf /var/lib/apt/lists/*
RUN _locale="${LANG}" && unset LANG LC_ALL LANGUAGE && sed -i '/'${_locale}'/s/^# *//' /etc/locale.gen && locale-gen
ENV LANG=${LANG}
ENV LANGUAGE=${LANGUAGE}
ENV LC_ALL=${LC_ALL}

ENV WORKSPACE=/workspaces/pokerth

COPY Makefile ${WORKSPACE}/
COPY scripts ${WORKSPACE}/scripts/

# Shared + Android/Windows host tools only. Linux app dev (Boost, Qt6, …) comes from setup_linux.sh — same as native.
RUN grep -hv '^#' ${WORKSPACE}/scripts/apt-packages.txt ${WORKSPACE}/scripts/android-apt-packages.txt ${WORKSPACE}/scripts/windows-apt-packages.txt \
    > /tmp/apt-packages.txt
RUN apt-get update && apt-get install -y --no-install-recommends $(cat /tmp/apt-packages.txt) \
    && rm -rf /var/lib/apt/lists/*

RUN userdel ubuntu 2>/dev/null || true; rm -Rf /home/ubuntu
RUN groupadd -g ${USER_GID} ${USERNAME} && useradd -s /bin/bash -m -d "/home/${USERNAME}" -u ${USER_UID} -g ${USERNAME} ${USERNAME}
RUN mkdir -p /etc/sudoers.d && echo ${USERNAME} ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/${USERNAME} && chmod 0440 /etc/sudoers.d/${USERNAME}
RUN mkdir -p /opt/pokerth/android /opt/pokerth/windows && chown -R ${USERNAME}:${USERNAME} /opt/pokerth

RUN chown -R ${USERNAME}:${USERNAME} ${WORKSPACE}
WORKDIR ${WORKSPACE}
USER ${USERNAME}

# Native Linux: identical to host `TARGET_PLATFORM=linux USE_AQT=no` (LINUX_APT_EXTRA + system Qt6 apt list in setup_linux.sh).
# System packages persist in the image; `build_linux/.manifest.env` written here is hidden when the workspace bind-mount replaces this tree at devcontainer run — run `make setup-linux` or `make linux` if the manifest is missing on the host.
RUN env SKIP_SYSTEM_PACKAGES=no USE_AQT=no TARGET_PLATFORM=linux bash ./scripts/setup_linux.sh

FROM base AS final
ENV IN_DOCKER=1
LABEL org.opencontainers.image.description="PokerTH unified devcontainer (Android + Windows toolchains; native Linux deps via setup_linux.sh; Qt/vcpkg per-kind on docker run)"

# Toolchains for all unified targets (android + windows): setup-toolchains. Qt/vcpkg: first make android / make windows (or host make *-docker + ensure).
# CACHE_ROOT=/opt/pokerth from scripts/functions.sh when IN_DOCKER=1; docker run bind-mounts host ~/.pokerth here.
RUN make setup-toolchains

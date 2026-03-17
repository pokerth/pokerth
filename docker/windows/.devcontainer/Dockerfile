# Windows cross-build image: base + scripts/setup.sh (single source of truth for vcpkg, Qt, ports).
# Build from repo root: docker build -f docker/windows/.devcontainer/Dockerfile .
# Run: make windows-docker (mounts repo, runs make windows which uses scripts/build.sh).
#
# Cache: apt runs in a layer that only depends on scripts/windows-apt-packages.txt; other script/doc changes don't invalidate it. setup.sh uses the same file (SKIP_SYSTEM_PACKAGES=yes here).

# Stage "base": everything before setup.sh (use --target base to run setup with a log mount and capture vcpkg logs on failure).
FROM ubuntu:25.10 AS base

ARG USERNAME=vscode
ARG USER_UID=1000
ARG USER_GID=1000
ARG LANG=de_DE.UTF-8
ARG LANGUAGE=de_DE.UTF-8
ARG LC_ALL=de_DE.UTF-8

ENV DEBIAN_FRONTEND=noninteractive
ENV TARGET=pokerth_client

RUN sed -i 's|http://archive.ubuntu.com/ubuntu/|http://de.archive.ubuntu.com/ubuntu/|' /etc/apt/sources.list.d/ubuntu.sources

# Locales first so perl (during apt) does not warn
RUN unset LANG LC_ALL LANGUAGE && apt-get update && apt-get install -y --no-install-recommends locales && rm -rf /var/lib/apt/lists/*
RUN _locale="${LANG}" && unset LANG LC_ALL LANGUAGE && sed -i '/'${_locale}'/s/^# *//' /etc/locale.gen && locale-gen
ENV LANG=${LANG}
ENV LANGUAGE=${LANGUAGE}
ENV LC_ALL=${LC_ALL}

# System packages from shared list (scripts/windows-apt-packages.txt); only this COPY + RUN invalidate when the list changes.
COPY scripts/windows-apt-packages.txt /tmp/windows-apt-packages.txt
RUN apt-get update && apt-get install -y --no-install-recommends \
    $(grep -v '^#' /tmp/windows-apt-packages.txt | tr '\n' ' ') \
    && rm -rf /var/lib/apt/lists/*

# User and dir for vcpkg/Qt (setup.sh will populate these)
RUN userdel ubuntu 2>/dev/null || true; rm -Rf /home/ubuntu
RUN groupadd -g ${USER_GID} ${USERNAME} && useradd -s /bin/bash -m -d /home/vscode -u ${USER_UID} -g ${USERNAME} ${USERNAME}
RUN mkdir -p /etc/sudoers.d && echo ${USERNAME} ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/${USERNAME} && chmod 0440 /etc/sudoers.d/${USERNAME}
RUN mkdir -p /opt/pokerth-windows && chown -R ${USERNAME}:${USERNAME} /opt/pokerth-windows

# Default to vscode so make/setup create repo and cache files as vscode (not root); matches remoteUser in devcontainer.
USER ${USERNAME}

# Copy only what setup.sh needs (scripts + docs for patches); build context must be repo root.
COPY scripts /workspaces/pokerth/scripts
COPY docs    /workspaces/pokerth/docs
WORKDIR /workspaces/pokerth

# Stage "final": run setup (skip apt; vcpkg + aqtinstall only). Single source of truth for ports and Qt.
FROM base AS final
RUN SKIP_SYSTEM_PACKAGES=yes \
    VCPKG_DIR=/opt/pokerth-windows/vcpkg \
    QT_OUTPUT_DIR=/opt/pokerth-windows/Qt \
    TARGET_PLATFORM=windows \
    USE_AQT=yes \
    USE_VCPKG=yes \
    ./scripts/setup.sh

RUN chown -R ${USERNAME}:${USERNAME} /opt/pokerth-windows
USER ${USERNAME}

# At runtime the repo is mounted at /workspaces/pokerth; make windows runs scripts/build.sh.
# VCPKG_DIR and QT_OUTPUT_DIR are passed by the Makefile when running the container.

# =============================================================================
# PokerTH — Windows cross (docker/windows/Dockerfile)
# Base + final: MinGW toolchain in image; Qt + vcpkg on docker run (ensure -> setup.sh deps).
# Host: vcpkg/Qt under docker/${TARGET_PLATFORM}/build/ (workspace mount); functions.sh sets VCPKG_* / QT_OUTPUT_DIR when IN_DOCKER=1.
# Intended: make windows-docker or devcontainer rebuild (scripts/run_devcontainer.py). No RUN --mount.
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

# --- Platform: Windows ---
# TARGET_PLATFORM, ROOT, BUILD_DIR for setup.sh; ROOT for /opt mkdir. vcpkg/Qt paths: scripts/functions.sh when IN_DOCKER=1.
ENV TARGET_PLATFORM=windows
ENV ROOT=/opt/pokerth-${TARGET_PLATFORM}
ENV BUILD_DIR=docker/${TARGET_PLATFORM}/build

RUN sed -i 's|http://archive.ubuntu.com/ubuntu/|http://de.archive.ubuntu.com/ubuntu/|' /etc/apt/sources.list.d/ubuntu.sources

# Locales first (so perl during apt does not warn)
RUN unset LANG LC_ALL LANGUAGE && apt-get update && apt-get install -y --no-install-recommends locales && rm -rf /var/lib/apt/lists/*
RUN _locale="${LANG}" && unset LANG LC_ALL LANGUAGE && sed -i '/'${_locale}'/s/^# *//' /etc/locale.gen && locale-gen
ENV LANG=${LANG}
ENV LANGUAGE=${LANGUAGE}
ENV LC_ALL=${LC_ALL}

ENV WORKSPACE=/workspaces/pokerth

# --- Repo scripts (patches/, etc.); context = repo root ---
COPY scripts ${WORKSPACE}/scripts/

# --- System packages (scripts/apt-packages.txt and scripts/${TARGET_PLATFORM}-apt-packages.txt) ---
RUN grep -hv '^#' ${WORKSPACE}/scripts/apt-packages.txt ${WORKSPACE}/scripts/${TARGET_PLATFORM}-apt-packages.txt \
    > /tmp/apt-packages.txt
RUN apt-get update && apt-get install -y --no-install-recommends $(cat /tmp/apt-packages.txt) \
    && rm -rf /var/lib/apt/lists/*

# --- Platform: MinGW (posix gcc/g++ for vcpkg / std::mutex) ---
RUN for t in gcc g++; do update-alternatives --set x86_64-w64-mingw32-${t} /usr/bin/x86_64-w64-mingw32-${t}-posix 2>/dev/null || true; done

# --- User + /opt (setup.sh populates caches) ---
RUN userdel ubuntu 2>/dev/null || true; rm -Rf /home/ubuntu
RUN groupadd -g ${USER_GID} ${USERNAME} && useradd -s /bin/bash -m -d "/home/${USERNAME}" -u ${USER_UID} -g ${USERNAME} ${USERNAME}
RUN mkdir -p /etc/sudoers.d && echo ${USERNAME} ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/${USERNAME} && chmod 0440 /etc/sudoers.d/${USERNAME}
RUN mkdir -p ${ROOT} && chown -R ${USERNAME}:${USERNAME} ${ROOT}

# --- Repo scripts (patches/, etc.); context = repo root ---
RUN chown -R ${USERNAME}:${USERNAME} ${WORKSPACE}
WORKDIR ${WORKSPACE}

# Base runs as ${USERNAME} so make/setup use correct uid; matches remoteUser in devcontainer.
USER ${USERNAME}

# --- Final: IN_DOCKER + toolchain (Qt/vcpkg still from docker run / mounts) ---
FROM base AS final
# Makefile REPO_BUILD_ROOT=docker/<kind>/build when IN_DOCKER=1; bake for shells if containerEnv is missing.
ENV IN_DOCKER=1
LABEL org.opencontainers.image.description="PokerTH Windows cross devcontainer (toolchain in image; deps on docker run)"

RUN ./scripts/setup.sh toolchain

# Runtime: workspace at /workspaces/pokerth when mounted; make windows -> scripts/build.sh.

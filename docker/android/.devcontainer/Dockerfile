# Android cross-build image: base + scripts/setup_android.sh (single source of truth for SDK/NDK/Gradle/Qt/vcpkg).
# Build from repo root: docker build -f docker/android/.devcontainer/Dockerfile .
# Run: make android-docker (mounts repo, runs make android which uses scripts/build_android.sh).
#
# Mirrors docker/windows/.devcontainer/Dockerfile: apt from file, COPY scripts/docs, run setup script.
# Cache: apt runs in a layer that only depends on scripts/android-apt-packages.txt.

# Stage "base": everything before setup_android.sh.
FROM ubuntu:25.10 AS base

ARG USERNAME=vscode
ARG USER_UID=1000
ARG USER_GID=1000
ARG LANG=de_DE.UTF-8
ARG LANGUAGE=de_DE.UTF-8
ARG LC_ALL=de_DE.UTF-8

ENV DEBIAN_FRONTEND=noninteractive
ENV TARGET=pokerth_client

# Architecture names: android (armeabi-v7a/arm64-v8a/x86/x86_64), vcpkg (arm-android/arm64-android/...), Qt (armv7/arm64_v8a/...)
ENV ANDROID_ARCH=arm64-v8a
ENV VCPKG_TRIPLET=arm64-android
ENV QT_ARCH=arm64_v8a

ARG ROOT=/opt/pokerth-android
ENV ROOT=${ROOT}
ENV VCPKG_DIR=${ROOT}/vcpkg

RUN sed -i 's|http://archive.ubuntu.com/ubuntu/|http://de.archive.ubuntu.com/ubuntu/|' /etc/apt/sources.list.d/ubuntu.sources

# Locales first (so perl during apt does not warn)
RUN unset LANG LC_ALL LANGUAGE && apt-get update && apt-get install -y --no-install-recommends locales && rm -rf /var/lib/apt/lists/*
RUN _locale="${LANG}" && unset LANG LC_ALL LANGUAGE && sed -i '/'${_locale}'/s/^# *//' /etc/locale.gen && locale-gen
ENV LANG=${LANG}
ENV LANGUAGE=${LANGUAGE}
ENV LC_ALL=${LC_ALL}

# System packages from shared list (scripts/android-apt-packages.txt)
COPY scripts/android-apt-packages.txt /tmp/android-apt-packages.txt
RUN apt-get update && apt-get install -y --no-install-recommends \
    $(grep -v '^#' /tmp/android-apt-packages.txt | tr '\n' ' ') \
    && rm -rf /var/lib/apt/lists/*

# Docker-in-Docker (Android devcontainer convenience)
RUN install -m 0755 -d /etc/apt/keyrings && \
    curl -fsSL https://download.docker.com/linux/ubuntu/gpg -o /etc/apt/keyrings/docker.asc && \
    chmod a+r /etc/apt/keyrings/docker.asc && \
    echo "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.asc] https://download.docker.com/linux/ubuntu $(. /etc/os-release && echo "${UBUNTU_CODENAME:-$VERSION_CODENAME}") stable" | \
    tee /etc/apt/sources.list.d/docker.list > /dev/null
RUN apt-get update && apt-get install -y docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin && rm -rf /var/lib/apt/lists/*

ENV TZ=Europe/Berlin
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

# User and dir for SDK/NDK/Qt/vcpkg (setup_android.sh will populate these)
RUN userdel ubuntu 2>/dev/null || true; rm -Rf /home/ubuntu
RUN groupadd -g ${USER_GID} ${USERNAME} && useradd -s /bin/bash -m -d /home/vscode -u ${USER_UID} -g ${USERNAME} ${USERNAME}
RUN mkdir -p /etc/sudoers.d && echo ${USERNAME} ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/${USERNAME} && chmod 0440 /etc/sudoers.d/${USERNAME}
RUN usermod -aG docker ${USERNAME}
RUN mkdir -p ${ROOT} && chown -R ${USERNAME}:${USERNAME} ${ROOT}

# Copy only what setup_android.sh needs (scripts + docs); build context must be repo root.
COPY scripts /workspaces/pokerth/scripts
COPY docs    /workspaces/pokerth/docs
RUN chown -R ${USERNAME}:${USERNAME} /workspaces/pokerth
WORKDIR /workspaces/pokerth

USER ${USERNAME}

# Stage "final": run setup_android.sh (skip apt; SDK/NDK/Gradle/Qt/vcpkg). Single source of truth.
FROM base AS final
RUN SKIP_SYSTEM_PACKAGES=yes \
    ANDROID_CACHE_ROOT=${ROOT} \
    VCPKG_DIR=${VCPKG_DIR} \
    ROOT=${ROOT} \
    BUILD_DIR=docker/android/build \
    ./scripts/setup_android.sh

# setup_android.sh writes ${ROOT}/.android_env (survives repo mount). build_android.sh sources it when repo-local .android_env is missing.
# At runtime the repo is mounted at /workspaces/pokerth; make android runs scripts/build_android.sh.
# SDK/NDK/Qt/vcpkg live under /opt/pokerth-android. ensure_docker_deps runs setup when vcpkg not ready.

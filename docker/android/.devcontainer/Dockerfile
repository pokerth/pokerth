FROM ubuntu:25.10

# Set architecture environment variables:
# as android, vcpkg and qt architectures have slightly different namings, we define all three here
# possible values are:
# ANDROID_ARCH: armeabi-v7a, arm64-v8a, x86, x86_64
# VCPKG_ARCH: arm, arm64, x86, x64
# QT_ARCH: armv7, arm64_v8a, x86, x86_64
ENV ANDROID_ARCH=arm64-v8a
ENV VCPKG_ARCH=arm64
ENV QT_ARCH=arm64_v8a
# set target (pokerth_client or pokerth_qml-client)
ENV TARGET=pokerth_client

RUN sed -i 's|http://archive.ubuntu.com/ubuntu/|http://de.archive.ubuntu.com/ubuntu/|' /etc/apt/sources.list.d/ubuntu.sources

# better not touch these values
ENV ANDROID_API_LEVEL=35
ENV QT_VERSION=6.9.3
ENV GRADLE_VERSION=8.3
ENV ANDROID_NDK_VERSION=28.0.13004108

# Set other build environment variables
ARG USERNAME=vscode
ARG USER_UID=1000
ARG USER_GID=1000
ARG LANG=de_DE.UTF-8
ARG LANGUAGE=de_DE.UTF-8
ARG LC_ALL=de_DE.UTF-8

ARG ROOT=/opt/pokerth-android
ENV ROOT=${ROOT}
ENV DEBIAN_FRONTEND=noninteractive

ARG ANDROID_SDK_ROOT=${ROOT}/android-sdk
ENV ANDROID_SDK_ROOT=${ANDROID_SDK_ROOT}
ENV ANDROID_NDK_ROOT=${ANDROID_SDK_ROOT}/ndk/${ANDROID_NDK_VERSION}
ENV ANDROID_NDK_HOME=${ANDROID_SDK_ROOT}/ndk/${ANDROID_NDK_VERSION}

ENV VCPKG_ROOT=${ROOT}/vcpkg
ENV VCPKG_CHAINLOAD_TOOLCHAIN_FILE $ANDROID_NDK_ROOT/build/cmake/android.toolchain.cmake

ENV QT_ANDROID_DIR=${ROOT}/Qt/${QT_VERSION}/android
ENV QT_HOST_PATH=${ROOT}/Qt/${QT_VERSION}/gcc_64

ENV PATH=$PATH:${ANDROID_SDK_ROOT}/platform-tools:${ANDROID_SDK_ROOT}/cmdline-tools/latest/bin:${ROOT}/gradle/gradle-${GRADLE_VERSION}/bin:${ROOT}/Qt/${QT_VERSION}/gcc_64/bin:${ROOT}/vcpkg

# Install dependencies
RUN apt-get update \
  && apt-get install -y --no-install-recommends \
    ca-certificates curl wget unzip git build-essential pkg-config \
    python3 python3-pip python3-venv jq \
    libwebsocketpp-dev \
    libboost1.88-all-dev \
    locales sudo nano mc openjdk-17-jdk-headless cmake ninja-build zip unzip \
    google-android-build-tools-28.0.1-installer \
  && rm -rf /var/lib/apt/lists/*

# Install docker
RUN install -m 0755 -d /etc/apt/keyrings && \
    curl -fsSL https://download.docker.com/linux/ubuntu/gpg -o /etc/apt/keyrings/docker.asc && \
    chmod a+r /etc/apt/keyrings/docker.asc && \
    echo \
    "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.asc] https://download.docker.com/linux/ubuntu \
    $(. /etc/os-release && echo "${UBUNTU_CODENAME:-$VERSION_CODENAME}") stable" | \
    tee /etc/apt/sources.list.d/docker.list > /dev/null
RUN apt-get update && apt-get install -y docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin

# locales & timezone
RUN sed -i '/'${LANG}'/s/^# //g' /etc/locale.gen && \
    locale-gen
ENV LANG ${LANG}
ENV LANGUAGE ${LANGUAGE}
ENV LC_ALL ${LC_ALL}
ENV TZ=Europe/Berlin
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

# apt cleanup
RUN apt-get autoremove --purge && rm -Rf /var/cache/apt/archives && \
    unset DEBIAN_FRONTEND

# vscode user
RUN userdel ubuntu && \
    rm -Rf /home/ubuntu && \
    groupadd -g $USER_GID $USERNAME && \
    useradd -s /bin/bash -m -d /home/vscode -u $USER_UID -g $USER_GID $USERNAME && \
    mkdir -p /etc/sudoers.d && \
    echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME && \
    chmod 0440 /etc/sudoers.d/$USERNAME && \
    usermod -aG docker $USERNAME



# entry-point
COPY docker-entrypoint.sh /usr/local/bin/docker-entrypoint
RUN chmod u+rwx,g+rx,o+rx /usr/local/bin/docker-entrypoint

# Create sdk root
RUN mkdir -p ${ANDROID_SDK_ROOT} \
  && chown -R vscode:vscode ${ROOT}

USER vscode

### Android stuff

# Install Android command line tools
WORKDIR /tmp
RUN curl -fSL https://dl.google.com/android/repository/commandlinetools-linux-9477386_latest.zip -o cmdline-tools.zip \
  && unzip cmdline-tools.zip -d /tmp \
  && mkdir -p ${ANDROID_SDK_ROOT}/cmdline-tools/latest \
  && if [ -d /tmp/cmdline-tools ]; then \
       mv /tmp/cmdline-tools/* ${ANDROID_SDK_ROOT}/cmdline-tools/latest/; \
     else \
       mv /tmp/commandlinetools* ${ANDROID_SDK_ROOT}/cmdline-tools/latest/ || true; \
     fi \
  && rm -rf /tmp/cmdline-tools* cmdline-tools.zip

ENV JAVA_HOME=/usr/lib/jvm/java-17-openjdk-amd64

# Install SDK components (platform-tools, build-tools, ndk)
RUN yes | ${ANDROID_SDK_ROOT}/cmdline-tools/latest/bin/sdkmanager --sdk_root=${ANDROID_SDK_ROOT} --licenses || true
RUN ${ANDROID_SDK_ROOT}/cmdline-tools/latest/bin/sdkmanager --sdk_root=${ANDROID_SDK_ROOT} "platform-tools" "platforms;android-35" "build-tools;33.0.2" "ndk;${ANDROID_NDK_VERSION}" "cmake;3.22.1"

# Install Gradle (binary distribution)
RUN mkdir -p ${ROOT}/gradle \
  && curl -fSL https://services.gradle.org/distributions/gradle-${GRADLE_VERSION}-bin.zip -o /tmp/gradle.zip \
  && unzip /tmp/gradle.zip -d ${ROOT}/gradle \
  && rm /tmp/gradle.zip

# install aqtinstall
RUN python3 -m venv ${ROOT}/venv
ENV PATH="${ROOT}/venv/bin:$PATH"
RUN pip install --upgrade pip
RUN pip install aqtinstall

RUN mkdir -p ${ROOT}/Qt

# install Qt for Android
RUN cd ${ROOT}/Qt && aqt install-qt all_os android ${QT_VERSION} android_${QT_ARCH} --autodesktop --modules qt3d qt5compat qtcharts qtconnectivity qtdatavis3d qtgraphs qtgrpc qthttpserver qtimageformats qtlocation qtlottie qtmultimedia qtnetworkauth qtpositioning qtquick3d qtquick3dphysics qtquicktimeline qtremoteobjects qtscxml qtsensors qtserialbus qtserialport qtshadertools qtspeech qtvirtualkeyboard qtwebchannel qtwebsockets qtwebview
ENV QT_ANDROID_DIR=${ROOT}/Qt/${QT_VERSION}/android_${QT_ARCH}
# install host (desktop) Qt needed as QT_HOST_PATH for cross-build
RUN cd ${ROOT}/Qt && aqt install-qt linux desktop ${QT_VERSION} linux_gcc_64 --modules qt3d qt5compat qtcharts qtconnectivity qtdatavis3d qtgraphs qtgrpc qthttpserver qtimageformats qtlocation qtlottie qtmultimedia qtnetworkauth qtpositioning qtquick3d qtquick3dphysics qtquicktimeline qtremoteobjects qtscxml qtsensors qtserialbus qtserialport qtshadertools qtspeech qtvirtualkeyboard qtwebchannel qtwebsockets qtwebview

# boost & openssl for android
RUN git clone https://github.com/microsoft/vcpkg.git ${ROOT}/vcpkg
RUN ${ROOT}/vcpkg/bootstrap-vcpkg.sh && \
  ${ROOT}/vcpkg/vcpkg install \
  boost-system:${VCPKG_ARCH}-android \
  boost-filesystem:${VCPKG_ARCH}-android \
  boost-thread:${VCPKG_ARCH}-android boost-regex:${VCPKG_ARCH}-android \
  boost-chrono:${VCPKG_ARCH}-android \
  boost-date-time:${VCPKG_ARCH}-android boost-serialization:${VCPKG_ARCH}-android \
  boost-asio:${VCPKG_ARCH}-android \
  boost-interprocess:${VCPKG_ARCH}-android \
  boost-iostreams:${VCPKG_ARCH}-android \
  boost-program-options:${VCPKG_ARCH}-android \
  boost-lambda:${VCPKG_ARCH}-android \
  boost-foreach:${VCPKG_ARCH}-android \
  boost-uuid:${VCPKG_ARCH}-android \
  openssl:${VCPKG_ARCH}-android \
  protobuf:x64-linux

### ###

RUN cd ${ROOT} && git clone https://github.com/pokerth/pokerth.git && cd pokerth && git checkout stable

# build protobuf with tls-emulation
RUN mkdir -p ${ROOT}/vcpkg-overlays/protobuf
RUN cp -r ${ROOT}/vcpkg/ports/protobuf/* ${ROOT}/vcpkg-overlays/protobuf/

# Füge TLS-Emulation am Anfang der portfile.cmake ein
RUN sed -i '1i\# Workaround für TLS-Emulation\nset(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--no-warn-execstack")\nset(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-warn-execstack")\n' ${ROOT}/vcpkg-overlays/protobuf/portfile.cmake

# Entferne Protobuf komplett und lösche Build-Cache
RUN ${ROOT}/vcpkg/vcpkg remove protobuf:${VCPKG_ARCH}-android --recurse
RUN rm -rf ${ROOT}/vcpkg/buildtrees/protobuf
RUN rm -rf ${ROOT}/vcpkg/packages/protobuf_${VCPKG_ARCH}-android
RUN rm -rf /tmp/vcpkg-buildtrees/protobuf

# Installiere Protobuf neu mit Overlay
RUN ${ROOT}/vcpkg/vcpkg install protobuf:${VCPKG_ARCH}-android \
    --overlay-ports=${ROOT}/vcpkg-overlays/protobuf \
    --x-buildtrees-root=/tmp/vcpkg-buildtrees \
    --no-binarycaching

# RUN cd ${ROOT}/pokerth && bash docker/android/build_android.sh

# Entrypoint/Command used from docker-compose.yml
# ENTRYPOINT ["/usr/local/bin/docker-entrypoint"]

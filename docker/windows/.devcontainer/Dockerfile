FROM ubuntu:25.10

ENV QT_VERSION=6.9.3
# set target (pokerth_client or pokerth_qml-client)
ENV TARGET=pokerth_client

RUN sed -i 's|http://archive.ubuntu.com/ubuntu/|http://de.archive.ubuntu.com/ubuntu/|' /etc/apt/sources.list.d/ubuntu.sources

# Set up cross-compilation environment
ENV CMAKE_TOOLCHAIN_FILE=${ROOT}/vcpkg/scripts/buildsystems/vcpkg.cmake
ENV VCPKG_TARGET_TRIPLET=x64-mingw-static

# Set other build environment variables
ARG USERNAME=vscode
ARG USER_UID=1000
ARG USER_GID=1000
ARG LANG=de_DE.UTF-8
ARG LANGUAGE=de_DE.UTF-8
ARG LC_ALL=de_DE.UTF-8

ARG ROOT=/opt/pokerth-windows
ENV ROOT=${ROOT}
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies for Windows cross-compilation
RUN apt-get update && apt-get install -y \
    locales \
    mingw-w64 \
    cmake \
    git \
    wget \
    curl \
    zip unzip \
    p7zip-full \
    build-essential \
    pkg-config \
    autoconf \
    automake \
    libtool \
    nsis \
    imagemagick \
    python3 python3-pip python3-venv jq \
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
# COPY docker-entrypoint.sh /usr/local/bin/docker-entrypoint
# RUN chmod u+rwx,g+rx,o+rx /usr/local/bin/docker-entrypoint

RUN mkdir -p ${ROOT} && chown -R vscode:vscode ${ROOT}

USER vscode

# Install vcpkg for Windows dependencies
RUN git clone https://github.com/Microsoft/vcpkg.git ${ROOT}/vcpkg 
RUN ${ROOT}/vcpkg/bootstrap-vcpkg.sh

ENV VCPKG_ROOT=${ROOT}/vcpkg
ENV PATH="${VCPKG_ROOT}:${PATH}"

# install aqtinstall
RUN python3 -m venv ${ROOT}/venv
ENV PATH="${ROOT}/venv/bin:$PATH"
RUN pip install --upgrade pip
RUN pip install aqtinstall

RUN mkdir -p ${ROOT}/Qt

# install Qt for Windows
RUN cd ${ROOT}/Qt && aqt install-qt windows desktop ${QT_VERSION} win64_mingw --autodesktop --modules qt3d qt5compat qtcharts qtconnectivity qtdatavis3d qtgraphs qtgrpc qthttpserver qtimageformats qtlocation qtlottie qtmultimedia qtnetworkauth qtpositioning qtquick3d qtquick3dphysics qtquicktimeline qtremoteobjects qtscxml qtsensors qtserialbus qtserialport qtshadertools qtspeech qtvirtualkeyboard qtwebchannel qtwebsockets qtwebview
ENV QT_WINDOWS_DIR=${ROOT}/Qt/${QT_VERSION}/mingw_64
# install host (desktop) Qt needed as QT_HOST_PATH for cross-build
RUN cd ${ROOT}/Qt && aqt install-qt linux desktop ${QT_VERSION} linux_gcc_64 --modules qt3d qt5compat qtcharts qtconnectivity qtdatavis3d qtgraphs qtgrpc qthttpserver qtimageformats qtlocation qtlottie qtmultimedia qtnetworkauth qtpositioning qtquick3d qtquick3dphysics qtquicktimeline qtremoteobjects qtscxml qtsensors qtserialbus qtserialport qtshadertools qtspeech qtvirtualkeyboard qtwebchannel qtwebsockets qtwebview

# Install common Windows libraries via vcpkg
RUN vcpkg install --triplet=x64-mingw-static \
    boost-system \
    boost-thread \
    boost-program-options \
    boost-filesystem \
    boost-chrono \
    boost-date-time \
    boost-asio \
    boost-interprocess \
    boost-iostreams \
    boost-lambda \
    boost-foreach \
    boost-uuid \
    protobuf \
    openssl
RUN vcpkg install protobuf:x64-linux

RUN cd ${ROOT} && git clone https://github.com/pokerth/pokerth.git && cd pokerth && git checkout testing

# Entrypoint/Command used from docker-compose.yml
# ENTRYPOINT ["/usr/local/bin/docker-entrypoint"]
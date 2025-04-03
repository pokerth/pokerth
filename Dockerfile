FROM ubuntu:plucky
# ubuntu:24.04 aka ubuntu:noble might also work with the following procedure (changing deb-src entries from plucky to noble)

ENV TZ=America/NewYork

USER root

RUN echo '\nTypes: deb-src' >> /etc/apt/sources.list.d/ubuntu.sources
RUN echo 'URIs: http://archive.ubuntu.com/ubuntu/' >> /etc/apt/sources.list.d/ubuntu.sources
RUN echo 'Suites: plucky plucky-updates plucky-backports' >> /etc/apt/sources.list.d/ubuntu.sources
RUN echo 'Components: main universe restricted multiverse' >> /etc/apt/sources.list.d/ubuntu.sources
RUN echo 'Signed-By: /usr/share/keyrings/ubuntu-archive-keyring.gpg' >> /etc/apt/sources.list.d/ubuntu.sources

# build-deps
RUN apt update && DEBIAN_FRONTEND="noninteractive" && apt upgrade -y
RUN apt install -y wget git ca-certificates build-essential libgsasl-dev libtinyxml-dev debhelper libircclient-dev libmysql++-dev \ 
    libwebsocketpp-dev libprotobuf-dev protobuf-compiler libsdl-mixer1.2-dev libcurl4-gnutls-dev libsdl1.2-dev libgcrypt20-dev libsqlite3-dev \
    qt6-base-dev qt6-svg-dev qt6-declarative-dev
# INFO: qt6-declarative-dev (and qt6-svg-dev) not yet needed as not yet using qml, libmysql++-dev only required for official_server build, libircclient-dev is obsolete?

# build & install boost from source
RUN cd /root && wget -O boost-1.87.0-b2-nodocs.tar.xz https://github.com/boostorg/boost/releases/download/boost-1.87.0/boost-1.87.0-b2-nodocs.tar.xz && \
    tar xvf boost-1.87.0-b2-nodocs.tar.xz && cd boost-1.87.0 && \
    ./bootstrap.sh --prefix=/usr && ./b2 stage link=shared && \
    ./b2 install link=shared

# cleanup
RUN apt clean -y && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/* /root/boost*

# the following will compile the client:
#RUN cd /opt && git clone https://github.com/w2vy/pokerth.git && cd pokerth && git checkout dev && \
#    qmake6 CONFIG+="client c++11" QMAKE_CFLAGS_ISYSTEM="" -spec linux-g++ pokerth.pro && make -j$(nproc)

# the following will compile the server:
# RUN cd /opt && git clone https://github.com/w2vy/pokerth.git && cd pokerth && git checkout dev && \
#    qmake6 CONFIG+="official_server c++11" QMAKE_CFLAGS_ISYSTEM="" -spec linux-g++ pokerth.pro && make -j$(nproc)

ENTRYPOINT ["/bin/bash"]

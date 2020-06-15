#!/usr/bin/env bash
set -euo pipefail
set -x

# Error trap to write out line number in case of a build failure!
err_report() {
  echo "Script failed on line $(caller)" >&2
}
trap err_report ERR

REPO_ROOT=$(git rev-parse --show-toplevel)      # Abs path to repository root
QMAKE_CMD="qmake"                                # Default qmake command

OS=""
case "$OSTYPE" in
  linux*) OS=linux ;;
  darwin*) OS=mac ;;
  *) echo "Can't build on this platform ($OSTYPE)"; exit 1 ;;
esac


function install_linux_deps() {
  # Basic dependencies we can get from the OS
  # List dependencies: apt show pokerth | grep ^Depends | tr ',' "\n"
  cat /etc/apt/sources.list
  sudo sed -i '/deb-src/s/^# //' /etc/apt/sources.list && sudo apt update
  sudo apt build-dep -y pokerth
  sudo apt install -y mysql++-dev

  numCpu=$(grep "cpu cores" /proc/cpuinfo | head  -n 1 | awk '{ print $4 }')
  QMAKE_CMD="/usr/lib/x86_64-linux-gnu/qt5/bin/qmake QMAKE_CFLAGS_ISYSTEM="" -spec linux-g++"
}

# Based on https://github.com/pokerth/pokerth/wiki/Building-PokerTH-for-MacOS
function install_mac_deps() {
  
  brew install protobuf boost tinyxml mysql++ openssl qt5

  mkdir -p ~/cache
  pushd ~/cache

  # ~/cache is cached when running on GHA  
  if [[ ! -f deps.cache.ready ]]; then 
    wget --no-clobber ftp://ftp.gnu.org/gnu/gsasl/libgsasl-1.8.1.tar.gz
    rm -rf libgsasl-1.8.1
    tar xf libgsasl-1.8.1.tar.gz
    pushd libgsasl-1.8.1
    ./configure CC="gcc -arch x86_64 -mmacosx-version-min=10.7" --disable-gs2 --disable-gssapi
    make
    popd

    wget --no-clobber "https://www.libsdl.org/release/SDL-1.2.15.dmg"
    
    # Build instructions on the wiki mentions SDL mixer 1.2.15, but that doesn't seem to exists
    wget --no-clobber "https://www.libsdl.org/projects/SDL_mixer/release/SDL_mixer-1.2.12.dmg"
    
    touch deps.cache.ready
  fi

  hdiutil attach SDL-1.2.15.dmg 
  sudo cp -R /Volumes/SDL/SDL.framework /Library/Frameworks/
  hdiutil attach SDL_mixer-1.2.12.dmg
  cp -R /Volumes/SDL_mixer/SDL_mixer.framework/ /Library/Frameworks/

  popd

  numCpu=$(sysctl -n hw.physicalcpu)
  export PATH="/usr/local/opt/qt/bin:$PATH"
}

echo "Installing ${OS} dependencies"
install_${OS}_deps

echo "Building PokerTH"
pushd ${REPO_ROOT}

for i in client official_server; do
    echo Building $i
    $QMAKE_CMD CONFIG+="$i" pokerth.pro
    make -j${numCpu}
done

# Any tests to run?

echo
echo '********************* SUCCESS! *********************'
echo

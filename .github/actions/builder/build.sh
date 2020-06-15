#!/usr/bin/env bash
set -euo pipefail
set -x

# Error trap to write out line number in case of a build failure!
err_report() {
  echo "Script failed on line $(caller)" >&2
}
trap err_report ERR

REPO_ROOT=$(git rev-parse --show-toplevel)      # Abs path to repository root

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

  qmakeCmd="/usr/lib/x86_64-linux-gnu/qt5/bin/qmake QMAKE_CFLAGS_ISYSTEM="" -spec linux-g++"
}

function install_mac_deps() {
  qmakeCmd="qmake"

}

echo "Installing ${OS} dependencies"
install_${OS}_deps

echo "Building PokerTH"
pushd ${REPO_ROOT}

for i in client official_server; do
    echo Building $i
    $qmakeCmd CONFIG+="$i" pokerth.pro
    make -j 4
done

# Any tests to run?

echo
echo '********************* SUCCESS! *********************'
echo

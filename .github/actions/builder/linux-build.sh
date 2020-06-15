#!/usr/bin/env bash
set -euo pipefail
set -x

# Error trap to write out line number in case of a build failure!
err_report() {
  echo "Script failed on line $(caller)" >&2
}
trap err_report ERR

SCRIPT_ROOT=$(dirname "$(realpath $0)")         # Directory containg this script
REPO_ROOT=$(git rev-parse --show-toplevel)      # Abs path to repository root

# Basic dependencies we can get from the OS
# List dependencies: apt show pokerth | grep ^Depends | tr ',' "\n"
echo "Installing dependencies"
cat /etc/apt/sources.list 
sudo sed -i '/deb-src/s/^# //' /etc/apt/sources.list && sudo apt update
sudo apt build-dep -y pokerth
sudo apt install -y mysql++-dev

echo "Building PokerTH"
qtchooser -l
qtchooser -print-env
pushd ${REPO_ROOT}
qmakeCmd="qmake QMAKE_CFLAGS_ISYSTEM="" -spec linux-g++"

for i in client official_server; do
    echo Building $i
    $qmakeCmd CONFIG+="$i" pokerth.pro
    make -j 4
done

# Any tests to run?

echo
echo '********************* SUCCESS! *********************'
echo

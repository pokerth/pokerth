#!/usr/bin/env bash
set -euo pipefail
set -x

# Fetch deps and build PokerTH on linux.
# Usage:
#    ./linux-build.sh
# or WORK_DIR=/some/path/to/clean/workspace ./linux-build.sh

# There's a bit of a overhead to help with caching when running on Github Actions


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
sudo apt build-dep -y pokerth
sudo apt install -y mysql++-dev

echo "Building PokerTH"
pushd ${REPO_ROOT}
rm Makefile*     # delete Makefiles generated by qmake
qmakeCmd="qmake INCLUDEPATH+=/usr INCLUDEPATH+=/usr/include/x86_64-linux-gnu/qt5 QMAKE_CFLAGS_ISYSTEM= -spec linux-g++"

for i in client official_server; do
    echo Building $i
    $qmakeCmd CONFIG+="$i" pokerth.pro
    make -j 4
done

# Any tests to run?

echo
echo '********************* SUCCESS! *********************'
echo

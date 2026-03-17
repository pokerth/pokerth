#!/bin/bash
cd "$(dirname "$0")"
export LANG=en_US.UTF-8
export LC_ALL=en_US.UTF-8
export QT_QPA_PLATFORM=windows
export WINEDEBUG=-all
wine pokerth_client.exe "$@"

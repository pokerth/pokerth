#!/bin/bash
# This script is only really used if the container is run
# from something like Docker run. It's designed to setup some
# things vscode would otherwise do
if [[ $(id -u) = "0" ]]; then
    exec su -l vscode
else
    echo "Unable to switch to vscode user, starting a shell."
    exec /bin/bash
fi

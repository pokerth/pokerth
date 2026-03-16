#!/bin/bash
# PulseAudio: snapd bind-mounts the host pulse socket into
# $XDG_RUNTIME_DIR/pulse/ when audio-playback is connected.
# If that path doesn't exist (e.g. PipeWire-Pulse), fall back
# to the real user runtime directory.
if [ -z "$PULSE_SERVER" ] && [ -n "$XDG_RUNTIME_DIR" ]; then
    if [ ! -S "$XDG_RUNTIME_DIR/pulse/native" ]; then
        REAL_RUNTIME="$(dirname "$XDG_RUNTIME_DIR")"
        if [ -S "$REAL_RUNTIME/pulse/native" ]; then
            export PULSE_SERVER="unix:$REAL_RUNTIME/pulse/native"
        fi
    fi
fi
exec "$@"

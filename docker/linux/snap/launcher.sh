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

# Wayland: the compositor socket lives in the real runtime dir, not in the
# snap's $XDG_RUNTIME_DIR ("Failed to create wl_display"). Qt accepts an
# absolute WAYLAND_DISPLAY, and the wayland plug permits the socket access.
case "$WAYLAND_DISPLAY" in
    ""|/*) ;;
    *)
        if [ -n "$XDG_RUNTIME_DIR" ] && [ ! -S "$XDG_RUNTIME_DIR/$WAYLAND_DISPLAY" ]; then
            REAL_RUNTIME="$(dirname "$XDG_RUNTIME_DIR")"
            if [ -S "$REAL_RUNTIME/$WAYLAND_DISPLAY" ]; then
                export WAYLAND_DISPLAY="$REAL_RUNTIME/$WAYLAND_DISPLAY"
            fi
        fi
        ;;
esac
exec "$@"

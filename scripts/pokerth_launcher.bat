@echo off
chcp 65001
set LANG=en_US.UTF-8
set LC_ALL=en_US.UTF-8
set QT_QPA_PLATFORM=windows
set QT_DEBUG_PLUGINS=0
start "" "%~dp0pokerth_client.exe" %*

@echo off
echo PokerTH Win mingw prepare build script version 1.0.
echo Copyright (C) 2010 Lothar May. License: GPL 2 or later
echo BECAUSE THE PROGRAM IS LICENSED FREE OF CHARGE, THERE IS NO WARRANTY
echo FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW.
echo.

SET PATH=%POKERTH_WIN32_LIB_DIR%\qt\bin;%POKERTH_WIN32_LIB_DIR%\mingw32\bin;%PATH%
SET QMAKESPEC=win32-g++
SET QTDIR=%POKERTH_WIN32_LIB_DIR%\qt

if exist Makefile. (
	mingw32-make distclean
)
qmake DEFINES+=BOOST_USE_WINDOWS_H pokerth.pro
mingw32-make release
IF NOT "%ERRORLEVEL%"=="0" exit /b %ERRORLEVEL%
REM Add automatic packaging or copying here.
mingw32-make distclean


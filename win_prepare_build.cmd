@echo off
echo PokerTH Win mingw prepare build script version 1.0.
echo Copyright (C) 2010 Lothar May. License: GPL 2 or later
echo BECAUSE THE PROGRAM IS LICENSED FREE OF CHARGE, THERE IS NO WARRANTY
echo FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW.
echo.

SET PKTH_OldDir=%CD%
SET PKTH_BaseDir=%1
if exist %PKTH_BaseDir%\mingw64\bin\mingw32-make.exe goto mingw64
SET PATH=%PKTH_BaseDir%\qt\bin;%PKTH_BaseDir%\mingw32\bin;%PATH%
SET PKTH_Conf=""
goto postmingw
:mingw64
SET PATH=%PKTH_BaseDir%\qt\bin;%PKTH_BaseDir%\mingw64\bin;%PATH%
SET PKTH_Conf=pkth_win64
:postmingw
SET QMAKESPEC=win32-g++
SET QTDIR=%PKTH_BaseDir%\qt

if exist Makefile. (
mingw32-make distclean
)
qmake "CONFIG+=release %PKTH_Conf%" DEFINES+=BOOST_USE_WINDOWS_H pokerth.pro

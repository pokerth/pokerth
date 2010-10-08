@echo off
echo PokerTH Win32 mingw build script version 1.0.
echo Copyright (C) 2008-2010 Lothar May. License: GPL 2 or later
echo BECAUSE THE PROGRAM IS LICENSED FREE OF CHARGE, THERE IS NO WARRANTY
echo FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW.
echo.

SET PKTH_OldDir=%CD%
SET PKTH_BaseDir=%1
if not defined PKTH_BaseDir goto noDir
if not exist %PKTH_BaseDir% goto dirDoesNotExist
cd /d %PKTH_BaseDir%
echo test > pokerth123_delete_me.txt
if not exist %PKTH_BaseDir%\pokerth123_delete_me.txt goto noWriteAccess
del %PKTH_BaseDir%\pokerth123_delete_me.txt
if exist %PKTH_BaseDir%\pokerth123_delete_me.txt goto noWriteAccess
echo.
echo Using "%PKTH_BaseDir%" as base directory.
echo.
echo This script should be run AFTER pokerth_download_libs.cmd.
echo.
echo It will perform the following steps:
echo - Clean the current build of PokerTH
echo - Update the PokerTH sources from svn
echo - Build PokerTH from the sources
echo.
echo You may run this script again at any time, in case you
echo wish to update your version.
echo.
echo Press Ctrl+c to abort.
echo The process will start if you press any other key.
pause
echo.
echo Checking for an installation of Qt
if not exist %PKTH_BaseDir%\qt\bin\qmake.exe goto noQmake
if not exist %PKTH_BaseDir%\svn\bin\svn.exe goto noSvn
if not exist %PKTH_BaseDir%\pokerth goto otherScriptFirst
if exist %PKTH_BaseDir%\mingw64\bin\mingw32-make.exe goto mingw64
SET PATH=%PKTH_BaseDir%\qt\bin;%PKTH_BaseDir%\mingw32\bin;%PATH%
goto postmingw
:mingw64
SET PATH=%PKTH_BaseDir%\qt\bin;%PKTH_BaseDir%\mingw64\bin;%PATH%
:postmingw
SET QMAKESPEC=win32-g++
SET QTDIR=%PKTH_BaseDir%\qt

cd pokerth
if exist Makefile. (
echo.
echo Cleaning the current build
mingw32-make distclean
)
echo.
echo Running svn update
%PKTH_BaseDir%\svn\bin\svn update
echo.
echo Building PokerTH
qmake CONFIG+=release DEFINES+=BOOST_USE_WINDOWS_H pokerth.pro
mingw32-make release
move release\pokerth.exe .\pokerth.exe
move release\bin\pokerth_server.exe .\pokerth_server.exe
echo.
echo Done compiling PokerTH. Have a lot of fun!
goto end

:noSvn
echo Could not find svn.exe
goto otherScriptFirst

:noQmake
echo Could not find qmake.exe
goto otherScriptFirst

:otherScriptFirst
echo Make sure to run pokerth_download_libs.cmd BEFORE this script.
echo Also make sure that you use the same base directory for both scripts.
goto end

:noWriteAccess
echo Could not write files to directory "%PKTH_BaseDir%".
echo Make sure the directory name is correct (do not use spaces or non-ASCII).
goto end

:dirDoesNotExist
echo The directory "%PKTH_BaseDir%" does not exist.
echo Please provide a valid directory.
goto end

:noDir
echo Usage: compile_pokerth_win32.cmd ^<BaseDir^>.
echo Make sure that the base directory exists and is writable.
echo Use a FULL PATH without trailing slash for the base directory.
goto end

:end
cd /d %PKTH_OldDir%

@echo off
echo PokerTH Win32 mingw download script version 1.0.
echo Copyright (C) 2008-2010 Lothar May. License: GPL 2 or later
echo BECAUSE THE PROGRAM IS LICENSED FREE OF CHARGE, THERE IS NO WARRANTY
echo FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW.
echo.
echo This script uses and downloads other software.
echo Please mind their licenses.
echo.
echo Press enter to confirm, Ctrl+c to abort. Further information will follow.
pause
echo.

SET SOURCEFORGE_MIRROR=heanet.dl
SET PKTH_OldDir=%CD%
SET PKTH_BaseDir=%1
if not defined PKTH_BaseDir goto noDir
if not exist %PKTH_BaseDir% goto dirDoesNotExist
cd /d %PKTH_BaseDir%
echo test > pokerth123_delete_me.txt
if not exist %PKTH_BaseDir%\pokerth123_delete_me.txt goto noWriteAccess
del %PKTH_BaseDir%\pokerth123_delete_me.txt
if exist %PKTH_BaseDir%\pokerth123_delete_me.txt goto noWriteAccess
if not exist %PKTH_OldDir%\third_party_apps\wget.exe goto noTools
if not exist %PKTH_OldDir%\third_party_apps\7za.exe goto noTools

echo Using "%PKTH_BaseDir%" as base directory.
echo.
echo This script will download and install the prerequisites for building PokerTH
echo - mingw-w64 (sezero's 4.5.2 prerelease with target Win32), mingw-utils
echo - Qt - will be compiled from the sources
echo - GnuTLS - using the binary release
echo - gsasl - using the binary release
echo - zlib and libcurl - will be compiled from the sources
echo - boost - will be compiled from the sources
echo - SDL, SDL_mixer - using binary releases
echo - subversion
echo.
echo Downloading all and building some of these libraries might require up to
echo 5 (five) GB hard disc space. Roughly 350 MB will be downloaded.
echo ONLY continue, if you are SURE that you have enough disc space available.
echo.
echo Files within the base directory might be OVERWRITTEN WITHOUT WARNING.
echo Make sure that you have FULL RIGHTS in the base directory!
echo.
echo You need to run this script only once (if successful).
echo It might take SEVERAL HOURS.
echo.
echo If unsure, press Ctrl+c and abort.
echo The process will start if you press any other key.
pause
echo.
echo Downloading mingw packages
%PKTH_OldDir%\third_party_apps\wget http://%SOURCEFORGE_MIRROR%.sourceforge.net/sourceforge/mingw-w64/mingw-w32-bin_i686-mingw_20101002_4.5_sezero.zip
%PKTH_OldDir%\third_party_apps\wget http://%SOURCEFORGE_MIRROR%.sourceforge.net/sourceforge/mingw/mingw-utils-0.3.tar.gz
if not errorlevel 0 goto downloadFailed
echo.
echo Unpacking mingw packages
%PKTH_OldDir%\third_party_apps\7za x -y mingw-w32-bin_i686-mingw_20101002_4.5_sezero.zip
%PKTH_OldDir%\third_party_apps\7za x -y mingw-utils-0.3.tar.gz
%PKTH_OldDir%\third_party_apps\7za x -y -omingw32 mingw-utils-0.3.tar
del mingw-utils-0.3.tar
cd /d %PKTH_BaseDir%
echo.
echo Testing g++
set PATH=%PKTH_BaseDir%\mingw32\bin;%PKTH_OldDir%\third_party_apps;%PATH%
g++ %PKTH_OldDir%\helper_src\pkth_test.cpp -o pkth_test
if not errorlevel 0 goto gppFailure
pkth_test
if not errorlevel 42 goto gppFailure
del pkth_test.exe
echo Successful
cd /d %PKTH_BaseDir%
if not exist %PKTH_BaseDir%\qt. (
echo.
echo Downloading Qt
%PKTH_OldDir%\third_party_apps\wget http://get.qt.nokia.com/qt/source/qt-everywhere-opensource-src-4.7.0.zip
if not errorlevel 0 goto downloadFailed
echo.
echo Unpacking Qt
%PKTH_OldDir%\third_party_apps\7za x -y qt-everywhere-opensource-src-4.7.0.zip
REM Wait 5 seconds for the file cache, else ren might fail.
@ping 127.0.0.1 -n 5 -w 1000 > nul
ren qt-everywhere-opensource-src-4.7.0 qt
)
if not exist %PKTH_BaseDir%\qt goto qtFailure
echo.
echo Compiling Qt
SET PATH=%PKTH_BaseDir%\qt\bin;%PATH%
SET QMAKESPEC=win32-g++
SET QTDIR=%PKTH_BaseDir%\qt
cd qt
configure -static -fast -no-qt3support -qt-sql-sqlite -no-dbus -no-opengl -no-openssl -no-phonon -no-webkit
qmake projects.pro -o Makefile -spec win32-g++
mingw32-make sub-src
echo.
echo Done compiling Qt
cd /d %PKTH_BaseDir%
if not exist %PKTH_BaseDir%\zlib. (
echo.
echo Downloading zlib
%PKTH_OldDir%\third_party_apps\wget http://%SOURCEFORGE_MIRROR%.sourceforge.net/sourceforge/libpng/zlib-1.2.5.tar.gz
if not errorlevel 0 goto downloadFailed
echo.
echo Unpacking zlib
%PKTH_OldDir%\third_party_apps\7za x -y zlib-1.2.5.tar.gz
%PKTH_OldDir%\third_party_apps\7za x -y zlib-1.2.5.tar
del zlib-1.2.5.tar
REM Wait 5 seconds for the file cache, else ren might fail.
@ping 127.0.0.1 -n 5 -w 1000 > nul
ren zlib-1.2.5 zlib
)
echo.
echo Compiling zlib
cd zlib
mingw32-make -fwin32\makefile.gcc
echo.
echo Done compiling zlib
SET ZLIB_PATH=%PKTH_BaseDir%\zlib
SET ZLIB_LIBPATH=%PKTH_BaseDir%\zlib
SET ZLIB_SOURCE=%PKTH_BaseDir%\zlib
SET ZLIB_BINARY=zlib
cd /d %PKTH_BaseDir%
if not exist %PKTH_BaseDir%\GnuTLS. (
echo.
echo Downloading GnuTLS
%PKTH_OldDir%\third_party_apps\wget http://josefsson.org/gnutls4win/gnutls-2.8.6.zip
mkdir GnuTLS
%PKTH_OldDir%\third_party_apps\7za x -y -oGnuTLS gnutls-2.8.6.zip
REM Wait 5 seconds for the file cache.
@ping 127.0.0.1 -n 5 -w 1000 > nul
REM Remove files for dynamic linking, we do not want to accidently use them
mkdir GnuTLS\lib\unused
move GnuTLS\lib\*.dll.a GnuTLS\lib\unused
move GnuTLS\lib\*.la GnuTLS\lib\unused
)
if not exist %PKTH_BaseDir%\gsasl. (
echo.
echo Downloading gsasl
%PKTH_OldDir%\third_party_apps\wget http://josefsson.org/gnutls4win/gsasl-1.4.4.zip
mkdir gsasl
%PKTH_OldDir%\third_party_apps\7za x -y -ogsasl gsasl-1.4.4.zip
REM Wait 5 seconds for the file cache.
@ping 127.0.0.1 -n 5 -w 1000 > nul
REM Remove files for dynamic linking, we do not want to accidently use them
mkdir gsasl\lib\unused
move gsasl\lib\*.dll.a gsasl\lib\unused
move gsasl\lib\*.la gsasl\lib\unused
)
cd /d %PKTH_BaseDir%
if not exist %PKTH_BaseDir%\curl. (
echo.
echo Downloading curl
%PKTH_OldDir%\third_party_apps\wget http://curl.haxx.se/download/curl-7.21.1.tar.bz2
if not errorlevel 0 goto downloadFailed
echo.
echo Unpacking curl
%PKTH_OldDir%\third_party_apps\7za x -y curl-7.21.1.tar.bz2
%PKTH_OldDir%\third_party_apps\7za x -y curl-7.21.1.tar
del curl-7.21.1.tar
REM Wait 5 seconds for the file cache, else ren might fail.
@ping 127.0.0.1 -n 5 -w 1000 > nul
ren curl-7.21.1 curl
)
echo.
echo Compiling curl
cd curl
mingw32-make mingw32-zlib
echo.
echo Done compiling curl
cd /d %PKTH_BaseDir%
if not exist %PKTH_BaseDir%\boost. (
echo.
echo Downloading boost and bjam
%PKTH_OldDir%\third_party_apps\wget http://%SOURCEFORGE_MIRROR%.sourceforge.net/sourceforge/boost/boost_1_44_0.7z http://%SOURCEFORGE_MIRROR%.sourceforge.net/sourceforge/boost/boost-jam-3.1.17-1-ntx86.zip
if not errorlevel 0 goto downloadFailed
echo.
echo Unpacking boost and bjam
%PKTH_OldDir%\third_party_apps\7za x -y boost_1_44_0.7z
%PKTH_OldDir%\third_party_apps\7za x -y boost-jam-3.1.17-1-ntx86.zip
REM Wait 5 seconds for the file cache, else ren might fail.
@ping 127.0.0.1 -n 5 -w 1000 > nul
move boost-jam-3.1.17-1-ntx86\bjam.exe mingw32\bin\bjam.exe
move boost-jam-3.1.17-1-ntx86\LICENSE_1_0.txt mingw32\bin\bjam_LICENSE_1_0.txt
rd boost-jam-3.1.17-1-ntx86
ren boost_1_44_0 boost
)
echo.
echo Compiling boost
cd boost
bjam link=shared --build-dir=%PKTH_BaseDir%\pkth_boost_delete_me_after_build --toolset=gcc define=BOOST_USE_WINDOWS_H stage
echo.
echo Done compiling boost
cd /d %PKTH_BaseDir%
if not exist %PKTH_BaseDir%\SDL. (
echo.
echo Downloading SDL precompiled
%PKTH_OldDir%\third_party_apps\wget http://www.libsdl.org/release/SDL-devel-1.2.14-mingw32.tar.gz
if not errorlevel 0 goto downloadFailed
echo.
echo Unpacking SDL
%PKTH_OldDir%\third_party_apps\7za x -y SDL-devel-1.2.14-mingw32.tar.gz
%PKTH_OldDir%\third_party_apps\7za x -y SDL-devel-1.2.14-mingw32.tar
del SDL-devel-1.2.14-mingw32.tar
REM Wait 5 seconds for the file cache, else ren might fail.
@ping 127.0.0.1 -n 5 -w 1000 > nul
ren SDL-1.2.14 SDL
)
if not exist %PKTH_BaseDir%\SDL_mixer. (
echo.
echo Downloading SDL_mixer precompiled
%PKTH_OldDir%\third_party_apps\wget http://www.libsdl.org/projects/SDL_mixer/release/SDL_mixer-1.2.11.zip http://www.libsdl.org/projects/SDL_mixer/release/SDL_mixer-1.2.11-win32.zip
if not errorlevel 0 goto downloadFailed
echo.
echo Unpacking SDL_mixer
%PKTH_OldDir%\third_party_apps\7za x -y SDL_mixer-1.2.11.zip
REM Wait 5 seconds for the file cache, else ren might fail.
@ping 127.0.0.1 -n 5 -w 1000 > nul
ren SDL_mixer-1.2.11 SDL_mixer
mkdir SDL_mixer\bin
mkdir SDL_mixer\lib
mkdir SDL_mixer\include
mkdir SDL_mixer\include\SDL
%PKTH_OldDir%\third_party_apps\7za x -y -oSDL_mixer\bin SDL_mixer-1.2.11-win32.zip
cd SDL_mixer
copy SDL_mixer.h include\SDL
echo.
echo Creating import library
cd bin
pexports SDL_mixer.dll > SDL_mixer.def
dlltool -d SDL_mixer.def -D SDL_mixer.dll -l libsdl_mixer.a
move libsdl_mixer.a ..\lib
cd /d %PKTH_BaseDir%
)
if not exist %PKTH_BaseDir%\svn. (
echo.
echo Downloading subversion
%PKTH_OldDir%\third_party_apps\wget http://subversion.tigris.org/files/documents/15/47914/svn-win32-1.6.6.zip
if not errorlevel 0 goto downloadFailed
echo.
echo Unpacking subversion
%PKTH_OldDir%\third_party_apps\7za x -y svn-win32-1.6.6.zip
REM Wait 5 seconds for the file cache, else ren might fail.
@ping 127.0.0.1 -n 5 -w 1000 > nul
ren svn-win32-1.6.6 svn
)
if not exist %PKTH_BaseDir%\svn\bin\svn.exe goto svnFailure
if not exist %PKTH_BaseDir%\pokerth. (
echo.
echo Checking out latest PokerTH sources from svn
%PKTH_BaseDir%\svn\bin\svn co http://pokerth.svn.sourceforge.net/svnroot/pokerth/trunk/pokerth
echo.
echo Done checking out latest PokerTH sources
)
echo.
echo Copying DLLs
copy /Y %PKTH_BaseDir%\SDL\bin\SDL.dll %PKTH_BaseDir%\pokerth\
copy /Y %PKTH_BaseDir%\SDL_mixer\bin\SDL_mixer.dll %PKTH_BaseDir%\pokerth\
copy /Y "%PKTH_BaseDir%\mingw32\bin\libgcc_s_sjlj-1.dll" %PKTH_BaseDir%\pokerth\
copy /Y "%PKTH_BaseDir%\mingw32\bin\libstdc++-6.dll" %PKTH_BaseDir%\pokerth\
copy /Y %PKTH_BaseDir%\boost\stage\lib\libboost_filesystem-mgw45-mt-1_44.dll %PKTH_BaseDir%\pokerth\
copy /Y %PKTH_BaseDir%\boost\stage\lib\libboost_iostreams-mgw45-mt-1_44.dll %PKTH_BaseDir%\pokerth\
copy /Y %PKTH_BaseDir%\boost\stage\lib\libboost_system-mgw45-mt-1_44.dll %PKTH_BaseDir%\pokerth\
copy /Y %PKTH_BaseDir%\boost\stage\lib\libboost_thread-mgw45-mt-1_44.dll %PKTH_BaseDir%\pokerth\
copy /Y %PKTH_BaseDir%\boost\stage\lib\libboost_zlib-mgw45-mt-1_44.dll %PKTH_BaseDir%\pokerth\
echo.
echo This script has finished. You may run the pokerth_build.cmd now.
goto pkth_end

:svnFailure
echo Could not find svn.exe
echo subversion installation failed.
goto pkth_end

:perlFailure
echo Could not find perl.exe
echo perl installation failed.
goto pkth_end

:qtFailure
echo Could not unzip qt and/or rename qt folder.
echo Check that you have full rights.
goto pkth_end

:gppFailure
echo Could not compile/run test.cpp.
echo mingw installation failed.
goto pkth_end

:downloadFailed
echo Download failed.
goto pkth_end

:noWriteAccess
echo Could not write files to directory "%PKTH_BaseDir%".
echo Make sure the directory name is correct (do not use spaces or non-ASCII).
goto pkth_end

:dirDoesNotExist
echo The directory "%PKTH_BaseDir%" does not exist.
echo Please provide a valid directory.
goto pkth_end

:noDir
echo Usage: compile_pokerth_win32.cmd ^<BaseDir^>.
echo Make sure that the base directory exists and is writable.
echo Use a FULL PATH without trailing slash for the base directory.
goto pkth_end

:noTools
echo Tools are missing. Please download and extract the full package
echo before running this script.
goto pkth_end

:pkth_end
cd /d %PKTH_OldDir%

These are some tools to help compiling the development version of PokerTH on Windows
using the mingw g++ compiler.

This script has been tested on Windows 7 x64. When updating this script from a
previous version, delete the base pokerth directory first, and start from the scratch.

Compiling all libraries and PokerTH itself could take SEVERAL HOURS, and uses up a lot of disc space.
You need up to 5 GB of free disc space to perform a full build.

Direct internet access (without proxy) is required!
Please note that there is a known conflict if cygwin is installed on the system.
There is no workaround available except uninstalling cygwin and removing its environment variables.

Please follow these instructions:

1. Create a base directory for all PokerTH related projects.
   This should be an empty directory. There should be no spaces or non-ASCII characters in the directory name.
   Example: C:\pokerth_root
   The directory must be fully writable.

2. Open a command line window (Start -> Run -> "cmd.exe")

3. Change directory to where you have unpacked the package which contains this readme.
   There should be no spaces or non-ASCII characters in the directory name.
   Example: "cd /d C:\Tools\pokerth_win32_build"

4. Run the script which downloads and builds all required libraries, and provide
   the base directory as parameter. Interaction is required at the beginning after
   downloading Qt.
   Example: "pokerth_download_libs.cmd C:\pokerth_root"

   In order to build the x64 version, run "pokerth_download_libs64.cmd c:\pokerth_root".

   This could take several hours. If you need to interrupt the script, you can do so
   by pressing Ctrl+c. After interrupting, you may
   - start from the scratch (delete everything within the base directory)
   - if you know what happened when you interrupted the script, you might delete
     the folder from the current step which was running (e.g. the "boost"-folder
     or whichever) and restart the script.
   Please keep the command line open for step 6.

5. You may delete the directory "pkth_boost_delete_me_after_build" within the base
   directory, but you should keep all other directories for step 6.

6. If the script was successful, you can finally build PokerTH. This takes only a few minutes.
   Run the script which updates and builds the current PokerTH svn version.
   Example: "pokerth_build.cmd C:\pokerth_root"
   You will find the executable file in C:\pokerth_root\pokerth.

7. At a later time, if you wish to update your development version of PokerTH, you only
   need to repeat step 6.


/***************************************************************************
 *   Copyright (C) 2008-2010 by Lothar May                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

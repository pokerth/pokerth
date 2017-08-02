#!/bin/bash
# Author: Lothar May (originally from Erhard List)

# ATTENTION: this is for MAC only!
#
# This script integrates the necessary frameworks into the binary.
#
# The integration of Qt is optional (commandlineswitch --without-qt) as you can save 30MB 
# of binary-size if you leave Qt out.
# (see http://trolltech.com/developer/downloads/qt/mac)

QT_FW_PATH="/Users/$USER/Qt/5.9/clang_64/lib"
QT_PLUGIN_PATH="/Users/$USER/Qt/5.9/clang_64/plugins"
SDL_FW_PATH="/Library/Frameworks"
APPLICATION="./pokerth.app"
BINARY="$APPLICATION/Contents/MacOs/pokerth"
RESOURCES="$APPLICATION/Contents/Resources"

# strip binary
strip $BINARY

cp -R ./data $RESOURCES/
find $RESOURCES/data -name ".svn" | xargs rm -Rf
# create framework-path
BINARY_FW_PATH="$APPLICATION/Contents/Frameworks"
mkdir $BINARY_FW_PATH
BINARY_PLUGIN_PATH="$APPLICATION/Contents/plugins"
mkdir -p $BINARY_PLUGIN_PATH/imageformats
mkdir -p $BINARY_PLUGIN_PATH/sqldrivers
mkdir -p $BINARY_PLUGIN_PATH/platforms

# integrate SDL-frameworks into binary
cp -R $SDL_FW_PATH/SDL.framework $BINARY_FW_PATH
cp -R $SDL_FW_PATH/SDL_mixer.framework $BINARY_FW_PATH

# integrate Qt-frameworks into binary

if [ "$1" != "--without-qt" ] ; then
        cp $QT_PLUGIN_PATH/imageformats/libqgif.dylib $BINARY_PLUGIN_PATH/imageformats
        cp $QT_PLUGIN_PATH/imageformats/libqjpeg.dylib $BINARY_PLUGIN_PATH/imageformats
        cp $QT_PLUGIN_PATH/sqldrivers/libqsqlite.dylib $BINARY_PLUGIN_PATH/sqldrivers
        cp $QT_PLUGIN_PATH/platforms/libqcocoa.dylib $BINARY_PLUGIN_PATH/platforms
        cp -R $QT_FW_PATH/QtCore.framework $BINARY_FW_PATH
        cp -R $QT_FW_PATH/QtGui.framework $BINARY_FW_PATH
        cp -R $QT_FW_PATH/QtWidgets.framework $BINARY_FW_PATH
        cp -R $QT_FW_PATH/QtSql.framework $BINARY_FW_PATH
        cp -R $QT_FW_PATH/QtNetwork.framework $BINARY_FW_PATH
        cp -R $QT_FW_PATH/QtPrintSupport.framework $BINARY_FW_PATH
        # remove debug versions
        rm -f $APPLICATION/Contents/Frameworks/QtCore.framework/QtCore_debug
        rm -f $APPLICATION/Contents/Frameworks/QtCore.framework/QtCore_debug.prl
        rm -f $APPLICATION/Contents/Frameworks/QtCore.framework/Versions/5/QtCore_debug
        rm -f $APPLICATION/Contents/Frameworks/QtGui.framework/QtGui_debug
        rm -f $APPLICATION/Contents/Frameworks/QtGui.framework/QtGui_debug.prl
        rm -f $APPLICATION/Contents/Frameworks/QtGui.framework/Versions/5/QtGui_debug
        rm -f $APPLICATION/Contents/Frameworks/QtWidgets.framework/QtWidgets_debug
        rm -f $APPLICATION/Contents/Frameworks/QtWidgets.framework/QtWidgets_debug.prl
        rm -f $APPLICATION/Contents/Frameworks/QtWidgets.framework/Versions/5/QtWidgets_debug
        rm -f $APPLICATION/Contents/Frameworks/QtSql.framework/QtSql_debug
        rm -f $APPLICATION/Contents/Frameworks/QtSql.framework/QtSql_debug.prl
        rm -f $APPLICATION/Contents/Frameworks/QtSql.framework/Versions/5/QtSql_debug
        rm -f $APPLICATION/Contents/Frameworks/QtNetwork.framework/QtNetwork_debug
        rm -f $APPLICATION/Contents/Frameworks/QtNetwork.framework/QtNetwork_debug.prl
        rm -f $APPLICATION/Contents/Frameworks/QtNetwork.framework/Versions/5/QtNetwork_debug
        rm -f $APPLICATION/Contents/Frameworks/QtPrintSupport.framework/QtPrintSupport_debug
        rm -f $APPLICATION/Contents/Frameworks/QtPrintSupport.framework/QtPrintSupport_debug.prl
        rm -f $APPLICATION/Contents/Frameworks/QtPrintSupport.framework/Versions/5/QtPrintSupport_debug

        # redirect binary to use integrated frameworks
        QTCORE="QtCore.framework/Versions/5/QtCore"
        QTGUI="QtGui.framework/Versions/5/QtGui"
        QTWIDGETS="QtWidgets.framework/Versions/5/QtWidgets"
        QTSQL="QtSql.framework/Versions/5/QtSql"
        QTNETWORK="QtNetwork.framework/Versions/5/QtNetwork"
        QTPRINT="QtPrintSupport.framework/Versions/5/QtPrintSupport"
        install_name_tool -id @executable_path/../Frameworks/$QTCORE $BINARY_FW_PATH/$QTCORE
        install_name_tool -id @executable_path/../Frameworks/$QTGUI $BINARY_FW_PATH/$QTGUI
        install_name_tool -id @executable_path/../Frameworks/$QTWIDGETS $BINARY_FW_PATH/$QTWIDGETS
        install_name_tool -id @executable_path/../Frameworks/$QTSQL $BINARY_FW_PATH/$QTSQL
        install_name_tool -id @executable_path/../Frameworks/$QTNETWORK $BINARY_FW_PATH/$QTNETWORK
        install_name_tool -id @executable_path/../Frameworks/$QTPRINT $BINARY_FW_PATH/$QTPRINT
        QTCORE_LINK=$(otool -L $BINARY | grep QtCore | cut -d"(" -f1 | cut -f2)
        QTGUI_LINK=$(otool -L $BINARY | grep QtGui | cut -d"(" -f1 | cut -f2)
        QTWIDGETS_LINK=$(otool -L $BINARY | grep QtWidgets | cut -d"(" -f1 | cut -f2)
        QTSQL_LINK=$(otool -L $BINARY | grep QtSql | cut -d"(" -f1 | cut -f2)
        QTNETWORK_LINK=$(otool -L $BINARY | grep QtNetwork | cut -d"(" -f1 | cut -f2)
        QTPRINT_LINK=$(otool -L $BINARY_PLUGIN_PATH/platforms/libqcocoa.dylib | grep QtPrintSupport | cut -d"(" -f1 | cut -f2)
        install_name_tool -change $QTCORE_LINK @executable_path/../Frameworks/$QTCORE $BINARY
        install_name_tool -change $QTGUI_LINK @executable_path/../Frameworks/$QTGUI $BINARY
        install_name_tool -change $QTWIDGETS_LINK @executable_path/../Frameworks/$QTWIDGETS $BINARY
        install_name_tool -change $QTSQL_LINK @executable_path/../Frameworks/$QTSQL $BINARY
        install_name_tool -change $QTNETWORK_LINK @executable_path/../Frameworks/$QTNETWORK $BINARY
        install_name_tool -change $QTCORE_LINK @executable_path/../Frameworks/$QTCORE $BINARY_PLUGIN_PATH/imageformats/libqgif.dylib
        install_name_tool -change $QTGUI_LINK @executable_path/../Frameworks/$QTGUI $BINARY_PLUGIN_PATH/imageformats/libqgif.dylib
        install_name_tool -change $QTWIDGETS_LINK @executable_path/../Frameworks/$QTWIDGETS $BINARY_PLUGIN_PATH/imageformats/libqgif.dylib
        install_name_tool -change $QTCORE_LINK @executable_path/../Frameworks/$QTCORE $BINARY_PLUGIN_PATH/imageformats/libqjpeg.dylib
        install_name_tool -change $QTGUI_LINK @executable_path/../Frameworks/$QTGUI $BINARY_PLUGIN_PATH/imageformats/libqjpeg.dylib
        install_name_tool -change $QTWIDGETS_LINK @executable_path/../Frameworks/$QTWIDGETS $BINARY_PLUGIN_PATH/imageformats/libqjpeg.dylib
        install_name_tool -change $QTCORE_LINK @executable_path/../Frameworks/$QTCORE $BINARY_PLUGIN_PATH/sqldrivers/libqsqlite.dylib
        install_name_tool -change $QTSQL_LINK @executable_path/../Frameworks/$QTSQL $BINARY_PLUGIN_PATH/sqldrivers/libqsqlite.dylib
        install_name_tool -change $QTCORE_LINK @executable_path/../Frameworks/$QTCORE $BINARY_PLUGIN_PATH/platforms/libqcocoa.dylib
        install_name_tool -change $QTGUI_LINK @executable_path/../Frameworks/$QTGUI $BINARY_PLUGIN_PATH/platforms/libqcocoa.dylib
        install_name_tool -change $QTWIDGETS_LINK @executable_path/../Frameworks/$QTWIDGETS $BINARY_PLUGIN_PATH/platforms/libqcocoa.dylib
        install_name_tool -change $QTPRINT_LINK @executable_path/../Frameworks/$QTPRINT $BINARY_PLUGIN_PATH/platforms/libqcocoa.dylib

        QTCORE_LINK=$(otool -L $BINARY_FW_PATH/$QTGUI | grep QtCore | head -1 | cut -d"(" -f1 | cut -f2)
        install_name_tool -change $QTCORE_LINK @executable_path/../Frameworks/$QTCORE $BINARY_FW_PATH/$QTGUI
        install_name_tool -change $QTCORE_LINK @executable_path/../Frameworks/$QTCORE $BINARY_FW_PATH/$QTWIDGETS
        install_name_tool -change $QTCORE_LINK @executable_path/../Frameworks/$QTCORE $BINARY_FW_PATH/$QTSQL
        install_name_tool -change $QTCORE_LINK @executable_path/../Frameworks/$QTCORE $BINARY_FW_PATH/$QTNETWORK
        install_name_tool -change $QTCORE_LINK @executable_path/../Frameworks/$QTCORE $BINARY_FW_PATH/$QTPRINT

        QTGUI_LINK=$(otool -L $BINARY_FW_PATH/$QTWIDGETS | grep QtGui | cut -d"(" -f1 | cut -f2)
        install_name_tool -change $QTGUI_LINK @executable_path/../Frameworks/$QTGUI $BINARY_FW_PATH/$QTWIDGETS
        install_name_tool -change $QTGUI_LINK @executable_path/../Frameworks/$QTGUI $BINARY_FW_PATH/$QTPRINT

        QTWIDGETS_LINK=$(otool -L $BINARY_FW_PATH/$QTPRINT | grep QtWidgets | cut -d"(" -f1 | cut -f2)
        install_name_tool -change $QTWIDGETS_LINK @executable_path/../Frameworks/$QTWIDGETS $BINARY_FW_PATH/$QTPRINT
fi

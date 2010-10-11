#!/bin/bash
# Author: Lothar May (originally from Erhard List)

# ATTENTION: this is for MAC only!
#
# This script integrates the necessary frameworks into the binary.
#
# The integration of Qt is optional (commandlineswitch --without-qt) as you can save 30MB 
# of binary-size if you leave Qt out.
# (see http://trolltech.com/developer/downloads/qt/mac)

QT_FW_PATH="/Library/Frameworks"
QT_PLUGIN_PATH="/Developer/Applications/Qt/plugins"
SDL_FW_PATH="/Library/Frameworks"
APPLICATION="./pokerth.app"
BINARY="$APPLICATION/Contents/MacOs/pokerth"
RESOURCES="$APPLICATION/Contents/Resources/"

# strip binary
strip $BINARY

cp -R ./data $RESOURCES
# create framework-path
BINARY_FW_PATH="$APPLICATION/Contents/Frameworks"
mkdir $BINARY_FW_PATH
BINARY_PLUGIN_PATH="$APPLICATION/Contents/plugins"
mkdir -p $BINARY_PLUGIN_PATH/imageformats
mkdir -p $BINARY_PLUGIN_PATH/sqldrivers

# integrate SDL-frameworks into binary
cp -R $SDL_FW_PATH/SDL.framework $BINARY_FW_PATH
cp -R $SDL_FW_PATH/SDL_mixer.framework $BINARY_FW_PATH

# integrate Qt-frameworks into binary

if [ "$1" != "--without-qt" ] ; then
        cp $QT_PLUGIN_PATH/imageformats/libqgif.dylib $BINARY_PLUGIN_PATH/imageformats
        cp $QT_PLUGIN_PATH/imageformats/libqjpeg.dylib $BINARY_PLUGIN_PATH/imageformats
        cp $QT_PLUGIN_PATH/sqldrivers/libqsqlite.dylib $BINARY_PLUGIN_PATH/sqldrivers
	cp -R $QT_FW_PATH/QtCore.framework $BINARY_FW_PATH
        cp -R $QT_FW_PATH/QtGui.framework $BINARY_FW_PATH
        cp -R $QT_FW_PATH/QtSql.framework $BINARY_FW_PATH
        cp -R $QT_FW_PATH/QtNetwork.framework $BINARY_FW_PATH
        # remove debug versions
        rm -f $APPLICATION/Contents/Frameworks/QtCore.framework/QtCore_debug
        rm -rf $APPLICATION/Contents/Frameworks/QtCore.framework/QtCore_debug.dSYM
        rm -f $APPLICATION/Contents/Frameworks/QtCore.framework/QtCore_debug.prl
        rm -f $APPLICATION/Contents/Frameworks/QtCore.framework/Versions/4/QtCore_debug
        rm -f $APPLICATION/Contents/Frameworks/QtGui.framework/QtGui_debug
        rm -rf $APPLICATION/Contents/Frameworks/QtGui.framework/QtGui_debug.dSYM
        rm -f $APPLICATION/Contents/Frameworks/QtGui.framework/QtGui_debug.prl
        rm -f $APPLICATION/Contents/Frameworks/QtGui.framework/Versions/4/QtGui_debug
        rm -f $APPLICATION/Contents/Frameworks/QtSql.framework/QtSql_debug
        rm -rf $APPLICATION/Contents/Frameworks/QtSql.framework/QtSql_debug.dSYM
        rm -f $APPLICATION/Contents/Frameworks/QtSql.framework/QtSql_debug.prl
        rm -f $APPLICATION/Contents/Frameworks/QtSql.framework/Versions/4/QtSql_debug
        rm -f $APPLICATION/Contents/Frameworks/QtNetwork.framework/QtNetwork_debug
        rm -rf $APPLICATION/Contents/Frameworks/QtNetwork.framework/QtNetwork_debug.dSYM
        rm -f $APPLICATION/Contents/Frameworks/QtNetwork.framework/QtNetwork_debug.prl
        rm -f $APPLICATION/Contents/Frameworks/QtNetwork.framework/Versions/4/QtNetwork_debug

	# redirect binary to use integrated frameworks
        QTCORE="QtCore.framework/Versions/4.0/QtCore"
        QTGUI="QtGui.framework/Versions/4.0/QtGui"
        QTSQL="QtSql.framework/Versions/4.0/QtSql"
        QTNETWORK="QtNetwork.framework/Versions/4.0/QtNetwork"
        install_name_tool -id @executable_path/../Frameworks/$QTCORE $BINARY_FW_PATH/$QTCORE
	install_name_tool -id @executable_path/../Frameworks/$QTGUI $BINARY_FW_PATH/$QTGUI
        install_name_tool -id @executable_path/../Frameworks/$QTSQL $BINARY_FW_PATH/$QTSQL
        install_name_tool -id @executable_path/../Frameworks/$QTSQL $BINARY_FW_PATH/$QTNETWORK
        QTCORE_LINK=$(otool -L $BINARY | grep QtCore | cut -d"(" -f1 | cut -f2)
	QTGUI_LINK=$(otool -L $BINARY | grep QtGui | cut -d"(" -f1 | cut -f2)
        QTSQL_LINK=$(otool -L $BINARY | grep QtSql | cut -d"(" -f1 | cut -f2)
        QTNETWORK_LINK=$(otool -L $BINARY | grep QtNetwork | cut -d"(" -f1 | cut -f2)
        install_name_tool -change $QTCORE_LINK @executable_path/../Frameworks/$QTCORE $BINARY
	install_name_tool -change $QTGUI_LINK @executable_path/../Frameworks/$QTGUI $BINARY
        install_name_tool -change $QTSQL_LINK @executable_path/../Frameworks/$QTSQL $BINARY
        install_name_tool -change $QTNETWORK_LINK @executable_path/../Frameworks/$QTNETWORK $BINARY
        install_name_tool -change $QTCORE_LINK @executable_path/../Frameworks/$QTCORE $BINARY_PLUGIN_PATH/imageformats/libqgif.dylib
        install_name_tool -change $QTGUI_LINK @executable_path/../Frameworks/$QTGUI $BINARY_PLUGIN_PATH/imageformats/libqgif.dylib
        install_name_tool -change $QTCORE_LINK @executable_path/../Frameworks/$QTCORE $BINARY_PLUGIN_PATH/imageformats/libqjpeg.dylib
        install_name_tool -change $QTGUI_LINK @executable_path/../Frameworks/$QTGUI $BINARY_PLUGIN_PATH/imageformats/libqjpeg.dylib
        install_name_tool -change $QTCORE_LINK @executable_path/../Frameworks/$QTCORE $BINARY_PLUGIN_PATH/sqldrivers/libqsqlite.dylib
        install_name_tool -change $QTSQL_LINK @executable_path/../Frameworks/$QTSQL $BINARY_PLUGIN_PATH/sqldrivers/libqsqlite.dylib

        QTCORE_LINK=$(otool -L $BINARY_FW_PATH/$QTGUI | grep QtCore | head -1 | cut -d"(" -f1 | cut -f2)
        install_name_tool -change $QTCORE_LINK @executable_path/../Frameworks/$QTCORE $BINARY_FW_PATH/$QTGUI
        install_name_tool -change $QTCORE_LINK @executable_path/../Frameworks/$QTCORE $BINARY_FW_PATH/$QTSQL
        install_name_tool -change $QTCORE_LINK @executable_path/../Frameworks/$QTCORE $BINARY_FW_PATH/$QTNETWORK

fi

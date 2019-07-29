#!/bin/bash
# Author: Lothar May (originally from Erhard List)

# ATTENTION: this is for MAC only!
#
# This script integrates the necessary frameworks into the binary.
#
# The integration of Qt is optional (commandlineswitch --without-qt) as you can save 30MB 
# of binary-size if you leave Qt out.
# (see http://trolltech.com/developer/downloads/qt/mac)

QT_FW_PATH="/usr/local/Cellar/qt/5.13.0/lib"
QT_PLUGIN_PATH="/usr/local/Cellar/qt/5.13.0/plugins"
SDL_FW_PATH="/Library/Frameworks"
APPLICATION="./pokerth.app"
BINARY="$APPLICATION/Contents/MacOs/pokerth"
RESOURCES="$APPLICATION/Contents/Resources"
DYLIB_PATH="$APPLICATION/Contents/MacOs/dylibs"

# strip binary
strip $BINARY

cp -R ./data $RESOURCES/
find $RESOURCES/data -name ".svn" | xargs rm -Rf
# create framework-path
BINARY_FW_PATH="$APPLICATION/Contents/Frameworks"
BINARY_PLUGIN_PATH="$APPLICATION/Contents/plugins"

rm -rf $BINARY_FW_PATH
rm -rf $BINARY_PLUGIN_PATH
rm -rf $DYLIB_PATH

mkdir $BINARY_FW_PATH
mkdir -p $BINARY_PLUGIN_PATH/imageformats
mkdir -p $BINARY_PLUGIN_PATH/sqldrivers
mkdir -p $BINARY_PLUGIN_PATH/platforms
mkdir -p $DYLIB_PATH

# integrate SDL-frameworks into binary
cp -R $SDL_FW_PATH/SDL.framework $BINARY_FW_PATH
cp -R $SDL_FW_PATH/SDL_mixer.framework $BINARY_FW_PATH

# integrate dylibs
cp /usr/lib/libcurl.4.dylib $DYLIB_PATH/.
cp /usr/local/opt/openssl/lib/libcrypto.1.0.0.dylib $DYLIB_PATH/.
cp /usr/local/opt/openssl/lib/libssl.1.0.0.dylib $DYLIB_PATH/.
cp /usr/lib/libsqlite3.dylib $DYLIB_PATH/.
cp /usr/local/opt/tinyxml/lib/libtinyxml.dylib $DYLIB_PATH/.
cp /usr/local/opt/protobuf/lib/libprotobuf.18.dylib $DYLIB_PATH/.
cp /usr/lib/libz.1.dylib $DYLIB_PATH/.

LIBCURL_LINK=$(otool -L $BINARY | grep libcurl | cut -d"(" -f1 | cut -f2)
LIBCRYPTO_LINK=$(otool -L $BINARY | grep libcrypto | cut -d"(" -f1 | cut -f2)
LIBSSL_LINK=$(otool -L $BINARY | grep libssl | cut -d"(" -f1 | cut -f2)
LIBTINYXML_LINK=$(otool -L $BINARY | grep libtinyxml | cut -d"(" -f1 | cut -f2)
LIBSQLITE_LINK=$(otool -L $BINARY | grep libsqlite | cut -d"(" -f1 | cut -f2)
LIBPROTOBUF_LINK=$(otool -L $BINARY | grep libprotobuf | cut -d"(" -f1 | cut -f2)
LIBZ_LINK=$(otool -L $BINARY | grep libz | cut -d"(" -f1 | cut -f2)

install_name_tool -change $LIBCURL_LINK @loader_path/dylibs/libcurl.4.dylib $BINARY
install_name_tool -change $LIBCRYPTO_LINK @loader_path/dylibs/libcrypto.1.0.0.dylib  $BINARY
install_name_tool -change $LIBSSL_LINK @loader_path/dylibs/libssl.1.0.0.dylib $BINARY
install_name_tool -change $LIBSSL_LINK @loader_path/dylibs/libssl.1.0.0.dylib $APPLICATION/Contents/MacOS/dylibs/libcrypto.1.0.0.dylib
install_name_tool -change $LIBTINYXML_LINK @loader_path/dylibs/libtinyxml.dylib $BINARY
install_name_tool -change $LIBSQLITE_LINK @loader_path/dylibs/libsqlite3.dylib $BINARY
install_name_tool -change $LIBPROTOBUF_LINK @loader_path/dylibs/libprotobuf.18.dylib $BINARY
install_name_tool -change $LIBSQLITE_LINK @loader_path/dylibs/libsqlite3.dylib $BINARY
install_name_tool -change $LIBZ_LINK @loader_path/dylibs/libz.1.dylib $BINARY

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
        cp -R /usr/local/Cellar/qt/5.13.0/lib/QtDBus.framework $BINARY_FW_PATH
        chmod -R 775 $BINARY_FW_PATH
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
        rm -f $APPLICATION/Contents/Frameworks/QtDBus.framework/QtDBus_debug
        rm -f $APPLICATION/Contents/Frameworks/QtDBus.framework/QtDBus_debug.prl
        rm -f $APPLICATION/Contents/Frameworks/QtDBus.framework/Versions/5/QtDBus_debug

        # redirect binary to use integrated frameworks
        QTCORE="QtCore.framework/Versions/5/QtCore"
        QTGUI="QtGui.framework/Versions/5/QtGui"
        QTWIDGETS="QtWidgets.framework/Versions/5/QtWidgets"
        QTSQL="QtSql.framework/Versions/5/QtSql"
        QTNETWORK="QtNetwork.framework/Versions/5/QtNetwork"
        QTPRINT="QtPrintSupport.framework/Versions/5/QtPrintSupport"
        QTDBUS="QtDBus.framework/Versions/5/QtDBus"
        install_name_tool -id @executable_path/../Frameworks/$QTCORE $BINARY_FW_PATH/$QTCORE
        install_name_tool -id @executable_path/../Frameworks/$QTGUI $BINARY_FW_PATH/$QTGUI
        install_name_tool -id @executable_path/../Frameworks/$QTWIDGETS $BINARY_FW_PATH/$QTWIDGETS
        install_name_tool -id @executable_path/../Frameworks/$QTSQL $BINARY_FW_PATH/$QTSQL
        install_name_tool -id @executable_path/../Frameworks/$QTNETWORK $BINARY_FW_PATH/$QTNETWORK
        install_name_tool -id @executable_path/../Frameworks/$QTPRINT $BINARY_FW_PATH/$QTPRINT
        install_name_tool -id @executable_path/../Frameworks/$QDBUS $BINARY_FW_PATH/$QTDBUS
        QTCORE_LINK=$(otool -L $BINARY | grep QtCore | cut -d"(" -f1 | cut -f2)
        QTGUI_LINK=$(otool -L $BINARY | grep QtGui | cut -d"(" -f1 | cut -f2)
        QTWIDGETS_LINK=$(otool -L $BINARY | grep QtWidgets | cut -d"(" -f1 | cut -f2)
        QTSQL_LINK=$(otool -L $BINARY | grep QtSql | cut -d"(" -f1 | cut -f2)
        QTNETWORK_LINK=$(otool -L $BINARY | grep QtNetwork | cut -d"(" -f1 | cut -f2)

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

        QTDBUS_LINK=$(otool -L $BINARY_PLUGIN_PATH/platforms/libqcocoa.dylib | grep QtDBus | cut -d"(" -f1 | cut -f2)
        QTPRINT_LINK=$(otool -L $BINARY_PLUGIN_PATH/platforms/libqcocoa.dylib | grep QtPrintSupport | cut -d"(" -f1 | cut -f2)
        QTZ_LINK=$(otool -L $BINARY_PLUGIN_PATH/platforms/libqcocoa.dylib | grep libz | cut -d"(" -f1 | cut -f2)
        QTCORE_LINK=$(otool -L $BINARY_PLUGIN_PATH/platforms/libqcocoa.dylib | grep QtCore | cut -d"(" -f1 | cut -f2)
        QTGUI_LINK=$(otool -L $BINARY_PLUGIN_PATH/platforms/libqcocoa.dylib | grep QtGui | cut -d"(" -f1 | cut -f2)
        QTWIDGETS_LINK=$(otool -L $BINARY_PLUGIN_PATH/platforms/libqcocoa.dylib | grep QtWidgets | cut -d"(" -f1 | cut -f2)
        install_name_tool -change $QTCORE_LINK @executable_path/../Frameworks/$QTCORE $BINARY_PLUGIN_PATH/platforms/libqcocoa.dylib
        install_name_tool -change $QTGUI_LINK @executable_path/../Frameworks/$QTGUI $BINARY_PLUGIN_PATH/platforms/libqcocoa.dylib
        install_name_tool -change $QTWIDGETS_LINK @executable_path/../Frameworks/$QTWIDGETS $BINARY_PLUGIN_PATH/platforms/libqcocoa.dylib
        install_name_tool -change $QTPRINT_LINK @executable_path/../Frameworks/$QTPRINT $BINARY_PLUGIN_PATH/platforms/libqcocoa.dylib
        install_name_tool -change $QTDBUS_LINK @executable_path/../Frameworks/$QTDBUS $BINARY_PLUGIN_PATH/platforms/libqcocoa.dylib
        install_name_tool -change $QTZ_LINK @loader_path/dylibs/libz.1.dylib $BINARY_PLUGIN_PATH/platforms/libqcocoa.dylib

        QTCORE_LINK=$(otool -L $BINARY_FW_PATH/$QTDBUS | grep QtCore | head -1 | cut -d"(" -f1 | cut -f2)
        install_name_tool -change $QTCORE_LINK @executable_path/../Frameworks/$QTCORE $BINARY_FW_PATH/$QTDBUS

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

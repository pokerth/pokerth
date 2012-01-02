#!/bin/bash
PKTH_GSASL="libgsasl-1.6.1"
PKTH_GSASL_FILE="$PKTH_GSASL.tar.gz"
PKTH_QT="qt-everywhere-opensource-src-4.8.0"
PKTH_QT_FILE="$PKTH_QT.tar.gz"
PKTH_BOOST="boost_1_48_0"
PKTH_BOOST_FILE="$PKTH_BOOST.tar.bz2"

if [ ! -e $PKTH_GSASL_FILE ]; then
        wget ftp://ftp.gnu.org/gnu/gsasl/$PKTH_GSASL_FILE
fi
if [ ! -d libgsasl ]; then
        tar xzf $PKTH_GSASL_FILE
        mv $PKTH_GSASL libgsasl
fi
if [ ! -e libgsasl/src/.libs/libgsasl.a ]; then
        cd libgsasl
        ./configure --enable-static --disable-shared --disable-ntlm --disable-gs2 --disable-gssapi --disable-kerberos_v5 --enable-scram-sha1
        make
        cd ..
fi

if [ ! -e $PKTH_QT_FILE ]; then
        wget http://get.qt.nokia.com/qt/source/$PKTH_QT_FILE
fi
if [ ! -d qt ]; then
        tar xzf $PKTH_QT_FILE
        mv $PKTH_QT qt
fi
if [ ! -e qt/lib/libQtGui.a ]; then
        cd qt
        ./configure -opensource -static -prefix /usr/local -qt-libpng -qt-libjpeg -qt-gif -no-libtiff -no-libmng -no-openssl -no-glib -no-dbus -no-qdbus -no-cups -no-opengl -no-openvg -no-script -no-webkit -no-phonon -no-qt3support -qt-sql-sqlite -fast
        make sub-src
        cd ..
fi
if [ ! -e $PKTH_BOOST_FILE ]; then
        wget http://heanet.dl.sourceforge.net/sourceforge/boost/$PKTH_BOOST_FILE
fi
if [ ! -d boost ]; then
        tar xjf $PKTH_BOOST_FILE
        mv $PKTH_BOOST boost
fi
if [ ! -e boost/stage/lib/libboost_system.a ]; then
        cd boost
        ./bootstrap.sh
        ./b2 link=static --build-dir=../boost_temp --toolset=gcc stage
        cd ..
        rm -rf boost_temp
fi


QMAKE_MOC = $$QMAKE_MOC -DBOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION

isEmpty( PREFIX ){
        PREFIX =/usr
}
DEFINES += PREFIX=\"$${PREFIX}\"

TEMPLATE = app
CODECFORSRC = UTF-8
QT += core qml quick widgets svg sql
CONFIG += qt thread embed_manifest_exe exceptions rtti stl warn_on
DEFINES += ENABLE_IPV6 TIXML_USE_STL BOOST_FILESYSTEM_DEPRECATED

DEFINES += QML_CLIENT
RESOURCES += src/gui/qml/qml.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH = \
    src/gui/qml/views/startview/ \
    src/gui/qml/views/createlocalgameview/ \
    src/gui/qml/views/ \
    src/gui/qml/components/styles/ \
    src/gui/qml/components/ \
    src/gui/qml/js/

# Default rules for deployment.
include(src/gui/qml/deployment.pri)

INCLUDEPATH += . \
    src \
    src/config \
    src/core \
    src/core/common \
    src/core/common/qttools \
    src/core/common/qttools/qthelper \
    src/gui \
    src/gui/qml/cpp

DEPENDPATH += . \
    src \
    src/config \
    src/core \
    src/core/common \
    src/core/common/qttools \
    src/core/common/qttools/qthelper \
    src/gui \
    src/gui/qml/cpp

HEADERS += \
    src/core/loghelper.h \
    src/core/common/qttools/qthelper/qthelper.h \
    src/core/common/qttools/qttoolswrapper.h \
    src/core/common/qttoolsinterface.h \
    src/gui/qml/cpp/qmlwrapper.h \
    src/gui/qml/cpp/qmlconfig.h \
    src/gui/qml/cpp/startviewimpl.h \
    src/gui/qml/cpp/createlocalgameviewimpl.h


SOURCES += \
    src/pokerth.cpp \
    src/core/common/loghelper_client.cpp \
    src/core/common/qttools/qthelper/qthelper.cpp \
    src/core/common/qttools/qttoolswrapper.cpp \
    src/core/common/qttoolsinterface.cpp \
    src/gui/qml/cpp/qmlwrapper.cpp \
    src/gui/qml/cpp/qmlconfig.cpp \
    src/gui/qml/cpp/createlocalgameviewimpl.cpp \
    src/gui/qml/cpp/startviewimpl.cpp

LIBS += \
    -lpokerth_lib \
    -lpokerth_db \
    -lpokerth_protocol \
    -lcurl

unix:!mac {
    # #### My release static build options
    # QMAKE_CXXFLAGS += -ffunction-sections -fdata-sections
    # QMAKE_LFLAGS += -Wl,--gc-sections
    INCLUDEPATH += $${PREFIX}/include
    QMAKE_LIBDIR += lib
    !android{
        LIBPATH += $${PREFIX}/lib /opt/gsasl/lib
        LIB_DIRS = $${PREFIX}/lib \
            $${PREFIX}/lib64 \
            $${PREFIX}/lib/x86_64-linux-gnu \
            $$system(qmake -query QT_INSTALL_LIBS)
    }
    android{
          LIBPATH += $${PREFIX}/lib/armv5
          LIB_DIRS = $${PREFIX}/lib/armv5
    }

# BEGIN TODO: Cleanup!!! --> This is just a copy from pokerth_game.pro when starting with c++ integration
    BOOST_FS = boost_filesystem \
            boost_filesystem-mt
    BOOST_THREAD = boost_thread \
            boost_thread-mt
    BOOST_IOSTREAMS = boost_iostreams \
            boost_iostreams-mt
    BOOST_REGEX = boost_regex \
            boost_regex-mt
    BOOST_CHRONO = boost_chrono \
            boost_chrono-mt
    BOOST_SYS = boost_system \
            boost_system-mt
    BOOST_RANDOM = boost_random \
            boost_random-mt

    # searching in $PREFIX/lib, $PREFIX/lib64 and $$system(qmake -query QT_INSTALL_LIBS)
    # to override the default '/usr' pass PREFIX
    # variable to qmake.
    for(dir, LIB_DIRS):exists($$dir) {
        for(lib, BOOST_THREAD):exists($${dir}/lib$${lib}.so*) {
                message("Found $$lib")
                BOOST_THREAD = -l$$lib
        }
        for(lib, BOOST_THREAD):exists($${dir}/lib$${lib}.a) {
                message("Found $$lib")
                BOOST_THREAD = -l$$lib
        }
        for(lib, BOOST_FS):exists($${dir}/lib$${lib}.so*) {
                message("Found $$lib")
                BOOST_FS = -l$$lib
        }
        for(lib, BOOST_FS):exists($${dir}/lib$${lib}.a) {
                message("Found $$lib")
                BOOST_FS = -l$$lib
        }
        for(lib, BOOST_IOSTREAMS):exists($${dir}/lib$${lib}.so*) {
                message("Found $$lib")
                BOOST_IOSTREAMS = -l$$lib
        }
        for(lib, BOOST_IOSTREAMS):exists($${dir}/lib$${lib}.a) {
                message("Found $$lib")
                BOOST_IOSTREAMS = -l$$lib
        }
        for(lib, BOOST_REGEX):exists($${dir}/lib$${lib}.so*) {
                message("Found $$lib")
                BOOST_REGEX = -l$$lib
        }
        for(lib, BOOST_REGEX):exists($${dir}/lib$${lib}.a) {
                message("Found $$lib")
                BOOST_REGEX = -l$$lib
        }
        for(lib, BOOST_CHRONO):exists($${dir}/lib$${lib}.so*) {
                message("Found $$lib")
                BOOST_CHRONO = -l$$lib
        }
        for(lib, BOOST_CHRONO):exists($${dir}/lib$${lib}.a) {
                message("Found $$lib")
                BOOST_CHRONO = -l$$lib
        }
        !android{
                for(lib, BOOST_RANDOM):exists($${dir}/lib$${lib}.so*) {
                        message("Found $$lib")
                        BOOST_RANDOM = -l$$lib
                }
                for(lib, BOOST_RANDOM):exists($${dir}/lib$${lib}.a) {
                        message("Found $$lib")
                        BOOST_RANDOM = -l$$lib
                }
        }
        for(lib, BOOST_SYS):exists($${dir}/lib$${lib}.so*) {
                message("Found $$lib")
                BOOST_SYS = -l$$lib
        }
        for(lib, BOOST_SYS):exists($${dir}/lib$${lib}.a) {
                message("Found $$lib")
                BOOST_SYS = -l$$lib
        }
    }
    !android{
            BOOST_LIBS = $$BOOST_THREAD \
                    $$BOOST_FS \
                    $$BOOST_IOSTREAMS \
                    $$BOOST_REGEX \
                    $$BOOST_CHRONO \
                    $$BOOST_RANDOM \
                    $$BOOST_SYS
            !count(BOOST_LIBS, 7):error("Unable to find boost libraries in PREFIX=$${PREFIX}")
            if($$system(sdl-config --version)):error("sdl-config not found in PATH - libSDL_mixer, libSDL are required!")
            UNAME = $$system(uname -s)
            BSD = $$find(UNAME, "BSD")
            kFreeBSD = $$find(UNAME, "kFreeBSD")
            LIBS += -lsqlite3 \
                            -ltinyxml \
                            -lprotobuf
            LIBS += $$BOOST_LIBS
            LIBS += -lSDL \
                            -lSDL_mixer \
                    -lgsasl
            !isEmpty( BSD ):isEmpty( kFreeBSD ):LIBS += -lcrypto
            else:LIBS += -lgcrypt
    }
    android{
            BOOST_LIBS = $$BOOST_THREAD \
                    $$BOOST_FS \
                    $$BOOST_IOSTREAMS \
                    $$BOOST_REGEX \
                    $$BOOST_CHRONO \
                    $$BOOST_SYS
            !count(BOOST_LIBS, 6):error("Unable to find boost libraries in PREFIX=$${PREFIX}lib/armv5")
            LIBS += -ltinyxml
            LIBS += $$BOOST_LIBS
            LIBS += -lgsasl -lidn
            LIBS += -lssl -lcrypto -lgcrypt -lgpg-error -lprotobuf-lite

            !android_api8 {
                    #android sound lib for api9 and higher
                    LIBS += -lOpenSLES
            }
    }
    TARGETDEPS += ./lib/libpokerth_lib.a \
            ./lib/libpokerth_db.a \
            ./lib/libpokerth_protocol.a

    !android{
            # #### My release static libs
            # LIBS += -lgcrypt_static -lgpg-error_static -lgnutls_static -lSDL_mixer_static -lSDL -lmikmod -lcurl
            # ### INSTALL ####
            binary.path += $${PREFIX}/bin/
            binary.files += pokerth
            data.path += $${PREFIX}/share/pokerth/data/
            data.files += data/*
            pixmap.path += $${PREFIX}/share/pixmaps/
            pixmap.files += pokerth.png
            desktop.path += $${PREFIX}/share/applications/
            desktop.files += pokerth.desktop
            INSTALLS += binary \
                    data \
                    pixmap \
                    desktop
    }

# END TODO: Cleanup!!! --> This is just a copy from pokerth_game.pro when starting with c++ integration

}

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android

contains(ANDROID_TARGET_ARCH,armeabi) {
    ANDROID_EXTRA_LIBS =
}


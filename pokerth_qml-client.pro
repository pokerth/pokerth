TEMPLATE = app
QT += core qml quick widgets svg

DEFINES += QML_CLIENT

INCLUDEPATH += \
    src/gui/qml \
    src/gui/qml/views \
    src/gui/qml/components

DEPENDPATH += \
    src \
    src/gui/qml \
    src/gui/qml/views \
    src/gui/qml/components

RESOURCES += src/gui/qml/qml.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH = \
    src/gui/qml/views/ \
    src/gui/qml/views/startview/ \
    src/gui/qml/views/createlocalgameview/ \
    src/gui/qml/components

# Default rules for deployment.
include(src/gui/qml/deployment.pri)

HEADERS += \
    src/gui/qml/qmlwrapper.h \
    src/gui/qml/views/startview/startview.h \

SOURCES += \
    src/pokerth.cpp \
    src/gui/qml/qmlwrapper.cpp \
    src/gui/qml/views/startview/startview.cpp

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
}

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android

contains(ANDROID_TARGET_ARCH,armeabi) {
    ANDROID_EXTRA_LIBS =
}

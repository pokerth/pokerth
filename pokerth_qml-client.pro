TEMPLATE = app
QT += core qml quick widgets

DEFINES += QML_CLIENT

INCLUDEPATH += \
    src/gui/qml \
    src/gui/qml/views

DEPENDPATH += \
    src \
    src/gui/qml \
    src/gui/qml/views

RESOURCES += src/gui/qml/views/qml.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH = src/gui/qml/views/

# Default rules for deployment.
include(src/gui/qml/views/deployment.pri)

HEADERS += \
    src/gui/qml/qmlwrapper.h \
    src/gui/qml/views/startview/startview.h \

SOURCES += \
    src/pokerth.cpp \
    src/gui/qml/qmlwrapper.cpp \
    src/gui/qml/views/startview/startview.cpp

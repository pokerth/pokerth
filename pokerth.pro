# Diese Datei wurde mit dem qmake-Manager von KDevelop erstellt. 
# ------------------------------------------- 
# Unterordner relativ zum Projektordner: .
# Das Target ist eine Anwendung:  bin/pokerth

INSTALLS += TARGET 
TARGET.files += bin/* 
TARGET.path = /usr/bin/ 
FORMS += mainwindow.ui \
         aboutpokerth.ui \
         newgamedialog.ui \
	 settingsdialog.ui
HEADERS += mainwindowimpl.h \
           newgamedialogimpl.h \
           aboutpokerthimpl.h \
	   settingsdialogimpl.h \
	   configfile.h \
           log.h \
           guiinterface.h \
           guiwrapper.h \
	   session.h \
           game.h \
	   localhand.h \
           localboard.h \
           localplayer.h \
           cardsvalue.h \
           localpreflop.h \
           localflop.h \
           localturn.h \
           localriver.h \
           tools.h \
	   handinterface.h \
           playerinterface.h \
           boardinterface.h \
           preflopinterface.h \
           flopinterface.h \
           turninterface.h \
           riverinterface.h \
	   enginefactory.h \
	   localenginefactory.h 

SOURCES += pokerth.cpp \
	   mainwindowimpl.cpp \
           newgamedialogimpl.cpp \
           aboutpokerthimpl.cpp \
	   settingsdialogimpl.cpp \
	   configfile.cpp \
           log.cpp \
           guiinterface.cpp \
           guiwrapper.cpp \
	   session.cpp \
           game.cpp \
	   localhand.cpp \
           localboard.cpp \
           localplayer.cpp \
           cardsvalue.cpp \
           localpreflop.cpp \
           localflop.cpp \
           localturn.cpp \
           localriver.cpp \
           tools.cpp \
	   handinterface.cpp \
           playerinterface.cpp \
           boardinterface.cpp \
           preflopinterface.cpp \
           flopinterface.cpp \
           turninterface.cpp \
           riverinterface.cpp \
	   enginefactory.cpp \
	   localenginefactory.cpp

RESOURCES = src/gui/qt/deck.qrc
TEMPLATE = app
DEPENDPATH += . src uics src/gui src/gui/qt src/engine src/engine/local_engine src/engine/network_engine
INCLUDEPATH += . src uics src/gui src/gui/qt src/engine src/engine/local_engine src/engine/network_engine
CONFIG += qt release
UI_DIR = uics
TARGET = bin/pokerth
MOC_DIR = mocs
OBJECTS_DIR = obj
QT += qt3support

# QMake pro-file for the PokerTH dedicated server

isEmpty( PREFIX ){
	PREFIX =/usr
}

TEMPLATE = app
CODECFORSRC = UTF-8

CONFIG += thread console embed_manifest_exe exceptions rtti stl warn_on

UI_DIR = uics
TARGET = bin/load
MOC_DIR = mocs
OBJECTS_DIR = obj
DEFINES += PREFIX=\"$${PREFIX}\"
QT -= core gui
#PRECOMPILED_HEADER = src/pch_lib.h

INCLUDEPATH += . \
		src \
		src/third_party/asn1

DEPENDPATH += . \
		src \
		src/third_party/asn1

# Input
HEADERS += \
		src/game_defs.h

SOURCES += \
		src/load.cpp

LIBS += -lpokerth_protocol

unix : !mac {

	##### My release static build options
	#QMAKE_CXXFLAGS += -ffunction-sections -fdata-sections
	#QMAKE_LFLAGS += -Wl,--gc-sections

	QMAKE_LIBDIR += lib $${PREFIX}/lib /opt/gsasl/lib
	INCLUDEPATH += $${PREFIX}/include
	LIB_DIRS = $${PREFIX}/lib $${PREFIX}/lib64 $$system(qmake -query QT_INSTALL_LIBS)
	BOOST_THREAD = boost_thread boost_thread-mt
	BOOST_PROGRAM_OPTIONS = boost_program_options boost_program_options-mt
	BOOST_SYS = boost_system boost_system-mt


	#
	# searching in $PREFIX/lib, $PREFIX/lib64 and $$system(qmake -query QT_INSTALL_LIBS)
	# to override the default '/usr' pass PREFIX
	# variable to qmake.
	#
	for(dir, LIB_DIRS){
		exists($$dir){
			for(lib, BOOST_THREAD):exists($${dir}/lib$${lib}.so*) {
				message("Found $$lib")
				BOOST_THREAD = -l$$lib
			}
			for(lib, BOOST_THREAD):exists($${dir}/lib$${lib}.a) {
				message("Found $$lib")
				BOOST_THREAD = -l$$lib
			}
			for(lib, BOOST_PROGRAM_OPTIONS):exists($${dir}/lib$${lib}.so*) {
				message("Found $$lib")
				BOOST_PROGRAM_OPTIONS = -l$$lib
			}
			for(lib, BOOST_PROGRAM_OPTIONS):exists($${dir}/lib$${lib}.a) {
				message("Found $$lib")
				BOOST_PROGRAM_OPTIONS = -l$$lib
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
	}
	BOOST_LIBS = $$BOOST_PROGRAM_OPTIONS $$BOOST_SYS $$BOOST_THREAD
	!count(BOOST_LIBS, 3){
		error("Unable to find boost libraries in PREFIX=$${PREFIX}")
	}

	UNAME = $$system(uname -s)
	BSD = $$find(UNAME, "BSD")
	kFreeBSD = $$find(UNAME, "kFreeBSD")

	LIBS += $$BOOST_LIBS
	LIBS += -lgsasl

	POST_TARGETDEPS += ./lib/libpokerth_protocol.a

	#### INSTALL ####

	binary.path += $${PREFIX}/bin/
	binary.files += load

	INSTALLS += binary
}


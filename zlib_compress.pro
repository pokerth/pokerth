# QMake pro-file for the file compression tool

isEmpty( PREFIX ) {
    PREFIX=/usr
}

TEMPLATE = app
CODECFORSRC = UTF-8

#CONFIG += thread console embed_manifest_exe exceptions rtti stl warn_on release
CONFIG += thread console embed_manifest_exe exceptions rtti stl warn_on debug

UI_DIR = uics
TARGET = bin/zlib_compress
MOC_DIR = mocs
OBJECTS_DIR = obj
DEFINES += PREFIX=\"$${PREFIX}\"
DEFINES += BOOST_FILESYSTEM_DEPRECATED
QT -= core gui

INCLUDEPATH += . \
		src

DEPENDPATH += . \
		src

SOURCES += \
		src/zlib_compress.cpp

win32 {
    INCLUDEPATH += ../boost/

    LIBPATH += ../boost/stage/lib

	win32-g++{
		LIBS += -llibboost_filesystem-mgw34-mt-1_35
		LIBS += -llibboost_system-mgw34-mt-1_35
		LIBS += -llibboost_iostreams-mgw34-mt-1_35
		LIBS += -llibboost_zlib-mgw34-mt-1_35
	}

    LIBS += -lgdi32 -lcomdlg32 -loleaut32 -limm32 -lwinmm -lwinspool -lole32 -luuid -luser32 -lmsimg32 -lshell32 -lkernel32
}

unix : !mac {

	##### My release static build options
	#QMAKE_CXXFLAGS += -ffunction-sections -fdata-sections
	#QMAKE_LFLAGS += -Wl,--gc-sections

	LIB_DIRS = $${PREFIX}/lib $${PREFIX}/lib64 $$system(qmake -query QT_INSTALL_LIBS)
	BOOST_FS = boost_filesystem boost_filesystem-mt
	BOOST_IOSTREAMS = boost_iostreams boost_iostreams-mt
	BOOST_SYSTEM = boost_system boost_system-mt

	for(dir, LIB_DIRS) {
		exists($$dir) {
			for(lib, BOOST_FS) {
				exists($${dir}/lib$${lib}.so*) {
					message("Found $$lib")
					BOOST_FS = -l$$lib
				}
			}
			for(lib, BOOST_IOSTREAMS) {
				exists($${dir}/lib$${lib}.so*) {
					message("Found $$lib")
					BOOST_IOSTREAMS = -l$$lib
				}
			}
			for(lib, BOOST_SYSTEM) {
				exists($${dir}/lib$${lib}.so*) {
					message("Found $$lib")
					BOOST_SYSTEM = -l$$lib
				}
			}
 		}
 	}
	BOOST_LIBS = $$BOOST_FS $$BOOST_IOSTREAMS $$BOOST_SYSTEM
	!count(BOOST_LIBS, 3) {
		error("could not locate required library: \
		    libboost (version >= 1.34.1)  --> http://www.boost.org/")
	}
	
	LIBS += $$BOOST_LIBS

	#### INSTALL ####

	binary.path += $${PREFIX}/bin/
	binary.files += zlib_compress

	INSTALLS += binary
}

mac{
	# make it universal  
	CONFIG += x86
	CONFIG += ppc
	QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.3

	# workaround for problems with boost_filesystem exceptions
	QMAKE_LFLAGS += -no_dead_strip_inits_and_terms

	# for universal-compilation on PPC-Mac uncomment the following line
	# on Intel-Mac you have to comment this line out or build will fail.
	#       QMAKE_MAC_SDK=/Developer/SDKs/MacOSX10.4u.sdk/

	# standard path for darwinports
	# make sure you have a universal version of boost
	LIBS += /usr/local/lib/libboost_filesystem-mt-1_35.a
	LIBS += /usr/local/lib/libboost_system-mt-1_35.a
	LIBS += /usr/local/lib/libboost_iostreams-mt-1_35.a
	# libraries installed on every mac
	LIBPATH += /Developer/SDKs/MacOSX10.4u.sdk/usr/lib
	INCLUDEPATH += /Developer/SDKs/MacOSX10.4u.sdk/usr/include/
}

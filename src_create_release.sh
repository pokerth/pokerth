#!/bin/bash
if [[ -n "$1" && -n "$2"  ]] ; then

	rm -r PokerTH-$1-src
	mkdir PokerTH-$1-src
	cd PokerTH-$1-src
	
	# CHECKOUT PokerTH SOURCECODE
	svn co http://pokerth.svn.sourceforge.net/svnroot/pokerth/trunk/pokerth .
	
	# CLEAN SOURCES
	svn cleanup
	find . -type d -name .svn -exec rm -rf {} \;
	find . -type f -name .directory -exec rm -rf {} \;
	find . -type f -name *~ -exec rm -rf {} \;
	rm -rf debug
	rm -rf mocs
	rm -rf obj
	rm -rf old
	rm -rf qtc-gdbmacros
	rm -rf release
	rm -rf uics
	rm -rf bin
	rm -rf lib	

	cd ..
	tar -cvjf PokerTH-$1-src.tar.bz2 PokerTH-$1-src/
	scp PokerTH-$1-src.tar.bz2 doitux,pokerth@frs.sourceforge.net:/home/frs/project/p/po/pokerth/pokerth/$2/
	md5sum PokerTH-$1-src.tar.bz2
	  
	rm PokerTH-$1-src.tar.bz2
	rm -r PokerTH-$1-src

else
echo "Please use 'linux-create-release.sh version sf-dir-version'"
fi

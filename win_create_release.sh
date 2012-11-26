#!/bin/bash
if [[ -n "$1" && -n "$2" ]] ; then

        rm -r ./PokerTH-$1*
        mkdir ./PokerTH-$1
        cp release/pokerth.exe ./PokerTH-$1/
        cp release/pokerth.exe .
        cp -R ./data ./PokerTH-$1/
        find ./PokerTH-$1/ -name ".svn" | xargs rm -Rf
        zip -r PokerTH-$1-windows.zip ./PokerTH-$1

        ~/bitrock-installbuilder/bin/builder build ~/slave/pokerth-release-win32/build/pokerth_bitrock_windows.xml windows

        mv ~/bitrock-installbuilder/output/PokerTH-$1-windows-installer.exe .
        scp PokerTH-$1-windows-installer.exe PokerTH-$1-windows.zip lotodore,pokerth@frs.sourceforge.net:/home/frs/project/p/po/pokerth/pokerth/$2/
        md5sum PokerTH-$1-windows-installer.exe
        rm PokerTH-$1-windows-installer.exe
        md5sum PokerTH-$1-windows.zip
        rm PokerTH-$1-windows.zip
        rm -r ./PokerTH-$1*
        rm pokerth.exe
else
echo "Please use 'win-create-release.sh version sf-dir-version'"
fi

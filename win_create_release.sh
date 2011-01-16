#!/bin/bash
mkdir ./PokerTH
cp release/pokerth.exe ./PokerTH/
cp -R ./data ./PokerTH/
find ./PokerTH/ -name ".svn" | xargs rm -Rf 
zip -r PokerTH-windows.zip ./PokerTH

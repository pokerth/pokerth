#!/bin/bash
mkdir ./PokerTH
mv release/pokerth.exe ./PokerTH/
cp -R ./data ./PokerTH/
find ./PokerTH/ -name ".svn" -exec rm -rf {} \;
zip -r PokerTH-windows.zip ./PokerTH

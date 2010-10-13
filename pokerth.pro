# QMake pro-file for PokerTH

TEMPLATE = subdirs
SUBDIRS = pokerth_protocol.pro pokerth_db.pro pokerth_lib.pro pokerth_game.pro pokerth_server.pro chatcleaner.pro pokerth_tests.pro
CONFIG += ordered

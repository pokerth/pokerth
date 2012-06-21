# QMake pro-file for PokerTH

TEMPLATE = subdirs
SUBDIRS = pokerth_protocol.pro pokerth_db.pro pokerth_lib.pro pokerth_game.pro
!mac:!gui_800x480:!client {
	SUBDIRS += pokerth_server.pro chatcleaner.pro
}
CONFIG += ordered

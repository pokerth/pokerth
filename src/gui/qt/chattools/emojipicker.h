/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 *                                                                           *
 * Emoji-Picker-Popup für die Chat-Eingabefelder (Lobby, LAN-Lobby,          *
 * Gametable) sowie für die Emoji-Reaktionen am Spieltisch.                  *
 *****************************************************************************/
#ifndef EMOJIPICKER_H
#define EMOJIPICKER_H

#include <QtWidgets>

// Popup-Raster mit Emoji-Buttons. Ein Klick liefert das Emoji über das
// picked()-Signal und schließt das Popup. Ohne explizite Liste wird die
// umfangreiche Standard-Emoji-Auswahl angezeigt (wie im QML-Client).
class EmojiPicker : public QWidget
{
	Q_OBJECT

public:
	explicit EmojiPicker(QWidget *parent = nullptr,
	                     const QStringList &emojis = QStringList(),
	                     int columns = 10);

	// Popup unterhalb (bzw. oberhalb, falls kein Platz) des Ankers anzeigen.
	void showAt(QWidget *anchor);

	// Die 30 Schnell-Reaktionen (identisch zu QML-/Web-Client).
	static QStringList reactionEmojis();
	// Umfangreiche Standard-Auswahl für den Chat.
	static QStringList defaultEmojis();
	// Emoji als Icon rendern (für QLineEdit-Actions/Buttons).
	static QIcon emojiIcon(const QString &emoji, int size = 18);
	// Family-Name des gebündelten Farb-Emoji-Fonts (Noto Color Emoji,
	// CBDT/CBLC-Bitmap-Variante – rendert auch auf älterer FreeType, anders
	// als COLRv1). Registriert wird der Font in src/pokerth.cpp aus data/fonts/.
	static QString emojiFontFamily();
	// Emoji in GARANTIERTER Zielgröße rendern. Farb-Emoji-Fonts sind
	// Bitmap-Fonts, deren Glyphen Qt beim Text-Rendering nicht hochskaliert –
	// hier wird gerendert, der tatsächlich gezeichnete Bereich gemessen und
	// auf targetPx skaliert.
	static QPixmap emojiPixmap(const QString &emoji, int targetPx);

signals:
	void picked(const QString &emoji);

private:
	void buildGrid(const QStringList &emojis, int columns);
};

#endif // EMOJIPICKER_H

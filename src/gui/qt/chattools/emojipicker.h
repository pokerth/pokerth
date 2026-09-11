/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 *                                                                           *
 * Emoji picker popup for the chat input fields (lobby, LAN lobby,          *
 * game table) as well as for the emoji reactions at the game table.        *
 *****************************************************************************/
#ifndef EMOJIPICKER_H
#define EMOJIPICKER_H

#include <QtWidgets>

// Popup grid with emoji buttons. A click delivers the emoji via the
// picked() signal and closes the popup. Without an explicit list the
// comprehensive default emoji selection is shown (as in the QML client).
class EmojiPicker : public QWidget
{
	Q_OBJECT

public:
	explicit EmojiPicker(QWidget *parent = nullptr,
						 const QStringList &emojis = QStringList(),
						 int columns = 10);

	// Reaction picker: the 90 quick reactions on three pages, plus a
	// pager ‹ N/3 › in the header (identical to the QML/web client). startPage is
	// the page used last, pageChanged reports every page change.
	static EmojiPicker *createReactionPicker(QWidget *parent, int startPage);

	// Show the popup below (or above, if there is no room) the anchor.
	void showAt(QWidget *anchor);

	// The 90 quick reactions, three pages of 30 (identical to the QML/web client).
	static QList<QStringList> reactionEmojiPages();
	// Comprehensive default selection for the chat.
	static QStringList defaultEmojis();
	// Render an emoji as an icon (for QLineEdit actions/buttons).
	static QIcon emojiIcon(const QString &emoji, int size = 18);
	// Family name of the bundled colour emoji font (Noto Color Emoji,
	// CBDT/CBLC bitmap variant – renders on older FreeType as well, unlike
	// COLRv1). The font is registered in src/pokerth.cpp from data/fonts/.
	static QString emojiFontFamily();
	// Render an emoji in a GUARANTEED target size. Colour emoji fonts are
	// bitmap fonts whose glyphs Qt does not scale up when rendering text –
	// here it is rendered, the area actually drawn is measured and scaled
	// to targetPx.
	static QPixmap emojiPixmap(const QString &emoji, int targetPx);

signals:
	void picked(const QString &emoji);
	void pageChanged(int page);

private:
	// Constructor of the multi-page reaction picker (see
	// createReactionPicker).
	EmojiPicker(QWidget *parent, const QList<QStringList> &pages,
				int columns, int startPage);

	void buildGrid(const QStringList &emojis, int columns);
	// Multi-page grid with a pager in the header (reaction picker).
	void buildPages(const QList<QStringList> &pages, int columns, int startPage);
	QWidget *buildGridWidget(const QStringList &emojis, int columns, QWidget *parent);
	void setCurrentPage(int page);

	QStackedWidget *myPageStack = nullptr;
	QLabel *myPageIcon = nullptr;
	QLabel *myPageIndicator = nullptr;
	int myCurrentPage = 0;
};

#endif // EMOJIPICKER_H

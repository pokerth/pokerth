/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 *                                                                           *
 * Emoji-Picker-Popup für die Chat-Eingabefelder (Lobby, LAN-Lobby,          *
 * Gametable) sowie für die Emoji-Reaktionen am Spieltisch.                  *
 *****************************************************************************/
#include "emojipicker.h"
#include <QGuiApplication>
#include <QInputMethod>

EmojiPicker::EmojiPicker(QWidget *parent, const QStringList &emojis, int columns)
	: QWidget(parent, Qt::Popup)
{
	setAttribute(Qt::WA_DeleteOnClose, false);
#ifdef Q_OS_ANDROID
	// Der Picker selbst soll keinen Tastatur-Fokus ziehen – sonst öffnet
	// sich auf Touch-Geräten die virtuelle Tastatur.
	setFocusPolicy(Qt::NoFocus);
#endif
	buildGrid(emojis.isEmpty() ? defaultEmojis() : emojis, columns);
}

EmojiPicker::EmojiPicker(QWidget *parent, const QList<QStringList> &pages,
                         int columns, int startPage)
	: QWidget(parent, Qt::Popup)
{
	setAttribute(Qt::WA_DeleteOnClose, false);
#ifdef Q_OS_ANDROID
	setFocusPolicy(Qt::NoFocus);
#endif
	buildPages(pages, columns, startPage);
}

EmojiPicker *EmojiPicker::createReactionPicker(QWidget *parent, int startPage)
{
	return new EmojiPicker(parent, reactionEmojiPages(), 6, startPage);
}

void EmojiPicker::buildGrid(const QStringList &emojis, int columns)
{
	QVBoxLayout *outer = new QVBoxLayout(this);
	outer->setContentsMargins(4, 4, 4, 4);

	QScrollArea *scroll = new QScrollArea(this);
	scroll->setWidgetResizable(true);
	scroll->setFrameShape(QFrame::NoFrame);
	scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	scroll->setWidget(buildGridWidget(emojis, columns, scroll));
	outer->addWidget(scroll);

	const int cell = 48;
	const int gridW = columns * cell + 24;
	const int rows = (emojis.size() + columns - 1) / columns;
	const int gridH = qMin(rows * cell + 12, 5 * cell + 12);
	setFixedSize(gridW, gridH);
}

QWidget *EmojiPicker::buildGridWidget(const QStringList &emojis, int columns, QWidget *parent)
{
	QWidget *grid = new QWidget(parent);
	QGridLayout *gl = new QGridLayout(grid);
	gl->setContentsMargins(0, 0, 0, 0);
	gl->setSpacing(2);

	int row = 0, col = 0;
	for (const QString &e : emojis) {
		QToolButton *btn = new QToolButton(grid);
		// Als Icon in fester Pixelgröße rendern – setFont skaliert
		// Bitmap-Emoji-Glyphen nicht (sie blieben winzig).
		btn->setIcon(QIcon(emojiPixmap(e, 32)));
		btn->setIconSize(QSize(32, 32));
		btn->setAutoRaise(true);
		btn->setFixedSize(46, 46);
#ifdef Q_OS_ANDROID
		// Auf Touch-Geräten keinen Fokus ziehen → die virtuelle Tastatur
		// poppt beim Antippen nicht auf.
		btn->setFocusPolicy(Qt::NoFocus);
#endif
		btn->setCursor(Qt::PointingHandCursor);
		connect(btn, &QToolButton::clicked, this, [this, e]() {
			emit picked(e);
			hide();
		});
		gl->addWidget(btn, row, col);
		if (++col >= columns) {
			col = 0;
			++row;
		}
	}
	return grid;
}

// Mehrseitiges Raster mit kompaktem Pager ‹ 😀 1/3 › im Kopf – wie der
// Reaktions-Picker des Web-Clients; die Pfeile laufen um.
void EmojiPicker::buildPages(const QList<QStringList> &pages, int columns, int startPage)
{
	QVBoxLayout *outer = new QVBoxLayout(this);
	outer->setContentsMargins(4, 4, 4, 4);
	outer->setSpacing(4);

	QHBoxLayout *pager = new QHBoxLayout();
	pager->setContentsMargins(0, 0, 0, 0);
	pager->setSpacing(6);
	pager->addStretch();

	QToolButton *prev = new QToolButton(this);
	prev->setText(QString::fromUtf8("‹"));
	prev->setAutoRaise(true);
	prev->setFixedSize(22, 22);
	prev->setFocusPolicy(Qt::NoFocus);
	prev->setCursor(Qt::PointingHandCursor);
	connect(prev, &QToolButton::clicked, this, [this]() { setCurrentPage(myCurrentPage - 1); });
	pager->addWidget(prev);

	myPageIcon = new QLabel(this);
	pager->addWidget(myPageIcon);
	myPageIndicator = new QLabel(this);
	pager->addWidget(myPageIndicator);

	QToolButton *next = new QToolButton(this);
	next->setText(QString::fromUtf8("›"));
	next->setAutoRaise(true);
	next->setFixedSize(22, 22);
	next->setFocusPolicy(Qt::NoFocus);
	next->setCursor(Qt::PointingHandCursor);
	connect(next, &QToolButton::clicked, this, [this]() { setCurrentPage(myCurrentPage + 1); });
	pager->addWidget(next);

	pager->addStretch();
	outer->addLayout(pager);

	myPageStack = new QStackedWidget(this);
	for (const QStringList &page : pages)
		myPageStack->addWidget(buildGridWidget(page, columns, myPageStack));
	outer->addWidget(myPageStack);

	const int cell = 48;
	int rows = 0;
	for (const QStringList &page : pages)
		rows = qMax(rows, (page.size() + columns - 1) / columns);
	setFixedSize(columns * cell + 24, rows * cell + 12 + 30);

	myCurrentPage = -1;
	setCurrentPage(startPage);
}

void EmojiPicker::setCurrentPage(int page)
{
	if (!myPageStack || myPageStack->count() == 0)
		return;
	const int count = myPageStack->count();
	page = ((page % count) + count) % count;   // die Pfeile laufen um
	if (page == myCurrentPage)
		return;
	myCurrentPage = page;
	myPageStack->setCurrentIndex(page);
	// Symbol der Seite (Emotionen / Stimmung & Gesten / Poker & Glück).
	static const char *icons[] = {"😀", "👏", "♠️"};
	if (myPageIcon && page < 3)
		myPageIcon->setPixmap(emojiPixmap(QString::fromUtf8(icons[page]), 16));
	if (myPageIndicator)
		myPageIndicator->setText(QString("%1/%2").arg(page + 1).arg(count));
	emit pageChanged(page);
}

void EmojiPicker::showAt(QWidget *anchor)
{
#ifdef Q_OS_ANDROID
	// Virtuelle Tastatur ausblenden, damit der Picker klar vom Texteingabe-
	// Feld abgegrenzt ist und die Tastatur nicht darüber liegt (Android).
	if (QGuiApplication::inputMethod())
		QGuiApplication::inputMethod()->hide();
#endif

	QPoint below = anchor->mapToGlobal(QPoint(0, anchor->height() + 4));
	QScreen *screen = anchor->screen();
	if (screen && below.y() + height() > screen->availableGeometry().bottom())
		below = anchor->mapToGlobal(QPoint(0, -height() - 4));
	// horizontal im Bildschirm halten
	if (screen && below.x() + width() > screen->availableGeometry().right())
		below.setX(screen->availableGeometry().right() - width());
	move(below);
	show();
}

QList<QStringList> EmojiPicker::reactionEmojiPages()
{
	// Identisch zum Reaktions-Katalog in QML- und Web-Client:
	// 90 Reaktionen auf drei thematischen Seiten.
	return {
		// Seite 1 – Emotionen
		QStringList{
			"😂", "🤣", "😅", "😭", "🥺", "😢",
			"😏", "🙄", "😳", "🤪", "😇", "😍",
			"🥰", "😘", "😬", "😴", "🤔", "👀",
			"😮", "😱", "🤯", "😡", "😤", "🤢",
			"🥴", "🙃", "🫣", "😐", "🥱", "🙈"
		},
		// Seite 2 – Stimmung & Gesten
		QStringList{
			"😎", "🤩", "🤡", "😈", "🫠", "🥶",
			"🥵", "🎉", "🥳", "🍿", "👏", "🙌",
			"💪", "👍", "👎", "🤝", "👊", "🙏",
			"🤞", "🫵", "🫡", "🤫", "🤦", "🚬",
			"⏳", "🍺", "☕", "💣", "🚀", "⚡"
		},
		// Seite 3 – Poker & Glück
		QStringList{
			"💰", "🤑", "💵", "💎", "🎰", "🍀",
			"🃏", "♠️", "🎲", "🎯", "🏆", "🥇",
			"💸", "🪤", "👑", "🔥", "💀", "🦈",
			"🐟", "🐔", "🫏", "🎩", "🧊", "🌪️",
			"🔫", "📈", "📉", "🔮", "💯", "⭐"
		}
	};
}

QStringList EmojiPicker::defaultEmojis()
{
	// Gängige Auswahl (Smileys, Gesten, Symbole, Poker) – kompaktere
	// Variante der QML-EmojiPicker-Liste.
	return {
		"😀", "😃", "😄", "😁", "😆", "😅", "🤣", "😂", "🙂", "🙃",
		"😉", "😊", "😇", "🥰", "😍", "🤩", "😘", "😋", "😛", "😜",
		"🤪", "😝", "🤑", "🤗", "🤭", "🤫", "🤔", "🤐", "🤨", "😐",
		"😏", "😒", "🙄", "😬", "🤥", "😌", "😔", "😪", "🤤", "😴",
		"😷", "🤒", "🥵", "🥶", "🥴", "😵", "🤯", "🤠", "🥳", "😎",
		"🤓", "🧐", "😕", "😟", "🙁", "😮", "😯", "😲", "😳", "🥺",
		"😦", "😨", "😰", "😥", "😢", "😭", "😱", "😖", "😣", "😞",
		"😓", "😩", "😫", "🥱", "😤", "😡", "😠", "🤬", "😈", "👿",
		"💀", "💩", "🤡", "👻", "👽", "🤖", "😺", "😹", "😻", "🙀",
		"👋", "✋", "👌", "✌️", "🤞", "🤟", "🤘", "🤙", "👈", "👉",
		"👆", "👇", "👍", "👎", "✊", "👊", "👏", "🙌", "🤝", "🙏",
		"💪", "👀", "🧠", "💋", "💘", "💖", "💕", "💔", "❤️", "🧡",
		"💛", "💚", "💙", "💜", "🖤", "🤍", "💯", "💢", "💥", "💫",
		"💦", "💨", "💬", "💭", "💤", "🔥", "✨", "⭐", "🌟", "⚡",
		"🌈", "☀️", "❄️", "💧", "🍀", "🌹", "🌻", "🍺", "🍻", "🥂",
		"🍷", "🥃", "☕", "🍕", "🍔", "🍟", "🌮", "🍿", "🎂", "🍰",
		"🎉", "🥳", "🎊", "🎁", "🏆", "🥇", "🥈", "🥉", "🎯", "🎲",
		"🎰", "🃏", "💰", "💵", "🪙", "💎", "👑", "🫵", "🫡", "🤌"
	};
}

QIcon EmojiPicker::emojiIcon(const QString &emoji, int size)
{
	return QIcon(emojiPixmap(emoji, size));
}

QString EmojiPicker::emojiFontFamily()
{
	return QStringLiteral("Noto Color Emoji");
}

QPixmap EmojiPicker::emojiPixmap(const QString &emoji, int targetPx)
{
	// Auf großzügiger Leinwand rendern …
	QFont f;
	// Explizit den gebündelten Farb-Emoji-Font verwenden, damit Emojis nicht
	// vom (auf manchen Distributionen fehlenden/zu alten) System-Emoji-Font
	// abhängen und als weiße Rechtecke erscheinen.
	f.setFamily(emojiFontFamily());
	f.setPixelSize(targetPx);
	const int canvas = qMax(targetPx * 3, 64);
	QImage img(canvas, canvas, QImage::Format_ARGB32_Premultiplied);
	img.fill(Qt::transparent);
	{
		QPainter p(&img);
		p.setFont(f);
		p.setPen(Qt::white);
		p.drawText(QRect(0, 0, canvas, canvas), Qt::AlignCenter, emoji);
	}

	// … den tatsächlich gezeichneten Bereich messen (Bitmap-Emoji-Glyphen
	// erscheinen unabhängig von der Font-Größe in ihrer nativen Größe) …
	int minX = canvas, minY = canvas, maxX = -1, maxY = -1;
	for (int y = 0; y < canvas; ++y) {
		const QRgb *line = reinterpret_cast<const QRgb *>(img.constScanLine(y));
		for (int x = 0; x < canvas; ++x) {
			if (qAlpha(line[x]) > 8) {
				if (x < minX) minX = x;
				if (x > maxX) maxX = x;
				if (y < minY) minY = y;
				if (y > maxY) maxY = y;
			}
		}
	}
	if (maxX < 0)
		return QPixmap();

	// … und auf die GARANTIERTE Zielgröße skalieren.
	const QImage cropped = img.copy(QRect(QPoint(minX, minY), QPoint(maxX, maxY)));
	return QPixmap::fromImage(cropped.scaled(targetPx, targetPx,
	                                         Qt::KeepAspectRatio,
	                                         Qt::SmoothTransformation));
}

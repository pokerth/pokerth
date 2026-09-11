/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 *                                                                           *
 * Emoji picker popup for the chat input fields (lobby, LAN lobby,          *
 * game table) as well as for the emoji reactions at the game table.        *
 *****************************************************************************/
#include "emojipicker.h"
#include <QGuiApplication>
#include <QInputMethod>

EmojiPicker::EmojiPicker(QWidget *parent, const QStringList &emojis, int columns)
	: QWidget(parent, Qt::Popup)
{
	setAttribute(Qt::WA_DeleteOnClose, false);
#ifdef Q_OS_ANDROID
	// The picker itself should not take the keyboard focus – otherwise the
	// virtual keyboard opens on touch devices.
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
		// Render as an icon in a fixed pixel size – setFont does not scale
		// bitmap emoji glyphs (they would stay tiny).
		btn->setIcon(QIcon(emojiPixmap(e, 32)));
		btn->setIconSize(QSize(32, 32));
		btn->setAutoRaise(true);
		btn->setFixedSize(46, 46);
#ifdef Q_OS_ANDROID
		// Do not take the focus on touch devices → the virtual keyboard
		// does not pop up when tapping.
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

// Multi-page grid with a compact pager ‹ 😀 1/3 › in the header – like the
// reaction picker of the web client; the arrows wrap around.
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
	connect(prev, &QToolButton::clicked, this, [this]() {
		setCurrentPage(myCurrentPage - 1);
	});
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
	connect(next, &QToolButton::clicked, this, [this]() {
		setCurrentPage(myCurrentPage + 1);
	});
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
	page = ((page % count) + count) % count;   // the arrows wrap around
	if (page == myCurrentPage)
		return;
	myCurrentPage = page;
	myPageStack->setCurrentIndex(page);
	// Symbol of the page (emotions / mood & gestures / poker & luck).
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
	// Hide the virtual keyboard so that the picker is clearly separated from the
	// text input field and the keyboard does not lie above it (Android).
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
	// Identical to the reaction catalogue in the QML and web client:
	// 90 reactions on three thematic pages.
	return {
		// Seite 1 – Emotionen
		QStringList{
			"😂", "🤣", "😅", "😭", "🥺", "😢",
			"😏", "🙄", "😳", "🤪", "😇", "😍",
			"🥰", "😘", "😬", "😴", "🤔", "👀",
			"😮", "😱", "🤯", "😡", "😤", "🤢",
			"🥴", "🙃", "🫣", "😐", "🥱", "🙈"
		},
		// Page 2 – mood & gestures
		QStringList{
			"😎", "🤩", "🤡", "😈", "🫠", "🥶",
			"🥵", "🎉", "🥳", "🍿", "👏", "🙌",
			"💪", "👍", "👎", "🤝", "👊", "🙏",
			"🤞", "🫵", "🫡", "🤫", "🤦", "🚬",
			"⏳", "🍺", "☕", "💣", "🚀", "⚡"
		},
		// Page 3 – poker & luck
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
	// Common selection (smileys, gestures, symbols, poker) – a more compact
	// variant of the QML EmojiPicker list.
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
	// Render on a generous canvas …
	QFont f;
	// Use the bundled colour emoji font explicitly so that emojis do not depend
	// on the system emoji font (missing/too old on some distributions) and
	// appear as white rectangles.
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

	// … measure the area actually drawn (bitmap emoji glyphs appear in their
	// native size regardless of the font size) …
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

	// … and scale it to the GUARANTEED target size.
	const QImage cropped = img.copy(QRect(QPoint(minX, minY), QPoint(maxX, maxY)));
	return QPixmap::fromImage(cropped.scaled(targetPx, targetPx,
							  Qt::KeepAspectRatio,
							  Qt::SmoothTransformation));
}

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

void EmojiPicker::buildGrid(const QStringList &emojis, int columns)
{
	QVBoxLayout *outer = new QVBoxLayout(this);
	outer->setContentsMargins(4, 4, 4, 4);

	QScrollArea *scroll = new QScrollArea(this);
	scroll->setWidgetResizable(true);
	scroll->setFrameShape(QFrame::NoFrame);
	scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	QWidget *grid = new QWidget(scroll);
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

	scroll->setWidget(grid);
	outer->addWidget(scroll);

	const int cell = 48;
	const int gridW = columns * cell + 24;
	const int rows = (emojis.size() + columns - 1) / columns;
	const int gridH = qMin(rows * cell + 12, 5 * cell + 12);
	setFixedSize(gridW, gridH);
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

QStringList EmojiPicker::reactionEmojis()
{
	// Identisch zur Reaktions-Liste in QML- und Web-Client.
	return {
		"🎉", "🥳", "👏", "🙌", "💪", "🤣",
		"😂", "😬", "🤦", "😴", "👍", "😎",
		"🤩", "👀", "🤔", "😱", "😡", "😤",
		"🔥", "😮", "💰", "💎", "🎰", "🍀",
		"🃏", "💀", "🤑", "🫵", "🫡", "🤫"
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

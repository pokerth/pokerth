/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2012 Felix Hammer, Florian Thauer, Lothar May          *
 *                                                                           *
 * This program is free software: you can redistribute it and/or modify      *
 * it under the terms of the GNU Affero General Public License as            *
 * published by the Free Software Foundation, either version 3 of the        *
 * License, or (at your option) any later version.                           *
 *                                                                           *
 * This program is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU Affero General Public License for more details.                       *
 *                                                                           *
 * You should have received a copy of the GNU Affero General Public License  *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.     *
 *                                                                           *
 *                                                                           *
 * Additional permission under GNU AGPL version 3 section 7                  *
 *                                                                           *
 * If you modify this program, or any covered work, by linking or            *
 * combining it with the OpenSSL project's OpenSSL library (or a             *
 * modified version of that library), containing parts covered by the        *
 * terms of the OpenSSL or SSLeay licenses, the authors of PokerTH           *
 * (Felix Hammer, Florian Thauer, Lothar May) grant you additional           *
 * permission to convey the resulting work.                                  *
 * Corresponding Source for a non-source form of such a combination          *
 * shall include the source code for the parts of OpenSSL used as well       *
 * as that of the covered work.                                              *
 *****************************************************************************/
#include "chattools.h"
#include "emojipicker.h"
#include "gui/chat_emote_shortcuts.h"
#include "chattranslatorcore.h"
#include <QProxyStyle>
#include <QDateTime>
#include <QDesktopServices>
#include "core/appimage_utils.h"
#include <QTextDocument>
#include <QTextBlock>
#include <QMouseEvent>
#include <QScrollBar>
#include <QCursor>
#include "session.h"
#include "configfile.h"
#include "gametablestylereader.h"
#include "gamelobbydialogimpl.h"
#include "soundevents.h"
#include <cmath>
#include <iostream>


using namespace std;

namespace
{

// QLineEdit draws trailing action icons in PM_SmallIconSize (by default
// ~16px) – independently of the resolution of the icon that is passed. This small
// proxy style raises only this measure for the respective input field, so that the
// emoji trigger icons (🙂/🎉) become larger and easier to tap.
class BiggerActionIconStyle : public QProxyStyle
{
public:
	explicit BiggerActionIconStyle(int iconSize) : myIconSize(iconSize) {}
	int pixelMetric(PixelMetric metric, const QStyleOption *option = nullptr,
					const QWidget *widget = nullptr) const override
	{
		if (metric == QStyle::PM_SmallIconSize)
			return myIconSize;
		return QProxyStyle::pixelMetric(metric, option, widget);
	}
private:
	int myIconSize;
};

// ── Colour of the player name in the chat ───────────────────────────────────
// Relative luminance and contrast ratio according to WCAG 2.1. With them the
// name colour is checked against the ACTUAL background of the respective history
// instead of assuming a light or a dark surface.
double srgbToLinear(double c)
{
	return c <= 0.03928 ? c / 12.92 : std::pow((c + 0.055) / 1.055, 2.4);
}

double relativeLuminance(const QColor &c)
{
	return 0.2126 * srgbToLinear(c.redF())
		   + 0.7152 * srgbToLinear(c.greenF())
		   + 0.0722 * srgbToLinear(c.blueF());
}

double contrastRatio(const QColor &a, const QColor &b)
{
	const double la = relativeLuminance(a);
	const double lb = relativeLuminance(b);
	return (qMax(la, lb) + 0.05) / (qMin(la, lb) + 0.05);
}

// The name colour for a given background. Violet, because this hue differs from
// all the other colours assigned in the chat: grey/white
// message text, blue links and own lines (the palette link), the red
// bot warning and the yellow nick notification of the table styles. The
// brightness is pushed away from the background until the contrast
// is sufficient – that way the name stays readable on a light theme, a dark theme and on any
// (freely choosable) table style felt.
QColor nickColorForBackground(const QColor &bg)
{
	const double hue = 285.0 / 360.0;
	const double sat = 0.72;
	const bool darkBg = relativeLuminance(bg) < 0.18;

	double lightness = darkBg ? 0.70 : 0.42;
	QColor nick = QColor::fromHslF(hue, sat, lightness);
	// Brighten or darken up to the target contrast (4.5:1, WCAG AA for normal
	// text). The bounds prevent an endless loop with extreme
	// backgrounds (e.g. a medium grey, where 4.5:1 is not reachable with this
	// saturation) – there it stays at the maximum possible contrast.
	for(int i = 0; i < 30 && contrastRatio(nick, bg) < 4.5; ++i) {
		lightness += darkBg ? 0.02 : -0.02;
		if(lightness > 0.92 || lightness < 0.20)
			break;
		nick = QColor::fromHslF(hue, sat, lightness);
	}
	return nick;
}

bool isEmojiCodepoint(uint cp)
{
	return (cp >= 0x1F000 && cp <= 0x1FAFF)   // Emojis, Symbole, Erweiterungen
		   || (cp >= 0x2600 && cp <= 0x27BF)  // Misc Symbols, Dingbats
		   || (cp >= 0x2B00 && cp <= 0x2BFF)  // ⭐ u. a.
		   || cp == 0x2764 || cp == 0x203C || cp == 0x2049
		   || (cp >= 0x1F1E6 && cp <= 0x1F1FF); // Flaggen
}

// Checks whether a reaction payload ("/emoji <x>") consists exclusively of real
// emoji characters (incl. variation selectors, ZWJ and skin tone modifiers)
// and contains at least one emoji. That way text messages disguised as a
// reaction ("/emoji haha") are discarded, while arbitrary real emojis are
// allowed.
bool isEmojiOnlyReaction(const QString &text)
{
	if (text.isEmpty())
		return false;
	bool hasEmoji = false;
	int i = 0;
	while (i < text.size()) {
		const QChar ch = text.at(i);
		uint cp = ch.unicode();
		int len = 1;
		if (ch.isHighSurrogate() && i + 1 < text.size()) {
			cp = QChar::surrogateToUcs4(ch, text.at(i + 1));
			len = 2;
		}
		const bool joiner = cp == 0x200D || cp == 0x20E3
							|| (cp >= 0xFE00 && cp <= 0xFE0F)
							|| (cp >= 0x1F3FB && cp <= 0x1F3FF);
		if (isEmojiCodepoint(cp))
			hasEmoji = true;
		else if (!joiner)
			return false;   // a letter/digit/punctuation mark → no reaction
		i += len;
	}
	return hasEmoji;
}

// Enlarge Unicode emojis in the (HTML) chat text: every emoji run (incl.
// ZWJ sequences, variation selectors and skin tone modifiers) is wrapped into a
// font-size span. Content inside HTML tags stays
// untouched.
QString wrapEmojisLarger(const QString &msg, int pixelSize)
{
	QString out;
	out.reserve(msg.size() + 64);
	bool inTag = false;
	int i = 0;
	while (i < msg.size()) {
		const QChar ch = msg.at(i);
		if (ch == QLatin1Char('<')) inTag = true;
		else if (ch == QLatin1Char('>')) inTag = false;
		if (inTag || ch == QLatin1Char('>')) {
			out += ch;
			++i;
			continue;
		}
		uint cp = ch.unicode();
		int len = 1;
		if (ch.isHighSurrogate() && i + 1 < msg.size()) {
			cp = QChar::surrogateToUcs4(ch, msg.at(i + 1));
			len = 2;
		}
		if (isEmojiCodepoint(cp)) {
			const int start = i;
			while (i < msg.size()) {
				const QChar c2 = msg.at(i);
				uint cp2 = c2.unicode();
				int l2 = 1;
				if (c2.isHighSurrogate() && i + 1 < msg.size()) {
					cp2 = QChar::surrogateToUcs4(c2, msg.at(i + 1));
					l2 = 2;
				}
				const bool joiner = cp2 == 0xFE0F || cp2 == 0x200D
									|| (cp2 >= 0x1F3FB && cp2 <= 0x1F3FF);
				if (!isEmojiCodepoint(cp2) && !joiner)
					break;
				i += l2;
			}
			out += QStringLiteral("<span style=\"font-size:%1px; font-family:'%2';\">")
				   .arg(pixelSize).arg(EmojiPicker::emojiFontFamily())
				   + msg.mid(start, i - start) + QStringLiteral("</span>");
		} else {
			out += msg.mid(i, len);
			i += len;
		}
	}
	return out;
}

} // namespace


// The translation symbols. 🌐 = clickable, ⏳ = running. Built via the code point
// (not via a "\xF0…" literal – that would be read as Latin-1 and yield mojibake).
static const QString kTranslateGlobe   = QString::fromUcs4(U"\U0001F310"); // 🌐
static const QString kTranslateSpinner = QString::fromUcs4(U"\U000023F3"); // ⏳
// An invisible placeholder for lines that are not under the mouse cursor:
// the anchor thus stays in the document (and the line findable through it), but shows
// nothing. A non-breaking space, because normal spaces at the end of a line
// are dropped during the HTML import – without a fragment there would be no anchor any more.
static const QString kTranslateHidden  = QStringLiteral("&nbsp;");

// The upper limit for the chat history (text blocks in the document). Without a limit
// the QTextDocument grew over the whole session: the memory, the layout and the linear
// block search of the translate symbol (findTranslateBlock, twice per line
// under the mouse cursor) got more and more expensive. The same value as in the
// QML client (LobbyHandler::pushChatLine).
static const int kMaxChatBlocks = 400;

// Hover mode: on the desktop the globe only appears on the line under the
// mouse cursor (otherwise the history was plastered with symbols). On touch
// devices there is no hover – there the symbols stay visible, otherwise the
// function would no longer be reachable.
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
static const bool kTranslateHoverOnly = false;
#else
static const bool kTranslateHoverOnly = true;
#endif

ChatTools::ChatTools(QLineEdit* l, ConfigFile *c, ChatType ct, QTextBrowser *b, QStandardItemModel *m, gameLobbyDialogImpl *lo) : nickAutoCompletitionCounter(0), myLineEdit(l), myNickListModel(m), myNickStringList(nullptr), myTextBrowser(b), myChatType(ct), myConfig(c), myNick(""), myStyle(nullptr), myLobby(lo), myEmojiPicker(nullptr), myShortcodeCompleter(nullptr), myShortcodeModel(nullptr), myShortcodeTokenStart(-1), myTranslator(nullptr), myTranslateNextId(1), myTranslateHoverId(0), myTranslateLastFailNoteMs(0)
{
	myNick = QString::fromUtf8(myConfig->readConfigString("MyName").c_str());
	setupEmojiPickerAction();
	setupShortcodeCompleter();

	// Chat translation. Otherwise the QTextBrowser navigates by itself on a click
	// (openExternalLinks in the .ui); we switch openLinks off and handle
	// the clicks ourselves: our pseudo scheme translates, http(s) opens externally (as
	// before via openExternalLinks). That way the external opening is kept and
	// our globe link triggers no document navigation.
	myTranslator = new ChatTranslatorCore(myConfig, this);
	connect(myTranslator, &ChatTranslatorCore::translated, this, &ChatTools::onChatTranslated);
	if(myTextBrowser) {
		// Limit the length of the history (see kMaxChatBlocks). The history is
		// read-only, the undo history that is switched off with this is not
		// needed here.
		myTextBrowser->document()->setMaximumBlockCount(kMaxChatBlocks);
		// With bundled libs (AppImage/tarball/deb) the dialog constructor has
		// called AppImageUtils::patchExternalLinks() before; that attaches an
		// anchorClicked handler of its own to EVERY browser with openExternalLinks,
		// which passes every URL on to the desktop. On a globe click
		// "pokerthtranslate:<id>" thus ended up at xdg-open ("Failed to open URL"), real links
		// were opened twice. For this history WE are the only
		// link handler: dissolve foreign connections, switch openExternalLinks off
		// (that makes patchExternalLinks skip the history in the future as well).
		QObject::disconnect(myTextBrowser, SIGNAL(anchorClicked(QUrl)), nullptr, nullptr);
		myTextBrowser->setOpenExternalLinks(false);
		myTextBrowser->setOpenLinks(false);
		connect(myTextBrowser, &QTextBrowser::anchorClicked, this, &ChatTools::onChatAnchorClicked);
		// The globe symbol should only stand on the line UNDER THE MOUSE CURSOR
		// (otherwise the history is plastered with symbols). For that, read
		// the mouse movements over the history along; the QTextEdit viewport has mouse
		// tracking on for the link detection anyway.
		myTextBrowser->viewport()->setMouseTracking(true);
		myTextBrowser->viewport()->installEventFilter(this);
		// While scrolling, the lines travel under the (standing) mouse cursor
		// – the symbol has to follow the new line.
		connect(myTextBrowser->verticalScrollBar(), &QAbstractSlider::valueChanged,
		this, [this]() {
			if(myTextBrowser && myTextBrowser->viewport()->underMouse())
				updateTranslateHover(myTextBrowser->viewport()->mapFromGlobal(QCursor::pos()));
		});
	}
}

void ChatTools::setupEmojiPickerAction()
{
	if (!myLineEdit)
		return;

	// Enlarge the trigger icons in the input field. The proxy style applies to this
	// input field (and thereby to the 🎉 reaction icon as well, which the game table
	// puts into the same field on the desktop). The size depends on the platform.
#ifdef Q_OS_ANDROID
	const int triggerIconSize = 30;
#else
	const int triggerIconSize = 22;
#endif
	BiggerActionIconStyle *iconStyle = new BiggerActionIconStyle(triggerIconSize);
	iconStyle->setParent(myLineEdit);   // Couple the lifetime to the input field
	myLineEdit->setStyle(iconStyle);

	// The emoji picker button in the input field (on the right) – uniform for the
	// internet lobby, the LAN lobby and the game table chat.
	QAction *emojiAction = myLineEdit->addAction(EmojiPicker::emojiIcon(QStringLiteral("🙂"), triggerIconSize),
						   QLineEdit::TrailingPosition);
	emojiAction->setToolTip(tr("Insert emoji"));
	QObject::connect(emojiAction, &QAction::triggered, this, [this]() {
		if (!myEmojiPicker) {
			myEmojiPicker = new EmojiPicker(myLineEdit);
			QObject::connect(myEmojiPicker, &EmojiPicker::picked, this, [this](const QString &e) {
				myLineEdit->insert(e);
#ifndef Q_OS_ANDROID
				// Do NOT refocus on Android – that would make the
				// virtual keyboard pop up again.
				myLineEdit->setFocus();
#endif
			});
		}
		myEmojiPicker->showAt(myLineEdit);
	});
}

// Auto-completion for emoji shortcodes (":smi…" → 😄), like the ChatBox
// of the QML client. The suggestions come from the same map that replaces them when
// displaying (chat_emote_shortcuts.h) – only what really works is
// offered. The QCompleter only delivers the popup, the keyboard navigation
// (up/down/enter/esc) and activated(); filtering and sorting is done by ourselves
// (UnfilteredPopupCompletion), so that prefix matches stand before substring matches.
void ChatTools::setupShortcodeCompleter()
{
	if (!myLineEdit)
		return;

	const QHash<QString, QString> &map = chatEmoteShortcodeMap();
	QStringList codes = map.keys();
	codes.sort();
	myShortcodeList.reserve(codes.size());
	QStringListIterator it(codes);
	while (it.hasNext()) {
		const QString code = it.next();
		myShortcodeList.append(qMakePair(code, map.value(code)));
	}

	myShortcodeModel = new QStandardItemModel(this);
	myShortcodeCompleter = new QCompleter(myShortcodeModel, this);
	myShortcodeCompleter->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
	myShortcodeCompleter->setMaxVisibleItems(8);
	// Deliberately setWidget() instead of QLineEdit::setCompleter(): the latter would replace the
	// WHOLE line when accepting – here only the token is replaced.
	myShortcodeCompleter->setWidget(myLineEdit);
	QObject::connect(myShortcodeCompleter, QOverload<const QModelIndex &>::of(&QCompleter::activated),
					 this, &ChatTools::insertShortcodeCompletion);
	// Tab should accept the highlighted suggestion (as in the QML client) –
	// a filter of our own on the popup makes that deterministic (see eventFilter).
	myShortcodeCompleter->popup()->installEventFilter(this);

	// textEdited instead of textChanged: a programmatic setText (a history recall,
	// a length truncation) should not open the popup.
	QObject::connect(myLineEdit, &QLineEdit::textEdited,
					 this, &ChatTools::updateShortcodeCompletion);
	// A cursor movement can change the token under the cursor – but only re-evaluate it
	// when the popup is already open (setText moves the cursor as well).
	QObject::connect(myLineEdit, &QLineEdit::cursorPositionChanged,
	this, [this](int, int) {
		if (shortcodeCompletionActive())
			updateShortcodeCompletion();
	});
}

bool ChatTools::shortcodeCompletionActive() const
{
	return myShortcodeCompleter && myShortcodeCompleter->popup()->isVisible();
}

void ChatTools::updateShortcodeCompletion()
{
	if (!myShortcodeCompleter)
		return;
	const QString upto = myLineEdit->text().left(myLineEdit->cursorPosition());
	// The token = ":" (at the beginning or after a space) + at least 2 code
	// characters directly before the cursor (as in ChatBox.qml): that way the
	// ASCII shortcuts (":P", ":D") do not collide with the popup, and the closing ":"
	// of a fully typed shortcode closes the popup by itself.
	static const QRegularExpression tokenRe(QStringLiteral("(?:^|\\s):([a-z0-9_+-]{2,})$"));
	const QRegularExpressionMatch match = tokenRe.match(upto);
	if (!match.hasMatch()) {
		myShortcodeCompleter->popup()->hide();
		return;
	}
	const QString typed = match.captured(1);
	myShortcodeTokenStart = int(upto.size() - typed.size()) - 1;

	// Prefix matches before substring matches, each alphabetically.
	QList<QPair<QString, QString> > prefix, substr;
	QListIterator<QPair<QString, QString> > it(myShortcodeList);
	while (it.hasNext()) {
		const QPair<QString, QString> &entry = it.next();
		const int idx = entry.first.indexOf(typed);
		if (idx == 0)
			prefix.append(entry);
		else if (idx > 0)
			substr.append(entry);
	}
	prefix += substr;

	myShortcodeModel->clear();
	static const int maxRows = 30;
	for (int i = 0; i < prefix.size() && i < maxRows; ++i) {
		const QString &code = prefix.at(i).first;
		const QString &emoji = prefix.at(i).second;
		// Render the emoji as an icon (EmojiPicker::emojiIcon guarantees the
		// target size of the bitmap emoji font) and cache it.
		QHash<QString, QIcon>::const_iterator cached = myShortcodeIconCache.constFind(emoji);
		if (cached == myShortcodeIconCache.constEnd())
			cached = myShortcodeIconCache.insert(emoji, EmojiPicker::emojiIcon(emoji, 18));
		QStandardItem *item = new QStandardItem(cached.value(), QString(":%1:").arg(code));
		item->setData(emoji, Qt::UserRole);
		item->setEditable(false);
		myShortcodeModel->appendRow(item);
	}
	if (myShortcodeModel->rowCount() == 0) {
		myShortcodeCompleter->popup()->hide();
		return;
	}
	myShortcodeCompleter->complete();
	// Preselect the first suggestion, so that Enter/Tab accept it right away.
	myShortcodeCompleter->popup()->setCurrentIndex(
		myShortcodeCompleter->completionModel()->index(0, 0));
}

void ChatTools::insertShortcodeCompletion(const QModelIndex &index)
{
	const QString emoji = index.data(Qt::UserRole).toString();
	if (emoji.isEmpty() || myShortcodeTokenStart < 0 || !myLineEdit)
		return;
	const QString text = myLineEdit->text();
	const int cursor = myLineEdit->cursorPosition();
	// Only accept it if the typed token still stands there unchanged.
	// A protection against an activated() with a stale state (e.g. when the
	// text was cleared otherwise between showing the popup and accepting).
	if (myShortcodeTokenStart >= text.size()
			|| text.at(myShortcodeTokenStart) != QLatin1Char(':')
			|| cursor <= myShortcodeTokenStart)
		return;
	// Replace the typed token (":smi") by the emoji – as an emoji instead of
	// ":smile:", exactly like the emoji picker (WYSIWYG and fewer bytes in the
	// 128 byte server limit; checkInputLength applies via textChanged).
	myLineEdit->setText(text.left(myShortcodeTokenStart) + emoji + text.mid(cursor));
	myLineEdit->setCursorPosition(qMin(myShortcodeTokenStart + int(emoji.size()),
									   int(myLineEdit->text().size())));
}

bool ChatTools::eventFilter(QObject *obj, QEvent *event)
{
	// The mouse over the history: the translate symbol follows the line under the
	// cursor. Only read along, never swallow.
	if(myTextBrowser && obj == myTextBrowser->viewport()) {
		if(event->type() == QEvent::MouseMove)
			updateTranslateHover(static_cast<QMouseEvent*>(event)->position().toPoint());
		// Hide it when leaving – but not while something is selected:
		// resetting the block would discard the selection shortly before it
		// is copied.
		else if(event->type() == QEvent::Leave
				&& !myTextBrowser->textCursor().hasSelection())
			setTranslateHoverId(0);
	}

	// Tab/Enter in the open suggestion popup accept the highlighted
	// suggestion – HERE, before the filter of the QCompleter (this filter is
	// installed later and therefore runs first). The QCompleter passes
	// Return on to the input field FIRST and accepts afterwards:
	// returnPressed would send the half typed message (":su…") beforehand
	// and the emoji would land in the cleared field in addition. Tab would fall through
	// to the dialogs as a nickname completion.
	if (myShortcodeCompleter && obj == myShortcodeCompleter->popup()
			&& event->type() == QEvent::KeyPress) {
		QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
		if (keyEvent->key() == Qt::Key_Tab
				|| keyEvent->key() == Qt::Key_Return
				|| keyEvent->key() == Qt::Key_Enter) {
			QModelIndex idx = myShortcodeCompleter->popup()->currentIndex();
			if (!idx.isValid())
				idx = myShortcodeCompleter->completionModel()->index(0, 0);
			insertShortcodeCompletion(idx);
			myShortcodeCompleter->popup()->hide();
			return true;
		}
	}
	return QObject::eventFilter(obj, event);
}

ChatTools::~ChatTools()
{
}

void ChatTools::sendMessage()
{

	if(myLineEdit->text().size() && mySession) {
		fillChatLinesHistory(myLineEdit->text());
		QString chatText(myLineEdit->text());

		// Safety: truncate to server max chat message size (128 bytes UTF-8)
		// to prevent server from closing the connection on validation failure.
		static const int MAX_CHAT_TEXT_SIZE = 128;
		while(chatText.toUtf8().size() > MAX_CHAT_TEXT_SIZE) {
			chatText.chop(1);
		}

		if(myChatType == INGAME_CHAT) {
			mySession->sendGameChatMessage(chatText.toUtf8().constData());
		} else {
			// Parse user name for private messages.
			if(chatText.indexOf(QString("/msg ")) == 0) {
				chatText.remove(0, 5);
				unsigned playerId = parsePrivateMessageTarget(chatText);
				if (playerId) {
					sendPrivateMessage(playerId, chatText);
				}
			} else {
				mySession->sendLobbyChatMessage(chatText.toUtf8().constData());
			}
		}
		myLineEdit->setText("");
	}
}

QColor ChatTools::chatBackgroundColor() const
{
	// The game chat: the history gets its background via a stylesheet from the
	// table style (setChatLogStyle), the widget palette says nothing about it.
	if(myChatType == INGAME_CHAT && myStyle) {
		const QColor styleBg("#" + myStyle->getChatLogBgColor());
		if(styleBg.isValid())
			return styleBg;
	}

	// The lobby chats: the QTextBrowser paints its surface with QPalette::Base. Always
	// evaluate the palette that is actually set and NOT the dark mode
	// setting: on Windows 10 Qt reports a light system palette for "auto"
	// even when Windows itself runs dark (Win10 does not pass that on to
	// Qt applications) – the history is light then, although the
	// system counts as dark. Conversely the forced dark palette
	// from DarkModeHelper applies here automatically.
	if(myTextBrowser) {
		const QPalette pal = myTextBrowser->palette();
		QColor base = pal.color(QPalette::Base);
		if(!base.isValid() || base.alpha() == 0)
			base = pal.color(QPalette::Window);
		if(base.isValid())
			return base;
	}
	return QApplication::palette().color(QPalette::Base);
}

QString ChatTools::nickHtml(const QString &nickText) const
{
	return "<span style=\"color:" + nickColorForBackground(chatBackgroundColor()).name()
		   + "; font-weight:bold;\">" + nickText.toHtmlEscaped() + "</span>";
}

void ChatTools::receiveMessage(QString playerName, QString message, bool pm)
{
	// Messages of ignored players are discarded completely – BEFORE the
	// reaction handling, so that their emoji reactions stay silent as well
	// (otherwise the table played the animation, because the ignore filter only applied
	// when appending to the chat history).
	if(nickIsOnIgnoreList(playerName))
		return;

	// Emoji reactions (the convention of the QML/web client): "/emoji 🎉" or
	// the legacy "[R]🎉" – only in the game chat. Do not display them, but play them
	// as a reaction animation at the seat of the sender (gametableimpl).
	if(myChatType == INGAME_CHAT) {
		const QString trimmedMsg = message.trimmed();
		bool isReactionMsg = false;
		QString reactionEmoji;
		if(trimmedMsg.startsWith(QStringLiteral("/emoji ")) && trimmedMsg.size() < 22) {
			isReactionMsg = true;
			reactionEmoji = trimmedMsg.mid(7).trimmed();
		} else if(trimmedMsg.startsWith(QStringLiteral("[R]")) && trimmedMsg.size() < 14) {
			isReactionMsg = true;
			reactionEmoji = trimmedMsg.mid(3).trimmed();
		}
		if(isReactionMsg) {
			// Reaction messages never appear in the chat history. Only play real
			// emojis – text disguised as a reaction is discarded.
			if(isEmojiOnlyReaction(reactionEmoji))
				emit reactionReceived(playerName, reactionEmoji);
			return;
		}
	}

	if(myTextBrowser) {

		// Remember the raw text (before the HTML escaping/markup) for the translation; a
		// leading "/me " does not belong to the message.
		QString rawSource = message;
		if(rawSource.startsWith("/me "))
			rawSource = rawSource.mid(4);

		message = message.replace("<","&lt;");
		message = message.replace(">","&gt;");
		// Convert the ASCII shortcuts (":-)", "8-)", "<3", …) on the escaped text
		// before link/style markup is added – that way short shortcuts never collide
		// with our own HTML such as "color:#...".
		message = applyChatEmoteShortcuts(message);
		//doing the links
		message = message.replace(QRegularExpression("((?:https?)://\\S+)"), "<a href=\"\\1\">\\1</a>");

		//refresh myNick if it was changed during runtime
		myNick = QString::fromUtf8(myConfig->readConfigString("MyName").c_str());

		QString tempMsg;

		if(myChatType == INET_LOBBY_CHAT && playerName == "(chat bot)" && message.startsWith(myNick)) {
			tempMsg = QString("<span style=\"font-weight:bold; color:red;\">"+message+"</span>");
		} else if(message.contains(myNick, Qt::CaseInsensitive)) {
			switch (myChatType) {
			case INET_LOBBY_CHAT: {
				tempMsg = QString("<span style=\"font-weight:bold; color:"+myLobby->palette().link().color().name()+";\">"+message+"</span>");
			}
			break;
			case LAN_LOBBY_CHAT:
				tempMsg = QString("<span style=\"font-weight:bold;\">"+message+"</span>");
				break;
			case INGAME_CHAT: {
				message = message.replace("<a href","<a style=\"color:#"+myStyle->getChatLogTextColor()+"; text-decoration: underline;\" href");
				tempMsg = QString("<span style=\"color:#"+myStyle->getChatTextNickNotifyColor()+";\">"+message+"</span>");
			}
			break;
			default:
				tempMsg = message;
			}
		} else if(playerName == myNick) {
			switch (myChatType) {
			case INET_LOBBY_CHAT:
				tempMsg = QString("<span style=\"font-weight:normal; color:"+myLobby->palette().link().color().name()+";\">"+message+"</span>");
				break;
			case LAN_LOBBY_CHAT:
				tempMsg = QString("<span style=\"font-weight:normal;\">"+message+"</span>");
				break;
			case INGAME_CHAT: {
				message = message.replace("<a href","<a style=\"color:#"+myStyle->getChatTextNickNotifyColor()+"; text-decoration: underline;\" href");
				tempMsg = QString("<span style=\"color:#"+myStyle->getChatLogTextColor()+";\">"+message+"</span>");
			}
			break;
			default:
				tempMsg = message;
			}
		} else {
			switch (myChatType) {
			case INET_LOBBY_CHAT:
				tempMsg = QString("<span style=\"font-weight:normal; color:"+myLobby->palette().text().color().name()+";\">"+message+"</span>");
				break;
			case LAN_LOBBY_CHAT:
				tempMsg = QString("<span style=\"font-weight:normal;\">"+message+"</span>");
				break;
			case INGAME_CHAT: {
				message = message.replace("<a href","<a style=\"color:#"+myStyle->getChatTextNickNotifyColor()+"; text-decoration: underline;\" href");
				tempMsg = QString("<span style=\"color:#"+myStyle->getChatLogTextColor()+";\">"+message+"</span>");
			}
			break;
			default:
				tempMsg = message;
			}

		}

		// The sender themselves is already caught above; here only
		// the chatbot warning ABOUT an ignored player ("<nick> …") is left.
		bool chatBotWarnNickFoundOnIgnoreList = false;
		if(myChatType == INET_LOBBY_CHAT && playerName == "(chat bot)") {
			const std::list<std::string> ignoreList = myConfig->readConfigStringList("PlayerIgnoreList");
			list<std::string>::const_iterator it1;
			for(it1=ignoreList.begin(); it1 != ignoreList.end(); ++it1) {
				if(message.startsWith(QString::fromUtf8(it1->c_str()))) {
					chatBotWarnNickFoundOnIgnoreList = true;
				}
			}
		}

		if(!chatBotWarnNickFoundOnIgnoreList) {
			// An incoming private message: ALWAYS a sound – it is directed at
			// you directly and would otherwise be lost in the running lobby chat.
			// Unlike the nick match below it can NOT be switched off via
			// "PlayLobbyChatNotification"; only the global
			// sound switch (QtAudioPlayer) decides. Deliberately only in the
			// internet lobby instance: the same PM reaches the LAN chat as well,
			// otherwise it would sound twice.
			if(pm && myChatType == INET_LOBBY_CHAT && myLobby && myLobby->getMyW()) {
				myLobby->getMyW()->getMySoundEventHandler()->playSound("lobbychatnotify",0);
			}
			//play beep sound as notification
			if(myChatType == INET_LOBBY_CHAT && message.contains(myNick, Qt::CaseInsensitive) && playerName != myNick) {
				if(myLobby->isVisible() && myConfig->readConfigInt("PlayLobbyChatNotification")) {
					myLobby->getMyW()->getMySoundEventHandler()->playSound("lobbychatnotify",0);
				}
			}

			// Display Unicode emojis larger (the old PNG emoticons
			// were replaced by native emojis).
			tempMsg = wrapEmojisLarger(tempMsg, 20);

			// Assemble the line without the globe. The body (tempMsg or the
			// body freed from "/me ") is a clean, replaceable
			// substring – when the translation is shown it is REPLACED
			// by it (not hung to the right of it).
			const bool isAction = (message.indexOf(QString("/me "))==0);
			QString bodyHtml = tempMsg;
			if(isAction)
				bodyHtml.replace("/me ", "");   // "/me " does not belong to the message body
			QString lineNoGlobe;
			// The name (including the separator) gets a strong colour of its own, so that
			// it stands out from the message text – so far it was appended unstyled
			// and thus ran along in the text colour of the history.
			if(isAction)
				lineNoGlobe = "<i>" + nickHtml("*" + playerName) + " " + bodyHtml + "</i>";
			else if(pm == true)
				lineNoGlobe = "<i>" + nickHtml(playerName + "(pm):") + " " + bodyHtml + "</i>";
			else
				lineNoGlobe = nickHtml(playerName + ":") + " " + bodyHtml;

			// The translate symbol only on messages of others (you do not have to
			// translate your own).
			if(myTranslator && myTranslator->enabled() && playerName != myNick) {
				const int xid = myTranslateNextId++;
				TranslateEntry e;
				e.sourceText  = rawSource;
				e.lineNoGlobe = lineNoGlobe;
				e.bodyHtml    = bodyHtml;
				myTranslateEntries.insert(xid, e);
				pruneTranslateEntries();
				// The symbol is invisible at first – it only appears when the
				// mouse is over the line (translateGlyph).
				myTextBrowser->append(lineNoGlobe + " " + translateAnchorHtml(xid, translateGlyph(xid)));
			} else {
				myTextBrowser->append(lineNoGlobe);
			}
		}
	}
}

void ChatTools::showLocalNote(QString message)
{
	// Local display only (in italics, like an incoming PM) – it is NOT sent.
	if(!myTextBrowser)
		return;
	message = message.replace("<","&lt;").replace(">","&gt;");
	message = wrapEmojisLarger(message, 20);
	// Multi-line notices (e.g. the community suggestion with one player per
	// line) come as plain text with "\n" – append() expects HTML, where
	// that would only be a space. Replace it only after the escaping.
	message = message.replace(QLatin1Char('\n'), QLatin1String("<br>"));
	myTextBrowser->append("<i>" + message + "</i>");
}

void ChatTools::privateMessage(QString playerName, QString message)
{
	bool pm=true;
	receiveMessage(playerName, message, pm);
}

void ChatTools::sendPrivateMessage(unsigned playerId, QString message)
{
	if(!mySession || !playerId || !myTextBrowser)
		return;

	message = message.trimmed();
	// The same 128 byte limit as in the chat: longer messages are discarded by the
	// packet validator of the server.
	static const int MAX_CHAT_TEXT_SIZE = 128;
	while(message.toUtf8().size() > MAX_CHAT_TEXT_SIZE) {
		message.chop(1);
	}
	if(message.isEmpty())
		return;

	// Guests cannot chat at all on the server side – not as a RECIPIENT either.
	// The check sits here because, besides the context menu (which already hides
	// the action), the chat command runs through here as well; without it such a
	// message would disappear without comment.
	if(mySession->getClientPlayerInfo(playerId).isGuest) {
		showLocalNote(tr("Guests cannot receive private messages."));
		return;
	}

	mySession->sendPrivateChatMessage(playerId, message.toUtf8().constData());

	const QString playerName = QString::fromUtf8(
								   mySession->getClientPlayerInfo(playerId).playerName.c_str());

	// A confirmation in your own history – the same preparation as an incoming
	// message (escaping, ASCII shortcuts, links, larger emojis), so that the line
	// does not look different from the rest of the chat.
	QString body = message;
	body = body.replace("<","&lt;").replace(">","&gt;");
	body = applyChatEmoteShortcuts(body);
	body = body.replace(QRegularExpression("((?:https?)://\\S+)"), "<a href=\"\\1\">\\1</a>");
	body = wrapEmojisLarger(body, 20);

	// Deliberately the confirmation line that has always been translated – only extended by the
	// wording that was missing so far (a sent PM could otherwise not be read
	// anywhere).
	// An em dash as the separator (like kTranslateGlobe deliberately without non-ASCII
	// in the source, so that the file stays independent of the encoding).
	const QString separator = QStringLiteral(" ") + QChar(0x2013) + QStringLiteral(" ");
	myTextBrowser->append("<i>" + tr("private message sent to player: %1").arg(nickHtml(playerName))
						  + separator + body + "</i>");
}

void ChatTools::clearChat()
{

	if(myTextBrowser)
		myTextBrowser->clear();

	// The translation state belongs to the history that has now been cleared.
	myTranslateEntries.clear();
	myTranslateReqToId.clear();
	myTranslateHoverId = 0;
}

// Replaces the content of a text block (without the paragraph separator) by html.
static void replaceBlockContentHtml(const QTextBlock &block, const QString &html)
{
	if(!block.isValid())
		return;
	// Select the block content WITHOUT the paragraph separator (otherwise the block would merge with
	// the next one). EndOfBlock is correct for the last block as well.
	QTextCursor cursor(block);
	cursor.setPosition(block.position());
	cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
	cursor.insertHtml(html);
}

QString ChatTools::translateAnchorHtml(int id, const QString &glyph) const
{
	// Render the symbol like a chat emoji, but deliberately a bit smaller than the
	// message emojis (wrapEmojisLarger: 20px) – clearly recognisable without dominating
	// the line. The size is fixed, i.e. independent of the text size.
	return QString("<a href=\"pokerthtranslate:%1\" style=\"text-decoration:none;\">"
				   "<span style=\"font-size:14px; font-family:'%2';\">%3</span></a>")
		   .arg(id).arg(EmojiPicker::emojiFontFamily()).arg(glyph);
}

QString ChatTools::translateGlyph(int id) const
{
	QHash<int, TranslateEntry>::const_iterator it = myTranslateEntries.find(id);
	if(it == myTranslateEntries.end())
		return kTranslateHidden;
	if(it->inFlight)
		return kTranslateSpinner;
	// Visible while the translation is shown (it indicates that the
	// line is translated, and is the way back to the original) – otherwise only on the
	// line under the mouse cursor.
	if(!kTranslateHoverOnly || it->shown || myTranslateHoverId == id)
		return kTranslateGlobe;
	return kTranslateHidden;
}

int ChatTools::translateIdAtBlock(const QTextBlock &block) const
{
	if(!block.isValid())
		return 0;
	static const QString prefix = QStringLiteral("pokerthtranslate:");
	for(QTextBlock::iterator it = block.begin(); !it.atEnd(); ++it) {
		const QTextFragment frag = it.fragment();
		if(frag.isValid() && frag.charFormat().isAnchor()
				&& frag.charFormat().anchorHref().startsWith(prefix))
			return frag.charFormat().anchorHref().mid(prefix.size()).toInt();
	}
	return 0;
}

void ChatTools::updateTranslateHover(const QPoint &viewportPos)
{
	if(!kTranslateHoverOnly || !myTextBrowser || myTranslateEntries.isEmpty())
		return;
	// While something is selected, do NOT write into the document: resetting the
	// symbol replaces the content of whole text blocks and would destroy an existing
	// selection (or a drag in progress).
	if(myTextBrowser->textCursor().hasSelection())
		return;
	const QTextCursor cursor = myTextBrowser->cursorForPosition(viewportPos);
	// cursorForPosition always snaps to the nearest position – in the
	// empty area below the history that would be the last line. Only evaluate it
	// if the cursor really stands on the line (horizontally the whole
	// strip is meant, to the right of short text as well).
	const QRect lineRect = myTextBrowser->cursorRect(cursor);
	if(viewportPos.y() < lineRect.top() || viewportPos.y() > lineRect.bottom()) {
		setTranslateHoverId(0);
		return;
	}
	setTranslateHoverId(translateIdAtBlock(cursor.block()));
}

void ChatTools::setTranslateHoverId(int id)
{
	if(id == myTranslateHoverId)
		return;
	const int previous = myTranslateHoverId;
	myTranslateHoverId = id;
	if(previous > 0)          // Hide the symbol on the old line …
		rebuildTranslateBlock(previous);
	if(id > 0)                // … and show it on the new one
		rebuildTranslateBlock(id);
}

QTextBlock ChatTools::findTranslateBlock(int id) const
{
	if(!myTextBrowser)
		return QTextBlock();
	const QString href = QString("pokerthtranslate:") + QString::number(id);
	QTextDocument *doc = myTextBrowser->document();
	for(QTextBlock block = doc->begin(); block.isValid(); block = block.next()) {
		for(QTextBlock::iterator it = block.begin(); !it.atEnd(); ++it) {
			const QTextFragment frag = it.fragment();
			if(frag.isValid() && frag.charFormat().isAnchor()
					&& frag.charFormat().anchorHref() == href)
				return block;
		}
	}
	return QTextBlock();
}

void ChatTools::rebuildTranslateBlock(int id)
{
	QHash<int, TranslateEntry>::iterator it = myTranslateEntries.find(id);
	if(it == myTranslateEntries.end())
		return;
	const QTextBlock block = findTranslateBlock(id);
	if(!block.isValid())
		return; // The line no longer exists (e.g. the history was cleared)

	// The body: the original or – if shown – the translation in the same
	// place (the colour/look of the original message via styledTranslation).
	QString bodyLine = it->lineNoGlobe;
	if(it->shown) {
		const QString tb = ChatTranslatorCore::styledTranslation(it->bodyHtml, it->translated);
		// lastIndexOf: the message body stands at the end of the line, before it lies
		// the markup of the coloured name now. A search from the front could
		// hit its style attribute (with an unwrapped body).
		const int p = bodyLine.lastIndexOf(it->bodyHtml);
		if(p >= 0)
			bodyLine.replace(p, it->bodyHtml.size(), tb);
	}
	replaceBlockContentHtml(block, bodyLine + " " + translateAnchorHtml(id, translateGlyph(id)));
}

void ChatTools::pruneTranslateEntries()
{
	if(myTranslateEntries.size() <= kMaxChatBlocks)
		return;
	// Every entry belongs to exactly ONE text block, and the document holds
	// at most kMaxChatBlocks blocks (the oldest ones fall out at the front). The
	// ids are assigned in ascending order, so everything below this limit
	// can no longer have a line in the history.
	const int oldestPossible = myTranslateNextId - kMaxChatBlocks;
	QHash<int, TranslateEntry>::iterator it = myTranslateEntries.begin();
	while(it != myTranslateEntries.end()) {
		if(it.key() < oldestPossible)
			it = myTranslateEntries.erase(it);
		else
			++it;
	}
}

void ChatTools::refreshTranslationEnabled()
{
	if(!myTranslator || myTranslator->enabled())
		return; // Enabling affects new messages; existing ones stay.

	// Disabled: rebuild every line to the original WITHOUT the globe.
	if(myTextBrowser) {
		const QList<int> ids = myTranslateEntries.keys();
		for(int id : ids) {
			const QTextBlock block = findTranslateBlock(id);
			if(block.isValid())
				replaceBlockContentHtml(block, myTranslateEntries.value(id).lineNoGlobe);
		}
	}
	myTranslateEntries.clear();
	myTranslateReqToId.clear();
	myTranslateHoverId = 0;
}

void ChatTools::onChatAnchorClicked(const QUrl &url)
{
	if(url.scheme() == QLatin1String("pokerthtranslate")) {
		// "pokerthtranslate:5" -> id (robustly from the string, independently of
		// whether QUrl treats "5" as a path or as opaque).
		const int id = url.toString().mid(QStringLiteral("pokerthtranslate:").size()).toInt();
		if(!myTranslator || !myTranslator->enabled())
			return;
		QHash<int, TranslateEntry>::iterator it = myTranslateEntries.find(id);
		if(it == myTranslateEntries.end() || it->inFlight)
			return;

		if(it->shown) {                    // Toggle: hide the translation (show the original)
			it->shown = false;
			rebuildTranslateBlock(id);
			return;
		}
		if(!it->translated.isEmpty()) {    // show it from the cache (no new request)
			it->shown = true;
			rebuildTranslateBlock(id);
			return;
		}
		it->inFlight = true;               // fetch it
		rebuildTranslateBlock(id);         // show the spinner
		const int req = myTranslator->translate(it->sourceText);
		myTranslateReqToId.insert(req, id);
	} else {
		// Open real links externally (previously via openExternalLinks in the .ui).
		// openUrlSafe instead of QDesktopServices::openUrl: with bundled libs
		// (AppImage/tarball) xdg-open would inherit our LD_LIBRARY_PATH.
		AppImageUtils::openUrlSafe(url);
	}
}

void ChatTools::onChatTranslated(int requestId, const QString &text, bool ok)
{
	if(!myTranslateReqToId.contains(requestId))
		return;
	const int id = myTranslateReqToId.take(requestId);
	QHash<int, TranslateEntry>::iterator it = myTranslateEntries.find(id);
	if(it == myTranslateEntries.end())
		return;
	it->inFlight = false;
	const bool haveText = ok && !text.trimmed().isEmpty();
	if(haveText) {
		it->translated = text;   // cache it (another click hides/shows it)
		it->shown = true;        // the translation replaces the original
	}
	// The spinner goes back to the globe; show the translation if there is one. On an error
	// the original stays (the globe allows another attempt).
	rebuildTranslateBlock(id);

	// Make a failure visible: otherwise only the hourglass flashes up and nothing
	// happens – indistinguishable from the function being broken.
	// It is throttled so that several clicks do not repeat the same notice
	// while the service is down.
	if(!haveText) {
		static const qint64 failNoteIntervalMs = 60 * 1000;
		const qint64 now = QDateTime::currentMSecsSinceEpoch();
		if(myTranslateLastFailNoteMs == 0
				|| (now - myTranslateLastFailNoteMs) >= failNoteIntervalMs) {
			myTranslateLastFailNoteMs = now;
			showLocalNote(tr("Translation is currently unavailable. Please try again later."));
		}
	}
}

void ChatTools::checkInputLength(QString string)
{
	// Server validates: VALIDATE_STRING_SIZE(chattext, 1, MAX_CHAT_TEXT_SIZE)
	// and closes the connection on violation (asioreceivebuffer.cpp).
	// Old code only called setMaxLength(string.length()) which did NOT
	// prevent already-pasted oversized text from being sent.
	static const int MAX_CHAT_TEXT_SIZE = 128;

	if(string.toUtf8().size() > MAX_CHAT_TEXT_SIZE) {
		// Truncate at character boundary until UTF-8 fits within server limit
		while(string.length() > 0 && string.toUtf8().size() > MAX_CHAT_TEXT_SIZE) {
			string.chop(1);
		}
		myLineEdit->blockSignals(true);
		myLineEdit->setText(string);
		myLineEdit->setCursorPosition(string.length());
		myLineEdit->blockSignals(false);
	}
}

void ChatTools::fillChatLinesHistory(QString fillString)
{

	chatLinesHistory << fillString;
	if(chatLinesHistory.size() > 50) chatLinesHistory.removeFirst();


}

void ChatTools::showChatHistoryIndex(int index)
{

	if(index <= chatLinesHistory.size()) {

		// 		cout << chatLinesHistory.size() << " : " <<  index << endl;
		if(index > 0)
			myLineEdit->setText(chatLinesHistory.at(chatLinesHistory.size()-(index)));
		else
			myLineEdit->setText("");
	}
}

void ChatTools::nickAutoCompletition()
{

	QString myChatString = myLineEdit->text();
	QStringList myChatStringList = myChatString.split(" ");

	QStringList matchStringList;

	if(nickAutoCompletitionCounter == 0) {

		if(myNickListModel) {
			int it = 0;
			while (myNickListModel->item(it)) {
				QString text = myNickListModel->item(it, 0)->data(Qt::DisplayRole).toString();
				if(text.startsWith(myChatStringList.last(), Qt::CaseInsensitive) && myChatStringList.last() != "") {
					matchStringList << text;
				}
				++it;
			}
		}

		if(!myNickStringList.isEmpty()) {

			QStringListIterator it(myNickStringList);
			while (it.hasNext()) {
				QString next = it.next();
				if (next.startsWith(myChatStringList.last(), Qt::CaseInsensitive) && myChatStringList.last() != "")
					matchStringList << next;
			}
		}
	}

	if(!matchStringList.isEmpty() || nickAutoCompletitionCounter > 0) {

		myChatStringList.removeLast();

		// 		cout << nickAutoCompletitionCounter << endl;

		if(nickAutoCompletitionCounter == 0) {
			//first one
			lastChatString = myChatStringList.join(" ");
			lastMatchStringList = matchStringList;
		}

		if(nickAutoCompletitionCounter == lastMatchStringList.size()) nickAutoCompletitionCounter = 0;

		// 		cout << nickAutoCompletitionCounter << "\n";

		if(lastChatString == "") {
			myLineEdit->setText(lastMatchStringList.at(nickAutoCompletitionCounter)+": ");
		} else {
			//check if lastChatString is pm-code
			if((lastChatString == "/msg" || lastChatString == "/msg ") && lastMatchStringList.at(nickAutoCompletitionCounter).contains(" ")) {
				myLineEdit->setText(lastChatString+" \""+lastMatchStringList.at(nickAutoCompletitionCounter)+"\" ");
			} else {
				myLineEdit->setText(lastChatString+" "+lastMatchStringList.at(nickAutoCompletitionCounter)+" ");
			}
		}

		nickAutoCompletitionCounter++;
	}
}

void ChatTools::setChatTextEdited()
{

	nickAutoCompletitionCounter = 0;
}

bool ChatTools::nickIsOnIgnoreList(const QString &playerName) const
{
	if(!myConfig || playerName.isEmpty())
		return false;

	const std::list<std::string> ignoreList = myConfig->readConfigStringList("PlayerIgnoreList");
	list<std::string>::const_iterator it1;
	for(it1=ignoreList.begin(); it1 != ignoreList.end(); ++it1) {
		if(playerName == QString::fromUtf8(it1->c_str()))
			return true;
	}
	return false;
}

unsigned ChatTools::parsePrivateMessageTarget(QString &chatText)
{
	QString playerName;
	int endPosName = -1;
	// Target player is either in the format "this is a user" or singlename.
	if (chatText.startsWith('"')) {
		chatText.remove(0, 1);
		endPosName = chatText.indexOf('"');
	} else {
		endPosName = chatText.indexOf(' ');
	}
	if (endPosName > 0) {
		playerName = chatText.left(endPosName);
		chatText.remove(0, endPosName + 1);
	}
	chatText = chatText.trimmed();
	unsigned playerId = 0;
	if (!playerName.isEmpty() && !chatText.isEmpty()) {
		if(myNickListModel) {
			int it = 0;
			while (myNickListModel->item(it)) {
				QString text = myNickListModel->item(it, 0)->data(Qt::DisplayRole).toString();
				if(text == playerName) {
					playerId = myNickListModel->item(it, 0)->data(Qt::UserRole).toUInt();
					break;
				}
				++it;
			}
		}
	}
	return playerId;
}



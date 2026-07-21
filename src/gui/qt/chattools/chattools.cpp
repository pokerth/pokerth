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
#include <QDesktopServices>
#include <QTextDocument>
#include <QTextBlock>
#include "session.h"
#include "configfile.h"
#include "gametablestylereader.h"
#include "gamelobbydialogimpl.h"
#include "soundevents.h"
#include <iostream>


using namespace std;

namespace
{

// QLineEdit zeichnet Trailing-Action-Icons in PM_SmallIconSize (per Default
// ~16px) – unabhängig von der Auflösung des übergebenen Icons. Dieser kleine
// Proxy-Style hebt nur dieses Maß für das jeweilige Eingabefeld an, damit die
// Emoji-Auslöser-Icons (🙂/🎉) größer und besser tippbar werden.
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

bool isEmojiCodepoint(uint cp)
{
	return (cp >= 0x1F000 && cp <= 0x1FAFF)   // Emojis, Symbole, Erweiterungen
	       || (cp >= 0x2600 && cp <= 0x27BF)  // Misc Symbols, Dingbats
	       || (cp >= 0x2B00 && cp <= 0x2BFF)  // ⭐ u. a.
	       || cp == 0x2764 || cp == 0x203C || cp == 0x2049
	       || (cp >= 0x1F1E6 && cp <= 0x1F1FF); // Flaggen
}

// Prüft, ob ein Reaktions-Payload ("/emoji <x>") ausschließlich aus echten
// Emoji-Zeichen besteht (inkl. Variation-Selektoren, ZWJ und Hautton-Modifiern)
// und mindestens ein Emoji enthält. So werden als Reaktion getarnte
// Textnachrichten ("/emoji haha") verworfen, beliebige echte Emojis aber
// zugelassen.
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
			return false;   // Buchstabe/Ziffer/Satzzeichen → keine Reaktion
		i += len;
	}
	return hasEmoji;
}

// Unicode-Emojis im (HTML-)Chat-Text vergrößern: jeder Emoji-Lauf (inkl.
// ZWJ-Sequenzen, Variation Selectors und Hautton-Modifier) wird in einen
// font-size-Span gewickelt. Inhalte innerhalb von HTML-Tags bleiben
// unangetastet.
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


// Übersetzungs-Symbole. 🌐 = anklickbar, ⏳ = läuft. Über den Codepoint bauen
// (nicht per "\xF0…"-Literal – das würde als Latin-1 gelesen und Mojibake geben).
static const QString kTranslateGlobe   = QString::fromUcs4(U"\U0001F310"); // 🌐
static const QString kTranslateSpinner = QString::fromUcs4(U"\U000023F3"); // ⏳

ChatTools::ChatTools(QLineEdit* l, ConfigFile *c, ChatType ct, QTextBrowser *b, QStandardItemModel *m, gameLobbyDialogImpl *lo) : nickAutoCompletitionCounter(0), myLineEdit(l), myNickListModel(m), myNickStringList(nullptr), myTextBrowser(b), myChatType(ct), myConfig(c), myNick(""), myLobby(lo), myEmojiPicker(nullptr), myShortcodeCompleter(nullptr), myShortcodeModel(nullptr), myShortcodeTokenStart(-1), myTranslator(nullptr), myTranslateNextId(1)
{
	myNick = QString::fromUtf8(myConfig->readConfigString("MyName").c_str());
	ignoreList = myConfig->readConfigStringList("PlayerIgnoreList");
	setupEmojiPickerAction();
	setupShortcodeCompleter();

	// Chat-Übersetzung. Der QTextBrowser navigiert sonst beim Klick selbst
	// (openExternalLinks in der .ui); wir schalten openLinks aus und behandeln
	// Klicks selbst: unser Pseudo-Schema übersetzt, http(s) öffnet extern (wie
	// zuvor über openExternalLinks). So bleibt das externe Öffnen erhalten und
	// unser Globus-Link löst keine Dokument-Navigation aus.
	myTranslator = new ChatTranslatorCore(myConfig, this);
	connect(myTranslator, &ChatTranslatorCore::translated, this, &ChatTools::onChatTranslated);
	if(myTextBrowser) {
		myTextBrowser->setOpenLinks(false);
		connect(myTextBrowser, &QTextBrowser::anchorClicked, this, &ChatTools::onChatAnchorClicked);
	}
}

void ChatTools::setupEmojiPickerAction()
{
	if (!myLineEdit)
		return;

	// Auslöser-Icons im Eingabefeld vergrößern. Der Proxy-Style gilt für dieses
	// Eingabefeld (und damit auch für das 🎉-Reaktions-Icon, das der Gametable
	// auf Desktop in dasselbe Feld legt). Größe je nach Plattform.
#ifdef Q_OS_ANDROID
	const int triggerIconSize = 30;
#else
	const int triggerIconSize = 22;
#endif
	BiggerActionIconStyle *iconStyle = new BiggerActionIconStyle(triggerIconSize);
	iconStyle->setParent(myLineEdit);   // Lebensdauer an das Eingabefeld koppeln
	myLineEdit->setStyle(iconStyle);

	// Emoji-Picker-Knopf im Eingabefeld (rechts) – einheitlich für
	// Internet-Lobby, LAN-Lobby und Gametable-Chat.
	QAction *emojiAction = myLineEdit->addAction(EmojiPicker::emojiIcon(QStringLiteral("🙂"), triggerIconSize),
	                                             QLineEdit::TrailingPosition);
	emojiAction->setToolTip(tr("Insert emoji"));
	QObject::connect(emojiAction, &QAction::triggered, this, [this]() {
		if (!myEmojiPicker) {
			myEmojiPicker = new EmojiPicker(myLineEdit);
			QObject::connect(myEmojiPicker, &EmojiPicker::picked, this, [this](const QString &e) {
				myLineEdit->insert(e);
#ifndef Q_OS_ANDROID
				// Auf Android NICHT zurückfokussieren – das würde die
				// virtuelle Tastatur erneut aufpoppen lassen.
				myLineEdit->setFocus();
#endif
			});
		}
		myEmojiPicker->showAt(myLineEdit);
	});
}

// Autovervollständigung für Emoji-Shortcodes (":smi…" → 😄), wie die ChatBox
// des QML-Clients. Vorschläge kommen aus derselben Map, die beim Anzeigen
// ersetzt (chat_emote_shortcuts.h) – angeboten wird nur, was auch wirklich
// funktioniert. Der QCompleter liefert dabei nur Popup, Tastatur-Navigation
// (Hoch/Runter/Enter/Esc) und activated(); gefiltert und sortiert wird selbst
// (UnfilteredPopupCompletion), damit Präfix- vor Substring-Treffern stehen.
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
	// Bewusst setWidget() statt QLineEdit::setCompleter(): Letzteres würde bei
	// Übernahme die GANZE Zeile ersetzen – hier wird nur der Token ersetzt.
	myShortcodeCompleter->setWidget(myLineEdit);
	QObject::connect(myShortcodeCompleter, QOverload<const QModelIndex &>::of(&QCompleter::activated),
	                 this, &ChatTools::insertShortcodeCompletion);
	// Tab soll den markierten Vorschlag übernehmen (wie im QML-Client) –
	// eigener Filter auf dem Popup macht das deterministisch (s. eventFilter).
	myShortcodeCompleter->popup()->installEventFilter(this);

	// textEdited statt textChanged: programmatisches setText (History-Abruf,
	// Längen-Kürzung) soll das Popup nicht öffnen.
	QObject::connect(myLineEdit, &QLineEdit::textEdited,
	                 this, &ChatTools::updateShortcodeCompletion);
	// Cursorbewegung kann den Token unter dem Cursor ändern – aber nur bei
	// bereits offenem Popup neu bewerten (setText bewegt auch den Cursor).
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
	// Token = ":" (am Anfang oder nach Leerzeichen) + mindestens 2 Code-
	// Zeichen direkt vor dem Cursor (wie ChatBox.qml): so kollidieren die
	// ASCII-Kürzel (":P", ":D") nicht mit dem Popup, und der schließende ":"
	// eines fertig getippten Shortcodes schließt das Popup von selbst.
	static const QRegularExpression tokenRe(QStringLiteral("(?:^|\\s):([a-z0-9_+-]{2,})$"));
	const QRegularExpressionMatch match = tokenRe.match(upto);
	if (!match.hasMatch()) {
		myShortcodeCompleter->popup()->hide();
		return;
	}
	const QString typed = match.captured(1);
	myShortcodeTokenStart = int(upto.size() - typed.size()) - 1;

	// Präfix-Treffer vor Substring-Treffern, jeweils alphabetisch.
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
		// Emoji als Icon rendern (EmojiPicker::emojiIcon garantiert die
		// Zielgröße des Bitmap-Emoji-Fonts) und cachen.
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
	// Ersten Vorschlag vorauswählen, damit Enter/Tab sofort übernehmen.
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
	// Nur übernehmen, wenn der getippte Token noch unverändert dasteht.
	// Schutz gegen ein activated() mit veraltetem Zustand (z. B. wenn der
	// Text zwischen Popup-Anzeige und Übernahme anderweitig geleert wurde).
	if (myShortcodeTokenStart >= text.size()
			|| text.at(myShortcodeTokenStart) != QLatin1Char(':')
			|| cursor <= myShortcodeTokenStart)
		return;
	// Getippten Token (":smi") durch das Emoji ersetzen – als Emoji statt
	// ":smile:", genau wie der Emoji-Picker (WYSIWYG und weniger Bytes im
	// 128-Byte-Server-Limit; checkInputLength greift über textChanged).
	myLineEdit->setText(text.left(myShortcodeTokenStart) + emoji + text.mid(cursor));
	myLineEdit->setCursorPosition(qMin(myShortcodeTokenStart + int(emoji.size()),
	                                   int(myLineEdit->text().size())));
}

bool ChatTools::eventFilter(QObject *obj, QEvent *event)
{
	// Tab/Enter im offenen Vorschlags-Popup übernehmen den markierten
	// Vorschlag – HIER, vor dem Filter des QCompleters (dieser Filter ist
	// später installiert und läuft daher zuerst). Der QCompleter reicht
	// Return nämlich ERST an das Eingabefeld weiter und übernimmt dann:
	// returnPressed würde die halb getippte Nachricht (":su…") vorab senden
	// und das Emoji landete zusätzlich im geleerten Feld. Tab fiele als
	// Nick-Vervollständigung an die Dialoge durch.
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
					mySession->sendPrivateChatMessage(playerId, chatText.toUtf8().constData());
					QString tmp = tr("private message sent to player: %1");
					myTextBrowser->append("<i>"+tmp.arg(QString::fromUtf8(mySession->getClientPlayerInfo(playerId).playerName.c_str()))+"</i>");
				}
			} else {
				mySession->sendLobbyChatMessage(chatText.toUtf8().constData());
			}
		}
		myLineEdit->setText("");
	}
}

void ChatTools::receiveMessage(QString playerName, QString message, bool pm)
{

	// Emoji-Reaktionen (Konvention des QML-/Web-Clients): "/emoji 🎉" bzw.
	// legacy "[R]🎉" – nur im Spiel-Chat. Nicht anzeigen, sondern als
	// Reaktions-Animation am Sitz des Absenders abspielen (gametableimpl).
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
			// Reaktions-Nachrichten erscheinen nie im Chat-Verlauf. Nur echte
			// Emojis abspielen – als Reaktion getarnter Text wird verworfen.
			if(isEmojiOnlyReaction(reactionEmoji))
				emit reactionReceived(playerName, reactionEmoji);
			return;
		}
	}

	if(myTextBrowser) {

		// Rohtext (vor HTML-Escaping/Markup) für die Übersetzung merken; ein
		// führendes "/me " gehört nicht zur Nachricht.
		QString rawSource = message;
		if(rawSource.startsWith("/me "))
			rawSource = rawSource.mid(4);

		message = message.replace("<","&lt;");
		message = message.replace(">","&gt;");
		// ASCII-Kürzel (":-)", "8-)", "<3", …) auf dem escapten Text umsetzen,
		// bevor Link-/Style-Markup hinzukommt – so kollidieren kurze Kürzel nie
		// mit eigenem HTML wie "color:#...".
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

		bool nickFoundOnIgnoreList = false;
		bool chatBotWarnNickFoundOnIgnoreList = false;
		list<std::string>::iterator it1;
		for(it1=ignoreList.begin(); it1 != ignoreList.end(); ++it1) {
			if(playerName == QString::fromUtf8(it1->c_str())) {
				nickFoundOnIgnoreList = true;
			}
			if(myChatType == INET_LOBBY_CHAT && playerName == "(chat bot)" && message.startsWith(QString::fromUtf8(it1->c_str()))) {
				chatBotWarnNickFoundOnIgnoreList = true;
			}
		}

		if(!nickFoundOnIgnoreList && !chatBotWarnNickFoundOnIgnoreList) {
			//play beep sound as notification
			if(myChatType == INET_LOBBY_CHAT && message.contains(myNick, Qt::CaseInsensitive) && playerName != myNick) {
				if(myLobby->isVisible() && myConfig->readConfigInt("PlayLobbyChatNotification")) {
					myLobby->getMyW()->getMySoundEventHandler()->playSound("lobbychatnotify",0);
				}
			}

			// Unicode-Emojis größer darstellen (die alten PNG-Emoticons
			// wurden durch native Emojis ersetzt).
			tempMsg = wrapEmojisLarger(tempMsg, 20);

			// Zeile ohne Globus zusammenbauen. Der Körper (tempMsg bzw. der
			// von "/me " befreite Körper) ist ein sauberer, ersetzbarer
			// Teilstring – beim Einblenden wird er durch die Übersetzung
			// ERSETZT (nicht rechts daneben gehängt).
			const bool isAction = (message.indexOf(QString("/me "))==0);
			QString bodyHtml = tempMsg;
			if(isAction)
				bodyHtml.replace("/me ", "");   // "/me " gehört nicht zum Nachrichtenkörper
			QString lineNoGlobe;
			if(isAction)
				lineNoGlobe = "<i>*" + playerName + " " + bodyHtml + "</i>";
			else if(pm == true)
				lineNoGlobe = "<i>" + playerName + "(pm): " + bodyHtml + "</i>";
			else
				lineNoGlobe = playerName + ": " + bodyHtml;

			// Übersetzen-Symbol nur an Nachrichten anderer (die eigenen muss
			// man nicht übersetzen).
			if(myTranslator && myTranslator->enabled() && playerName != myNick) {
				const int xid = myTranslateNextId++;
				TranslateEntry e;
				e.sourceText  = rawSource;
				e.lineNoGlobe = lineNoGlobe;
				e.bodyHtml    = bodyHtml;
				myTranslateEntries.insert(xid, e);
				myTextBrowser->append(lineNoGlobe + " " + translateAnchorHtml(xid, kTranslateGlobe));
			} else {
				myTextBrowser->append(lineNoGlobe);
			}
		}
	}
}

void ChatTools::privateMessage(QString playerName, QString message)
{
	bool pm=true;
	receiveMessage(playerName, message, pm);
}

void ChatTools::clearChat()
{

	if(myTextBrowser)
		myTextBrowser->clear();

	// Übersetzungs-Zustand gehört zum jetzt geleerten Verlauf.
	myTranslateEntries.clear();
	myTranslateReqToId.clear();
}

// Ersetzt den Inhalt eines Textblocks (ohne den Absatztrenner) durch html.
static void replaceBlockContentHtml(const QTextBlock &block, const QString &html)
{
	if(!block.isValid())
		return;
	// Blockinhalt selektieren OHNE den Absatztrenner (sonst würde der Block mit
	// dem nächsten verschmelzen). EndOfBlock ist auch für den letzten Block korrekt.
	QTextCursor cursor(block);
	cursor.setPosition(block.position());
	cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
	cursor.insertHtml(html);
}

QString ChatTools::translateAnchorHtml(int id, const QString &glyph) const
{
	// Symbol wie ein Chat-Emoji rendern, aber bewusst etwas kleiner als die
	// Nachrichten-Emojis (wrapEmojisLarger: 20px) – gut erkennbar, ohne die
	// Zeile zu dominieren. Größe ist fix, also unabhängig von der Textgröße.
	return QString("<a href=\"pokerthtranslate:%1\" style=\"text-decoration:none;\">"
	               "<span style=\"font-size:14px; font-family:'%2';\">%3</span></a>")
	       .arg(id).arg(EmojiPicker::emojiFontFamily()).arg(glyph);
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
		return; // Zeile nicht mehr vorhanden (z. B. Verlauf geleert)

	// Körper: Original oder – wenn eingeblendet – die Übersetzung an gleicher
	// Stelle (Farbe/Look der Originalnachricht via styledTranslation).
	QString bodyLine = it->lineNoGlobe;
	if(it->shown) {
		const QString tb = ChatTranslatorCore::styledTranslation(it->bodyHtml, it->translated);
		const int p = bodyLine.indexOf(it->bodyHtml);
		if(p >= 0)
			bodyLine.replace(p, it->bodyHtml.size(), tb);
	}
	const QString glyph = it->inFlight ? kTranslateSpinner : kTranslateGlobe;
	replaceBlockContentHtml(block, bodyLine + " " + translateAnchorHtml(id, glyph));
}

void ChatTools::refreshTranslationEnabled()
{
	if(!myTranslator || myTranslator->enabled())
		return; // Aktivieren wirkt auf neue Nachrichten; Bestehende bleiben.

	// Deaktiviert: jede Zeile auf das Original OHNE Globus zurückbauen.
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
}

void ChatTools::onChatAnchorClicked(const QUrl &url)
{
	if(url.scheme() == QLatin1String("pokerthtranslate")) {
		// "pokerthtranslate:5" -> id (robust aus dem String, unabhängig davon,
		// ob QUrl "5" als path oder opaque behandelt).
		const int id = url.toString().mid(QStringLiteral("pokerthtranslate:").size()).toInt();
		if(!myTranslator || !myTranslator->enabled())
			return;
		QHash<int, TranslateEntry>::iterator it = myTranslateEntries.find(id);
		if(it == myTranslateEntries.end() || it->inFlight)
			return;

		if(it->shown) {                    // Toggle: Übersetzung ausblenden (Original zeigen)
			it->shown = false;
			rebuildTranslateBlock(id);
			return;
		}
		if(!it->translated.isEmpty()) {    // aus Cache einblenden (keine neue Anfrage)
			it->shown = true;
			rebuildTranslateBlock(id);
			return;
		}
		it->inFlight = true;               // holen
		rebuildTranslateBlock(id);         // Spinner anzeigen
		const int req = myTranslator->translate(it->sourceText);
		myTranslateReqToId.insert(req, id);
	} else {
		// Echte Links extern öffnen (früher über openExternalLinks in der .ui).
		QDesktopServices::openUrl(url);
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
	if(ok && !text.trimmed().isEmpty()) {
		it->translated = text;   // cachen (erneuter Klick blendet aus/ein)
		it->shown = true;        // Übersetzung ersetzt das Original
	}
	// Spinner zurück auf Globus; ggf. Übersetzung einblenden. Bei Fehler bleibt
	// das Original stehen (Globus erlaubt erneuten Versuch).
	rebuildTranslateBlock(id);
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

void ChatTools::refreshIgnoreList()
{
	ignoreList = myConfig->readConfigStringList("PlayerIgnoreList");
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



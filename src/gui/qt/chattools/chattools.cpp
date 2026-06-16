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
#include <QProxyStyle>
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


ChatTools::ChatTools(QLineEdit* l, ConfigFile *c, ChatType ct, QTextBrowser *b, QStandardItemModel *m, gameLobbyDialogImpl *lo) : nickAutoCompletitionCounter(0), myLineEdit(l), myNickListModel(m), myNickStringList(nullptr), myTextBrowser(b), myChatType(ct), myConfig(c), myNick(""), myLobby(lo), myEmojiPicker(nullptr)
{
	myNick = QString::fromUtf8(myConfig->readConfigString("MyName").c_str());
	ignoreList = myConfig->readConfigStringList("PlayerIgnoreList");
	setupEmojiPickerAction();
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
		QString reactionEmoji;
		if(trimmedMsg.startsWith(QStringLiteral("/emoji ")) && trimmedMsg.size() < 22)
			reactionEmoji = trimmedMsg.mid(7).trimmed();
		else if(trimmedMsg.startsWith(QStringLiteral("[R]")) && trimmedMsg.size() < 14)
			reactionEmoji = trimmedMsg.mid(3).trimmed();
		if(!reactionEmoji.isEmpty()) {
			emit reactionReceived(playerName, reactionEmoji);
			return;
		}
	}

	if(myTextBrowser) {

		message = message.replace("<","&lt;");
		message = message.replace(">","&gt;");
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

			if(message.indexOf(QString("/me "))==0) {
				myTextBrowser->append(tempMsg.replace("/me ","<i>*"+playerName+" ")+"</i>");
			} else if(pm == true) {
				myTextBrowser->append("<i>"+playerName+"(pm): " + tempMsg+"</i>");
			} else {
				myTextBrowser->append(playerName + ": " + tempMsg);
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



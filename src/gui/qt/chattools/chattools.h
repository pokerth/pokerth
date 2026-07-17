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
#ifndef CHATTOOLS_H
#define CHATTOOLS_H

#include <string>
#include <QtCore>
#include <QtWidgets>
#include <QtGui>
#include <boost/shared_ptr.hpp>

enum ChatType { INET_LOBBY_CHAT, LAN_LOBBY_CHAT, INGAME_CHAT };

class Session;
class ConfigFile;
class GameTableStyleReader;
class gameLobbyDialogImpl;
class ChatTranslatorCore;

class ChatTools : public QObject
{
	Q_OBJECT

public:
	ChatTools(QLineEdit*, ConfigFile*, ChatType, QTextBrowser *b = NULL, QStandardItemModel *m = NULL, gameLobbyDialogImpl *lo = NULL);

	~ChatTools();

	void setSession(boost::shared_ptr<Session> session)
	{
		mySession = session;
	}

public slots:

	void sendMessage();
	void receiveMessage(QString playerName, QString message, bool pm=false);
	void privateMessage(QString playerName, QString message);
	void clearChat();
	void checkInputLength(QString string);

	void fillChatLinesHistory(QString fillString);
	void showChatHistoryIndex(int index);
	int getChatLinesHistorySize()
	{
		return chatLinesHistory.size();
	}

	void nickAutoCompletition();
	void setChatTextEdited();

	// Offenes Shortcode-Vorschlags-Popup (":smi…")? Die Dialoge lassen dann
	// Tab/Hoch/Runter in Ruhe (keine Nick-Vervollständigung/History), damit
	// die Tasten das Popup steuern können.
	bool shortcodeCompletionActive() const;

	void setPlayerNicksList(QStringList value)
	{
		myNickStringList = value;
	}
	void setMyNick ( const QString& theValue )
	{
		myNick = theValue;
	}
	QString getMyNick ()
	{
		return myNick;
	}

	void setMyStyle ( GameTableStyleReader* theValue )
	{
		myStyle = theValue;
	}
	void refreshIgnoreList();

signals:

	// Emoji-Reaktion empfangen (Chat-Konvention "/emoji 🎉" wie im QML-/
	// Web-Client). Nur INGAME_CHAT; die Nachricht erscheint nicht im Chat.
	void reactionReceived(QString playerName, QString emoji);

protected:

	unsigned parsePrivateMessageTarget(QString &chatText);
	// Fängt Tab im Shortcode-Popup ab (Vorschlag übernehmen statt weiterreichen).
	bool eventFilter(QObject *obj, QEvent *event) override;

private:

	void setupEmojiPickerAction();
	void setupShortcodeCompleter();
	void updateShortcodeCompletion();
	void insertShortcodeCompletion(const QModelIndex &index);

	// ── Chat-Übersetzung ────────────────────────────────────────────────
	// Baut das anklickbare Globus-Anchor-HTML ("pokerthtranslate:<id>").
	QString translateAnchorHtml(int id, const QString &glyph) const;
	// Eingeblendete Übersetzung: bleibt EIN Anchor (gleicher href) -> ein
	// erneuter Klick blendet sie wieder aus (Toggle).
	QString translateAnchorShownHtml(int id, const QString &text) const;
	// Findet den Anchor mit passender id im QTextBrowser und liefert einen
	// Cursor, der ihn selektiert (null, wenn nicht mehr vorhanden).
	QTextCursor findTranslateAnchor(int id) const;
	// Ersetzt den Anchor inline durch nur ein Symbol (🌐/⏳) bzw. durch das
	// Symbol + die eingeblendete Übersetzung.
	void setTranslateGlyph(int id, const QString &glyph);
	void setTranslateShown(int id, const QString &text);

private slots:
	// QTextBrowser-Links (openLinks ist aus): unser Pseudo-Schema übersetzt,
	// echte http(s)-Links werden extern geöffnet.
	void onChatAnchorClicked(const QUrl &url);
	// Ergebnis aus dem Übersetzer-Kern.
	void onChatTranslated(int requestId, const QString &text, bool ok);

private:

	QStringList chatLinesHistory;
	QString lastChatString;
	QStringList lastMatchStringList;
	int nickAutoCompletitionCounter;

	QLineEdit *myLineEdit;
	QStandardItemModel *myNickListModel;
	QStringList myNickStringList;
	QTextBrowser *myTextBrowser;
	boost::shared_ptr<Session> mySession;
	ChatType myChatType;
	ConfigFile *myConfig;

	QString myNick;

	GameTableStyleReader *myStyle;
	gameLobbyDialogImpl *myLobby;

	class EmojiPicker *myEmojiPicker;

	// Shortcode-Autovervollständigung (":smi…" → 😄)
	QCompleter *myShortcodeCompleter;
	QStandardItemModel *myShortcodeModel;
	QList<QPair<QString, QString> > myShortcodeList;   // (code, emoji), sortiert
	QHash<QString, QIcon> myShortcodeIconCache;
	int myShortcodeTokenStart;

	std::list<std::string> ignoreList;

	// ── Chat-Übersetzung ────────────────────────────────────────────────
	ChatTranslatorCore *myTranslator;
	QHash<int, QString> myTranslateSource;   // Anchor-id -> Rohtext der Nachricht
	QHash<int, QString> myTranslateCache;    // Anchor-id -> gecachte Übersetzung
	QHash<int, int> myTranslateReqToId;      // Core-Request-id -> Anchor-id
	QSet<int> myTranslateInFlight;           // laufende Anfragen (Doppel-Tap-Schutz)
	QSet<int> myTranslateShown;              // aktuell eingeblendete Übersetzungen (Toggle)
	int myTranslateNextId;
};

#endif

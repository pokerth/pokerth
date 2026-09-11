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
	// Sends a private message and confirms it in your own chat history –
	// WITH the full text, because sent PMs do not show up anywhere else. A shared
	// path for the chat shortcut "/msg" and the context menu entry of the
	// nick list (gameLobbyDialogImpl).
	void sendPrivateMessage(unsigned playerId, QString message);
	// Appends a line ONLY locally to your own chat history (no sending,
	// no broadcast) – for notices that only the triggering user should see,
	// e.g. the community "suggest" result (like the PM reply of the bbcbot).
	void showLocalNote(QString message);
	void clearChat();
	// Called after applying the settings. Applies the switch
	// "AllowChatTranslation" to the VISIBLE history: on deactivation
	// all existing globe symbols/translations are removed immediately
	// (new messages are handled live by receiveMessage() anyway).
	void refreshTranslationEnabled();
	void checkInputLength(QString string);

	void fillChatLinesHistory(QString fillString);
	void showChatHistoryIndex(int index);
	int getChatLinesHistorySize()
	{
		return chatLinesHistory.size();
	}

	void nickAutoCompletition();
	void setChatTextEdited();

	// An open shortcode suggestion popup (":smi…")? The dialogs then leave
	// Tab/up/down alone (no nickname completion/history), so that
	// the keys can control the popup.
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

signals:

	// An emoji reaction was received (chat convention "/emoji 🎉" as in the QML/
	// web client). INGAME_CHAT only; the message does not appear in the chat.
	void reactionReceived(QString playerName, QString emoji);

protected:

	unsigned parsePrivateMessageTarget(QString &chatText);
	// Intercepts Tab in the shortcode popup (accept the suggestion instead of passing it on).
	bool eventFilter(QObject *obj, QEvent *event) override;

private:

	// Is the sender on the ignore list? It is read afresh from the configuration
	// on EVERY message (as everywhere else in the client, e.g.
	// MyAvatarLabel/gameLobbyDialogImpl): the lobby and the game chat are separate
	// ChatTools instances, so a cached list would inevitably drift
	// apart as soon as somebody was ignored in only one of the two.
	bool nickIsOnIgnoreList(const QString &playerName) const;

	// Actual background colour of the chat history: in the game chat the colour
	// of the table style, in the lobby chats the palette set on the QTextBrowser.
	// The basis for a name colour that stays readable in every theme.
	QColor chatBackgroundColor() const;
	// Player name (with a separator) as a coloured, bold HTML fragment.
	QString nickHtml(const QString &nickText) const;

	void setupEmojiPickerAction();
	void setupShortcodeCompleter();
	void updateShortcodeCompletion();
	void insertShortcodeCompletion(const QModelIndex &index);

	// ── Chat translation ────────────────────────────────────────────────
	// Builds the clickable globe anchor HTML ("pokerthtranslate:<id>").
	QString translateAnchorHtml(int id, const QString &glyph) const;
	// The symbol the line should carry in its current state: the spinner while
	// the translation is running, the globe when the translation is shown or
	// the line is under the mouse cursor – otherwise an invisible placeholder.
	QString translateGlyph(int id) const;
	// Anchor id of the chat line in this text block (0 = not translatable).
	int translateIdAtBlock(const QTextBlock &block) const;
	// Determines the line under the (viewport) mouse position and moves the
	// globe symbol there.
	void updateTranslateHover(const QPoint &viewportPos);
	void setTranslateHoverId(int id);
	// Finds the text block (chat line) that contains the globe anchor of this id.
	QTextBlock findTranslateBlock(int id) const;
	// Rebuilds the content of the chat line from the stored state
	// (original OR translation body + globe/spinner) and replaces the
	// corresponding block in the document with it. That way the translation REPLACES the
	// text in place (instead of appearing to the right of it).
	void rebuildTranslateBlock(int id);
	// Throw away translator entries whose chat line has dropped out of the
	// history (limited to kMaxChatBlocks). Without that, an entry with the source
	// text + the message body would grow along for the whole session with every
	// received foreign message, although its line is long gone.
	void pruneTranslateEntries();

private slots:
	// QTextBrowser links (openLinks is off): our pseudo scheme translates,
	// real http(s) links are opened externally.
	void onChatAnchorClicked(const QUrl &url);
	// Result from the translator core.
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

	// Shortcode auto-completion (":smi…" → 😄)
	QCompleter *myShortcodeCompleter;
	QStandardItemModel *myShortcodeModel;
	QList<QPair<QString, QString> > myShortcodeList;   // (code, emoji), sortiert
	QHash<QString, QIcon> myShortcodeIconCache;
	int myShortcodeTokenStart;

	// ── Chat translation ────────────────────────────────────────────────
	struct TranslateEntry {
		QString sourceText;   // raw text of the message (for the request)
		QString lineNoGlobe;  // rendered chat line WITHOUT the globe (original body)
		QString bodyHtml;     // original message body (substring of lineNoGlobe)
		QString translated;   // cached translation (empty = not fetched yet)
		bool inFlight = false;
		bool shown = false;   // translation currently shown? (toggle)
	};
	ChatTranslatorCore *myTranslator;
	QHash<int, TranslateEntry> myTranslateEntries;  // Anchor-id -> Zustand
	QHash<int, int> myTranslateReqToId;             // Core-Request-id -> Anchor-id
	int myTranslateNextId;
	int myTranslateHoverId;                         // line under the mouse (0 = none)
	qint64 myTranslateLastFailNoteMs;               // last failure notice (throttling)
};

#endif

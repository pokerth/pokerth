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
	// Sendet eine private Nachricht und bestätigt sie im eigenen Chat-Verlauf –
	// MIT vollem Text, denn gesendete PMs tauchen sonst nirgends auf. Gemeinsamer
	// Weg für den Chat-Kurzbefehl "/msg" und den Kontextmenü-Eintrag der
	// Nickliste (gameLobbyDialogImpl).
	void sendPrivateMessage(unsigned playerId, QString message);
	// Hängt eine Zeile NUR lokal an den eigenen Chat-Verlauf an (kein Senden,
	// kein Broadcast) – für Hinweise, die nur der auslösende Nutzer sehen soll,
	// z. B. das Community-„Suggest"-Ergebnis (wie die PM-Antwort des bbcbot).
	void showLocalNote(QString message);
	void clearChat();
	// Wird nach dem Übernehmen der Einstellungen aufgerufen. Wendet den Schalter
	// "AllowChatTranslation" auf den SICHTBAREN Verlauf an: bei Deaktivierung
	// werden alle vorhandenen Globus-Symbole/Übersetzungen sofort entfernt
	// (neue Nachrichten regelt receiveMessage() ohnehin live).
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

signals:

	// Emoji-Reaktion empfangen (Chat-Konvention "/emoji 🎉" wie im QML-/
	// Web-Client). Nur INGAME_CHAT; die Nachricht erscheint nicht im Chat.
	void reactionReceived(QString playerName, QString emoji);

protected:

	unsigned parsePrivateMessageTarget(QString &chatText);
	// Fängt Tab im Shortcode-Popup ab (Vorschlag übernehmen statt weiterreichen).
	bool eventFilter(QObject *obj, QEvent *event) override;

private:

	// Steht der Absender auf der Ignore-Liste? Wird bei JEDER Nachricht frisch
	// aus der Konfiguration gelesen (wie überall sonst im Client, z. B.
	// MyAvatarLabel/gameLobbyDialogImpl): Lobby- und Spiel-Chat sind getrennte
	// ChatTools-Instanzen, eine zwischengespeicherte Liste lief zwangsläufig
	// auseinander, sobald in nur einem der beiden ignoriert wurde.
	bool nickIsOnIgnoreList(const QString &playerName) const;

	// Tatsächliche Hintergrundfarbe des Chat-Verlaufs: im Spiel-Chat die Farbe
	// des Tischstils, in den Lobby-Chats die gesetzte Palette des QTextBrowsers.
	// Grundlage für eine Namensfarbe, die in jedem Thema lesbar bleibt.
	QColor chatBackgroundColor() const;
	// Spielername (mit Trenner) als eingefärbtes, fettes HTML-Fragment.
	QString nickHtml(const QString &nickText) const;

	void setupEmojiPickerAction();
	void setupShortcodeCompleter();
	void updateShortcodeCompletion();
	void insertShortcodeCompletion(const QModelIndex &index);

	// ── Chat-Übersetzung ────────────────────────────────────────────────
	// Baut das anklickbare Globus-Anchor-HTML ("pokerthtranslate:<id>").
	QString translateAnchorHtml(int id, const QString &glyph) const;
	// Symbol, das die Zeile im aktuellen Zustand tragen soll: Spinner solange
	// die Übersetzung läuft, Globus wenn die Übersetzung eingeblendet ist oder
	// die Zeile unter dem Mauszeiger liegt – sonst ein unsichtbarer Platzhalter.
	QString translateGlyph(int id) const;
	// Anchor-id der Chat-Zeile in diesem Textblock (0 = nicht übersetzbar).
	int translateIdAtBlock(const QTextBlock &block) const;
	// Ermittelt die Zeile unter der (Viewport-)Mausposition und schiebt das
	// Globus-Symbol dorthin.
	void updateTranslateHover(const QPoint &viewportPos);
	void setTranslateHoverId(int id);
	// Findet den Textblock (Chat-Zeile), der den Globus-Anker dieser id enthält.
	QTextBlock findTranslateBlock(int id) const;
	// Baut den Inhalt der Chat-Zeile aus dem gespeicherten Zustand neu auf
	// (Original- ODER Übersetzungs-Körper + Globus/Spinner) und ersetzt damit
	// den zugehörigen Block im Dokument. So ERSETZT die Übersetzung den Text an
	// gleicher Stelle (statt rechts daneben zu erscheinen).
	void rebuildTranslateBlock(int id);
	// Übersetzer-Einträge wegwerfen, deren Chat-Zeile aus dem (auf
	// kMaxChatBlocks begrenzten) Verlauf herausgefallen ist. Ohne das wüchse je
	// empfangener Fremdnachricht ein Eintrag mit Quelltext + Nachrichtenkörper
	// über die ganze Sitzung mit, obwohl seine Zeile längst weg ist.
	void pruneTranslateEntries();

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

	// ── Chat-Übersetzung ────────────────────────────────────────────────
	struct TranslateEntry {
		QString sourceText;   // Rohtext der Nachricht (für die Anfrage)
		QString lineNoGlobe;  // gerenderte Chat-Zeile OHNE Globus (Original-Körper)
		QString bodyHtml;     // Original-Nachrichtenkörper (Teilstring von lineNoGlobe)
		QString translated;   // gecachte Übersetzung (leer = noch nicht geholt)
		bool inFlight = false;
		bool shown = false;   // Übersetzung aktuell eingeblendet? (Toggle)
	};
	ChatTranslatorCore *myTranslator;
	QHash<int, TranslateEntry> myTranslateEntries;  // Anchor-id -> Zustand
	QHash<int, int> myTranslateReqToId;             // Core-Request-id -> Anchor-id
	int myTranslateNextId;
	int myTranslateHoverId;                         // Zeile unter der Maus (0 = keine)
	qint64 myTranslateLastFailNoteMs;               // letzter Fehlschlag-Hinweis (Drosselung)
};

#endif

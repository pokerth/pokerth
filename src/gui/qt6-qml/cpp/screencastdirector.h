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
 *****************************************************************************/

#ifndef SCREENCASTDIRECTOR_H
#define SCREENCASTDIRECTOR_H

#include <QElapsedTimer>
#include <QList>
#include <QObject>
#include <QString>

class GameHandler;
class LobbyHandler;
class ServerConnectionHandler;
class StyleProvider;
class QTimer;

// Plays back a scripted screencast: table theme, card deck and card back are
// switched at fixed points in time WITHOUT the settings dialog appearing in the
// recording, and emoji reactions are triggered.
//
// The client's own turns are NOT handled here: the client already has an
// "Auto Check/Fold" playing mode (F7 / Alt+F, GameActionBar::runAutoAction),
// which does exactly that and with the same call(0) safeguard. The runner
// presses that key - a second implementation would only drift away from the
// real one.
//
// The director is dead code unless the environment variable
// POKERTH_SCREENCAST_CONFIG points at a JSON script – nothing is constructed,
// no context property is registered and no signal is connected. It is a
// recording aid for preview/record_screencast.py, not a game feature.
//
// The style switch itself needs nothing new: StyleProvider::setTableStyle() and
// friends already write the config key, reload the assets and emit changed(),
// so every binding in the running client rebinds on the spot.
//
// Everything the director does is written to a cue sheet (JSON lines) together
// with a wall clock timestamp. The Python runner knows when ffmpeg started and
// therefore converts those timestamps into exact video positions – that is the
// list the slide effects are cut against in post production.
class ScreencastDirector : public QObject
{
	Q_OBJECT

public:
	// The recording is scripted against one of two clocks.
	enum Anchor {
		AppStart,   // t = 0 when the director is started (the window is up)
		GameStart   // t = 0 with the first dealt hand – the usual case
	};

	explicit ScreencastDirector(QObject *parent = nullptr);

	// The script path from POKERTH_SCREENCAST_CONFIG (empty = the director stays off).
	static QString scriptPathFromEnvironment();
	static bool isEnabled()
	{
		return !scriptPathFromEnvironment().isEmpty();
	}

	// Reads and validates the script. false + errorOut on a broken script –
	// the caller then leaves the director switched off instead of recording
	// half a screencast.
	bool loadScript(const QString &path, QString *errorOut);

	void setStyleProvider(StyleProvider *provider)
	{
		m_styleProvider = provider;
	}
	// The hand number: the anchor of the game clock and the trigger of the hand
	// based cues.
	void setGameHandler(GameHandler *handler);
	// Observation only: login, join and game start are written to the log and the
	// cue sheet as [SCREENCAST] state markers. The runner synchronises its key
	// presses on them – the [NAV] lines of the QML pages are commented out and
	// are therefore no reliable signal.
	void setLobbyHandler(LobbyHandler *handler);
	void setConnectionHandler(ServerConnectionHandler *handler);

	// Starts the clock and the cue polling. Called once the QML window exists.
	void start();

signals:
	// A reaction that is to be SEEN being operated: the game page opens the real
	// reaction picker, highlights the emoji and only then sends it. Reactions
	// without this signal go out silently and only the effect above the seat
	// is visible.
	void visibleReactionRequested(const QString &emoji);

	// Puts the keyboard focus into the lobby game list, so that the runner can
	// pick and join a game with the arrow keys and Enter. The lobby deliberately
	// does NOT focus the list by itself (entering the lobby would select a row),
	// and the number of Tab steps up to the list is nothing a recording should
	// depend on.
	void focusGameListRequested();

private slots:
	void onTick();
	void onHandNumberChanged();

private:
	// One scripted step. Empty fields are not touched, so a cue may switch the
	// deck alone or the whole table look at once.
	struct Cue {
		double at = -1.0;       // seconds after the anchor (< 0 = not time based)
		int atHand = -1;        // alternatively: from this hand number on
		QString table;
		QString deck;
		QString back;
		QString reaction;
		bool reactionVisible = false;
		QString chat;
		QString marker;         // a pure post production marker (no client effect)
		bool done = false;
	};

	double elapsedSeconds() const;
	bool anchorReached() const;
	void runCue(Cue &cue);
	// A [SCREENCAST] state marker for the runner and the cue sheet.
	void markState(const QString &state);
	void writeCue(const QString &kind, const QString &detail);
	void logLine(const QString &text) const;

	StyleProvider *m_styleProvider = nullptr;
	GameHandler *m_gameHandler = nullptr;

	QTimer *m_pollTimer = nullptr;

	QList<Cue> m_cues;
	Anchor m_anchor = GameStart;
	QElapsedTimer m_clock;       // runs from start(), independent of the anchor
	double m_anchorOffset = -1.0;  // elapsedSeconds() at the moment the anchor was hit
	int m_lastHandNumber = 0;

	QString m_cueSheetPath;
};

#endif // SCREENCASTDIRECTOR_H

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

#include "screencastdirector.h"

#include "gamehandler.h"
#include "lobbyhandler.h"
#include "serverconnectionhandler.h"
#include "styleprovider.h"

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QTextStream>
#include <QTimer>

#include <algorithm>

namespace
{
// The cue poll runs deliberately coarse: the script works in seconds, and a
// finer grid would only cost wake-ups during the recording.
const int POLL_INTERVAL_MS = 200;

// The lobby page needs a moment to be built before its game list can take the
// focus.
const int LOBBY_FOCUS_DELAY_MS = 1200;

QString jsonToCompactString(const QJsonObject &obj)
{
	return QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}
}

ScreencastDirector::ScreencastDirector(QObject *parent)
	: QObject(parent)
{
}

QString ScreencastDirector::scriptPathFromEnvironment()
{
	return qEnvironmentVariable("POKERTH_SCREENCAST_CONFIG");
}

bool ScreencastDirector::loadScript(const QString &path, QString *errorOut)
{
	const auto fail = [errorOut](const QString &msg) {
		if (errorOut)
			*errorOut = msg;
		return false;
	};

	QFile file(path);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return fail(QStringLiteral("cannot read %1: %2").arg(path, file.errorString()));

	QJsonParseError parseError{};
	const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
	file.close();
	if (parseError.error != QJsonParseError::NoError)
		return fail(QStringLiteral("%1: %2 at offset %3")
					.arg(path, parseError.errorString())
					.arg(parseError.offset));
	if (!doc.isObject())
		return fail(QStringLiteral("%1: the top level element is not an object").arg(path));

	const QJsonObject root = doc.object();

	const QString anchor = root.value(QStringLiteral("clock")).toString(QStringLiteral("gameStart"));
	if (anchor == QLatin1String("appStart")) {
		m_anchor = AppStart;
	} else if (anchor == QLatin1String("gameStart")) {
		m_anchor = GameStart;
	} else {
		return fail(QStringLiteral("%1: unknown clock \"%2\" (appStart|gameStart)").arg(path, anchor));
	}

	// Default: next to the script, so a run never has to be told where its cue
	// sheet went.
	m_cueSheetPath = root.value(QStringLiteral("cueSheet")).toString();
	if (m_cueSheetPath.isEmpty()) {
		const QFileInfo info(path);
		m_cueSheetPath = info.dir().filePath(info.completeBaseName() + QStringLiteral("_cues.jsonl"));
	}

	const QJsonArray timeline = root.value(QStringLiteral("timeline")).toArray();
	if (timeline.isEmpty())
		return fail(QStringLiteral("%1: \"timeline\" is empty – there is nothing to play back").arg(path));

	m_cues.clear();
	for (int i = 0; i < timeline.size(); ++i) {
		const QJsonObject entry = timeline.at(i).toObject();
		Cue cue;
		cue.at = entry.contains(QStringLiteral("at"))
				 ? entry.value(QStringLiteral("at")).toDouble(-1.0) : -1.0;
		cue.atHand = entry.value(QStringLiteral("atHand")).toInt(-1);
		cue.table = entry.value(QStringLiteral("table")).toString();
		cue.deck = entry.value(QStringLiteral("deck")).toString();
		cue.back = entry.value(QStringLiteral("back")).toString();
		cue.reaction = entry.value(QStringLiteral("reaction")).toString();
		cue.reactionVisible = entry.value(QStringLiteral("visible")).toBool(false);
		cue.chat = entry.value(QStringLiteral("chat")).toString();
		cue.marker = entry.value(QStringLiteral("marker")).toString();

		if (cue.at < 0.0 && cue.atHand < 0)
			return fail(QStringLiteral("%1: timeline[%2] has neither \"at\" nor \"atHand\"").arg(path).arg(i));
		if (cue.table.isEmpty() && cue.deck.isEmpty() && cue.back.isEmpty()
				&& cue.reaction.isEmpty() && cue.chat.isEmpty() && cue.marker.isEmpty())
			return fail(QStringLiteral("%1: timeline[%2] does nothing").arg(path).arg(i));

		m_cues.append(cue);
	}

	// The script may be written in any order; the playback wants it sorted by
	// time, with the hand anchored cues behind them (they fire on their own trigger).
	std::stable_sort(m_cues.begin(), m_cues.end(), [](const Cue &a, const Cue &b) {
		if ((a.at < 0.0) != (b.at < 0.0))
			return b.at < 0.0;
		return a.at < b.at;
	});

	logLine(QStringLiteral("script %1 loaded: %2 cues, clock=%3, cueSheet=%4")
			.arg(path).arg(m_cues.size()).arg(anchor).arg(m_cueSheetPath));
	return true;
}

void ScreencastDirector::setGameHandler(GameHandler *handler)
{
	m_gameHandler = handler;
	if (!m_gameHandler)
		return;

	// The hand number is the anchor of the game clock and the trigger of the
	// hand based cues.
	connect(m_gameHandler, &GameHandler::handNumberChanged,
			this, &ScreencastDirector::onHandNumberChanged);
}

void ScreencastDirector::setLobbyHandler(LobbyHandler *handler)
{
	if (!handler)
		return;
	connect(handler, &LobbyHandler::selfJoinedGame, this, [this]() {
		markState(QStringLiteral("joinedGame"));
	});
	connect(handler, &LobbyHandler::gameStarted, this, [this]() {
		markState(QStringLiteral("gameStarted"));
	});
	connect(handler, &LobbyHandler::removedFromGame, this, [this](int reason) {
		markState(QStringLiteral("removedFromGame reason=%1").arg(reason));
	});
	connect(handler, &LobbyHandler::errorOccurred, this, [this](const QString &message) {
		markState(QStringLiteral("lobbyError %1").arg(message));
	});
}

void ScreencastDirector::setConnectionHandler(ServerConnectionHandler *handler)
{
	if (!handler)
		return;
	connect(handler, &ServerConnectionHandler::showLobby, this, [this]() {
		markState(QStringLiteral("lobby"));
		// The page is only built after this signal – put the focus into the game
		// list one event loop pass later, then report it. The runner waits for
		// that marker before it presses an arrow key.
		QTimer::singleShot(LOBBY_FOCUS_DELAY_MS, this, [this]() {
			emit focusGameListRequested();
			markState(QStringLiteral("gameListFocused"));
		});
	});
	connect(handler, &ServerConnectionHandler::connectionFailed, this, [this](const QString &message) {
		markState(QStringLiteral("connectionFailed %1").arg(message));
	});
}

void ScreencastDirector::markState(const QString &state)
{
	writeCue(QStringLiteral("state"), state);
	logLine(QStringLiteral("state %1").arg(state));
}

void ScreencastDirector::start()
{
	if (m_cues.isEmpty())
		return;

	m_clock.start();
	if (m_anchor == AppStart)
		m_anchorOffset = 0.0;

	m_pollTimer = new QTimer(this);
	m_pollTimer->setInterval(POLL_INTERVAL_MS);
	connect(m_pollTimer, &QTimer::timeout, this, &ScreencastDirector::onTick);
	m_pollTimer->start();

	writeCue(QStringLiteral("start"),
			 m_anchor == AppStart ? QStringLiteral("appStart") : QStringLiteral("waiting for gameStart"));
	logLine(QStringLiteral("started"));
}

double ScreencastDirector::elapsedSeconds() const
{
	return m_clock.isValid() ? (double(m_clock.elapsed()) / 1000.0) : 0.0;
}

bool ScreencastDirector::anchorReached() const
{
	return m_anchorOffset >= 0.0;
}

void ScreencastDirector::onHandNumberChanged()
{
	if (!m_gameHandler)
		return;
	const int hand = m_gameHandler->handNumber();
	if (hand == m_lastHandNumber)
		return;
	m_lastHandNumber = hand;

	// The first dealt hand starts the game clock – everything the script says
	// in seconds is counted from here.
	if (m_anchor == GameStart && !anchorReached() && hand > 0) {
		m_anchorOffset = elapsedSeconds();
		writeCue(QStringLiteral("anchor"), QStringLiteral("gameStart at hand %1").arg(hand));
		logLine(QStringLiteral("game clock started with hand %1").arg(hand));
	}

	// Hand anchored cues fire here, not in the poll.
	for (Cue &cue : m_cues) {
		if (cue.done || cue.atHand < 0 || hand < cue.atHand)
			continue;
		runCue(cue);
	}
}

void ScreencastDirector::onTick()
{
	if (!anchorReached())
		return;

	const double t = elapsedSeconds() - m_anchorOffset;
	bool pending = false;
	for (Cue &cue : m_cues) {
		if (cue.done)
			continue;
		if (cue.at < 0.0) {
			pending = true;   // hand anchored – waits for its own trigger
			continue;
		}
		if (t + 1e-6 >= cue.at)
			runCue(cue);
		else
			pending = true;
	}

	if (!pending) {
		m_pollTimer->stop();
		writeCue(QStringLiteral("end"), QStringLiteral("timeline finished"));
		logLine(QStringLiteral("timeline finished"));
	}
}

void ScreencastDirector::runCue(Cue &cue)
{
	cue.done = true;

	QJsonObject detail;
	if (m_styleProvider) {
		if (!cue.table.isEmpty()) {
			m_styleProvider->setTableStyle(cue.table);
			detail.insert(QStringLiteral("table"), cue.table);
		}
		if (!cue.deck.isEmpty()) {
			m_styleProvider->setCardDeckStyle(cue.deck);
			detail.insert(QStringLiteral("deck"), cue.deck);
		}
		if (!cue.back.isEmpty()) {
			m_styleProvider->setCardBackStyle(cue.back);
			detail.insert(QStringLiteral("back"), cue.back);
		}
	}

	if (!cue.reaction.isEmpty()) {
		detail.insert(QStringLiteral("reaction"), cue.reaction);
		detail.insert(QStringLiteral("visible"), cue.reactionVisible);
		if (cue.reactionVisible) {
			// The game page opens the real picker and only sends afterwards, so
			// the viewer sees how the feature is operated.
			emit visibleReactionRequested(cue.reaction);
		} else if (m_gameHandler) {
			// The chat convention of the web client – the same path the picker uses.
			m_gameHandler->sendChat(QStringLiteral("/emoji ") + cue.reaction);
		}
	}

	if (!cue.chat.isEmpty() && m_gameHandler) {
		m_gameHandler->sendChat(cue.chat);
		detail.insert(QStringLiteral("chat"), cue.chat);
	}

	if (!cue.marker.isEmpty())
		detail.insert(QStringLiteral("marker"), cue.marker);

	writeCue(QStringLiteral("cue"), jsonToCompactString(detail));
	logLine(QStringLiteral("cue %1").arg(jsonToCompactString(detail)));
}

void ScreencastDirector::writeCue(const QString &kind, const QString &detail)
{
	if (m_cueSheetPath.isEmpty())
		return;

	QJsonObject entry;
	// Two clocks per line: the script time the cue was written for, and the wall
	// clock. The runner knows the ffmpeg start and turns the latter into an
	// exact video position for the post production.
	entry.insert(QStringLiteral("kind"), kind);
	entry.insert(QStringLiteral("detail"), detail);
	entry.insert(QStringLiteral("t"), anchorReached() ? (elapsedSeconds() - m_anchorOffset) : -1.0);
	entry.insert(QStringLiteral("appT"), elapsedSeconds());
	entry.insert(QStringLiteral("wallMs"), QDateTime::currentMSecsSinceEpoch());
	entry.insert(QStringLiteral("hand"), m_lastHandNumber);

	// Appended and flushed line by line: an aborted run still leaves a usable
	// cue sheet behind.
	QFile file(m_cueSheetPath);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
		return;
	QTextStream out(&file);
	out << jsonToCompactString(entry) << Qt::endl;
	file.close();
}

void ScreencastDirector::logLine(const QString &text) const
{
	// The marker the Python runner greps for in the client log.
	qDebug().noquote() << QStringLiteral("[SCREENCAST]") << text;
}

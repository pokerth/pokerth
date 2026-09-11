/*****************************************************************************
 * Community "suggest" for the widget client (see communitysuggest.h).
 * Ported from the legacy bbcbot (bbcbotplayerdb) and identical to the
 * QML version (BotSuggest.qml).
 *****************************************************************************/
#include "communitysuggest.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDateTime>
#include <QRegularExpression>
#include <QRandomGenerator>
#include <algorithm>

namespace
{

const char *const BASE_URL = "https://bbc.pokerth.net/exp3/bbcbot/";
const qint64 CACHE_TTL_MS = 15 * 60 * 1000;
// Allowlisted by Cloudflare – identical to the Qt widgets client (upload/download helper).
const char *const POKERTH_USER_AGENT = "PokerTH/2.0 (Qt Network)";

// suggestionscore2 of the bbcbot: no tickets ⇒ 0; otherwise (tickets<<11)+(games<<4)+rating.
int score2(int rating, int tickets, int games)
{
	if (tickets <= 0)
		return 0;
	return (tickets << 11) + (games << 4) + rating;
}

// Lookup key for matching the lobby nick ⇔ the botfile. Server nicks
// may contain leading/trailing spaces (the registered account
// "tammnt " for instance), the botfiles carry the same player trimmed – and
// the other way round the minidb contains "silver skies- " with a space. Without
// this normalization such a player silently drops out of every suggestion.
// Only the key is trimmed, the name that is output stays
// unchanged (names may carry decorative characters, e.g. "* ghoti *").
QString suggestKey(const QString &name)
{
	return name.trimmed().toLower();
}

// Request for a botfile. The user agent expected by Cloudflare has to hang
// on EVERY one of these requests, otherwise the filter replies instead of the file.
QNetworkRequest botFileRequest(const QString &filename)
{
	QNetworkRequest request(QUrl(QString::fromLatin1(BASE_URL) + filename));
	request.setRawHeader("User-Agent", POKERTH_USER_AGENT);
	request.setTransferTimeout(15000);
	return request;
}

struct Scored {
	QString name;   // the name to output (the DB or WEC list name)
	int score;
	QString game;   // gesetzt = spielt gerade an diesem Tisch
};

bool scoreDesc(const Scored &a, const Scored &b)
{
	return a.score > b.score;
}

// Builds the suggestion text: header row, then ONE player per line – first the
// idle players, then in last place those currently playing, each annotated with
// " (playing in game …)". Both groups are limited to `limit`;
// emptyText if both are empty.
// The line break is a real "\n"; the chat display converts it into <br>
// (ChatTools::showLocalNote), because the chat history is HTML.
QString buildMessage(const QString &headline, const QList<Scored> &idle,
					 const QList<Scored> &busy, int limit, const QString &emptyText)
{
	if (idle.isEmpty() && busy.isEmpty())
		return emptyText;
	QStringList parts;
	for (int i = 0; i < idle.size() && i < limit; ++i)
		parts << idle.at(i).name;
	for (int j = 0; j < busy.size() && j < limit; ++j)
		parts << (busy.at(j).name + " (playing in game " + busy.at(j).game + ")");
	return headline + QLatin1Char('\n') + parts.join(QLatin1Char('\n'));
}

} // namespace

CommunitySuggest::CommunitySuggest(QObject *parent)
	: QObject(parent)
{
}

// Identical to the communityPresets of the QML client (Config.BotSuggest.presets).
const QList<CommunitySuggest::CommunityTemplate> &CommunitySuggest::templates()
{
	static const QList<CommunityTemplate> table = QList<CommunityTemplate> {
		{
			"BBC Step 1", "step1", "", 3000, 15, false, 11, 5, 10,
			{20,25,30,40,50,60,80,100,120,150,200,250,300,400,500,600,800,1000,1200,1500,2000,2500,3000,4000,5000,6000,8000,10000,12000,15000}
		},
		{
			"BBC Step 2", "step2", "", 4000, 20, false, 11, 5, 10,
			{25,30,40,50,60,80,100,120,150,200,250,300,400,500,600,800,1000,1200,1500,2000,2500,3000,4000,5000,6000,8000,10000,12000,15000,20000}
		},
		{
			"BBC Step 3", "step3", "", 5000, 25, false, 11, 5, 10,
			{30,40,50,60,80,100,120,150,200,250,300,400,500,600,800,1000,1200,1500,2000,2500,3000,4000,5000,6000,8000,10000,12000,15000,20000,25000}
		},
		{
			"BBC Step 4", "step4", "", 10000, 50, false, 11, 5, 10,
			{60,80,100,120,150,200,250,300,400,500,600,800,1000,1200,1500,2000,2500,3000,4000,5000,6000,8000,10000,12000,15000,20000,25000,30000,40000,50000}
		},
		{ "Monthly Cup", "", "mcup", 10000, 50, true, 16, 5, 10, {} },
		{ "Monthly Cup Final", "", "mcupfinal", 10000, 50, true, 22, 5, 12, {} },
		{ "WEC", "wec", "", 10000, 50, true, 22, 5, 12, {} },
		{ "WEC Monthly Final", "", "", 10000, 50, true, 25, 5, 15, {} },
		{ "WEC Grand Final", "wec", "", 10000, 50, true, 35, 5, 25, {} }
	};
	return table;
}

QString CommunitySuggest::suggestTypeForGame(const GameData &data)
{
	for (const CommunityTemplate &t : templates()) {
		if (t.suggestType.isEmpty())
			continue;
		if (t.startCash != data.startMoney || t.firstSmallBlind != data.firstSmallBlind)
			continue;
		if (t.blinds.size() != static_cast<int>(data.manualBlindsList.size()))
			continue;
		if (!t.blinds.isEmpty()) {
			bool same = true;
			int i = 0;
			for (int blind : data.manualBlindsList) {
				if (t.blinds.at(i++) != blind) {
					same = false;
					break;
				}
			}
			if (!same)
				continue;
		} else {
			// Double the blinds (WEC): starting money + small blind are no
			// signature here, so check the raise interval and the timeout as well.
			const bool onHands = data.raiseIntervalMode == RAISE_ON_HANDNUMBER;
			if (t.raiseOnHands != onHands)
				continue;
			if (onHands) {
				if (t.raiseEveryHands != data.raiseSmallBlindEveryHandsValue)
					continue;
			} else if (t.raiseEveryMinutes != data.raiseSmallBlindEveryMinutesValue) {
				continue;
			}
			if (t.playerActionTimeout != data.playerActionTimeoutSec)
				continue;
		}
		return t.suggestType;
	}
	return QString();
}

QString CommunitySuggest::adminFile(const QString &type)
{
	if (type == QLatin1String("wec"))
		return QStringLiteral("wecadmins.txt");
	static const QRegularExpression stepRe(QStringLiteral("^step[1-4]$"));
	if (stepRe.match(type).hasMatch())
		return QStringLiteral("bbcadmins.txt");
	return QString();
}

bool CommunitySuggest::isCommunityAdmin(const QString &type) const
{
	const QString file = adminFile(type);
	if (file.isEmpty())
		return false;
	return m_admins.value(file).isAdmin;
}

void CommunitySuggest::applyCommunityAdmin(const QString &file, const QString &nick)
{
	m_admins[file].isAdmin = m_admins[file].names.contains(suggestKey(nick));
	emit communityAdminResolved();
}

void CommunitySuggest::requestCommunityAdmin(const QString &type, const QString &nick)
{
	const QString file = adminFile(type);
	if (file.isEmpty() || nick.isEmpty())
		return;
	AdminList &list = m_admins[file];
	if (list.inFlight)
		return;

	const qint64 now = QDateTime::currentMSecsSinceEpoch();
	if (list.loaded && (now - list.ts) < CACHE_TTL_MS) {
		applyCommunityAdmin(file, nick);   // fresh cache ⇒ without the network
		return;
	}
	// Throttle FAILURES as well: the caller hangs off the button visibility
	// and asks again on every change of the player list – without this lock
	// a missing/unreachable file would cause one download per join/leave.
	if (list.lastTry != 0 && (now - list.lastTry) < CACHE_TTL_MS)
		return;
	list.lastTry = now;

	list.inFlight = true;
	if (!m_nam)
		m_nam = new QNetworkAccessManager(this);

	QNetworkReply *reply = m_nam->get(botFileRequest(file));
	const QString wanted = nick;
	connect(reply, &QNetworkReply::finished, this, [this, reply, file, wanted]() {
		AdminList &entry = m_admins[file];
		entry.inFlight = false;
		const bool ok = reply->error() == QNetworkReply::NoError;
		const QByteArray data = ok ? reply->readAll() : QByteArray();
		reply->deleteLater();
		if (ok && !data.isEmpty()) {
			parseNameList(data, entry.names);
			entry.loaded = true;
			entry.ts = QDateTime::currentMSecsSinceEpoch();
		}
		// Failure ⇒ fall back to the (possibly expired) old data; if there is
		// nothing at all, the button stays off – as without the feature.
		applyCommunityAdmin(file, wanted);
	});
}

bool CommunitySuggest::isSuggestType(const QString &type)
{
	if (type == QLatin1String("wec"))
		return true;
	static const QRegularExpression stepRe(QStringLiteral("^step[1-4]$"));
	return stepRe.match(type).hasMatch();
}

QString CommunitySuggest::fileNameForKind(const QString &kind)
{
	if (kind == QLatin1String("wec"))
		return QStringLiteral("weclist.txt");
	if (kind == QLatin1String("gameslist"))
		return QStringLiteral("gameslist.txt");
	return QStringLiteral("minidb.txt");
}

void CommunitySuggest::ensure(const QString &kind, const std::function<void(bool)> &done)
{
	FileCache &c = m_files[kind];
	const qint64 now = QDateTime::currentMSecsSinceEpoch();
	if (c.loaded && (now - c.ts) < CACHE_TTL_MS) {
		done(true);   // fresh cache ⇒ without the network and without a detour
		return;
	}

	c.queue.append(done);
	if (c.inFlight)
		return;       // already running: this caller only attaches itself
	c.inFlight = true;

	if (!m_nam)
		m_nam = new QNetworkAccessManager(this);

	QNetworkReply *reply = m_nam->get(botFileRequest(fileNameForKind(kind)));
	connect(reply, &QNetworkReply::finished, this, [this, reply, kind]() {
		const QByteArray data = reply->error() == QNetworkReply::NoError
								? reply->readAll() : QByteArray();
		reply->deleteLater();

		FileCache &entry = m_files[kind];
		entry.inFlight = false;
		if (!data.isEmpty()) {
			if (kind == QLatin1String("wec"))
				parseNameList(data, m_wec);
			else if (kind == QLatin1String("gameslist"))
				parseGameslist(data);
			else
				parseDb(data);
			entry.loaded = true;
			entry.ts = QDateTime::currentMSecsSinceEpoch();
		}
		// Failure ⇒ fall back to the (possibly expired) old data; if there is
		// nothing at all, the callback reports false (the caller then shows nothing).
		const bool ok = entry.loaded;
		// Copy and clear the queue BEFORE calling: a callback may call
		// ensure() again, and inserting into m_files would invalidate the
		// reference `entry` (QHash rehash).
		const QList<std::function<void(bool)> > waiting = entry.queue;
		entry.queue.clear();
		for (int i = 0; i < waiting.size(); ++i)
			waiting.at(i)(ok);
	});
}

// minidb.txt: name<TAB>ts2<TAB>ts3<TAB>ts4<TAB>rating<TAB>games. The name that is
// output is NOT trimmed (it may contain leading/trailing characters, e.g.
// "* ghoti *"), only the key (suggestKey); only take over lines with rating > 0
// (like the bot).
void CommunitySuggest::parseDb(const QByteArray &data)
{
	m_db.clear();
	const QStringList lines = QString::fromUtf8(data).split(QRegularExpression(QStringLiteral("\\r?\\n")));
	for (const QString &line : lines) {
		if (line.isEmpty())
			continue;
		const QStringList f = line.split(QLatin1Char('\t'));
		if (f.size() < 6)
			continue;
		bool ok = false;
		const int rating = f.at(4).toInt(&ok);
		if (!ok || rating <= 0)
			continue;
		DbEntry e;
		e.name = f.at(0);
		e.ts2 = f.at(1).toInt();
		e.ts3 = f.at(2).toInt();
		e.ts4 = f.at(3).toInt();
		e.rating = rating;
		e.games = f.at(5).toInt();
		m_db.insert(suggestKey(e.name), e);
	}
}

// weclist.txt / bbcadmins.txt: one player name per line → { lowercase:
// originalName }. Both botfiles share this format.
void CommunitySuggest::parseNameList(const QByteArray &data, QHash<QString, QString> &target)
{
	target.clear();
	const QStringList lines = QString::fromUtf8(data).split(QRegularExpression(QStringLiteral("\\r?\\n")));
	for (const QString &line : lines) {
		const QString name = line.trimmed();
		if (name.isEmpty())
			continue;
		target.insert(suggestKey(name), name);
	}
}

// gameslist.txt: "#command#permgroup#Game Title Prefix#" (at least 4×'#'; ignore
// comments "//" and lines with fewer '#') → { command: titlePrefix }.
void CommunitySuggest::parseGameslist(const QByteArray &data)
{
	m_gameslist.clear();
	const QStringList lines = QString::fromUtf8(data).split(QRegularExpression(QStringLiteral("\\r?\\n")));
	for (const QString &raw : lines) {
		const QString line = raw.trimmed();
		if (line.isEmpty() || line.startsWith(QLatin1String("//")))
			continue;
		const QStringList parts = line.split(QLatin1Char('#'));
		if (parts.size() < 5)   // "" + command + perm + title + "" ⇒ ≥ 4 '#'
			continue;
		const QString cmd = parts.at(1).trimmed();
		const QString title = parts.at(3).trimmed();
		if (cmd.isEmpty() || title.isEmpty())
			continue;
		m_gameslist.insert(cmd, title);
	}
}

QString CommunitySuggest::suggestStep(int step, const QStringList &idleNames,
									  const QList<PlayingPlayer> &playing) const
{
	QList<Scored> idle;
	for (const QString &n : idleNames) {
		auto it = m_db.constFind(suggestKey(n));
		if (it == m_db.constEnd())
			continue;
		const int tickets = step == 1 ? 1 : (step == 2 ? it->ts2 : (step == 3 ? it->ts3 : it->ts4));
		const int s = score2(it->rating, tickets, it->games);
		if (s <= 10)
			continue;
		idle << Scored{ it->name, s, QString() };
	}
	// Step 1 computes with a fixed ticket=1 → practically every DB player
	// qualifies. So do NOT suggest those currently playing as well, otherwise
	// the list gets too long (only show them from step 2 on).
	QList<Scored> busy;
	if (step != 1) {
		for (const PlayingPlayer &p : playing) {
			auto it = m_db.constFind(suggestKey(p.name));
			if (it == m_db.constEnd())
				continue;
			const int tickets = step == 2 ? it->ts2 : (step == 3 ? it->ts3 : it->ts4);
			const int s = score2(it->rating, tickets, it->games);
			if (s <= 10)
				continue;
			busy << Scored{ it->name, s, p.game };
		}
	}
	std::sort(idle.begin(), idle.end(), scoreDesc);
	std::sort(busy.begin(), busy.end(), scoreDesc);
	return buildMessage(QStringLiteral("I suggest the following players for step %1:").arg(step),
						idle, busy, 12, QStringLiteral("Sorry, no player found to suggest"));
}

QString CommunitySuggest::suggestWec(const QStringList &idleNames,
									 const QList<PlayingPlayer> &playing) const
{
	QList<Scored> idle;
	for (const QString &n : idleNames) {
		auto it = m_wec.constFind(suggestKey(n));
		if (it == m_wec.constEnd())
			continue;
		// Random score as in the legacy bot → a random order.
		idle << Scored{ it.value(), int(QRandomGenerator::global()->bounded(1, 1000000)), QString() };
	}
	QList<Scored> busy;
	for (const PlayingPlayer &p : playing) {
		auto it = m_wec.constFind(suggestKey(p.name));
		if (it == m_wec.constEnd())
			continue;
		busy << Scored{ it.value(), int(QRandomGenerator::global()->bounded(1, 1000000)), p.game };
	}
	std::sort(idle.begin(), idle.end(), scoreDesc);
	std::sort(busy.begin(), busy.end(), scoreDesc);
	return buildMessage(QStringLiteral("I suggest the following players for wec:"),
						idle, busy, 10, QStringLiteral("Sorry, no wec player found to suggest"));
}

void CommunitySuggest::suggest(const QString &type, const QStringList &idleNames,
							   const QList<PlayingPlayer> &playing,
							   QObject *context, const ResultCallback &onReady)
{
	// A guard for the caller: the download runs asynchronously, the dialog may
	// have been closed (and destroyed) by the time the reply arrives.
	const QPointer<QObject> guard(context);
	static const QRegularExpression stepRe(QStringLiteral("^step([1-4])$"));
	const QRegularExpressionMatch m = stepRe.match(type);
	if (m.hasMatch()) {
		const int step = m.captured(1).toInt();
		ensure(QStringLiteral("db"), [this, guard, context, onReady, step, idleNames, playing](bool ok) {
			if (context && guard.isNull())
				return;
			onReady(ok ? suggestStep(step, idleNames, playing) : QString());
		});
		return;
	}
	if (type == QLatin1String("wec")) {
		ensure(QStringLiteral("wec"), [this, guard, context, onReady, idleNames, playing](bool ok) {
			if (context && guard.isNull())
				return;
			onReady(ok ? suggestWec(idleNames, playing) : QString());
		});
		return;
	}
	onReady(QString());
}

void CommunitySuggest::gameTitlePrefix(const QString &command, QObject *context,
									   const ResultCallback &onReady)
{
	const QPointer<QObject> guard(context);
	ensure(QStringLiteral("gameslist"), [this, guard, context, onReady, command](bool ok) {
		if (context && guard.isNull())
			return;
		onReady(ok ? m_gameslist.value(command) : QString());
	});
}

void CommunitySuggest::prefetchGameTitles()
{
	ensure(QStringLiteral("gameslist"), [](bool) {});
}

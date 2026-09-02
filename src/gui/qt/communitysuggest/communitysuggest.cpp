/*****************************************************************************
 * Community-„Suggest" für den Widget-Client (siehe communitysuggest.h).
 * Portiert aus dem Legacy-bbcbot (bbcbotplayerdb) und deckungsgleich mit der
 * QML-Fassung (BotSuggest.qml).
 *****************************************************************************/
#include "communitysuggest.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDateTime>
#include <QRegularExpression>
#include <QRandomGenerator>
#include <algorithm>

namespace {

const char *const BASE_URL = "https://bbc.pokerth.net/exp3/bbcbot/";
const qint64 CACHE_TTL_MS = 15 * 60 * 1000;
// Von Cloudflare allowlistet – identisch zum Qt-Widgets-Client (upload-/downloadhelper).
const char *const POKERTH_USER_AGENT = "PokerTH/2.0 (Qt Network)";

// suggestionscore2 des bbcbot: keine Tickets ⇒ 0; sonst (tickets<<11)+(games<<4)+rating.
int score2(int rating, int tickets, int games)
{
	if (tickets <= 0)
		return 0;
	return (tickets << 11) + (games << 4) + rating;
}

// Nachschlage-Schlüssel für den Abgleich Lobby-Nick ⇔ Botfile. Server-Nicks
// dürfen führende/anhängende Leerzeichen enthalten (der registrierte Account
// "tammnt " z. B.), die Botfiles führen denselben Spieler getrimmt – und
// umgekehrt steht in der minidb auch "silver skies- " mit Leerzeichen. Ohne
// diese Normalisierung fällt so ein Spieler stillschweigend aus jedem Vorschlag
// heraus. Nur der Schlüssel wird getrimmt, der ausgegebene Name bleibt
// unverändert (Namen dürfen Zierzeichen tragen, z. B. "* ghoti *").
QString suggestKey(const QString &name)
{
	return name.trimmed().toLower();
}

// Anfrage auf eine Botfile-Datei. Der von Cloudflare erwartete User-Agent muss
// an JEDER dieser Anfragen hängen, sonst antwortet der Filter statt der Datei.
QNetworkRequest botFileRequest(const QString &filename)
{
	QNetworkRequest request(QUrl(QString::fromLatin1(BASE_URL) + filename));
	request.setRawHeader("User-Agent", POKERTH_USER_AGENT);
	request.setTransferTimeout(15000);
	return request;
}

struct Scored {
	QString name;   // auszugebender Name (DB- bzw. WEC-Listenname)
	int score;
	QString game;   // gesetzt = spielt gerade an diesem Tisch
};

bool scoreDesc(const Scored &a, const Scored &b) { return a.score > b.score; }

// Baut den Vorschlagstext: Kopfzeile, danach EIN Spieler pro Zeile – zuerst die
// idle Spieler, dann an letzter Stelle die gerade spielenden, je mit
// " (playing in game …)" annotiert. Beide Gruppen auf `limit` begrenzt;
// emptyText, falls beide leer.
// Der Zeilenumbruch ist ein echtes "\n"; die Chat-Anzeige setzt es in <br> um
// (ChatTools::showLocalNote), weil der Chat-Verlauf HTML ist.
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

// Deckungsgleich mit den communityPresets des QML-Clients (Config.BotSuggest.presets).
const QList<CommunitySuggest::CommunityTemplate> &CommunitySuggest::templates()
{
	static const QList<CommunityTemplate> table = QList<CommunityTemplate>{
		{ "BBC Step 1", "step1", "", 3000, 15, false, 11, 5, 10,
		  {20,25,30,40,50,60,80,100,120,150,200,250,300,400,500,600,800,1000,1200,1500,2000,2500,3000,4000,5000,6000,8000,10000,12000,15000} },
		{ "BBC Step 2", "step2", "", 4000, 20, false, 11, 5, 10,
		  {25,30,40,50,60,80,100,120,150,200,250,300,400,500,600,800,1000,1200,1500,2000,2500,3000,4000,5000,6000,8000,10000,12000,15000,20000} },
		{ "BBC Step 3", "step3", "", 5000, 25, false, 11, 5, 10,
		  {30,40,50,60,80,100,120,150,200,250,300,400,500,600,800,1000,1200,1500,2000,2500,3000,4000,5000,6000,8000,10000,12000,15000,20000,25000} },
		{ "BBC Step 4", "step4", "", 10000, 50, false, 11, 5, 10,
		  {60,80,100,120,150,200,250,300,400,500,600,800,1000,1200,1500,2000,2500,3000,4000,5000,6000,8000,10000,12000,15000,20000,25000,30000,40000,50000} },
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
			// Blinds verdoppeln (WEC): Startgeld + Small Blind sind hier keine
			// Signatur, deshalb zusätzlich Raise-Intervall und Timeout prüfen.
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
		applyCommunityAdmin(file, nick);   // frischer Cache ⇒ ohne Netz
		return;
	}
	// Auch FEHLSCHLÄGE drosseln: der Aufrufer hängt an der Button-Sichtbarkeit
	// und fragt bei jeder Änderung der Spielerliste erneut – ohne diese Sperre
	// liefe bei fehlender/unerreichbarer Datei ein Download pro Join/Leave.
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
		// Fehlschlag ⇒ auf (ggf. abgelaufene) Altdaten zurückfallen; ist gar
		// nichts da, bleibt der Button aus – wie ohne das Feature.
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
		done(true);   // frischer Cache ⇒ ohne Netz und ohne Umweg
		return;
	}

	c.queue.append(done);
	if (c.inFlight)
		return;       // läuft schon: dieser Aufrufer hängt sich nur an
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
		// Fehlschlag ⇒ auf (ggf. abgelaufene) Altdaten zurückfallen; ist gar
		// nichts da, meldet der Callback false (der Aufrufer zeigt dann nichts).
		const bool ok = entry.loaded;
		// Warteschlange VOR dem Aufrufen kopieren und leeren: ein Callback darf
		// ensure() erneut aufrufen, und das Einfügen in m_files würde die
		// Referenz `entry` ungültig machen (QHash-Rehash).
		const QList<std::function<void(bool)> > waiting = entry.queue;
		entry.queue.clear();
		for (int i = 0; i < waiting.size(); ++i)
			waiting.at(i)(ok);
	});
}

// minidb.txt: Name<TAB>ts2<TAB>ts3<TAB>ts4<TAB>rating<TAB>games. Der ausgegebene
// Name wird NICHT getrimmt (kann führende/anhängende Zeichen enthalten, z. B.
// "* ghoti *"), nur der Schlüssel (suggestKey); nur Zeilen mit rating > 0
// übernehmen (wie der Bot).
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

// weclist.txt / bbcadmins.txt: ein Spielername pro Zeile → { lowercase:
// originalName }. Beide Botfiles teilen dieses Format.
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

// gameslist.txt: "#command#permgroup#Game Title Prefix#" (mind. 4×'#'; Kommentare
// "//" und Zeilen mit weniger '#' ignorieren) → { command: titlePrefix }.
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
	// Step 1 rechnet mit festem Ticket=1 → praktisch jeder DB-Spieler
	// qualifiziert sich. Die gerade spielenden dann NICHT mit vorschlagen, sonst
	// wird die Liste zu lang (erst ab Step 2 einblenden).
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
		// Zufalls-Score wie im Legacy-Bot → zufällige Reihenfolge.
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
	// Wächter für den Aufrufer: der Download läuft asynchron, der Dialog kann
	// bis zur Antwort geschlossen (und zerstört) worden sein.
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

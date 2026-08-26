/*****************************************************************************
 * Community-„Suggest" für den Widget-Client (siehe communitysuggest.h).
 * Portiert aus dem Legacy-bbcbot (bbcbotplayerdb) und deckungsgleich mit der
 * QML-Fassung (BotSuggest.qml).
 *****************************************************************************/
#include "communitysuggest.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
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

QString CommunitySuggest::suggestTypeForGame(int startMoney, int firstSmallBlind,
                                             const std::list<int> &manualBlinds)
{
	if (manualBlinds.empty())
		return QString();
	for (const CommunityTemplate &t : templates()) {
		if (t.suggestType.isEmpty() || t.blinds.isEmpty())
			continue;
		if (t.startCash != startMoney || t.firstSmallBlind != firstSmallBlind)
			continue;
		if (t.blinds.size() != static_cast<int>(manualBlinds.size()))
			continue;
		bool same = true;
		int i = 0;
		for (int blind : manualBlinds) {
			if (t.blinds.at(i++) != blind) {
				same = false;
				break;
			}
		}
		if (same)
			return t.suggestType;
	}
	return QString();
}

bool CommunitySuggest::isBbcAdmin() const
{
	return m_bbcAdmin;
}

void CommunitySuggest::applyBbcAdmin(const QString &nick)
{
	m_bbcAdmin = m_bbcAdmins.contains(suggestKey(nick));
	emit bbcAdminResolved();
}

void CommunitySuggest::requestBbcAdmin(const QString &nick)
{
	if (nick.isEmpty() || m_bbcAdminInFlight)
		return;

	const qint64 now = QDateTime::currentMSecsSinceEpoch();
	if (m_bbcAdminsLoaded && (now - m_bbcAdminsTs) < CACHE_TTL_MS) {
		applyBbcAdmin(nick);   // frischer Cache ⇒ ohne Netz
		return;
	}
	// Auch FEHLSCHLÄGE drosseln: der Aufrufer hängt an der Button-Sichtbarkeit
	// und fragt bei jeder Änderung der Spielerliste erneut – ohne diese Sperre
	// liefe bei fehlender/unerreichbarer Datei ein Download pro Join/Leave.
	if (m_bbcAdminLastTry != 0 && (now - m_bbcAdminLastTry) < CACHE_TTL_MS)
		return;
	m_bbcAdminLastTry = now;

	m_bbcAdminInFlight = true;
	if (!m_bbcAdminNam)
		m_bbcAdminNam = new QNetworkAccessManager(this);

	QNetworkRequest request(QUrl(QString::fromLatin1(BASE_URL) + QStringLiteral("bbcadmins.txt")));
	request.setRawHeader("User-Agent", POKERTH_USER_AGENT);
	request.setTransferTimeout(15000);

	QNetworkReply *reply = m_bbcAdminNam->get(request);
	const QString wanted = nick;
	connect(reply, &QNetworkReply::finished, this, [this, reply, wanted]() {
		m_bbcAdminInFlight = false;
		const bool ok = reply->error() == QNetworkReply::NoError;
		const QByteArray data = ok ? reply->readAll() : QByteArray();
		reply->deleteLater();
		if (ok && !data.isEmpty()) {
			parseNameList(data, m_bbcAdmins);
			m_bbcAdminsLoaded = true;
			m_bbcAdminsTs = QDateTime::currentMSecsSinceEpoch();
		}
		// Fehlschlag ⇒ auf (ggf. abgelaufene) Altdaten zurückfallen; ist gar
		// nichts da, bleibt der Button aus – wie ohne das Feature.
		applyBbcAdmin(wanted);
	});
}

bool CommunitySuggest::isSuggestType(const QString &type)
{
	if (type == QLatin1String("wec"))
		return true;
	static const QRegularExpression stepRe(QStringLiteral("^step[1-4]$"));
	return stepRe.match(type).hasMatch();
}

QByteArray CommunitySuggest::download(const QString &filename) const
{
	QNetworkAccessManager manager;
	QNetworkRequest request(QUrl(QString::fromLatin1(BASE_URL) + filename));
	request.setRawHeader("User-Agent", POKERTH_USER_AGENT);
	request.setTransferTimeout(15000);

	QNetworkReply *reply = manager.get(request);
	QEventLoop loop;
	QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
	loop.exec();

	QByteArray data;
	if (reply->error() == QNetworkReply::NoError)
		data = reply->readAll();
	reply->deleteLater();
	return data;
}

bool CommunitySuggest::ensure(const QString &kind)
{
	const qint64 now = QDateTime::currentMSecsSinceEpoch();
	bool *loaded;
	qint64 *ts;
	QString filename;
	if (kind == QLatin1String("wec")) {
		loaded = &m_wecLoaded; ts = &m_wecTs; filename = QStringLiteral("weclist.txt");
	} else if (kind == QLatin1String("gameslist")) {
		loaded = &m_gameslistLoaded; ts = &m_gameslistTs; filename = QStringLiteral("gameslist.txt");
	} else {
		loaded = &m_dbLoaded; ts = &m_dbTs; filename = QStringLiteral("minidb.txt");
	}

	if (*loaded && (now - *ts) < CACHE_TTL_MS)
		return true;

	const QByteArray data = download(filename);
	if (!data.isEmpty()) {
		if (kind == QLatin1String("wec"))
			parseNameList(data, m_wec);
		else if (kind == QLatin1String("gameslist"))
			parseGameslist(data);
		else
			parseDb(data);
		*loaded = true;
		*ts = now;
		return true;
	}
	// Download fehlgeschlagen ⇒ auf (ggf. abgelaufene) Altdaten zurückfallen.
	return *loaded;
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

QString CommunitySuggest::suggest(const QString &type, const QStringList &idleNames,
                                  const QList<PlayingPlayer> &playing)
{
	static const QRegularExpression stepRe(QStringLiteral("^step([1-4])$"));
	const QRegularExpressionMatch m = stepRe.match(type);
	if (m.hasMatch()) {
		if (!ensure(QStringLiteral("db")))
			return QString();
		return suggestStep(m.captured(1).toInt(), idleNames, playing);
	}
	if (type == QLatin1String("wec")) {
		if (!ensure(QStringLiteral("wec")))
			return QString();
		return suggestWec(idleNames, playing);
	}
	return QString();
}

QString CommunitySuggest::gameTitlePrefix(const QString &command)
{
	if (!ensure(QStringLiteral("gameslist")))
		return QString();
	return m_gameslist.value(command);
}

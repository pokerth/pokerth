/*****************************************************************************
 * Community-„Suggest" für den Widget-Client – dieselbe Logik wie im QML-Client
 * (Config.BotSuggest): schlägt für ein eigenes BBC-Step-/WEC-Invite-Spiel passende
 * Spieler vor. Aus dem Legacy-bbcbot portiert (bbcbotplayerdb: printsuggest/
 * wecsuggest, Scoring, DB/WEC/gameslist-Parsing).
 *
 * Die Botfiles (minidb.txt, weclist.txt, gameslist.txt) werden per HTTP von
 * bbc.pokerth.net gezogen und 15 Minuten gecacht (danach beim nächsten Bedarf
 * frisch). Der von Cloudflare erwartete User-Agent "PokerTH/2.0 (Qt Network)"
 * wird gesetzt (wie upload-/downloadhelper des Clients).
 *
 * Bewusst SYNCHRON (QEventLoop) – identisch zum bbcbot-Download; dank Cache
 * passiert ein echter Download nur selten (erstmalig / nach Ablauf). Ausnahme
 * ist der BBC-Admin-Abgleich (bbcadmins.txt): der hängt an der Sichtbarkeit des
 * Suggest-Buttons statt an einem Klick und läuft deshalb asynchron – siehe
 * requestBbcAdmin().
 *****************************************************************************/
#ifndef COMMUNITYSUGGEST_H
#define COMMUNITYSUGGEST_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QHash>
#include <QList>

#include <list>

QT_BEGIN_NAMESPACE
class QNetworkAccessManager;
QT_END_NAMESPACE

class CommunitySuggest : public QObject
{
	Q_OBJECT
public:
	// Ein gerade spielender Kandidat: Name + Tischname (für "(playing in game …)").
	struct PlayingPlayer {
		QString name;
		QString game;
	};

	// Eine offizielle Community-Turniervorlage (BBC / Monthly Cup / WEC).
	struct CommunityTemplate {
		QString name;
		QString suggestType;    // "step1".."step4" | "wec" | "" (kein Suggest)
		QString titleCommand;   // "mcup"/"mcupfinal" (monatl. Titel) | ""
		int startCash;
		int firstSmallBlind;
		bool raiseOnHands;
		int raiseEveryHands;
		int raiseEveryMinutes;
		int playerActionTimeout;
		QList<int> blinds;      // leer = Blinds verdoppeln
	};

	explicit CommunitySuggest(QObject *parent = 0);

	// Ist der Typ ein gültiges Suggest-Ziel? ("step1".."step4" | "wec")
	static bool isSuggestType(const QString &type);

	// Die Vorlagentabelle. Sie liegt hier und nicht mehr im Create-Dialog, weil
	// sie zwei Aufgaben hat: die Dialogfelder beim Erstellen füllen UND als
	// Fingerprint dienen, um den Typ eines FREMDEN Tisches zu erkennen (siehe
	// suggestTypeForGame). Beides muss aus derselben Quelle kommen.
	static const QList<CommunityTemplate> &templates();

	// Suggest-Typ eines Tisches aus SEINEN EINSTELLUNGEN ableiten. Ein
	// beitretender Spieler kennt den Vorlagen-Typ nicht (der steckt nur im
	// Client des Erstellers), und das Protokoll überträgt ihn nicht. Der
	// Tischname taugt NICHT als Quelle – er ist frei editierbar. Startgeld +
	// erster Small Blind + die vollständige manuelle Blindreihenfolge
	// identifizieren einen BBC-Step dagegen eindeutig.
	// Vorlagen ohne feste Blindliste (Monthly Cup, WEC) werden übersprungen:
	// „Blinds verdoppeln" mit 10000/50 ist keine Signatur, das träfe auch
	// beliebige fremde Tische. Rückgabe: "step1".."step4" oder "" (unbekannt).
	static QString suggestTypeForGame(int startMoney, int firstSmallBlind,
	                                  const std::list<int> &manualBlinds);

	// ── BBC-Admin-Abgleich (bbcadmins.txt, Format wie weclist.txt) ───────────
	// Entscheidet, ob der eigene Spieler an einem FREMDEN BBC-Step-Tisch
	// vorschlagen darf. Bewusst ASYNCHRON (anders als suggest()): der Abgleich
	// hängt an der Sichtbarkeit des Buttons und liefe damit in den Signalpfaden
	// des Warteraums – ein verschachtelter QEventLoop hätte dort die UI
	// eingefroren. Erst aufrufen, wenn suggestTypeForGame bereits „BBC-Step"
	// sagt; dann kostet das Feature an allen anderen Tischen keinen Request.
	// Antwort kommt über bbcAdminResolved(); bis dahin liefert isBbcAdmin() false.
	void requestBbcAdmin(const QString &nick);
	bool isBbcAdmin() const;

signals:
	// Ergebnis des Admin-Abgleichs liegt vor (auch bei Fehlschlag).
	void bbcAdminResolved();

public:
	// Erzeugt die Vorschlagszeile für den gegebenen Typ. idleNames = idle
	// Nicht-Gast-Spieler; playing = Nicht-Gast-Spieler an ANDEREN Tischen (der
	// eigene Tisch wird vom Aufrufer bereits ausgefiltert). Bei Fehlschlag der
	// (nötigen) Datei wird auf ggf. vorhandene Altdaten zurückgegriffen; ist gar
	// nichts verfügbar, liefert die Funktion einen leeren String.
	QString suggest(const QString &type,
	                const QStringList &idleNames,
	                const QList<PlayingPlayer> &playing);

	// Aktueller "Game Title Prefix" eines Community-Spiels aus gameslist.txt
	// (z. B. command "mcup"/"mcupfinal" → "July Cup"/"July Cup Final"). "" wenn
	// nicht ermittelbar.
	QString gameTitlePrefix(const QString &command);

private:
	struct DbEntry {
		QString name;   // Original-Schreibweise aus der DB
		int ts2 = 0;
		int ts3 = 0;
		int ts4 = 0;
		int rating = 0;
		int games = 0;
	};

	// Datei sicherstellen (Download+Parse, wenn Cache älter als 15 min oder leer).
	// kind: "db" | "wec" | "gameslist". Liefert true, wenn brauchbare Daten da sind.
	bool ensure(const QString &kind);
	QByteArray download(const QString &filename) const;

	void parseDb(const QByteArray &data);
	void parseNameList(const QByteArray &data, QHash<QString, QString> &target);
	void parseGameslist(const QByteArray &data);

	// Ergebnis aus dem (ggf. gerade geladenen) Admin-Cache setzen + melden.
	void applyBbcAdmin(const QString &nick);

	QString suggestStep(int step, const QStringList &idleNames,
	                    const QList<PlayingPlayer> &playing) const;
	QString suggestWec(const QStringList &idleNames,
	                   const QList<PlayingPlayer> &playing) const;

	// Cache
	QHash<QString, DbEntry> m_db;          // key: lowercase name
	QHash<QString, QString> m_wec;         // key: lowercase name → Original
	QHash<QString, QString> m_gameslist;   // key: command → title prefix
	QHash<QString, QString> m_bbcAdmins;   // key: lowercase name → Original
	qint64 m_dbTs = 0;
	qint64 m_wecTs = 0;
	qint64 m_gameslistTs = 0;
	qint64 m_bbcAdminsTs = 0;
	bool m_dbLoaded = false;
	bool m_wecLoaded = false;
	bool m_gameslistLoaded = false;
	bool m_bbcAdminsLoaded = false;

	// Asynchroner Pfad (nur bbcadmins.txt), siehe requestBbcAdmin().
	QNetworkAccessManager *m_bbcAdminNam = 0;
	qint64 m_bbcAdminLastTry = 0;   // drosselt auch fehlgeschlagene Versuche
	bool m_bbcAdminInFlight = false;
	bool m_bbcAdmin = false;
};

#endif // COMMUNITYSUGGEST_H

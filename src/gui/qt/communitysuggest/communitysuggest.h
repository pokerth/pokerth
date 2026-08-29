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
 * ist der Community-Admin-Abgleich (bbcadmins.txt / wecadmins.txt): der hängt an
 * der Sichtbarkeit des Suggest-Buttons statt an einem Klick und läuft deshalb
 * asynchron – siehe requestCommunityAdmin().
 *****************************************************************************/
#ifndef COMMUNITYSUGGEST_H
#define COMMUNITYSUGGEST_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QHash>
#include <QList>

#include "gamedata.h"

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
	// Die WEC-Vorlagen verdoppeln die Blinds und haben keine solche Liste; für
	// sie müssen zusätzlich Raise-Intervall (Modus + Wert) und Aktions-Timeout
	// passen – Startgeld + Small Blind allein wären keine Signatur. Bekannte
	// Unschärfe: „Monthly Cup Final" hat exakt dieselben Einstellungen wie
	// „WEC"; die beiden sind über die Einstellungen nicht trennbar (der
	// Vorschlag landet nur lokal beim Klickenden, deshalb in Kauf genommen).
	// Rückgabe: "step1".."step4", "wec" oder "" (unbekannt).
	static QString suggestTypeForGame(const GameData &data);

	// ── Community-Admin-Abgleich ────────────────────────────────────────────
	// Je Community eine Adminliste im Format von weclist.txt: bbcadmins.txt für
	// die BBC-Steps, wecadmins.txt für die WEC-Tische. Sie entscheidet, ob der
	// eigene Spieler an einem FREMDEN Tisch dieser Community vorschlagen darf.
	// Bewusst ASYNCHRON (anders als suggest()): der Abgleich hängt an der
	// Sichtbarkeit des Buttons und liefe damit in den Signalpfaden des
	// Warteraums – ein verschachtelter QEventLoop hätte dort die UI eingefroren.
	// Erst aufrufen, wenn suggestTypeForGame bereits einen Typ liefert; dann
	// kostet das Feature an allen anderen Tischen keinen Request. Antwort kommt
	// über communityAdminResolved(); bis dahin liefert isCommunityAdmin() false.
	void requestCommunityAdmin(const QString &type, const QString &nick);
	bool isCommunityAdmin(const QString &type) const;

signals:
	// Ergebnis des Admin-Abgleichs liegt vor (auch bei Fehlschlag).
	void communityAdminResolved();

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

	// Zustand einer Adminliste (eine je Community-Datei).
	struct AdminList {
		QHash<QString, QString> names;   // key: lowercase name → Original
		qint64 ts = 0;
		qint64 lastTry = 0;              // drosselt auch fehlgeschlagene Versuche
		bool loaded = false;
		bool inFlight = false;
		bool isAdmin = false;
	};

	// Datei sicherstellen (Download+Parse, wenn Cache älter als 15 min oder leer).
	// kind: "db" | "wec" | "gameslist". Liefert true, wenn brauchbare Daten da sind.
	bool ensure(const QString &kind);
	QByteArray download(const QString &filename) const;

	void parseDb(const QByteArray &data);
	void parseNameList(const QByteArray &data, QHash<QString, QString> &target);
	void parseGameslist(const QByteArray &data);

	// Suggest-Typ → Dateiname der zuständigen Adminliste ("" = keine).
	static QString adminFile(const QString &type);
	// Ergebnis aus dem (ggf. gerade geladenen) Admin-Cache setzen + melden.
	void applyCommunityAdmin(const QString &file, const QString &nick);

	QString suggestStep(int step, const QStringList &idleNames,
	                    const QList<PlayingPlayer> &playing) const;
	QString suggestWec(const QStringList &idleNames,
	                   const QList<PlayingPlayer> &playing) const;

	// Cache
	QHash<QString, DbEntry> m_db;          // key: lowercase name
	QHash<QString, QString> m_wec;         // key: lowercase name → Original
	QHash<QString, QString> m_gameslist;   // key: command → title prefix
	qint64 m_dbTs = 0;
	qint64 m_wecTs = 0;
	qint64 m_gameslistTs = 0;
	bool m_dbLoaded = false;
	bool m_wecLoaded = false;
	bool m_gameslistLoaded = false;

	// Asynchroner Pfad (nur die Adminlisten), siehe requestCommunityAdmin();
	// key: Dateiname ("bbcadmins.txt" / "wecadmins.txt").
	QHash<QString, AdminList> m_admins;
	QNetworkAccessManager *m_adminNam = 0;
};

#endif // COMMUNITYSUGGEST_H

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
 * ALLE Downloads laufen asynchron. Früher zog suggest()/gameTitlePrefix() die
 * Datei in einem verschachtelten QEventLoop – das fror bei jedem Ablauf des
 * Caches die gesamte GUI ein (bis zum Transfer-Timeout von 15 s), und weil in
 * dieser Schleife weiter Events verarbeitet wurden, konnte ein zweiter Klick
 * eine weitere Schleife darüberstapeln. Ergebnisse kommen deshalb per Callback;
 * der Aufrufer übergibt sich selbst als Kontextobjekt und wird nicht mehr
 * gerufen, wenn er inzwischen zerstört wurde (wie connect(..., context, ...)).
 *****************************************************************************/
#ifndef COMMUNITYSUGGEST_H
#define COMMUNITYSUGGEST_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QHash>
#include <QList>
#include <QPointer>

#include <functional>

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
	// Der Abgleich hängt an der Sichtbarkeit des Buttons, läuft also in den
	// Signalpfaden des Warteraums: Fehlschläge werden zusätzlich gedrosselt
	// (lastTry), sonst liefe bei unerreichbarer Datei ein Download pro
	// Join/Leave.
	// Erst aufrufen, wenn suggestTypeForGame bereits einen Typ liefert; dann
	// kostet das Feature an allen anderen Tischen keinen Request. Antwort kommt
	// über communityAdminResolved(); bis dahin liefert isCommunityAdmin() false.
	void requestCommunityAdmin(const QString &type, const QString &nick);
	bool isCommunityAdmin(const QString &type) const;

	// Ergebnis eines asynchronen Aufrufs (leerer String = nicht ermittelbar).
	typedef std::function<void(const QString &)> ResultCallback;

signals:
	// Ergebnis des Admin-Abgleichs liegt vor (auch bei Fehlschlag).
	void communityAdminResolved();

public:
	// Erzeugt die Vorschlagszeile für den gegebenen Typ. idleNames = idle
	// Nicht-Gast-Spieler; playing = Nicht-Gast-Spieler an ANDEREN Tischen (der
	// eigene Tisch wird vom Aufrufer bereits ausgefiltert). Bei Fehlschlag der
	// (nötigen) Datei wird auf ggf. vorhandene Altdaten zurückgegriffen; ist gar
	// nichts verfügbar, kommt ein leerer String.
	// onReady läuft sofort (frischer Cache) oder nach dem Download; context ist
	// der Aufrufer, dessen Zerstörung den Callback verfallen lässt.
	void suggest(const QString &type,
	             const QStringList &idleNames,
	             const QList<PlayingPlayer> &playing,
	             QObject *context,
	             const ResultCallback &onReady);

	// Aktueller "Game Title Prefix" eines Community-Spiels aus gameslist.txt
	// (z. B. command "mcup"/"mcupfinal" → "July Cup"/"July Cup Final"). ""
	// wenn nicht ermittelbar.
	void gameTitlePrefix(const QString &command, QObject *context,
	                     const ResultCallback &onReady);

	// gameslist.txt vorab in den Cache holen (beim Öffnen des Erstellen-Dialogs
	// aufrufen, die Datei ist ~1 kB). Ohne das käme der monatliche Titel erst
	// nach dem Download an – wer sofort bestätigt, verschickte den
	// Vorlagen-Fallbacknamen ("Monthly Cup Final" statt "August Cup Final").
	void prefetchGameTitles();

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

	// Zustand einer der drei Botfiles (minidb/weclist/gameslist).
	struct FileCache {
		qint64 ts = 0;                              // letztes erfolgreiches Laden
		bool loaded = false;
		bool inFlight = false;
		QList<std::function<void(bool)> > queue;    // wartende Aufrufer
	};

	// Datei sicherstellen (Download+Parse, wenn Cache älter als 15 min oder
	// leer). kind: "db" | "wec" | "gameslist". done(true) = brauchbare Daten da,
	// sofort aus dem Cache oder nach dem Download; mehrere Aufrufer derselben
	// Datei teilen sich EINEN Download (queue).
	void ensure(const QString &kind, const std::function<void(bool)> &done);
	static QString fileNameForKind(const QString &kind);

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
	// Ladezustand je Datei; key: kind ("db" | "wec" | "gameslist").
	QHash<QString, FileCache> m_files;

	// Adminlisten, siehe requestCommunityAdmin();
	// key: Dateiname ("bbcadmins.txt" / "wecadmins.txt").
	QHash<QString, AdminList> m_admins;
	QNetworkAccessManager *m_nam = 0;      // für alle Botfiles
};

#endif // COMMUNITYSUGGEST_H

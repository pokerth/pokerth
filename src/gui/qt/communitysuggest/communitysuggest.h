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
 * passiert ein echter Download nur selten (erstmalig / nach Ablauf).
 *****************************************************************************/
#ifndef COMMUNITYSUGGEST_H
#define COMMUNITYSUGGEST_H

#include <QString>
#include <QStringList>
#include <QHash>
#include <QList>

class CommunitySuggest
{
public:
	// Ein gerade spielender Kandidat: Name + Tischname (für "(playing in game …)").
	struct PlayingPlayer {
		QString name;
		QString game;
	};

	CommunitySuggest();

	// Ist der Typ ein gültiges Suggest-Ziel? ("step1".."step4" | "wec")
	static bool isSuggestType(const QString &type);

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
	void parseWec(const QByteArray &data);
	void parseGameslist(const QByteArray &data);

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
};

#endif // COMMUNITYSUGGEST_H

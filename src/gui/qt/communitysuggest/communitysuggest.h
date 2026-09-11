/*****************************************************************************
 * Community "suggest" for the widget client – the same logic as in the QML client
 * (Config.BotSuggest): suggests matching players for your own BBC step/WEC invite
 * game. Ported from the legacy bbcbot (bbcbotplayerdb: printsuggest/
 * wecsuggest, scoring, DB/WEC/gameslist parsing).
 *
 * The botfiles (minidb.txt, weclist.txt, gameslist.txt) are pulled via HTTP from
 * bbc.pokerth.net and cached for 15 minutes (afterwards fetched freshly on the next
 * demand). The user agent "PokerTH/2.0 (Qt Network)" expected by Cloudflare
 * is set (as in the upload/download helper of the client).
 *
 * ALL downloads run asynchronously. Previously suggest()/gameTitlePrefix() pulled the
 * file in a nested QEventLoop – that froze the whole GUI whenever the
 * cache expired (until the transfer timeout of 15 s), and because events kept
 * being processed in that loop, a second click could stack
 * another loop on top. Results therefore come via a callback;
 * the caller passes itself as the context object and is no longer
 * called if it has been destroyed meanwhile (like connect(..., context, ...)).
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
	// A candidate currently playing: name + table name (for "(playing in game …)").
	struct PlayingPlayer {
		QString name;
		QString game;
	};

	// An official community tournament template (BBC / monthly cup / WEC).
	struct CommunityTemplate {
		QString name;
		QString suggestType;    // "step1".."step4" | "wec" | "" (no suggest)
		QString titleCommand;   // "mcup"/"mcupfinal" (monthly title) | ""
		int startCash;
		int firstSmallBlind;
		bool raiseOnHands;
		int raiseEveryHands;
		int raiseEveryMinutes;
		int playerActionTimeout;
		QList<int> blinds;      // leer = Blinds verdoppeln
	};

	explicit CommunitySuggest(QObject *parent = 0);

	// Is the type a valid suggest target? ("step1".."step4" | "wec")
	static bool isSuggestType(const QString &type);

	// The template table. It lives here and no longer in the create dialog, because
	// it has two jobs: filling the dialog fields when creating AND serving as a
	// fingerprint to recognise the type of a FOREIGN table (see
	// suggestTypeForGame). Both have to come from the same source.
	static const QList<CommunityTemplate> &templates();

	// Derive the suggest type of a table from ITS SETTINGS. A
	// joining player does not know the template type (that only sits in the
	// client of the creator), and the protocol does not transmit it. The
	// table name is NOT suitable as a source – it is freely editable. The starting money +
	// the first small blind + the complete manual blind order, by contrast,
	// identify a BBC step unambiguously.
	// The WEC templates double the blinds and have no such list; for
	// them the raise interval (mode + value) and the action timeout have to match
	// in addition – the starting money + the small blind alone would be no signature. A known
	// fuzziness: "Monthly Cup Final" has exactly the same settings as
	// "WEC"; the two cannot be separated by their settings (the
	// suggestion only lands locally at whoever clicks, so this is accepted).
	// Return value: "step1".."step4", "wec" or "" (unknown).
	static QString suggestTypeForGame(const GameData &data);

	// ── Community admin match ───────────────────────────────────────────────
	// One admin list per community in the format of weclist.txt: bbcadmins.txt for
	// the BBC steps, wecadmins.txt for the WEC tables. It decides whether your
	// own player may suggest at a FOREIGN table of this community.
	// The match hangs off the visibility of the button, so it runs in the
	// signal paths of the waiting room: failures are throttled in addition
	// (lastTry), otherwise an unreachable file would cause one download per
	// join/leave.
	// Only call it once suggestTypeForGame already delivers a type; then
	// the feature costs no request at all other tables. The reply comes
	// via communityAdminResolved(); until then isCommunityAdmin() returns false.
	void requestCommunityAdmin(const QString &type, const QString &nick);
	bool isCommunityAdmin(const QString &type) const;

	// Result of an asynchronous call (an empty string = not determinable).
	typedef std::function<void(const QString &)> ResultCallback;

signals:
	// The result of the admin match is available (on failure as well).
	void communityAdminResolved();

public:
	// Creates the suggestion line for the given type. idleNames = idle
	// non-guest players; playing = non-guest players at OTHER tables (your
	// own table is already filtered out by the caller). If the (needed)
	// file fails, any old data available is used; if nothing
	// at all is available, an empty string comes back.
	// onReady runs immediately (fresh cache) or after the download; context is
	// the caller, whose destruction lets the callback lapse.
	void suggest(const QString &type,
				 const QStringList &idleNames,
				 const QList<PlayingPlayer> &playing,
				 QObject *context,
				 const ResultCallback &onReady);

	// The current "game title prefix" of a community game from gameslist.txt
	// (e.g. command "mcup"/"mcupfinal" → "July Cup"/"July Cup Final"). ""
	// if it cannot be determined.
	void gameTitlePrefix(const QString &command, QObject *context,
						 const ResultCallback &onReady);

	// Fetch gameslist.txt into the cache in advance (call it when opening the create
	// dialog, the file is ~1 kB). Without that the monthly title would only arrive
	// after the download – whoever confirms immediately would send the
	// template fallback name ("Monthly Cup Final" instead of "August Cup Final").
	void prefetchGameTitles();

private:
	struct DbEntry {
		QString name;   // the original spelling from the DB
		int ts2 = 0;
		int ts3 = 0;
		int ts4 = 0;
		int rating = 0;
		int games = 0;
	};

	// State of an admin list (one per community file).
	struct AdminList {
		QHash<QString, QString> names;   // key: lowercase name → Original
		qint64 ts = 0;
		qint64 lastTry = 0;              // throttles failed attempts as well
		bool loaded = false;
		bool inFlight = false;
		bool isAdmin = false;
	};

	// State of one of the three botfiles (minidb/weclist/gameslist).
	struct FileCache {
		qint64 ts = 0;                              // letztes erfolgreiches Laden
		bool loaded = false;
		bool inFlight = false;
		QList<std::function<void(bool)> > queue;    // wartende Aufrufer
	};

	// Ensure the file (download+parse if the cache is older than 15 min or
	// empty). kind: "db" | "wec" | "gameslist". done(true) = usable data is there,
	// either immediately from the cache or after the download; several callers of the same
	// file share ONE download (queue).
	void ensure(const QString &kind, const std::function<void(bool)> &done);
	static QString fileNameForKind(const QString &kind);

	void parseDb(const QByteArray &data);
	void parseNameList(const QByteArray &data, QHash<QString, QString> &target);
	void parseGameslist(const QByteArray &data);

	// Suggest type → file name of the responsible admin list ("" = none).
	static QString adminFile(const QString &type);
	// Set and report the result from the admin cache (possibly just loaded).
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
	QNetworkAccessManager *m_nam = 0;      // for all botfiles
};

#endif // COMMUNITYSUGGEST_H

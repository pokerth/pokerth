/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2025 Felix Hammer, Florian Thauer, Lothar May          *
 *****************************************************************************/

#include "loghandler.h"

#include <configfile.h>
#include <log.h>
#include <game_defs.h>
#include <net/uploaderthread.h>
#include <core/thread.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileInfoList>
#include <QTextStream>
#include <QUrl>
#include <QtSql>
#include <QVariantMap>
#include <QFileDialog>
#include <core/appimage_utils.h>

#include <boost/lexical_cast.hpp>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <string>

using namespace std;

// ─── Minimal sqlite3_get_table-style shim on top of QSqlDatabase ─────────────
// Identical to the compatibility layer in the Qt-Widgets guilog.cpp, but with
// internal linkage (static) so the ported parsing code below can stay unchanged
// without exporting symbols that could clash with libsqlite3.
namespace {

#define LH_SQLITE_OK 0
#define LH_SQLITE_ERROR 1

struct sqlite3 {
    QString connName;
};

int sqlite3_open(const char *filename, sqlite3 **ppDb)
{
    if (!ppDb) return LH_SQLITE_ERROR;
    sqlite3 *p = new sqlite3();
    p->connName = QString("loghandler_conn_%1_%2")
        .arg((qulonglong)QDateTime::currentMSecsSinceEpoch())
        .arg((qulonglong)(quintptr)p);
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", p->connName);
    // Wait on lock contention instead of failing immediately with SQLITE_BUSY:
    // this read connection may open the .pdb that the engine is still writing
    // (live log). Mirrors the writer connections in engine/log.cpp. Must be set
    // before open().
    db.setConnectOptions("QSQLITE_BUSY_TIMEOUT=5000");
    db.setDatabaseName(QString::fromUtf8(filename));
    if (!db.open()) {
        QSqlDatabase::removeDatabase(p->connName);
        delete p;
        *ppDb = nullptr;
        return LH_SQLITE_ERROR;
    }
    *ppDb = p;
    return LH_SQLITE_OK;
}

int sqlite3_get_table(sqlite3 *pDb, const char *zSql, char ***pazResult, int *pnRow, int *pnColumn, char **pErrMsg)
{
    if (!pDb || !pazResult || !pnRow || !pnColumn) return LH_SQLITE_ERROR;
    QSqlDatabase db = QSqlDatabase::database(pDb->connName, false);
    QSqlQuery q(db);
    if (!q.exec(QString::fromUtf8(zSql))) {
        if (pErrMsg) {
            std::string err = q.lastError().text().toStdString();
            *pErrMsg = strdup(err.c_str());
        }
        *pazResult = nullptr;
        *pnRow = 0;
        *pnColumn = 0;
        return LH_SQLITE_ERROR;
    }

    QSqlRecord rec = q.record();
    int nCol = rec.count();
    QVector<QString> columnNames;
    for (int i = 0; i < nCol; ++i) columnNames.append(rec.fieldName(i));

    QVector<QVector<QString>> rows;
    while (q.next()) {
        QVector<QString> row;
        for (int i = 0; i < nCol; ++i) row.append(q.value(i).toString());
        rows.append(row);
    }

    int nRow = rows.size();
    int total = (nRow + 1) * nCol;
    char **result = (char **)malloc(sizeof(char *) * (total + 1));
    if (!result) {
        *pazResult = nullptr;
        *pnRow = 0;
        *pnColumn = 0;
        return LH_SQLITE_ERROR;
    }

    int idx = 0;
    for (int c = 0; c < nCol; ++c)
        result[idx++] = strdup(columnNames[c].toStdString().c_str());
    for (int r = 0; r < nRow; ++r) {
        for (int c = 0; c < nCol; ++c) {
            const QString &v = rows[r][c];
            if (v.isNull()) result[idx++] = nullptr;
            else result[idx++] = strdup(v.toStdString().c_str());
        }
    }
    result[total] = nullptr;

    *pazResult = result;
    *pnRow = nRow;
    *pnColumn = nCol;
    return LH_SQLITE_OK;
}

void sqlite3_free_table(char **result)
{
    if (!result) return;
    for (char **p = result; *p != nullptr; ++p)
        free(*p);
    free(result);
}

int sqlite3_close(sqlite3 *pDb)
{
    if (!pDb) return LH_SQLITE_ERROR;
    if (QSqlDatabase::contains(pDb->connName)) {
        {
            QSqlDatabase db = QSqlDatabase::database(pDb->connName);
            if (db.isOpen()) db.close();
        }
        QSqlDatabase::removeDatabase(pDb->connName);
    }
    delete pDb;
    return LH_SQLITE_OK;
}

struct result_struct {
    char **result_Session;
    char **result_Game;
    char **result_Player;
    char **result_Hand;
    char **result_Hand_ID;
    char **result_Action;
};

void cleanUp(result_struct &results, sqlite3 *mySqliteLogDb)
{
    sqlite3_free_table(results.result_Session);
    sqlite3_free_table(results.result_Game);
    sqlite3_free_table(results.result_Hand);
    sqlite3_free_table(results.result_Hand_ID);
    sqlite3_free_table(results.result_Action);
    sqlite3_close(mySqliteLogDb);
}

// Faithful port of guiLog::convertCardIntToString.
string convertCardIntToString(int code, int modus)
{
    string tmp;
    switch (code % 13) {
    case 0:  tmp = "2"; break;
    case 1:  tmp = "3"; break;
    case 2:  tmp = "4"; break;
    case 3:  tmp = "5"; break;
    case 4:  tmp = "6"; break;
    case 5:  tmp = "7"; break;
    case 6:  tmp = "8"; break;
    case 7:  tmp = "9"; break;
    case 8:  tmp = "T"; break;
    case 9:  tmp = "J"; break;
    case 10: tmp = "Q"; break;
    case 11: tmp = "K"; break;
    case 12: tmp = "A"; break;
    default: return "";
    }

    if (modus == 2) {
        switch (code / 13) {
        case 0: tmp += "d"; break;
        case 1: tmp += "h"; break;
        case 2: tmp += "s"; break;
        case 3: tmp += "c"; break;
        default: return "";
        }
    } else {
        switch (code / 13) {
        case 0: tmp += "<font size=+1><b>&diams;</b></font>"; break;
        case 1: tmp += "<font size=+1><b>&hearts;</b></font>"; break;
        case 2: tmp += "<font size=+1><b>&spades;</b></font>"; break;
        case 3: tmp += "<font size=+1><b>&clubs;</b></font>"; break;
        default: return "";
        }
    }
    return tmp;
}

QString urlToLocalPath(const QString &pathOrUrl)
{
    if (pathOrUrl.startsWith("file:"))
        return QUrl(pathOrUrl).toLocalFile();
    return pathOrUrl;
}

} // namespace

// ─── LogHandler ──────────────────────────────────────────────────────────────

LogHandler::LogHandler(ConfigFile *config, QObject *parent)
    : QObject(parent), m_config(config)
{
    connect(this, &LogHandler::uploadCompletedSignal,
            this, &LogHandler::onUploadCompleted, Qt::QueuedConnection);
    connect(this, &LogHandler::uploadErrorSignal,
            this, &LogHandler::onUploadError, Qt::QueuedConnection);
    m_uploader.reset(new UploaderThread(this));
    refresh();
}

LogHandler::~LogHandler()
{
    if (m_uploaderRunning) {
        m_uploader->SignalTermination();
        m_uploader->Join(THREAD_WAIT_INFINITE);
    }
}

QString LogHandler::currentLogFileName() const
{
    if (!m_log) return QString();
    return QFileInfo(QString::fromStdString(m_log->getMySqliteLogFileName())).fileName();
}

void LogHandler::refresh()
{
    QVariantList list;
    if (m_config) {
        QDir logFileDir(QString::fromUtf8(m_config->readConfigString("LogDir").c_str()));
        QStringList filters;
        filters << "*.pdb";
        QFileInfoList dbFiles = logFileDir.entryInfoList(filters, QDir::Files, QDir::Time);
        const QString current = currentLogFileName();
        for (const QFileInfo &fi : dbFiles) {
            QVariantMap m;
            m["name"] = fi.fileName();
            m["path"] = fi.absoluteFilePath();
            m["current"] = (!current.isEmpty() && fi.fileName() == current);
            list.append(m);
        }
    }
    m_logFiles = list;
    emit logFilesChanged();
}

QVariantList LogHandler::gameList(const QString &fileStringPdb)
{
    QVariantList gameList;

    result_struct results;
    results.result_Session = 0;
    results.result_Game = 0;
    results.result_Player = 0;
    results.result_Hand = 0;
    results.result_Hand_ID = 0;
    results.result_Action = 0;

    int nRow_Game = 0;
    int nCol_Game = 0;
    char *errmsg = 0;
    int game_ctr = 0;
    int i = 0;

    sqlite3 *mySqliteLogDb;
    sqlite3_open(fileStringPdb.toStdString().c_str(), &mySqliteLogDb);
    if (mySqliteLogDb != 0) {
        string sql = "SELECT * FROM Game";
        if (sqlite3_get_table(mySqliteLogDb, sql.c_str(), &results.result_Game, &nRow_Game, &nCol_Game, &errmsg) != LH_SQLITE_OK) {
            cout << "Error in statement: " << sql.c_str() << "[" << (errmsg ? errmsg : "") << "]." << endl;
        } else {
            for (game_ctr = 1; game_ctr <= nRow_Game; game_ctr++) {
                for (i = 0; i < nCol_Game; i++) {
                    if (boost::lexical_cast<std::string>(results.result_Game[i]) == "UniqueGameID") {
                        gameList.append(boost::lexical_cast<int>(results.result_Game[i + nCol_Game * game_ctr]));
                    }
                }
            }
        }
    }

    cleanUp(results, mySqliteLogDb);
    return gameList;
}

QString LogHandler::previewHtml(const QString &path, int uniqueGameID)
{
    m_previewBuf.clear();
    m_outStream = nullptr;
    buildLog(path, 3, uniqueGameID);
    return m_previewBuf;
}

bool LogHandler::exportHtml(const QString &path, const QString &destUrl)
{
    const QString dest = urlToLocalPath(destUrl);
    if (dest.isEmpty()) return false;

    QFile file(dest);
    if (!file.open(QIODevice::ReadWrite | QFile::Truncate)) return false;
    QTextStream stream(&file);
    m_outStream = &stream;

    stream << "<html>\n<head>\n"
              "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf8\">"
              "</head>\n<body style=\"font-size:smaller\">\n";
    buildLog(path, 1, 0);

    m_outStream = nullptr;
    file.close();
    return true;
}

bool LogHandler::exportTxt(const QString &path, const QString &destUrl)
{
    const QString dest = urlToLocalPath(destUrl);
    if (dest.isEmpty()) return false;

    QFile file(dest);
    if (!file.open(QIODevice::ReadWrite | QFile::Truncate)) return false;
    QTextStream stream(&file);
    m_outStream = &stream;
    buildLog(path, 2, 0);
    m_outStream = nullptr;
    file.close();
    return true;
}

bool LogHandler::saveAs(const QString &path, const QString &destUrl)
{
    const QString dest = urlToLocalPath(destUrl);
    if (dest.isEmpty()) return false;
    if (QFile::exists(dest))
        QFile::remove(dest);
    return QFile::copy(path, dest);
}

void LogHandler::exportHtmlDialog(const QString &path)
{
    if (path.isEmpty()) return;
    const QString suggested = QDir::homePath() + "/" + baseName(path) + ".html";
    const QString dest = QFileDialog::getSaveFileName(
        nullptr, tr("Export PokerTH log file to HTML"), suggested,
        tr("PokerTH HTML log (*.html)"),
        nullptr, AppImageUtils::fileDialogOptions());
    if (!dest.isEmpty())
        exportHtml(path, dest);
}

void LogHandler::exportTxtDialog(const QString &path)
{
    if (path.isEmpty()) return;
    const QString suggested = QDir::homePath() + "/" + baseName(path) + ".txt";
    const QString dest = QFileDialog::getSaveFileName(
        nullptr, tr("Export PokerTH log file to plain text"), suggested,
        tr("PokerTH plain text log (*.txt)"),
        nullptr, AppImageUtils::fileDialogOptions());
    if (!dest.isEmpty())
        exportTxt(path, dest);
}

void LogHandler::saveAsDialog(const QString &path)
{
    if (path.isEmpty()) return;
    const QString suggested = QDir::homePath() + "/" + baseName(path) + ".pdb";
    const QString dest = QFileDialog::getSaveFileName(
        nullptr, tr("Save PokerTH log file"), suggested,
        tr("PokerTH SQL log (*.pdb)"),
        nullptr, AppImageUtils::fileDialogOptions());
    if (!dest.isEmpty())
        saveAs(path, dest);
}

bool LogHandler::deleteFiles(const QStringList &paths)
{
    const QString current = currentLogFileName();
    bool allOk = true;
    for (const QString &p : paths) {
        if (!current.isEmpty() && QFileInfo(p).fileName() == current)
            continue; // never delete the active log file
        if (!QFile::remove(p))
            allOk = false;
    }
    refresh();
    return allOk;
}

QString LogHandler::debugLogPath() const
{
    if (!m_config)
        return QString();
    QString dir = QString::fromUtf8(m_config->readConfigString("LogDir").c_str());
    if (dir.isEmpty())
        return QString();
    if (!dir.endsWith(QLatin1Char('/')) && !dir.endsWith(QLatin1Char('\\')))
        dir += QLatin1Char('/');
    return dir + QStringLiteral("pokerth-debug.log");
}

QString LogHandler::debugLogTail(int maxBytes) const
{
    const QString path = debugLogPath();
    if (path.isEmpty())
        return QString();
    QFile f(path);
    // Binary read (no newline translation) so seek() offsets stay exact.
    if (!f.open(QIODevice::ReadOnly))
        return QString();
    const qint64 size = f.size();
    bool truncated = false;
    if (maxBytes > 0 && size > maxBytes) {
        // The interesting connect/[REJOIN] lines are the most recent ones; show
        // only the tail and drop the (likely partial) first line after seeking.
        f.seek(size - maxBytes);
        f.readLine();
        truncated = true;
    }
    const QByteArray data = f.readAll();
    f.close();
    QString text = QString::fromUtf8(data);
    if (truncated)
        text.prepend(tr("… (showing the last %1 KB of the log)").arg(maxBytes / 1024)
                     + QStringLiteral("\n\n"));
    return text;
}

void LogHandler::saveDebugLogDialog()
{
    const QString src = debugLogPath();
    if (src.isEmpty() || !QFile::exists(src))
        return;
    const QString suggested = QDir::homePath() + QStringLiteral("/pokerth-debug.log");
    const QString dest = QFileDialog::getSaveFileName(
        nullptr, tr("Save PokerTH debug log"), suggested,
        tr("PokerTH debug log (*.log)"),
        nullptr, AppImageUtils::fileDialogOptions());
    if (dest.isEmpty())
        return;
    if (QFile::exists(dest))
        QFile::remove(dest);
    QFile::copy(src, dest);
}

QString LogHandler::homePath() const
{
    return QDir::homePath();
}

QString LogHandler::baseName(const QString &path) const
{
    return QFileInfo(path).baseName();
}

void LogHandler::setUploadInProgress(bool inProgress)
{
    if (m_uploadInProgress != inProgress) {
        m_uploadInProgress = inProgress;
        emit uploadInProgressChanged();
    }
}

void LogHandler::analyse(const QString &path)
{
    QFileInfo fi(path);
    if (!fi.exists()) {
        emit analyseFailed(tr("Log file not found."));
        return;
    }
    if (!m_uploaderRunning) {
        m_uploader->Run();
        m_uploaderRunning = true;
    }
    setUploadInProgress(true);
    m_uploader->QueueUpload(
        "https://www.pokerth.net/log_file_analysis/upload.php",
        "",
        "",
        path.toStdString(),
        static_cast<size_t>(fi.size()),
        "pdb_file");
}

// Called on the uploader thread → marshal to GUI thread via queued signals.
void LogHandler::UploadCompleted(const std::string &filename, const std::string &returnMessage)
{
    emit uploadCompletedSignal(QString::fromStdString(filename), QString::fromStdString(returnMessage));
}

void LogHandler::UploadError(const std::string &filename, const std::string &errorMessage)
{
    emit uploadErrorSignal(QString::fromStdString(filename), QString::fromStdString(errorMessage));
}

void LogHandler::onUploadCompleted(const QString & /*filename*/, const QString &returnMessageIn)
{
    setUploadInProgress(false);

    QString returnMessage = returnMessageIn.trimmed();
    const QString retStr = returnMessage.mid(0, returnMessage.indexOf(' '));

    if (retStr == LOG_UPLOAD_OK_STR) {
        QString hash = returnMessage.mid(retStr.size()).trimmed();
        hash = hash.mid(0, hash.indexOf(' '));
        const QString url = "https://logfile-analysis.pokerth.net/?ID=" + hash;
        AppImageUtils::openUrlSafe(QUrl(url));   // sanitized env (xdg-open) when bundled
        emit analyseSucceeded(url);
    } else {
        QString serverMsg(tr("Processing of the log file on the web server failed.\n"
                             "Please verify that you are uploading a valid PokerTH log file."));
        if (retStr == LOG_UPLOAD_ERROR_STR) {
            const QString errorId = returnMessage.mid(retStr.size()).trimmed();
            serverMsg += "\n" + tr("Failure reason: ");
            switch (errorId.toInt()) {
            case LOG_UPLOAD_ERROR_NO_FILE:
                serverMsg += tr("No file received."); break;
            case LOG_UPLOAD_ERROR_MAX_NUM_TOTAL:
                serverMsg += tr("File rejected because of too many uploads."); break;
            case LOG_UPLOAD_ERROR_MAX_NUM_IP:
                serverMsg += tr("File rejected because of too many recent uploads. Please try again later."); break;
            case LOG_UPLOAD_ERROR_FILE_SIZE:
                serverMsg += tr("The file is too large."); break;
            case LOG_UPLOAD_ERROR_FILE_EXT:
            case LOG_UPLOAD_ERROR_FILE_HEAD:
                serverMsg += tr("This file is not a valid and current PokerTH log file."); break;
            case LOG_UPLOAD_ERROR_OPEN_DB:
            case LOG_UPLOAD_ERROR_ID:
            case LOG_UPLOAD_ERROR_FILE_MOVE:
            case LOG_UPLOAD_ERROR_INSERT_DB:
            default:
                serverMsg += tr("Internal error. Please try again later. ID: ") + returnMessage; break;
            }
        }
        emit analyseFailed(serverMsg);
    }
}

void LogHandler::onUploadError(const QString & /*filename*/, const QString &errorMessage)
{
    setUploadInProgress(false);
    QString uploadMsg(tr("Upload failed. Please check your internet connection!\n"
                         "Uploading log files may fail if you are using an http proxy."));
    uploadMsg += "\n" + tr("Failure reason: ") + errorMessage;
    emit analyseFailed(uploadMsg);
}

void LogHandler::emitChunk(const std::string &log_string)
{
    if (m_curModus == 3)
        m_previewBuf += QString::fromUtf8(log_string.c_str());
    else if (m_outStream)
        (*m_outStream) << QString::fromUtf8(log_string.c_str());
}

// Faithful port of guiLog::exportLog. Output is routed through emitChunk()
// (preview buffer for modus 3, the open QTextStream for modus 1/2).
int LogHandler::buildLog(const QString &fileStringPdb, int modus, int uniqueGameID_req)
{
    m_curModus = modus;
    bool neu = false;

    result_struct results;
    results.result_Session = 0;
    results.result_Game = 0;
    results.result_Player = 0;
    results.result_Hand = 0;
    results.result_Hand_ID = 0;
    results.result_Action = 0;

    string sql = "";
    string log_string = "";
    string round_string = "";
    string action_string = "";
    bool data_found = false;
    int nRow_Session = 0, nRow_Game = 0, nRow_Player = 0, nRow_Hand = 0, nRow_Hand_ID = 0, nRow_Action = 0;
    int nCol_Session = 0, nCol_Game = 0, nCol_Player = 0, nCol_Hand = 0, nCol_Action = 0;
    char *errmsg = 0;
    int game_ctr = 0, hand_ctr = 0, round_ctr = 0, action_ctr = 0;
    int i = 0, j = 0;
    int gameID = 0;
    int uniqueGameID = 0;
    string cmpString = "", string_tmp = "";
    string player[MAX_NUMBER_OF_PLAYERS];
    for (i = 1; i <= MAX_NUMBER_OF_PLAYERS; i++) {
        player[i - 1] = "";
    }

    sqlite3 *mySqliteLogDb;
    sqlite3_open(fileStringPdb.toStdString().c_str(), &mySqliteLogDb);
    if (mySqliteLogDb != 0) {

        // read session
        sql = "SELECT * FROM Session";
        if (sqlite3_get_table(mySqliteLogDb, sql.c_str(), &results.result_Session, &nRow_Session, &nCol_Session, &errmsg) != LH_SQLITE_OK) {
            cleanUp(results, mySqliteLogDb);
            return 1;
        }
        if (nRow_Session != 1) {
            cleanUp(results, mySqliteLogDb);
            return 1;
        }

        log_string += "Log-File for PokerTH ";

        data_found = false;
        for (i = 0; i < nCol_Session; i++) {
            if (boost::lexical_cast<std::string>(results.result_Session[i]) == "PokerTH_Version") {
                log_string += boost::lexical_cast<std::string>(results.result_Session[i + nCol_Session]);
                data_found = true;
            }
        }
        if (!data_found) { cleanUp(results, mySqliteLogDb); return 1; }

        log_string += " Session started on ";

        data_found = false;
        for (i = 0; i < nCol_Session; i++) {
            if (boost::lexical_cast<std::string>(results.result_Session[i]) == "Date") {
                log_string += boost::lexical_cast<std::string>(results.result_Session[i + nCol_Session]);
                data_found = true;
            }
        }
        if (!data_found) { cleanUp(results, mySqliteLogDb); return 1; }

        log_string += " at ";

        data_found = false;
        for (i = 0; i < nCol_Session; i++) {
            if (boost::lexical_cast<std::string>(results.result_Session[i]) == "Time") {
                log_string += boost::lexical_cast<std::string>(results.result_Session[i + nCol_Session]);
                data_found = true;
            }
        }
        if (!data_found) { cleanUp(results, mySqliteLogDb); return 1; }

        switch (modus) {
        case 1: log_string = "<h3><b>" + log_string + "</b></h3>\n"; break;
        case 2: log_string += ""; break;
        case 3: log_string = "<h4><b>" + log_string + "</b></h4>"; break;
        default: ;
        }
        emitChunk(log_string);
        log_string = "";

        // read game
        if (uniqueGameID_req > 0) {
            sql = "SELECT * FROM Game WHERE UniqueGameID=" + boost::lexical_cast<std::string>(uniqueGameID_req);
        } else {
            sql = "SELECT * FROM Game";
        }
        if (sqlite3_get_table(mySqliteLogDb, sql.c_str(), &results.result_Game, &nRow_Game, &nCol_Game, &errmsg) != LH_SQLITE_OK) {
            cleanUp(results, mySqliteLogDb);
            return 1;
        }

        for (game_ctr = 1; game_ctr <= nRow_Game; game_ctr++) {

            data_found = false;
            for (i = 0; i < nCol_Game; i++) {
                if (boost::lexical_cast<std::string>(results.result_Game[i]) == "UniqueGameID") {
                    uniqueGameID = boost::lexical_cast<int>(results.result_Game[i + nCol_Game * game_ctr]);
                    data_found = true;
                }
            }
            if (!data_found) { cleanUp(results, mySqliteLogDb); return 1; }

            data_found = false;
            for (i = 0; i < nCol_Game; i++) {
                if (boost::lexical_cast<std::string>(results.result_Game[i]) == "GameID") {
                    gameID = boost::lexical_cast<int>(results.result_Game[i + nCol_Game * game_ctr]);
                    data_found = true;
                }
            }
            if (!data_found) { cleanUp(results, mySqliteLogDb); return 1; }

            // read player
            sql  = "SELECT Player,Seat FROM Player WHERE UniqueGameID=";
            sql += boost::lexical_cast<std::string>(uniqueGameID);
            sql += " ORDER BY Seat;";
            if (sqlite3_get_table(mySqliteLogDb, sql.c_str(), &results.result_Player, &nRow_Player, &nCol_Player, &errmsg) != LH_SQLITE_OK) {
                cleanUp(results, mySqliteLogDb);
                return 1;
            }
            for (i = 1; i <= nRow_Player; i++) {
                player[i - 1] = boost::lexical_cast<std::string>(results.result_Player[nCol_Player * i]);
            }

            // read all hand id
            sql = "SELECT HandID FROM Hand WHERE UniqueGameID=";
            sql += boost::lexical_cast<std::string>(uniqueGameID);
            if (sqlite3_get_table(mySqliteLogDb, sql.c_str(), &results.result_Hand_ID, &nRow_Hand_ID, &nCol_Hand, &errmsg) != LH_SQLITE_OK) {
                cleanUp(results, mySqliteLogDb);
                return 1;
            }

            for (hand_ctr = 1; hand_ctr <= nRow_Hand_ID; hand_ctr++) {

                log_string += "Game: ";
                log_string += boost::lexical_cast<std::string>(gameID);
                log_string += " | Hand: ";
                log_string += boost::lexical_cast<std::string>(results.result_Hand_ID[hand_ctr]);

                switch (modus) {
                case 1:
                    log_string = "<table><tr><td width=\"600\" align=\"center\"><hr noshade size=\"3\"><b>" + log_string;
                    if (!neu) log_string += "</b></td><td></td></tr></table>";
                    else log_string += "</b></td></tr></table>";
                    break;
                case 2:
                    log_string = "\n\n----------- " + log_string;
                    log_string += " -----------\n";
                    break;
                case 3:
                    log_string = "----------- <b>" + log_string;
                    log_string += "</b> -----------<br />";
                    break;
                default: ;
                }

                // read current hand
                sql = "SELECT * FROM Hand WHERE UniqueGameID=";
                sql += boost::lexical_cast<std::string>(uniqueGameID);
                sql += " AND HandID=";
                sql += boost::lexical_cast<std::string>(results.result_Hand_ID[hand_ctr]);
                if (sqlite3_get_table(mySqliteLogDb, sql.c_str(), &results.result_Hand, &nRow_Hand, &nCol_Hand, &errmsg) != LH_SQLITE_OK) {
                    cleanUp(results, mySqliteLogDb);
                    return 1;
                }

                log_string += "BLIND LEVEL: $";

                data_found = false;
                for (i = 0; i < nCol_Hand; i++) {
                    if (boost::lexical_cast<std::string>(results.result_Hand[i]) == "Sb_Amount") {
                        log_string += boost::lexical_cast<std::string>(results.result_Hand[i + nCol_Hand]);
                        data_found = true;
                    }
                }
                if (!data_found) { cleanUp(results, mySqliteLogDb); return 1; }

                log_string += " / $";

                data_found = false;
                for (i = 0; i < nCol_Hand; i++) {
                    if (boost::lexical_cast<std::string>(results.result_Hand[i]) == "Bb_Amount") {
                        log_string += boost::lexical_cast<std::string>(results.result_Hand[i + nCol_Hand]);
                        data_found = true;
                    }
                }
                if (!data_found) { cleanUp(results, mySqliteLogDb); return 1; }

                switch (modus) {
                case 1: if (!neu) log_string += "</br>"; else log_string += "<br />"; break;
                case 2: log_string += "\n"; break;
                case 3: log_string += "<br />"; break;
                default: ;
                }

                // read seat cash
                for (i = 1; i <= MAX_NUMBER_OF_PLAYERS; i++) {
                    data_found = false;
                    for (j = 0; j < nCol_Hand; j++) {
                        cmpString = "Seat_";
                        cmpString += boost::lexical_cast<std::string>(i);
                        cmpString += "_Cash";
                        if (boost::lexical_cast<std::string>(results.result_Hand[j]) == cmpString) {
                            if (results.result_Hand[j + nCol_Hand]) {
                                log_string += "Seat ";
                                log_string += boost::lexical_cast<std::string>(i);
                                log_string += ": ";
                                if (modus == 1 || modus == 3) log_string += "<b>";
                                log_string += player[i - 1];
                                if (modus == 1 || modus == 3) log_string += "</b>";
                                log_string += " ($";
                                log_string += boost::lexical_cast<std::string>(results.result_Hand[j + nCol_Hand]);
                                log_string += ")";
                                switch (modus) {
                                case 1: if (!neu) log_string += "</br>"; else log_string += "<br />"; break;
                                case 2: log_string += "\n"; break;
                                case 3: log_string += "<br />"; break;
                                default: ;
                                }
                            }
                            data_found = true;
                        }
                    }
                    if (!data_found) { cleanUp(results, mySqliteLogDb); return 1; }
                }

                if (neu) {

                    if (modus == 1) log_string += "<br />";

                    sql = "SELECT Player,Action,Amount FROM Action WHERE UniqueGameID=";
                    sql += boost::lexical_cast<std::string>(uniqueGameID);
                    sql += " AND HandID=";
                    sql += boost::lexical_cast<std::string>(results.result_Hand_ID[hand_ctr]);
                    sql += " AND BeRo=";
                    sql += boost::lexical_cast<std::string>(GAME_STATE_PREFLOP);
                    sql += " AND (Action='posts small blind' OR Action='posts big blind' OR Action='starts as dealer')";

                    if (sqlite3_get_table(mySqliteLogDb, sql.c_str(), &results.result_Action, &nRow_Action, &nCol_Action, &errmsg) != LH_SQLITE_OK) {
                        cleanUp(results, mySqliteLogDb);
                        return 1;
                    }
                    if (nRow_Action < 1) { cleanUp(results, mySqliteLogDb); return 1; }
                    for (i = 1; i <= nRow_Action; i++) {
                        log_string += player[boost::lexical_cast<int>(results.result_Action[3 * i]) - 1];
                        log_string += " ";
                        log_string += boost::lexical_cast<std::string>(results.result_Action[3 * i + 1]);
                        if (results.result_Action[3 * i + 2]) {
                            log_string += " $";
                            log_string += boost::lexical_cast<std::string>(results.result_Action[3 * i + 2]);
                        }
                        log_string += ".";
                        switch (modus) {
                        case 1: log_string += "<br />"; break;
                        case 2: log_string += "\n"; break;
                        case 3: log_string += "<br />"; break;
                        default: ;
                        }
                    }

                } else {

                    log_string += "BLINDS: ";

                    sql = "SELECT Player,Amount FROM Action WHERE UniqueGameID=";
                    sql += boost::lexical_cast<std::string>(uniqueGameID);
                    sql += " AND HandID=";
                    sql += boost::lexical_cast<std::string>(results.result_Hand_ID[hand_ctr]);
                    sql += " AND BeRo=0 AND Action='posts small blind'";

                    if (sqlite3_get_table(mySqliteLogDb, sql.c_str(), &results.result_Action, &nRow_Action, &nCol_Action, &errmsg) != LH_SQLITE_OK) {
                        cleanUp(results, mySqliteLogDb);
                        return 1;
                    }
                    if (nRow_Action < 1 || nRow_Action > 1) { cleanUp(results, mySqliteLogDb); return 1; }

                    log_string += player[boost::lexical_cast<int>(results.result_Action[2]) - 1];
                    log_string += " ($";
                    log_string += boost::lexical_cast<std::string>(results.result_Action[3]);
                    log_string += "), ";

                    sql = "SELECT Player,Amount FROM Action WHERE UniqueGameID=";
                    sql += boost::lexical_cast<std::string>(uniqueGameID);
                    sql += " AND HandID=";
                    sql += boost::lexical_cast<std::string>(results.result_Hand_ID[hand_ctr]);
                    sql += " AND BeRo=0 AND Action='posts big blind'";

                    if (sqlite3_get_table(mySqliteLogDb, sql.c_str(), &results.result_Action, &nRow_Action, &nCol_Action, &errmsg) != LH_SQLITE_OK) {
                        cleanUp(results, mySqliteLogDb);
                        return 1;
                    }
                    if (nRow_Action < 1 || nRow_Action > 1) { cleanUp(results, mySqliteLogDb); return 1; }

                    log_string += player[boost::lexical_cast<int>(results.result_Action[2]) - 1];
                    log_string += " ($";
                    log_string += boost::lexical_cast<std::string>(results.result_Action[3]);
                    log_string += ")";

                    sql = "SELECT Player,Amount FROM Action WHERE UniqueGameID=";
                    sql += boost::lexical_cast<std::string>(uniqueGameID);
                    sql += " AND HandID=";
                    sql += boost::lexical_cast<std::string>(results.result_Hand_ID[hand_ctr]);
                    sql += " AND BeRo=0 AND Action='starts as dealer'";

                    if (sqlite3_get_table(mySqliteLogDb, sql.c_str(), &results.result_Action, &nRow_Action, &nCol_Action, &errmsg) != LH_SQLITE_OK) {
                        cleanUp(results, mySqliteLogDb);
                        return 1;
                    }
                    if (nRow_Action > 1) { cleanUp(results, mySqliteLogDb); return 1; }

                    if (nRow_Action == 1) {
                        switch (modus) {
                        case 1: if (!neu) log_string += "</br>"; else log_string += "<br />"; break;
                        case 2: log_string += "\n"; break;
                        case 3: log_string += "<br />"; break;
                        default: ;
                        }
                        log_string += player[boost::lexical_cast<int>(results.result_Action[2]) - 1];
                        log_string += " starts as dealer.";
                    }

                }

                if (!neu && modus == 1) log_string += "</br>";

                emitChunk(log_string);
                log_string = "";

                for (round_ctr = GAME_STATE_PREFLOP; round_ctr <= GAME_STATE_POST_RIVER; round_ctr++) {

                    round_string = "";
                    if (round_ctr <= GAME_STATE_RIVER) {
                        switch (round_ctr) {
                        case GAME_STATE_PREFLOP: round_string += "PREFLOP"; break;
                        case GAME_STATE_FLOP:    round_string += "FLOP"; break;
                        case GAME_STATE_TURN:    round_string += "TURN"; break;
                        case GAME_STATE_RIVER:   round_string += "RIVER"; break;
                        default: ;
                        }
                        switch (modus) {
                        case 1:
                            if (!neu) round_string = "</br><b>" + round_string + "</b>";
                            else round_string = "<br /><b>" + round_string + "</b>";
                            if (round_ctr >= GAME_STATE_FLOP) {
                                if (!neu) round_string = "</br>\n" + round_string;
                                else round_string = "<br />\n" + round_string;
                            }
                            break;
                        case 2:
                            round_string = "\n\n" + round_string;
                            break;
                        case 3:
                            round_string = "<b>" + round_string + "</b>";
                            if (round_ctr >= GAME_STATE_FLOP) {
                                round_string = "<br /><br />" + round_string;
                            }
                            break;
                        default: ;
                        }
                        if (round_ctr >= GAME_STATE_FLOP) {
                            round_string += " [board cards ";
                            for (i = 1; i <= round_ctr + 2; i++) {
                                data_found = false;
                                for (j = 0; j < nCol_Hand; j++) {
                                    if (boost::lexical_cast<std::string>(results.result_Hand[j]) == "BoardCard_" + boost::lexical_cast<std::string>(i)) {
                                        if (results.result_Hand[j + nCol_Hand]) {
                                            if (modus == 1 || modus == 3) round_string += "<b>";
                                            string_tmp = convertCardIntToString(boost::lexical_cast<int>(results.result_Hand[j + nCol_Hand]), modus);
                                            if (string_tmp == "") {
                                                cleanUp(results, mySqliteLogDb);
                                                return 1;
                                            }
                                            round_string += boost::lexical_cast<std::string>(string_tmp.at(0));
                                            if (modus == 1 || modus == 3) round_string += "</b>";
                                            round_string += boost::lexical_cast<std::string>(string_tmp.erase(0, 1));
                                            if (round_ctr + 2 - i > 0) round_string += ",";

                                            data_found = true;
                                        }
                                    }
                                }
                            }
                            round_string += "]";
                        }
                        if (data_found) {
                            log_string += round_string;
                        } else {
                            continue;
                        }
                    }

                    sql = "SELECT Player,Action,Amount FROM Action WHERE UniqueGameID=";
                    sql += boost::lexical_cast<std::string>(uniqueGameID);
                    sql += " AND HandID=";
                    sql += boost::lexical_cast<std::string>(results.result_Hand_ID[hand_ctr]);
                    sql += " AND BeRo=";
                    sql += boost::lexical_cast<std::string>(round_ctr);
                    sql += " AND Action<>'starts as dealer' AND Action<>'posts big blind' AND Action<>'posts small blind'";

                    if (sqlite3_get_table(mySqliteLogDb, sql.c_str(), &results.result_Action, &nRow_Action, &nCol_Action, &errmsg) != LH_SQLITE_OK) {
                        cleanUp(results, mySqliteLogDb);
                        return 1;
                    }

                    for (action_ctr = 1; action_ctr <= nRow_Action; action_ctr++) {
                        switch (modus) {
                        case 1:
                            if (!neu) {
                                if (action_ctr > 1 && (boost::lexical_cast<std::string>(results.result_Action[3 * (action_ctr - 1) + 1]) == "wins" || boost::lexical_cast<std::string>(results.result_Action[3 * (action_ctr - 1) + 1]) == "sits out" || boost::lexical_cast<std::string>(results.result_Action[3 * (action_ctr - 1) + 1]) == "wins (side pot)"))
                                    log_string += "\n";
                                else
                                    log_string += "</br>\n";
                            } else {
                                log_string += "<br />\n";
                            }
                            break;
                        case 2:
                            log_string += "\n";
                            break;
                        case 3:
                            log_string += "<br />";
                            break;
                        default: ;
                        }
                        if (!neu && boost::lexical_cast<std::string>(results.result_Action[3 * action_ctr + 1]) == "wins (side pot)") {
                            action_string += player[boost::lexical_cast<int>(results.result_Action[3 * action_ctr]) - 1];
                            action_string += " wins $";
                            action_string += boost::lexical_cast<std::string>(results.result_Action[3 * action_ctr + 2]);
                            action_string += " (side pot)";
                        } else {
                            action_string += player[boost::lexical_cast<int>(results.result_Action[3 * action_ctr]) - 1];
                            action_string += " ";
                            action_string += boost::lexical_cast<std::string>(results.result_Action[3 * action_ctr + 1]);
                            if (results.result_Action[3 * action_ctr + 2]) {
                                action_string += " $";
                                action_string += boost::lexical_cast<std::string>(results.result_Action[3 * action_ctr + 2]);
                            }
                        }

                        if (boost::lexical_cast<std::string>(results.result_Action[3 * action_ctr + 1]) == "wins game") {
                            switch (modus) {
                            case 1:
                                if (!neu) action_string = "</br></br><i><b>" + action_string + " " + boost::lexical_cast<std::string>(gameID) + "!</i></b></br>";
                                else action_string = "</br><i><b>" + action_string + " " + boost::lexical_cast<std::string>(gameID) + "!</b></i>";
                                break;
                            case 2:
                                action_string += action_string + " " + boost::lexical_cast<std::string>(gameID) + "!";
                                break;
                            case 3:
                                action_string = "<i><b>" + action_string + " " + boost::lexical_cast<std::string>(gameID) + "!</b></i>";
                                break;
                            default: ;
                            }
                        }

                        if (boost::lexical_cast<std::string>(results.result_Action[3 * action_ctr + 1]) == "wins" || boost::lexical_cast<std::string>(results.result_Action[3 * action_ctr + 1]) == "wins (side pot)") {
                            switch (modus) {
                            case 1:
                                if (!neu) action_string = "</br><i>" + action_string + "</i>";
                                else action_string = "<i>" + action_string + "</i>";
                                break;
                            case 3:
                                action_string = "<i>" + action_string + "</i>";
                                break;
                            default: ;
                            }
                        }

                        if (boost::lexical_cast<std::string>(results.result_Action[3 * action_ctr + 1]) == "has left the game" || boost::lexical_cast<std::string>(results.result_Action[3 * action_ctr + 1]) == "was kicked from the game" || boost::lexical_cast<std::string>(results.result_Action[3 * action_ctr + 1]) == "is game admin now" || boost::lexical_cast<std::string>(results.result_Action[3 * action_ctr + 1]) == "has joined the game") {
                            switch (modus) {
                            case 1:
                                if (!neu) action_string = "<i>" + action_string + "!</i>";
                                else action_string = "<i>" + action_string + "</i>";
                                break;
                            case 3:
                                action_string = "<i>" + action_string + "</i>";
                                break;
                            default: ;
                            }
                        }

                        if (boost::lexical_cast<std::string>(results.result_Action[3 * action_ctr + 1]) == "sits out") {
                            switch (modus) {
                            case 1:
                                if (!neu) action_string = "</br><i><span style=\"font-size:smaller;\">" + action_string + "</span></i>";
                                else action_string = "<i><span style=\"font-size:smaller;\">" + action_string + "</span></i>";
                                break;
                            case 3:
                                action_string = "<i><span style=\"font-size:smaller;\">" + action_string + "</span></i>";
                                break;
                            default: ;
                            }
                        }

                        log_string += action_string;
                        action_string = "";

                        if (boost::lexical_cast<std::string>(results.result_Action[3 * action_ctr + 1]) == "shows" || boost::lexical_cast<std::string>(results.result_Action[3 * action_ctr + 1]) == "has") {
                            if (!neu && round_ctr == GAME_STATE_POST_RIVER) log_string += " [ ";
                            else log_string += " [";
                            if (modus == 1 || modus == 3) log_string += "<b>";

                            data_found = false;
                            for (i = 0; i < nCol_Hand; i++) {
                                cmpString = "Seat_";
                                cmpString += boost::lexical_cast<std::string>(results.result_Action[3 * action_ctr]);
                                cmpString += "_Card_1";
                                if (boost::lexical_cast<std::string>(results.result_Hand[i]) == cmpString) {
                                    string_tmp = convertCardIntToString(boost::lexical_cast<int>(results.result_Hand[i + nCol_Hand]), modus);
                                    if (string_tmp == "") {
                                        cleanUp(results, mySqliteLogDb);
                                        return 1;
                                    }
                                    log_string += boost::lexical_cast<std::string>(string_tmp.at(0));
                                    if (modus == 1 || modus == 3) log_string += "</b>";
                                    log_string += boost::lexical_cast<std::string>(string_tmp.erase(0, 1));
                                    log_string += ",";
                                    if (modus == 1 || modus == 3) log_string += "<b>";
                                    data_found = true;
                                }
                            }
                            if (!data_found) { cleanUp(results, mySqliteLogDb); return 1; }

                            data_found = false;
                            for (i = 0; i < nCol_Hand; i++) {
                                cmpString = "Seat_";
                                cmpString += boost::lexical_cast<std::string>(results.result_Action[3 * action_ctr]);
                                cmpString += "_Card_2";
                                if (boost::lexical_cast<std::string>(results.result_Hand[i]) == cmpString) {
                                    string_tmp = convertCardIntToString(boost::lexical_cast<int>(results.result_Hand[i + nCol_Hand]), modus);
                                    if (string_tmp == "") {
                                        cleanUp(results, mySqliteLogDb);
                                        return 1;
                                    }
                                    log_string += boost::lexical_cast<std::string>(string_tmp.at(0));
                                    if (modus == 1 || modus == 3) log_string += "</b>";
                                    log_string += boost::lexical_cast<std::string>(string_tmp.erase(0, 1));
                                    log_string += "]";
                                    data_found = true;
                                }
                            }
                            if (!data_found) { cleanUp(results, mySqliteLogDb); return 1; }

                            if (round_ctr == GAME_STATE_POST_RIVER) {
                                for (i = 0; i < nCol_Hand; i++) {
                                    cmpString = "Seat_";
                                    cmpString += boost::lexical_cast<std::string>(results.result_Action[3 * action_ctr]);
                                    cmpString += "_Hand_text";
                                    if (boost::lexical_cast<std::string>(results.result_Hand[i]) == cmpString && results.result_Hand[i + nCol_Hand]) {
                                        log_string += " - " + boost::lexical_cast<std::string>(results.result_Hand[i + nCol_Hand]);
                                    }
                                }
                            }

                        }

                        if (!neu && boost::lexical_cast<std::string>(results.result_Action[3 * action_ctr + 1]) != "wins" && boost::lexical_cast<std::string>(results.result_Action[3 * action_ctr + 1]) != "shows" && boost::lexical_cast<std::string>(results.result_Action[3 * action_ctr + 1]) != "has" && boost::lexical_cast<std::string>(results.result_Action[3 * action_ctr + 1]) != "sits out" && boost::lexical_cast<std::string>(results.result_Action[3 * action_ctr + 1]) != "wins (side pot)" && boost::lexical_cast<std::string>(results.result_Action[3 * action_ctr + 1]) != "wins game" && boost::lexical_cast<std::string>(results.result_Action[3 * action_ctr + 1]) != "has left the game" && boost::lexical_cast<std::string>(results.result_Action[3 * action_ctr + 1]) != "was kicked from the game" && boost::lexical_cast<std::string>(results.result_Action[3 * action_ctr + 1]) != "is game admin now" && boost::lexical_cast<std::string>(results.result_Action[3 * action_ctr + 1]) != "has joined the game") {
                            log_string += ".";
                        }
                        if (neu && boost::lexical_cast<std::string>(results.result_Action[3 * action_ctr + 1]) != "wins game" && boost::lexical_cast<std::string>(results.result_Action[3 * action_ctr + 1]) != "has left the game" && boost::lexical_cast<std::string>(results.result_Action[3 * action_ctr + 1]) != "was kicked from the game" && boost::lexical_cast<std::string>(results.result_Action[3 * action_ctr + 1]) != "is game admin now" && boost::lexical_cast<std::string>(results.result_Action[3 * action_ctr + 1]) != "has joined the game")
                            log_string += ".";

                    }

                }

                if (modus == 1) log_string += "\n";
                emitChunk(log_string);
                log_string = "";

            }

        }

    }

    cleanUp(results, mySqliteLogDb);
    return 0;
}

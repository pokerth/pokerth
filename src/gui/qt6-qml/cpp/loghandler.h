/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2025 Felix Hammer, Florian Thauer, Lothar May          *
 *****************************************************************************/

#ifndef LOGHANDLER_H
#define LOGHANDLER_H

#include <QObject>
#include <QVariantList>
#include <QStringList>
#include <QString>
#include <memory>
#include <string>

#include <net/uploadcallback.h>

class ConfigFile;
class Log;
class UploaderThread;
class QTextStream;

// Backs the QML "Logs" page (LogsPage.qml). Ports the functionality of the
// Qt-Widgets LogFileDialog/guiLog: it lists the SQLite (.pdb) log files in the
// configured LogDir, lets the user pick a game, renders a readable HTML/text
// preview, and supports export (HTML/TXT), save-as, delete and the
// "Analyse Logfile" upload to pokerth.net.
class LogHandler : public QObject, public UploadCallback
{
    Q_OBJECT

    Q_PROPERTY(QVariantList logFiles READ logFiles NOTIFY logFilesChanged)
    Q_PROPERTY(bool uploadInProgress READ uploadInProgress NOTIFY uploadInProgressChanged)

public:
    explicit LogHandler(ConfigFile *config, QObject *parent = nullptr);
    ~LogHandler() override;

    // The engine Log knows the file currently being written → marked "current".
    void setLog(Log *log) { m_log = log; }

    QVariantList logFiles() const { return m_logFiles; }
    bool uploadInProgress() const { return m_uploadInProgress; }

    // Re-scan the LogDir (one entry per .pdb file, newest first).
    Q_INVOKABLE void refresh();
    // Unique game IDs contained in a log file.
    Q_INVOKABLE QVariantList gameList(const QString &path);
    // Rendered rich-text (HTML) preview of one game in a log file.
    Q_INVOKABLE QString previewHtml(const QString &path, int uniqueGameID = 0);
    // Export / save: show a native (QtWidgets) save dialog and write the file.
    // (QtQuick.Dialogs' FileDialog falls back to a QML implementation that needs
    // Qt.labs.folderlistmodel, which is not shipped with the binary.)
    Q_INVOKABLE void exportHtmlDialog(const QString &path);
    Q_INVOKABLE void exportTxtDialog(const QString &path);
    Q_INVOKABLE void saveAsDialog(const QString &path);
    // Lower-level variants (dest may be a file:// URL or a local path).
    Q_INVOKABLE bool exportHtml(const QString &path, const QString &destUrl);
    Q_INVOKABLE bool exportTxt(const QString &path, const QString &destUrl);
    Q_INVOKABLE bool saveAs(const QString &path, const QString &destUrl);
    Q_INVOKABLE bool deleteFiles(const QStringList &paths);
    // Helpers for default file-dialog names.
    Q_INVOKABLE QString homePath() const;
    Q_INVOKABLE QString baseName(const QString &path) const;
    // Upload the file to pokerth.net for web analysis.
    Q_INVOKABLE void analyse(const QString &path);

    // UploadCallback – invoked on the uploader thread.
    void UploadCompleted(const std::string &filename, const std::string &returnMessage) override;
    void UploadError(const std::string &filename, const std::string &errorMessage) override;

signals:
    void logFilesChanged();
    void uploadInProgressChanged();
    void analyseSucceeded(const QString &url);     // open this URL in a browser
    void analyseFailed(const QString &message);
    // Cross-thread bridge (uploader thread → GUI thread, queued).
    void uploadCompletedSignal(const QString &filename, const QString &returnMessage);
    void uploadErrorSignal(const QString &filename, const QString &errorMessage);

private slots:
    void onUploadCompleted(const QString &filename, const QString &returnMessage);
    void onUploadError(const QString &filename, const QString &errorMessage);

private:
    QString currentLogFileName() const;
    void setUploadInProgress(bool inProgress);
    // Faithful port of guiLog::exportLog. modus: 1=HTML file, 2=TXT file,
    // 3=HTML preview buffer. Returns 0 on success.
    int buildLog(const QString &fileStringPdb, int modus, int uniqueGameID = 0);
    void emitChunk(const std::string &log_string);

    ConfigFile *m_config = nullptr;
    Log *m_log = nullptr;
    QVariantList m_logFiles;
    bool m_uploadInProgress = false;
    std::unique_ptr<UploaderThread> m_uploader;
    bool m_uploaderRunning = false;

    // Output sinks used by buildLog/emitChunk.
    int m_curModus = 0;
    QString m_previewBuf;
    QTextStream *m_outStream = nullptr;
};

#endif // LOGHANDLER_H

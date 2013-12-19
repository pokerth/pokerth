/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2012 Felix Hammer, Florian Thauer, Lothar May          *
 *                                                                           *
 * This program is free software: you can redistribute it and/or modify      *
 * it under the terms of the GNU Affero General Public License as            *
 * published by the Free Software Foundation, either version 3 of the        *
 * License, or (at your option) any later version.                           *
 *                                                                           *
 * This program is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU Affero General Public License for more details.                       *
 *                                                                           *
 * You should have received a copy of the GNU Affero General Public License  *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.     *
 *                                                                           *
 *                                                                           *
 * Additional permission under GNU AGPL version 3 section 7                  *
 *                                                                           *
 * If you modify this program, or any covered work, by linking or            *
 * combining it with the OpenSSL project's OpenSSL library (or a             *
 * modified version of that library), containing parts covered by the        *
 * terms of the OpenSSL or SSLeay licenses, the authors of PokerTH           *
 * (Felix Hammer, Florian Thauer, Lothar May) grant you additional           *
 * permission to convey the resulting work.                                  *
 * Corresponding Source for a non-source form of such a combination          *
 * shall include the source code for the parts of OpenSSL used as well       *
 * as that of the covered work.                                              *
 *****************************************************************************/

#ifndef LOGFILEDIALOG_H
#define LOGFILEDIALOG_H

#include <QDialog>
#include <QFile>
#include <net/uploadcallback.h>
#include <boost/shared_ptr.hpp>

class UploaderThread;

namespace Ui
{
class LogFileDialog;
}

enum ShowLogMode { INIT_VIEW, SELECTED_GAME };

class guiLog;
class ConfigFile;
class Callback;

class LogFileDialog : public QDialog, public UploadCallback
{
	Q_OBJECT

public:
	explicit LogFileDialog(QWidget *parent = 0, ConfigFile *c = 0);
	~LogFileDialog();

	void setGuiLog(guiLog *g) {
		myGuiLog = g;
	}
	int exec();

	virtual void UploadCompleted(const std::string &filename, const std::string &returnMessage);
	virtual void UploadError(const std::string &filename, const std::string &errorMessage);

signals:
	void signalUploadCompleted(QString filename, QString returnMessage);
	void signalUploadError(QString filename, QString errorMessage);

public slots:
	void refreshLogFileList();
	void deleteLogFile();
	void exportLogToHtml();
	void exportLogToTxt();
	void saveLogFileAs();
	void showLogFilePreviewInit() {
		showLogFilePreview(INIT_VIEW);
	}
	void showLogFilePreviewSelected() {
		showLogFilePreview(SELECTED_GAME);
	}
	void showLogFilePreview(ShowLogMode);
	void keyPressEvent ( QKeyEvent * event );
	void uploadFile();
	void uploadInProgressAnimationStart();
	void uploadInProgressAnimationStop();

	void showLogAnalysis(QString filename, QString returnMessage);
	void showUploadError(QString filename, QString errorMessage);

private:
	ConfigFile *myConfig;
	guiLog *myGuiLog;
	Ui::LogFileDialog *ui;
	int writer(char *data, size_t size, size_t nmemb,std::string *buffer);
	QFile file;
	QString id;
	boost::shared_ptr<UploaderThread> uploader;
};

#endif // LOGFILEDIALOG_H

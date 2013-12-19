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

#include "logfiledialog.h"
#ifdef GUI_800x480
#include "ui_logfiledialog_800x480.h"
#else
#include "ui_logfiledialog.h"
#endif

#include <QtCore>
#include "guilog.h"
#include "configfile.h"
#include "mymessagebox.h"
#include <game_defs.h>
#include <net/uploaderthread.h>

LogFileDialog::LogFileDialog(QWidget *parent, ConfigFile *c) :
	QDialog(parent), myConfig(c),
	ui(new Ui::LogFileDialog)
{
	ui->setupUi(this);
	ui->label_animation->setMaximumWidth(0);
	ui->horizontalLayout_animation->setSpacing(0);
#ifdef ANDROID
	this->setWindowState(Qt::WindowFullScreen);
#endif
	connect( ui->pushButton_deleteLog, SIGNAL(clicked()), this, SLOT (deleteLogFile()));
	connect( ui->pushButton_exportLogHtml, SIGNAL(clicked()), this, SLOT (exportLogToHtml()));
	connect( ui->pushButton_exportLogTxt, SIGNAL(clicked()), this, SLOT (exportLogToTxt()));
	connect( ui->pushButton_saveLogAs, SIGNAL(clicked()), this, SLOT (saveLogFileAs()));
	connect( ui->treeWidget_logFiles, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), this, SLOT(showLogFilePreviewInit()));
	connect( ui->pushButton_analyseLogfile, SIGNAL(clicked()), this, SLOT (uploadFile()));
	connect( ui->comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(showLogFilePreviewSelected()));
	connect( this, SIGNAL(signalUploadCompleted(QString,QString)), this, SLOT(showLogAnalysis(QString,QString)));
	connect( this, SIGNAL(signalUploadError(QString,QString)), this, SLOT(showUploadError(QString,QString)));

	uploader.reset(new UploaderThread(this));
}

LogFileDialog::~LogFileDialog()
{
	delete ui;
}

int LogFileDialog::exec()
{
	uploader->Run();
	refreshLogFileList();
	int ret	= QDialog::exec();
	uploader->SignalTermination();
	uploader->Join(THREAD_WAIT_INFINITE);
	return ret;
}

void LogFileDialog::UploadCompleted(const std::string &filename, const std::string &returnMessage)
{
	signalUploadCompleted(QString::fromStdString(filename), QString::fromStdString(returnMessage));
}

void LogFileDialog::UploadError(const std::string &filename, const std::string &errorMessage)
{
	signalUploadError(QString::fromStdString(filename), QString::fromStdString(errorMessage));
}

void LogFileDialog::refreshLogFileList()
{
	QDir logFileDir;
	logFileDir.setPath(QString::fromUtf8(myConfig->readConfigString("LogDir").c_str()));
	QStringList filters;
	filters << "*.pdb";
	QFileInfoList dbFilesList = logFileDir.entryInfoList(filters, QDir::Files, QDir::Time);

	QFileInfo currentSqliteLogFile(QString::fromStdString(myGuiLog->getMySqliteLogFileName()));

	ui->treeWidget_logFiles->blockSignals(true);
	ui->treeWidget_logFiles->clear();
	int i;
	for (i=0; i < dbFilesList.size(); i++) {

		QTreeWidgetItem *item = new QTreeWidgetItem;
#ifdef GUI_800x480
		item->setText(0, dbFilesList.at(i).fileName().remove("pokerth-log-"));
#else
		item->setText(0, dbFilesList.at(i).fileName());
#endif
		item->setData(0, Qt::UserRole, dbFilesList.at(i).absoluteFilePath());
		if(currentSqliteLogFile.fileName() == dbFilesList.at(i).fileName()) {
			item->setData(0, Qt::BackgroundColorRole, QColor(Qt::red));
			item->setData(0, Qt::TextColorRole, QColor(Qt::white));
			item->setData(0, Qt::UserRole+1, "current");
		}
		ui->treeWidget_logFiles->addTopLevelItem(item);
	}
	ui->treeWidget_logFiles->sortItems(0, Qt::DescendingOrder);
	ui->treeWidget_logFiles->blockSignals(false);
	ui->treeWidget_logFiles->setCurrentItem(ui->treeWidget_logFiles->topLevelItem(0));
}

void LogFileDialog::deleteLogFile()
{
	QList<QTreeWidgetItem*> selectedItemsList = ui->treeWidget_logFiles->selectedItems();

	if(!selectedItemsList.isEmpty() && !( selectedItemsList.size() == 1 && selectedItemsList.front()->data(0, Qt::UserRole+1).toString() == "current" )) {

		int ret = MyMessageBox::warning(this, tr("PokerTH - Delete log files"),
										tr("Do you really want to delete the selected log files?"),
										QMessageBox::Yes | QMessageBox::No);

		if(ret == QMessageBox::Yes) {
			for (int i = 0; i < selectedItemsList.size(); ++i) {
				if(selectedItemsList.at(i)->data(0, Qt::UserRole+1).toString() != "current") {

					if(!QFile::remove(selectedItemsList.at(i)->data(0, Qt::UserRole).toString())) {
						MyMessageBox::warning(this, tr("Remove log file"), tr("PokerTH cannot remove this log file, please verify that you have write access to this file!"), QMessageBox::Close );
					}
				}
			}
			refreshLogFileList();
		}
	}
}

void LogFileDialog::exportLogToHtml()
{
	QTreeWidgetItem* selectedItem = ui->treeWidget_logFiles->currentItem();

	if(selectedItem) {
		QFileInfo fi(selectedItem->data(0, Qt::UserRole).toString());
		QString fileName = QFileDialog::getSaveFileName(this, tr("Export PokerTH log file to HTML"),
						   QDir::homePath()+"/"+fi.baseName()+".html",
						   tr("PokerTH HTML log (*.html)"));

		if(!fileName.isEmpty()) {
			myGuiLog->exportLogPdbToHtml(selectedItem->data(0, Qt::UserRole).toString(),fileName);
		}
	}
}

void LogFileDialog::exportLogToTxt()
{
	QTreeWidgetItem* selectedItem = ui->treeWidget_logFiles->currentItem();

	if(selectedItem) {
		QFileInfo fi(selectedItem->data(0, Qt::UserRole).toString());
		QString fileName = QFileDialog::getSaveFileName(this, tr("Export PokerTH log file to plain text"),
						   QDir::homePath()+"/"+fi.baseName()+".txt",
						   tr("PokerTH plain text log (*.txt)"));

		if(!fileName.isEmpty()) {
			myGuiLog->exportLogPdbToTxt(selectedItem->data(0, Qt::UserRole).toString(),fileName);
		}
	}
}

void LogFileDialog::saveLogFileAs()
{
	QTreeWidgetItem* selectedItem = ui->treeWidget_logFiles->currentItem();

	if(selectedItem) {
		QFileInfo fi(selectedItem->data(0, Qt::UserRole).toString());
		QString fileName = QFileDialog::getSaveFileName(this, tr("Save PokerTH log file"),
						   QDir::homePath()+"/"+fi.baseName()+".pdb",
						   tr("PokerTH SQL log (*.pdb)"));

		if(!fileName.isEmpty()) {
			QFile::copy(selectedItem->data(0, Qt::UserRole).toString(), fileName);
		}
	}
}

void LogFileDialog::showLogFilePreview(ShowLogMode mode)
{
	QTreeWidgetItem* selectedItem = ui->treeWidget_logFiles->currentItem();

	if(mode == INIT_VIEW) {
		ui->comboBox->blockSignals(true);
		ui->comboBox->clear();
		QList<int> gameIds = myGuiLog->getGameList(selectedItem->data(0, Qt::UserRole).toString());
		QList<int>::const_iterator it;
		for (it = gameIds.constBegin(); it != gameIds.constEnd(); ++it) {
			ui->comboBox->addItem(QString::number(*it));
		}
		ui->comboBox->blockSignals(false);
	}

	if(selectedItem) {
		myGuiLog->showLog(selectedItem->data(0, Qt::UserRole).toString(), ui->textBrowser_logPreview, ui->comboBox->itemText(ui->comboBox->currentIndex()).toInt());
	}

	QTextCursor cursor(ui->textBrowser_logPreview->textCursor());
	cursor.movePosition(QTextCursor::Start);
	ui->textBrowser_logPreview->setTextCursor(cursor);
}

void LogFileDialog::keyPressEvent ( QKeyEvent * event )
{
	if (event->key() == Qt::Key_Delete ) { /*Delete*/
		if(ui->treeWidget_logFiles->hasFocus()) {
			ui->pushButton_deleteLog->click();
		}
	}
}

void LogFileDialog::uploadFile()
{
	QTreeWidgetItem* selectedItem = ui->treeWidget_logFiles->currentItem();

	if(selectedItem) {
		file.setFileName(selectedItem->data(0, Qt::UserRole).toString());

		uploadInProgressAnimationStart();
		uploader->QueueUpload(
			"http://pokerth.net/log_file_analysis/upload.php",
			"",
			"",
			file.fileName().toStdString(),
			file.size(),
			"pdb_file");
	}
}

void LogFileDialog::uploadInProgressAnimationStart()
{
	//const QString buttonText(tr("Upload in progress"));
	ui->pushButton_analyseLogfile->setDisabled(true);

	QMovie *movie = new QMovie(":/gfx/loader.gif");
	ui->label_animation->setMovie(movie);
	ui->label_animation->setMaximumWidth(32);
	ui->horizontalLayout_animation->setSpacing(-1);
	ui->label_animation->setGeometry(0,0,32,32);
	movie->start();
}

void LogFileDialog::uploadInProgressAnimationStop()
{
	ui->pushButton_analyseLogfile->setDisabled(false);

	ui->label_animation->setMaximumWidth(0);
	ui->horizontalLayout_animation->setSpacing(0);
	ui->label_animation->setGeometry(0,0,0,0);
}

void LogFileDialog::showLogAnalysis(QString /*filename*/, QString returnMessage)
{
	uploadInProgressAnimationStop();

	returnMessage = returnMessage.trimmed();
	QString retStr(returnMessage.mid(0, returnMessage.indexOf(' ')));

	if (retStr == LOG_UPLOAD_OK_STR) {
		QString hash(returnMessage.mid(retStr.size()).trimmed());
		qDebug() << hash << endl;
		QDesktopServices::openUrl(QUrl("http://logfile-analysis.pokerth.net/?ID=" + hash));
	} else {
		qDebug() << returnMessage << endl;
		QString serverMsg(tr("Processing of the log file on the web server failed.\nPlease verify that you are uploading a valid PokerTH log file."));
		// if there is an error code, display a corresponding message.
		if (retStr == LOG_UPLOAD_ERROR_STR) {
			QString errorId(returnMessage.mid(retStr.size()).trimmed());
			serverMsg += "\n" + tr("Failure reason: ");
			switch (errorId.toInt()) {
			case LOG_UPLOAD_ERROR_NO_FILE :
				serverMsg += tr("No file received.");
				break;
			case LOG_UPLOAD_ERROR_MAX_NUM_TOTAL :
				serverMsg += tr("File rejected because of too many uploads.");
				break;
			case LOG_UPLOAD_ERROR_MAX_NUM_IP :
				serverMsg += tr("File rejected because of too many recent uploads. Please try again later.");
				break;
			case LOG_UPLOAD_ERROR_FILE_SIZE :
				serverMsg += tr("The file is too large.");
				break;
			case LOG_UPLOAD_ERROR_FILE_EXT :
			case LOG_UPLOAD_ERROR_FILE_HEAD :
				serverMsg += tr("This file is not a valid and current PokerTH log file.");
				break;
			case LOG_UPLOAD_ERROR_OPEN_DB :
			case LOG_UPLOAD_ERROR_ID :
			case LOG_UPLOAD_ERROR_FILE_MOVE :
			case LOG_UPLOAD_ERROR_INSERT_DB :
			default :
				serverMsg += tr("Internal error. Please try again later. ID: ") + returnMessage;
				break;
			}
		}
		MyMessageBox::warning(
			this, tr("Uploading log file"), serverMsg, QMessageBox::Close );
	}
}

void LogFileDialog::showUploadError(QString /*filename*/, QString errorMessage)
{
	uploadInProgressAnimationStop();

	QString uploadMsg(tr("Upload failed. Please check your internet connection!\nUploading log files may fail if you are using an http proxy."));
	uploadMsg += "\n" + tr("Failure reason: ") + errorMessage;
	MyMessageBox::warning(
		this, tr("Uploading log file"), uploadMsg, QMessageBox::Close );
}


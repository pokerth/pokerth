/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2011 Felix Hammer, Florian Thauer, Lothar May          *
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
 *****************************************************************************/

#ifndef LOGFILEDIALOG_H
#define LOGFILEDIALOG_H

#include <QDialog>


namespace Ui {
	class LogFileDialog;
}


class guiLog;
class ConfigFile;

class LogFileDialog : public QDialog
{
	Q_OBJECT
	
public:
	explicit LogFileDialog(QWidget *parent = 0, ConfigFile *c = 0);
	~LogFileDialog();

	void setGuiLog(guiLog *g) { myGuiLog = g; }
	void exec();

public slots:
	void refreshLogFileList();
	void deleteLogFile();
	void exportLogToHtml();
	void exportLogToTxt();
	void saveLogFileAs();
	void showLogFilePreview();
	void keyPressEvent ( QKeyEvent * event );
	
private:
	ConfigFile *myConfig;
	guiLog *myGuiLog;
	Ui::LogFileDialog *ui;
};

#endif // LOGFILEDIALOG_H

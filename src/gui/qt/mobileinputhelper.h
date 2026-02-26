/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2025 PokerTH development team                          *
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
 *****************************************************************************/

#ifndef MOBILEINPUTHELPER_H
#define MOBILEINPUTHELPER_H

#include <QObject>
#include <QScrollArea>
#include <QWidget>
#include <QEvent>
#include <QLineEdit>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QDialog>
#include <QStackedWidget>

/**
 * @brief Interim helper for Qt Widgets mobile keyboard handling
 * 
 * This is NOT a perfect solution but pragmatic improvements for mobile usage
 * until the planned Qt Quick/QML UI rewrite.
 * 
 * Features:
 * - Auto-scroll input widgets into view when focused
 * - Enable proper input method support
 * - Inform OS about text input location
 * 
 * Limitations (intentional):
 * - No keyboard height detection
 * - No native platform hooks
 * - Still suboptimal on phones (acceptable on tablets)
 */
class MobileInputHelper : public QObject
{
	Q_OBJECT

public:
	static MobileInputHelper& instance();
	
	// Install on a QScrollArea to enable auto-scrolling for input widgets
	void installOnScrollArea(QScrollArea *scrollArea);
	
	// Prepare a line edit for mobile usage
	static void prepareMobileLineEdit(QLineEdit *lineEdit);
	
	// Prepare a text edit for mobile usage
	static void prepareMobileTextEdit(QTextEdit *textEdit);
	static void prepareMobilePlainTextEdit(QPlainTextEdit *textEdit);

	// Make a QDialog fullscreen on Android with correct geometry.
	// Call AFTER setupUi().  Uses FramelessWindowHint + screen geometry
	// so the dialog reliably fills the screen with QT_SCALE_FACTOR.
	static void prepareAndroidDialog(QDialog *dialog);

	// Wrap each page of a QStackedWidget in a QScrollArea so content
	// that doesn't fit vertically becomes scrollable.
	static void wrapStackedWidgetPagesInScrollAreas(QStackedWidget *sw);

protected:
	bool eventFilter(QObject *watched, QEvent *event) override;

private:
	MobileInputHelper();
	~MobileInputHelper();
	MobileInputHelper(const MobileInputHelper&) = delete;
	MobileInputHelper& operator=(const MobileInputHelper&) = delete;
	
	void handleFocusIn(QWidget *widget);
	void updateInputItemRectangle(QWidget *widget);
	
	QScrollArea *currentScrollArea;
};

#endif // MOBILEINPUTHELPER_H

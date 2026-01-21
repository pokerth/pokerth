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

#include "mobileinputhelper.h"
#include <QGuiApplication>
#include <QInputMethod>
#include <QRect>
#include <QInputMethodEvent>
#include <QKeyEvent>

MobileInputHelper::MobileInputHelper()
	: QObject(nullptr), currentScrollArea(nullptr)
{
}

MobileInputHelper::~MobileInputHelper()
{
}

MobileInputHelper& MobileInputHelper::instance()
{
	static MobileInputHelper inst;
	return inst;
}

void MobileInputHelper::installOnScrollArea(QScrollArea *scrollArea)
{
	if (scrollArea) {
		scrollArea->installEventFilter(this);
		currentScrollArea = scrollArea;
	}
}

void MobileInputHelper::prepareMobileLineEdit(QLineEdit *lineEdit)
{
	if (!lineEdit) return;
	
#ifdef ANDROID
	// Enable input method for proper IME support
	lineEdit->setAttribute(Qt::WA_InputMethodEnabled, true);
	
	// Install focus event handler for location updates
	lineEdit->installEventFilter(&instance());
#endif
}

void MobileInputHelper::prepareMobileTextEdit(QTextEdit *textEdit)
{
	if (!textEdit) return;
	
#ifdef ANDROID
	textEdit->setAttribute(Qt::WA_InputMethodEnabled, true);
	textEdit->installEventFilter(&instance());
#endif
}

void MobileInputHelper::prepareMobilePlainTextEdit(QPlainTextEdit *textEdit)
{
	if (!textEdit) return;
	
#ifdef ANDROID
	textEdit->setAttribute(Qt::WA_InputMethodEnabled, true);
	textEdit->installEventFilter(&instance());
#endif
}

bool MobileInputHelper::eventFilter(QObject *watched, QEvent *event)
{
	if (event->type() == QEvent::FocusIn) {
		QWidget *widget = qobject_cast<QWidget*>(watched);
		if (widget) {
			handleFocusIn(widget);
		}
	}
	
	// Let the widget handle all events normally
	return QObject::eventFilter(watched, event);
}

void MobileInputHelper::handleFocusIn(QWidget *widget)
{
#ifdef ANDROID
	// Only update input method about widget location - don't force show/commit
	// as that causes keyboard flicker on delete/backspace
	updateInputItemRectangle(widget);
	
	// Scroll widget into view if inside a scroll area
	if (currentScrollArea && currentScrollArea->widget()) {
		// Add some extra margin to ensure keyboard doesn't cover it
		int extraMargin = 100; // pixels
		QRect widgetRect = widget->geometry();
		widgetRect.adjust(0, -extraMargin, 0, extraMargin);
		
		currentScrollArea->ensureVisible(
			widgetRect.center().x(),
			widgetRect.center().y(),
			widgetRect.width() / 2 + 50,
			widgetRect.height() / 2 + extraMargin
		);
	}
#endif
}

void MobileInputHelper::updateInputItemRectangle(QWidget *widget)
{
	if (!widget) return;
	
	QInputMethod *im = QGuiApplication::inputMethod();
	if (im) {
		// Map widget geometry to global coordinates
		QRect globalRect = QRect(
			widget->mapToGlobal(QPoint(0, 0)),
			widget->size()
		);
		
		// Inform the input method where text input happens
		im->setInputItemRectangle(globalRect);
	}
}

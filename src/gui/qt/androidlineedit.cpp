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

#include "androidlineedit.h"
#include <QGuiApplication>
#include <QInputMethod>
#include <QInputMethodEvent>

AndroidLineEdit::AndroidLineEdit(QWidget *parent)
	: QLineEdit(parent)
#ifdef ANDROID
	, m_inComposition(false)
	, m_blockRepaint(false)
#endif
{
#ifdef ANDROID
	setAttribute(Qt::WA_InputMethodEnabled, true);
	// Disable automatic IME updates that cause flicker
	setAttribute(Qt::WA_InputMethodTransparent, false);
#endif
}

AndroidLineEdit::AndroidLineEdit(const QString &contents, QWidget *parent)
	: QLineEdit(contents, parent)
#ifdef ANDROID
	, m_inComposition(false)
	, m_blockRepaint(false)
#endif
{
#ifdef ANDROID
	setAttribute(Qt::WA_InputMethodEnabled, true);
	// Disable automatic IME updates that cause flicker
	setAttribute(Qt::WA_InputMethodTransparent, false);
#endif
}

#ifdef ANDROID
void AndroidLineEdit::inputMethodEvent(QInputMethodEvent *event)
{
	// Block repaint during composition to prevent keyboard flicker
	m_inComposition = true;
	m_blockRepaint = true;
	
	// Process the event normally
	QLineEdit::inputMethodEvent(event);
	
	// If this is the final commit (not preedit), allow repaint
	if (!event->commitString().isEmpty() && event->preeditString().isEmpty()) {
		m_blockRepaint = false;
		m_inComposition = false;
		// Force one clean update after commit
		QLineEdit::update();
	} else if (event->preeditString().isEmpty()) {
		// No preedit and no commit = deletion, allow repaint
		m_blockRepaint = false;
		m_inComposition = false;
		QLineEdit::update();
	}
	
	// Accept without triggering additional updates
	event->accept();
}

void AndroidLineEdit::focusInEvent(QFocusEvent *event)
{
	m_blockRepaint = false;
	m_inComposition = false;
	QLineEdit::focusInEvent(event);
	event->accept();
}

void AndroidLineEdit::keyPressEvent(QKeyEvent *event)
{
	// For direct key events, don't block repaint
	m_blockRepaint = false;
	QLineEdit::keyPressEvent(event);
	event->accept();
}

void AndroidLineEdit::paintEvent(QPaintEvent *event)
{
	// Skip repaint during composition to prevent keyboard flicker
	if (m_blockRepaint) {
		event->accept();
		return;
	}
	
	// Normal paint
	QLineEdit::paintEvent(event);
}

QVariant AndroidLineEdit::inputMethodQuery(Qt::InputMethodQuery property) const
{
	// During composition, return minimal info to prevent flicker
	if (m_inComposition) {
		switch (property) {
		case Qt::ImEnabled:
			return true;
		case Qt::ImCursorRectangle:
			return cursorRect();
		case Qt::ImCursorPosition:
			return cursorPosition();
		case Qt::ImSurroundingText:
			return text();
		case Qt::ImCurrentSelection:
			return selectedText();
		default:
			return QVariant();
		}
	}
	
	// Normal behavior when not composing
	return QLineEdit::inputMethodQuery(property);
}
#endif

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

#ifndef ANDROIDLINEEDIT_H
#define ANDROIDLINEEDIT_H

#include <QLineEdit>

/**
 * @brief QLineEdit with improved Android IME handling
 * 
 * This class fixes the keyboard flicker issue when deleting characters
 * on Android by properly handling input method events without retriggering
 * keyboard show/hide cycles.
 */
class AndroidLineEdit : public QLineEdit
{
	Q_OBJECT

public:
	explicit AndroidLineEdit(QWidget *parent = nullptr);
	explicit AndroidLineEdit(const QString &contents, QWidget *parent = nullptr);

protected:
#ifdef ANDROID
	void inputMethodEvent(QInputMethodEvent *event) override;
	void focusInEvent(QFocusEvent *event) override;
	void keyPressEvent(QKeyEvent *event) override;
	QVariant inputMethodQuery(Qt::InputMethodQuery property) const override;
	void paintEvent(QPaintEvent *event) override;

private:
	bool m_inComposition;
	bool m_blockRepaint;
#endif
};

#endif // ANDROIDLINEEDIT_H

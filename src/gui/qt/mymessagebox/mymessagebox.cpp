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
#include "mymessagebox.h"
#include <QMessageBox>

MyMessageBox::MyMessageBox(QWidget *parent) :
	QMessageBox(parent)
{
#ifdef ANDROID
	this->setWindowModality(Qt::WindowModal);
	this->setWindowFlags(Qt::ToolTip);
	this->setStyleSheet("QDialog{ border: 1px solid black; border-style: solid; border-radius: 4px; }");
#endif
}

MyMessageBox::MyMessageBox(Icon icon, const QString &title, const QString &text, QFlags<QMessageBox::StandardButton> buttons, QWidget *parent, Qt::WindowFlags flags) :
	QMessageBox(icon, title, text, buttons, parent, flags)
{
#ifdef ANDROID
	this->setWindowModality(Qt::WindowModal);
	this->setWindowFlags(Qt::ToolTip);
	this->setStyleSheet("QDialog{ border: 1px solid black; border-style: solid; border-radius: 4px; }");
#endif
}

MyMessageBox::~MyMessageBox()
{

}

QMessageBox::StandardButton MyMessageBox::information(QWidget *parent, const QString &title, const QString &text, QFlags<QMessageBox::StandardButton> buttons, QMessageBox::StandardButton defaultButton)
{
	QMessageBox box(QMessageBox::Information, title, text, buttons, parent);
#ifdef ANDROID
	box.setWindowModality(Qt::WindowModal);
	box.setWindowFlags(Qt::ToolTip);
	box.setStyleSheet("QDialog{ border: 1px solid black; border-style: solid; border-radius: 4px; }");
#endif
	box.setDefaultButton(defaultButton);
	return static_cast<QMessageBox::StandardButton>(box.exec());
}

QMessageBox::StandardButton MyMessageBox::question(QWidget *parent, const QString &title, const QString &text, QFlags<QMessageBox::StandardButton> buttons, QMessageBox::StandardButton defaultButton)
{
	QMessageBox box(QMessageBox::Question, title, text, buttons, parent);
#ifdef ANDROID
	box.setWindowModality(Qt::WindowModal);
	box.setWindowFlags(Qt::ToolTip);
	box.setStyleSheet("QDialog{ border: 1px solid black; border-style: solid; border-radius: 4px; }");
#endif
	box.setDefaultButton(defaultButton);
	return static_cast<QMessageBox::StandardButton>(box.exec());
}

QMessageBox::StandardButton MyMessageBox::warning(QWidget *parent, const QString &title, const QString &text, QFlags<QMessageBox::StandardButton> buttons, QMessageBox::StandardButton defaultButton)
{
	QMessageBox box(QMessageBox::Warning, title, text, buttons, parent);
#ifdef ANDROID
	box.setWindowModality(Qt::WindowModal);
	box.setWindowFlags(Qt::ToolTip);
	box.setStyleSheet("QDialog{ border: 1px solid black; border-style: solid; border-radius: 4px; }");
#endif
	box.setDefaultButton(defaultButton);
	return static_cast<QMessageBox::StandardButton>(box.exec());
}

QMessageBox::StandardButton MyMessageBox::critical(QWidget *parent, const QString &title, const QString &text, QFlags<QMessageBox::StandardButton> buttons, QMessageBox::StandardButton defaultButton)
{
	QMessageBox box(QMessageBox::Critical, title, text, buttons, parent);
#ifdef ANDROID
	box.setWindowModality(Qt::WindowModal);
	box.setWindowFlags(Qt::ToolTip);
	box.setStyleSheet("QDialog{ border: 1px solid black; border-style: solid; border-radius: 4px; }");
#endif
	box.setDefaultButton(defaultButton);
	return static_cast<QMessageBox::StandardButton>(box.exec());
}


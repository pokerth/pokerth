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
#include "mygamelistsortfilterproxymodel.h"
#include <QtGui>
#include <QtCore>

MyGameListSortFilterProxyModel::MyGameListSortFilterProxyModel(QObject *parent)
	: QSortFilterProxyModel(parent)
{
}

bool MyGameListSortFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{

	QModelIndex index0 = sourceModel()->index(sourceRow, 0, sourceParent);
	QModelIndex index1 = sourceModel()->index(sourceRow, 1, sourceParent);
	QModelIndex index2 = sourceModel()->index(sourceRow, 2, sourceParent);
	QModelIndex index3 = sourceModel()->index(sourceRow, 3, sourceParent);
	QModelIndex index4 = sourceModel()->index(sourceRow, 4, sourceParent);
	QModelIndex index5 = sourceModel()->index(sourceRow, 5, sourceParent);

	return ((sourceModel()->data(index1, 16).toString().contains(column1RegExp)
			 && sourceModel()->data(index2, 16).toString().contains(column2RegExp)
			 && sourceModel()->data(index3, 16).toString().contains(column3RegExp)
			 && sourceModel()->data(index4, 16).toString().contains(column4RegExp)
			 && sourceModel()->data(index5, 16).toString().contains(column5RegExp)) || sourceModel()->data(index0, 16) == "MeInThisGame");
}

bool MyGameListSortFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
	switch(sortColumn()) {

	case 5: {
		return sourceModel()->data(left, 16).toString() < sourceModel()->data(right, 16).toString();
	}
	break;
	default: {
		return QSortFilterProxyModel::lessThan(left, right);
	}
	break;
	}

	return false;
}

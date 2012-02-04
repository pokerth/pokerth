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
#include "mynicklistsortfilterproxymodel.h"
#include <QtGui>
#include <QtCore>

MyNickListSortFilterProxyModel::MyNickListSortFilterProxyModel(QObject *parent)
: QSortFilterProxyModel(parent), filterState(0), lastFilterStateAlpha(true), lastFilterStateCountry(false)
{
}

bool MyNickListSortFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{

	switch(filterState) {

	case 0: {
		return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
	}
	break;
	case 1: {
		return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
	}
	break;
	case 2: {
		QModelIndex index0 = sourceModel()->index(sourceRow, 0, sourceParent);
		return sourceModel()->data(index0, 34).toString().contains("idle") && QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
	}
	break;

	}

	return false;
}

bool MyNickListSortFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
	switch(filterState) {

	case 0: {
		return QSortFilterProxyModel::lessThan(left, right);
	}
	break;
	case 1: {
		QString leftCountryAndName(sourceModel()->data(left, 33).toString().toUpper()+sourceModel()->data(left, Qt::DisplayRole).toString().toLower());
		QString rightCountryAndName(sourceModel()->data(right, 33).toString().toUpper()+sourceModel()->data(right, Qt::DisplayRole).toString().toLower());
		return leftCountryAndName < rightCountryAndName;
	}
	break;
	case 2: {
		if(lastFilterStateCountry) {
			QString leftCountryAndName(sourceModel()->data(left, 33).toString().toUpper()+sourceModel()->data(left, Qt::DisplayRole).toString().toLower());
			QString rightCountryAndName(sourceModel()->data(right, 33).toString().toUpper()+sourceModel()->data(right, Qt::DisplayRole).toString().toLower());
			return leftCountryAndName < rightCountryAndName;
		}
		else if(lastFilterStateAlpha) {
			return QSortFilterProxyModel::lessThan(left, right);
		}
	}
	break;
	}

	return false;
}

void MyNickListSortFilterProxyModel::setFilterState(int state)
{
	filterState = state;
}


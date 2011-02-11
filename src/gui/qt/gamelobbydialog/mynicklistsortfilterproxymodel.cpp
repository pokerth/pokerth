#include "mynicklistsortfilterproxymodel.h"
#include <QtGui>
#include <QtCore>

MyNickListSortFilterProxyModel::MyNickListSortFilterProxyModel(QObject *parent)
	: QSortFilterProxyModel(parent), filterState(0)
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
		return QSortFilterProxyModel::lessThan(left, right);
	}
	break;
	}

	return false;
}

void MyNickListSortFilterProxyModel::setFilterState(int state)
{
	filterState = state;
}


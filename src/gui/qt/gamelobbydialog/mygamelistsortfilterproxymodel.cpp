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

	return ((sourceModel()->data(index1, 16).toString().contains(column1RegExp)
			 && sourceModel()->data(index2, 16).toString().contains(column2RegExp)
			 && sourceModel()->data(index3, 16).toString().contains(column3RegExp)
			 && sourceModel()->data(index4, 16).toString().contains(column4RegExp)) || sourceModel()->data(index0, 16) == "MeInThisGame");
}


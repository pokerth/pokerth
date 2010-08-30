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

//	QModelIndex index0 = sourceModel()->index(sourceRow, 0, sourceParent);
//	QModelIndex index1 = sourceModel()->index(sourceRow, 1, sourceParent);
//	QModelIndex index2 = sourceModel()->index(sourceRow, 2, sourceParent);
//	QModelIndex index3 = sourceModel()->index(sourceRow, 3, sourceParent);
//        QModelIndex index4 = sourceModel()->index(sourceRow, 4, sourceParent);
//
//	return ((sourceModel()->data(index1, 16).toString().contains(column1RegExp)
//			&& sourceModel()->data(index2, 16).toString().contains(column2RegExp)
//                        && sourceModel()->data(index3, 16).toString().contains(column3RegExp)
//                        && sourceModel()->data(index4, 16).toString().contains(column4RegExp)) || sourceModel()->data(index0, 16) == "MeInThisGame");
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
}

void MyNickListSortFilterProxyModel::setFilterState(int state)
{
   filterState = state;
}


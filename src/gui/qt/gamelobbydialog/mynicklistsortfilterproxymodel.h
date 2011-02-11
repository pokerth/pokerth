#ifndef MYNICKLISTSORTFILTERPROXYMODEL_H
#define MYNICKLISTSORTFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>

class MyNickListSortFilterProxyModel : public QSortFilterProxyModel
{
	Q_OBJECT

public:
	MyNickListSortFilterProxyModel(QObject *parent = 0);

	void setFilterState(int state);

protected:
	bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;
	bool lessThan(const QModelIndex &left, const QModelIndex &right) const;

private:

	int filterState;


};

#endif // MYGAMELISTSORTFILTERPROXYMODEL_H

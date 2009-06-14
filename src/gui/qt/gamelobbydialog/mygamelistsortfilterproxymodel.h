#ifndef MYGAMELISTSORTFILTERPROXYMODEL_H
#define MYGAMELISTSORTFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>

class MyGameListSortFilterProxyModel : public QSortFilterProxyModel
{  
Q_OBJECT

 public:
	MyGameListSortFilterProxyModel(QObject *parent = 0);   
	void setColumn1RegExp(QRegExp column1) { column1RegExp = column1; }
	void setColumn2RegExp(QRegExp column2) { column2RegExp = column2; }
	void setColumn3RegExp(QRegExp column3) { column3RegExp = column3; }
	
 protected:
     bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;

 private:
	 QRegExp column1RegExp;
	 QRegExp column2RegExp;
	 QRegExp column3RegExp;

};

#endif // MYGAMELISTSORTFILTERPROXYMODEL_H


#include "proxymodels.h"


MostUsedProxyModel::MostUsedProxyModel(QObject * parent)
	: QSortFilterProxyModel(parent) {}


bool MostUsedProxyModel::filterAcceptsRow(int row,const QModelIndex & parent) const {

	const auto model = sourceModel();
	const auto index = model -> index(row,0,parent);

	return (model -> data(index,sortRole()).toInt() > 0);
}


BooleanFilterProxyModel::BooleanFilterProxyModel(QObject * parent)
	: QSortFilterProxyModel(parent) {}


bool BooleanFilterProxyModel::filterAcceptsRow(int row,const QModelIndex & parent) const {

	const auto model = sourceModel();
	const auto index = model -> index(row,0,parent);

	return model -> data(index,filterRole()).toBool();
}

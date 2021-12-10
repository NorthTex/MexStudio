
#include "proxymodels.h"


MostUsedProxyModel::MostUsedProxyModel(QObject * parent)
	: QSortFilterProxyModel(parent) {}


bool MostUsedProxyModel::filterAcceptsRow(int row,const QModelIndex & parent) const {

	auto index = sourceModel() -> index(row,0,parent);

	int count = sourceModel() -> data(index, sortRole()).toInt();
	
	return count > 0;
}


BooleanFilterProxyModel::BooleanFilterProxyModel(QObject * parent)
	: QSortFilterProxyModel(parent) {}


bool BooleanFilterProxyModel::filterAcceptsRow(int row,const QModelIndex & parent) const {

	const auto model = sourceModel();
	const auto index = model -> index(row,0,parent);

	return model -> data(index,filterRole()).toBool();
}

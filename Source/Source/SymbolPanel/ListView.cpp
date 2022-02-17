#include "symbollistview.h"
#include "symbollistmodel.h"
#include "Include/UtilsUI.hpp"


SymbolListView::SymbolListView(QWidget * parent) 
	: QListView(parent)
	, m_symbolSize(32)
	, m_gridSize(40) {

	setResizeMode(QListView::Adjust);
	setFrameShape(QFrame::NoFrame);
	setViewMode(QListView::IconMode);
	setWrapping(true);

	UtilsUi::enableTouchScrolling(this);
}


void SymbolListView::setSymbolSize(int size){

	m_symbolSize = size;
	m_gridSize = size + 8;

	setIconSize(QSize(m_symbolSize,m_symbolSize));
	setGridSize(QSize(m_gridSize,m_gridSize));
}


QSize SymbolListView::sizeHint() const {
	auto size = QListView::sizeHint();
	return QSize(size.width(),2 * m_gridSize);
}


QSize SymbolListView::minimumSizeHint() const {
	auto size = QListView::minimumSizeHint();
	return QSize(size.width(),m_gridSize);
}


void SymbolListView::contextMenuEvent(QContextMenuEvent * event){

	auto index = indexAt(event -> pos());
	
	if(!index.isValid())
		return;

	QMenu menu(this);


	const auto addAction = [ & ](const auto name,auto data,auto event){
		auto action = menu.addAction(tr(name));
		action -> setData(data);
		connect(action,SIGNAL(triggered()),this,event);
	};
	
	if(index.data(SymbolListModel::FavoriteRole).toBool())
		addAction(
			"Remove From Favorites",
			index.model() -> data(index,SymbolListModel::IdRole),
			SLOT(emitRemoveFromFavorites())
		);
	else
		addAction(
			"Add to Favorites",
			index.model() -> data(index,SymbolListModel::IdRole),
			SLOT(emitAddToFavorites())
		);
	
	if(index.data(SymbolListModel::CommandRole).toString().length())
		addAction(
			"Insert command",
			index.data(SymbolListModel::CommandRole),
			SLOT(emitInsertSymbol())
		);

	
	if(index.data(SymbolListModel::UnicodeRole).toString().length())
		addAction(
			"Insert unicode",
			index.data(SymbolListModel::UnicodeRole),
			SLOT(emitInsertSymbol())
		);

	menu.exec(event -> globalPos());
}


void SymbolListView::emitAddToFavorites(){

	auto action = qobject_cast<QAction *>(sender());
	
	if(action){
		QString id = action -> data().toString();
		emit addToFavorites(id);
	}
}


void SymbolListView::emitRemoveFromFavorites(){

	auto action = qobject_cast<QAction *>(sender());
	
	if(action){
		QString id = action -> data().toString();
		emit removeFromFavorites(id);
	}
}


void SymbolListView::emitInsertSymbol(){
	
	auto action = qobject_cast<QAction *>(sender());
	
	if(action){
		QString symbol = action -> data().toString();
		emit insertSymbol(symbol);
	}
}



#include "symbolwidget.h"
#include "symbollistmodel.h"
#include "symbollistview.h"
#include "proxymodels.h"
#include "configmanagerinterface.h"
#include "Include/UtilsUI.hpp"

#include <QSortFilterProxyModel>


SymbolWidget::SymbolWidget(SymbolListModel * model,bool & insertUnicode,QWidget * parent) 
	: QWidget(parent)
	, insertUnicode(insertUnicode) {
	
	setupData(model);

	auto layout = new QVBoxLayout();
	setLayout(layout);

	layout -> setContentsMargins(0,0,0,0);
	layout -> setSpacing(0);

	setupFavoritesArea(layout);
	addHLine(layout);
	setupMostUsedArea(layout);
	addHLine(layout);
	setupSearchArea(layout);

	setSymbolSize(32);
}


void SymbolWidget::setupData(SymbolListModel * model){

	categories = QStringList() 
		<< "operators" 
		<< "relation" 
		<< "arrows" 
		<< "delimiters" 
		<< "greek"
        << "cyrillic" 
		<< "misc-math" 
		<< "misc-text" 
		<< "wasysym" 
		<< "fontawesome5" 
		<< "special";

	categoryNames["operators"] = tr("Operators","Operator category");
	categoryNames["relation"] = tr("Relations","Operator category");
	categoryNames["arrows"] = tr("Arrows","Operator category");
	categoryNames["delimiters"] = tr("Delimiters","Operator category");
	categoryNames["greek"] = tr("Greek","Operator category");
	categoryNames["cyrillic"] = tr("Cyrillic","Operator category");
	categoryNames["misc-math"] = tr("Misc. Math","Operator category");
	categoryNames["misc-text"] = tr("Misc. Text","Operator category");
	categoryNames["wasysym"] = tr("wasysym","Operator category");
    categoryNames["fontawesome5"] = tr("fontawesome5","Operator category");
	categoryNames["special"] = tr("Special","Operator category");

	Q_ASSERT(categories.count() == categoryNames.count());

	symbolListModel = model;

	for(const auto & category : categories)
		symbolListModel -> load(category);

	favoritesProxyModel = new BooleanFilterProxyModel;
	favoritesProxyModel -> setSourceModel(symbolListModel);
	favoritesProxyModel -> setFilterRole(SymbolListModel::FavoriteRole);
	connect(symbolListModel,SIGNAL(favoritesChanged()),favoritesProxyModel,SLOT(invalidate()));

	mostUsedProxyModel = new MostUsedProxyModel;
	mostUsedProxyModel -> setSourceModel(symbolListModel);
	mostUsedProxyModel -> setSortRole(SymbolListModel::UsageCountRole);

	categoryFilterProxyModel = new QSortFilterProxyModel;
	categoryFilterProxyModel -> setSourceModel(symbolListModel);
	categoryFilterProxyModel -> setFilterRole(SymbolListModel::CategoryRole);
	categoryFilterProxyModel -> setFilterCaseSensitivity(Qt::CaseInsensitive);

	commandFilterProxyModel = new QSortFilterProxyModel;
	commandFilterProxyModel -> setSourceModel(categoryFilterProxyModel);
	commandFilterProxyModel -> setFilterRole(SymbolListModel::CommandRole);
	commandFilterProxyModel -> setFilterCaseSensitivity(Qt::CaseInsensitive);
}


void SymbolWidget::setupFavoritesArea(QVBoxLayout * vertical){

	auto horizontal = new QHBoxLayout;
	horizontal -> setContentsMargins(4,6,4,6);
	horizontal -> setSpacing(8);
	vertical -> addLayout(horizontal);

	horizontal -> addWidget(new QLabel(tr("Favorites")));

	addHLine(vertical);

	favoritesListView = new SymbolListView();
	favoritesListView -> setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Maximum);
	favoritesListView -> setModel(favoritesProxyModel);
	initSymbolListView(favoritesListView);
	vertical -> addWidget(favoritesListView);
}


void SymbolWidget::setupMostUsedArea(QVBoxLayout * vertical){

	auto horizontal = new QHBoxLayout;
	horizontal -> setContentsMargins(4,6,4,6);
	horizontal -> setSpacing(8);
	vertical -> addLayout(horizontal);

	horizontal -> addWidget(new QLabel(tr("Most Used")));

	addHLine(vertical);

	mostUsedListView = new SymbolListView();
	mostUsedListView -> setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
	mostUsedListView -> setModel(mostUsedProxyModel);
	initSymbolListView(mostUsedListView);
	vertical -> addWidget(mostUsedListView);
}


void SymbolWidget::setupSearchArea(QVBoxLayout * vertical){

	auto horizontal = new QHBoxLayout;
	horizontal -> setContentsMargins(4,2,4,2);
	horizontal -> setSpacing(8);
	vertical -> addLayout(horizontal);

	leFilter = new QLineEdit();
	leFilter -> setPlaceholderText(tr("Search"));
	horizontal -> addWidget(leFilter);

	categoryFilterButton = new QToolButton();
	categoryFilterButton -> setToolTip(tr("Category"));
	categoryFilterButton -> setPopupMode(QToolButton::InstantPopup);
	categoryFilterButton -> setAutoRaise(true);

	int width = 0;
	auto font = fontMetrics();

    for(const auto & name : categoryNames)
		width = qMax(width,UtilsUi::getFmWidth(font,name) + 20);

	categoryFilterButton -> setMinimumWidth(width);
	horizontal -> addWidget(categoryFilterButton);

    auto actAllCategories = new QAction(tr("All"),nullptr);  // does not need data
	connect(actAllCategories,SIGNAL(triggered()),this,SLOT(setCategoryFilterFromAction()));

	categoryFilterButton -> addAction(actAllCategories);
	
	bool isFirst = true;
	
	for(const auto & category : categories){

        auto action = new QAction(categoryNames[category],nullptr);
		categoryFilterButton -> addAction(action);
		action -> setData(category);

		connect(action,SIGNAL(triggered()),this,SLOT(setCategoryFilterFromAction()));

		if(isFirst){
			action -> trigger();  // initialize the title and category filter
			isFirst = false;
		}
	}

	connect(leFilter,SIGNAL(textChanged(QString)),commandFilterProxyModel,SLOT(setFilterFixedString(QString)));

	addHLine(vertical);

	symbolListView = new SymbolListView();
	symbolListView -> setModel(commandFilterProxyModel);
	initSymbolListView(symbolListView);
	vertical -> addWidget(symbolListView);
}


void SymbolWidget::initSymbolListView(SymbolListView *symbolListView){
	connect(symbolListView,SIGNAL(clicked(QModelIndex)),this, SLOT(symbolClicked(QModelIndex)));
	connect(symbolListView,SIGNAL(addToFavorites(QString)),symbolListModel,SLOT(addFavorite(QString)));
	connect(symbolListView,SIGNAL(removeFromFavorites(QString)),symbolListModel,SLOT(removeFavorite(QString)));
	connect(symbolListView,SIGNAL(insertSymbol(QString)),this,SIGNAL(insertSymbol(QString)));
}


void SymbolWidget::addHLine(QVBoxLayout * vertical){
	auto line = new QFrame();
	line -> setFrameShape(QFrame::HLine);
	vertical -> addWidget(line);
}


void SymbolWidget::setSymbolSize(int size){
	favoritesListView -> setSymbolSize(size);
	mostUsedListView -> setSymbolSize(size);
	symbolListView -> setSymbolSize(size);
}


void SymbolWidget::setCategoryFilterFromAction(){
	
	auto action = qobject_cast<QAction *>(sender());
	
	if(!action)
		return;
	
	categoryFilterButton -> setText(action -> text());
	categoryFilterProxyModel -> setFilterFixedString(action -> data().toString());
}


void SymbolWidget::symbolClicked(const QModelIndex & index){

	const auto dataOf = [ & ](int model) -> QString {
		return index.model() -> data(index,model).toString();
	};


	QString command;
	
	if(insertUnicode)
		command = dataOf(SymbolListModel::UnicodeRole);
	
	if(command.isEmpty())
		command = dataOf(SymbolListModel::CommandRole);
	

	const QString id = command = dataOf(SymbolListModel::IdRole);
	
	if(!id.isEmpty())
		symbolListModel -> incrementUsage(id);


	emit insertSymbol(command);

	mostUsedProxyModel -> invalidate();
	mostUsedProxyModel -> sort(0,Qt::DescendingOrder);
}


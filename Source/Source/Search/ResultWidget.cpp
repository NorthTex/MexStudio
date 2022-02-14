
#include "searchresultwidget.h"
#include "configmanagerinterface.h"
#include "Latex/Document.hpp"


SearchResultWidget::SearchResultWidget(QWidget * widget) 
	: QWidget(widget)
	, query(nullptr) {

	query = new SearchQuery("","",SearchQuery::NoFlags);

	auto config = ConfigManagerInterface::getInstance();

	auto editorFontFamily = config
		-> getOption("Editor/Font Family")
		.  toString();

	auto searchDelegate = new SearchTreeDelegate(editorFontFamily,this);

	auto hLayout = new QHBoxLayout;
	hLayout -> setContentsMargins(4,2,4,1);
	hLayout -> setSpacing(8);

	searchScopeBox = new QComboBox;
	searchScopeBox -> setEditable(false);
	searchScopeBox -> addItem(tr("Current Doc"),static_cast<uint>(SearchQuery::CurrentDocumentScope));
	searchScopeBox -> addItem(tr("All Docs"),static_cast<uint>(SearchQuery::GlobalScope));
	searchScopeBox -> addItem(tr("Project"),static_cast<uint>(SearchQuery::ProjectScope));
	searchScopeBox -> setCurrentIndex(config -> getOption("Search/ScopeIndex").toInt());
	connect(searchScopeBox,SIGNAL(currentIndexChanged(int)),SLOT(updateSearch()));

	searchTypeLabel = new QLabel;
	searchTextLabel = new QLabel;

	auto font = searchTextLabel -> font();
	font.setItalic(true);

	searchTextLabel -> setFont(font);
    
	searchAgainButton = new QPushButton(tr("Update Search"));
	connect(searchAgainButton,SIGNAL(clicked()),this,SLOT(updateSearch()));
	
	replaceTextEdit = new QLineEdit;
	replaceTextEdit -> setClearButtonEnabled(true);
	
	replaceButton = new QPushButton(tr("Replace all"));

	hLayout -> addWidget(searchScopeBox);
	hLayout -> addWidget(searchTypeLabel);
	hLayout -> addWidget(searchTextLabel,1);
	hLayout -> addWidget(searchAgainButton);
	hLayout -> addWidget(new QLabel(tr("Replace by:")));
	hLayout -> addWidget(replaceTextEdit,1);
	hLayout -> addWidget(replaceButton);

	searchTree = new QTreeView(this);
	searchTree -> header() -> hide();
	searchTree -> setUniformRowHeights(true);
	searchTree -> setItemDelegate(searchDelegate);
	searchTree -> setFrameShape(QFrame::NoFrame);

	auto vLayout = new QVBoxLayout();
	setLayout(vLayout);
	vLayout -> setContentsMargins(0,0,0,0);
	vLayout -> setSpacing(0);
	vLayout -> addLayout(hLayout);

	auto hLine = new QFrame();
	hLine -> setFrameShape(QFrame::HLine);
	vLayout -> addWidget(hLine);
	vLayout -> addWidget(searchTree,1);

	auto actExpand = new QAction(tr("Expand All"),this);
	connect(actExpand, SIGNAL(triggered()),searchTree,SLOT(expandAll()));
	searchTree->addAction(actExpand);
	
	auto actCollapse = new QAction(tr("Collapse All"),this);
	connect(actCollapse,SIGNAL(triggered()),searchTree,SLOT(collapseAll()));
	searchTree -> addAction(actCollapse);
	
	auto actClear = new QAction(tr("Clear"),this);
	connect(actClear,SIGNAL(triggered()),this,SLOT(clearSearch()));
	searchTree -> addAction(actClear);

	searchTree -> setContextMenuPolicy(Qt::ActionsContextMenu);

	connect(searchTree,SIGNAL(clicked(QModelIndex)),this,SLOT(clickedSearchResult(QModelIndex)));
}


/*!
 * The widget takes ownership of the result.
 * It will be destoyed when a new SearchResult is set
 */

void SearchResultWidget::setQuery(SearchQuery * query){

	delete this -> query;
	this -> query = query;

	query -> setParent(this);

	const auto replacement = query -> replacementText();
	const auto search = query -> searchExpression();
	const auto scope = query -> scope();
	const auto model = query -> model();
	const auto type = query -> type() + ":";

	const bool repeatableSearch = query -> flag(SearchQuery::SearchAgainAllowed);
	const bool changableScope = query -> flag(SearchQuery::ScopeChangeAllowed);
	const bool replacable = query -> flag(SearchQuery::ReplaceAllowed);

	searchTypeLabel -> setText(type);
	searchTextLabel -> setText(search);
	searchScopeBox -> setEnabled(changableScope);
	
	updateSearchScopeBox(scope);
	
	searchAgainButton -> setEnabled(repeatableSearch);
	replaceTextEdit -> setEnabled(replacable);
	replaceTextEdit -> setText(replacement);
	replaceButton -> setEnabled(replacable);
	searchTree -> setModel(model);
	
	connect(replaceTextEdit,SIGNAL(textChanged(QString)),query,SLOT(setReplacementText(QString)));
	connect(replaceTextEdit,SIGNAL(returnPressed()),query,SLOT(replaceAll()));
	connect(replaceButton,SIGNAL(clicked()),query,SLOT(replaceAll()));
	connect(query,SIGNAL(runCompleted()),this,SLOT(searchCompleted()));
}


void SearchResultWidget::updateSearch(){
	
	if(query)
		query -> setScope(searchScope());
	
	emit runSearch(query);
}


void SearchResultWidget::searchCompleted(){

	// only one top-level element. i.e. file

	if(!query)
		return;

	const auto rows = query 
		-> model() 
		-> rowCount(QModelIndex());

	if(rows == 1)
		searchTree -> expandAll();
}


void SearchResultWidget::updateSearchExpr(QString query){
	searchTextLabel -> setText(query);
}


void SearchResultWidget::saveConfig(){
	ConfigManagerInterface::getInstance()
		-> setOption("Search/ScopeIndex",searchScopeBox -> currentIndex());
}


void SearchResultWidget::updateSearchScopeBox(SearchQuery::Scope scope){

	const int index = searchScopeBox 
		-> findData(scope);
	
	if(index < 0)
		return;
	
	searchScopeBox -> setCurrentIndex(index);
}


void SearchResultWidget::clickedSearchResult(const QModelIndex & index){

	const auto document = query
		-> model()
		-> getDocument(index);

	if(!document)
		return;

	const int line = query
		-> model()
		-> getLineNumber(index);

	if(line < 0)
		return;

	emit jumpToSearchResult(document,line,query);
}


void SearchResultWidget::clearSearch(){
	setQuery(new SearchQuery("","",SearchQuery::NoFlags));
}


SearchQuery::Scope SearchResultWidget::searchScope() const {

	const auto index = searchScopeBox -> currentIndex();

	const auto data = searchScopeBox 
		-> itemData(index)
		.  toUInt();

	return static_cast<SearchQuery::Scope>(data);
}


//====================================================================
// CustomDelegate for search results
//====================================================================


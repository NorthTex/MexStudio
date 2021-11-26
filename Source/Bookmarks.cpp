#include "Bookmarks.hpp"
#include "latexdocument.h"
#include "latexeditorview.h"



Bookmarks::Bookmarks(const LatexDocuments * documents,QObject * parent)
	: QObject(parent)
	, documents(documents)
	, m_darkMode(false){

	initializeWidget();
}


void Bookmarks::initializeWidget(){

	bookmarksWidget = new QListWidget();
	bookmarksWidget -> setAlternatingRowColors(true);
	bookmarksWidget -> setStyleSheet(
	    "QListWidget::item {"
	    "padding: 4px;"
	    "border-bottom: 1px solid palette(dark); }"
	    "QListWidget::item:selected {"
	    "color: palette(highlighted-text);"
	    "background-color: palette(highlight); }");

	connect(bookmarksWidget,SIGNAL(itemPressed(QListWidgetItem*)),SLOT(clickedOnBookmark(QListWidgetItem*))); //single click

	UtilsUi::enableTouchScrolling(bookmarksWidget);

	createContextMenu();
}


void Bookmarks::addWidgetAction(const char * label,std::function<void()> f){
	auto action = new QAction(tr(label),bookmarksWidget);
	connect(action,SIGNAL(triggered()),SLOT(f));
	bookmarksWidget -> addAction(action);
}

void Bookmarks::createContextMenu(){

	bookmarksWidget -> setContextMenuPolicy(Qt::ActionsContextMenu);

	QAction * action;

	action = new QAction(tr("Move Up"),bookmarksWidget);
	connect(action,SIGNAL(triggered()),SLOT(moveBookmarkUp()));
	bookmarksWidget -> addAction(action);

	action = new QAction(tr("Move Down"),bookmarksWidget);
	connect(action,SIGNAL(triggered()),SLOT(moveBookmarkDown()));
	bookmarksWidget -> addAction(action);

	action = new QAction(tr("Remove"),bookmarksWidget);
	connect(action,SIGNAL(triggered()),SLOT(removeBookmark()));
	bookmarksWidget -> addAction(action);

	action = new QAction(tr("Remove All"),bookmarksWidget);
	connect(action,SIGNAL(triggered()),SLOT(removeAllBookmarks()));
	bookmarksWidget -> addAction(action);
}


void Bookmarks::setBookmarks(const QList<Bookmark> & bookmarks){

	bookmarksWidget -> clear();

	for(const auto & bookmark : bookmarks){

		auto item = new QListWidgetItem(bookmark.label,bookmarksWidget);
		item -> setData(FileName,bookmark.path);
		item -> setData(LineNr,bookmark.line);
		item -> setData(BookmarkNr,bookmark.id);

		auto document = documents -> findDocumentFromName(bookmark.path);

		if(document && bookmark.line < (document -> lineCount()) && bookmark.line >= 0){
			auto handle = document -> line(bookmark.line).handle();
			item -> setData(DocLineHandle,QVariant::fromValue(handle));
		} else {
			item -> setData(DocLineHandle,0);
		}
	}
}


QList<Bookmark> Bookmarks::getBookmarks(){

	QList<Bookmark> bookmarks;

	for(int i = 0;i < bookmarksWidget -> count();i++){

		Bookmark bookmark;

		auto item = bookmarksWidget -> item(i);
		auto path = item -> data(FileName).toString();

		int line = item -> data(LineNr).toInt();
		int bookmarkNumber = item -> data(BookmarkNr).toInt();

		auto handle = qvariant_cast<QDocumentLineHandle *>(item -> data(DocLineHandle));
		auto document = documents -> findDocumentFromName(path);

		if(document){

			int temp = document -> indexOf(handle);

			if(temp >= 0)
				line = temp;
		}

		bookmark.label = item -> text();
		bookmark.path = path;
		bookmark.line = line;
		bookmark.id = bookmarkNumber;

		bookmarks << bookmark;
	}

	return bookmarks;
}


void Bookmarks::bookmarkDeleted(QDocumentLineHandle * handle){

	QString text = handle -> document() -> getFileInfo().fileName();
	auto items = bookmarksWidget -> findItems(text,Qt::MatchStartsWith);

	for(auto item : items){

		auto linehandle = qvariant_cast<QDocumentLineHandle *>(item -> data(DocLineHandle));

		if(linehandle != handle)
			continue;

		int row = bookmarksWidget -> row(item);
		bookmarksWidget -> takeItem(row);

		return;
	}
}


void Bookmarks::bookmarkAdded(QDocumentLineHandle * handle,int bookmark){

	auto document = handle -> document();

	QString text = document -> getFileInfo().fileName();
	text += "\n" + handle -> text().trimmed();

	auto item = new QListWidgetItem(text,bookmarksWidget);
	item -> setData(FileName, document -> getFileName());
	item -> setData(LineNr, document -> indexOf(handle));
	item -> setData(DocLineHandle,QVariant::fromValue(handle));
	item -> setData(BookmarkNr,bookmark);

	int line = document -> indexOf(handle);
	line = (line > 1) ? line - 2 : 0;

	item -> setToolTip(document -> exportAsHtml(document -> cursor(line,0,line + 4),true,true,60));
}


void Bookmarks::updateLineWithBookmark(int line){

	auto document = qobject_cast<LatexDocument *>(sender());

	if(!document)
		return;

	QString text = document -> getFileInfo().fileName();
	auto handle = document -> line(line).handle();

    if(!handle)
        return;

    auto items = bookmarksWidget -> findItems(text,Qt::MatchStartsWith);

	for(auto item : items){

		auto linehandle = qvariant_cast<QDocumentLineHandle *>(item -> data(DocLineHandle));

		if(linehandle != handle)
			continue;
		
		QString text = document -> getFileInfo().fileName();
		text += '\n' + handle -> text().trimmed();
		item -> setText(text);
		line = (line > 1) ? line - 2 : 0;
		item -> setToolTip(document -> exportAsHtml(document -> cursor(line,0,line + 4),true,true,60));

		return;
	}
}


void Bookmarks::restoreBookmarks(LatexEditorView * view){

	Q_ASSERT(view);

	auto document = view -> document;

	if(!document)
		return;

	// go trough bookmarks

	for(int i = 0;i < bookmarksWidget -> count();i++){

		auto item = bookmarksWidget -> item(i);
		auto fn = item -> data(FileName).toString();

		if(document -> getFileName() != fn)
			continue;

		int line = item -> data(LineNr).toInt();
		int id = item -> data(BookmarkNr).toInt();

		view -> addBookmark(line,id);

		auto handle = document -> line(line).handle();

		if(!handle)
			continue;

		item -> setData(DocLineHandle,QVariant::fromValue(handle));
		item -> text() = handle -> text();
		item -> setToolTip(document -> exportAsHtml(document -> cursor(line,0,line + 4),true,true,60));

	}
}


void Bookmarks::updateBookmarks(LatexEditorView * view){

	if(!view)
		return;

	auto document = view -> document;
	QString text = document -> getFileInfo().fileName();
	auto items = bookmarksWidget -> findItems(text, Qt::MatchStartsWith);

	for(auto item : items){

		auto handle = qvariant_cast<QDocumentLineHandle *>(item -> data(DocLineHandle));

		if(!handle)
			continue;

		if(text.isEmpty()){
			if(handle -> document() == document){
				int row = bookmarksWidget -> row(item);

				if(row < 0)
					continue;

				bookmarksWidget -> takeItem(row);
			}
		} else {
			int line = document -> indexOf(handle);
            item -> setData(LineNr,line);
            item -> setData(DocLineHandle,0);
		}
	}
}


const char * const style_dark =
    "QListWidget {alternate-background-color: #202040;}"
    "QListWidget::item {"
    "padding: 4px;"
    "border-bottom: 1px solid palette(dark); }"
    "QListWidget::item:selected {"
    "color: palette(highlighted-text);"
    "background-color: palette(highlight); }";

const char * const style_light =
    "QListWidget::item {"
    "padding: 4px;"
    "border-bottom: 1px solid palette(dark); }"
    "QListWidget::item:selected {"
    "color: palette(highlighted-text);"
    "background-color: palette(highlight); }";

void Bookmarks::setDarkMode(bool useDark){

    m_darkMode = useDark;

    bookmarksWidget -> setStyleSheet(useDark ? style_dark : style_light);
}


/*** context menu ***/

void Bookmarks::clickedOnBookmark(QListWidgetItem * item){

	if(QApplication::mouseButtons() == Qt::RightButton)
		return; // avoid jumping to line if contextmenu is called

	QString fn = item -> data(FileName).toString();
	int line = item -> data(LineNr).toInt();

	auto handle = qvariant_cast<QDocumentLineHandle *>(item -> data(DocLineHandle));

	auto document = documents -> findDocumentFromName(fn);

	if(!document){
		emit loadFileRequest(fn);
		document = documents -> findDocumentFromName(fn);

		if(!document)
			return;
	}

	int ln = document -> indexOf(handle);

	if(ln < 0){
		handle = document -> line(line).handle();
		item -> setData(DocLineHandle,QVariant::fromValue(handle));
	} else { // linenr in case it has been shifted
        line = ln;
    }

    emit gotoLineRequest(line,0,document -> getEditorView());
}


void Bookmarks::moveBookmarkUp(){

	auto item = bookmarksWidget -> currentItem();

	if(!item)
		return;

	int row = bookmarksWidget -> row(item);

	if(row <= 0)
		return;

	bookmarksWidget -> takeItem(row);
	bookmarksWidget -> insertItem(row - 1,item);
	bookmarksWidget -> setCurrentRow(row - 1);
}


void Bookmarks::moveBookmarkDown(){

	QListWidgetItem * item = bookmarksWidget -> currentItem();

	if(!item)
		return;

	int row = bookmarksWidget -> row(item);

	if(row < 0)
		return;

	if(row == bookmarksWidget -> count() - 1)
		return;

	bookmarksWidget -> takeItem(row);
	bookmarksWidget -> insertItem(row + 1, item);
	bookmarksWidget -> setCurrentRow(row + 1);
}


void Bookmarks::removeBookmark(){

	int row = bookmarksWidget -> currentRow();

	if(row < 0)
		return;

	QListWidgetItem * item = bookmarksWidget -> takeItem(row);
	QString fn = item -> data(FileName).toString();

	int lineNr = item -> data(LineNr).toInt();
	int bookmarkNumber = item -> data(BookmarkNr).toInt();

	QDocumentLineHandle * handle = qvariant_cast<QDocumentLineHandle *>(item -> data(DocLineHandle));

	LatexDocument * document = documents -> findDocumentFromName(fn);

	if(!document)
		return;

	auto view = document -> getEditorView();

	if(handle){
		view -> removeBookmark(handle,bookmarkNumber);
	} else {
		view -> removeBookmark(lineNr,bookmarkNumber);
	}
}


void Bookmarks::removeAllBookmarks(){

	while(bookmarksWidget -> count() > 0){

		auto item = bookmarksWidget -> takeItem(0);
		QString path = item -> data(FileName).toString();

		int line = item -> data(LineNr).toInt();
		int id = item -> data(BookmarkNr).toInt();

		auto handle = qvariant_cast<QDocumentLineHandle *>(item -> data(DocLineHandle));

		auto document = documents -> findDocumentFromName(path);

		if(!document)
			continue;

		auto view = document -> getEditorView();

		if(handle){
			view -> removeBookmark(handle,id);
		} else {
			view -> removeBookmark(line,id);
		}
	}
}

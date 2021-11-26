#include "Bookmarks.hpp"
#include "latexdocument.h"
#include "latexeditorview.h"



Bookmark::Bookmark()
	: bookmarkNumber(-1)
	, lineNumber(0) {}


Bookmark Bookmark::fromStringList(QStringList list){

	Bookmark bookmark;

	if(!list.isEmpty())
		bookmark.filename = list.takeFirst();

	if(!list.isEmpty())
		bookmark.lineNumber = list.takeFirst().toInt();

	if (!list.isEmpty()) {

		bool ok;
		int n = list.first().toInt(& ok);

		if(ok){
			bookmark.bookmarkNumber = n;
			list.removeFirst();
		}
	}

	if(!list.isEmpty())
		bookmark.text = list.takeFirst();

	return bookmark;
}


QStringList Bookmark::toStringList() const {

	QStringList slist;

	slist << filename;
	slist << QString::number(lineNumber);
	slist << QString::number(bookmarkNumber);
	slist << text;

	return slist;
}


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


void Bookmarks::createContextMenu(){

	bookmarksWidget -> setContextMenuPolicy(Qt::ActionsContextMenu);

	QAction * action = new QAction(tr("Move Up"),bookmarksWidget);
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

	foreach (const Bookmark & bookmark,bookmarks){

		QListWidgetItem * item = new QListWidgetItem(bookmark.text,bookmarksWidget);
		item -> setData(FileName,bookmark.filename);
		item -> setData(LineNr,bookmark.lineNumber);
		item -> setData(BookmarkNr,bookmark.bookmarkNumber);

		LatexDocument * document = documents -> findDocumentFromName(bookmark.filename);

		if(document && bookmark.lineNumber < document -> lineCount() && bookmark.lineNumber >= 0){
			QDocumentLineHandle * handle = document -> line( bookmark.lineNumber).handle();
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
		QListWidgetItem * item = bookmarksWidget -> item(i);
		QString path = item -> data(FileName).toString();

		int line = item -> data(LineNr).toInt();
		int bookmarkNumber = item -> data(BookmarkNr).toInt();

		QDocumentLineHandle * handle = qvariant_cast<QDocumentLineHandle *>(item -> data(DocLineHandle));
		LatexDocument * document = documents -> findDocumentFromName(path);

		if(document){

			int temp = document -> indexOf(handle);

			if(temp >= 0)
				line = temp;
		}

		bookmark.filename = path;
		bookmark.lineNumber = line;
		bookmark.bookmarkNumber = bookmarkNumber;
		bookmark.text = item -> text();

		bookmarks << bookmark;
	}

	return bookmarks;
}


void Bookmarks::bookmarkDeleted(QDocumentLineHandle * handle){

	QString text = handle -> document() -> getFileInfo().fileName();
	QList<QListWidgetItem *> items = bookmarksWidget -> findItems(text,Qt::MatchStartsWith);

	foreach (QListWidgetItem * item,items){

		QDocumentLineHandle * lineHandle = qvariant_cast<QDocumentLineHandle *>(item->data(DocLineHandle));

		if(lineHandle != handle)
			continue;

		int row = bookmarksWidget -> row(item);
		bookmarksWidget -> takeItem(row);
		return;
	}
}


void Bookmarks::bookmarkAdded(QDocumentLineHandle * handle,int bookmark){

	QDocument * document = handle -> document();

	QString text = document -> getFileInfo().fileName();
	text += "\n" + handle -> text().trimmed();

	QListWidgetItem * item = new QListWidgetItem(text,bookmarksWidget);
	item -> setData(FileName, document -> getFileName());
	item -> setData(LineNr, document -> indexOf(handle));
	item -> setData(DocLineHandle,QVariant::fromValue(handle));
	item -> setData(BookmarkNr,bookmark);

	int line = document -> indexOf(handle);
	line = (line > 1) ? line - 2 : 0;

	item -> setToolTip(document -> exportAsHtml(document -> cursor(line,0,line + 4),true,true,60));
}


void Bookmarks::updateLineWithBookmark(int line){

	LatexDocument * document = qobject_cast<LatexDocument *>(sender());

	if(!document)
		return;

	QString text = document -> getFileInfo().fileName();
	QDocumentLineHandle * handle = document -> line(line).handle();

    if(!handle)
        return;

    QList<QListWidgetItem *> items = bookmarksWidget -> findItems(text, Qt::MatchStartsWith);

    foreach (QListWidgetItem * item,items){

        QDocumentLineHandle * lineHandle = qvariant_cast<QDocumentLineHandle *>(item -> data(DocLineHandle));

		if(lineHandle != handle)
			continue;

		QString text = document -> getFileInfo().fileName();
		text += "\n" + handle -> text().trimmed();
		item -> setText(text);
		line = (line > 1) ? line - 2 : 0;
		item -> setToolTip(document -> exportAsHtml(document -> cursor(line,0,line + 4),true,true,60));

		return;
	}
}


void Bookmarks::restoreBookmarks(LatexEditorView * view){

	Q_ASSERT(view);

	LatexDocument * document = view -> document;

	if(!document)
		return;

	// go trough bookmarks
	for (int i = 0; i < bookmarksWidget -> count(); i++){

		QListWidgetItem *item = bookmarksWidget -> item(i);
		QString fn = item->data(FileName).toString();

		if(document -> getFileName() != fn)
			continue;

		int lineNr = item -> data(LineNr).toInt();
		int bookmarkNumber = item->data(BookmarkNr).toInt();

		view -> addBookmark(lineNr,bookmarkNumber);

		QDocumentLineHandle * handle = document -> line(lineNr).handle();

		if(!handle)
			continue;

		item -> setData(DocLineHandle,QVariant::fromValue(handle));
		item -> text() = handle -> text();
		item -> setToolTip(document -> exportAsHtml(document -> cursor(lineNr,0,lineNr + 4),true,true,60));
	}
}


void Bookmarks::updateBookmarks(LatexEditorView * view){

	if(!view)
		return;

	LatexDocument * document = view -> document;
	QString text = document -> getFileInfo().fileName();
	QList<QListWidgetItem *> items = bookmarksWidget -> findItems(text, Qt::MatchStartsWith);

	foreach (QListWidgetItem * item,items){

		QDocumentLineHandle * handle = qvariant_cast<QDocumentLineHandle *>(item -> data(DocLineHandle));

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
            int lineNr = document -> indexOf(handle);
            item -> setData(LineNr,lineNr);
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
	int lineNr = item -> data(LineNr).toInt();

	QDocumentLineHandle * handle = qvariant_cast<QDocumentLineHandle *>(item -> data(DocLineHandle));

	LatexDocument * document = documents -> findDocumentFromName(fn);

	if(!document){
		emit loadFileRequest(fn);
		document = documents -> findDocumentFromName(fn);

		if(!document)
			return;
	}

	int ln = document -> indexOf(handle);

	if(ln < 0){
		handle = document -> line(lineNr).handle();
		item -> setData(DocLineHandle,QVariant::fromValue(handle));
	} else { // linenr in case it has been shifted
        lineNr=ln;
    }

    emit gotoLineRequest(lineNr,0,document -> getEditorView());
}


void Bookmarks::moveBookmarkUp(){

	QListWidgetItem * item = bookmarksWidget -> currentItem();

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

	LatexEditorView * view = document -> getEditorView();

	if(handle){
		view -> removeBookmark(handle,bookmarkNumber);
	} else {
		view -> removeBookmark(lineNr,bookmarkNumber);
	}
}


void Bookmarks::removeAllBookmarks(){

	while(bookmarksWidget -> count() > 0){

		QListWidgetItem * item = bookmarksWidget -> takeItem(0);
		QString path = item -> data(FileName).toString();

		int line = item -> data(LineNr).toInt();
		int bookmarkNumber = item -> data(BookmarkNr).toInt();

		QDocumentLineHandle * handle = qvariant_cast<QDocumentLineHandle *>(item -> data(DocLineHandle));

		LatexDocument * document = documents -> findDocumentFromName(path);

		if(!document)
			continue;

		LatexEditorView * view = document -> getEditorView();

		if(handle){
			view -> removeBookmark(handle,bookmarkNumber);
		} else {
			view -> removeBookmark(line,bookmarkNumber);
		}
	}
}

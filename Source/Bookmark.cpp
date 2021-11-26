
#include "Bookmark.hpp"


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
#ifndef Header_Bookmark
#define Header_Bookmark

#include <QStringList>


struct Bookmark {

	Bookmark();
	
	static Bookmark fromStringList(QStringList slist);
	QStringList toStringList() const;
	
	QString filename;
	QString text;

	int bookmarkNumber;
	int lineNumber;
};


#endif


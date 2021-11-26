#ifndef Header_Bookmark
#define Header_Bookmark

#include <QStringList>


struct Bookmark {

	public:

		QString label , path;
		int id , line;

	public:

		Bookmark();

	public:

		QStringList toStringList() const;

	public:

		static Bookmark fromStringList(QStringList slist);

};


#endif


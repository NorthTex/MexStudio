
#include "Bookmark.hpp"


Bookmark::Bookmark()
	: id(-1)
	, line(0) {}


/**
 * @brief Creates a bookmark object from a list of strings.
 * @details Uses the strings in the following order:
 *          - Path to the file the bookmark refers to
 *          - Line number the bookmark points to
 *          - The bookmarks id
 *          - The label of the bookmark
 * 
 *          If there are less strings than fields the
 *          process will prematurely return the object.
 * 
 * @param list A list of strings making up the bookmark. 
 * @return A new bookmark object made from the given strings.
 */

Bookmark Bookmark::fromStringList(QStringList list){

	Bookmark bookmark;

    #define returnIfEmpty \
        if(list.isEmpty()) \
            return bookmark;

    // if(list.isEmpty())
    //     return bookmark;

    returnIfEmpty

    bookmark.path = list.takeFirst();

	// if(list.isEmpty())
    //     return bookmark;

    returnIfEmpty
    
    bookmark.line = list.takeFirst().toInt();

    // if(list.isEmpty())
    //     return bookmark;

    returnIfEmpty

    bool ok;
    int n = list.first().toInt(& ok);

    if(ok){
        bookmark.id = n;
        list.removeFirst();
    }

    // if(list.isEmpty())
    //     return bookmark;

    returnIfEmpty

    bookmark.label = list.takeFirst();

	return bookmark;
    #undef returnIfEmpty
}


QStringList Bookmark::toStringList() const {
    return {
        path,
        QString::number(line),
        QString::number(id),
        label
    };

//	QStringList list;

//	list << path;
//	list << QString::number(line);
//	list << QString::number(id);
//	list << label;

//	return list;
}

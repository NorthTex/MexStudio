#ifndef Header_Bookmarks
#define Header_Bookmarks

#include "mostQtHeaders.h"
#include "Bookmark.hpp"

#include <functional>

#include "Latex/EditorView.hpp"
#include "Latex/Document.hpp"



class Bookmarks : public QObject {

	Q_OBJECT

    private:

        enum DataRole {
            FileName = Qt::UserRole,
            LineNr,
            DocLineHandle,
            BookmarkNr
        };

    private:

        const LatexDocuments * documents;
        QListWidget * bookmarksWidget;

        bool m_darkMode;
        
    private:

        void initializeWidget();
        void createContextMenu();

        void addWidgetAction(const char * label,std::function<void()>);

	public:

		Bookmarks(const LatexDocuments * docs,QObject * parent = nullptr);

        void setBookmarks(const QList<Bookmark> & bookmarkList);
        QList<Bookmark> getBookmarks();

        QListWidget * widget(){
            return bookmarksWidget;
        }


    signals:

        void gotoLineRequest(int lineNr,int col,LatexEditorView * edView);
        void loadFileRequest(const QString & fileName);


    public slots:

        void updateLineWithBookmark(int line);
        void bookmarkDeleted(QDocumentLineHandle * handle);
        void bookmarkAdded(QDocumentLineHandle * handle,int nr);

        void restoreBookmarks(LatexEditorView * view);
        void updateBookmarks(LatexEditorView * view);
        void setDarkMode(bool useDark);


    protected slots:

        void clickedOnBookmark(QListWidgetItem * item);

        void moveBookmarkUp();
        void moveBookmarkDown();

        void removeBookmark();
        void removeAllBookmarks();

};


#endif
